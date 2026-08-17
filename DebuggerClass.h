#define _DEBUGGERCLASS_
#ifdef _DEBUGGERCLASS_

#include "Shader.h"
#include "Imgui_layer.h"
#include "ShadowMap.h"

class DebuggerClass
{
public:
	DebuggerClass(){}

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
    Model *newModel;

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

    // Drone checks
    std::string droneInfo = "";

    glm::quat quatDebug;
    glm::vec3 currentPos;
    glm::vec3 quatDummyTest;
    bool startMotors = true;
    glm::vec3 cameraLocation = glm::vec3(0.0f, -0.3f, 0.0f);
    glm::vec3 lookingCamera = glm::vec3(0.0f, -0.5f, 2.0f);
    float getMaxPower()
    {
        return maxPower;
    }
    // States
    bool droneCamera = false;
    bool gamepadState = false;
    bool physicsDebugRender = true;

    //SSR values
    int uMaxSteps = 60;
    int uBinarySearchSteps = 8;
    float uStepSize = 0.1;
    float uThickness = 0.2;

    //SSAO values
    float aoMultiplayer = 1.0f;
    float aoRadius = 0.5f;
    float aoBias = 0.025f;

    // Drone target and power
    float droneTargetHeight = 0.0f;
    float maxPower = 4.0f;
    float minPower = 0.0f;
    
    // Drone values for vision
    float rollValue = 0.0f, pitchValue = 0.0f, middlePointDebug = 0.0f, outputRoll = 0.0f, outputPitch = 0.0f;
    
    // Filter variables
    float MIN_ALPHA = 0.1f, MAX_ALPHA = 1.0f, NOISE_THRESHOLD = 0.004f, JUMP_THRESHOLD = 0.003f;
    float SENSITIVITY = 2.0f;

    // Ai variables
    bool inteligenta_artificiala = false;
    glm::vec3 targetPosition = glm::vec3(0.0f, 2.0f, 0.0f);
    bool collision = false;
    bool remoteControl = false;
    float depth_near = 0.001f, depth_far = 1000.0f;
    
    // Propeller positions
    glm::vec3 posPropeller_FRONTLEFT = glm::vec3(1.88f, -0.35f, 1.62f);
    glm::vec3 posPropeller_BACKLEFT  = glm::vec3(1.69f, -0.35f, -1.56f);
    glm::vec3 posPropeller_FRONTRIGHT= glm::vec3(-1.90f, -0.35f, 1.61f);
    glm::vec3 posPropeller_BACKRIGHT = glm::vec3(-1.72f, -0.35f, -1.57f);

    glm::vec3 targetCollision = glm::vec3(0.0f);
    glm::vec3 forwardDir = glm::vec3(0.0f);

    float lightWidth = 0.05f;

