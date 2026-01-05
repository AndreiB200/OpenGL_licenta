#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class ShadowMap
{
public:
	unsigned int depthMapFBO, depthMap, shadow_WIDTH, shadow_HEIGHT;
	unsigned int shadowRes = 2048;

	glm::mat4 lightProjection, lightView, lightSpaceMatrix;
	glm::vec3 lightPos;
	float near_plane = 0.0f, far_plane = 20.0f;
	float frustrum = 20.0f;

	Shader shader = Shader("shadow.vert", "shadow.frag");

	ShadowMap(glm::vec3 _lightPos) 
	{ 
		lightPos = _lightPos;
		
		lightProjection = glm::ortho(-frustrum, frustrum, -frustrum, frustrum, near_plane, far_plane);
		lightView = glm::lookAt(_lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		lightSpaceMatrix = lightProjection * lightView;		

		createDepthMap(depthMapFBO, depthMap, shadow_WIDTH, shadow_HEIGHT); 
	}

	void createDepthMap(unsigned int &depthMapFBO, unsigned int &depthMap, unsigned int &shadow_WIDTH, unsigned int &shadow_HEIGHT)
	{
		shadow_WIDTH = shadow_HEIGHT = shadowRes;

		glGenFramebuffers(1, &depthMapFBO);

		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_WIDTH, shadow_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

	void bindShadow()
	{
		shader.use();

		lightProjection = glm::ortho(-frustrum, frustrum, -frustrum, frustrum, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		lightSpaceMatrix = lightProjection * lightView;

		shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, shadow_WIDTH, shadow_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
};

#endif