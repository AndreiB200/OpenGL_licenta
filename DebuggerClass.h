#define _DEBUGGERCLASS_
#ifdef _DEBUGGERCLASS_

#include "Shader.h"
#include "Imgui_layer.h"
#include "ShadowMap.h"

class DebuggerClass
{
public:
	DebuggerClass(){}

    Shader noColorDebugShader = Shader("debug_shader.vert", "debug_shader.frag");

	bool secvential = false;

	//textures edit
	float metal = 0.1f, roughness = 0.4f;
	float normal[3] = {0.0f,0.0f,0.0f};
	float color[3] = {0.7f, 0.7f, 0.7f};

	float shadowUp = 0.000f, shadowBias = 0.0001f, multipalyer = 0.1f, ambient_occlusion = 1.0f, lightMultiplayer = 1.0f;
	int pcfSize = 3;

	int index = 0;

	int textureSelect = 1; //0 constant color, 1 textured, 2 color, 3 metal, 4 roughness

    // Variables that need to be attached (prototype, needs improvment)
    float *deltaTime;
    float *fps;
    ShadowMap *shadow;
    glm::vec3 *lightPositions;

    std::vector<Model*> models;
    Model* newModel;

	void initImgui(Window myWindow)
	{
		Imgui_layer::getInstance().Init(myWindow.window);
	}

    void attachDeltaTimeAndFps(float &_deltaTime, float &_fps)
    {
        deltaTime = &_deltaTime;
        fps = &_fps;
    }

    void attachLight(glm::vec3 &_lightPositions)
    {
        lightPositions = &_lightPositions;
    }

    void attachShadow(ShadowMap &_shadow)
    {
        shadow = &_shadow;
    }

    void attachModels(std::vector<Model*> &_models)
    {
        models = _models;
    }

    void attach_newModel(Model &model)
    {
        newModel = &model;
    }

