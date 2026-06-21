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
	float metal = 0.0f, roughness = 0.0f;
	float normal[3] = {0.0f,0.0f,0.0f};
	float color[3] = {0.7f, 0.7f, 0.7f};

	float shadowUp = 0.000f, shadowBias = 0.0001f, multipalyer = 0.1f, ambient_occlusion = 1.0f, lightMultiplayer = 1.0f;
	int pcfSize = 3;

	int index = 0;

	int textureSelect = 0; //0 constant color, 1 textured, 2 color, 3 metal, 4 roughness

    // Variables that need to be attached (prototype, needs improvment)
    float *deltaTime;
    ShadowMap *shadow;
    glm::vec3 *lightPositions;

    std::vector<Model*> models;
    Model *newModel;

	void initImgui(Window myWindow)
	{
		Imgui_layer::getInstance().Init(myWindow.window);
	}

    void attachdeltaTime(float &_deltaTime)
    {
        deltaTime = &_deltaTime;
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
    float maxPower = 2.0f;
    glm::quat quatDebug;
    glm::vec3 quatDummyTest;
    bool startMotors = true;
    float getMaxPower()
    {
        return maxPower;
    }
    // PID
    bool droneCamera = false;


	void setSlides()
	{
        Imgui_layer::getInstance().addWidget(new Value("deltaTime:", deltaTime));
        Imgui_layer::getInstance().addWidget(new Value("W:", &quatDebug.w)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" X:", &quatDebug.x)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Y:", &quatDebug.y)); Imgui_layer::getInstance().addWidget(new SameLine());
        Imgui_layer::getInstance().addWidget(new Value(" Z:", &quatDebug.z));

        // PID values
        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("pidPitch"));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidPitch.kp, 0.1f)); 
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidPitch.ki, 0.1f)); 
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidPitch.kd, 0.1f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("pidRoll"));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidRoll.kp, 0.1f)); 
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidRoll.ki, 0.1f)); 
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidRoll.kd, 0.1f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("pidYaw"));
        Imgui_layer::getInstance().addWidget(new DragFloat("kp", &pidYaw.kp, 0.1f)); 
        Imgui_layer::getInstance().addWidget(new DragFloat("ki", &pidYaw.ki, 0.1f)); 
        Imgui_layer::getInstance().addWidget(new DragFloat("kd", &pidYaw.kd, 0.1f));


        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new DragFloat("FarPlane", &shadow->far_plane, 0.1f));
        Imgui_layer::getInstance().addWidget(new DragFloat("Frustrum", &shadow->frustrum, 0.1f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("LightPosition"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(lightPositions, 0.1f));

        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new DragFloat("Motor thrust power", &maxPower, 0.1f));
        Imgui_layer::getInstance().addWidget(new ImGUI_text(""));
        Imgui_layer::getInstance().addWidget(new CheckBox("Start motors", &startMotors));
        Imgui_layer::getInstance().addWidget(new ImGUI_text("Quaternion TEST"));
        Imgui_layer::getInstance().addWidget(new DragPosRotScale(&quatDummyTest, 0.1f));
        Imgui_layer::getInstance().addWidget(new CheckBox("Drone camera", &droneCamera));





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
        Imgui_layer::getInstance().addWidget(new InputInt("Map Select", &textureSelect, 0, 4));
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

	void setShader(Shader& shader) const
	{
		shader.setInt("textureSelect", textureSelect);

		shader.setVec3("u_albedo", glm::vec3(color[0], color[1], color[2]));
		shader.setFloat("u_metallic", metal);
		shader.setFloat("u_roughness", roughness);
		shader.setVec3("u_normal", glm::vec3(normal[0], normal[1], normal[2]));

		shader.setFloat("shadowUp", shadowUp);
		shader.setFloat("shadowBias", shadowBias);
		shader.setInt("pcfSize", pcfSize);
		shader.setFloat("multiplayer", multipalyer);
		shader.setFloat("ao", ambient_occlusion);
	}




};


#endif // !_DEBUGGERCLASS_
