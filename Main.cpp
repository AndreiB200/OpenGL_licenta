#include <winsock2.h>
#include <ws2tcpip.h>

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
#include "Texture.h"
#include "Scene.h"
#include "Window.h"
#include "OpenGL_Settings.h"
#include "ShadowMap.h"

#include "PrimitiveObj.h"
#include "DebuggerClass.h"

#include "PhysicsEngine.h"
#include "Gamepad.h"
#include "NetworkStreamer.h"
#include "Drone.h"
#include "FrameCapturer.h"


//Scene
unsigned int WIDTH = 1280;
unsigned int HEIGHT = 720;

//Scene
//Camera camera(glm::vec3(0.0f, 1.0f, 2.0f));

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fps = 0.0f;
Camera camera(glm::vec3(0.0f, 3.0f, 10.0f));

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    xoffset = xpos - lastX;
    yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xpos, ypos);
}

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
             // bottom face
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
              1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             // top face
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
              1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
              1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
              1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}


GLuint gBuffer, gAlbedo; // GL_COLOR_ATTACHMENT0
GLuint depthTexture_drone, rbo_depth; // GL_COLOR_ATTACHMENT1
GLuint gPositionDepth; // GL_COLOR_ATTACHMENT2
GLuint gNormalRoughness; // GL_COLOR_ATTACHMENT3
GLuint gARM; // GL_COLOR_ATTACHMENT4
GLenum drawBuffers[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
void createColorAndDepth()
{
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // Albedo - 3
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gAlbedo, 0);

    // Position depth - 3
    glGenTextures(1, &gPositionDepth);
    glBindTexture(GL_TEXTURE_2D, gPositionDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, WIDTH, HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gPositionDepth, 0);

    // Normal + Roughness Texture - 3
    glGenTextures(1, &gNormalRoughness);
    glBindTexture(GL_TEXTURE_2D, gNormalRoughness);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WIDTH, HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gNormalRoughness, 0);

    // gARM (without ambient occlusion on first pass)
    glGenTextures(1, &gARM);
    glBindTexture(GL_TEXTURE_2D, gARM);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gARM, 0);

    // Renderbuffer depth
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, WIDTH, HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

    glDrawBuffers(5, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Framebuffer INCOMPLETE/NON-VALID!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint ssrFBO;
GLuint ssrTexture;
void createSSR_framebuffer()
{
    glGenFramebuffers(1, &ssrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);

    glGenTextures(1, &ssrTexture);
    glBindTexture(GL_TEXTURE_2D, ssrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssrTexture, 0);

    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuf);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: SSR Framebuffer INCOMPLETE!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint pbrFBO;
GLuint pbrTexture, depthTextureSaved;
void createPBR_framebuffer()
{
    glGenFramebuffers(1, &pbrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pbrFBO);

    glGenTextures(1, &pbrTexture);
    glBindTexture(GL_TEXTURE_2D, pbrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pbrTexture, 0);

    glGenTextures(1, &depthTextureSaved);
    glBindTexture(GL_TEXTURE_2D, depthTextureSaved);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); // GL_RGBA before
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, depthTextureSaved, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

    GLenum drawBuf[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuf);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: PBR Framebuffer INCOMPLETE!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint noiseTexture;
GLuint ssaoFBO, ssaoBlurFBO, ssaoTexture, ssaoBlured;
void createSSAOandSamples(std::vector<glm::vec3> &ssaoKernel, std::vector<glm::vec3> &ssaoNoise, Shader &ssaoShader)
{
    glGenFramebuffers(1, &ssaoFBO);
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
    std::default_random_engine generator;

    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0, // X: [-1, 1]
            randomFloats(generator) * 2.0 - 1.0, // Y: [-1, 1]
            randomFloats(generator)              // Z: [0, 1] (emisferă)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // Concetrare mai mare de sample-uri aproape de origine
        float scale = float(i) / 64.0f;
        scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
        sample *= scale;

        ssaoKernel.push_back(sample);
    }

    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f
        );
        ssaoNoise.push_back(noise);
    }

    ssaoShader.use();
    for (unsigned int i = 0; i < 64; ++i) {
        ssaoShader.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &ssaoTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, WIDTH, HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoBlured);
    glBindTexture(GL_TEXTURE_2D, ssaoBlured);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, WIDTH, HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlured, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

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
void renderColorAndDepth(Window &myWindow, Shader &quadShader)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // check FBO
    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    quadShader.use();
    quadShader.setInt("gAlbedo", 0);
    quadShader.setInt("depthTexture", 1);
    quadShader.setInt("gPositionDepth", 2);
    quadShader.setInt("gNormalRoughness", 3);
    quadShader.setInt("ssrTexture", 4);
    quadShader.setInt("ssaoTexture", 5);
    quadShader.setInt("ssaoBlurTexture", 6);


    if (glfwGetKey(myWindow.window, GLFW_KEY_J) == GLFW_PRESS)
        quadShader.setInt("u_mode", 1);
    else if (glfwGetKey(myWindow.window, GLFW_KEY_K) == GLFW_PRESS)
        quadShader.setInt("u_mode", 2);
    else if (glfwGetKey(myWindow.window, GLFW_KEY_L) == GLFW_PRESS)
        quadShader.setInt("u_mode", 3);
    else if (glfwGetKey(myWindow.window, GLFW_KEY_P) == GLFW_PRESS)
        quadShader.setInt("u_mode", 4);
    else if (glfwGetKey(myWindow.window, GLFW_KEY_O) == GLFW_PRESS)
        quadShader.setInt("u_mode", 5);
    else if (glfwGetKey(myWindow.window, GLFW_KEY_I) == GLFW_PRESS)
        quadShader.setInt("u_mode", 6);
    else if (glfwGetKey(myWindow.window, GLFW_KEY_H) == GLFW_PRESS)
        quadShader.setInt("u_mode", 7);
    else
        quadShader.setInt("u_mode", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pbrTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTextureSaved);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gPositionDepth);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gNormalRoughness);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, ssrTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, ssaoBlured);

    renderQuad();
}


