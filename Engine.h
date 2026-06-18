#define ENGINE
#ifdef ENGINE

#include <GLFW/glfw3.h>
#include "Window.h"
#include "Scene.h"
#include "Imgui_layer.h"
#include "DebuggerClass.h"

class Engine
{
public:
	Engine(){}
	
	void createWindow()
	{
		Window myWindow = Window(WIDTH, HEIGHT, &scene.getCamera(), "OpenGL");
	}

	// Create PBR Shader, activate and set variables
	void createShader()
	{
		shader = Shader("pbr.vert", "pbr.frag");

		shader.use();
		shader.setInt("shadowMap", 0);

		shader.setInt("irradianceMap", 1);
		shader.setInt("prefilterMap", 2);
		shader.setInt("brdfLUT", 3);

		shader.setInt("albedoMap", 4);
		shader.setInt("metallicMap", 5);
		shader.setInt("normalMap", 6);
		shader.setInt("roughnessMap", 7);
		shader.setInt("alphaMap", 8);

		shader.setFloat("ao", 1.0f);
	}

	// The scene has the models already inside 
	void createScene()
	{
		scene.setCamera();
		scene.loadModels();
		scene.loadIBL();

		std::vector<Light> light = scene.getLights();
		shadow = ShadowMap(light[0].lightPos);
	}

	void buildScene()
	{
		imgui_helper.initImgui(myWindow);

		/*imgui_helper.attachdeltaTime(deltaTime);
		imgui_helper.attachLight(lightPositions[0]);
		imgui_helper.attachShadow(shadow);
		imgui_helper.attachModels(models);
		imgui_helper.attach_newModel(newModel);*/

		imgui_helper.setSlides();
	}

	void RenderLoop()
	{
		while (!glfwWindowShouldClose(myWindow.window))
		{
			float currentFrame = static_cast<float>(glfwGetTime());
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			input(deltaTime);
			// -> here we will have JOLT Physics in the future
			renderScene();

			glfwSwapBuffers(myWindow.window);
			glfwPollEvents();
		}
	}

	void input(float delatTime)
	{
		scene.getCamera().processInput(deltaTime);
	}

	void renderScene()
	{
		shadow.bindShadowMap(scene.getModelList());

		glViewport(0, 0, WIDTH, HEIGHT);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 projection = scene.getCamera().GetProjection();
		glm::mat4 view = scene.getCamera().GetViewMatrix();

		shader.use();

		// Shadow Matrix
		shader.setMat4("lightSpaceMatrix", shadow.lightSpaceMatrix);

		// imgui debug
		imgui_helper.setShader(shader);

		//IBL
		scene.getIBL().bind(1);

		shader.setVec3("lightPositions[0]", scene.getLights()[0].lightPos);
		shader.setVec3("lightColors[0]", scene.getLights()[0].lightColor);
		scene.getLights()[0].lightColor = glm::vec3(imgui_helper.lightMultiplayer);

		auto models = scene.getModelList();
		for (int i = 0; i < models.size(); i++)
			models[i]->draw(shader);

		scene.getIBL().drawBackground(view, projection);
	}

private:
	// Variables
	unsigned int WIDTH = 1280, HEIGHT = 720;
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	
	// Engine Objects
	Window myWindow;
	Shader shader;
	Scene scene;
	ShadowMap shadow;

	// Debugger
	DebuggerClass imgui_helper;
};

#endif //ENGINE