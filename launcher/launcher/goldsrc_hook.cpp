#include "goldsrc_hook.h"

#include <stdio.h>
#include <Windows.h>
#include "funchook.h"

#include "imgui_manager.hpp"

#include <gl/GL.h>

CImGuiManager imgui;

_SDL_CreateWindow ORIG_SDL_CreateWindow = NULL;
#ifdef IMGUI
_SDL_GL_SwapWindow ORIG_SDL_GL_SwapWindow = NULL;
#endif
SDL_Window* goldsrcWindow;

funchook_t* g_pSdlHook;

//-----------------------------------------------------------------------------
// By hooking SDL_CreateWindow we set attributes and init ImGUI backends
// Result: goldsrcWindow (original SDL_Window* from engine)
//-----------------------------------------------------------------------------
SDL_Window* HOOKED_SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags)
{
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	goldsrcWindow = ORIG_SDL_CreateWindow(title, x, y, w, h, flags);

	// Cycle through opengl version
	if (!goldsrcWindow)
	{
		for (int i = 4; i > 0; i--)
		{
			switch (i)
			{
			case 4:
				{
					for (int j = 6; j >= 0; j--)
					{
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, i);
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, j);
						goldsrcWindow = ORIG_SDL_CreateWindow(title, x, y, w, h, flags);
						if (goldsrcWindow)
							break;
					}
				}
				break;
			case 3:
				{
					for (int j = 3; j >= 0; j--)
					{
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, i);
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, j);
						goldsrcWindow = ORIG_SDL_CreateWindow(title, x, y, w, h, flags);
						if (goldsrcWindow)
							break;
					}
				}
				break;
			case 2:
				{
					for (int j = 1; j >= 0; j--)
					{
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, i);
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, j);
						goldsrcWindow = ORIG_SDL_CreateWindow(title, x, y, w, h, flags);
						if (goldsrcWindow)
							break;
					}
				}
				break;
			case 1:
				{
					for (int j = 5; j >= 0; j--)
					{
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, i);
						SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, j);
						goldsrcWindow = ORIG_SDL_CreateWindow(title, x, y, w, h, flags);
						if (goldsrcWindow)
							break;
					}
				}
				break;
			}
			if (goldsrcWindow)
				break;
		}

		if (!goldsrcWindow)
		{
			MessageBox(NULL, "How???", "GPU does not support OpenGL", MB_ICONERROR);
			exit(0);
			return nullptr;
		}
	}

	if (!goldsrcWindow)
	{
		// Try 16 bit color depth.
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 3);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 3);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 3);
		goldsrcWindow = SDL_CreateWindow(title, 0, 0, 640, 480, flags);

		if (!goldsrcWindow)
		{
			MessageBox(NULL, "Failed to create SDL Window", "Fatal Error", MB_ICONERROR);
			exit(0);
			return nullptr;
		}
	}

#ifdef IMGUI	
	imgui.InitBackends(goldsrcWindow);
#endif

	return goldsrcWindow;
}

#ifdef IMGUI
//-----------------------------------------------------------------------------
// By hooking SDL_GL_SwapWindow we can render ImGui
//-----------------------------------------------------------------------------
void HOOKED_SDL_GL_SwapWindow(SDL_Window* window)
{
	imgui.Draw(window);
	ORIG_SDL_GL_SwapWindow(window);
}
#endif
//-----------------------------------------------------------------------------
// Hook SDL2.dll
//-----------------------------------------------------------------------------
void HookSDL2()
{
	// Get SDL functions
	HMODULE hSdl2 = GetModuleHandle("SDL2.dll");
	assert(hSdl2 != 0);

	ORIG_SDL_CreateWindow = (_SDL_CreateWindow)GetProcAddress(hSdl2, "SDL_CreateWindow");

	g_pSdlHook = funchook_create();

#ifdef IMGUI	
	ORIG_SDL_GL_SwapWindow = (_SDL_GL_SwapWindow)GetProcAddress(hSdl2, "SDL_GL_SwapWindow");
#endif

	if (ORIG_SDL_CreateWindow)
		printf("[SDL2.dll] Got SDL_CreateWindow! Setting up stencil buffer...\n");
	else
		printf("[SDL2.dll] Can't get SDL_CreateWindow! There will be no stencil buffer.\n");

#ifdef IMGUI
	if (ORIG_SDL_GL_SwapWindow)
		printf("[SDL2.dll] Got SDL_GL_SwapWindow! Now you can use ImGUI...\n");
	else
		printf("[SDL2.dll] Can't get SDL_GL_SwapWindow! There will be no ImGUI.\n");
#endif

	if (ORIG_SDL_CreateWindow)
	{
		funchook_prepare(g_pSdlHook, (void**)&ORIG_SDL_CreateWindow, (void*)HOOKED_SDL_CreateWindow);
	}

#ifdef IMGUI
	if (ORIG_SDL_GL_SwapWindow)
	{
		imgui.Init();

		funchook_prepare(g_pSdlHook, (void**)&ORIG_SDL_GL_SwapWindow, (void*)HOOKED_SDL_GL_SwapWindow);

		SDL_AddEventWatch(ImGui_ProcessEvent, NULL);
	}
#endif

	funchook_install(g_pSdlHook,0);
}

void ImGui_Shutdown()
{
	imgui.Shutdown();
}

//-----------------------------------------------------------------------------
// Hook hw.dll
//-----------------------------------------------------------------------------
void HookEngine()
{
#if 0
	void* handle;
	void* base;
	size_t size;

	if (!MemUtils::GetModuleInfo(L"hw.dll", &handle, &base, &size))
	{
		printf("[hl.exe] Can't get module info about hw.dll! Stopping hooking...\n");
		return;
	}

	Utils utils = Utils::Utils(handle, base, size);
	printf("[hl.exe] Hooked hw.dll!\n");
#endif	
}
