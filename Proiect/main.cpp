//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <random>
#include <vector>

int glWindowWidth = 800;
int glWindowHeight = 600;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

// a higher value will result in a more detailed shadow map
const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;

glm::mat4 model;
GLuint modelLoc;

glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

gps::Camera myCamera(
				glm::vec3(0.0f, 2.0f, 5.5f), 
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.1f;

bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle;

gps::Model3D nanosuit;
gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D screenQuad;

gps::Model3D scena;
gps::Model3D balon;
gps::Model3D masina;
gps::Model3D masina2;

gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap;
bool showLight = true;

// New skybox texture
GLuint textureID;

gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

//animatie balon
// blender = (25, 0, 17) -> openGL = (25, 17, 0)
glm::vec3 balonPosition = glm::vec3(25.0f, 17.0f, 0.0f);
float balonAngle = 0.0f;
float balonHeight = 0.0f; 

//animatie masina 1
float carPosition = 0.0f;
float carSpeed = 0.1f;
// blender = (-7, 35, 
glm::vec3 masinaPosition = glm::vec3(-7.0f, 35.0f, 0.0f);

//animatie masina 2
//blender = (45,7,0.1) + rotate pe y cu -90
float carPosition2 = 0.0f;
float carSpeed2 = 0.3f;

float fogDensityFactor = 0.01f;
const float MIN_FOG = 0.001f;
const float MAX_FOG = 0.05f;
const float FOG_STEP = 0.001f;

float animationTime = 0.0f;
bool isAnimating = false;
const float ANIMATION_DURATION = 20.0f; // 20 seconds animation

glm::vec3 mix(const glm::vec3& a, const glm::vec3& b, float t) {
	return a * (1.0f - t) + b * t;
}

float startTime;
float totalDuration = 15.0f;

void animateCamera() { 
	float currentTime = glfwGetTime() - startTime;

	std::vector<glm::vec3> positions = {
		glm::vec3(387.0f, 93.0f, 382.0f),
		glm::vec3(7.0f, 10.0f, 70.0f),
		glm::vec3(7.0f, 10.0f, -70.0f),
		glm::vec3(155.02f, 80.0f, -206.0f)
	};

	std::vector<glm::vec3> targets = {
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.2f, -0.08f, -0.49f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(-0.63f, -0.15f, 0.77f)
	};

	if (currentTime >= totalDuration) {
		// Set final position and target
		//                                 st-dr  sus-jos fata-spate
		myCamera.setCameraPosition(glm::vec3(0.0f, 15.0f, 35.0f));
		myCamera.setCameraTarget(glm::vec3(0.0f, 0.0f, -5.0f));
		isAnimating = false;
		return;
	}

	float normalizedTime = currentTime / totalDuration;
	float segmentTime = normalizedTime * 4.0f;
	int currentIndex = (int)segmentTime;
	float t = segmentTime - currentIndex;

	int nextIndex = (currentIndex + 1) % 4;

	glm::vec3 newPosition = glm::mix(positions[currentIndex], positions[nextIndex], t);
	glm::vec3 newTarget = glm::normalize(glm::mix(targets[currentIndex], targets[nextIndex], t));

	myCamera.setCameraPosition(newPosition);
	myCamera.setCameraTarget(newTarget);
}

struct RainParticle {
	glm::vec3 position;
	glm::vec3 velocity;
	float randomOffset;  // Add random offset for varied falling
};

const int NUM_PARTICLES = 5000;
std::vector<RainParticle> rainParticles;
GLuint rainVBO, rainVAO;
gps::Shader rainShader;
bool showRain = false;
float lastFrame = 0.0f;

void initRain() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> xDist(-500.0f, 500.0f);
	std::uniform_real_distribution<float> yDist(0.0f, 500.0f);
	std::uniform_real_distribution<float> zDist(-500.0f, 500.0f);
	std::uniform_real_distribution<float> randomOffset(0.0f, 100.0f);  // Random offset for varied falling

	rainParticles.resize(NUM_PARTICLES);
	for (auto& particle : rainParticles) {
		// Random starting position
		particle.position = glm::vec3(xDist(gen), yDist(gen), zDist(gen));
		// Add slight random horizontal movement
		float randX = (gen() % 100) / 1000.0f - 0.05f;
		float randZ = (gen() % 100) / 1000.0f - 0.05f;
		particle.velocity = glm::vec3(randX, -1.0f, randZ);
		particle.randomOffset = randomOffset(gen);
	}

	// Create and bind VAO
	glGenVertexArrays(1, &rainVAO);
	glBindVertexArray(rainVAO);

	// Create and bind VBO
	glGenBuffers(1, &rainVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(RainParticle) * NUM_PARTICLES, rainParticles.data(), GL_DYNAMIC_DRAW);

	// Set vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RainParticle), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RainParticle), (void*)offsetof(RainParticle, velocity));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(RainParticle), (void*)offsetof(RainParticle, randomOffset));
}

