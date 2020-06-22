#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "stb_image.h"

#include <iostream>
#include <cmath>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void norm(float* v, float mod);
void copyTri(float* dst, float* v0, float* v1, float* v2);

// global settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// light settings
const glm::vec3 LIGHT_COLOR = glm::vec3(1.0f, 1.0f, 1.0f);
const glm::vec3 LIGHT_POS = glm::vec3(-0.2f, 1.0f, 0.3f);

// texture settings
const char* IMG_PATH = "name.jpg";
const int REPEAT = 3;

// camera settings
const glm::vec3 CAMERA_POS = glm::vec3(0.0f, 0.0f, 3.0f);
const glm::vec3 CAMERA_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);
const glm::vec3 CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);

// pantagram settings
const float LINE_WIDTH = 1.0;
const float PAI = 3.141592654f;
const int ANGLE_NUM = 5; // 5 for pantagram, 6 for hexagram, etc.
const glm::vec3 TRANSLATE_PANTAGRAM = glm::vec3(0.7f, 0.0f, 0.3f);
const glm::vec3 SCALE_PANTAGRAM = glm::vec3(0.5f);
const glm::vec3 LINE_COLOR = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 CORE_COLOR = glm::vec3(1.0f, 1.0f, 0.0f);

// cube settings
const glm::vec3 CUBE_COLOR = glm::vec3(1.0f, 1.0f, 1.0f);
const glm::vec3 TRANSLATE_CUBE = glm::vec3(0.0f);
const glm::vec3 SCALE_CUBE = glm::vec3(0.4f);

// sphere settings
const unsigned int EPOCH = 7;
const float RADIUS = 0.8f;
const float SPHERE_SCALE = 0.4f;
const float SQRT2 = 1.414214f;
const float SQRT3 = 1.732051f;
const glm::vec3 TRANSLATE_SPHERE = glm::vec3(-0.7f, 0.0f, -0.3f);
const glm::vec3 SCALE_SPHERE = glm::vec3(SPHERE_SCALE);
const glm::vec3 SPHERE_COLOR = glm::vec3(1.0f, 0.5f, 0.3f);

