#pragma once

#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Model.h"

class Physics
{
public:
	glm::vec3 speedObject = glm::vec3(0.0f);
	glm::vec3 bbMax, bbMin;
	float deltaTime;

	Physics() 
	{

	}

	void createBB(glm::vec3 i_Min, glm::vec3 i_Max)
	{
		bbMax = i_Max;
		bbMin = i_Min;
	}

	void setSpeed(glm::vec3 i_Speed)
	{
		speedObject = i_Speed;
	}

	void animate(Model& ourModel, glm::vec3& speed)
	{
		collision(ourModel, speed);
		ourModel.position = ourModel.position + (deltaTime * speed);
		ourModel.move();
	}

	void collision(Model& ourModel, glm::vec3& speed)
	{
		float size = 1.0f;
		if (ourModel.position.x + size > bbMax.x)
		{
			speed.x = -speed.x;
			ourModel.position.x = ourModel.position.x - ((ourModel.position.x + size) - bbMax.x);
		}
		if (ourModel.position.x - size < bbMin.x)
		{
			speed.x = -speed.x;
			ourModel.position.x = ourModel.position.x - ((ourModel.position.x - size) - bbMin.x);
		}

		if (ourModel.position.y + size > bbMax.y)
		{
			speed.y = -speed.y;
			ourModel.position.y = ourModel.position.y - ((ourModel.position.y + size) - bbMax.y);
		}
		if (ourModel.position.y + size < bbMin.y)
		{
			speed.y = -speed.y;
			ourModel.position.y = ourModel.position.y - ((ourModel.position.y - size) - bbMin.y);
		}

		if (ourModel.position.z + size > bbMax.z)
		{
			speed.z = -speed.z;
			ourModel.position.z = ourModel.position.z - ((ourModel.position.z + size) - bbMax.z);
		}
		if (ourModel.position.z + size < bbMin.z)
		{
			speed.z = -speed.z;
			ourModel.position.z = ourModel.position.z - ((ourModel.position.z - size) - bbMin.z);
		}
	}
};