	void setSlides()
	{
        Imgui_layer::getInstance().addWidget(new Value("deltaTime:", deltaTime));
        Imgui_layer::getInstance().addWidget(new Value("FPS:", fps));
        Imgui_layer::getInstance().addWidget(new CheckBox("Gamepad connected", &gamepadState));
        Imgui_layer::getInstance().addWidget(new CheckBox("Remote controll Ai", &remoteControl));
        Imgui_layer::getInstance().addWidget(new CheckBox("Debug Physics collisions", &physicsDebugRender));

        Imgui_layer::getInstance().addWidget(new InputInt("SSR uMaxSteps", &uMaxSteps, 0, 600));
        Imgui_layer::getInstance().addWidget(new InputInt("SSR uBinarySearchSteps", &uBinarySearchSteps, 0, 80));
        Imgui_layer::getInstance().addWidget(new DragFloat("SSR uStepSize", &uStepSize, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("SSR uThickness", &uThickness, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("SSAO multiplayer", &aoMultiplayer, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("SSAO radius", &aoRadius, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("SSAO bias", &aoBias, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("Light Width", &lightWidth, 0.05f));


        


        Imgui_layer::getInstance().addWidget(new Value("Height target:", &droneTargetHeight));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("Drone quaternion"));
        Imgui_layer::getInstance().addWidget(new Value("W:", &quatDebug.w)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" X:", &quatDebug.x)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Y:", &quatDebug.y)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Z:", &quatDebug.z));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("Drone Position"));
        Imgui_layer::getInstance().addWidget(new Value(" X:", &currentPos.x)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Y:", &currentPos.y)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Z:", &currentPos.z));
        Imgui_layer::getInstance().addWidget(new DragFloat("depth_near", &depth_near, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("depth_far", &depth_far, 0.1f));

        positionSlide("target", targetCollision);
        Imgui_layer::getInstance().addWidget(new CheckBox("Collision detected?", &collision));
        positionSlide("directie de mers", forwardDir);


        // PID values
        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("pidPitch"));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidPitch.kp, 0.05f)); 
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidPitch.ki, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidPitch.kd, 0.05f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("pidRoll"));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidRoll.kp, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidRoll.ki, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidRoll.kd, 0.05f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("pidYaw"));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidYaw.kp, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidYaw.ki, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidYaw.kd, 0.05f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("pidHeight"));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidHeight.kp, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidHeight.ki, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidHeight.kd, 0.05f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("pid X and Z"));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidX.kp, 0.01f));
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidX.ki, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidX.kd, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidZ.kp, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidZ.ki, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidZ.kd, 0.05f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("Target position"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&targetPosition, 0.02f));

        //Drone
        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new CheckBox("inteligenta_artificiala", &inteligenta_artificiala));
        Imgui_layer::getInstance().addWidget(new DragFloat("Motor thrust power", &maxPower, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("Motor minimum power", &minPower, 0.1f));
        Imgui_layer::getInstance().addWidget(new Value("ROLL Value:", &rollValue));
        Imgui_layer::getInstance().addWidget(new Value("PITCH value:", &pitchValue));
        Imgui_layer::getInstance().addWidget(new Value("MIDDDLE point:", &middlePointDebug));
        Imgui_layer::getInstance().addWidget(new Value("Output Roll:", &outputRoll));
        Imgui_layer::getInstance().addWidget(new Value("Output Pitch:", &outputPitch));
        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new CheckBox("Start motors", &startMotors));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("Quaternion TEST"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&quatDummyTest, 0.1f));
        Imgui_layer::getInstance().addWidget(new CheckBox("Drone camera", &droneCamera));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("Camera Location and Look"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&cameraLocation, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&lookingCamera, 0.1f));

        Imgui_layer::getInstance().addWidget(new DragFloat("MIN_ALPHA", &MIN_ALPHA, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("MAX_ALPHA", &MAX_ALPHA, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("NOISE_THRESHOLD", &NOISE_THRESHOLD, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("JUMP_THRESHOLD", &JUMP_THRESHOLD, 0.05f));
        Imgui_layer::getInstance().addWidget(new DragFloat("SENSITIVITY TRUST", &SENSITIVITY, 0.1f));

        Imgui_layer::getInstance().addWidget(new ImGUI_text("FRONT LEFT"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&posPropeller_FRONTLEFT, 0.01f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("BACK LEFT"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&posPropeller_BACKLEFT, 0.01f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("FRONT RIGHT"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&posPropeller_FRONTRIGHT, 0.01f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("BACK RIGHT"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&posPropeller_BACKRIGHT, 0.01f));



        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new DragFloat("FarPlane", &shadow->far_plane, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("Frustrum", &shadow->frustrum, 0.1f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("LightPosition"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(lightPositions, 0.1f));


        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new DragFloat("shadowUp", &shadowUp, 0.0001f));
        Imgui_layer::getInstance().addWidget(new DragFloat("shadowBias", &shadowBias, 0.000f));
        Imgui_layer::getInstance().addWidget(new InputInt("pcfSize", &pcfSize, 0, 20));
        Imgui_layer::getInstance().addWidget(new DragFloat("multiplyer", &multipalyer, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("ambient_occlusion", &ambient_occlusion, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("lightMultiplayer", &lightMultiplayer, 0.5f));



        Imgui_layer::getInstance().addWidget(new ImGUI_text("Material select"));
        Imgui_layer::getInstance().addWidget(new CheckBox("Secvential", &secvential));
        Imgui_layer::getInstance().addWidget(new InputInt("Mesh ID", &newModel->selectMesh, 0, static_cast<int>(newModel->meshNumbers - 1)));
        Imgui_layer::getInstance().addWidget(new InputInt("Texture ID", &newModel->selectTexture, 0, static_cast<int>(newModel->texture.size() - 1)));

        Imgui_layer::getInstance().addWidget(new ImGUI_text("Modify material"));
        Imgui_layer::getInstance().addWidget(new InputInt("Map Select", &textureSelect, 0, 1));
        Imgui_layer::getInstance().addWidget(new Slider3("Color", color, 0.0f, 1.0f));
        Imgui_layer::getInstance().addWidget(new Slider("Metal", &metal, 0.0f, 1.0f));
        Imgui_layer::getInstance().addWidget(new Slider("Roughness", &roughness, 0.0f, 1.0f));
        Imgui_layer::getInstance().addWidget(new Slider3("Normal", normal, 0.0f, 1.0f));

        for (int i = 0; i < models.size(); i++)
        {
            Imgui_layer::getInstance().addWidget(new ImGUI_text(""));

            std::string modelNumber = "Model##" + std::to_string(i);
            Imgui_layer::getInstance().addWidget(new ImGUI_text(modelNumber));

            Imgui_layer::getInstance().addWidget(new ImGUI_text("POSITION"));
            Imgui_layer::getInstance().addWidget(new DragPosRotScale(&models[i]->position, 0.01f));

            Imgui_layer::getInstance().addWidget(new ImGUI_text("ROTATE"));
            Imgui_layer::getInstance().addWidget(new DragPosRotScale(&models[i]->rotation, 0.01f));

            Imgui_layer::getInstance().addWidget(new ImGUI_text("SCALE"));
            Imgui_layer::getInstance().addWidget(new DragPosRotScale(&models[i]->size, 0.01f));
        }


        Imgui_layer::getInstance().addWidget(new InputInt("Map Select", &index, 0, 6));
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
