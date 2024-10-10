#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include<imgui/imgui.h>
#include<imgui/imgui_impl_glfw.h>
#include<imgui/imgui_impl_opengl3.h>
DISABLE_WARNINGS_POP()

#include<iostream>
#include<random>
#include<framework/window.h>
#include<framework/shader.h>
#include<framework/trackball.h>
#include "shapes.h"

//Resolution and boilerplate for the window
constexpr glm::ivec2 resolution{ 512, 512 };
std::unique_ptr<Window> pWindow;
std::unique_ptr<Trackball> pTrackball;

//Forward declaration for GLFW callback function
void keyboard(int key, int /* scancode */, int /* action */, int /* mods */);
void reshape(const glm::ivec2& size);
void rasterize_shape(const GLuint& VAO, const GLuint& frameBuffer, const Shader& shader, const GLuint& circleBuffer, const GLuint& lineBuffer, const float& line_width, const Shape& shapetype);

int constexpr file_name_buffer_size = 40;

//Show the debug menu
bool debug_menu_on = true;

//Seed and number of circles to generate
int circle_seed = 1;
int number_of_circles = 12;

//Path to XML files and the xml file itself
std::filesystem::path xml_folder(RESOURCE_ROOT "/resources/diffusionCurveXMLs");
//The xml file name is in a larger buffer for some leniency when the user changes it
char file_name_buffer[file_name_buffer_size] = "arch.xml";

//The maximum amount curves should be subdivided to fit the lines better.
int max_curve_subdivision = 0;

//Uniform for the maximum distance from a shape to be considered part of it
float rasterize_width = 0.75f;
//Step size and maximum steps for raymarching
float step_size = 0.05f;
unsigned int max_raymarch_iters = 10000;

//If sampling is paused, and a flag to take 1 sample even if paused
bool paused = false;
bool one_sample = false;

//Which output should be shown, 
// 0 - standard output
// 1 - rasterize_texture
// 2 - accumulator_texture
int output_type = 0;


/// <summary>
/// Creates nr_circles random circle shapes and adds them to circles
/// </summary>
/// <param name="circles">Place to store the created circles</param>
/// <param name="nr_circles">The number of circles to make</param>
void randomize_circles(std::vector<Circle>& circles, int nr_circles, int seed = -1) {
    //Create a random number engine, seeded by device random, replace rd() by 0 for deterministic generation.
    std::random_device rd;
    std::mt19937 re;
    
    if (seed == -1) {
        re = std::mt19937(rd());
    }
    else {
        re = std::mt19937(seed);
    }
    //Setup distributions
    std::uniform_real_distribution<float> distC(0, 1); // Distribution for color channels
    std::uniform_real_distribution<float> distR(0, resolution.y/2); // Distribution for radii

    circles.clear();
    //Create nr_circles shape::Circle using initializer list 
    int circle_id = (int) circles.size() + 1;
    for (int i = 0; i < nr_circles; i++) {
        circles.push_back({ 
            {distC(re), distC(re), distC(re),1}, // RGBA color
            {(int) (distC(re) * resolution.y), (int) (distC(re) * resolution.y)}, //Center
            distR(re), //Radius
        });
    }
}

