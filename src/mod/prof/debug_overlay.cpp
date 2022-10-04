#include "mod.h"


struct OverlayBase_t;


namespace Mod::Prof::Debug_Overlay
{
	DETOUR_DECL_MEMBER(void, CDebugOverlay_Paint)
	{
		VPROF_BUDGET("CDebugOverlay::Paint", "DebugOverlayRender");
		DETOUR_MEMBER_CALL(CDebugOverlay_Paint)();
	}
	
	
	DETOUR_DECL_STATIC(void, CDebugOverlay_DrawAllOverlays)
	{
		VPROF_BUDGET("CDebugOverlay::DrawAllOverlays", "DebugOverlayRender");
		DETOUR_STATIC_CALL(CDebugOverlay_DrawAllOverlays)();
	}
	
	DETOUR_DECL_STATIC(void, CDebugOverlay_DrawOverlay, OverlayBase_t *pOverlay)
	{
		VPROF_BUDGET("CDebugOverlay::DrawOverlay", "DebugOverlayRender");
		DETOUR_STATIC_CALL(CDebugOverlay_DrawOverlay)(pOverlay);
	}
	
	
	DETOUR_DECL_STATIC(void, RenderWireframeSphere, const Vector& vCenter, float flRadius, int nTheta, int nPhi, Color c, bool bZBuffer)
	{
		VPROF_BUDGET("RenderWireframeSphere", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderWireframeSphere)(vCenter, flRadius, nTheta, nPhi, c, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderSphere, const Vector& vCenter, float flRadius, int nTheta, int nPhi, Color c, IMaterial *pMaterial)
	{
		VPROF_BUDGET("RenderSphere", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderSphere)(vCenter, flRadius, nTheta, nPhi, c, pMaterial);
	}
	DETOUR_DECL_STATIC(void, RenderWireframeBox, const Vector& vOrigin, const QAngle& angles, const Vector& vMins, const Vector& vMaxs, Color c, bool bZBuffer)
	{
		VPROF_BUDGET("RenderWireframeBox", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderWireframeBox)(vOrigin, angles, vMins, vMaxs, c, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderWireframeSweptBox, const Vector& vStart, const Vector& vEnd, const QAngle& angles, const Vector& vMins, const Vector& vMaxs, Color c, bool bZBuffer)
	{
		VPROF_BUDGET("RenderWireframeSweptBox", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderWireframeSweptBox)(vStart, vEnd, angles, vMins, vMaxs, c, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderBox, const Vector& origin, const QAngle& angles, const Vector& mins, const Vector& maxs, Color c, IMaterial *pMaterial, bool bInsideOut)
	{
		VPROF_BUDGET("RenderBox", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderBox)(origin, angles, mins, maxs, c, pMaterial, bInsideOut);
	}
	DETOUR_DECL_STATIC(void, RenderAxes_vec, const Vector& vOrigin, float flScale, bool bZBuffer)
	{
		VPROF_BUDGET("RenderAxes", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderAxes_vec)(vOrigin, flScale, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderAxes_mat, const matrix3x4_t& transform, float flScale, bool bZBuffer)
	{
		VPROF_BUDGET("RenderAxes", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderAxes_mat)(transform, flScale, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderLine, const Vector& v1, const Vector& v2, Color c, bool bZBuffer)
	{
		VPROF_BUDGET("RenderLine", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderLine)(v1, v2, c, bZBuffer);
	}
	DETOUR_DECL_STATIC(void, RenderTriangle, const Vector& p1, const Vector& p2, const Vector& p3, Color c, IMaterial *pMaterial)
	{
		VPROF_BUDGET("RenderTriangle", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderTriangle)(p1, p2, p3, c, pMaterial);
	}
	DETOUR_DECL_STATIC(void, RenderQuad, IMaterial *pMaterial, float x, float y, float w, float h, float z, float s0, float t0, float s1, float t1, const Color& clr)
	{
		VPROF_BUDGET("RenderQuad", "DebugOverlayRender");
		DETOUR_STATIC_CALL(RenderQuad)(pMaterial, x, y, w, h, z, s0, t0, s1, t1, clr);
	}
	DETOUR_DECL_STATIC(void, DrawScreenSpaceRectangle, IMaterial *pMaterial, int nDestX, int nDestY, int nWidth, int nHeight, float flSrcTextureX0, float flSrcTextureY0, float flSrcTextureX1, float flSrcTextureY1, int nSrcTextureWidth, int nSrcTextureHeight, void *pClientRenderable, int nXDice, int nYDice, float fDepth)
	{
		VPROF_BUDGET("DrawScreenSpaceRectangle", "DebugOverlayRender");
		DETOUR_STATIC_CALL(DrawScreenSpaceRectangle)(pMaterial, nDestX, nDestY, nWidth, nHeight, flSrcTextureX0, flSrcTextureY0, flSrcTextureX1, flSrcTextureY1, nSrcTextureWidth, nSrcTextureHeight, pClientRenderable, nXDice, nYDice, fDepth);
	}
	
	
	DETOUR_DECL_MEMBER(int, CMatSystemSurface_DrawColoredText, vgui::HFont font, int x, int y, int r, int g, int b, int a, const char *fmt, va_list argptr)
	{
		VPROF_BUDGET("CMatSystemSurface::DrawColoredText", "DebugOverlayRender");
		return DETOUR_MEMBER_CALL(CMatSystemSurface_DrawColoredText)(font, x, y, r, g, b, a, fmt, argptr);
	}
	
	
#if defined _WINDOWS
	DETOUR_DECL_STATIC(int, D_vsnprintf, char *buffer, size_t count, const char *format, va_list argptr)
	{
		VPROF_BUDGET("_vsnprintf", "DebugOverlayRender");
		return DETOUR_STATIC_CALL(D_vsnprintf)(buffer, count, format, argptr);
	}
	
	DETOUR_DECL_STATIC(int, D_MultiByteToWideChar, UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
	{
		VPROF_BUDGET("MultiByteToWideChar", "DebugOverlayRender");
		return DETOUR_STATIC_CALL(D_MultiByteToWideChar)(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
	}
#endif
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Prof:Debug_Overlay")
		{
			/* class CDebugOverlay (2D overlay VGUI panel) */
			MOD_ADD_DETOUR_MEMBER(CDebugOverlay_Paint, "[client] CDebugOverlay::Paint");
			
			/* namespace CDebugOverlay (IVDebugOverlay implementation) */
			MOD_ADD_DETOUR_STATIC(CDebugOverlay_DrawAllOverlays, "[client] CDebugOverlay::DrawAllOverlays");
			MOD_ADD_DETOUR_STATIC(CDebugOverlay_DrawOverlay,     "[client] CDebugOverlay::DrawOverlay");
			
			/* tier2 renderutils */
			MOD_ADD_DETOUR_STATIC(RenderWireframeSphere,    "[client] RenderWireframeSphere");
			MOD_ADD_DETOUR_STATIC(RenderSphere,             "[client] RenderSphere [material]");
			MOD_ADD_DETOUR_STATIC(RenderWireframeBox,       "[client] RenderWireframeBox");
			MOD_ADD_DETOUR_STATIC(RenderWireframeSweptBox,  "[client] RenderWireframeSweptBox");
			MOD_ADD_DETOUR_STATIC(RenderBox,                "[client] RenderBox [material]");
		//	MOD_ADD_DETOUR_STATIC(RenderAxes_vec,           "[client] RenderAxes [vec]");
		//	MOD_ADD_DETOUR_STATIC(RenderAxes_mat,           "[client] RenderAxes [mat]");
			MOD_ADD_DETOUR_STATIC(RenderLine,               "[client] RenderLine");
			MOD_ADD_DETOUR_STATIC(RenderTriangle,           "[client] RenderTriangle [material]");
		//	MOD_ADD_DETOUR_STATIC(RenderQuad,               "[client] RenderQuad");
		//	MOD_ADD_DETOUR_STATIC(DrawScreenSpaceRectangle, "[client] DrawScreenSpaceRectangle");
			
			
			// something below this point is crashy
			
			
			/* vguimatsurface */
			MOD_ADD_DETOUR_MEMBER(CMatSystemSurface_DrawColoredText, "[client] CMatSystemSurface::DrawColoredText [va_list]");
			
#if defined _WINDOWS
			this->AddDetour(new CDetour("_vsnprintf",          reinterpret_cast<void *>(&_vsnprintf),          GET_STATIC_CALLBACK(D_vsnprintf),           GET_STATIC_INNERPTR(D_vsnprintf)));
			this->AddDetour(new CDetour("MultiByteToWideChar", reinterpret_cast<void *>(&MultiByteToWideChar), GET_STATIC_CALLBACK(D_MultiByteToWideChar), GET_STATIC_INNERPTR(D_MultiByteToWideChar)));
#endif
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_prof_debug_overlay", "0", FCVAR_NOTIFY,
		"Mod: profile debug overlay rendering",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
