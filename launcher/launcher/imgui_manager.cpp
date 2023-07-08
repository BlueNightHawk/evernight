#ifdef IMGUI

#include "imgui_manager.hpp"

#include <Windows.h>
#include <gl/GL.h>
#include "SDL2/SDL_opengl.h"

//-----------------------------------------------------------------------------
// Initialize ImGui by creating context
//-----------------------------------------------------------------------------
void CImGuiManager::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
}

//-----------------------------------------------------------------------------
// Initialize OpenGL2/SDL2 backends of ImGui
//-----------------------------------------------------------------------------
void CImGuiManager::InitBackends(SDL_Window* window)
{
	ImGui_ImplOpenGL2_Init();
	ImGui_ImplSDL2_InitForOpenGL(window, ImGui::GetCurrentContext());

	// Do other things...
}

//-----------------------------------------------------------------------------
// Draw ImGui elements
//-----------------------------------------------------------------------------
void CImGuiManager::Draw(SDL_Window* window)
{
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();

	// Here you draw...
	// -------------------------

	//ImGui::ShowDemoWindow();
	DrawVersionString();

	// -------------------------

	glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void CImGuiManager::DrawVersionString()
{
	ImGui::SetNextWindowPos(ImVec2(4, 4), ImGuiCond_Always);

	ImGui::SetNextWindowBgAlpha(0.5f);

	ImGui::Begin("Version Info", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
	ImFont font = *ImGui::GetFont();
	font.Scale = 1.2f;

	ImGui::PushFont(&font);
	ImGui::TextColored(ImVec4(0.9f,0.9f,0.9f,1.0f), "Evernight Build : %s\n", __TIMESTAMP__);
	ImGui::PopFont();

	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.8f), "OpenGL %s\n", glGetString(GL_VERSION));

	ImGui::End();
}


//-----------------------------------------------------------------------------
// ImGui process event for SDL2
//-----------------------------------------------------------------------------
int ImGui_ProcessEvent(void* data, SDL_Event* event)
{
	return ImGui_ImplSDL2_ProcessEvent(event);
}

#endif