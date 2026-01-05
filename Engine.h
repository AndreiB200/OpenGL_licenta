#include "Scene.h"
#include "Model.h"
#include "Camera.h"
#include "ShadowMap.h"


class Engine
{
	Shader shader = Shader("pbr.vert", "pbr.frag");

	Shader screenShader = Shader("frame.vert", "frame.frag");
	Shader shadowShader = Shader("shadow.vert", "shadow.frag");
};