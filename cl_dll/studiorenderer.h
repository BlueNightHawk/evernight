#pragma once

#include "mathlib.h"
#include "shader.h"
#include "glad/glad.h"
#include "memory.h"
#include <algorithm>
#include "keyvalue.h"
#include "gamma.h"

inline static float Degrees(float rad) { return (rad * (float)(180.0 / M_PI)); }
inline static float Radians(float deg) { return (deg * (float)(M_PI / 180.0)); }

extern bool studio_gpuskin;
extern bool studio_fastpath;

extern Vector v_vieworg;
extern Vector v_viewforward, v_viewright, v_viewup;

typedef float mat3x4_t[3][4];

typedef struct
{
	unsigned ofs_indices;
	unsigned num_indices;

	// only for cpu skinning
	unsigned ofs_verts;
	unsigned num_verts;
} mem_mesh_t;

typedef struct mem_model_s
{
	mem_mesh_t* meshes;

	// only for cpu skinning
	unsigned ofs_verts;
	unsigned num_verts;
} mem_model_t;

typedef struct
{
	mem_model_t* models;
} mem_bodypart_t;

typedef struct
{
	float pos[3];
	float norm[3];
	float texcoord[2];
} studio_cpu_vert_t;

typedef struct
{
	float pos[3];
	float norm[3];
	float texcoord[2];
	float bones[2];
} studio_gpu_vert_t;

// only for cpu skinning
typedef struct
{
	byte bones[2];
} studio_vertbone_t;

typedef struct
{
	char name[64];
	GLuint diffuse;
} mem_texture_t;

typedef struct studio_cache_s
{
	uint32 hash;

	// name of the configuration file
	char config_path[256];

	// renderer specfic stuff
	mem_texture_t* textures;
	int numtextures;

	mem_bodypart_t* bodyparts;

	GLuint studio_vbo;
	GLuint studio_ebo;

	// only for cpu skinning
	studio_cpu_vert_t* verts;
	studio_vertbone_t* vertbones;
} studio_cache_t;

extern unsigned int flush_count;

studio_cache_t* GetStudioCache(model_t* model, studiohdr_t* header);

// mikkotodo might be necessary again if we need extremely expensive tangent calc
void UpdateStudioCaches(void);

void StudioCacheStats(int* count, int* max);
void StudioConfigFlush_f(void);

studiohdr_t* R_LoadTextures(model_t* model, studiohdr_t* header);

typedef struct
{
	cl_entity_t* entity;
	studiohdr_t* header;
	model_t* model;
	studio_cache_t* cache;

	mat3x4_t (*bonetransform)[];

	mstudiomodel_t* submodel;
	mem_model_t* mem_submodel;

	float ambientlight;
	float shadelight;
	Vector lightcolor;
	Vector lightvec;
} studio_context_t;

extern engine_studio_api_s IEngineStudio;

extern int studio_drawcount;

void MK_StudioInit(void);

void R_StudioInitContext(studio_context_t* ctx, cl_entity_t* entity, model_t* model, studiohdr_t* header);
void R_StudioEntityLight(studio_context_t* ctx);
void R_StudioSetupLighting(studio_context_t* ctx, alight_t* lighting);
void R_StudioSetupRenderer(studio_context_t* ctx);
void R_StudioRestoreRenderer(studio_context_t* ctx);

void R_StudioSetupModel(studio_context_t* ctx, int bodypart_index);
void R_StudioDrawPoints(studio_context_t* ctx);

void Hk_StudioSetupModel(int bodypart, void** ppbodypart, void** ppsubmodel);
void HookEngineStudio(engine_studio_api_t* studio);


void My_StudioSetupModel(int bodypart, void** ppbodypart, void** ppsubmodel);
int Hk_GetStudioModelInterface(int version, r_studio_interface_t** ppinterface, engine_studio_api_t* pstudio);

void StudioLoadTexture(keyValue_t* kv, mem_texture_t* dest, bool flush);

byte* TgaLoad(char* path, int* pwidth, int* pheight, int* pcomp);
const char* TgaLoadError(void);