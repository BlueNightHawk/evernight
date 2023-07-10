#pragma once

#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_sdl2.h"

class CImGui
{
public:
	void Init();
	void Update();
	void Shutdown();

private:
};

extern CImGui g_ImGui;