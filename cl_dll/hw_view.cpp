#include "hud.h"
#include "cvardef.h"
#include "Utils.hpp"

#include "reGS_enginehook.h"

typedef void (*_BuildGammaTable)(float g);
_BuildGammaTable ORIG_BuildGammaTable = NULL;

cvar_t *brightness, *texgamma;
byte texgammatable[256];

/*
int lightgammatable[1024];
int lineargammatable[1024];
int screengammatable[1024];
*/

float BuildGammaForClient(float g)
{
	int inf;
	float g1, g3;
	float original_g = g;

	if (g == 0.0)
		g = 2.5;

	g = 1.0 / g;
	g1 = texgamma->value * g;

	if (brightness->value <= 0.0)
		g3 = 0.125;
	else if (brightness->value > 1.0)
		g3 = 0.05;
	else
		g3 = 0.125 - (brightness->value * brightness->value) * 0.075;

	for (int i = 0; i < 256; i++)
	{
		inf = 255 * pow(i / 255.0, g1);

		if (inf < 0)
			inf = 0;

		if (inf > 255)
			inf = 255;

		texgammatable[i] = inf;
	}

	return original_g;
}

void BuildGammaTable(float g)
{
	ORIG_BuildGammaTable(BuildGammaForClient(g));
}


void V_Hook()
{
	brightness = gEngfuncs.pfnGetCvarPointer("brightness");
	texgamma = gEngfuncs.pfnGetCvarPointer("texgamma");

	Hook(BuildGammaTable);

	BuildGammaForClient(gEngfuncs.pfnGetCvarFloat("gamma"));
}