#include "utils.h"
#include <iostream>

//Get subdirectories
std::vector<std::string> getSubdirectories(const std::filesystem::path& directoryPath) {
    std::vector<std::string> subdirectories;

    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
        std::cerr << "Directory does not exist or is not a directory." << std::endl;
        return subdirectories;
    }

    // Iterate through the directory to find subdirectories
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_directory()) {
            subdirectories.push_back(entry.path().filename().string());
        }
    }

    return subdirectories;
}

//Show directory dropdown
void showDirectoryDropdown(const std::filesystem::path& rootDirectory, std::filesystem::path& selectedDirectoryPath) {

    // Gets sub-directories
    std::vector<std::string> folders = getSubdirectories(rootDirectory);

    if (folders.empty()) {
        ImGui::Text("No folders found in the directory.");
    }

    // Creates a dropdown for choosing the folder
    if (ImGui::BeginCombo("Select Folder", selectedFolderIndex >= 0 ? folders[selectedFolderIndex].c_str() : "Default")) {
        for (int i = 0; i < folders.size(); ++i) {
            bool isSelected = (selectedFolderIndex == i);
            if (ImGui::Selectable(folders[i].c_str(), isSelected)) {
                selectedFolderIndex = i;  // Memorizes the folder index
                selectedDirectoryPath = rootDirectory / folders[i];
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();  // focus on element
            }
        }
        ImGui::EndCombo();
    }

    if (selectedFolderIndex >= 0 ) {
        ImGui::Text("Selected Folder: %s", selectedDirectoryPath.string().c_str());
    }
}

void renderSpacing(const unsigned short spaces = 1, const bool separator = false) {
    for (int i = 0; i < spaces; i++) {
        ImGui::Spacing();
        if(separator && i == static_cast<int>(spaces)/2 - 1)
            ImGui::Separator();
    }
}

void renderLightsIcons(const Light &light, const glm::mat4 &mvpMatrix) {
    const Shader lightShader = ShaderBuilder()
        .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vert.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl")
        .build();

    lightShader.bind();
    GLuint vao;

    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // This is where we would set the attribute pointers, if apple supported it.
    const glm::vec4 screenPos = mvpMatrix * glm::vec4(light.position, 1.0f);
    const glm::vec3 color { 1, 1, 0 };

    glPointSize(40.0f);
    glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
    glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(color));
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);

}

//For movement on bezier curve
glm::vec3 calculatePositionOnBezier(const Curve &curve, const float &t){
    float b0 = pow((1.0 - t), 3);
    float b1 = 3.0 * t * (1.0 - t) * (1.0 - t);
    float b2 = 3.0 * t * t * (1.0 - t);
    float b3 = pow(t, 3);
    //curve evaluation
    glm::vec3 p = b0 * curve.controlPoints[0] + b1 * curve.controlPoints[1] + b2 * curve.controlPoints[2] + b3 * curve.controlPoints[3];
    return p;
}

std::map<float, float> computeLookupTable(const Curve& curve){
    std::map<float, float> lookup;
    lookup[0.0] = 0.0;
    float currentDistanceTravelled = 0.0;
    glm::vec3 prevPoint = calculatePositionOnBezier(curve, 0.0);
    for(int i = 1; i <= 100000; i++){
        float t = static_cast<float>(i) / static_cast<float>(1000);
        glm::vec3 currentPoint = calculatePositionOnBezier(curve, t);
        float arcLength = glm::distance(currentPoint, prevPoint);
        currentDistanceTravelled += arcLength;
        lookup[t] = currentDistanceTravelled;
        prevPoint = currentPoint;
    }
    return lookup;
}

float getTimeFromDistance(const std::map<float, float> &lookup, float distance){
    auto it = lookup.begin();
    // loop to get to the first distance in lookup equal or greater than the input distance
    for(; it != lookup.end(); it++){
        if(it->second >= distance) break;
    }
    if(it != lookup.end()){
        float distance1 = it->second;
        float time1 = it->first;
        if(it != lookup.begin()){
            it--;
            float distance2 = it->second;
            float time2 = it->first;
            return time1 + (time2 - time1) * ((distance - distance1) / (distance2 - distance1));
        }
    }
    return it->second;
}

void initializePlaneVAO(const int res, const int width, GLuint * planeVAO, GLuint * planeVBO, GLuint * planeEBO) {

	//const int res = 3;
	const int nPoints = res * res;
	const int size = nPoints * 3 + nPoints * 3 + nPoints * 2;
	float * vertices = new float[size];
	for (int i = 0; i < res; i++) {
		for (int j = 0; j < res; j++) {
			//add position
			float x = j * (float)width / (res - 1) - width / 2.0;
			float y = 0.0;
			float z = -i * (float)width / (res - 1) + width / 2.0;

			vertices[(i + j * res) * 8] = x; //8 = 3 + 3 + 2, float per point
			vertices[(i + j * res) * 8 + 1] = y;
			vertices[(i + j * res) * 8 + 2] = z;

			//add normal
			float x_n = 0.0;
			float y_n = 1.0;
			float z_n = 0.0;

			vertices[(i + j * res) * 8 + 3] = x_n;
			vertices[(i + j * res) * 8 + 4] = y_n;
			vertices[(i + j * res) * 8 + 5] = z_n;

			//add texcoords
			vertices[(i + j * res) * 8 + 6] = (float)j / (res - 1);
			vertices[(i + j * res) * 8 + 7] = (float)(res - i - 1) / (res - 1);
		}
	}

	const int nTris = (res - 1)*(res - 1) * 2;
	int * trisIndices = new int[nTris * 3];

	for (int i = 0; i < nTris; i++) {
		int trisPerRow = 2 * (res - 1);
		for (int j = 0; j < trisPerRow; j++) {
			if (!(i % 2)) { //upper triangle
				int k = i * 3;
				int triIndex = i % trisPerRow;

				int row = i / trisPerRow;
				int col = triIndex / 2;
				trisIndices[k] = row * res + col;
				trisIndices[k + 1] = ++row*res + col;
				trisIndices[k + 2] = --row* res + ++col;
			}
			else {
				int k = i * 3;
				int triIndex = i % trisPerRow;

				int row = i / trisPerRow;
				int col = triIndex / 2;
				trisIndices[k] = row * res + ++col;
				trisIndices[k + 1] = ++row * res + --col;
				trisIndices[k + 2] = row * res + ++col;
			}
		}
	}

	glGenVertexArrays(1, planeVAO);
	glGenBuffers(1, planeVBO);
	glGenBuffers(1, planeEBO);

	glBindVertexArray(*planeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, *planeVBO);
	glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *planeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, nTris * 3 * sizeof(unsigned int), trisIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	delete[] vertices;
}
