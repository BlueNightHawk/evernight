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

#define CACHE_ID (('c' << 0) + ('s' << 8) + ('l' << 16) + ('d' << 24))
#define CACHE_VERSION (('r' << 0) + ('m' << 8) + ('d' << 16) + ('l' << 24))

#define MAX_CACHED 4096

static studio_cache_t studio_cache[MAX_CACHED];
static int num_cache;

unsigned int flush_count;

// FNV-1a hash
#define FNV_OFFSET_BASIS32 0x811c9dc5
#define FNV_PRIME32 0x01000193

static uint32 HashData(const void* data, int size)
{
	uint32 hash = FNV_OFFSET_BASIS32;

	for (int i = 0; i < size; i++)
	{
		hash ^= ((const unsigned char*)data)[i];
		hash *= FNV_PRIME32;
	}

	return hash;
}

// mikkotodo make somewhat dynamic?
// mikkotodo make sure that these won't overflow
#define BUILD_NUM_VERTICES (102400)
#define BUILD_NUM_INDICES (BUILD_NUM_VERTICES * 3)

typedef struct
{
	unsigned num_verts;
	unsigned num_indices;
	GLuint indices[BUILD_NUM_INDICES];
} base_build_buffer_t;

typedef struct
{
	base_build_buffer_t base;
	studio_cpu_vert_t verts[BUILD_NUM_VERTICES];
	studio_vertbone_t vertbones[BUILD_NUM_VERTICES];
} cpu_build_buffer_t;

typedef struct
{
	base_build_buffer_t base;
	studio_gpu_vert_t verts[BUILD_NUM_VERTICES];
} gpu_build_buffer_t;

static void ParseTricmds(base_build_buffer_t* build, short* tricmds, Vector* vertices, Vector* normals, byte* vertinfo, byte* norminfo, float s, float t)
{
	while (1)
	{
		int value = *tricmds++;
		if (!value)
			break;

		bool trifan = false;

		if (value < 0)
		{
			trifan = true;
			value = -value;
		}

		unsigned count = (unsigned)value;

		unsigned offset = build->num_verts;
		build->num_verts += count;

		if (studio_gpuskin)
		{
			gpu_build_buffer_t* gpu_build = (gpu_build_buffer_t*)build;
			studio_gpu_vert_t* vert = &gpu_build->verts[offset];

			for (unsigned l = 0; l < count; l++)
			{
				for (unsigned m = 0; m < 3; m++)
				{
					vert->pos[m] = vertices[tricmds[0]][m];
					vert->norm[m] = normals[tricmds[1]][m];
				}

				vert->texcoord[0] = s * tricmds[2];
				vert->texcoord[1] = t * tricmds[3];

				vert->bones[0] = vertinfo[tricmds[0]];
				vert->bones[1] = norminfo[tricmds[1]];

				tricmds += 4;
				vert++;
			}
		}
		else
		{
			cpu_build_buffer_t* cpu_build = (cpu_build_buffer_t*)build;
			studio_cpu_vert_t* vert = &cpu_build->verts[offset];
			studio_vertbone_t* vertbone = &cpu_build->vertbones[offset];

			for (unsigned l = 0; l < count; l++)
			{
				for (unsigned m = 0; m < 3; m++)
				{
					vert->pos[m] = vertices[tricmds[0]][m];
					vert->norm[m] = normals[tricmds[1]][m];
				}

				vert->texcoord[0] = s * tricmds[2];
				vert->texcoord[1] = t * tricmds[3];

				vertbone->bones[0] = vertinfo[tricmds[0]];
				vertbone->bones[1] = norminfo[tricmds[1]];

				tricmds += 4;
				vert++;
				vertbone++;
			}
		}

		if (trifan)
		{
			for (unsigned i = 2; i < count; i++)
			{
				build->indices[build->num_indices++] = offset;
				build->indices[build->num_indices++] = offset + i - 1;
				build->indices[build->num_indices++] = offset + i;
			}
		}
		else
		{
			for (unsigned i = 2; i < count; i++)
			{
				if (!(i % 2))
				{
					build->indices[build->num_indices++] = offset + i - 2;
					build->indices[build->num_indices++] = offset + i - 1;
					build->indices[build->num_indices++] = offset + i;
				}
				else
				{
					build->indices[build->num_indices++] = offset + i - 1;
					build->indices[build->num_indices++] = offset + i - 2;
					build->indices[build->num_indices++] = offset + i;
				}
			}
		}
	}
}

