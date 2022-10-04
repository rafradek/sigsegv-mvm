#include "mod.h"
#include "stub/cdebugoverlay.h"
#include "util/scope.h"


enum OverlayType_t : int32_t
{
	OVERLAY_BOX = 0,
	OVERLAY_SPHERE,
	OVERLAY_LINE,
	OVERLAY_TRIANGLE,
	OVERLAY_SWEPT_BOX,
	OVERLAY_BOX2,
};


struct OverlayBase_t
{
	OverlayType_t m_Type;
	int m_nCreationTick;
	int m_nServerCount;
	float m_flEndTime;
	OverlayBase_t *m_pNextOverlay;
};

struct OverlayBox_t : public OverlayBase_t
{
	Vector origin;
	Vector mins;
	Vector maxs;
	QAngle angles;
	int r;
	int g;
	int b;
	int a;
};

struct OverlaySphere_t : public OverlayBase_t
{
	Vector vOrigin;
	float flRadius;
	int nTheta;
	int nPhi;
	int r;
	int g;
	int b;
	int a;
};


/* hacky re-use of OverlayBox_t for ScreenRect */
struct OverlayScreenRect_t : public OverlayBase_t
{
	// Vector origin
	float _nan1; float _nan2; float _nan3;
	
	// Vector mins
	float xFrom; float yFrom; float _nan4;
	
	// Vector maxs
	float xTo; float yTo; float _nanID;
	
	// QAngle angles
	float _neg_zero[3];
	
	// int r
	Color cFill;
	
	// int g
	Color cEdge;
	
	// int b
	int _int_max;
	
	// int a
	int _int_min;
};
static_assert(sizeof(OverlayScreenRect_t) == sizeof(OverlayBox_t));


/* hacky re-use of OverlayBox_t for ScreenLine */
struct OverlayScreenLine_t : public OverlayBase_t
{
	// Vector origin
	float _nan1; float _nan2; float _nan3;
	
	// Vector mins
	float xFrom; float yFrom; float _nan4;
	
	// Vector maxs
	float xTo; float yTo; float _nanID;
	
	// QAngle angles
	float _neg_zero[3];
	
	// int r
	Color cFrom;
	
	// int g
	Color cTo;
	
	// int b
	int _int_max;
	
	// int a
	int _int_min;
};
static_assert(sizeof(OverlayScreenLine_t) == sizeof(OverlayBox_t));



namespace Mod::Util::Overlay_Enhancements
{
	void CustomRenderScreenSpaceRect(float xFrom, float yFrom, float xTo, float yTo, const Color& cFill, const Color& cEdge)
	{
		
	}
	
	void CustomRenderScreenSpaceLine(float xFrom, float yFrom, float xTo, float yTo, const Color& cFrom, const Color& cTo)
	{
		
	}
	
	
	bool VerifyScreenRectOverlay(const OverlayScreenRect_t *pOverlay)
	{
		static constexpr uint32_t nan1  = 0x7fc00001;
		static constexpr uint32_t nan2  = 0x7fc00002;
		static constexpr uint32_t nan3  = 0x7fc00003;
		static constexpr uint32_t nan4  = 0x7fc00004;
		static constexpr uint32_t nanID = 0x7fc00100;
		
		/* do some checks with magic numbers to verify that this is indeed a ScreenRect overlay */
		if (memcmp(&pOverlay->_nan1,  &nan1,  sizeof(float)) != 0) return false;
		if (memcmp(&pOverlay->_nan2,  &nan2,  sizeof(float)) != 0) return false;
		if (memcmp(&pOverlay->_nan3,  &nan3,  sizeof(float)) != 0) return false;
		if (memcmp(&pOverlay->_nan4,  &nan4,  sizeof(float)) != 0) return false;
		if (memcmp(&pOverlay->_nanID, &nanID, sizeof(float)) != 0) return false;
		
		if (pOverlay->_neg_zero[0] != -0.0f || !std::signbit(pOverlay->_neg_zero[0])) return false;
		if (pOverlay->_neg_zero[1] != -0.0f || !std::signbit(pOverlay->_neg_zero[1])) return false;
		if (pOverlay->_neg_zero[2] != -0.0f || !std::signbit(pOverlay->_neg_zero[2])) return false;
		
		if (pOverlay->_int_max != std::numeric_limits<int>::max()) return false;
		if (pOverlay->_int_min != std::numeric_limits<int>::min()) return false;
		
		return true;
	}
	
