#pragma once

struct mspriteframe_t
{
	int width;
	int height;
	float up, down, left, right;
	int gl_texturenum;
};

typedef struct
{
	int width, height;
	byte data[4]; // variably sized
} qpic_t;

typedef int (*_Draw_String)(int x, int y, char* str);
typedef int (*_GL_Bind)(int texnum);
typedef void (*_Draw_Frame)(mspriteframe_t* pFrame, int ix, int iy, const Rect* prcSubRect);
typedef void (*_BoxFilter3x3)(byte* out, byte* in, int w, int h, int x, int y);
typedef void (*_ComputeScaledSize)(int* wscale, int* hscale, int width, int height);
typedef void (*_GL_ResampleTexture)(unsigned int* in, int inwidth, int inheight, unsigned int* out, int outwidth, int outheight);
typedef void (*_GL_ResampleAlphaTexture)(byte* in, int inwidth, int inheight, byte* out, int outwidth, int outheight);
typedef void (*_GL_Upload32)(unsigned int* data, int width, int height, qboolean mipmap, int iType, int filter);
typedef void (*_GL_Upload16)(unsigned char* data, int width, int height, qboolean mipmap, int iType, unsigned char* pPal, int filter);

typedef void (*_VGUI2_ResetCurrentTexture)();

typedef void (*_VideoMode_GetCurrentVideoMode)(int* wide, int* tall, int* bpp);

extern _GL_Bind ORIG_GL_Bind;
extern _Draw_Frame ORIG_Draw_Frame;

void EnableScissorTest(int x, int y, int width, int height);
void DisableScissorTest(void);

void Draw_Init();

void Draw_FillRGBA(int x, int y, int w, int h, int r, int g, int b, int a);

int Draw_Character(int x, int y, int num, unsigned int font);

int Draw_MessageCharacterAdd(int x, int y, int num, int rr, int gg, int bb, unsigned int font);

int Draw_String(int x, int y, char* str);

int Draw_StringLen(const char* psz, unsigned int font);

void Draw_SetTextColor(float r, float g, float b);

void Draw_GetDefaultColor();

void Draw_ResetTextColor();

void Draw_FillRGBABlend(int x, int y, int w, int h, int r, int g, int b, int a);

// void GL_SelectTexture(GLenum target);

void GL_Bind(int texnum);

bool ValidateWRect(const Rect* prc);
bool IntersectWRect(const Rect* prc1, const Rect* prc2, Rect* prc);

void AdjustSubRect(mspriteframe_t* pFrame, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom, int* pw, int* ph, const Rect* prcSubRect);

void Draw_SpriteFrame(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const Rect* prcSubRect);
void Draw_SpriteFrameHoles(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const Rect* prcSubRect);
void Draw_SpriteFrameAdditive(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const Rect* prcSubRect);
void Draw_SpriteFrameGeneric(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const Rect* prcSubRect, int src, int dest, int width, int height);
void Draw_Pic(int x, int y, qpic_t* pic);

void Draw_BeginDisc();

void GLDraw_Hook();

#define TEX_TYPE_NONE 0
#define TEX_TYPE_ALPHA 1
#define TEX_TYPE_LUM 2
#define TEX_TYPE_ALPHA_GRADIENT 3
#define TEX_TYPE_RGBA 4
#define TEX_IS_ALPHA(type) ((type) == TEX_TYPE_ALPHA || (type) == TEX_TYPE_ALPHA_GRADIENT || (type) == TEX_TYPE_RGBA)

#define MAX_GLTEXTURES 4800
