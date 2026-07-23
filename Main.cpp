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
#include "Drone.h"
#include "NetworkStreamer.h"


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


GLuint framebuffer_colordepth, colorTexture_drone, depthTexture_drone, rbo_depth;
GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
void createColorAndDepth()
{
    glGenFramebuffers(1, &framebuffer_colordepth);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_colordepth);

    glGenTextures(1, &colorTexture_drone);
    glBindTexture(GL_TEXTURE_2D, colorTexture_drone);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_drone, 0);

    glGenTextures(1, &depthTexture_drone);
    glBindTexture(GL_TEXTURE_2D, depthTexture_drone);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, depthTexture_drone, 0);

    // 3. Renderbuffer-ul de adâncime (Hardware Depth Test - necesar ca obiectele 3D să nu se deseneze unele peste altele)
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, WIDTH, HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

    glDrawBuffers(2, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "EROARE: Framebuffer-ul nu este complet/valid!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Extrem de important! Revenim la ecranul principal.
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

    // 2. Activăm shaderul pentru quad-ul pe tot ecranul
    quadShader.use();
    quadShader.setInt("screenTexture", 0);
    quadShader.setInt("depthTexture", 1);
    if (glfwGetKey(myWindow.window, GLFW_KEY_M) == GLFW_PRESS)
        quadShader.setInt("u_mode", 1);
    else
        quadShader.setInt("u_mode", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture_drone);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture_drone);

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
    
    pbrShader.use();

    IBL ibl = IBL();
    ibl.initCubeFromHDR("HDRI/mappo.hdr");


    Model floor = Model("Models/Env/floor.fbx");
    floor.buildTexture("Models/Env", "Models/Env/textures_floor.txt");
    floor.rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));

    Model depthScene = Model("Models/Env/depth_test.fbx");
    depthScene.buildTexture("Models/Env", "Models/Env/textures_floor.txt");
    depthScene.rotate_Q(glm::vec3(-90.0f, 0.0f, 0.0f));
    depthScene.move(0.0f, 15.0f, 0.0f);

    Model newModel = Model("Models/model.fbx", true);
    //Model newModel = Model("Models/Porsche/porsche.fbx");
    //newModel.buildTexture("Models/Porsche", "Models/Porsche/textures.txt");
    //newModel.textureAlpha = textures.texture2Dfile("Models/Porsche/BODY_alpha.png");
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
    drone.move(-2.0f, 5.0f, 1.0f);
    drone.scale(0.5f);
    drone.applyPhysicsConvexHull();
    drone.applyPhysicsMatrix(true);

    Model propeler = Model("Models/Drone/propeler.fbx");
    propeler.scale(0.5f);

    drone.texture = newModel.texture;
    propeler.texture = newModel.texture;

    std::vector<Model*> models = { 
        &newModel, 
        //&proxy, 
        &floor,
        &depthScene,
        &metal, 
        &plane,
        &drone,
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

    createColorAndDepth();
    FFmpegStreamer streamer = FFmpegStreamer("100.70.184.28", 5555, 1280, 720);
    std::vector<unsigned char> colorData(1280 * 720 * 4);
    std::vector<unsigned char> depthData(1280 * 720 * 4);

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
    glm::mat4 projDrone = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
    droneAttachedCamera.setPerspective(projDrone);

    Drone droneSim = Drone(&myWindow, &drone, &propeler, &primObj, &imgui_helper);

    PhysicsEngine::getInstance().showBodyInfo(drone.physics_id);

    while (!glfwWindowShouldClose(myWindow.window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        fps = 1.0f / deltaTime;

        gamepad.update();

        // input
        // -----
        camera.processInput(deltaTime);

        // render
        // ------
        shadow.bindShadowMap(models);

        glViewport(0, 0, WIDTH, HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_colordepth); // check FBO
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // render scene, supplying the convoluted irradiance map to the final shader.
        // ------------------------------------------------------------------------------------------
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjection();

        // activate PBR Shader
        // ------------------------------------------------------------------------------------------
        pbrShader.use();
        pbrShader.setVec3("u_albedo", glm::vec3(0.7f, 0.7f, 0.7f));
        // set SHADOW Map 0
        pbrShader.setInt("shadowMap", 0);
        // set IBL Maps 1-->3
        pbrShader.setInt("irradianceMap", 1);
        pbrShader.setInt("prefilterMap", 2);
        pbrShader.setInt("brdfLUT", 3);
        // set Texture2D Maps for the models in order 4-->8 (albedo, metal, normal, roughness, ~alpha)
        pbrShader.setInt("albedoMap", 4);
        pbrShader.setInt("metallicMap", 5);
        pbrShader.setInt("normalMap", 6);
        pbrShader.setInt("roughnessMap", 7);
        pbrShader.setInt("alphaMap", 8);
        
        if(!start)
        {
            pbrShader.setMat4("projection", projection);
            pbrShader.setMat4("view", view);
            pbrShader.setVec3("camPos", camera.Position);
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
            pbrShader.setMat4("projection", projDrone);
            pbrShader.setMat4("view", viewMatrix);
            pbrShader.setVec3("camPos", cameraWorldPosition);
        }
        
        pbrShader.setMat4("lightSpaceMatrix", shadow.lightSpaceMatrix);
        
        imgui_helper.setShader(pbrShader);

        // bind pre-computed IBL data
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadow.depthMap);
        ibl.bind(1);

        for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
        {
            glm::vec3 newPos = lightPositions[i];
            pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
            pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);
            lightColors[i] = glm::vec3(imgui_helper.lightMultiplayer);

            if (imgui_helper.secvential)
            {
                for (int i = 0; i < models.size(); i++)
                    models[i]->drawDebug(pbrShader);
            }
            else
            {
                for (int i = 0; i < models.size(); i++)
                    models[i]->draw(pbrShader);
            }
        }

        droneSim.gamepadControl(gamepad);
        if (imgui_helper.startMotors) {
            for (int i = 0; i < 8; i++)
            {
                droneSim.flyDrone(pbrShader);
            }
        }

        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, 1280, 720, GL_RGBA, GL_UNSIGNED_BYTE, colorData.data());
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glReadPixels(0, 0, 1280, 720, GL_RGBA, GL_UNSIGNED_BYTE, depthData.data());
        droneSim.depthProc(depthData);
        streamer.PushFrame(colorData, depthData);


        //ibl.drawBackground(view, projection);



        glLineWidth(2.0f);
        if (!imgui_helper.droneCamera)
            PhysicsEngine::getInstance().drawDebug(view, projection);
        else
        {

        }
        
        renderColorAndDepth(myWindow, quadShader);

        if (glfwGetKey(myWindow.window, GLFW_KEY_1) == GLFW_PRESS) {
            shadow.lightPos = lightPositions[0];
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