	bool VerifyScreenLineOverlay(const OverlayScreenLine_t *pOverlay)
	{
		static constexpr uint32_t nan1  = 0x7fc00001;
		static constexpr uint32_t nan2  = 0x7fc00002;
		static constexpr uint32_t nan3  = 0x7fc00003;
		static constexpr uint32_t nan4  = 0x7fc00004;
		static constexpr uint32_t nanID = 0x7fc00200;
		
		/* do some checks with magic numbers to verify that this is indeed a ScreenLine overlay */
		if (memcmp(&pOverlay->_nan1,  &nan1,  sizeof(float)) != 0) return false;
		if (memcmp(&pOverlay->_nan2,  &nan2,  sizeof(float)) != 0) return false;
		if (memcmp(&pOverlay->_nan3,  &nan3,  sizeof(float)) != 0) return false;
		if (memcmp(&pOverlay->_nan4,  &nan4,  sizeof(float)) != 0) return false;
		if (memcmp(&pOverlay->_nanID, &nanID, sizeof(float)) != 0) return false;
		
		if (pOverlay->_neg_zero[0] != -0.0f || !std::signbit(pOverlay->_neg_zero[0])) return false;
		if (pOverlay->_neg_zero[1] != -0.0f || !std::signbit(pOverlay->_neg_zero[1])) return false;
		if (pOverlay->_neg_zero[2] != -0.0f || !std::signbit(pOverlay->_neg_zero[2])) return false;
		
		if (pOverlay->_int_max != std::numeric_limits<int>::max()) return false;
		if (pOverlay->_int_min != std::numeric_limits<int>::min()) return false;
		
		return true;
	}
	
	
	RefCount rc_CDebugOverlay_Paint;
	DETOUR_DECL_MEMBER(void, CDebugOverlay_Paint)
	{
		SCOPED_INCREMENT(rc_CDebugOverlay_Paint);
		DETOUR_MEMBER_CALL(CDebugOverlay_Paint)();
	}
	
	
	ConVar cvar_textspacing("sig_util_overlay_enhancements_textspacing", "0", FCVAR_NOTIFY,
		"Utility: overlay forwarding: client rendering enhancements: use non-hardcoded height for text overlay line spacing");
	
	DETOUR_DECL_MEMBER(int, CMatSystemSurface_DrawColoredText, vgui::HFont font, int x, int y, int r, int g, int b, int a, const char *fmt, va_list argptr)
	{
		if (cvar_textspacing.GetBool() && rc_CDebugOverlay_Paint > 0 && g_pVGuiSurface != nullptr && strcmp(fmt, "%s") == 0) {
			va_list va;
			va_copy(va, argptr);
			
			const char *str = va_arg(va, const char *);
			auto pCurrText = reinterpret_cast<const OverlayText_t *>((uintptr_t)str - offsetof(OverlayText_t, text));
			
			/* undo the stupid line spacing */
			y -= (pCurrText->lineOffset * 13);
			
			/* apply proper line spacing */
			y += (pCurrText->lineOffset * (g_pVGuiSurface->GetFontTall(font) + 1));
			
			va_end(va);
		}
		
		return DETOUR_MEMBER_CALL(CMatSystemSurface_DrawColoredText)(font, x, y, r, g, b, a, fmt, argptr);
	}
	
	
	ConVar cvar_sphere("sig_util_overlay_enhancements_sphere", "0", FCVAR_NOTIFY,
		"Utility: overlay forwarding: client rendering enhancements: use better sphere rendering");
	