// surface settings
const float SURFACE_Y = -RADIUS * SPHERE_SCALE;
const glm::vec3 TRANSLATE_SURFACE = glm::vec3(0.0f, SURFACE_Y - 0.01f, 0.0f);
const glm::vec3 SCALE_SURFACE = glm::vec3(2.0f, 2.0f, 2.0f);

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "BUAA CG", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	Shader reflectShader("Resource/reflection.vs", "Resource/reflection.fs");
	Shader plainShader("Resource/plain.vs", "Resource/plain.fs");
	Shader textShader("Resource/texture.vs", "Resource/texture.fs");
	Shader shadowShader("Resource/shadow.vs", "Resource/shadow.fs");

	// set up vertex data (and buffer(s)) and configure vertex attributes for patagram
	// ------------------------------------------------------------------
	float gramVertices[2 * 3 * ANGLE_NUM + 3];
	unsigned int innerIndices[3 * ANGLE_NUM];
	unsigned int outerIndices[3 * ANGLE_NUM];

	// origin point
	gramVertices[0] = 0;
	gramVertices[1] = 0;
	gramVertices[2] = 0;

	// outer vertices
	float* outer = gramVertices + 3 + 3 * ANGLE_NUM;
	for (int i = 0; i < ANGLE_NUM; ++i) {
		float radian = (0.5f + (float)i / ANGLE_NUM * 2) * PAI;
		outer[3 * i] = RADIUS * std::cos(radian);
		outer[3 * i + 1] = RADIUS * std::sin(radian);
		outer[3 * i + 2] = 0;
	}

	// calculate inner vertices using outers
	float* inner = gramVertices + 3;

	// another way to calculate inners
	float radian = PAI * (1 - 2.0f / ANGLE_NUM);
	float innerRadius = std::sin(radian - PAI / 2) * RADIUS / std::sin(PAI - radian / 2);
	for (int i = 0; i < ANGLE_NUM; ++i) {
		float radian = (0.5f - 1.0f / ANGLE_NUM + (float)i / ANGLE_NUM * 2) * PAI;
		inner[3 * i] = innerRadius * std::cos(radian);
		inner[3 * i + 1] = innerRadius * std::sin(radian);
		inner[3 * i + 2] = 0;
	}

	// calculate triangle index
	for (int i = 0; i < ANGLE_NUM; ++i) {
		// i-th outer triangle include: i-th outer vertex, i-th inner vertex, i+1-th inner vertex
		outerIndices[3 * i] = ANGLE_NUM + 1 + i;
		outerIndices[3 * i + 1] = 1 + i;
		outerIndices[3 * i + 2] = 1 + (i + 1) % ANGLE_NUM;
		// i-th inner triangle include: origin point, i-th inner vertex, i+1-th inner vertex
		innerIndices[3 * i] = 0;
		innerIndices[3 * i + 1] = 1 + i;
		innerIndices[3 * i + 2] = 1 + (i + 1) % ANGLE_NUM;
	}

	unsigned int gramVBOs[2], gramVAOs[2], gramEBOs[2];
	glGenVertexArrays(2, gramVAOs);
	glGenBuffers(2, gramVBOs);
	glGenBuffers(2, gramEBOs);
	glBindVertexArray(gramVAOs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, gramVBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (3 * ANGLE_NUM + 3), gramVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gramEBOs[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(innerIndices), innerIndices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(gramVAOs[1]);
	glBindBuffer(GL_ARRAY_BUFFER, gramVBOs[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gramVertices), gramVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gramEBOs[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(outerIndices), outerIndices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); 
	glEnableVertexAttribArray(0);

	// set up vertex data (and buffer(s)) and configure vertex attributes for cube
	// ------------------------------------------------------------------
	float cubeVertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	unsigned int cubeVBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// set up sphere and surface data
	// ------------------------------
	float vertices[] = {
		0.0f, 0.0f, RADIUS,
		0.0f, 2 * SQRT2 / 3 * RADIUS, -RADIUS / 3,
		SQRT2 * SQRT3 / 3 * RADIUS, -SQRT2 / 3 * RADIUS, -RADIUS / 3,

		0.0f, 0.0f, RADIUS,
		0.0f, 2 * SQRT2 / 3 * RADIUS, -RADIUS / 3,
		-SQRT2 * SQRT3 / 3 * RADIUS, -SQRT2 / 3 * RADIUS, -RADIUS / 3,

		0.0f, 0.0f, RADIUS,
		SQRT2 * SQRT3 / 3 * RADIUS, -SQRT2 / 3 * RADIUS, -RADIUS / 3,
		-SQRT2 * SQRT3 / 3 * RADIUS, -SQRT2 / 3 * RADIUS, -RADIUS / 3,

		0.0f, 2 * SQRT2 / 3 * RADIUS, -RADIUS / 3,
		SQRT2 * SQRT3 / 3 * RADIUS, -SQRT2 / 3 * RADIUS, -RADIUS / 3,
		-SQRT2 * SQRT3 / 3 * RADIUS, -SQRT2 / 3 * RADIUS, -RADIUS / 3,
	};

	float surfaceVertices[] = {
		 1.0f,  0.0f,  1.0f,
		 1.0f,  0.0f, -1.0f,
		-1.0f,  0.0f, -1.0f,
		-1.0f,  0.0f, -1.0f,
		-1.0f,  0.0f,  1.0f,
		 1.0f,  0.0f,  1.0f,
	};


	const unsigned int vertexSize = sizeof(vertices) / 3 / sizeof(float) * pow(4, EPOCH);
	float* finalVertices = new float[vertexSize * (long long)3];
	float* textCoords = new float[vertexSize * (long long)2];
	float* normals = new float[vertexSize * (long long)3];

	memcpy(finalVertices, vertices, sizeof(vertices));

	for (int size = sizeof(vertices) / sizeof(float); size < vertexSize * (long long)3; size *= 4) {
		for (int j = 0; j < size; j += 9) {
			float* v0 = finalVertices + size - j - 9;
			float* v1 = v0 + 3;
			float* v2 = v1 + 3;
			float v01[] = { v0[0] + v1[0], v0[1] + v1[1], v0[2] + v1[2] };
			float v02[] = { v0[0] + v2[0], v0[1] + v2[1], v0[2] + v2[2] };
			float v12[] = { v2[0] + v1[0], v2[1] + v1[1], v2[2] + v1[2] };
			norm(v01, RADIUS);
			norm(v02, RADIUS);
			norm(v12, RADIUS);
			copyTri(finalVertices + size * (long long)4 - (long long)4 * j - 9, v0, v01, v02);
			copyTri(finalVertices + size * (long long)4 - (long long)4 * j - 18, v1, v01, v12);
			copyTri(finalVertices + size * (long long)4 - (long long)4 * j - 27, v2, v02, v12);
			copyTri(finalVertices + size * (long long)4 - (long long)4 * j - 36, v01, v02, v12);
		}
	}

	for (int i = 0; i < vertexSize; ++i) {
		float* v = finalVertices + (long long)3 * i;
		float x = acos(v[0] / RADIUS) / (2 * PAI);
		float y = acos(v[1] / RADIUS / sin(2 * PAI * x)) / (2 * PAI);
		textCoords[2 * i] = -x * REPEAT;
		textCoords[2 * i + 1] = -y * REPEAT;
	}

	for (int i = 0; i < vertexSize * 3; i += 9) {
		float* v[] = { finalVertices + i, finalVertices + i + 3, finalVertices + i + 6 };
		float normal[3];
		for (int j = 0; j < 3; ++j) {
			normal[j] = 0;
			for (int k1 = 0; k1 < 3; ++k1) {
				normal[j] += v[k1][(j + 1) % 3] * v[(k1 + 1) % 3][(j + 2) % 3] - v[(k1 + 1) % 3][(j + 1) % 3] * v[k1][(j + 2) % 3];
			}
		}
		norm(normal, 1.0f);
		if (normal[0] * v[0][0] + normal[1] * v[0][1] + normal[2] * v[0][2] < 0) {
			normal[0] = -normal[0];
			normal[1] = -normal[1];
			normal[2] = -normal[2];
		}
		copyTri(normals + i, normal, normal, normal);
	}

	float* tmp = new float[vertexSize * (long long)8];
	for (int i = 0; i < vertexSize; ++i) {
		memcpy(tmp + (long long)8 * i, finalVertices + (long long)3 * i, 3 * sizeof(float));
		memcpy(tmp + (long long)8 * i + 3, textCoords + (long long)2 * i, 2 * sizeof(float));
		memcpy(tmp + (long long)8 * i + 5, normals + (long long)3 * i, 3 * sizeof(float));
	}

	unsigned int sphereVBO, sphereVAO;
	glGenVertexArrays(1, &sphereVAO);
	glGenBuffers(1, &sphereVBO);

	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, vertexSize * (long long)8 * sizeof(float), tmp, GL_STATIC_DRAW);

	glBindVertexArray(sphereVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	unsigned int surfaceVAO;
	glGenVertexArrays(1, &surfaceVAO);
	glGenBuffers(1, &sphereVBO);

	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(surfaceVertices), surfaceVertices, GL_STATIC_DRAW);

	glBindVertexArray(surfaceVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	delete[] finalVertices, textCoords, tmp;

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load("Resource/name.jpg", &width, &height, &nrChannels, 0);
	if (data) {
		// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	} else {
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	
	unsigned int cubemapTexture;
	glGenTextures(1, &cubemapTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL
		);
	}

	unsigned int framebuffer[6];
	glGenFramebuffers(6, framebuffer);

	for (int i = 0; i < 6; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[i]);
		// glBindTexture(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapTexture);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapTexture, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << i << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::vector<glm::mat4> views{
		glm::lookAt(glm::vec3(3.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(-3.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, -3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
	};

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window)) {
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!
		glm::mat4 view = glm::lookAt(CAMERA_POS, CAMERA_POS + CAMERA_FRONT, CAMERA_UP);
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 model = glm::mat4(1.0f);

		// render gram
		// -----------
		plainShader.use();
		
		plainShader.setMat4("projection", projection);
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first	
		model = glm::translate(model, TRANSLATE_PANTAGRAM);
		model = glm::scale(model, SCALE_PANTAGRAM);
		// model = glm::rotate(model, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		plainShader.setMat4("model", model);
		
		/*
		float colours[][4] = {
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f },
			{ 1.0f, 0.0f, 0.0f, 1.0f },
			{ 0.0f, 1.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f, 1.0f },
			{ 0.0f, 1.0f, 1.0f, 1.0f },
		};
		*/
		float colours[][4] = {
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f, 1.0f },
		};

		for (int i = 0; i < 6; ++i) {
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[i]);
			// glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

			// make sure we clear the framebuffer's content
			glClearColor(colours[i][0], colours[i][1], colours[i][2], colours[i][3]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			plainShader.setMat4("view", views[i]);
			plainShader.setVec3("colour", CORE_COLOR);
			glBindVertexArray(gramVAOs[0]);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElements(GL_TRIANGLES, sizeof(innerIndices) / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
			// then we draw the second triangle using the data from the second VAO
			// when we draw the second triangle we want to use a different shader program so we switch to the shader program with our yellow fragment shader.
			plainShader.setVec3("colour", LINE_COLOR);
			glBindVertexArray(gramVAOs[1]);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(LINE_WIDTH);
			glDrawElements(GL_TRIANGLES, sizeof(outerIndices) / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
		// clear all relevant buffers
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		plainShader.setMat4("view", view);
		plainShader.setVec3("colour", CORE_COLOR);
		glBindVertexArray(gramVAOs[0]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, sizeof(innerIndices) / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
		// then we draw the second triangle using the data from the second VAO
		// when we draw the second triangle we want to use a different shader program so we switch to the shader program with our yellow fragment shader.
		plainShader.setVec3("colour", LINE_COLOR);
		glBindVertexArray(gramVAOs[1]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(LINE_WIDTH);
		glDrawElements(GL_TRIANGLES, sizeof(outerIndices) / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// render cube
		// -----------
		// activate shader
		reflectShader.use();
		reflectShader.setVec3("colour", CUBE_COLOR);
		reflectShader.setVec3("cameraPos", CAMERA_POS);
		reflectShader.setMat4("view", view);
		reflectShader.setMat4("projection", projection);

		// create transformations
		model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first	
		model = glm::translate(model, TRANSLATE_CUBE);
		model = glm::scale(model, SCALE_CUBE);
		model = glm::rotate(model, (float)(glfwGetTime() / 10), glm::vec3(0.5f, 1.0f, 0.0f));
		
		// retrieve the matrix uniform locations
		// pass them to the shaders (3 different ways)
		// note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
		reflectShader.setMat4("model", model);

		// render box
		glBindVertexArray(cubeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);		
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// render sphere
		// -------------
		textShader.use();
		textShader.setVec3("lightColor", LIGHT_COLOR);
		textShader.setVec3("lightPos", LIGHT_POS);
		textShader.setVec3("viewPos", CAMERA_POS);
		textShader.setVec3("colour", SPHERE_COLOR);
		textShader.setInt("texture", 0);

		// view/projection transformations
		textShader.setMat4("projection", projection);
		textShader.setMat4("view", view);

		// world transformation
		model = glm::mat4(1.0f);
		model = glm::translate(model, TRANSLATE_SPHERE);
		model = glm::scale(model, SCALE_SPHERE);
		textShader.setMat4("model", model);

		// render the cube
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(sphereVAO);
		glDrawArrays(GL_TRIANGLES, 0, vertexSize);

		shadowShader.use();
		shadowShader.setMat4("projection", projection);
		shadowShader.setMat4("view", view);
		shadowShader.setMat4("model", model);
		shadowShader.setFloat("surfaceY", SURFACE_Y);
		shadowShader.setVec3("lightPos", LIGHT_POS);
		glBindVertexArray(sphereVAO);
		glDrawArrays(GL_TRIANGLES, 0, vertexSize);

		plainShader.use();
		plainShader.setVec3("colour", LIGHT_COLOR);
		plainShader.setMat4("projection", projection);
		plainShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, LIGHT_POS);
		model = glm::scale(model, glm::vec3(0.05f));
		plainShader.setMat4("model", model);

		glBindVertexArray(sphereVAO);
		glDrawArrays(GL_TRIANGLES, 0, vertexSize);

		plainShader.use();
		plainShader.setVec3("colour", LIGHT_COLOR);
		plainShader.setMat4("projection", projection);
		plainShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, SURFACE_Y - 0.01f, 0.0f));
		model = glm::scale(model, SCALE_SURFACE);
		plainShader.setMat4("model", model);

		glBindVertexArray(surfaceVAO);
		glDrawArrays(GL_TRIANGLES, 0, sizeof(surfaceVertices) / sizeof(float) / 3);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void norm(float* v, float mod) {
	float omod = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	float scale = mod / omod;
	v[0] *= scale;
	v[1] *= scale;
	v[2] *= scale;
}

void copyTri(float* dst, float* v0, float* v1, float* v2) {
	memcpy(dst, v0, 3 * sizeof(float));
	memcpy(dst + 3, v1, 3 * sizeof(float));
	memcpy(dst + 6, v2, 3 * sizeof(float));
}
