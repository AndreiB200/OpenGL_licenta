#define _DRONE_
#ifdef _DRONE_

#include <vector>
#include "VectorFieldHistogram.h"
#include "VoxelGrid.h"

struct droneCamera
{
    glm::vec3 position;
    glm::vec3 worldPos;
    glm::vec3 viewLook;
    glm::mat4 projection;
    glm::mat4 view;
};

class Drone
{
public:
    struct propellerData {
        float MAX_FORCE, fortaM0, fortaM1, fortaM2, fortaM3;
        glm::vec3 coltLocal0, coltLocal1, coltLocal2, coltLocal3; 
        glm::quat quaternionDum;
    };
    
    struct SampledDepth {
        float ndcX;
        float ndcY;
        float value;
    };

    std::vector<glm::vec3> LiDARpoints;
    LidarVoxelGrid lidarVoxelGrid;

    Window* myWindow;
    Model* drone;
    Model* propeler;
    PrimitiveObj* primObj;

    VFHPlus3D vfhPlanner = VFHPlus3D();

    std::vector<droneCamera> cameras;
    GLuint depthFBO;
    GLuint depthTextures[3];
    Shader shader = Shader("shadow.vert", "shadow.frag");
    Shader debugging = Shader("shadow_debug.vert", "shadow_debug.frag");

    DebuggerClass* imgui_helper;

    std::vector<SampledDepth> points, pointsPrevious;
    float middlePoint = 0.0f, previous_middlePoint = 0.0f;

    DronePose dronePose{ 0.0f, 0.0f ,0.0f ,0.0f, 0.0f, 0.0f ,0.0f };
    PythonCommand received_cmd{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f ,0.0f, 0 };

    ZmqNode publisher;

	Drone(Window* _myWindow, Model* droneModel, Model* _propeler, PrimitiveObj* _primObj, DebuggerClass* _imgui_helper)
    {
        myWindow = _myWindow;
        drone = droneModel;
        propeler = _propeler;
        primObj = _primObj;
        imgui_helper = _imgui_helper;
        points.reserve(64);
        LiDARpoints.reserve(64);
        pointsPrevious.reserve(64);
        publisher.init();
        publisher.start(dronePose, received_cmd, 100);
        vfhPlanner.bindPub(publisher);
    }

    bool resetPositionAndPID = false, control_position = false;
    float currentYaw = 0.0f;
    float pitchInput_X;
    float rollInput_Z;
    float yawInput_Y;
    float positioning_X = 0.0f;
    float positioning_Z = 0.0f;

    bool increaseRadians = false;
    void gamepadControl(Gamepad &gamepad)
    {
        pitchInput_X = gamepad.getRightStickY();
        rollInput_Z = gamepad.getRightStickX();
        yawInput_Y = gamepad.getLeftStickX();

        positioning_X = gamepad.getRightStickY();
        positioning_Z = gamepad.getRightStickX();

        resetPositionAndPID = gamepad.isButtonPressed(GLFW_GAMEPAD_BUTTON_A);

        increaseRadians = false;
        if (gamepad.isButtonPressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER))
            increaseRadians = true;

        static bool gamepad_B_PressedLastFrame = false;
        bool gamepad_B_CurrentlyPressed = gamepad.isButtonPressed(GLFW_GAMEPAD_BUTTON_B);

        control_position = false;
        if (gamepad_B_CurrentlyPressed && !gamepad_B_PressedLastFrame)
        {
            control_position = true;
        }
        gamepad_B_PressedLastFrame = gamepad_B_CurrentlyPressed;

        static bool gamepad_Y_PressedLastFrame = false;
        bool gamepad_Y_CurrentlyPressed = gamepad.isButtonPressed(GLFW_GAMEPAD_BUTTON_Y);

        bool droneCam = false;
        if (gamepad_Y_CurrentlyPressed && !gamepad_Y_PressedLastFrame)
        {
            imgui_helper->droneCamera = !imgui_helper->droneCamera;
        }
        gamepad_Y_PressedLastFrame = gamepad_Y_CurrentlyPressed;