	DETOUR_DECL_STATIC(void, CDebugOverlay_DrawOverlay, OverlayBase_t *pOverlay)
	{
		if (cvar_sphere.GetBool()) {
			/* for OVERLAY_SPHERE, CDebugOverlay::DrawOverlay calls the IMaterial *
			 * version of RenderSphere, which is extremely ugly */
			if (pOverlay->m_Type == OVERLAY_SPHERE) {
				OverlaySphere_t *pSphere = static_cast<OverlaySphere_t *>(pOverlay);
				
				if (pSphere->a > 0) {
					RenderSphere(pSphere->vOrigin, pSphere->flRadius, pSphere->nTheta, pSphere->nPhi, Color(pSphere->r, pSphere->g, pSphere->b, pSphere->a), false);
				}
				
				RenderWireframeSphere(pSphere->vOrigin, pSphere->flRadius, pSphere->nTheta, pSphere->nPhi, Color(pSphere->r, pSphere->g, pSphere->b, pSphere->a), true);
				return;
			}
		}
		
		/* hack to enable ScreenRect via an existing unrelated overlay type */
		if (pOverlay->m_Type == OVERLAY_BOX) {
			OverlayScreenRect_t *pScreenRect = static_cast<OverlayScreenRect_t *>(pOverlay);
			if (VerifyScreenRectOverlay(pScreenRect)) {
				DevMsg(">>> DrawScreenRect [xFrom %.3f] [yFrom %.3f] [xTo %.3f] [yTo %.3f] [cFill %02x%02x%02x%02x] [cEdge %02x%02x%02x%02x]\n",
					pScreenRect->xFrom, pScreenRect->yFrom, pScreenRect->xTo, pScreenRect->yTo,
					pScreenRect->cFill[0], pScreenRect->cFill[1], pScreenRect->cFill[2], pScreenRect->cFill[3],
					pScreenRect->cEdge[0], pScreenRect->cEdge[1], pScreenRect->cEdge[2], pScreenRect->cEdge[3]);
				DevMsg(
					">>> %08x %08x %08x %08x %08x %08x %08x %08x\n"
					">>> %08x %08x %08x %08x %08x %08x %08x %08x\n",
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x00),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x04),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x08),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x0c),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x10),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x14),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x18),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x1c),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x20),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x24),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x28),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x2c),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x30),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x34),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x38),
					*(uint32_t *)((uintptr_t)pScreenRect + 0x14 + 0x3c));
				
				CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
				
				/* these vectors are in terms of pixels rather than [0.0f, 1.0f] */
				Vector vecFrom; debugoverlay->ScreenPosition(pScreenRect->xFrom, pScreenRect->yFrom, vecFrom);
				Vector vecTo;   debugoverlay->ScreenPosition(pScreenRect->xTo,   pScreenRect->yTo,   vecTo);
				
				int x = (int)vecFrom.x;
				int y = (int)vecFrom.y;
				int w = (int)vecTo.x - x;
				int h = (int)vecTo.y - y;
				
				if (pScreenRect->cEdge[3] != 0 && w >= 1 && h >= 1) {
					IMaterial *pMatWireframe = GetMatWireframe();
					
					auto r = (float)pScreenRect->cEdge.r();
					auto g = (float)pScreenRect->cEdge.g();
					auto b = (float)pScreenRect->cEdge.b();
					auto a = (float)pScreenRect->cEdge.a();
					
					pMatWireframe->ColorModulate(r, g, b);
					pMatWireframe->AlphaModulate(a);
					
					pRenderContext->DrawScreenSpaceRectangle(pMatWireframe,
						x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
					
					pMatWireframe->ColorModulate(1.0f, 1.0f, 1.0f);
					pMatWireframe->AlphaModulate(1.0f);
					
					#ifdef __GNUC__
					#warning Not sure if we need to un-modulate the material
					#warning (e.g. by calling ColorModulate and AlphaModulate with ones)
					#endif
				}
				
				if (pScreenRect->cFill[3] != 0 && w >= 3 && h >= 3) {
					IMaterial *pMatVertexColor = GetMatVertexColor();
					
					auto r = (float)pScreenRect->cFill.r();
					auto g = (float)pScreenRect->cFill.g();
					auto b = (float)pScreenRect->cFill.b();
					auto a = (float)pScreenRect->cFill.a();
					
					pMatVertexColor->ColorModulate(r, g, b);
					pMatVertexColor->AlphaModulate(a);
					
					pRenderContext->DrawScreenSpaceRectangle(pMatVertexColor,
						x + 1, y + 1, w - 2, h - 2, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
					
					pMatVertexColor->ColorModulate(1.0f, 1.0f, 1.0f);
					pMatVertexColor->AlphaModulate(1.0f);
					
					#ifdef __GNUC__
					#warning Not sure if we need to un-modulate the material
					#warning (e.g. by calling ColorModulate and AlphaModulate with ones)
					#endif
				}
				
				
				
				// actually, try IMatRenderContext::DrawScreenSpaceRectangle
				// use CMatRenderContextPtr from imaterialsystem.h
				
				// NOPE: (call IMaterialSystem::GetRenderContext for ptr)
				
				
				// for ScreenRect:
				// DrawScreenSpaceRectangle (tier1/renderutils)
				// CMatSystemSurface::DrawQuadArray
				
				return;
			}
		}
		
		/* hack to enable ScreenLine via an existing unrelated overlay type */
		if (pOverlay->m_Type == OVERLAY_BOX) {
			OverlayScreenLine_t *pScreenLine = static_cast<OverlayScreenLine_t *>(pOverlay);
			if (VerifyScreenLineOverlay(pScreenLine)) {
				DevMsg(">>> DrawScreenLine [xFrom %.3f] [yFrom %.3f] [xTo %.3f] [yTo %.3f] [cFrom %02x%02x%02x%02x] [cTo %02x%02x%02x%02x]\n",
					pScreenLine->xFrom, pScreenLine->yFrom, pScreenLine->xTo, pScreenLine->yTo,
					pScreenLine->cFrom[0], pScreenLine->cFrom[1], pScreenLine->cFrom[2], pScreenLine->cFrom[3],
					pScreenLine->cTo  [0], pScreenLine->cTo  [1], pScreenLine->cTo  [2], pScreenLine->cTo  [3]);
				DevMsg(
					">>> %08x %08x %08x %08x %08x %08x %08x %08x\n"
					">>> %08x %08x %08x %08x %08x %08x %08x %08x\n",
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x00),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x04),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x08),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x0c),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x10),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x14),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x18),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x1c),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x20),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x24),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x28),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x2c),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x30),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x34),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x38),
					*(uint32_t *)((uintptr_t)pScreenLine + 0x14 + 0x3c));
				
				/* these vectors are in terms of pixels rather than [0.0f, 1.0f] */
				Vector vecFrom; debugoverlay->ScreenPosition(pScreenLine->xFrom, pScreenLine->yFrom, vecFrom);
				Vector vecTo;   debugoverlay->ScreenPosition(pScreenLine->xTo,   pScreenLine->yTo,   vecTo);
				
				/* skip drawing if alpha is all zero */
				if (pScreenLine->cFrom[3] != 0 || pScreenLine->cTo[3] != 0) {
					CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
					
					pRenderContext->MatrixMode(MATERIAL_VIEW);
					pRenderContext->PushMatrix();
					pRenderContext->LoadIdentity();
					
					pRenderContext->MatrixMode(MATERIAL_PROJECTION);
					pRenderContext->PushMatrix();
					pRenderContext->LoadIdentity();
					
					pRenderContext->Bind(GetMatWireframe());
					
					IMesh *pMesh = pRenderContext->GetDynamicMesh();
					
					CMeshBuilder builder;
					builder.Begin(pMesh, MATERIAL_LINES, 1);
					
					builder.Position3f(vecFrom.x, vecFrom.y, 0.0f);
					builder.Color4ubv(reinterpret_cast<const uint8_t *>(&pScreenLine->cFrom));
					builder.AdvanceVertex();
					
					builder.Position3f(vecTo.x, vecTo.y, 0.0f);
					builder.Color4ubv(reinterpret_cast<const uint8_t *>(&pScreenLine->cTo));
					builder.AdvanceVertex();
					
					builder.End();
					pMesh->Draw();
					
					pRenderContext->MatrixMode(MATERIAL_VIEW);
					pRenderContext->PopMatrix();
					
					pRenderContext->MatrixMode(MATERIAL_PROJECTION);
					pRenderContext->PopMatrix();
				}
				
				return;
			}
		}
		
		DETOUR_STATIC_CALL(CDebugOverlay_DrawOverlay)(pOverlay);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Overlay_Enhancements")
		{
			MOD_ADD_DETOUR_MEMBER(CDebugOverlay_Paint,               "[client] CDebugOverlay::Paint");
			MOD_ADD_DETOUR_MEMBER(CMatSystemSurface_DrawColoredText, "[client] CMatSystemSurface::DrawColoredText [va_list]");
			
			MOD_ADD_DETOUR_STATIC(CDebugOverlay_DrawOverlay, "[client] CDebugOverlay::DrawOverlay");
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_overlay_enhancements", "0", FCVAR_NOTIFY,
		"Utility: overlay forwarding: client rendering enhancements",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
