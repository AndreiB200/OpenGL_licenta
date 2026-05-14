#pragma once
#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glm/glm.hpp>

#include "Shader.h"
#include "Model.h"
#include "Texture.h"
#include "StructuresHelpers.h"

class Widget
{
public:
	virtual ~Widget() {}
	virtual void run() = 0;
};

class Value : public Widget
{
public:
	Value(const char* _name, float* _value) :
		name(_name), value(_value) {}
	void run()
	{
		ImGui::Value(name, *value);
	}
private:
	const char* name;
	float* value;
};

class Slider3 : public Widget
{
public:
	Slider3(const char* _name, float* _value, float _range0, float _range1) :
		name(_name), value(_value), range0(_range0), range1(_range1) {}

	void run()
	{
		ImGui::SliderFloat3(name, value, range0, range1);
	}
private:
	const char* name;
	float* value;
	float range0, range1;
};

class VecSlider3 : public Widget
{
public:
	VecSlider3(const char* _name, glm::vec3* _value, float _range0, float _range1) :
		name(_name), value(_value), range0(_range0), range1(_range1) {
	}

	void run()
	{
		values[0] = value->x; values[1] = value->y; values[2] = value->z;
		ImGui::SliderFloat3(name, values, range0, range1);
		value->x = values[0];
		value->y = values[1];
		value->z = values[2];
	}
private:
	const char* name;
	glm::vec3* value;
	float values[3];
	float range0, range1;
};

class Slider : public Widget
{
public:
	Slider(const char* _name, float* _value, float _range0, float _range1) :
		name(_name), value(_value), range0(_range0), range1(_range1) {}

	void run()
	{
		ImGui::SliderFloat(name, value, range0, range1);
	}
private:
	const char* name;
	float* value;
	float range0, range1;
};

class DragFloat : public Widget
{
public:
	DragFloat(const char* _name, float* _value, float _steps) :
		name(_name), value(_value), steps(_steps) {
	}

	void run()
	{
		ImGui::DragFloat(name, value, steps);
	}
private:
	const char* name;
	float* value;
	float steps;
};

class DragPosRotScale : public Widget
{
public:
	DragPosRotScale(glm::vec3* _input, float _steps) :
		input(_input), steps(_steps) {
	}

	void run()
	{
		ImGui::DragFloat("X:", &input->x, steps);
		ImGui::DragFloat("Y:", &input->y, steps);
		ImGui::DragFloat("Z:", &input->z, steps);
	}
private:
	glm::vec3* input;
	float steps;
};

class InputInt : public Widget
{
public:
	InputInt(const char* _name, int* _value, int _range0, int _range1) :
		name(_name), value(_value), range0(_range0), range1(_range1) {
	}

	void run()
	{
		ImGui::SliderInt(name, value, range0, range1);
	}
private:
	const char* name;
	int* value;
	int range0, range1;
};


class RadioButton : public Widget
{
public:
	RadioButton(const char* _name, int* _val, int _button, bool _sameline = false) :
		name(_name), val(_val), button(_button), sameline(_sameline) {}

	void run()
	{
		ImGui::RadioButton(name, val, button);
		if (sameline) ImGui::SameLine();
	}

private:
	const char* name;
	int* val;
	int button = 0;
	bool sameline = false;
};

class CheckBox : public Widget
{
public:
	CheckBox(const char* _name, bool* _value) :
		name(_name), value(_value) {}

	void run()
	{
		ImGui::Checkbox(name, value);
	}
private:
	const char* name;
	bool* value;
};

class SameLine : public Widget
{
public:
	SameLine(){}

	void run()
	{
		ImGui::SameLine();
	}
};

class ImGUI_text : public Widget
{
public:
	ImGUI_text(std::string _text): text(_text) {}

	void run()
	{
		ImGui::Text(text.c_str());
	}
private:
	std::string text = "";
};




class Imgui_layer
{
public:

	static Imgui_layer& getInstance()
	{
		static Imgui_layer instance;
		return instance;
	}

	Imgui_layer(const Imgui_layer&) = delete;
	void operator=(const Imgui_layer&) = delete;

	void Update()
	{		
		NewFrame();

		for (int i = 0; i < widgetList.size(); i++)
		{
			ImGui::PushID(i);
			widgetList[i]->run();
			ImGui::PopID();
		}

		getVRAM();
		
		Render();
	}

	void addWidget(Widget* widget)
	{
		widgetList.push_back(widget);
	}

	void Init(GLFWwindow* window)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		io = ImGui::GetIO(); (void)io;

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 460");
		ImGui::StyleColorsDark();
	}

	void ShutDown()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	
private:
	ImGuiIO io;
	float xTime = 0.0f, result = 0.0f;
	int a = 0;
	int cur_avail_mem_kb = 0;
	int total_mem_kb = 0;

	std::vector<Widget*> widgetList;

	Imgui_layer() {}

	void NewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("ImGUI window");
		ImGui::Text("Debugging window");
	}

	void Render()
	{
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void getVRAM()
	{
		glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &cur_avail_mem_kb);
		glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &total_mem_kb);
		ImGui::Value("VRAM used:", (total_mem_kb - cur_avail_mem_kb) / 1000);
	}

};