    void drawDebuggingState(GLuint gBufferFBO, int WIDTH, int HEIGHT, glm::mat4 &projection, glm::mat4 &view)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        noColorDebugShader.use();
        noColorDebugShader.setMat4("projection", projection);
        noColorDebugShader.setMat4("view", view);
    }

    // Drone checks
    std::string droneInfo = "";

    glm::quat quatDebug;
    glm::vec3 currentPos;
    glm::vec3 quatDummyTest;
    bool startMotors = true;
    glm::vec3 cameraLocation = glm::vec3(0.0f, 0.7f, 1.1f);
    glm::vec3 lookingCamera = glm::vec3(0.0f, 0.7f, 2.0f);
    float getMaxPower()
    {
        return maxPower;
    }

    bool time5sSend()
    {
        static float localTime = 0.0f;
        localTime = localTime + *deltaTime;
        if (localTime > 5.0f)
        {
            localTime = 0.0f;
            return true;
        }
        return false;
    }

    // States
    bool droneCamera = false;
    bool gamepadState = false;
    bool physicsDebugRender = false;
    int sensorID = 0;


    std::vector<glm::vec3> camPosition = {
        glm::vec3(0.0f, 0.7f, 1.4f), // front
        glm::vec3(0.0f, 0.7f, -0.8f), // back
        glm::vec3(0.9f, 0.7f, 0.0f), // left
        glm::vec3(-0.9f, 0.7f, 0.0f) // right
    }; 
    std::vector<glm::vec3> lookingPosition = {
        glm::vec3(0.0f, 0.7f, 2.0f), // front
        glm::vec3(0.0f, 0.7f, -2.0f), // back
        glm::vec3(2.0f, 0.8f, 0.0f), // left
        glm::vec3(-2.0f, 0.8f, 0.0f) // right
    };
    std::vector<glm::vec3> pillarsPositions = {
        glm::vec3(23.6f, 6.0f, 51.1f),
        glm::vec3(26.5f, 6.0f, 41.6f),
        glm::vec3(4.5f, 6.0f, 31.350f),

        glm::vec3(20.1f, 6.0f, 35.5f),
        glm::vec3(15.0f, 6.0f, 33.8f),
        glm::vec3(7.3f, 6.0f, 48.3f),

        glm::vec3(14.9f, 6.0f, 43.9f),
        glm::vec3(6.7f, 6.0f, 36.8f),
        glm::vec3(-0.3f, 6.0f, 44.3f),
    };

    //SSR values
    int uMaxSteps = 600;
    int uBinarySearchSteps = 8;
    float uStepSize = 0.1;
    float uThickness = 0.2;

    //SSAO values
    float aoMultiplayer = 5.0f;
    float aoRadius = 0.5f;
    float aoBias = 0.025f;
    float lightWidth = 4000.0f;

    // Drone target and power
    float droneTargetHeight = 0.0f;
    float maxPower = 4.0f;
    float minPower = 0.0f;
    
    // Drone values for vision
    float rollValue = 0.0f, pitchValue = 0.0f, middlePointDebug = 0.0f, outputYaw = 0.0f, outputHeight = 0.0f;
    
    // Algorithm settings
    float MIN_ALPHA = 0.8f, MAX_ALPHA = 1.0f, NOISE_THRESHOLD = 0.001f, JUMP_THRESHOLD = 0.001f;
    float SENSITIVITY_size = 5.8f;
    float SENSITIVITY_repulsion = 20.0f;
    float smoothness = 0.01;
    float safeSpeedDrone = 4.0f;
    bool showSpline = true;

    // Ai variables
    bool position_pid = false;
    glm::vec3 targetPosition = glm::vec3(0.0f, 2.0f, 0.0f);
    bool collision = false;
    bool remoteControl = false;
    float depth_near = 0.001f, depth_far = 100.0f;
    float cubeSizes = 0.03;
    float angularVel = 0.0f;
    bool startAvoidance = true;
    float learningRate = 2.5f;

    // Voxels data
    bool drawPoints = false;
    bool drawVoxelGrid = false;
    bool sendVoxelsNetwork_button = false;
    bool readVoxelGrid = true;
    
    // Propeller positions
    glm::vec3 posPropeller_FRONTLEFT = glm::vec3(1.88f, -0.35f, 1.62f);
    glm::vec3 posPropeller_BACKLEFT  = glm::vec3(1.69f, -0.35f, -1.56f);
    glm::vec3 posPropeller_FRONTRIGHT= glm::vec3(-1.90f, -0.35f, 1.61f);
    glm::vec3 posPropeller_BACKRIGHT = glm::vec3(-1.72f, -0.35f, -1.57f);

    glm::vec3 targetCollision = glm::vec3(0.0f);
    glm::vec3 forwardDir = glm::vec3(0.0f);

    glm::vec3 cameraDebugPos = glm::vec3(0.0f);
    bool applyPositionPillars = false;

	void setSlides()
	{
        // ---------------------------------------------------------------------------------------
        CollapsingHeader* stats = new CollapsingHeader("Stats");
        stats->addWidget(new Value("deltaTime:", deltaTime));
        stats->addWidget(new Value("FPS:", fps));
        stats->addWidget(new CheckBox("Gamepad connected", &gamepadState));
        Imgui_layer::getInstance().addWidget(stats);


        // ---------------------------------------------------------------------------------------
        CollapsingHeader* physicsDebug = new CollapsingHeader("Physics");
        physicsDebug->addWidget(new CheckBox("Debug Physics collisions", &physicsDebugRender));
        physicsDebug->addWidget(new CheckBox("Reset pillars", &applyPositionPillars));
        Imgui_layer::getInstance().addWidget(physicsDebug);


        // ---------------------------------------------------------------------------------------
        CollapsingHeader* visualLidarVoxels = new CollapsingHeader("Lidar n' Voxels");
        visualLidarVoxels->addWidget(new CheckBox("Draw debug LiDAR Points", &drawPoints));
        visualLidarVoxels->addWidget(new CheckBox("Draw debug Voxel Grid", &drawVoxelGrid));
        visualLidarVoxels->addWidget(new CheckBox("Save Voxels", &readVoxelGrid));
        visualLidarVoxels->addWidget(new CheckBox("Send Voxels Network", &sendVoxelsNetwork_button));
        visualLidarVoxels->addWidget(new DragFloat("Cube Sizes", &cubeSizes, 0.001f));
        Imgui_layer::getInstance().addWidget(visualLidarVoxels);


        // ---------------------------------------------------------------------------------------
        CollapsingHeader* obstacleAvoidance = new CollapsingHeader("Obstacle Avoidance");
        obstacleAvoidance->addWidget(new CheckBox("Activate position control", &position_pid));
        obstacleAvoidance->addWidget(new CheckBox("Start Avoidance", &startAvoidance));
        obstacleAvoidance->addWidget(new CheckBox("Remote controll Ai", &remoteControl));
        obstacleAvoidance->addWidget(new CheckBox("Show Spline", &showSpline));

        TreeNode* sensitivityNode = new TreeNode("Sensitivity and Learning Rate");
        sensitivityNode->addWidget(new DragFloat("Learning Rate", &learningRate, 0.01f));
        sensitivityNode->addWidget(new DragFloat("Speed transition", &smoothness, 0.01f));
        sensitivityNode->addWidget(new DragFloat("SENSITIVITY size", &SENSITIVITY_size, 0.1f));
        sensitivityNode->addWidget(new DragFloat("SENSITIVITY Repulsion", &SENSITIVITY_repulsion, 0.1f));      
        sensitivityNode->addWidget(new DragFloat("Maximum speed cruising", &safeSpeedDrone, 0.1f));


        TreeNode* targetNode = new TreeNode("Target drone position");
        targetNode->addWidget(new DragPosRotScale(&targetPosition, 0.05f));
        obstacleAvoidance->addWidget(sensitivityNode);
        obstacleAvoidance->addWidget(targetNode);
        Imgui_layer::getInstance().addWidget(obstacleAvoidance);

        // ---------------------------------------------------------------------------------------
        //Drone
        CollapsingHeader* droneHeader = new CollapsingHeader("Drone Sliders and Informations");
        TreeNode* positionAndQuat = new TreeNode("Current Position and Quaternion + target Quat");
        positionAndQuat->addWidget(new Value("Height target:", &droneTargetHeight));
        positionAndQuat->addWidget(new ImGUI_text("Quaternion"));
        positionAndQuat->addWidget(new Value("W:", &quatDebug.w));   positionAndQuat->addWidget(new SameLine());
        positionAndQuat->addWidget(new Value(" X:", &quatDebug.x));  positionAndQuat->addWidget(new SameLine());
        positionAndQuat->addWidget(new Value(" Y:", &quatDebug.y));  positionAndQuat->addWidget(new SameLine());
        positionAndQuat->addWidget(new Value(" Z:", &quatDebug.z));
        positionAndQuat->addWidget(new ImGUI_text("Position"));
        positionAndQuat->addWidget(new Value(" X:", &currentPos.x)); positionAndQuat->addWidget(new SameLine());
        positionAndQuat->addWidget(new Value(" Y:", &currentPos.y)); positionAndQuat->addWidget(new SameLine());
        positionAndQuat->addWidget(new Value(" Z:", &currentPos.z));
        positionAndQuat->addWidget(new ImGUI_text("Quaternion target"));
        positionAndQuat->addWidget(new DragPosRotScale(&quatDummyTest, 0.1f));

        TreeNode* velocityAndData = new TreeNode("Velocity/Roll-Pitch/Yaw/");
        velocityAndData->addWidget(new Value("Linear Velocity:", &angularVel));
        velocityAndData->addWidget(new Value("ROLL Value:", &rollValue));
        velocityAndData->addWidget(new Value("PITCH value:", &pitchValue));
        velocityAndData->addWidget(new Value("MIDDDLE point:", &middlePointDebug));
        velocityAndData->addWidget(new Value("Output Yaw:", &outputYaw));
        velocityAndData->addWidget(new Value("Output Pitch:", &outputHeight));

        TreeNode* motorData = new TreeNode("Thrust and arming");
        motorData->addWidget(new CheckBox("Start motors", &startMotors));
        motorData->addWidget(new DragFloat("Motor thrust power", &maxPower, 0.1f));
        motorData->addWidget(new DragFloat("Motor minimum power", &minPower, 0.1f));
        
        TreeNode* cameraAndSensors = new TreeNode("Camera and Sensors");
        cameraAndSensors->addWidget(new CheckBox("Drone camera", &droneCamera));
        cameraAndSensors->addWidget(new ImGUI_text("Down Camera Pos, Look"));
        cameraAndSensors->addWidget(new DragPosRotScale(&camPosition[0], 0.01f));
        cameraAndSensors->addWidget(new DragPosRotScale(&lookingPosition[0], 0.01f));
        cameraAndSensors->addWidget(new ImGUI_text("Sensor index"));
        cameraAndSensors->addWidget(new InputInt("Map Select", &sensorID, 0, 3));

        TreeNode* pidsEdit = new TreeNode("PIDs Edit");
        pidsEdit->addWidget(new ImGUI_text("pidPitch"));
        pidsEdit->addWidget(new DragFloat("kp", &pidPitch.kp, 0.05f));
        pidsEdit->addWidget(new DragFloat("ki", &pidPitch.ki, 0.05f));
        pidsEdit->addWidget(new DragFloat("kd", &pidPitch.kd, 0.05f));
        pidsEdit->addWidget(new ImGUI_text("pidRoll"));
        pidsEdit->addWidget(new DragFloat("kp", &pidRoll.kp, 0.05f));
        pidsEdit->addWidget(new DragFloat("ki", &pidRoll.ki, 0.05f));
        pidsEdit->addWidget(new DragFloat("kd", &pidRoll.kd, 0.05f));
        pidsEdit->addWidget(new ImGUI_text("pidYaw"));
        pidsEdit->addWidget(new DragFloat("kp", &pidYaw.kp, 0.05f));
        pidsEdit->addWidget(new DragFloat("ki", &pidYaw.ki, 0.05f));
        pidsEdit->addWidget(new DragFloat("kd", &pidYaw.kd, 0.05f));
        pidsEdit->addWidget(new ImGUI_text("pidHeight"));
        pidsEdit->addWidget(new DragFloat("kp", &pidHeight.kp, 0.05f));
        pidsEdit->addWidget(new DragFloat("ki", &pidHeight.ki, 0.05f));
        pidsEdit->addWidget(new DragFloat("kd", &pidHeight.kd, 0.05f));
        pidsEdit->addWidget(new ImGUI_text("pid X axis"));
        pidsEdit->addWidget(new DragFloat("kp", &pidX.kp, 0.01f));
        pidsEdit->addWidget(new DragFloat("ki", &pidX.ki, 0.05f));
        pidsEdit->addWidget(new DragFloat("kd", &pidX.kd, 0.05f));
        pidsEdit->addWidget(new ImGUI_text("pid Z axis"));
        pidsEdit->addWidget(new DragFloat("kp", &pidZ.kp, 0.05f));
        pidsEdit->addWidget(new DragFloat("ki", &pidZ.ki, 0.05f));
        pidsEdit->addWidget(new DragFloat("kd", &pidZ.kd, 0.05f));

        droneHeader->addWidget(positionAndQuat);
        droneHeader->addWidget(velocityAndData);
        droneHeader->addWidget(motorData);
        droneHeader->addWidget(cameraAndSensors);
        droneHeader->addWidget(pidsEdit);
        Imgui_layer::getInstance().addWidget(droneHeader);


        // ---------------------------------------------------------------------------------------
        CollapsingHeader* ssrNao = new CollapsingHeader("SSR and SSAO parameters");
        ssrNao->addWidget(new InputInt("SSR uMaxSteps", &uMaxSteps, 0, 1200));
        ssrNao->addWidget(new InputInt("SSR uBinarySearchSteps", &uBinarySearchSteps, 0, 80));
        ssrNao->addWidget(new DragFloat("SSR uStepSize", &uStepSize, 0.1f));
        ssrNao->addWidget(new DragFloat("SSR uThickness", &uThickness, 0.1f));
        ssrNao->addWidget(new DragFloat("SSAO multiplayer", &aoMultiplayer, 0.1f));
        ssrNao->addWidget(new DragFloat("SSAO radius", &aoRadius, 0.05f));
        ssrNao->addWidget(new DragFloat("SSAO bias", &aoBias, 0.05f));
        Imgui_layer::getInstance().addWidget(ssrNao);


        // ---------------------------------------------------------------------------------------
        CollapsingHeader* lightShadow = new CollapsingHeader("Light and Shadow sliders");
        TreeNode* shadowNode = new TreeNode("Shadows");
        shadowNode->addWidget(new DragFloat("depth_near", &depth_near, 0.01f));
        shadowNode->addWidget(new DragFloat("depth_far", &depth_far, 0.1f));
        shadowNode->addWidget(new DragFloat("FarPlane", &shadow->far_plane, 0.1f));
        shadowNode->addWidget(new DragFloat("Frustrum", &shadow->frustrum, 0.1f));
        shadowNode->addWidget(new DragFloat("shadowUp", &shadowUp, 0.0001f));
        shadowNode->addWidget(new DragFloat("shadowBias", &shadowBias, 0.000f));
        shadowNode->addWidget(new InputInt("pcfSize", &pcfSize, 0, 20));
        shadowNode->addWidget(new DragFloat("shadow contrast", &multipalyer, 0.1f));

        TreeNode* lightNode = new TreeNode("Lights");
        lightNode->addWidget(new DragFloat("lightMultiplayer", &lightMultiplayer, 0.5f));
        lightNode->addWidget(new ImGUI_text("LightPosition"));
        lightNode->addWidget(new DragPosRotScale(lightPositions, 0.1f));
        lightNode->addWidget(new DragFloat("Light Width", &lightWidth, 0.05f));
        lightNode->addWidget(new DragFloat("ambient_occlusion", &ambient_occlusion, 0.1f));

        lightShadow->addWidget(shadowNode);
        lightShadow->addWidget(lightNode);
        Imgui_layer::getInstance().addWidget(lightShadow);


        // ---------------------------------------------------------------------------------------
        CollapsingHeader* materials = new CollapsingHeader("Materials and Texture types");
        materials->addWidget(new InputInt("Texture type (with or without)", &textureSelect, 0, 1));
        materials->addWidget(new Slider3("Color", color, 0.0f, 1.0f));
        materials->addWidget(new Slider("Metal", &metal, 0.0f, 1.0f));
        materials->addWidget(new Slider("Roughness", &roughness, 0.0f, 1.0f));
        materials->addWidget(new Slider3("Normal", normal, 0.0f, 1.0f));
        Imgui_layer::getInstance().addWidget(materials);


        // ---------------------------------------------------------------------------------------
        // Models and Environment
        CollapsingHeader* environmentHeader = new CollapsingHeader("Environment");
        TreeNode* pillarsNode = new TreeNode("Pillars position with physics");
        for (int i = 0; i < pillarsPositions.size(); i++)
        {
            std::string pillarNumber = "Pillar##" + std::to_string(i);
            TreeNode* pillarInsert = new TreeNode(pillarNumber.c_str());
            pillarInsert->addWidget(new DragPosRotScale(&pillarsPositions[i], 0.01f));

            pillarsNode->addWidget(pillarInsert);
        }

        TreeNode* modelsNode = new TreeNode("Models Position + Rotation + Scale");
        for (int i = 0; i < models.size(); i++)
        {
            std::string modelNumber = "Model##" + std::to_string(i);
            TreeNode* modelInsert = new TreeNode(modelNumber.c_str());
            modelInsert->addWidget(new ImGUI_text("POSITION"));
            modelInsert->addWidget(new DragPosRotScale(&models[i]->position, 0.01f));
            modelInsert->addWidget(new ImGUI_text("ROTATE"));
            modelInsert->addWidget(new DragPosRotScale(&models[i]->rotation, 0.01f));
            modelInsert->addWidget(new ImGUI_text("SCALE"));
            modelInsert->addWidget(new DragPosRotScale(&models[i]->size, 0.01f));

            modelsNode->addWidget(modelInsert);
        }
	}

    void quaternionSlide(std::string text, glm::quat &quaternion)
    {
        std::string localText = "Quaternion " + text;
        Imgui_layer::getInstance().addWidget(new ImGUI_text(localText));
        Imgui_layer::getInstance().addWidget(new Value("W:",  &quaternion.w)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" X:", &quaternion.x)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Y:", &quaternion.y)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Z:", &quaternion.z));
    }

    void positionSlide(std::string text, glm::vec3& positioning)
    {
        std::string localText = "Position " + text;
        Imgui_layer::getInstance().addWidget(new ImGUI_text(localText));
        Imgui_layer::getInstance().addWidget(new Value(" X:", &positioning.x)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Y:", &positioning.y)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Z:", &positioning.z));
    }

	void setShader(Shader& shader, Shader &gShader) const
	{
        gShader.use();
        gShader.setInt("textureSelect", textureSelect);

        gShader.setVec3("u_albedo", glm::vec3(color[0], color[1], color[2]));
        gShader.setFloat("u_metallic", metal);
        gShader.setFloat("u_roughness", roughness);
        gShader.setVec3("u_normal", glm::vec3(normal[0], normal[1], normal[2]));

        shader.use();
		shader.setFloat("shadowUp", shadowUp);
		shader.setFloat("shadowBias", shadowBias);
		shader.setInt("pcfSize", pcfSize);
		shader.setFloat("multiplayer", multipalyer);
		shader.setFloat("ao", ambient_occlusion);
	}


};


#endif // !_DEBUGGERCLASS_
