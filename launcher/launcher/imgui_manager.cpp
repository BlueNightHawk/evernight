#ifdef IMGUI

#include "imgui_manager.hpp"

#include <stdio.h>
#include <Windows.h>
#include <gl/GL.h>
#include "SDL2/SDL.h"

#include "interface.h"


typedef void(*ClientDraw)();

ClientDraw pClientDraw = nullptr;
CSysModule* pClient = nullptr;

int SCR_DrawFPS(int height);

extern char com_gamedir[256];

ImFont* pUbuntuFont = nullptr;

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

	char font[256];
	sprintf(font, "./%s/resource/ubuntu.ttf", com_gamedir);

	pUbuntuFont = io.Fonts->AddFontFromFileTTF(font, 24);

	if (pUbuntuFont)
		io.FontGlobalScale = 0.7f;
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
	if (!pClientDraw && !pClient) 
	{
		pClient = (CSysModule*)GetModuleHandle("client.dll");
		if (pClient)
		{
			pClientDraw = (ClientDraw)Sys_GetProcAddress(pClient, "UpdateClientImgui");
		}
	}

	if (pClientDraw)
		pClientDraw();

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
	font.Scale = 1.6f;

	ImGui::PushFont(&font);
	ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.0f, 0.8f), "%4i FPS\n", SCR_DrawFPS(0));
	ImGui::PopFont();

	font.Scale = 1.2f;
	ImGui::PushFont(&font);
	ImGui::TextColored(ImVec4(0.9f,0.9f,0.9f,1.0f), "Evernight Build : %s\n", __TIMESTAMP__);
	ImGui::PopFont();

	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 0.8f), "OpenGL %s\n", glGetString(GL_VERSION));



	ImGui::End();
}

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime(void)
{
	static LARGE_INTEGER g_PerformanceFrequency;
	static LARGE_INTEGER g_ClockStart;
	LARGE_INTEGER CurrentTime;

	if (!g_PerformanceFrequency.QuadPart)
	{
		QueryPerformanceFrequency(&g_PerformanceFrequency);
		QueryPerformanceCounter(&g_ClockStart);
	}
	QueryPerformanceCounter(&CurrentTime);

	return (double)(CurrentTime.QuadPart - g_ClockStart.QuadPart) / (double)(g_PerformanceFrequency.QuadPart);
}


/*
==============
SCR_DrawFPS
==============
*/
int SCR_DrawFPS(int height)
{
	double calc;
	double newtime;
	static double nexttime = 0, lasttime = 0;
	static double framerate = 0;
	static int framecount = 0;

	newtime = Sys_DoubleTime();
	if (newtime >= nexttime)
	{
		framerate = framecount / (newtime - lasttime);
		lasttime = newtime;
		nexttime = max(nexttime + 1.0, lasttime - 1.0);
		framecount = 0;
	}

	calc = framerate;
	framecount++;

	return (int)(calc + 0.5f);
}


void CImGuiManager::Shutdown()
{
	pClient = nullptr;
	pClientDraw = nullptr;

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

#endif