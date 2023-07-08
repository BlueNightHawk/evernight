//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include <assert.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "Exports.h"

//
// Override the StudioModelRender virtual member functions here to implement custom bone
// setup, blending, etc.
//

// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;

// The renderer object, created on the stack.
CGameStudioModelRenderer g_StudioRenderer;
/*
====================
CGameStudioModelRenderer

====================
*/
CGameStudioModelRenderer::CGameStudioModelRenderer()
{
}

////////////////////////////////////
// Hooks to class implementation
////////////////////////////////////

/*
====================
R_StudioDrawPlayer

====================
*/
int R_StudioDrawPlayer(int flags, entity_state_t* pplayer)
{
	return static_cast<int>(g_StudioRenderer.StudioDrawPlayer(flags, pplayer));
}

/*
====================
R_StudioDrawModel

====================
*/
int R_StudioDrawModel(int flags)
{
	return static_cast<int>(g_StudioRenderer.StudioDrawModel(flags));
}

/*
====================
R_StudioInit

====================
*/
void R_StudioInit()
{
	g_StudioRenderer.Init();
}

/*
====================
R_CacheShadows

====================
*/

void R_CacheShadows()
{
	g_StudioRenderer.m_bCacheShadows = true;
	//	g_StudioRenderer.StudioCacheShadows();
}

// The simple drawing interface we'll pass back to the engine
r_studio_interface_t studio =
	{
		STUDIO_INTERFACE_VERSION,
		R_StudioDrawModel,
		R_StudioDrawPlayer,
};

r_studio_interface_t studio_h =
	{
		STUDIO_INTERFACE_VERSION,
		R_StudioDrawModel,
		R_StudioDrawPlayer,
};


int Hk_GetStudioModelInterface(int version,
	r_studio_interface_t** ppinterface,
	engine_studio_api_t* pstudio);
	/*
====================
HUD_GetStudioModelInterface

Export this function for the engine to use the studio renderer class to render objects.
====================
*/
int DLLEXPORT HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	//	RecClStudioInterface(version, ppinterface, pstudio);

	if (version != STUDIO_INTERFACE_VERSION)
		return 0;

	// Point the engine to our callbacks
	*ppinterface = &studio_h;

	memcpy(&IEngineStudio, pstudio, sizeof(engine_studio_api_t));

	Hk_GetStudioModelInterface(version, ppinterface, &IEngineStudio);

	// Initialize local variables, etc.
	
	R_StudioInit();

	// Success
	return 1;
}
