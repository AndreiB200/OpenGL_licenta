#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <vector>

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image_write.h"
//#include "stb_image.h"

#include "PrimitiveObj.h"
#include "Shader.h"

class OpenGL_Settings
{
public:
	static OpenGL_Settings& getInstance() 
	{
		static OpenGL_Settings instance;
		return instance;
	}

	OpenGL_Settings(const OpenGL_Settings&) = delete;
	void operator=(const OpenGL_Settings&) = delete;

	void contextVersions()
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	}

	void enableCullFace(bool culling)
	{
		if (culling == 1)
			glEnable(GL_CULL_FACE);
		if (culling == 0)
			glDisable(GL_CULL_FACE);
	}

	void enableDepth()
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}

	void glDefaultColorBuffer()
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void defaultFBO()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void saveImage(char* filepath, GLFWwindow* w)
	{
		int width, height;
		glfwGetFramebufferSize(w, &width, &height);
		GLsizei nrChannels = 3;
		GLsizei stride = nrChannels * width;
		stride += (stride % 4) ? (4 - stride % 4) : 0;
		GLsizei bufferSize = stride * height;
		std::vector<char> buffer(bufferSize);
		glPixelStorei(GL_PACK_ALIGNMENT, 4);
		glReadBuffer(GL_FRONT);
		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
		stbi_flip_vertically_on_write(true);
		stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	}

	void renderFrame(Shader& screenShader, unsigned int& depthMap) {
		screenShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		primitiveObj.renderQuad();
	}

private:
	PrimitiveObj primitiveObj;

	OpenGL_Settings(){}
};