// Add this function to render rain
void renderRain() {
	if (!showRain) return;

	float currentFrame = glfwGetTime();
	float deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Update and render particles
	rainShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1f(glGetUniformLocation(rainShader.shaderProgram, "deltaTime"), deltaTime);
	glUniform1f(glGetUniformLocation(rainShader.shaderProgram, "currentTime"), currentFrame);

	glBindVertexArray(rainVAO);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
	glDisable(GL_PROGRAM_POINT_SIZE);
}

GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	glWindowWidth = width;
	glWindowHeight = height;
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
	glViewport(0, 0, retina_width, retina_height);
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
}

int representation = 0;

void switchSceneRepresentation() {
	representation = (representation + 1) % 3;

	if (representation == 0) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (representation == 1) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}
}

bool isFullscreen = false;
GLFWmonitor* primaryMonitor = NULL;
int windowedWidth, windowedHeight, windowedPosX, windowedPosY;

void toggleFullscreen() {
	if (!isFullscreen) {
		// Store current windowed mode parameters
		glfwGetWindowPos(glWindow, &windowedPosX, &windowedPosY);
		glfwGetWindowSize(glWindow, &windowedWidth, &windowedHeight);

		// Get primary monitor
		primaryMonitor = glfwGetPrimaryMonitor();

		// Get video mode of primary monitor
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

		// Switch to fullscreen
		glfwSetWindowMonitor(glWindow, primaryMonitor,
			0, 0,
			mode->width, mode->height,
			mode->refreshRate);

		isFullscreen = true;
	}
	else {
		// Switch back to windowed mode
		glfwSetWindowMonitor(glWindow, NULL,
			windowedPosX, windowedPosY,
			windowedWidth, windowedHeight,
			0);

		isFullscreen = false;
	}
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		switchSceneRepresentation();

	if (key == GLFW_KEY_F && action == GLFW_PRESS)
		toggleFullscreen();

	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		showRain = !showRain;

	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		showLight = !showLight;


	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		startTime = glfwGetTime();
		isAnimating = !isAnimating;
		animationTime = 0.0f;
		myCamera.setCameraPosition(glm::vec3(0.0f, 2.0f, 5.0f));
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

float lastX = glWindowWidth / 2.0f;
float lastY = glWindowHeight / 2.0f;
bool firstMouse = true;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // invers pentru că y crește în sus
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	static float yaw = -90.0f;
	static float pitch = 0.0f;
	yaw += xoffset;
	pitch += yoffset;

	// limitarea unghiului pitch
	//if (pitch > 89.0f) pitch = 89.0f;
	//if (pitch < -89.0f) pitch = -89.0f;
	pitch = glm::clamp(pitch, -89.0f, 89.0f);

	myCamera.rotate(pitch, yaw);
}