int main()
{	
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window myWindow = Window(WIDTH, HEIGHT, &camera, "OpenGL");


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);  
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    PhysicsEngine::getInstance().init();


    Shader pbrShader("pbr.vert", "pbr.frag");
    Shader debug("bbVisual.vert", "bbVisual.frag");
    Shader quadShader("frame.vert", "frame.frag");

    Shader geometryShaderBuffer("screenbuffer.vert", "screenbuffer.frag");
    Shader ssrShader("ssr.vert", "ssr.frag");
    Shader ssaoShader("ssao.vert", "ssao.frag");
    Shader ssaoBlurShader("ssaoBlur.vert", "ssaoBlur.frag");
    

    IBL ibl = IBL();
    ibl.initCubeFromHDR("HDRI/outside.hdr");


    Model floor = Model("Models/Env/floor.fbx");
    floor.buildTexture("Models/Env", "Models/Env/textures_floor.txt");
    floor.rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));

    Model depthScene = Model("Models/Env/depth_test.fbx");
    depthScene.buildTexture("Models/Env", "Models/Env/textures_floor.txt");
    depthScene.rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));
    depthScene.move(0.0f, 15.0f, 0.0f);

    Model newModel = Model("Models/model.fbx", true);
    newModel.buildTexture("Models/Env", "Models/Env/textures_floor.txt");
    newModel.move(5.0f, 3.0f, 0.0f);
    newModel.applyPhysicsConvexHull();
    newModel.applyPhysicsMatrix(true);


    Model metal = Model("Models/Env/metal.fbx");
    metal.buildTexture("Models/Env", "Models/Env/textures_metal.txt");
    metal.rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));


    Model proxy = Model("Models/Porsche/porsche_proxy.fbx");
    proxy.rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));


    Model plane = Model("Models/Porsche/plane.fbx");
    plane.texture = newModel.texture;



    Model drone = Model("Models/Drone/drone.fbx", true);
    drone.buildTexture("Models/Drone", "Models/Drone/textures_drone.txt");
    drone.move(-2.0f, 5.0f, 1.0f);
    drone.scale(0.5f);
    drone.applyPhysicsConvexHull();
    drone.applyPhysicsMatrix(true);

    Model propeler = Model("Models/Drone/propeler.fbx");
    propeler.scale(0.5f);

    propeler.texture = newModel.texture;

    Model soldier = Model("Models/Soldier/soldier.fbx");
    soldier.buildTexture("Models/Soldier", "Models/Soldier/textures_soldier.txt");
    soldier.rotate_Q(glm::vec3(-90.0f, -90.0f, 0.0f));
    soldier.scale(3.0f);
    soldier.move(10.0f, 0.0f, 0.0f);

    Model human = Model("Models/human/human.fbx");
    human.buildTexture("Models/human", "Models/human/textures.txt");
    human.rotate_Q(glm::vec3(-90.0f, -90.0f, 0.0f));
    human.scale(0.61f);
    human.move(10.0f, 0.0f, 6.0f);

    //Model porsche = Model("Models/Porsche/porsche.fbx");
    //porsche.buildTexture("Models/Porsche", "Models/Porsche/textures.txt");
    //porsche.textureAlpha = textures.texture2Dfile("Models/Porsche/BODY_alpha.png");
    //porsche.rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));


    std::vector<Model*> models = { 
        &newModel, 
        //&proxy, 
        &floor,
        &depthScene,
        &metal, 
        &plane,
        &drone,
        &soldier,
        &human,
        //&propeler
    };


    // lights
    // ------
    glm::vec3 lightPositions[] = {
        glm::vec3(-3.0f,  5.0f, 3.0f)
    };
    glm::vec3 lightColors[] = {
        glm::vec3(1.0f, 1.0f, 1.0f)
    };
    

    ShadowMap shadow = ShadowMap(lightPositions[0]);

    std::vector<glm::vec3> ssaoKernel;
    std::vector<glm::vec3> ssaoNoise;

    createColorAndDepth();
    createPBR_framebuffer();
    createSSR_framebuffer();
    createSSAOandSamples(ssaoKernel, ssaoNoise, ssaoShader);
    FFmpegStreamer streamer = FFmpegStreamer("100.70.184.28", 5554, 1280, 720);
    std::vector<unsigned char> colorData(1280 * 720 * 4);
    std::vector<unsigned char> depthData(1280 * 720 * 4);
    FrameCapturer capturer(1280, 720);

    // initialize static shader uniforms before rendering
    // --------------------------------------------------
    glViewport(0, 0, WIDTH, HEIGHT);



    DebuggerClass imgui_helper;
    imgui_helper.initImgui(myWindow);

    imgui_helper.attachDeltaTimeAndFps(deltaTime, fps);
    imgui_helper.attachLight(lightPositions[0]);
    imgui_helper.attachShadow(shadow);
    imgui_helper.attachModels(models);
    imgui_helper.attach_newModel(newModel);

    imgui_helper.setSlides();
    Gamepad gamepad = Gamepad();
    gamepad.bindDebugger(&imgui_helper);


    //OpenGL_Settings::getInstance().enableCullFace(true);
    glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    camera.setPerspective(proj);

    bool start = false;
    unsigned int bakedMap = 0;


    pbrShader.setVec3("albedo", glm::vec3(0.5f, 0.0f, 0.0f));
    pbrShader.setFloat("ao", 1.0f);

    PrimitiveObj primObj;
    glm::vec3 cameraLocalOffset = glm::vec3(0.0f, 1.5f, 5.0f);
    glm::vec3 lookAtDrone = glm::vec3(0.0f, 0.0f, -1.0f);
    Camera droneAttachedCamera(drone.position + cameraLocalOffset);
    glm::mat4 projDrone = glm::perspective(glm::radians(60.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    droneAttachedCamera.setPerspective(projDrone);

    Drone droneSim = Drone(&myWindow, &drone, &propeler, &primObj, &imgui_helper);

    PhysicsEngine::getInstance().showBodyInfo(drone.physics_id);

    while (!glfwWindowShouldClose(myWindow.window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        fps = 1.0f / deltaTime;

        imgui_helper.setShader(pbrShader, geometryShaderBuffer);

        gamepad.update();

        // input
        // -----
        camera.processInput(deltaTime);

        // render
        // ------
        shadow.bindShadowMap(models);

        glViewport(0, 0, WIDTH, HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer); // check FBO
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // render scene, supplying the convoluted irradiance map to the final shader.
        // ------------------------------------------------------------------------------------------
        glm::mat4 projection = camera.GetProjection();
        glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 camPosition = camera.Position;

        // activate PBR Shader
        // ------------------------------------------------------------------------------------------
        //pbrShader.use();
        //pbrShader.setVec3("u_albedo", glm::vec3(0.7f, 0.7f, 0.7f));
        //// set SHADOW Map 0
        //pbrShader.setInt("shadowMap", 0);
        //// set IBL Maps 1-->3
        //pbrShader.setInt("irradianceMap", 1);
        //pbrShader.setInt("prefilterMap", 2);
        //pbrShader.setInt("brdfLUT", 3);
        //// set Texture2D Maps for the models in order 4-->8 (albedo, metal, normal, roughness, ~alpha)
        //pbrShader.setInt("albedoMap", 4);
        //pbrShader.setInt("metallicMap", 5);
        //pbrShader.setInt("normalMap", 6);
        //pbrShader.setInt("roughnessMap", 7);
        //pbrShader.setInt("alphaMap", 8);

        //pbrShader.setFloat("depth_near", imgui_helper.depth_near);
        //pbrShader.setFloat("depth_far", imgui_helper.depth_far);

        //pbrShader.setFloat("lightWidth", imgui_helper.lightWidth);


        
        if(!start)
        {
            projection = camera.GetProjection();
            view = camera.GetViewMatrix();
            camPosition = camera.Position;
        }
        if (imgui_helper.droneCamera)
        {
            cameraLocalOffset = imgui_helper.cameraLocation;
            lookAtDrone = imgui_helper.lookingCamera;

            glm::vec3 dronePosition = drone.position;
            glm::quat droneQuaternion = drone.quaternion;

            glm::vec3 rotateOffset = droneQuaternion * cameraLocalOffset;
            glm::vec3 cameraWorldPosition = dronePosition + rotateOffset;
            glm::vec3 cameraUp = droneQuaternion * glm::vec3(0.0f, 1.0f, 0.0f);
            glm::mat4 viewMatrix = glm::lookAt(cameraWorldPosition, dronePosition + (droneQuaternion * lookAtDrone), cameraUp);
            glm::mat4 projDrone = droneAttachedCamera.GetProjection();
            projection = projDrone;
            view = viewMatrix;
            camPosition = cameraWorldPosition;
        }

        glm::mat4 inverseProj = glm::inverse(projection);
        
        geometryShaderBuffer.use();
        geometryShaderBuffer.setMat4("view", view);
        geometryShaderBuffer.setMat4("projection", projection);
        geometryShaderBuffer.setInt("albedoMap", 4);
        geometryShaderBuffer.setInt("metallicMap", 5);
        geometryShaderBuffer.setInt("normalMap", 6);
        geometryShaderBuffer.setInt("roughnessMap", 7);

            glm::vec3 newPos = lightPositions[0];
            shadow.lightPos = lightPositions[0];
            //pbrShader.setVec3("lightPositions[" + std::to_string(0) + "]", newPos);
            //pbrShader.setVec3("lightColors[" + std::to_string(0) + "]", lightColors[0]);
            lightColors[0] = glm::vec3(imgui_helper.lightMultiplayer);

            if (imgui_helper.secvential)
            {
                for (int i = 0; i < models.size(); i++)
                    models[i]->drawDebug(geometryShaderBuffer);
            }
            else
            {
                for (int i = 0; i < models.size(); i++)
                    models[i]->draw(geometryShaderBuffer);
            }

        droneSim.gamepadControl(gamepad);
        if (imgui_helper.startMotors) {
            for (int i = 0; i < 8; i++)
            {
                droneSim.flyDrone(geometryShaderBuffer);
            }
        }

        //-------------------
        // SCREEN SPACE PASS
        // Ambient Occlussion
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);

        ssaoShader.use();
        ssaoShader.setMat4("projection", projection);
        ssaoShader.setMat4("view", view);

        ssaoShader.setInt("gPosition", 0);
        ssaoShader.setInt("gNormal", 1);
        ssaoShader.setInt("texNoise", 2);
        ssaoShader.setFloat("aoIntensity", imgui_helper.aoMultiplayer);
        ssaoShader.setFloat("radius", imgui_helper.aoRadius);
        ssaoShader.setFloat("bias", imgui_helper.aoBias);
        glm::vec2 screenWH = glm::vec2(WIDTH / 4.0f, HEIGHT / 4.0f);
        ssaoShader.setVec2("noiseScale", screenWH);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPositionDepth);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormalRoughness);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        renderQuad();

        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        ssaoBlurShader.use();
        ssaoBlurShader.setInt("ssaoInput", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoTexture);
        renderQuad();
        

        // PBR SHADING 
        glBindFramebuffer(GL_FRAMEBUFFER, pbrFBO);
        glClear(GL_COLOR_BUFFER_BIT);

        pbrShader.use();
        pbrShader.setVec3("camPos", camera.Position);
        pbrShader.setMat4("view", view);

        pbrShader.setInt("shadowMap", 0);
        //// set IBL Maps 1-->3
        pbrShader.setInt("irradianceMap", 1);
        pbrShader.setInt("prefilterMap", 2);
        pbrShader.setInt("brdfLUT", 3);

        pbrShader.setInt("gPosition", 4);
        pbrShader.setInt("gNormal", 5);
        pbrShader.setInt("gAlbedo", 6);
        pbrShader.setInt("gARM", 7);
        pbrShader.setInt("ssaoBlured", 8);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadow.depthMap);
        ibl.bind(1);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, gPositionDepth);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, gNormalRoughness);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, gARM);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, ssaoBlured);

        
        pbrShader.setVec3("camPos", camera.Position);
        pbrShader.setVec3("lightPositions[" + std::to_string(0) + "]", newPos);
        lightColors[0] = glm::vec3(imgui_helper.lightMultiplayer);
        pbrShader.setVec3("lightColors[" + std::to_string(0) + "]", lightColors[0]);
        pbrShader.setMat4("lightSpaceMatrix", shadow.lightSpaceMatrix);

        pbrShader.setFloat("depth_near", imgui_helper.depth_near);
        pbrShader.setFloat("depth_far", imgui_helper.depth_far);
        pbrShader.setFloat("lightWidth", imgui_helper.lightWidth);
        renderQuad();

        //ibl.drawBackground(view, projection);
       

        // Reflection
        glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);
        glClear(GL_COLOR_BUFFER_BIT);

        ssrShader.use();
        ssrShader.setInt("gPosition", 0);
        ssrShader.setInt("gNormal", 1);
        ssrShader.setInt("gAlbedo", 2);
        ssrShader.setInt("gARM", 3);


        ssrShader.setMat4("uProjection", projection);
        ssrShader.setMat4("uView", view);

        ssrShader.setInt("uMaxSteps", imgui_helper.uMaxSteps);
        ssrShader.setInt("uBinarySearchSteps", imgui_helper.uBinarySearchSteps);
        ssrShader.setFloat("uStepSize", imgui_helper.uStepSize);
        ssrShader.setFloat("uThickness", imgui_helper.uThickness);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPositionDepth);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormalRoughness);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, pbrTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gARM);
        renderQuad();


        //glBindFramebuffer(GL_FRAMEBUFFER, pbrFBO);
        //capturer.CaptureAndProcess(droneSim, streamer);

        renderColorAndDepth(myWindow, quadShader);

        glLineWidth(2.0f);
        if (imgui_helper.physicsDebugRender)
            PhysicsEngine::getInstance().drawDebug(view, projection);

        if (glfwGetKey(myWindow.window, GLFW_KEY_1) == GLFW_PRESS) 
        {
            shadow.shadowDebug();
        }

        if(!start)
            Imgui_layer::getInstance().Update();


        glfwSwapBuffers(myWindow.window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}