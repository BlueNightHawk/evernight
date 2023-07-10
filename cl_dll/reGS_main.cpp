#include "Platform.h"
#include "Utils.hpp"
#include "reGS_enginehook.h"

struct funchook;

funchook *g_pHooks = nullptr;

Utils utils = Utils::Utils(NULL, NULL, NULL);

void V_Hook();
void VGuiWrap2_Hook();
void GLDraw_Hook();

extern byte texgammatable[256];

bool HWHook()
{
	void* handle;
	void* base;
	size_t size;

	if (!MemUtils::GetModuleInfo(L"hw.dll", &handle, &base, &size))
		return false;

	g_pHooks = funchook_create();

	utils = Utils::Utils(handle, base, size);

	V_Hook();
	VGuiWrap2_Hook();
	GLDraw_Hook();

	int result = funchook_install(g_pHooks, 0);

	assert(result == FUNCHOOK_ERROR_SUCCESS);

	return true;
}

void HWUnHook()
{
	funchook_uninstall(g_pHooks,0);
	funchook_destroy(g_pHooks);

	memset(texgammatable, 0, 256);

	g_pHooks = nullptr;
}