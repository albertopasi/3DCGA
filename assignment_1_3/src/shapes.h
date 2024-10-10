#include <glm/glm.hpp>

#include <vector>

// Shape enumerator
enum class Shape {
	Circle,
	Line,
	BezierCurve
};

// Be sure to know what you are doing when editing these structs!
// Opengl std140 requires them to be 16 byte alligned, while MSVC alligns structs by 4 bytes

// Struct for the circle, with position, radius, id and color
struct Circle {
  glm::vec4 color;
  glm::vec2 position;
  float radius;
  float _pad;
};

// Struct for bezier curve, 4 control points, colors for the left and right side at both the start and end
struct BezierCurve {
	glm::vec2 control_points[4];
	glm::vec4 color_left[2];
	glm::vec4 color_right[2];
};

// Struct for line, start and end point left color and right color
struct Line {
	glm::vec2 start_point;
	glm::vec2 end_point;
	glm::vec4 color_left[2];
	glm::vec4 color_right[2];
};

/// <summary>
/// Loads a set of diffusion curves from an diffusion curve XML file
/// </summary>
/// <param name="curves">Vector to store the loaded Bezier curves</param>
/// <param name="path">Path to the file containing the diffusion curves</param>
/// <param name="resolution">The resolution we intent to render the curves at</param>
void load_Bezier_curves(std::vector<BezierCurve>& curves, const char* path, glm::ivec2 resolution);
/// <summary>
/// Creates a linear approximation for the given BezierCurve based on 
/// Fischer, Kaspar. "Piecewise Linear Approximation of B'ezier Curves," n.d.
/// </summary>
/// <param name="curve">The bezier curve to approximaate </param>
/// <param name="tolerance">If the line version of the curve is closer than tolarance stop</param>
/// <param name="max_depth">Stop after max_depth sub_divisions </param>
/// <returns>The set of lines that approximate the curve</returns>
std::vector<Line> linearize_bezier_curve(BezierCurve curve, float tolerance, int max_depth = INT_MAX);
