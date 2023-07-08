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

#include "studiorenderer.h"

engine_studio_api_t _IEngineStudio;

extern r_studio_interface_s studio;

#define Z_NEAR 1.0f /* was 4 but that's too far for viewmodels */
#define Z_FAR 4096.0f

static int Hk_StudioDrawModel(int flags)
{
	studio_drawcount++;
	return studio.StudioDrawModel(flags);
}

static int Hk_StudioDrawPlayer(int flags, entity_state_t* player)
{
	studio_drawcount++;
	return studio.StudioDrawPlayer(flags, player);
}

static bool UnflipKnife(float* value)
{
#if 0

	if (currentWeaponId != WEAPON_KNIFE || cl_mirror_knife->value)
		return false;

	*value = cl_righthand->value;
	cl_righthand->value = (float)!cl_righthand->value;
#endif
	return true;
}

static void ReflipKnife(float value)
{
#if 0
	cl_righthand->value = value;
#endif
}

static void DrawHands(cl_entity_t* weapon, int flags)
{
#if 0
	bool changed;
	model_t* model;
	cl_entity_t backup;
	static char hands_path[256] = "";
	static bool hands_valid = false;

	if (!*viewmodel_hands->string)
		return;

	/* skip "models/" for comparison */
	changed = strncmp(hands_path + 7, viewmodel_hands->string, sizeof(hands_path) - 7) != 0;
	if (changed)
	{
		/* update path */
		sprintf(hands_path, "models/%s", viewmodel_hands->string);
	}
	else if (!hands_valid)
	{
		/* no change and the last hands were invalid, don't even try */
		return;
	}

	/* mikkotodo: even though Mod_ForName returns null, it doesn't actually
	free the model so we'd need to do that manually... */
	model = _IEngineStudio.Mod_ForName(hands_path, false);
	hands_valid = model != NULL;
	if (!hands_valid)
		return;

	/* should probably see what changes and only backup the necessary */
	backup = *weapon;

	/* abuse existing bonemerge functionality */
	weapon->model = model;
	weapon->curstate.movetype = 12; /* MOVETYPE_FOLLOW */

	Hk_StudioDrawModel(flags);

	*weapon = backup;
#endif
}

static void SetProjectionMatrix(void)
{
	SCREENINFO scr;
	float aspect, fov, f, fn;
	float matrix[4][4];

	scr.iSize = sizeof(SCREENINFO);
	gEngfuncs.pfnGetScreenInfo(&scr);
	aspect = (float)scr.iWidth / (float)scr.iHeight;

	fov = 90;
	fov = std::clamp(fov, 1.0f, 179.0f);
	fov = 0.75f * tanf(Radians(fov) * 0.5f);

	f = 1.0f / fov;
	fn = (1.0f / (Z_NEAR - Z_FAR));

	memset(matrix, 0, sizeof(matrix));
	matrix[0][0] = f / aspect;
	matrix[1][1] = f;
	matrix[2][2] = (Z_NEAR + Z_FAR) * fn;
	matrix[2][3] = -1;
	matrix[3][2] = 2 * Z_NEAR * Z_FAR * fn;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(&matrix[0][0]);
	glMatrixMode(GL_MODELVIEW);
}

static void RestoreProjectionMatrix(void)
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

static int My_StudioDrawModel(int flags)
{
	int result;
	bool unflip_knife;
	float old_righthand;

	cl_entity_t* entity = _IEngineStudio.GetCurrentEntity();

	if (entity != _IEngineStudio.GetViewEntity())
		return Hk_StudioDrawModel(flags);

	SetProjectionMatrix();

	/* think about inspecting now since we're about to draw the vm */
	//InspectThink();

	//unflip_knife = UnflipKnife(&old_righthand);

	result = Hk_StudioDrawModel(flags);

	/* draw hands now that the scene is properly set up */
	//DrawHands(entity, flags);

	//if (unflip_knife)
	//	ReflipKnife(old_righthand);

	RestoreProjectionMatrix();

	return result;
}

void My_StudioSetupModel(int bodypart, void** ppbodypart, void** ppsubmodel)
{
	cl_entity_t* entity;
	bool viewModel;
	studiohdr_t* studiohdr;
	mstudiobodyparts_t* body;
	int oldbody;
	int current;
	int newbody;

	entity = _IEngineStudio.GetCurrentEntity();
	viewModel = (entity == _IEngineStudio.GetViewEntity());
	if (!viewModel) /* we don't want to do anything */
	{
		Hk_StudioSetupModel(bodypart, ppbodypart, ppsubmodel);
		return;
	}

	/* check if bodypart is right */
	studiohdr = (studiohdr_t*)entity->model->cache.data;
	if (!studiohdr)
	{
		Hk_StudioSetupModel(bodypart, ppbodypart, ppsubmodel);
		return;
	}

	body = (mstudiobodyparts_t*)((byte*)studiohdr + studiohdr->bodypartindex) + bodypart;

	/* check if the names matches and that there's enough submodels */
	if (*(uint32*)body->name != *(uint32*)"arms" || body->nummodels < 2)
	{
		Hk_StudioSetupModel(bodypart, ppbodypart, ppsubmodel);
		return;
	}

	/* bodypart is right one, do arm changing stuff */
	oldbody = entity->curstate.body;

#if 0
	if (user1 == 4)
		newbody = playerInfo[user2].team;
	else
		newbody = localTeam;
#endif
	current = (entity->curstate.body / body->base) % body->nummodels;
	entity->curstate.body = (entity->curstate.body - (current * body->base) + (newbody * body->base));

	/* set the arm bodygroup */
	Hk_StudioSetupModel(bodypart, ppbodypart, ppsubmodel);

	/* restore state */
	entity->curstate.body = oldbody;
}

int Hk_GetStudioModelInterface(int version,
	r_studio_interface_t** ppinterface,
	engine_studio_api_t* pstudio)
{
	int result = 1;

	/* install hooks for fast path rendering */
	memcpy(&_IEngineStudio, pstudio, sizeof(engine_studio_api_t));
	HookEngineStudio(pstudio);

	(*ppinterface)->StudioDrawModel = My_StudioDrawModel;
	(*ppinterface)->StudioDrawPlayer = Hk_StudioDrawPlayer;

	/* no one uses software or d3d renderers */
	if (pstudio->IsHardware() != 1)
		gEngfuncs.Con_Printf("Software and D3D modes are not supported\n");

	return result;
}