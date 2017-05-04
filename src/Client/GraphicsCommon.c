#include "GraphicsCommon.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "Platform.h"

void GfxCommon_Init() {
	GfxCommon_quadVb = Gfx_CreateDynamicVb(VertexFormat_P3fC4b, 4);
	GfxCommon_texVb = Gfx_CreateDynamicVb(VertexFormat_P3fT2fC4b, 4);
}

void GfxCommon_Free() {
	Gfx_DeleteVb(&GfxCommon_quadVb);
	Gfx_DeleteVb(&GfxCommon_texVb);
}

void GfxCommon_LoseContext(String reason) {
	Gfx_LostContext = true;
	Platform_Log(String_FromConstant("Lost graphics context:"));
	Platform_Log(reason);

	if (ContextLost != null) ContextLost();
	GfxCommon_Recreate();
}

void GfxCommon_RecreateContext() {
	Gfx_LostContext = false;
	Platform_Log(String_FromConstant("Recreating graphics context"));

	if (ContextRecreated != null) ContextRecreated();
	GfxCommon_Init();
}


void GfxCommon_UpdateDynamicVb(Int32 drawMode, Int32 vb, void* vertices, Int32 vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawVb(drawMode, 0, vCount);
}

void GfxCommon_UpdateDynamicIndexedVb(Int32 drawMode, Int32 vb, void* vertices, Int32 vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawIndexedVb(drawMode, vCount * 6 / 4, 0);
}

void GfxCommon_Draw2DFlat(Real32 x, Real32 y, Real32 width, Real32 height, 
	FastColour col) {
	VertexP3fC4b quadVerts[4];
	VertexP3C4b_Set(&quadVerts[0], x, y, 0, col);
	VertexP3C4b_Set(&quadVerts[1], x + width, y, 0, col);
	VertexP3C4b_Set(&quadVerts[2], x + width, y + height, 0, col);
	VertexP3C4b_Set(&quadVerts[3], x, y + height, 0, col);

	Gfx_SetBatchFormat(VertexFormat_P3fC4b);
	GfxCommon_UpdateDynamicIndexedVb(DrawMode_Triangles, GfxCommon_quadVb, quadVerts, 4);
}

void GfxCommon_Draw2DGradient(Real32 x, Real32 y, Real32 width, Real32 height,
	FastColour topCol, FastColour bottomCol) {
	VertexP3fC4b quadVerts[4];
	VertexP3C4b_Set(&quadVerts[0], x, y, 0, topCol);
	VertexP3C4b_Set(&quadVerts[1], x + width, y, 0, topCol);
	VertexP3C4b_Set(&quadVerts[2], x + width, y + height, 0, bottomCol);
	VertexP3C4b_Set(&quadVerts[3], x, y + height, 0, bottomCol);

	Gfx_SetBatchFormat(VertexFormat_P3fC4b);
	GfxCommon_UpdateDynamicIndexedVb(DrawMode_Triangles, GfxCommon_quadVb, quadVerts, 4);
}

void GfxCommon_Draw2DTexture(Texture* tex, FastColour col) {
	Int32 index = 0;
	VertexP3fT2fC4b texVerts[4];
	VertexP3fT2fC4b* ptr = texVerts;
	GfxCommon_Make2DQuad(tex, col, &ptr);
	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);
	GfxCommon_UpdateDynamicIndexedVb(DrawMode_Triangles, GfxCommon_texVb, texVerts, 4);
}

void GfxCommon_Make2DQuad(Texture* tex, FastColour col, VertexP3fT2fC4b** vertices) {
	float x1 = tex->X, y1 = tex->Y, x2 = tex->X + tex->Width, y2 = tex->Y + tex->Height;
#if USE_DX
	// NOTE: see "https://msdn.microsoft.com/en-us/library/windows/desktop/bb219690(v=vs.85).aspx",
	// i.e. the msdn article called "Directly Mapping Texels to Pixels (Direct3D 9)" for why we have to do this.
	x1 -= 0.5f; x2 -= 0.5f;
	y1 -= 0.5f; y2 -= 0.5f;
#endif
	VertexP3fT2fC4b_Set(*vertices, x1, y1, 0, tex->U1, tex->V1, col); vertices++;
	VertexP3fT2fC4b_Set(*vertices, x2, y1, 0, tex->U2, tex->V1, col); vertices++;
	VertexP3fT2fC4b_Set(*vertices, x2, y2, 0, tex->U2, tex->V2, col); vertices++;
	VertexP3fT2fC4b_Set(*vertices, x1, y2, 0, tex->U1, tex->V2, col); vertices++;
}

void GfxCommon_Mode2D(Real32 width, Real32 height, bool setFog) {
	Gfx_SetMatrixMode(MatrixType_Projection);
	Gfx_PushMatrix();
	Gfx_LoadOrthoMatrix(width, height);
	Gfx_SetMatrixMode(MatrixType_Modelview);
	Gfx_PushMatrix();
	Gfx_LoadIdentityMatrix();

	Gfx_SetDepthTest(false);
	Gfx_SetAlphaBlending(true);
	if (setFog) Gfx_SetFog(false);
}

void GfxCommon_Mode3D(bool setFog) {
	Gfx_SetMatrixMode(MatrixType_Projection);
	Gfx_PopMatrix(); // Get rid of orthographic 2D matrix.
	Gfx_SetMatrixMode(MatrixType_Modelview);
	Gfx_PopMatrix();

	Gfx_SetDepthTest(false);
	Gfx_SetAlphaBlending(false);
	if (setFog) Gfx_SetFog(true);
}

Int32 GfxCommon_MakeDefaultIb() {
	Int32 element = 0;
	UInt16 indices[Gfx_MaxIndices];
	UInt16* ptr = indices;
	
	for (Int32 i = 0; i < Gfx_MaxIndices; i += 6) {
		*ptr = (UInt16)(element + 0); ptr++;
		*ptr = (UInt16)(element + 1); ptr++;
		*ptr = (UInt16)(element + 2); ptr++;

		*ptr = (UInt16)(element + 2); ptr++;
		*ptr = (UInt16)(element + 3); ptr++;
		*ptr = (UInt16)(element + 0); ptr++;
		element += 4;
	}
	return Gfx_CreateIb(indices, Gfx_MaxIndices);
}