int main() {
	//Create Opengl 4.1 window
	pWindow = std::make_unique<Window>("Diffusion Curves", resolution, OpenGLVersion::GL41);
	//Create trackball camera
	pTrackball = std::make_unique<Trackball>(pWindow.get(), glm::radians(50.0f));

	//Setup GLFW callbacks
	pWindow->registerKeyCallback(keyboard);
	pWindow->registerWindowResizeCallback(reshape);

    //Create Quad covering the entire screen
    int quad_indices[6] = { 0,1,2,3,4,5, };
    glm::vec3  quad_vertices[6] = { {1,-1,0},{-1,1,0},{1,1,0}, {-1,-1,0}, {-1,1,0}, {1,-1,0} };

	//Set the shape to use
	Shape shape = Shape::Circle;

	// Create vertex (vbo) and index (ibo) buffer objects and fill them with the data for the quad, this will be the only geometry we need.
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec3), quad_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(int), quad_indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//Setup vertex array object so we dont have to mess with binding the buffers every time
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	glEnableVertexAttribArray(0);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    // Load shaders and build 3 shader programs
    // rasterizeShader : rasterizes shapes
    // sampleShader : ray-marches the rasterized shapes to integrate the color
    // colorShader : creates the final image by aggregating the acummulated samples
    const Shader rasterizeShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/rasterize_primitive.glsl").build();
    const Shader sampleShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/sample_shader.glsl").build();
    const Shader colorShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/color_shader.glsl").build();
    
    //Load debug shader for showing the intermediate textures.
    const Shader textureShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/texture_shader.glsl").build();


	//We load Both the bezier curves and circles so we can switch on the fly.

	//Load the bezier curves from the XML files into memory as shape::BezierCurves
	std::vector<BezierCurve> curves;
	load_Bezier_curves(curves, (xml_folder / file_name_buffer).string().c_str(), resolution);

	//Create a linear approximation of the bezier curves
	std::vector<Line> lines;
	for (BezierCurve curve : curves) {
		auto new_lines = linearize_bezier_curve(curve, 1, max_curve_subdivision);
		lines.insert(lines.begin(), new_lines.begin(), new_lines.end());
	}

    //Create random circles
    std::vector<Circle> circles;
    int circle_id = 1;
    randomize_circles(circles, number_of_circles, circle_seed);

	//Create Uniform buffer objects for the circles and lines, the structs are also in GLSL so we can simply put the data directly in the buffer
	number_of_circles = (int)circles.size();

	GLuint circleUbo;
	glGenBuffers(1, &circleUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, circleUbo);
	//Set buffer size but dont put anything in it just yet
	glBufferData(GL_UNIFORM_BUFFER, number_of_circles * sizeof(Circle) + 16, NULL, GL_DYNAMIC_DRAW);
	//put the number of circles in the first 4 bytes to the number of circles
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &number_of_circles);
	//GLSL data is 16 byte alligned so the actual data starts at byte 16
	glBufferSubData(GL_UNIFORM_BUFFER, 16, number_of_circles * sizeof(Circle), circles.data());
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	int number_of_lines = (int)lines.size();

	GLuint lineUbo;
	glGenBuffers(1, &lineUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, lineUbo);
	//Set buffer size but dont put anything in it just yet
	glBufferData(GL_UNIFORM_BUFFER, number_of_lines * sizeof(Line) + 16, NULL, GL_DYNAMIC_DRAW);
	//put the number of circles in the first 4 bytes to the number of circles
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &number_of_lines);
	//GLSL data is 16 byte alligned so the actual data starts at byte 16
	glBufferSubData(GL_UNIFORM_BUFFER, 16, number_of_lines * sizeof(Line), lines.data());
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//Create texture for the rasterized shapes
	GLuint texRasterized;
	glGenTextures(1, &texRasterized);
	glBindTexture(GL_TEXTURE_2D, texRasterized);

	//We want the texture to represent the id of the rasterized shape so we request a 32 bit int as internal format
	//We dont put any data in the texture as that is what the first renderpass is for
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, resolution.x, resolution.y, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);
	// Set behaviour for when texture coordinates are outside the [0, 1] range.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set interpolation for texture sampling (GL_NEAREST for no interpolation).
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);


	// Create framebuffer attached to the rasterized shape texture
	// We put this framewbuffer as output to the drawcall, just like shadowmapping.
	GLuint rasterized_shape_buffer;
	glGenFramebuffers(1, &rasterized_shape_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, rasterized_shape_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texRasterized, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Repeat the entire procces for the sample acummulator texture
	GLuint texAccumulator;
	glGenTextures(1, &texAccumulator);
	glBindTexture(GL_TEXTURE_2D, texAccumulator);

	//The acumulator needs to be reasonable precision, so we will use 32 bit floats for each color channel
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, resolution.x, resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Set behaviour for when texture coordinates are outside the [0, 1] range.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set interpolation for texture sampling (GL_NEAREST for no interpolation).
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint accumulator_buffer;
	glGenFramebuffers(1, &accumulator_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, accumulator_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texAccumulator, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Rasterize the shapes in an intial rendering pass, this only needs to happen once (or after a reset)    
	rasterize_shape(vao, rasterized_shape_buffer, rasterizeShader, circleUbo, lineUbo, rasterize_width, shape);

	//With the shapes rasterized we can start taking samples of our integral
	//Keep track of the frame nr for the random number generator
	unsigned int frame_nr = 0;

	while (!pWindow->shouldClose()) {
		pWindow->updateInput();

		// Clear screen
		glViewport(0, 0, pWindow->getWindowSize().x, pWindow->getWindowSize().y);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// Bind vertex data
		glBindVertexArray(vao);

		//Only take sample if not paused, or the take one sample flag is set
		if (!paused || one_sample) {
			//----- run the sample shader
			sampleShader.bind();

			sampleShader.bindUniformBlock("circleBuffer", 0, circleUbo);
			sampleShader.bindUniformBlock("lineBuffer", 1, lineUbo);

			glUniform1ui(sampleShader.getUniformLocation("shape_type"), static_cast<GLuint>(shape));

			glUniform1ui(sampleShader.getUniformLocation("frame_nr"), frame_nr);
			glUniform1ui(sampleShader.getUniformLocation("max_raymarch_iter"), max_raymarch_iters);
			glUniform2iv(sampleShader.getUniformLocation("screen_dimensions"), 1, glm::value_ptr(resolution));
			glUniform1f(sampleShader.getUniformLocation("step_size"), step_size);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texRasterized);
			glUniform1i(sampleShader.getUniformLocation("rasterized_texture"), 0);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, texAccumulator);
			glUniform1i(sampleShader.getUniformLocation("accumulator_texture"), 1);

			glBindFramebuffer(GL_FRAMEBUFFER, accumulator_buffer);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6), GL_UNSIGNED_INT, nullptr);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			//reset take one sample flag
			one_sample = false;
			frame_nr++;
		}


		//----- run the aggregate shader

		colorShader.bind();
		glUniform2iv(colorShader.getUniformLocation("screen_dimensions"), 1, glm::value_ptr(resolution));


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texAccumulator);
		glUniform1i(colorShader.getUniformLocation("accumulator_texture"), 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6), GL_UNSIGNED_INT, nullptr);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //Overwrite output framebuffer with texture if a texture should be shown instead.
        if (output_type > 0) {
            textureShader.bind();

            glUniform2iv(textureShader.getUniformLocation("screen_dimensions"), 1, glm::value_ptr(resolution));
            glUniform1i(textureShader.getUniformLocation("texture_id"), output_type);


            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texRasterized);
            glUniform1i(textureShader.getUniformLocation("rasterized_texture"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texAccumulator);
            glUniform1i(textureShader.getUniformLocation("accumulator_texture"), 1);


            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6), GL_UNSIGNED_INT, nullptr);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }



        glBindVertexArray(0);
        
        //----- Debug menu
        if (debug_menu_on) {
            //Flags used in the menu to reset textures or reload primitives.
            bool reset_rasterize = false;
            bool reset_accumulator = false;
            bool redo_circles = false;
            bool redo_lines = false;


			ImGui::Begin("Window");

            //Shape type selector
            const char* shape_list[2] = { "Circles","Curves" };
            if (ImGui::Combo("shape type", ((int*)&shape), shape_list, 2)) {
                reset_rasterize = true;
                reset_accumulator = true;
            };

            //Pause sampling and 1 sample buttons
            if (ImGui::Button("Pause sampling")) {
                paused = !paused;
            }
            ImGui::SameLine();
            one_sample = ImGui::Button("Take 1 Sample");

            //Rasterize width slider
            if (ImGui::SliderFloat("rasterize width", &rasterize_width,0,2)) {
                reset_rasterize = true;
                reset_accumulator = true;
            }

            //Step size slider
            if (ImGui::SliderFloat("Step size", &step_size, 0, 2)) {
                reset_accumulator = true;
            }

            //Max raymarching steps input
            if (ImGui::InputInt("max raymarch iters", ((int*)&max_raymarch_iters))) {
                max_raymarch_iters = std::max(max_raymarch_iters, 0u);
                reset_accumulator = true;
            }

            //Number of circles input
            if (ImGui::InputInt("number of circles", ((int*)&number_of_circles))) {
                reset_rasterize = true;
                reset_accumulator = true;
                redo_circles = true;
                randomize_circles(circles, number_of_circles, circle_seed);
            }

            //Seed for circles
            if (ImGui::InputInt("circle_seed of circles", ((int*)&circle_seed))) {
                reset_rasterize = true;
                reset_accumulator = true;
                redo_circles = true;
                randomize_circles(circles, number_of_circles, circle_seed);
            }

            //Text input for the diffusion curve file selector
            ImGui::InputText("Diffusioncurve file", file_name_buffer, file_name_buffer_size);
            if (ImGui::Button("Reload diffusion curves")) {
                reset_accumulator = true;
                reset_rasterize = true;
                redo_lines = true;
            }
            
            //Maximum level of subdivision of bezier curves into lines, after subdividing on color control points.
            if (ImGui::SliderInt("maximum diffusion curve subdivision", &max_curve_subdivision, 0, 10)) {
                reset_accumulator = true;
                reset_rasterize = true;
                redo_lines = true;
            }

            //Selector for the output shown on screen
            const char* output_list[3] = { "color_shader", "rasterize_texture", "accumulator_texture" };
            ImGui::Combo("output type", &output_type, output_list, 3);
            
            //Buttons to reset textures
            reset_accumulator |= ImGui::Button("reset sample");
            ImGui::SameLine();
            reset_rasterize |= ImGui::Button("reset shape");

           
            //Reset textures/ reload primitives if required
            if (redo_circles) {
                number_of_circles = circles.size();

				glBindBuffer(GL_UNIFORM_BUFFER, circleUbo);
				//Set buffer size but dont put anything in it just yet
				glBufferData(GL_UNIFORM_BUFFER, number_of_circles * sizeof(Circle) + 16, NULL, GL_DYNAMIC_DRAW);
				//put the number of circles in the first 4 bytes to the number of circles
				glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &number_of_circles);
				//GLSL data is 16 byte alligned so the actual data starts at byte 16
				glBufferSubData(GL_UNIFORM_BUFFER, 16, number_of_circles * sizeof(Circle), circles.data());
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}

			//Load a new diffusion curve file
			if (redo_lines) {
				//Clear current curves and load the new folder into it
				curves.clear();
				load_Bezier_curves(curves, (xml_folder / file_name_buffer).string().c_str(), resolution);

				//Create a linear approximation of the bezier curves
				lines.clear();

				for (BezierCurve curve : curves) {
					auto new_lines = linearize_bezier_curve(curve, 1, max_curve_subdivision);
					lines.insert(lines.begin(), new_lines.begin(), new_lines.end());
				}

				number_of_lines = (int)lines.size();

				glBindBuffer(GL_UNIFORM_BUFFER, 0);

				glBindBuffer(GL_UNIFORM_BUFFER, lineUbo);
				//Set buffer size but dont put anything in it just yet
				glBufferData(GL_UNIFORM_BUFFER, number_of_lines * sizeof(Line) + 16, NULL, GL_DYNAMIC_DRAW);
				//put the number of circles in the first 4 bytes to the number of circles
				glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &number_of_lines);
				//GLSL data is 16 byte alligned so the actual data starts at byte 16
				glBufferSubData(GL_UNIFORM_BUFFER, 16, number_of_lines * sizeof(Line), lines.data());
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}

			//Reset rasterized_texture, and re-rasterize
			if (reset_rasterize) {
				//Bind the rasterized framebuffer
				glBindFramebuffer(GL_FRAMEBUFFER, rasterized_shape_buffer);
				//Clear the bufffer
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				//unbind buffer
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				rasterize_shape(vao, rasterized_shape_buffer, rasterizeShader, circleUbo, lineUbo, rasterize_width, shape);
			}

			//Reset the acummulator texture
			if (reset_accumulator) {
				//Bind the accumulator framebuffer
				glBindFramebuffer(GL_FRAMEBUFFER, accumulator_buffer);
				//Clear the bufffer
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				//unbind buffer
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			ImGui::End();

		}

		pWindow->swapBuffers();
	}
}

//Function to create the rasterized_texture texture
void rasterize_shape(const GLuint& VAO, const GLuint& frameBuffer, const Shader& shader, const GLuint& circleBuffer, const GLuint& lineBuffer, const float& line_width, const Shape& shapetype) {
	//Bind all the data
	glBindVertexArray(VAO);
	shader.bind();
	shader.bindUniformBlock("circleBuffer", 0, circleBuffer);
	shader.bindUniformBlock("lineBuffer", 1, lineBuffer);
	glUniform1ui(shader.getUniformLocation("shape_type"), static_cast<GLuint>(shapetype));
	glUniform1f(shader.getUniformLocation("rasterize_width"), line_width);
	//Bind the rasterized shape framebuffer do the rendering pass and unbind the buffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6), GL_UNSIGNED_INT, nullptr);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindVertexArray(0);

}

//Key bindings
void keyboard(int key, int /* scancode */, int  action , int /* mods */) {
    if (key == '\\' && action == GLFW_PRESS) {
        debug_menu_on = !debug_menu_on;
    }
}

void reshape(const glm::ivec2& size) {
	glViewport(0, 0, size.x, size.y);
}