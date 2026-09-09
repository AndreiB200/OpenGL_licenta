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

class CollapsingHeader : public Widget {
public:
	CollapsingHeader(const char* _name, bool _defaultOpen = false)
		: name(_name), defaultOpen(_defaultOpen) {
	}

	void addWidget(Widget* widget) {
		children.push_back(widget);
	}

	void run() override {
		ImGuiTreeNodeFlags flags = defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;

		if (ImGui::CollapsingHeader(name, flags)) {
			for (size_t i = 0; i < children.size(); i++) {
				ImGui::PushID(static_cast<int>(i));
				children[i]->run();
				ImGui::PopID();
			}
		}
	}

private:
	const char* name;
	bool defaultOpen;
	std::vector<Widget*> children;
};

class TreeNode : public Widget {
public:
	TreeNode(const std::string& _name, bool _defaultOpen = false)
		: name(_name), defaultOpen(_defaultOpen) {
	}

	void addWidget(Widget* widget) {
		children.push_back(widget);
	}

	void run() override {
		ImGuiTreeNodeFlags flags = defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
		if (ImGui::TreeNodeEx(name.c_str(), flags)) 
		{
			for (size_t i = 0; i < children.size(); i++) {
				ImGui::PushID(static_cast<int>(i));
				children[i]->run();
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}

private:
	std::string name;
	bool defaultOpen;
	std::vector<Widget*> children;
};

class PIDDebuggerWidget : public Widget {
public:
	static constexpr size_t HISTORY_SIZE = 100;

	PIDDebuggerWidget(const std::string& label, const PIDController* pid, float maxTorque = 50.0f, float _y_Size = 60.0f)
		: label(label), pidPtr(pid), maxTorque(maxTorque),
		historyError(HISTORY_SIZE, 0.0f),
		historyP(HISTORY_SIZE, 0.0f),
		historyI(HISTORY_SIZE, 0.0f),
		historyD(HISTORY_SIZE, 0.0f),
		historyTorque(HISTORY_SIZE, 0.0f),
		y_Size(_y_Size){
	}

	void run() override {
		if (!pidPtr) return;

		ImGui::Text(label.c_str());
		historyError[offset] = pidPtr->lastError;
		historyP[offset] = pidPtr->p_term;
		historyI[offset] = pidPtr->i_term;
		historyD[offset] = pidPtr->d_term;
		historyTorque[offset] = pidPtr->lastOutput;

		offset = (offset + 1) % HISTORY_SIZE;

		ImGui::PushID(label.c_str());
		ImGui::PlotLines("Error data", historyError.data(), (int)HISTORY_SIZE, offset, nullptr, -3.14f, 3.14f, ImVec2(0, y_Size));

		ImGui::Separator();
		ImGui::Text("Components:");
		ImGui::PlotLines("P", historyP.data(), (int)HISTORY_SIZE, offset, nullptr, -maxTorque, maxTorque, ImVec2(0, y_Size));
		ImGui::PlotLines("I", historyI.data(), (int)HISTORY_SIZE, offset, nullptr, -maxTorque, maxTorque, ImVec2(0, y_Size));
		ImGui::PlotLines("D", historyD.data(), (int)HISTORY_SIZE, offset, nullptr, -maxTorque, maxTorque, ImVec2(0, y_Size));

		ImGui::Separator();
		ImGui::PlotLines("Torque Output", historyTorque.data(), (int)HISTORY_SIZE, offset, nullptr, -maxTorque, maxTorque, ImVec2(0, y_Size));
		ImGui::Separator();

		ImGui::Text("Integral Accumulator: %.3f", pidPtr->integral);
		ImGui::Text("Last Output:          %.3f", pidPtr->lastOutput);

		ImGui::PopID();
	}

private:
	std::string label;
	const PIDController* pidPtr;
	float maxTorque;
	float y_Size;

	int offset = 0;
	std::vector<float> historyError;
	std::vector<float> historyP;
	std::vector<float> historyI;
	std::vector<float> historyD;
	std::vector<float> historyTorque;
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
		ApplyModernStyle();
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

	void ApplyModernStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		style.WindowRounding = 8.0f;
		style.ChildRounding = 6.0f; 
		style.FrameRounding = 5.0f;
		style.PopupRounding = 6.0f;
		style.ScrollbarRounding = 8.0f;
		style.GrabRounding = 4.0f;

		style.WindowPadding = ImVec2(12.0f, 12.0f);
		style.FramePadding = ImVec2(8.0f, 5.0f);
		style.ItemSpacing = ImVec2(10.0f, 8.0f);
		style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
		style.IndentSpacing = 20.0f;
		style.ScrollbarSize = 14.0f;
		style.FrameBorderSize = 1.0f;

		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.16f, 0.98f);
		colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.27f, 0.50f);

		colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.33f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.42f, 1.00f);

		colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.33f, 1.00f);

		colors[ImGuiCol_CheckMark] = ImVec4(0.55f, 0.42f, 0.95f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.35f, 0.85f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.42f, 0.95f, 1.00f);

		colors[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.35f, 0.85f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.42f, 0.95f, 1.00f);

		colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.50f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
	}

	void getVRAM()
	{
		glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &cur_avail_mem_kb);
		glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &total_mem_kb);
		ImGui::Value("VRAM used:", (total_mem_kb - cur_avail_mem_kb) / 1000);
	}

};