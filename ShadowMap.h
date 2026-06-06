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
	unsigned int shadowRes = 4096;

	glm::mat4 lightProjection, lightView, lightSpaceMatrix;
	glm::vec3 lightPos;
	float near_plane = 0.1f, far_plane = 40.0f;
	float frustrum = 40.0f;

	Shader shader = Shader("shadow.vert", "shadow.frag");
	Shader debugging = Shader("shadow_debug.vert", "shadow_debug.frag");

	ShadowMap(){}

	ShadowMap(glm::vec3 _lightPos) 
	{ 
		lightPos = _lightPos;
		
		//lightProjection = glm::ortho(-frustrum, frustrum, -frustrum, frustrum, near_plane, far_plane);
		lightProjection = glm::perspective(glm::radians(120.0f), static_cast<float>(shadowRes), 0.1f, 100.0f);
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

	void bindShadowMap(std::vector<Model*> models)
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		shader.use();

		//lightProjection = glm::ortho(-frustrum, frustrum, -frustrum, frustrum, near_plane, far_plane);
		lightProjection = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		lightSpaceMatrix = lightProjection * lightView;

		shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, shadow_WIDTH, shadow_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (int i = 0; i < models.size(); i++)
			models[i]->drawShadow(shader);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void shadowDebug()
	{
		debugging.use();
		debugging.setFloat("near_plane", near_plane);
		debugging.setFloat("far_plane", far_plane);

		debugging.setInt("depthMap", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		renderQuad();
	}

private:
	unsigned int quadVAO = 0;
	unsigned int quadVBO;
	void renderQuad()
	{
		if (quadVAO == 0)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}
};

#endif