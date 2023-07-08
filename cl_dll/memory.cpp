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

#define BLOCK_SIZE (128 << 20)

static byte* mem_block;
static size_t mem_used;

void Mem_Init(void)
{
	mem_block = (byte*)malloc(BLOCK_SIZE);
	if (!mem_block)
		gEngfuncs.Con_DPrintf("Memory allocation failed\n");
}

void Mem_Shutdown(void)
{
	free(mem_block);
}

void* Mem_Alloc(size_t size)
{
	/* special case */
	if (!size)
		return mem_block;

	size = (size + 15) & ~15;

	if (mem_used + size > BLOCK_SIZE)
	{
		gEngfuncs.Con_DPrintf("Out of memory\n");
		return NULL; /* never reached */
	}

	void* ptr = &mem_block[mem_used];
	mem_used += size;
	return ptr;
}

char* Mem_Strdup(const char* s)
{
	size_t n = strlen(s) + 1;
	char* ptr = (char*)Mem_Alloc(n);
	memcpy(ptr, s, n);
	return ptr;
}

void* Mem_AllocTemp(size_t size)
{
	/* special case */
	if (!size)
		return mem_block;

	size = (size + 15) & ~15;

	if (mem_used + size > BLOCK_SIZE)
	{
		gEngfuncs.Con_DPrintf("Out of memory\n");
		return NULL; /* never reached */
	}

	return &mem_block[mem_used];
}

void Mem_GetInfo(size_t* used, size_t* size)
{
	*used = mem_used;
	*size = BLOCK_SIZE;
}