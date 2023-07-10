#include "hud.h"
#include "imgui_manager.h"
#include "SDL.h"
#include <stdio.h>

CImGui g_ImGui;
ImFont* pUbuntuFont = nullptr;

static bool g_bShuttingDown = false;

int ImGui_ProcessEvent(void* data, SDL_Event* event);

extern "C" void __declspec(dllexport) UpdateClientImgui();

// Called by hooked SDL_GL_SwapWindow in hl.exe
void __declspec(dllexport) UpdateClientImgui()
{
	g_ImGui.Update();
}

void CImGui::Init()
{
	g_bShuttingDown = false;

	IMGUI_CHECKVERSION();
	ImGui::SetCurrentContext(ImGui::CreateContext());
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	char font[256];
	sprintf(font, "./%s/resource/ubuntu.ttf", gEngfuncs.pfnGetGameDirectory());

	pUbuntuFont = io.Fonts->AddFontFromFileTTF(font, 24);

	if (pUbuntuFont)
		io.FontGlobalScale = 0.7f;

	ImGui_ImplOpenGL2_Init();
	ImGui_ImplSDL2_InitForOpenGL(SDL_GetWindowFromID(1), nullptr);

	SDL_AddEventWatch(ImGui_ProcessEvent, NULL);
}

void CImGui::Update()
{
	if (g_bShuttingDown)
		return;

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void CImGui::Shutdown()
{
	g_bShuttingDown = true;

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext();
	SDL_DelEventWatch(ImGui_ProcessEvent, NULL);
}

//-----------------------------------------------------------------------------
// ImGui process event for SDL2
//-----------------------------------------------------------------------------
int ImGui_ProcessEvent(void* data, SDL_Event* event)
{
	return ImGui_ImplSDL2_ProcessEvent(event);
}