        imgui_helper->targetPosition.y += 0.1f * (gamepad.getRightTrigger());
        imgui_helper->targetPosition.y -= 0.1f * (gamepad.getLeftTrigger());
        if (imgui_helper->targetPosition.y > 30.0f) imgui_helper->targetPosition.y = 30.0f;

        convertGamepad();
        dronePose.x = drone->position.x;
        dronePose.y = drone->position.y;
        dronePose.z = drone->position.z;
        dronePose.qw = drone->quaternion.w;
        dronePose.qx = drone->quaternion.x;
        dronePose.qy = drone->quaternion.y;
        dronePose.qz = drone->quaternion.z;
        /*std::cout << received_cmd.x << " " << received_cmd.y << " " << received_cmd.z << " "
            << received_cmd.qw << " " << received_cmd.qx << " " << received_cmd.qy << " " << received_cmd.qz << " " << received_cmd.flag << "\n";*/
    }

    float goTo_X = 0.0f;
    float goTo_Z = 0.0f;
    glm::quat targetQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    void convertGamepad()
    {
        float maxAngleRadian = increaseRadians ? glm::radians(60.0f) : glm::radians(30.0f);

        float yawSpeed = glm::radians(2.0f); // 2 degrees/frame
        currentYaw -= yawInput_Y * yawSpeed;

        glm::quat yawDeltaQuat = glm::angleAxis(currentYaw, glm::vec3(0.0f, 1.0f, 0.0f));

        float targetPitch = pitchInput_X * maxAngleRadian;
        float targetRoll = rollInput_Z * maxAngleRadian;

        glm::quat localTilt = glm::angleAxis(targetPitch, glm::vec3(1.0f, 0.0f, 0.0f)) *
                              glm::angleAxis(targetRoll, glm::vec3(0.0f, 0.0f, 1.0f));

        glm::quat baseYawQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        baseYawQuat = glm::normalize(yawDeltaQuat * baseYawQuat);

        targetQuat = glm::normalize(baseYawQuat * localTilt);

        imgui_helper->quatDummyTest.x = glm::degrees(targetPitch);
        imgui_helper->quatDummyTest.y = glm::degrees(currentYaw);
        imgui_helper->quatDummyTest.z = glm::degrees(targetRoll);
        imgui_helper->droneTargetHeight = imgui_helper->targetPosition.y;
    }

    void createCamera(int WIDTH, int HEIGHT)
    {
        // Main front camera index [0]
        droneCamera droneFrontCamera;
        droneFrontCamera.position = imgui_helper->cameraLocation;
        droneFrontCamera.viewLook = imgui_helper->lookingCamera;

        glm::vec3 dronePosition = drone->position;
        glm::quat droneQuaternion = drone->quaternion;

        glm::vec3 rotateOffset = droneQuaternion * droneFrontCamera.position;
        glm::vec3 cameraWorldPosition = dronePosition + rotateOffset;
        glm::vec3 cameraUp = droneQuaternion * glm::vec3(0.0f, 1.0f, 0.0f);

        droneFrontCamera.view = glm::lookAt(cameraWorldPosition, dronePosition + (droneQuaternion * droneFrontCamera.viewLook), cameraUp);
        droneFrontCamera.projection = glm::perspective(glm::radians(90.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

        cameras.push_back(droneFrontCamera);
    }

    void sensorsAttach(int WIDTH, int HEIGHT)
    {
        // Front Back Left Right order (in vector)
        glm::vec3 sensorLocalOffset = glm::vec3(0.0f, 0.7f, 1.0f);
        glm::mat4 projDrone = glm::perspective(glm::radians(90.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
        
        glm::vec3 sensorLookBACK = glm::vec3(0.0f, 0.7f, -2.0f);
        glm::vec3 sensorLookLEFT = glm::vec3(2.0f, 0.7f, 0.0f);
        glm::vec3 sensorLookRIGHT = glm::vec3(-2.0f, 0.7f, 0.0f);

        glm::vec3 dronePosition = drone->position;
        glm::quat droneQuaternion = drone->quaternion;
        glm::vec3 cameraUp = droneQuaternion * glm::vec3(0.0f, 1.0f, 0.0f);
        
        droneCamera droneBackCamera, droneLeftCamera, droneRightCamera;
        droneBackCamera.position = droneLeftCamera.position = droneRightCamera.position = imgui_helper->cameraLocation;
        droneBackCamera.viewLook = sensorLookBACK; droneLeftCamera.viewLook = sensorLookLEFT; droneRightCamera.viewLook = sensorLookRIGHT;

        glm::vec3 rotateOffset = droneQuaternion * droneBackCamera.position;
        glm::vec3 cameraWorldPosition = dronePosition + rotateOffset;

        droneBackCamera.view = glm::lookAt(cameraWorldPosition, dronePosition + (droneQuaternion * droneBackCamera.viewLook), cameraUp);
        droneLeftCamera.view = glm::lookAt(cameraWorldPosition, dronePosition + (droneQuaternion * droneLeftCamera.viewLook), cameraUp);
        droneRightCamera.view = glm::lookAt(cameraWorldPosition, dronePosition + (droneQuaternion * droneRightCamera.viewLook), cameraUp);

        droneBackCamera.projection = droneLeftCamera.projection = droneRightCamera.projection = glm::perspective(glm::radians(90.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
        cameras.push_back(droneBackCamera); cameras.push_back(droneLeftCamera); cameras.push_back(droneRightCamera);

        GLuint depthRBO;
        glGenRenderbuffers(1, &depthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);

        glGenFramebuffers(1, &depthFBO);
        glGenTextures(3, depthTextures);

        for (int i = 0; i < 3; i++) {
            glBindTexture(GL_TEXTURE_2D, depthTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, WIDTH, HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextures[0], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void saveSceneSensorData(std::vector<Model*> models, glm::mat4 cameraSpaceMatrix, int map_index)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextures[map_index], 0);

        glViewport(0, 0, 1280, 720);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glDisable(GL_CULL_FACE);

        shader.use();
        shader.setMat4("lightSpaceMatrix", cameraSpaceMatrix);

        for (int i = 0; i < models.size(); i++) {
            models[i]->drawShadow(shader);
        }

        // Unbind FBO
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void drawSensorData()
    {
        glDisable(GL_DEPTH_TEST);
        debugging.use();
        debugging.setFloat("near_plane", 0.1);
        debugging.setFloat("far_plane", 100.0);

        int WIDTH = 1280, HEIGHT = 720;

        debugging.setInt("depthMap", 0);
        glActiveTexture(GL_TEXTURE0);
        
        glViewport(WIDTH / 2, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);
        glBindTexture(GL_TEXTURE_2D, depthTextures[0]);
        renderQuad();

        glViewport(0, 0, WIDTH / 2, HEIGHT / 2);
        glBindTexture(GL_TEXTURE_2D, depthTextures[1]);
        renderQuad();

        glViewport(WIDTH / 2, 0, WIDTH / 2, HEIGHT / 2);
        glBindTexture(GL_TEXTURE_2D, depthTextures[2]);
        renderQuad();
    }

    void cameraAnimation(std::vector<Model*> models, glm::mat4 &proj, glm::mat4 &view, glm::vec3 &camPos)
    {
        for (int i = 0; i < cameras.size(); i++)
        {
            glm::vec3 cameraLocalOffset = imgui_helper->camPosition[i];
            glm::vec3 lookAtDrone = imgui_helper->lookingPosition[i];

            glm::vec3 dronePosition = drone->position;
            glm::quat droneQuaternion = drone->quaternion;

            glm::vec3 rotateOffset = droneQuaternion * cameraLocalOffset;
            glm::vec3 cameraWorldPosition = dronePosition + rotateOffset;
            glm::vec3 cameraUp = droneQuaternion * glm::vec3(0.0f, 1.0f, 0.0f);
            glm::mat4 viewMatrix = glm::lookAt(cameraWorldPosition, dronePosition + (droneQuaternion * lookAtDrone), cameraUp);
            glm::mat4 projDrone = cameras[i].projection;
            cameras[i].view = viewMatrix;
            cameras[i].worldPos = cameraWorldPosition;
            glm::mat4 spaceMatrix = projDrone * viewMatrix;
            if(i > 0)
                saveSceneSensorData(models, spaceMatrix, i-1);
        }

        camPos = cameras[0].worldPos;
        proj = cameras[0].projection;
        view = cameras[0].view;
    }



    void drawCubesFromPoints(Shader &localShader)
    {
        glm::mat4 invProj = glm::inverse(cameras[0].projection);
        glm::mat4 invView = glm::inverse(cameras[0].view);
        primObj->scale(imgui_helper->cubeSizes);
        localShader.setVec3("debugColor", glm::vec3(0.0f, 1.0f, 0.0f));

        for (size_t i = 0; i < points.size(); ++i) {
            float zView = points[i].value * imgui_helper->depth_far;
            glm::vec4 clipPos = glm::vec4(points[i].ndcX, points[i].ndcY, 1.0f, 1.0f);
            glm::vec4 viewTarget = invProj * clipPos;
            viewTarget /= viewTarget.w;

            glm::vec3 rayDir = glm::normalize(glm::vec3(viewTarget));

            glm::vec3 viewPos = rayDir * (zView / -rayDir.z);

            glm::vec3 worldPos = glm::vec3(invView * glm::vec4(viewPos, 1.0f));
            LiDARpoints[i] = worldPos;
            if (imgui_helper->drawPoints)
            {
                primObj->move(worldPos);
                primObj->renderCube_shader(localShader);
            }
        }
        
        if (imgui_helper->drawVoxelGrid)
        {
            lidarVoxelGrid.addPoints(LiDARpoints, drone->position);
            std::vector<glm::vec3> getVoxels = lidarVoxelGrid.getUniqueCenters();
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, getVoxels.size() - 1);

            for (int index = 0; index < 300; index++)
            {
                primObj->move(getVoxels[dist(gen)]);
                primObj->renderCube_shader(localShader);
            }

            if (imgui_helper->sendVoxelsNetwork_button)
            {
                publisher.sendVoxelData(getVoxels);
                imgui_helper->sendVoxelsNetwork_button = false;
            }
        }
    }

    void depthProc(std::vector<float> &depthProc)
    {
        getUniformNDCPoints(depthProc);
    }
    void getUniformNDCPoints(std::vector<float>& depthData) 
    {
        const int width = 1280;
        const int height = 720;
        const int count = 16; 

        if (points.size() != count * count) { points.resize(count * count); LiDARpoints.resize(count * count); }
        if (pointsPrevious.size() != count * count) pointsPrevious.resize(count * count);

        if (depthData.size() < static_cast<size_t>(width * height)) return;

        middlePoint = depthData[360 * width + 640];

        static bool isInitialized = false;
        bool firstFrame = !isInitialized;

        pointsPrevious = points;

        const float stepX = static_cast<float>(width) / (count + 1);
        const float stepY = static_cast<float>(height) / (count + 1);

        int pointIndex = 0;

        for (int r = 1; r <= count; ++r) {
            int y = std::clamp(static_cast<int>(r * stepY), 0, height - 1);

            for (int c = 1; c <= count; ++c) {
                int x = std::clamp(static_cast<int>(c * stepX), 0, width - 1);

                size_t idx = static_cast<size_t>(y) * width + x;
                float medianDepthValue = depthData[idx];

                float ndcX = ((static_cast<float>(x) + 0.5f) / static_cast<float>(width)) * 2.0f - 1.0f;
                float ndcY = ((static_cast<float>(y) + 0.5f) / static_cast<float>(height)) * 2.0f - 1.0f;

                float finalDepth = medianDepthValue;

                points[pointIndex++] = { ndcX, ndcY, finalDepth };
            }
        }

        isInitialized = true;
    }
    
    glm::vec3 calculateAvoidanceVector(glm::vec3& actualPos, glm::vec3 &targetPos, glm::vec3 &forward) {
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 direction = vfhPlanner.computeSteeringDirection(actualPos, forward, up, targetPos, LiDARpoints);
        imgui_helper->targetCollision = direction;
        return direction;
    }
    
    void collisionDetection(glm::vec3 &currentPos, glm::quat &currentQuat, glm::vec3 angularVel, glm::vec3 &targetCollision, glm::vec3 &droneDirection, float &collisionYaw)
    {
        goTo_X = +positioning_X + received_cmd.x;
        goTo_Z = +positioning_Z + received_cmd.z;

        // Gamepad direction
        float forwardPos = positioning_X; 
        float rightPos = positioning_Z;
        
        // Speed | distance | collisions
        float MAX_SPEED = 5.0f;
        float distanceToObstacle = middlePoint * 100.0f;    imgui_helper->middlePointDebug = distanceToObstacle; // debug

        float preventiveSpeed = 5.0f;
        
        // Direction crusing
        glm::vec3 forwardVec = glm::rotate(glm::normalize(currentQuat), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 forwardDirection = glm::vec3(forwardVec.x, 0.0f, forwardVec.z);
        glm::vec3 left_rightDirection = glm::normalize(glm::vec3(glm::cross(forwardDirection, glm::vec3(0.0f, 1.0f, 0.0f))));
        
        // Remote connection
        static float receivedIncreasingYaw = 0.0f;
        if (imgui_helper->remoteControl && received_cmd.flag == 1)
        {
            receivedIncreasingYaw = receivedIncreasingYaw + received_cmd.qy;
            collisionYaw = collisionYaw + receivedIncreasingYaw;
        }

        glm::vec3 moveDirection;
        
        static float smoothSpeed = 0.0f;
        smoothSpeed = glm::mix(smoothSpeed, preventiveSpeed, 0.01f);
        moveDirection = (forwardVec * smoothSpeed * forwardPos) + (left_rightDirection * smoothSpeed * rightPos);
        
        float saveY = targetCollision.y;
        glm::vec3 vectorMovement = currentPos + moveDirection;
        glm::vec3 saveTargetBefore = targetCollision;

        if (std::abs(positioning_X) > 0.0f || std::abs(positioning_Z) > 0.0f)
            targetCollision = vectorMovement;
        else
            targetCollision = targetCollision;

        targetCollision.y = saveY;
        glm::vec3 direction = glm::normalize(targetCollision - currentPos);

        glm::vec3 vfhRes = vfhPlanner.computeAPFSteering(currentPos, targetPosition, LiDARpoints, 5.0f, 1.0f, 15.0f);
        droneDirection = currentPos + vfhRes;
        imgui_helper->targetCollision = vfhRes;
    }


    glm::vec3 targetPosition;
    float targetYawAngle;
    propellerData propellers;
    void flyDrone(Shader pbrShader)
	{
        float dt = PhysicsEngine::getInstance().getPhysicsStep();
        bool inteligenta_artificiala = imgui_helper->inteligenta_artificiala;
        targetYawAngle = currentYaw;

        if (imgui_helper->maxPower < 0.01f) imgui_helper->maxPower = 0.0f;
        if (imgui_helper->minPower >= imgui_helper->maxPower) imgui_helper->minPower = imgui_helper->maxPower;

        if (glfwGetKey(myWindow->window, GLFW_KEY_R) == GLFW_PRESS || (resetPositionAndPID == true))
        {
            pidPitch.Reset();
            pidRoll.Reset();
            pidYaw.Reset();
            pidHeight.Reset();
            pidX.Reset();
            pidZ.Reset();
            PhysicsEngine::getInstance().resetBody(drone->physics_id, JPH::Vec3(-1.0f, 2.0f, 2.0f), JPH::Quat::sIdentity());
            imgui_helper->targetPosition = glm::vec3(-1.0f, 2.0f, 2.0f);
            imgui_helper->quatDummyTest.x = imgui_helper->quatDummyTest.y = imgui_helper->quatDummyTest.z = 0;
        }

        glm::vec3 directieRacheta(0.0f, 1.0f, 0.0f);

        glm::vec3 posPropeller_FRONTLEFT = imgui_helper->posPropeller_FRONTLEFT;
        glm::vec3 posPropeller_BACKLEFT = imgui_helper->posPropeller_BACKLEFT;
        glm::vec3 posPropeller_FRONTRIGHT = imgui_helper->posPropeller_FRONTRIGHT;
        glm::vec3 posPropeller_BACKRIGHT = imgui_helper->posPropeller_BACKRIGHT;
        glm::vec3 coltLocal0(posPropeller_FRONTLEFT);  // FRONT LEFT
        glm::vec3 coltLocal1(posPropeller_BACKLEFT); // BACK LEFT
        glm::vec3 coltLocal2(posPropeller_FRONTRIGHT); // FRONT RIGHT
        glm::vec3 coltLocal3(posPropeller_BACKRIGHT);// BACK RIGHT


        glm::vec3 currentPosition;
        glm::quat currentQuat;
        glm::vec3 currentAngVel, worldLinearVel;
        PhysicsEngine::getInstance().getModelMatrix(drone->physics_id, currentPosition, currentQuat);
        drone->position = currentPosition; drone->quaternion = currentQuat;

        PhysicsEngine::getInstance().GetAngularVelocity(drone->physics_id, currentAngVel);
        PhysicsEngine::getInstance().GetLinearVelocity(drone->physics_id, worldLinearVel);
        glm::vec3 localAngVel = glm::inverse(currentQuat) * currentAngVel;

        imgui_helper->currentPos = currentPosition;

        targetPosition = imgui_helper->targetPosition;
        glm::vec3 droneDirection;
        collisionDetection(currentPosition, currentQuat, currentAngVel, imgui_helper->targetPosition, droneDirection, targetYawAngle);

        //processAiNetwork(targetPosition, targetYawAngle);

        float heightError;
        heightError = droneDirection.y - currentPosition.y;

        float heightCorrection = pidHeight.Update(heightError, worldLinearVel.y, dt, 10.0f);

        float masaDronei = 1.0f;
        float hoverThrust = masaDronei * 9.81f;
        float fortaRacheta = hoverThrust + heightCorrection;

        glm::vec3 droneUpWorld = currentQuat * glm::vec3(0.0f, 1.0f, 0.0f);
        float cosinusInclinare = glm::dot(droneUpWorld, glm::vec3(0.0f, 1.0f, 0.0f));
        cosinusInclinare = std::clamp(cosinusInclinare, 0.3f, 1.0f);
        fortaRacheta = fortaRacheta / cosinusInclinare;

        fortaRacheta = std::clamp(fortaRacheta, imgui_helper->minPower, imgui_helper->getMaxPower());

        if (inteligenta_artificiala) 
        {
            layerPositionPID(droneDirection, targetQuat, currentQuat, currentPosition, worldLinearVel);
        }

        float fortaM0 = 0.0f, fortaM1 = 0.0f, fortaM2 = 0.0f, fortaM3 = 0.0f;
        layerAttitudePID(currentQuat, localAngVel, dt, fortaRacheta, fortaM0, fortaM1, fortaM2, fortaM3);
        

        PhysicsEngine::getInstance().ApplyRocketForce(drone->physics_id, coltLocal0, directieRacheta, fortaM0, -1.0f); // Front Left
        PhysicsEngine::getInstance().ApplyRocketForce(drone->physics_id, coltLocal2, directieRacheta, fortaM2, 1.0f);  // Front Right
        PhysicsEngine::getInstance().ApplyRocketForce(drone->physics_id, coltLocal1, directieRacheta, fortaM3, 1.0f);  // Back Left
        PhysicsEngine::getInstance().ApplyRocketForce(drone->physics_id, coltLocal3, directieRacheta, fortaM1, -1.0f); // Back Right

        savePropellers(imgui_helper->getMaxPower(), fortaM0, fortaM1, fortaM2, fortaM3, coltLocal0, coltLocal1, coltLocal2, coltLocal3, targetQuat);

        PhysicsEngine::getInstance().run();
	}

    void layerPositionPID(glm::vec3 targetPosition, glm::quat &targetQuat, glm::quat currentQuat, glm::vec3 &currentPosition, glm::vec3 worldLinearVel)
    {
        float dt = PhysicsEngine::getInstance().getPhysicsStep();
        float errorX = targetPosition.x - currentPosition.x;
        float errorZ = targetPosition.z - currentPosition.z;

        float worldAccelX = pidX.Update(errorX, worldLinearVel.x, dt, 15.0f);
        float worldAccelZ = pidZ.Update(errorZ, worldLinearVel.z, dt, 15.0f);

        glm::vec3 forwardDir = currentQuat * glm::vec3(0.0f, 0.0f, -1.0f);
        float currentYaw = atan2f(-forwardDir.x, -forwardDir.z);
        float cosYaw = cosf(currentYaw);
        float sinYaw = sinf(currentYaw);

        float localAccelRight = worldAccelX * cosYaw - worldAccelZ * sinYaw; // Pe axa X locală
        float localAccelForward = -worldAccelX * sinYaw - worldAccelZ * cosYaw; // Pe axa Z locală

        float MAX_TILT = glm::radians(20.0f);
        float targetPitch = std::clamp(-localAccelForward / 9.81f, -MAX_TILT, MAX_TILT);
        float targetRoll = std::clamp(-localAccelRight / 9.81f, -MAX_TILT, MAX_TILT);

        glm::quat qPitch = glm::angleAxis(targetPitch, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat qYaw = glm::angleAxis(targetYawAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat qRoll = glm::angleAxis(targetRoll, glm::vec3(0.0f, 0.0f, 1.0f));
        targetQuat = qYaw * qPitch * qRoll;
    }

    void layerAttitudePID(glm::quat &currentQuat, glm::vec3 &localAngVel, float &dt, float &fortaRacheta, float &fortaM0, float &fortaM1, float &fortaM2, float &fortaM3)
    {

        glm::quat targetRot = targetQuat;
        glm::quat errorQuat = glm::inverse(currentQuat) * targetRot;
        if (errorQuat.w < 0.0f) errorQuat = -errorQuat;

        float angle = 2.0f * acosf(std::clamp(errorQuat.w, -1.0f, 1.0f));
        glm::vec3 rotationError(0.0f);
        if (angle > 0.0001f) {
            float sinHalfAngle = sqrtf(1.0f - errorQuat.w * errorQuat.w);
            if (sinHalfAngle > 0.0001f) {
                rotationError = glm::vec3(errorQuat.x, errorQuat.y, errorQuat.z) / sinHalfAngle * angle;
            }
        }

        float pitchCorectie = pidPitch.Update(rotationError.x, localAngVel.x, dt, fortaRacheta);
        float yawCorectie = pidYaw.Update(rotationError.y, localAngVel.y, dt, fortaRacheta);
        float rollCorectie = pidRoll.Update(rotationError.z, localAngVel.z, dt, fortaRacheta);

        float throttle = fortaRacheta;

        fortaM0 = throttle - pitchCorectie + rollCorectie - yawCorectie;
        fortaM1 = throttle + pitchCorectie - rollCorectie - yawCorectie;
        fortaM2 = throttle - pitchCorectie - rollCorectie + yawCorectie;
        fortaM3 = throttle + pitchCorectie + rollCorectie + yawCorectie;

        float MAX_FORCE = imgui_helper->getMaxPower();
        fortaM0 = std::clamp(fortaM0, 0.0f, MAX_FORCE);
        fortaM1 = std::clamp(fortaM1, 0.0f, MAX_FORCE);
        fortaM2 = std::clamp(fortaM2, 0.0f, MAX_FORCE);
        fortaM3 = std::clamp(fortaM3, 0.0f, MAX_FORCE);
    }

    float propRot = 0.0f;
    void renderPropellers(Shader& shader)
    {
        propRot = propRot + 30.0f;
        if (propRot > 3600.0f)
            propRot = 0.0f;
        glm::quat propeller_rotation = glm::angleAxis(glm::radians(propRot), glm::vec3(0.0f, 1.0f, 0.0f));
        propeler->move(propellers.coltLocal0); // M0
        propeler->rotate_Q(drone->quaternion * propeller_rotation);
        propeler->scale(0.5);
        shader.setVec3("debugColor", glm::vec3(propellers.fortaM0 / propellers.MAX_FORCE, 0.0f, 0.0f));
        propeler->draw(shader);

        propeler->move(propellers.coltLocal1); // M3
        propeler->rotate_Q(drone->quaternion * propeller_rotation);
        propeler->scale(0.5f);
        shader.setVec3("debugColor", glm::vec3(0.0f, propellers.fortaM3 / propellers.MAX_FORCE, 0.0f));
        propeler->draw(shader);

        propeler->move(propellers.coltLocal2); // M2 
        propeler->rotate_Q(drone->quaternion * propeller_rotation);
        propeler->scale(0.5f);
        shader.setVec3("debugColor", glm::vec3(0.0f, 0.0f, propellers.fortaM2 / propellers.MAX_FORCE));
        propeler->draw(shader);

        propeler->move(propellers.coltLocal3); // M1 
        propeler->rotate_Q(drone->quaternion * propeller_rotation);
        propeler->scale(0.5f);
        shader.setVec3("debugColor", glm::vec3(propellers.fortaM1 / propellers.MAX_FORCE, propellers.fortaM1 / propellers.MAX_FORCE, 0.0f));
        propeler->draw(shader);

        /*primObj->move(glm::vec3(2.0f, 2.0f, -1.0f));
        primObj->rotate_Q(drone->quaternion);
        propeler->scale(1.0f);
        primObj->renderCube_shader(shader);*/

        imgui_helper->quatDebug = drone->quaternion;
    }

    void processAiNetwork(glm::vec3 &targetPosition, float &targetYaw)
    {
        if (received_cmd.flag == 1 && imgui_helper->remoteControl)
        {
            glm::vec3 pos = glm::vec3(received_cmd.x, received_cmd.y, received_cmd.z);
            targetPosition = pos;
        }
        else
            return;
    }

private:
    float MIN_ALPHA = 0.05f;  
    float MAX_ALPHA = 1.00f;  
    float NOISE_THRESHOLD = 0.05f;  
    float JUMP_THRESHOLD = 0.15f; 
    float applyAdaptiveFilter(float currentRaw, float previousFiltered) {
        float delta = std::abs(currentRaw - previousFiltered);

        if (delta < NOISE_THRESHOLD) {
            return previousFiltered;
        }

        float factor = (delta - NOISE_THRESHOLD) / (JUMP_THRESHOLD - NOISE_THRESHOLD);
        factor = std::clamp(factor, 0.0f, 1.0f);

        float dynamicAlpha = MIN_ALPHA + factor * (MAX_ALPHA - MIN_ALPHA);

        return dynamicAlpha * currentRaw + (1.0f - dynamicAlpha) * previousFiltered;
    }

    float dampenVibration(float current, float target, float& velocity, float damping, float springK, float dt) {
        float force = (target - current) * springK;
        float dampingForce = velocity * damping;

        float acceleration = force - dampingForce;
        velocity += acceleration * dt;
        current += velocity * dt;

        return current;
    }

    float getCenterWeight(float ndcX, float ndcY) {
        float distFromCenter = std::sqrt(ndcX * ndcX + ndcY * ndcY);
        float t = 1.0f - distFromCenter;
        t = std::clamp(t, 0.0f, 1.0f);
        return getGaussianWeight(t);
    }
    float getGaussianWeight(float x, float a = 2.6f, float b = 6.0f) {
        return glm::exp(-a * glm::pow(glm::abs(x), b));
    }

    void savePropellers(float MAX_FORCE, float& fortaM0, float& fortaM1, float& fortaM2, float& fortaM3, glm::vec3 coltLocal0, glm::vec3 coltLocal1, glm::vec3 coltLocal2, glm::vec3 coltLocal3, glm::quat quaternionDum)
    {
        propellers.MAX_FORCE = MAX_FORCE;
        propellers.fortaM0 = fortaM0; propellers.fortaM1 = fortaM1; propellers.fortaM2 = fortaM2; propellers.fortaM3 = fortaM3;
        propellers.coltLocal0 = coltLocal0; propellers.coltLocal1 = coltLocal1; propellers.coltLocal2 = coltLocal2; propellers.coltLocal3 = coltLocal3;
        propellers.quaternionDum = quaternionDum;
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
};

#endif // !_DRONE_
