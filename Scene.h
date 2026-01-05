#ifndef SCENE_H
#define SCENE_H 

#include <vector>
#include <string>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "ShadowMap.h"

struct Light
{
	glm::vec3 lightPos;
	glm::vec3 lightColor;
};

class Scene
{
public:
	Camera camera;
	glm::mat4 view;

	std::vector<Model*> sceneModels;
	std::vector<std::string> modelFiles;

	Model* modelTest = new Model("Models/test/test.fbx");

	//Shader shader = Shader("pbr.vert", "pbr.frag");
	Shader shader = Shader("vertex.vert", "fragment.frag");
	Shader cubeVis = Shader("bbVisual.vert", "bbVisual.frag");
	Shader screenShader = Shader("frame.vert", "frame.frag");

	std::vector<Light> lights;

	unsigned int WIDTH = 1280;
	unsigned int HEIGHT = 720;

	ShadowMap* shadowMap = nullptr;
	IBL ibl = IBL();

	Scene()
	{
		camera = Camera(glm::vec3(0.0f, 1.0f, 2.0f));

		Light light;
		light.lightColor = glm::vec3(1.0f);
		light.lightPos = glm::vec3(-10.0f, 6.0f, 10.0f);
		lights.push_back(light);

		modelFiles.push_back("Models/test/test.fbx");
		modelFiles.push_back("Models/plane/plane.obj");
		modelFiles.push_back("Models/plane/plane.obj");
		modelFiles.push_back("Models/plane/plane.obj");
		modelFiles.push_back("Models/plane/plane.obj");
		modelFiles.push_back("Models/plane/plane.obj");
		modelFiles.push_back("Models/plane/plane.obj");

		for (unsigned int i = 0; i < modelFiles.size(); i++)
		{
			sceneModels.push_back(new Model(modelFiles[i]));
		}
		for (unsigned int i = 0; i < sceneModels.size(); i++)
		{
			sceneModels[i]->applyData();
		}

		modelTest->applyData();

		setPosition();
		setShadow();

		ibl.initCubeFromHDR("HDRI/photo_studio_loft_hall_4k.hdr");
		shader.use();
	}

	void setPosition()
	{
		sceneModels[0]->scale(1.0f);
		sceneModels[0]->move(0.0f, 1.0f, 0.0f);
		sceneModels[0]->rotate(-90.0f, X);
		sceneModels[0]->setModelDynamic_ON();

		sceneModels[1]->move(0.0f, -1.0f, 0.0f);
		sceneModels[1]->scale(7.0f);

		sceneModels[2]->move(-3.0f, 2.0f, 0.0f);
		sceneModels[2]->scale(3.0f);
		sceneModels[2]->rotate(-90.0f, Z);

		sceneModels[3]->move(0.0f, 5.0f, 0.0f);
		sceneModels[3]->scale(3.0f);
		sceneModels[3]->rotate(-180.0f, Z);

		sceneModels[4]->move(0.0f, 2.0f, -3.0f);
		sceneModels[4]->scale(3.0f);
		sceneModels[4]->rotate(90.0f, X);

		sceneModels[5]->move(3.0f, 2.0f, 0.0f);
		sceneModels[5]->scale(3.0f);
		sceneModels[5]->rotate(90.0f, Z);

		sceneModels[6]->move(0.0f, 2.5f, 3.0f);
		sceneModels[6]->rotate(45.0f, X);
	}

	void setShadow()
	{
		shadowMap = new ShadowMap(lights[0].lightPos);
	}

	void renderShadow()
	{
		shadowMap->bindShadow();
		/*for (unsigned int i = 0; i < sceneModels.size(); i++)
		{
			sceneModels[i]->draw(shader);
		}*/

		modelTest->draw(shadowMap->shader);
	}

	void render()
	{
		shader.use();
		/*shader.setVec3("viewPos", camera.Position);

		view = camera.GetViewMatrix();

		shader.setMat4("projection", camera.projection);
		shader.setMat4("view", view);
		shader.setVec3("camPos", camera.Position);
		shader.setMat4("lightSpaceMatrix", shadowMap->lightSpaceMatrix);
		shader.setVec3("lightPositions[0]", lights[0].lightPos);
		shader.setVec3("lightColors[0]", glm::vec3(100.0f));

		shader.setInt("irradianceMap", 1);
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

		shader.setMat4("projection", camera.projection);
		view = camera.GetViewMatrix();
		shader.setMat4("view", view);
		shader.setVec3("viewPos", camera.Position);
		std::cout << camera.Position.x << " " << camera.Position.y << " " << camera.Position.z << std::endl;

		shader.setMat4("lightSpaceMatrix", shadowMap->lightSpaceMatrix);
		shader.setVec3("lightPos", lights[0].lightPos);
		std::cout << lights[0].lightPos.x << " " << lights[0].lightPos.y << " " << lights[0].lightPos.z << std::endl;
		shader.setVec3("lightColor", lights[0].lightColor);
		std::cout << lights[0].lightColor.x << " " << lights[0].lightColor.y << " " << lights[0].lightColor.z << std::endl;

		shader.setInt("shadowMap", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowMap->depthMap);

		for (unsigned int i = 0; i < sceneModels.size(); i++)
		{
			sceneModels[i]->draw(shader, 0);
		}
	}

};


#endif // !SCENE_H