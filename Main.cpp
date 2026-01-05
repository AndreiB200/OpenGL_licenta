#include <iostream>
#include <thread>
#include <mutex>
#include <windows.h>

//#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "stb_image.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Imgui_layer.h"
#include "Thread_Pool.h"

#include <assimp/camera.h>

#include <btBulletDynamicsCommon.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Porsche_texture.h"
#include "StructuresHelpers.h"
#include "IBL.h"
#include "Physics.h"
#include "Texture.h"
#include "Scene.h"
#include "Window.h"
#include "OpenGL_Settings.h"
#include "ShadowMap.h"

#include "PrimitiveObj.h"

//Scene
unsigned int WIDTH = 1280;
unsigned int HEIGHT = 720;

//Scene
//Camera camera(glm::vec3(0.0f, 1.0f, 2.0f));

float deltaTime = 0.0f;
float lastFrame = 0.0f;


void createDepthMap(unsigned int& depthMapFBO, unsigned int& depthMap, unsigned int& shadow_WIDTH, unsigned int& shadow_HEIGHT)
{
	shadow_WIDTH = shadow_HEIGHT = 2048;
	glGenFramebuffers(1, &depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_WIDTH, shadow_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0,1.0,1.0,1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


int main()
{	
	OpenGL_Settings::getInstance().contextVersions();

	Window window(WIDTH, HEIGHT, "OpenGL window");

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	/*camera.setPerspective(glm::perspective(glm::radians(100.0f), (float)WIDTH / (float)HEIGHT, 0.01f, 200.0f));
	camera.bindWindow(window.window);
	
	window.bindCamera(&camera);*/

	OpenGL_Settings::getInstance().enableDepth();

	stbi_set_flip_vertically_on_load(true);

	////////////////////////////////////////////
	//Scene *scene = new Scene();
	//scene->generateStandardModels();
	
	
	glfwSwapInterval(0);

	/////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////Render starts
	
	//Scene
	ShadowMap* shadowMap = new ShadowMap(glm::vec3(10.0f));	

	int scrWidth, scrHeight;
	glfwGetFramebufferSize(window.window, &scrWidth, &scrHeight);
	glViewport(0, 0, scrWidth, scrHeight);

	//ibl.moveCamera(0.0f, 2.0f, 0.0f);



	stbi_set_flip_vertically_on_load(false);
	Scene scene = Scene();
	
	scene.camera.setPerspective(glm::perspective(glm::radians(100.0f), (float)WIDTH / (float)HEIGHT, 0.01f, 200.0f));
	scene.camera.bindWindow(window.window);

	window.bindCamera(&scene.camera);

	//ourModel->buildTexture("Models/Sponza/textures", "Models/Sponza/sponza_textures.txt");
	//Texture_Values material;
	
	Physics movement;
	movement.createBB(glm::vec3(-3.0f,-1.0f,-3.0f), glm::vec3(3.0f));
	movement.setSpeed(glm::vec3(1.5f, 0.0f, 0.0f));

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	bool start = 0; bool animate = 0; unsigned int irradiare = 0;
	unsigned int face = 0;
	float multi = 0.2f, s_X = 0.0f, s_Y = 0.0f, s_Z = 0.0f;
	glm::vec3 speed = glm::vec3(7.5f,4.0f,4.5f);
	float degree = 0.0f; float rotate = 45.0f;

	bool animation = false, Shadow = false, getShadow = false;
	float distance = 2.5f;
	float height = 2.5f;
	float specValue = 0.0f;

	PrimitiveObj primitive;

	glm::vec3 lightPos = glm::vec3(6.0f);

	float time = 1.0f;
	Imgui_layer::getInstance().Init(window.window);

	Imgui_layer::getInstance().addWidget(new Value("FPS: ", &time));


	while (!glfwWindowShouldClose(window.window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		time = 1.0f / deltaTime;
		OpenGL_Settings::getInstance().enableCullFace(0);

		/*if (face == 6)
		{
			start = 0;
			face = 0;
		}*/
		
		window.cursorActivate();
		scene.camera.processInput(deltaTime);

		OpenGL_Settings::getInstance().glDefaultColorBuffer();

		//Shadow start
		
		//Draw to shadow map
		scene.renderShadow();

		OpenGL_Settings::getInstance().defaultFBO(); //reset framebuffer here we need OpenGL settings

		//OpenGL_Settings::getInstance().glDefaultColorBuffer();
		
		
		glViewport(0, 0, scrWidth, scrHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		//Shadow end
		glEnable(GL_DEPTH_TEST);




		//Render scene
		//camera.keyListener(0.1f);
		scene.camera.keyListener(0.1f);
		

		float camX = sin(float(glfwGetTime())) * distance;
		float camZ = cos(float(glfwGetTime())) * distance;

		//shader.use();
		//shader.setVec3("viewPos", camera.Position);
		//view = camera.GetViewMatrix();

		/*if (!start)
		{
			shader.setMat4("projection", camera.projection);
			shader.setMat4("view", view);
		}*/


		/*shader.setFloat("multi", multi);
		shader.setVec3("camPos", camera.Position);
		shader.setMat4("lightSpaceMatrix", shadowMap->lightSpaceMatrix);
		shader.setVec3("lightPositions[0]", lightPos);
		shader.setVec3("lightColors[0]", glm::vec3(100.0f));*/

		/*shader.setInt("irradianceMap", 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.irradianceMap);

		shader.setInt("prefilterMap", 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.prefilterMap);

		shader.setInt("brdfLUT", 3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ibl.brdfLUTTexture);

		shader.setInt("shadowMap", 4);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, shadowMap->depthMap);*/



		//for (unsigned int i = 0; i < sizeModel; i++)
		//{
		//	/*shader.setInt("albedoMap", 5);
		//	glActiveTexture(GL_TEXTURE5);
		//	glBindTexture(GL_TEXTURE_2D, ourModel->texture[i].albedo);
		//	shader.setInt("normalMap", 6);
		//	glActiveTexture(GL_TEXTURE6);
		//	glBindTexture(GL_TEXTURE_2D, ourModel->texture[i].albedo);
		//	shader.setInt("metallicMap", 7);
		//	glActiveTexture(GL_TEXTURE7);
		//	glBindTexture(GL_TEXTURE_2D, ourModel->texture[i].albedo);
		//	shader.setInt("roughnessMap", 8);
		//	glActiveTexture(GL_TEXTURE8);
		//	glBindTexture(GL_TEXTURE_2D, ourModel->texture[i].roughness);*/

		//	ourModel->draw(shader, i);
		//}

		/*ibl.backgroundShader.use();
		ibl.backgroundShader.setMat4("projection", projection);
		ibl.backgroundShader.setMat4("view", view);
		ibl.backgroundShader.setInt("environmentMap", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.irradianceMap);
		renderCube();*/

		/*if (!start)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			cubeVis.use();
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(s_X, s_Y, s_Z));
			cubeVis.setMat4("projection", projection);
			cubeVis.setMat4("view", view);
			cubeVis.setMat4("model", model);
			renderCubeBB();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}*/
		
		scene.render();

		if (glfwGetKey(window.window, GLFW_KEY_1) == GLFW_PRESS)
		{
			getShadow = !getShadow;
			Sleep(250);
		}

		if (getShadow)
		{
			OpenGL_Settings::getInstance().renderFrame(scene.screenShader, scene.shadowMap->depthMap);
		}		

		Imgui_layer::getInstance().Update();

		glfwSwapBuffers(window.window);
		glfwPollEvents();
	}
	
	Imgui_layer::getInstance().ShutDown();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	threads.stop();

	glfwTerminate();
	return 0;
}