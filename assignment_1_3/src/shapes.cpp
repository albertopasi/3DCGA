#include "shapes.h"

#include <glm/glm.hpp>
#include <algorithm> 

#include <array>

#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"

//THIS FILE ONLY CONTAINS HELPER FUNCTION, YOU DO NOT HAVE TO TOUCH THIS.

//Regex swap R and B to fix saves from te original paper (?<=R\=\")(\d+)(?=\")(.*)(?<=B\=\")(\d+)(?=\") , substitution $3$2$1

//Forward declarations for helper functions
void pushColor(rapidxml::xml_node<>* color_node, std::vector<glm::uvec2>& ind, std::vector<float>& color_u, std::vector<glm::vec3>& color);
void linearize_bezier_curve_helper(BezierCurve curve, float tolerance, std::vector<Line>& lines, int max_depth);
std::array<BezierCurve, 2> split_curve(BezierCurve curve, float alpha);
bool is_curve_flat(BezierCurve curve, float tolerance);

void load_Bezier_curves(std::vector<BezierCurve>& curves, const char* path, glm::ivec2 resolution) {
	//rapidxml boilerplate
	rapidxml::file<> xmlFile(path);
	rapidxml::xml_document<> doc;
	doc.parse<0>(xmlFile.data());
	rapidxml::xml_node<>* curve_set = doc.first_node();
	rapidxml::xml_node<>* set_node;
	rapidxml::xml_node<>* current_node;

	//The image_size the XML requests, used to fit the image to the actual resolution
	glm::uvec2 image_size = { std::atoi(curve_set->first_attribute("image_width")->value()) ,std::atoi(curve_set->first_attribute("image_height")->value()) };

	//Control point info
	std::vector<glm::vec2> vertices = {};					//The control points for all curves
	std::vector<unsigned int> curve_map = {};				//Curve number for each segment
	std::vector<unsigned int> curve_index = {};				//Segment number in each curve per segment

	//Color info
	std::vector<glm::uvec2> color_left_index{};				//for each curve starting index (x) and color control point count (y) in other arrays
	std::vector<glm::vec3> color_left = {};					//Color for each color_control point
	std::vector<float> color_left_u = {};					//curve parameter for each color control point

	//Same as left but for right
	std::vector<glm::uvec2> color_right_index{};
	std::vector<glm::vec3> color_right = {};
	std::vector<float> color_right_u = {};


	//Initialize counters
	int current_curve = 0;
	int current_curve_segment = 0;

	unsigned int n_colors_left = 0;
	unsigned int n_colors_right = 0;
	int n_segments = 0;

	//Read Diffusion curves
	for (rapidxml::xml_node<>* curve = curve_set->first_node(); curve; curve = curve->next_sibling()) {
		current_curve_segment = 0;
		set_node = curve->first_node("control_points_set", 18);

		current_node = set_node->first_node();

		//Loop over all splines
		while (current_node->next_sibling()) {
			//Insert the 4 vertexes of the spline
			for (int i = 0; i < 3; i++) {
				vertices.push_back({
					(float)std::atof((current_node->first_attribute("x", 1))->value()) / image_size.x * resolution.x,
					(float)std::atof((current_node->first_attribute( "y", 1))->value()) / image_size.y * resolution.y,
					});
				current_node = current_node->next_sibling();
			}

			//Repeat for last control point
			vertices.push_back({
				(float)std::atof((current_node->first_attribute("x", 1))->value()) / image_size.x * resolution.x,
				(float)std::atof((current_node->first_attribute("y", 1))->value()) / image_size.y * resolution.y,
				});

			//Push the gathered info in the buffers
			curve_map.push_back(current_curve);
			curve_index.push_back(current_curve_segment++);
		}

		//Set the current node to the left color nodes
		set_node = curve->first_node("left_colors_set", 15);
		color_left_index.push_back({ n_colors_left ,0 });
		current_node = set_node->first_node();


		//Read all the left colors
		while (current_node) {
			pushColor(current_node, color_left_index, color_left_u, color_left);
			current_node = current_node->next_sibling();
		}


		//set the current node to the right color nodes
		set_node = curve->first_node("right_colors_set", 16);
		color_right_index.push_back({ n_colors_right ,0 });
		current_node = set_node->first_node();

		//Read all the right colors
		while (current_node) {
			pushColor(current_node, color_right_index, color_right_u, color_right);
			current_node = current_node->next_sibling();
		}

		//Update the number of colors in the segment
		n_colors_left += color_left_index.back().y;
		n_colors_right += color_right_index.back().y;

		current_curve++;
		n_segments += current_curve_segment;
	}

	//Find all non-integer color control points, we will split the curve at these points so each curve only has a color control point at the start and end
	std::vector<std::vector<float>> split_points(current_curve);
	//Go over all strings of curves
	for (int curve_ind = 0; curve_ind < current_curve; curve_ind++) {
		//Start with the colors on the left side 
		glm::ivec2 curve_color_start = color_left_index[curve_ind];
		//Loop over all color control points of this curve
		int color_ind = curve_color_start.x;
		while (color_ind < curve_color_start.x + curve_color_start.y) {
			//If the control point is not integer (not at the boundry between the curves) add it to the points we need to split at
			if (std::floor(color_left_u[color_ind]) != color_left_u[color_ind]) {
				split_points[curve_ind].push_back(color_left_u[color_ind]);
			}
			color_ind++;
		}
		
		//Do the same for the right control points
		curve_color_start = color_right_index[curve_ind];
		color_ind = curve_color_start.x;
		while (color_ind < curve_color_start.x + curve_color_start.y) {
			//Instead of just integer we also check if the point is alread in the list and skip if so
			if (std::floor(color_right_u[color_ind]) != color_right_u[color_ind] && std::find(split_points[curve_ind].begin(), split_points[curve_ind].end(), color_right_u[color_ind]) == split_points[curve_ind].end()) {
				split_points[curve_ind].push_back(color_right_u[color_ind]);
			}
			color_ind++;
		}
		//Sort the list so all split points are in order
		std::sort(split_points[curve_ind].begin(), split_points[curve_ind].end());
	}

	//Actually split the curves
	int id = 0;
	int split_point_ind = 0;
	int prev_curve_id = -1;
	//go over all segments
	for (int i = 0; i < n_segments; i++) {
		//Location for the split up curves for this segment, it already contains 1 curve which is the unsplit curve, with potentially wrong color
		std::vector<BezierCurve> segment_split_curves = std::vector<BezierCurve>(1);
		 
		//Lookup curve string invo
		int curve_id = curve_map[i];
		int curve_segment_id = curve_index[i];

		//If this is a new curve string reset the the index of the split points
		if (prev_curve_id != curve_id) {
			split_point_ind = 0;
			prev_curve_id = curve_id; 
		}

		//Find the color bounds, and set the color indexes to the minimum value
		glm::ivec2 left_color_bounds = color_left_index[curve_id];
		glm::ivec2 right_color_bounds = color_right_index[curve_id];

		int left_color_ind = left_color_bounds.x;
		int right_color_ind = right_color_bounds.x;
		
		//Set the starting color to the first entry of the colors
		glm::vec3 prev_left_color = color_left[left_color_ind];
		glm::vec3 prev_right_color = color_right[right_color_ind];

		//insert all control points 
		for (int control_ind = 0; control_ind < 4; control_ind++) {
			segment_split_curves[0].control_points[control_ind] = vertices[i * 4 + control_ind];
		}

		//Set the index to the color closest to the start without going over
		while (color_left_u[left_color_ind] < curve_segment_id && (left_color_ind < left_color_bounds.x + left_color_bounds.y - 2)) left_color_ind++;
		while (color_right_u[right_color_ind] < curve_segment_id && (right_color_ind < right_color_bounds.x + right_color_bounds.y - 2)) right_color_ind++;

		//The closest color to the start might not be at the actual start, so we need to interpolate, same for the next color, however if this color is not further than the end, there will be a split point which will fix the colors
		segment_split_curves[0].color_left[0] = { glm::mix(color_left[left_color_ind], color_left[left_color_ind + 1], ((float) curve_segment_id - color_left_u[left_color_ind])/(color_left_u[left_color_ind+1] - color_left_u[left_color_ind])), 1};
		segment_split_curves[0].color_left[1] = { glm::mix(color_left[left_color_ind], color_left[left_color_ind + 1], ((float)curve_segment_id + 1 - color_left_u[left_color_ind]) / (color_left_u[left_color_ind + 1] - color_left_u[left_color_ind])), 1};
		segment_split_curves[0].color_right[0] = { glm::mix(color_right[right_color_ind], color_right[right_color_ind + 1], ((float)curve_segment_id - color_right_u[right_color_ind]) / (color_right_u[right_color_ind + 1] - color_right_u[right_color_ind])), 1 };
		segment_split_curves[0].color_right[1] = { glm::mix(color_right[right_color_ind], color_right[right_color_ind + 1], ((float)curve_segment_id + 1 - color_right_u[right_color_ind]) / (color_right_u[right_color_ind + 1] - color_right_u[right_color_ind])), 1 };

		//reset the index to check for split points
		left_color_ind = left_color_bounds.x;
		right_color_ind = right_color_bounds.x;

		//How far along the parameter of this segment we are
		float used_u = 0;
		//Loop over all split points
		while (split_point_ind < split_points[curve_id].size() && split_points[curve_id][split_point_ind] < curve_segment_id + 1) {
			//Check if color control point is the current split point (either left or right must be, possibly both)
			bool split_left = color_left_u[left_color_ind + 1] == split_points[curve_id][split_point_ind];
			bool split_right = color_right_u[right_color_ind + 1] == split_points[curve_id][split_point_ind];
			if (split_left) left_color_ind++;
			if (split_right) right_color_ind++;

			//Split the curve at the given parameter point, taking into acount it might have already been split before
			float u = split_points[curve_id][split_point_ind] - curve_segment_id;
			float mix_a = (u - used_u) / (1 - used_u);
			auto new_curves = split_curve(segment_split_curves.back(), mix_a);

			//If the color was split the new end color is the stored color otherwise we need to interpolate
			glm::vec3 mid_left_color = split_left ? color_left[left_color_ind] : glm::mix(prev_left_color, color_left[left_color_ind], mix_a);
			glm::vec3 mid_right_color = split_right ? color_right[right_color_ind] : glm::mix(prev_right_color, color_right[right_color_ind], mix_a);

			//Set the colors on the two new curves
			new_curves[0].color_left[0] = { prev_left_color, 1};
			new_curves[0].color_left[1] = { mid_left_color, 1 };
			new_curves[0].color_right[0] = { prev_right_color, 1 };
			new_curves[0].color_right[1] = { mid_right_color, 1 };

			new_curves[1].color_left[0] = { mid_left_color, 1 };
			new_curves[1].color_left[1] = { color_left[left_color_ind + 1], 1 };
			new_curves[1].color_right[0] = { mid_right_color, 1 };
			new_curves[1].color_right[1] = { color_right[right_color_ind + 1], 1 };

			//keep track of the end color to use for the next split
			prev_left_color = mid_left_color;
			prev_right_color = mid_right_color;

			//replace the last curve by the 2 new curves
			segment_split_curves[segment_split_curves.size() - 1] = new_curves[0];
			segment_split_curves.push_back(new_curves[1]);

			//Set the progress allong the segment and increment the split point index
			used_u = u;
			split_point_ind++;
		}





		curves.insert(curves.end(), segment_split_curves.begin(), segment_split_curves.end());
	}

};