void processMovement()
{
	if (pressedKeys[GLFW_KEY_Q]) {
		angleY -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angleY += 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle -= 1.0f;		
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);		
	}

	if (pressedKeys[GLFW_KEY_SPACE]) {
		myCamera.move(gps::MOVE_UP, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_LEFT_SHIFT]) {
		myCamera.move(gps::MOVE_DOWN, cameraSpeed);
	}

	//fog
	if (pressedKeys[GLFW_KEY_C]) {
		fogDensityFactor = glm::min(fogDensityFactor + FOG_STEP, MAX_FOG);
	}

	if (pressedKeys[GLFW_KEY_X]) {
		fogDensityFactor = glm::max(fogDensityFactor - FOG_STEP, MIN_FOG);
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    
    //window scaling for HiDPI displays
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    //for sRBG framebuffer
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    //for antialising
    glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);

	// decomenting this so we can rotate fully the camera
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (__APPLE__)
    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {
	nanosuit.LoadModel("objects/nanosuit/nanosuit.obj");
	lightCube.LoadModel("objects/cube/cube.obj");
	screenQuad.LoadModel("objects/quad/quad.obj");
	scena.LoadModel("objects/scena/scena.obj");
	balon.LoadModel("objects/balon/balon.obj");
	masina.LoadModel("objects/masina/masina.obj");
	masina2.LoadModel("objects/masina2/masina2.obj");


	// New skybox initialization
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	
	std::vector<const GLchar*> faces;
	faces.push_back("skybox/right.tga");
	faces.push_back("skybox/left.tga");
	faces.push_back("skybox/top.tga");
	faces.push_back("skybox/bottom.tga");
	faces.push_back("skybox/back.tga");
	faces.push_back("skybox/front.tga");
	mySkyBox.Load(faces);
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
	depthMapShader.useShaderProgram();
	//rain
	rainShader.loadShader("shaders/rainShader.vert", "shaders/rainShader.frag");
	rainShader.useShaderProgram();

	//
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");	
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//fog
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensityFactor"), fogDensityFactor);
}

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
	glGenFramebuffers(1, &shadowMapFBO);

	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	// Return the light-space transformation matrix
	glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	const GLfloat near_plane = -50.0f, far_plane = 50.0f;

	//                                     stanga  dreapta  jos    sus
	glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -40.0f, 40.0f, near_plane, far_plane);

	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	return lightSpaceTrMatrix;
}

void drawObjects(gps::Shader shader, bool depthPass) {
		
	shader.useShaderProgram();

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	
	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.5f));
	model = glm::translate(model, glm::vec3(0.0f, 1.1f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	nanosuit.Draw(shader);

	carPosition += carSpeed;  // Mașina se mișcă în jos (Y scade)

	// Verifică dacă a ajuns la limita de jos
	if (carPosition >= 65.0f) {
		carPosition = 0.0f;  // Reset la poziția inițială
	}
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, carPosition));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	masina.Draw(shader);

	carPosition2 -= carSpeed2;  // Mașina se mișcă în jos (Y scade)

	// Verifică dacă a ajuns la limita de jos
	if (carPosition2 <= -65.0f) {
		carPosition2 = 0.0f;  // Reset la poziția inițială
	}
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(carPosition2, 0.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	masina2.Draw(shader);

	balonAngle += 0.2f;
	balonHeight = sin(glfwGetTime() * 0.5f) * 7.0f;
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, balonHeight, 0.0f));
	model = glm::rotate(model, glm::radians(balonAngle), glm::vec3(0.0f, 1.0f, 0.0f)); 
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));	
	balon.Draw(shader);

	model = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	scena.Draw(shader);
}

void renderScene() {

	// depth maps creation pass
	//TODO - Send the light-space transformation matrix to the depth map creation shader and
	//		 render the scene in the depth map

	//render the scene to the depth buffer
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	drawObjects(depthMapShader, true);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // aici se scoate fbo ul de shadow map
	
	// render depth map on screen - toggled with the M key

	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {

		// final scene rendering pass (with shadows)

		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
				
		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(myCustomShader, false);

		//fog
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensityFactor"), fogDensityFactor);

		//showLight
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "showLight"), showLight);

		//draw a white cube around the light
		lightShader.useShaderProgram();

		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = lightRotation;
		model = glm::translate(model, 1.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);

		//rain
		renderRain();

		glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
		mySkyBox.Draw(skyboxShader, skyboxView, projection);
	}
}

void cleanup() {
	glDeleteVertexArrays(1, &rainVAO);
	glDeleteBuffers(1, &rainVBO);
	glDeleteTextures(1,& depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char * argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();
	initRain();

	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();
		renderScene();		

		float currentFrame = glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (isAnimating)
			animateCamera();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();

	return 0;
}
