#include "mod.h"


namespace Mod::Prof::Tier2_RenderUtils
{
	DETOUR_DECL_STATIC(void, GenerateBoxVertices, const Vector& vOrigin, const QAngle& angles, const Vector& vMins, const Vector& vMaxs, Vector pVerts[8])
	{
		DETOUR_STATIC_CALL(vOrigin, angles, vMins, vMaxs, pVerts);
	}
	
	DETOUR_DECL_STATIC(void, RenderWireframeSphere, const Vector& vCenter, float flRadius, int nTheta, int nPhi, Color c, bool bZBuffer)
	{
		DETOUR_STATIC_CALL(vCenter, flRadius, nTheta, nPhi, c, bZBuffer);
	}
	
	DETOUR_DECL_STATIC(void, RenderSphere1, const Vector& vCenter, float flRadius, int nTheta, int nPhi, Color c, bool bZBuffer)
	{
		DETOUR_STATIC_CALL(vCenter, flRadius, nTheta, nPhi, c, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderSphere2, const Vector& vCenter, float flRadius, int nTheta, int nPhi, Color c, IMaterial *pMaterial)
	{
		DETOUR_STATIC_CALL(vCenter, flRadius, nTheta, nPhi, c, pMaterial);
	}
	
	DETOUR_DECL_STATIC(void, RenderWireframeBox, const Vector& vOrigin, const QAngle& angles, const Vector& vMins, const Vector& vMaxs, Color c, bool bZBuffer)
	{
		DETOUR_STATIC_CALL(vOrigin, angles, vMins, vMaxs, c, bZBuffer);
	}
	
	DETOUR_DECL_STATIC(void, RenderWireframeSweptBox, const Vector& vStart, const Vector& vEnd, const QAngle& angles, const Vector& vMins, const Vector& vMaxs, Color c, bool bZBuffer)
	{
		DETOUR_STATIC_CALL(vStart, vEnd, angles, vMins, vMaxs, c, bZBuffer);
	}
	
	DETOUR_DECL_STATIC(void, RenderBox1, const Vector& origin, const QAngle& angles, const Vector& mins, const Vector& maxs, Color c, bool bZBuffer, bool bInsideOut)
	{
		DETOUR_STATIC_CALL(origin, angles, mins, maxs, c, bZBuffer, bInsideOut);
	}
	DETOUR_DECL_STATIC(void, RenderBox2, const Vector& origin, const QAngle& angles, const Vector& mins, const Vector& maxs, Color c, IMaterial *pMaterial, bool bInsideOut)
	{
		DETOUR_STATIC_CALL(origin, angles, mins, maxs, c, pMaterial, bInsideOut);
	}
	
	DETOUR_DECL_STATIC(void, RenderAxes1, const Vector& vOrigin, float flScale, bool bZBuffer)
	{
		DETOUR_STATIC_CALL(vOrigin, flScale, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderAxes2, const matrix3x4_t& transform, float flScale, bool bZBuffer)
	{
		DETOUR_STATIC_CALL(transform, flScale, bZBuffer);
	}
	
	DETOUR_DECL_STATIC(void, RenderLine, const Vector& v1, const Vector& v2, Color c, bool bZBuffer)
	{
		DETOUR_STATIC_CALL(v1, v2, c, bZBuffer);
	}
	
	DETOUR_DECL_STATIC(void, RenderTriangle1, const Vector& p1, const Vector& p2, const Vector& p3, Color c, bool bZBuffer)
	{
		DETOUR_STATIC_CALL(p1, p2, p3, c, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderTriangle2, const Vector& p1, const Vector& p2, const Vector& p3, Color c, IMaterial *pMaterial)
	{
		DETOUR_STATIC_CALL(p1, p2, p3, c, pMaterial);
	}
	
	DETOUR_DECL_STATIC(void, RenderQuad, IMaterial *pMaterial, float x, float y, float w, float h, float z, float s0, float t0, float s1, float t1, const Color& clr)
	{
		DETOUR_STATIC_CALL(pMaterial, x, y, w, h, z, s0, t0, s1, t1, clr);
	}
	
	DETOUR_DECL_STATIC(void, DrawScreenSpaceRectangle, IMaterial *pMaterial, int nDestX, int nDestY, int nWidth, int nHeight, float flSrcTextureX0, float flSrcTextureY0, float flSrcTextureX1, float flSrcTextureY1, int nSrcTextureWidth, int nSrcTextureHeight, void *pClientRenderable, int nXDice, int nYDice, float fDepth)
	{
		DETOUR_STATIC_CALL(pMaterial, nDestX, nDestY, nWidth, nHeight, flSrcTextureX0, flSrcTextureY0, flSrcTextureX1, flSrcTextureY1, nSrcTextureWidth, nSrcTextureHeight, pClientRenderable, nXDice, nYDice, fDepth);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Prof:Tier2_RenderUtils")
		{
			MOD_ADD_DETOUR_STATIC(GenerateBoxVertices,      "[client] GenerateBoxVertices");
			
			MOD_ADD_DETOUR_STATIC(RenderWireframeSphere,    "[client] RenderWireframeSphere");
			
		//	MOD_ADD_DETOUR_STATIC(RenderSphere1,            "[client] RenderSphere");
			MOD_ADD_DETOUR_STATIC(RenderSphere2,            "[client] RenderSphere [material]");
			
			MOD_ADD_DETOUR_STATIC(RenderWireframeBox,       "[client] RenderWireframeBox");
			
			MOD_ADD_DETOUR_STATIC(RenderWireframeSweptBox,  "[client] RenderWireframeSweptBox");
			
			MOD_ADD_DETOUR_STATIC(RenderBox1,               "[client] RenderBox");
			MOD_ADD_DETOUR_STATIC(RenderBox2,               "[client] RenderBox [material]");
			
		//	MOD_ADD_DETOUR_STATIC(RenderAxes1,              "[client] RenderAxes [vec]");
		//	MOD_ADD_DETOUR_STATIC(RenderAxes2,              "[client] RenderAxes [mat]");
			
			MOD_ADD_DETOUR_STATIC(RenderLine,               "[client] RenderLine");
			
			MOD_ADD_DETOUR_STATIC(RenderTriangle1,          "[client] RenderTriangle");
			MOD_ADD_DETOUR_STATIC(RenderTriangle2,          "[client] RenderTriangle [material]");
			
		//	MOD_ADD_DETOUR_STATIC(RenderQuad,               "[client] RenderQuad");
			
		//	MOD_ADD_DETOUR_STATIC(DrawScreenSpaceRectangle, "[client] DrawScreenSpaceRectangle");
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_prof_tier2_renderutils", "0", FCVAR_NOTIFY,
		"Mod: add some hooks to tier2 renderutils functions so we can do MSVC CPU profiling without symbols",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
