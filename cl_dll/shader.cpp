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

static GLuint CompileShader(const char* name, const char* source, int length, GLenum type)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, &length);
	glCompileShader(shader);

	GLint is_compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

	if (!is_compiled)
	{
		char message[1024];
		glGetShaderInfoLog(shader, sizeof(message), NULL, message);
		gEngfuncs.Con_DPrintf("Compiling %s shader failed:\n%s", name, message);
	}

	return shader;
}

#ifdef SHADER_DIR
static char* LoadFromDisk(const char* name, int* psize)
{
	char path[256];
	sprintf(path, SHADER_DIR "/%s", name);

	FILE* f = fopen(path, "rb");
	if (!f)
	{
		Plat_Error("Could not load shader file %s\n", path);
	}

	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* data = (char*)malloc(size);
	fread(data, 1, size, f);
	fclose(f);

	*psize = size;
	return data;
}
#endif

GLuint CreateShaderProgram(const char* name,
#ifdef SHADER_DIR
	char* vertex_name,
	char* fragment_name,
#else
	const char* vertex_source,
	int vertex_length,
	const char* fragment_source,
	int fragment_length,
#endif
	const attribute_t* attributes,
	int num_attributes,
	const uniform_t* uniforms,
	int num_uniforms)
{
#ifdef SHADER_DIR
	int vertex_length;
	char* vertex_source = LoadFromDisk(vertex_name, &vertex_length);
#endif
	GLuint vertex_shader = CompileShader(name, vertex_source, vertex_length, GL_VERTEX_SHADER);

#ifdef SHADER_DIR
	int fragment_length;
	char* fragment_source = LoadFromDisk(fragment_name, &fragment_length);
#endif
	GLuint fragment_shader = CompileShader(name, fragment_source, fragment_length, GL_FRAGMENT_SHADER);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	for (int i = 0; i < num_attributes; i++)
		glBindAttribLocation(program, attributes[i].location, attributes[i].name);

	glLinkProgram(program);

	GLint is_linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &is_linked);
	if (!is_linked)
	{
		char message[1024];
		glGetProgramInfoLog(program, sizeof(message), NULL, message);
		gEngfuncs.Con_DPrintf("Linking %s program failed:\n%s", name, message);
	}

	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	for (int i = 0; i < num_uniforms; i++)
		*(uniforms[i].location) = glGetUniformLocation(program, uniforms[i].name);

	return program;
}