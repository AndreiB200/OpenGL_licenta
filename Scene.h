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
//#include "IBL.h"

struct Light
{
	glm::vec3 lightPos;
	glm::vec3 lightColor;
};

class Scene
{
public:
	// Setters
	void setCamera()
	{
		camera = Camera(glm::vec3(0.0f, 0.0f, 10.0f));
	}

	void loadModels()
	{
		Model* newModel = new Model("Models/model.fbx");
		//Model newModel = Model("Models/Porsche/porsche.fbx");
		newModel->buildTexture("Models/Porsche", "Models/Porsche/textures.txt");
		newModel->textureAlpha = textures.texture2Dfile("Models/Porsche/BODY_alpha.png");
		newModel->rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));


		Model* floor = new Model("Models/Env/floor.fbx");
		floor->buildTexture("Models/Env", "Models/Env/textures_floor.txt");
		floor->rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));


		Model* metal = new Model("Models/Env/metal.fbx");
		metal->buildTexture("Models/Env", "Models/Env/textures_metal.txt");
		metal->rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));


		Model* proxy = new Model("Models/Porsche/porsche_proxy.fbx");
		proxy->rotate(-90, X);


		Model* plane = new Model("Models/Porsche/plane.fbx");

		plane->texture = newModel->texture;

		models = {
			newModel,
			//&proxy, 
			floor,
			metal,
			plane 
		};
	}

	void loadIBL()
	{
		ibl.initCubeFromHDR("HDRI/mappo.hdr");
	}

	void buildLights()
	{
		Light light;
		light.lightPos = glm::vec3(-0.0f, 0.0f, 0.0f);
		light.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		lights.push_back(light);
	}



	// Getters
	Camera& getCamera()
	{
		return camera;
	}
	
	std::vector<Model*> &getModelList()
	{
		return models;
	}

	std::vector<Light>& getLights()
	{
		return lights;
	}

	IBL& getIBL()
	{
		return ibl;
	}
private:
	Shader* shaders;
	Camera camera;
	std::vector<Model*> models;

	std::vector<Light> lights;

	IBL ibl;
};


#endif // !SCENE_H