static void BuildStudioVBO(studio_cache_t* cache, model_t* model, studiohdr_t* header)
{
	static base_build_buffer_t* build = NULL;

	if (!build)
	{
		if (studio_gpuskin)
			build = (base_build_buffer_t*)Mem_Alloc(sizeof(gpu_build_buffer_t));
		else
			build = (base_build_buffer_t*)Mem_Alloc(sizeof(cpu_build_buffer_t));
	}

	build->num_verts = 0;
	build->num_indices = 0;

	mstudiobodyparts_t* bodyparts = (mstudiobodyparts_t*)((byte*)header + header->bodypartindex);

	studiohdr_t* textureheader = R_LoadTextures(model, header);
	short* skins = (short*)((byte*)textureheader + textureheader->skinindex);
	mstudiotexture_t* textures = (mstudiotexture_t*)((byte*)textureheader + textureheader->textureindex);

	cache->bodyparts = (mem_bodypart_t*)Mem_Alloc(sizeof(*cache->bodyparts) * header->numbodyparts);

	for (int i = 0; i < header->numbodyparts; i++)
	{
		mstudiobodyparts_t* bodypart = &bodyparts[i];
		mstudiomodel_t* models = (mstudiomodel_t*)((byte*)header + bodypart->modelindex);

		mem_bodypart_t* mem_bodypart = &cache->bodyparts[i];
		mem_bodypart->models = (mem_model_t*)Mem_Alloc(sizeof(*mem_bodypart->models) * bodypart->nummodels);

		for (int j = 0; j < bodypart->nummodels; j++)
		{
			mstudiomodel_t* submodel = &models[j];
			mstudiomesh_t* meshes = (mstudiomesh_t*)((byte*)header + submodel->meshindex);

			Vector* vertices = (Vector*)((byte*)header + submodel->vertindex);
			Vector* normals = (Vector*)((byte*)header + submodel->normindex);

			byte* vertinfo = (byte*)((byte*)header + submodel->vertinfoindex);
			byte* norminfo = (byte*)((byte*)header + submodel->norminfoindex);

			mem_model_t* mem_model = &mem_bodypart->models[j];
			mem_model->meshes = (mem_mesh_t*)Mem_Alloc(sizeof(*mem_model->meshes) * submodel->nummesh);

			// only for cpu skinning
			unsigned vert_offset_model = build->num_verts;

			for (int k = 0; k < submodel->nummesh; k++)
			{
				mstudiomesh_t* mesh = &meshes[k];
				short* tricmds = (short*)((byte*)header + mesh->triindex);

				float s = 1.0f / (float)textures[skins[mesh->skinref]].width;
				float t = 1.0f / (float)textures[skins[mesh->skinref]].height;

				unsigned index_offset = build->num_indices;

				// only for cpu skinning
				unsigned vert_offset_mesh = build->num_verts;

				ParseTricmds(build, tricmds, vertices, normals, vertinfo, norminfo, s, t);

				mem_mesh_t* mem_mesh = &mem_model->meshes[k];
				mem_mesh->ofs_indices = index_offset * sizeof(*build->indices);
				mem_mesh->num_indices = build->num_indices - index_offset;

				// only for cpu skinning
				mem_mesh->ofs_verts = vert_offset_mesh;
				mem_mesh->num_verts = build->num_verts - vert_offset_mesh;
			}

			// only for cpu skinning
			mem_model->ofs_verts = vert_offset_model;
			mem_model->num_verts = build->num_verts - vert_offset_model;
		}
	}

	glGenBuffers(1, &cache->studio_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cache->studio_vbo);

	if (studio_gpuskin)
	{
		gpu_build_buffer_t* cpu_build = (gpu_build_buffer_t*)build;
		glBufferData(GL_ARRAY_BUFFER, build->num_verts * sizeof(studio_gpu_vert_t), cpu_build->verts, GL_STATIC_DRAW);
	}
	else
	{
		glBufferData(GL_ARRAY_BUFFER, build->num_verts * sizeof(studio_cpu_vert_t), NULL, GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &cache->studio_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cache->studio_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*build->indices) * build->num_indices, build->indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (!studio_gpuskin)
	{
		cpu_build_buffer_t* cpu_build = (cpu_build_buffer_t*)build;

		cache->verts = (studio_cpu_vert_t*)Mem_Alloc(sizeof(studio_cpu_vert_t) * build->num_verts);
		memcpy(cache->verts, cpu_build->verts, sizeof(studio_cpu_vert_t) * build->num_verts);

		cache->vertbones = (studio_vertbone_t*)Mem_Alloc(sizeof(studio_vertbone_t) * build->num_verts);
		memcpy(cache->vertbones, cpu_build->vertbones, sizeof(studio_vertbone_t) * build->num_verts);
	}
}

static void ParseTextures(studio_cache_t* cache, keyValue_t* key, bool flush)
{
	if (!studio_fastpath)
		return;

	for (int i = 0; i < key->numSubkeys; i++)
	{
		bool found = false;
		keyValue_t* subkey = &key->subkeys[i];

		for (int j = 0; j < cache->numtextures; j++)
		{
			mem_texture_t* texture = &cache->textures[j];

			if (!strcmp(subkey->name, texture->name))
			{
				StudioLoadTexture(subkey, texture, flush);
				found = true;
				break;
			}
		}

		if (!found)
		{
			gEngfuncs.Con_Printf("Texture %s not overriding an existing one in %s, ignoring\n",
				subkey->name, cache->config_path);
		}
	}
}

static void ParseConfig(studio_cache_t* cache, bool flush)
{
	char* text = (char*)gEngfuncs.COM_LoadFile(cache->config_path, 5, NULL);
	if (!text)
		return;

	keyValue_t root;
	if (!KeyValueParse(&root, text))
	{
		gEngfuncs.Con_Printf("Could not parse file %s\n", text);
		KeyValueFree(&root);
		gEngfuncs.COM_FreeFile(text);
		return;
	}

	for (int i = 0; i < root.numSubkeys; i++)
	{
		keyValue_t* subkey = &root.subkeys[i];

		if (!strcmp(subkey->name, "textures"))
		{
			ParseTextures(cache, subkey, flush);
		}
		else
		{
			gEngfuncs.Con_Printf("Unrecognized option %s in %s\n", subkey->name, cache->config_path);
		}
	}

	KeyValueFree(&root);
	gEngfuncs.COM_FreeFile(text);
}

static void SetupTextures(studio_cache_t* cache, model_t* model, studiohdr_t* header)
{
	studiohdr_t* textureheader = R_LoadTextures(model, header);
	mstudiotexture_t* textures = (mstudiotexture_t*)((byte*)textureheader + textureheader->textureindex);

	// we need to store numtextures, texture names and config path in the cache so we
	// can reload textures even when we don't have access to a model_t or studiohdr_t

	cache->textures = (mem_texture_t*)Mem_Alloc(sizeof(mem_texture_t) * textureheader->numtextures);
	memset(cache->textures, 0, sizeof(mem_texture_t) * textureheader->numtextures);
	cache->numtextures = textureheader->numtextures;

	// set texture names
	for (int i = 0; i < textureheader->numtextures; i++)
	{
		mstudiotexture_t* texture = &textures[i];
		mem_texture_t* mem_texture = &cache->textures[i];
		strcpy(mem_texture->name, texture->name); // will fit
	}
}

// this is ok for us because we know the buffer sizes
static void CopyWithoutExtension(char* dst, const char* src)
{
	while (*src && *src != '.')
		*dst++ = *src++;
	*dst = '\0';
}

static void SetConfigPath(studio_cache_t* cache, model_t* model)
{
	char modelname[64];
	CopyWithoutExtension(modelname, model->name);	  // will fit
	sprintf(cache->config_path, "%s.txt", modelname); // will fit
}

static void BuildStudioCache(studio_cache_t* cache, model_t* model, studiohdr_t* header)
{
	cache->hash = HashData(header, sizeof(*header));

	if (studio_fastpath)
	{
		// external textures only available with fastpath
		SetupTextures(cache, model, header);
	}

	SetConfigPath(cache, model);
	ParseConfig(cache, false);

	if (studio_fastpath)
	{
		// vbos only used with fastpath (duh)
		BuildStudioVBO(cache, model, header);
	}
}

studio_cache_t* GetStudioCache(model_t* model, studiohdr_t* header)
{
	// see if the cache pointer is in the header
	if (header->id == CACHE_ID && header->version == CACHE_VERSION)
		return &studio_cache[header->length];

	// see if this model is cached even
	uint32 hash = HashData(header, sizeof(*header));

	// mikkotodo revisit? slow but probably doesn't matter
	for (int i = 0; i < num_cache; i++)
	{
		if (studio_cache[i].hash == hash)
		{
			// ok, update header
			header->id = CACHE_ID;
			header->version = CACHE_VERSION;
			header->length = i;
			return &studio_cache[i];
		}
	}

	if (num_cache >= MAX_CACHED)
	{
		gEngfuncs.Con_DPrintf("Studio model cache full");
		return NULL; // not reached
	}

	// cache the model
	int index = num_cache++;
	studio_cache_t* cache = &studio_cache[index];

	BuildStudioCache(cache, model, header);

	// update header
	header->id = CACHE_ID;
	header->version = CACHE_VERSION;
	header->length = index;

	return cache;
}

// mikkotodo might be necessary again if we need extremely expensive tangent calc
void UpdateStudioCaches(void)
{
	static model_t* last_world = NULL;

	model_t* world = gEngfuncs.hudGetModelByIndex(1);
	if (world == last_world)
		return;

	last_world = world;
	if (!world)
		return;

	// some crackhead engines might have a greater model limit
	// so only break when GetModelByIndex returns null
	for (int i = 1;; i++)
	{
		model_t* model = IEngineStudio.GetModelByIndex(i);
		if (!model)
			break;

		if (model->type != mod_studio)
			continue;

		studiohdr_t* header = (studiohdr_t*)IEngineStudio.Mod_Extradata(model);
		(void)GetStudioCache(model, header);
	}
}

void StudioCacheStats(int* count, int* max)
{
	*count = num_cache;
	*max = MAX_CACHED;
}

void StudioConfigFlush_f(void)
{
	flush_count++;

	for (int i = 0; i < num_cache; i++)
	{
		studio_cache_t* cache = &studio_cache[i];
		ParseConfig(cache, true);
	}
}

// not an ideal place for this but ok
studiohdr_t* R_LoadTextures(model_t* model, studiohdr_t* header)
{
	assert(model);

	if (header->textureindex)
		return header;

	model_t* texmodel = (model_t*)model->texinfo;
	if (texmodel && IEngineStudio.Cache_Check(&texmodel->cache))
		return (studiohdr_t*)texmodel->cache.data;

	char path[128];
	strcpy(path, model->name);

	// unsafe but the engine does it too...
	// also lower case for linux??? what the fuck
	strcpy(path + strlen(path) - 4, "t.mdl");

	texmodel = IEngineStudio.Mod_ForName(path, true);
	model->texinfo = (mtexinfo_t*)texmodel;

	// not sure why this is done but ok
	studiohdr_t* textureheader = (studiohdr_t*)texmodel->cache.data;
	strcpy(textureheader->name, path);
	return textureheader;
}