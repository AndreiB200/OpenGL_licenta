#define _DEBUGGERCLASS_
#ifdef _DEBUGGERCLASS_

#include "Shader.h"

class DebuggerClass
{
public:
	DebuggerClass(){}

	bool secvential = false;

	//textures edit
	float metal = 0.0f, roughness = 0.0f;
	float normal[3] = {0.0f,0.0f,0.0f};
	float color[3] = {0.7f, 0.7f, 0.7f};

	int textureSelect = 0; //0 color, 1 metallic, 2 normal, 3 roughness
	void setShader(Shader& shader) const
	{
		shader.setInt("textureSelect", textureSelect);

		shader.setVec3("u_albedo", glm::vec3(color[0], color[1], color[2]));
		shader.setFloat("u_metallic", metal);
		shader.setFloat("u_roughness", roughness);
		shader.setVec3("u_normal", glm::vec3(normal[0], normal[1], normal[2]));
	}


};


#endif // !_DEBUGGERCLASS_