std::vector<Line> linearize_bezier_curve(BezierCurve curve, float tolerance, int max_depth) {
	std::vector<Line> lines;

	linearize_bezier_curve_helper(curve, tolerance, lines, max_depth);

	return lines;
};

//Helper function to handle the recusion of the approximation
void linearize_bezier_curve_helper(BezierCurve curve, float tolerance, std::vector<Line>& lines, int max_depth) {
	//If the curve is flat enough, it is well approximated by a line through the first and last control point
	if (is_curve_flat(curve, tolerance) || max_depth == 0) {
		lines.push_back({
			curve.control_points[0],
			curve.control_points[3],
			{curve.color_left[0],curve.color_left[1]},
			{curve.color_right[0],curve.color_right[1]},
			}
		);
	}
	//Otherwise split the curve in the middle and try again
	else {
		auto curves = split_curve(curve, 0.5f);

		linearize_bezier_curve_helper(curves[0], tolerance, lines, max_depth - 1);
		linearize_bezier_curve_helper(curves[1], tolerance, lines, max_depth - 1);
	}
}

//Splits a bezierCurve into 2 smaller curves at the point where the curve parameter is equal to alpha
std::array<BezierCurve, 2> split_curve(BezierCurve curve, float alpha) {
	
	std::array<BezierCurve,2> new_curves = std::array<BezierCurve, 2>();

	glm::vec2 m = glm::mix(curve.control_points[1], curve.control_points[2], alpha);

	glm::vec2 l_1 = glm::mix(curve.control_points[0], curve.control_points[1], alpha);
	glm::vec2 r_2 = glm::mix(curve.control_points[2], curve.control_points[3], alpha);

	glm::vec2 l_2 = glm::mix(l_1, m, alpha);
	glm::vec2 r_1 = glm::mix(m, r_2, alpha);

	glm::vec2 lr = glm::mix(l_2, r_1, alpha);

	glm::vec4 color_left_m = glm::mix(curve.color_left[0], curve.color_left[1], alpha);
	glm::vec4 color_right_m = glm::mix(curve.color_right[0], curve.color_right[1], alpha);


	new_curves[0] = {
		{
			curve.control_points[0],
			l_1,
			l_2,
			lr
		},
		{
			curve.color_left[0],
			color_left_m
		},
		{
			curve.color_right[0],
			color_right_m
		}
	};

	new_curves[1] = {
		{
			lr,
			r_1,
			r_2,
			curve.control_points[3]
		},
		{
			color_left_m,
			curve.color_left[1],
		},
		{
			color_right_m,
			curve.color_right[1]
		}
	};

	return new_curves;
}

//Checks if the curve is close enough to a line through the start and end point
bool is_curve_flat(BezierCurve curve, float tolerance) {
	glm::vec2 u = 3.0f * curve.control_points[1] - 2.0f * curve.control_points[0] - curve.control_points[3];
	u *= u;
	glm::vec2 v = 3.0f * curve.control_points[2] - 2.0f * curve.control_points[3] - curve.control_points[0];
	v *= v;

	return (fmaxf(u.x, v.x) + fmaxf(u.y, v.y) <= tolerance);
}

//Helper functionf for reading colors from diffusion curve file
void pushColor(rapidxml::xml_node<>* color_node, std::vector<glm::uvec2>& ind, std::vector<float>& color_u, std::vector<glm::vec3>& color) {
	float u = (float) (std::atof(color_node->first_attribute("globalID", 8)->value()) / 10.0f);
	color.push_back({
		std::atoi(color_node->first_attribute( "R",1)->value()) / 255.0f,
		std::atoi(color_node->first_attribute("G",1)->value()) / 255.0f,
		std::atoi(color_node->first_attribute( "B",1)->value()) / 255.0f
		});
	color_u.push_back(u);
	ind.back().y++;
}

