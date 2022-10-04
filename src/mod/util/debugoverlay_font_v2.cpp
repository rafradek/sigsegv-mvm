#include "mod.h"
#include "util/scope.h"
#include "stub/fonts.h"


namespace Mod::Util::DebugOverlay_Font_v2
{
#if 0
	void Experiment(vgui::HFont hFontFG, vgui::HFont hFontBG)
	{
		CWin32Font *pFontFG = FontManager().GetFontForChar(hFontFG, ' ');
		CWin32Font *pFontBG = FontManager().GetFontForChar(hFontBG, ' ');
		
		DevMsg("pFontFG = %p\npFontBG = %p\n", pFontFG, pFontBG);
	}
#endif
	
	
	ConVar cvar_spew("sig_util_debugoverlay_font_v2_spew", "0", FCVAR_NOTIFY,
		"Utility: spew debugging information");
	
	
	bool bFGDirty = false;
	void SetFGFontDirty(IConVar *pConVar, const char *pOldValue, float flOldValue) { bFGDirty = true; }
	
	ConVar cvar_fg_name("sig_util_debugoverlay_font_v2_fg_name", "Consolas", FCVAR_NOTIFY,
		"Utility: FG font name", &SetFGFontDirty);
	ConVar cvar_fg_tall("sig_util_debugoverlay_font_v2_fg_tall", "14", FCVAR_NOTIFY,
		"Utility: FG font tall", &SetFGFontDirty);
	ConVar cvar_fg_weight("sig_util_debugoverlay_font_v2_fg_weight", "400", FCVAR_NOTIFY,
		"Utility: FG font weight", &SetFGFontDirty);
	ConVar cvar_fg_scanlines("sig_util_debugoverlay_font_v2_fg_scanlines", "0", FCVAR_NOTIFY,
		"Utility: FG font scanlines", &SetFGFontDirty);
	ConVar cvar_fg_blur("sig_util_debugoverlay_font_v2_fg_blur", "5", FCVAR_NOTIFY,
		"Utility: FG font blur", &SetFGFontDirty);
	ConVar cvar_fg_italic("sig_util_debugoverlay_font_v2_fg_italic", "0", FCVAR_NOTIFY,
		"Utility: FG font italic", &SetFGFontDirty);
	ConVar cvar_fg_underline("sig_util_debugoverlay_font_v2_fg_underline", "0", FCVAR_NOTIFY,
		"Utility: FG font underline", &SetFGFontDirty);
	ConVar cvar_fg_strikeout("sig_util_debugoverlay_font_v2_fg_strikeout", "0", FCVAR_NOTIFY,
		"Utility: FG font strikeout", &SetFGFontDirty);
	ConVar cvar_fg_antialias("sig_util_debugoverlay_font_v2_fg_antialias", "1", FCVAR_NOTIFY,
		"Utility: FG font antialias", &SetFGFontDirty);
	ConVar cvar_fg_dropshadow("sig_util_debugoverlay_font_v2_fg_dropshadow", "0", FCVAR_NOTIFY,
		"Utility: FG font dropshadow", &SetFGFontDirty);
	ConVar cvar_fg_outline("sig_util_debugoverlay_font_v2_fg_outline", "0", FCVAR_NOTIFY,
		"Utility: FG font outline", &SetFGFontDirty);
	
	bool bBGDirty = false;
	void SetBGFontDirty(IConVar *pConVar, const char *pOldValue, float flOldValue) { bBGDirty = true; }
	
	ConVar cvar_bg_name("sig_util_debugoverlay_font_v2_bg_name", "Consolas", FCVAR_NOTIFY,
		"Utility: BG font name", &SetBGFontDirty);
	ConVar cvar_bg_tall("sig_util_debugoverlay_font_v2_bg_tall", "14", FCVAR_NOTIFY,
		"Utility: BG font tall", &SetBGFontDirty);
	ConVar cvar_bg_weight("sig_util_debugoverlay_font_v2_bg_weight", "400", FCVAR_NOTIFY,
		"Utility: BG font weight", &SetBGFontDirty);
	ConVar cvar_bg_scanlines("sig_util_debugoverlay_font_v2_bg_scanlines", "0", FCVAR_NOTIFY,
		"Utility: BG font scanlines", &SetBGFontDirty);
	ConVar cvar_bg_blur("sig_util_debugoverlay_font_v2_bg_blur", "5", FCVAR_NOTIFY,
		"Utility: BG font blur", &SetBGFontDirty);
	ConVar cvar_bg_italic("sig_util_debugoverlay_font_v2_bg_italic", "0", FCVAR_NOTIFY,
		"Utility: BG font italic", &SetBGFontDirty);
	ConVar cvar_bg_underline("sig_util_debugoverlay_font_v2_bg_underline", "0", FCVAR_NOTIFY,
		"Utility: BG font underline", &SetBGFontDirty);
	ConVar cvar_bg_strikeout("sig_util_debugoverlay_font_v2_bg_strikeout", "0", FCVAR_NOTIFY,
		"Utility: BG font strikeout", &SetBGFontDirty);
	ConVar cvar_bg_antialias("sig_util_debugoverlay_font_v2_bg_antialias", "1", FCVAR_NOTIFY,
		"Utility: BG font antialias", &SetBGFontDirty);
	ConVar cvar_bg_dropshadow("sig_util_debugoverlay_font_v2_bg_dropshadow", "0", FCVAR_NOTIFY,
		"Utility: BG font dropshadow", &SetBGFontDirty);
	ConVar cvar_bg_outline("sig_util_debugoverlay_font_v2_bg_outline", "0", FCVAR_NOTIFY,
		"Utility: BG font outline", &SetBGFontDirty);
	
	
	vgui::HFont hFontFG = vgui::INVALID_FONT;
	vgui::HFont hFontBG = vgui::INVALID_FONT;
	
	void UpdateDebugOverlayFonts()
	{
		if (hFontFG == vgui::INVALID_FONT || bFGDirty) {
			DevMsg("UpdateDebugOverlayFonts: regenerating FG font\n");
			
			hFontFG = g_pVGuiSurface->CreateFont();
			
			const char *name = cvar_fg_name.GetString();
			int tall         = cvar_fg_tall.GetInt();
			int weight       = cvar_fg_weight.GetInt();
			int blur         = cvar_fg_blur.GetInt();
			int scanlines    = cvar_fg_scanlines.GetInt();
			
			int flags = vgui::ISurface::FONTFLAG_NONE;
			if (cvar_fg_italic    .GetBool()) flags |= vgui::ISurface::FONTFLAG_ITALIC;
			if (cvar_fg_underline .GetBool()) flags |= vgui::ISurface::FONTFLAG_UNDERLINE;
			if (cvar_fg_strikeout .GetBool()) flags |= vgui::ISurface::FONTFLAG_STRIKEOUT;
			if (cvar_fg_antialias .GetBool()) flags |= vgui::ISurface::FONTFLAG_ANTIALIAS;
			if (cvar_fg_dropshadow.GetBool()) flags |= vgui::ISurface::FONTFLAG_DROPSHADOW;
			if (cvar_fg_outline   .GetBool()) flags |= vgui::ISurface::FONTFLAG_OUTLINE;
			
			bool bFGOK = g_pVGuiSurface->SetFontGlyphSet(hFontFG, name, tall, weight, blur, scanlines, flags);
			DevMsg("UpdateDebugOverlayFonts: FG: %-3s %08lx\n", (bFGOK ? "OK" : "BAD"), hFontFG);
			
			bFGDirty = false;
		}
		
		if (hFontBG == vgui::INVALID_FONT || bBGDirty) {
			DevMsg("UpdateDebugOverlayFonts: regenerating BG font\n");
			
			hFontBG = g_pVGuiSurface->CreateFont();
			
			const char *name = cvar_bg_name.GetString();
			int tall         = cvar_bg_tall.GetInt();
			int weight       = cvar_bg_weight.GetInt();
			int blur         = cvar_bg_blur.GetInt();
			int scanlines    = cvar_bg_scanlines.GetInt();
			
			int flags = vgui::ISurface::FONTFLAG_NONE;
			if (cvar_bg_italic    .GetBool()) flags |= vgui::ISurface::FONTFLAG_ITALIC;
			if (cvar_bg_underline .GetBool()) flags |= vgui::ISurface::FONTFLAG_UNDERLINE;
			if (cvar_bg_strikeout .GetBool()) flags |= vgui::ISurface::FONTFLAG_STRIKEOUT;
			if (cvar_bg_antialias .GetBool()) flags |= vgui::ISurface::FONTFLAG_ANTIALIAS;
			if (cvar_bg_dropshadow.GetBool()) flags |= vgui::ISurface::FONTFLAG_DROPSHADOW;
			if (cvar_bg_outline   .GetBool()) flags |= vgui::ISurface::FONTFLAG_OUTLINE;
			
			bool bBGOK = g_pVGuiSurface->SetFontGlyphSet(hFontBG, name, tall, weight, blur, scanlines, flags);
			DevMsg("UpdateDebugOverlayFonts: BG: %-3s %08lx\n", (bBGOK ? "OK" : "BAD"), hFontBG);
			
			bBGDirty = false;
		}
	}
	
	
	// for messing with font rendering, here's the crucial call chain:
	// - bool CFontTextureCache::GetTextureForChars( vgui::HFont font, vgui::FontDrawType_t type, wchar_t *wch, int *textureID, float **texCoords, int numChars )
	//  - void CWin32Font::GetCharRGBA(wchar_t ch, int rgbaWide, int rgbaTall, unsigned char *rgba)
	//    - void Apply______ToTexture(...)
	
	// CFontTextureCache::GetTextureForChar(s) is called by:
	// - void CMatSystemSurface::PrecacheFontCharacters( HFont font, wchar_t *pCharacterString )
	// - bool CMatSystemSurface::DrawGetUnicodeCharRenderInfo( wchar_t ch, CharRenderInfo& info )
	// - void CMatSystemSurface::DrawPrintText(const wchar_t *text, int iTextLen, FontDrawType_t drawType /*= FONT_DRAW_DEFAULT */ )
	
	
	RefCount rc_CFontTextureCache_GetTextureForChars_FG;
	RefCount rc_CFontTextureCache_GetTextureForChars_BG;
	DETOUR_DECL_MEMBER(bool, CFontTextureCache_GetTextureForChars, vgui::HFont font, vgui::FontDrawType_t type, wchar_t *wch, int *textureID, float **texCoords, int numChars)
	{
		SCOPED_INCREMENT_IF(rc_CFontTextureCache_GetTextureForChars_FG, font == hFontFG);
		SCOPED_INCREMENT_IF(rc_CFontTextureCache_GetTextureForChars_BG, font == hFontBG);
		
		return DETOUR_MEMBER_CALL(CFontTextureCache_GetTextureForChars)(font, type, wch, textureID, texCoords, numChars);
	}
	
//	DETOUR_DECL_STATIC(void, ApplyOutlineToTexture, int rgbaWide, int rgbaTall, unsigned char *rgba, int iOutlineSize)
//	{
//		
//	}
	
//	ConVar cvar_darken_bg("sig_util_debugoverlay_font_v2_darken_bg", "1.0", FCVAR_NOTIFY,
//		"Utility: multiply background blur alpha by this factor", &SetBGFontDirty);
	
	ConVar cvar_expand_bg("sig_util_debugoverlay_font_v2_expand_bg", "2", FCVAR_NOTIFY,
		"Utility: expand background glyph size by this many pixels", &SetBGFontDirty);
	ConVar cvar_expand_box("sig_util_debugoverlay_font_v2_expand_box", "1", FCVAR_NOTIFY,
		"Utility: use box expansion instead of circle expansion", &SetBGFontDirty);
	ConVar cvar_expand_factor("sig_util_debugoverlay_font_v2_expand_factor", "2", FCVAR_NOTIFY,
		"Utility: supersampling factor for expansion", &SetBGFontDirty);
	
	
#define RGBA_COORD(ptr, wide, x, y) ((ptr) + (((x) + ((y) * (wide))) * 4))
	
	void ExpandifyTexture(int rgbaWide, int rgbaTall, uint8_t *rgba, float radius)
	{
		std::vector<uint8_t> buf;
		
		size_t size = (rgbaWide * rgbaTall * 4);
		buf.resize(size);
		
		float radius_sqr = Square(radius);
		int radius_int = (int)trunc(radius);
		
		bool box_mode = cvar_expand_box.GetBool();
		
		for (int y = 0; y < rgbaTall; ++y) {
			for (int x = 0; x < rgbaWide; ++x) {
				const uint8_t *RESTRICT src = RGBA_COORD(rgba,       rgbaWide, x, y);
				      uint8_t *RESTRICT dst = RGBA_COORD(buf.data(), rgbaWide, x, y);
				
				uint8_t max_alpha = 0x00;
				for (int dy = -radius_int; dy <= radius_int && max_alpha < 0xff; ++dy) {
					for (int dx = -radius_int; dx <= radius_int && max_alpha < 0xff; ++dx) {
						if (x + dx < 0 || x + dx >= rgbaWide)                  continue;
						if (y + dy < 0 || y + dy >= rgbaTall)                  continue;
						if (!box_mode && Square(dx) + Square(dy) > radius_sqr) continue;
						
						uint8_t alpha = RGBA_COORD(rgba, rgbaWide, x + dx, y + dy)[3];
						max_alpha = std::max(max_alpha, alpha);
					}
				}
				
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
				dst[3] = max_alpha;
				
				if (cvar_spew.GetBool()) {
					DevMsg("ExpandifyTexture: alpha @ [%3d, %3d]: %02x -> %02x\n", x, y, src[3], dst[3]);
				}
			}
		}
		
		memcpy(rgba, buf.data(), size);
	}
	
	
	void ScaleUp(int factor, int rgbaWide, int rgbaTall, uint8_t *RESTRICT rgba_dst, const uint8_t *RESTRICT rgba_src)
	{
		for (int y = 0; y < rgbaTall; ++y) {
		for (int x = 0; x < rgbaWide; ++x) {
			uint32_t src = *reinterpret_cast<const uint32_t *>(RGBA_COORD(rgba_src, rgbaWide, x, y));
			
			for (int dy = 0; dy < factor; ++dy) {
			for (int dx = 0; dx < factor; ++dx) {
				auto dst = reinterpret_cast<uint8_t *>(RGBA_COORD(rgba_dst, rgbaWide * factor, (x * factor) + dx, (y * factor) + dy));
				
				*dst = src;
			}
			}
		}
		}
	}
	
	void ScaleDown(int factor, int rgbaWide, int rgbaTall, uint8_t *RESTRICT rgba_dst, const uint8_t *RESTRICT rgba_src)
	{
		int factor_sqr = Square(factor);
		
		for (int y = 0; y < rgbaTall; ++y) {
		for (int x = 0; x < rgbaWide; ++x) {
			float accum_r = 0.0f;
			float accum_g = 0.0f;
			float accum_b = 0.0f;
			float accum_a = 0.0f;
			
			for (int dy = 0; dy < factor; ++dy) {
			for (int dx = 0; dx < factor; ++dx) {
				const uint8_t *src = RGBA_COORD(rgba_src, rgbaWide * factor, (x * factor) + dx, (y * factor) + dy);
				
				accum_r += src[0];
				accum_g += src[1];
				accum_b += src[2];
				accum_a += src[3];
			}
			}
			
			uint8_t *dst = RGBA_COORD(rgba_dst, rgbaWide, x, y);
			
			dst[0] = (uint8_t)Clamp(accum_r / factor_sqr, 0.0f, 255.0f);
			dst[1] = (uint8_t)Clamp(accum_g / factor_sqr, 0.0f, 255.0f);
			dst[2] = (uint8_t)Clamp(accum_b / factor_sqr, 0.0f, 255.0f);
			dst[3] = (uint8_t)Clamp(accum_a / factor_sqr, 0.0f, 255.0f);
		}
		}
	}
	
	void ExpandifyTextureFrac(int rgbaWide, int rgbaTall, uint8_t *rgba, float radius)
	{
		int factor = cvar_expand_factor.GetInt();
		
		size_t size        = (rgbaWide * rgbaTall * 4);
		size_t size_scaled = size * Square(factor);
		
		std::vector<uint8_t> scaled; scaled.resize(size_scaled);
		
		ScaleUp(factor, rgbaWide, rgbaTall, scaled.data(), rgba);
		ExpandifyTexture(factor * rgbaWide, factor * rgbaTall, scaled.data(), factor * radius);
		ScaleDown(factor, rgbaWide, rgbaTall, rgba, scaled.data());
	}
	
	
	DETOUR_DECL_STATIC(void, ApplyGaussianBlurToTexture, int rgbaWide, int rgbaTall, unsigned char *rgba, int iBlur)
	{
		if (rc_CFontTextureCache_GetTextureForChars_FG > 0) {
			DevMsg("ApplyGaussianBlurToTexture for FG: [rgbaWide:%d] [rgbaTall:%d] [rgba:%p] [iBlur:%d]\n", rgbaWide, rgbaTall, rgba, iBlur);
			return;
		}
		if (rc_CFontTextureCache_GetTextureForChars_BG > 0) {
			DevMsg("ApplyGaussianBlurToTexture for BG: [rgbaWide:%d] [rgbaTall:%d] [rgba:%p] [iBlur:%d]\n", rgbaWide, rgbaTall, rgba, iBlur);
			ExpandifyTexture(rgbaWide, rgbaTall, rgba, cvar_expand_bg.GetFloat());
		//	ExpandifyTextureFrac(rgbaWide, rgbaTall, rgba, cvar_expand_bg.GetFloat());
			return;
		}
		
		DETOUR_STATIC_CALL(ApplyGaussianBlurToTexture)(rgbaWide, rgbaTall, rgba, iBlur);
		
	#if 0
		if (rc_CFontTextureCache_GetTextureForChars_BG > 0) {
			DevMsg("ApplyGaussianBlurToTexture for BG: [rgbaWide:%d] [rgbaTall:%d] [rgba:%p] [iBlur:%d]\n",
				rgbaWide, rgbaTall, rgba, iBlur);
			
			float factor = cvar_darken_bg.GetFloat();
			
			for (int y = 0; y < rgbaTall; ++y) {
				for (int x = 0; x < rgbaWide; ++x) {
					uint8_t *texel = rgba + ((x + (y * rgbaWide)) * 4);
					
					float mult = (float)(texel[3]) * factor;
					texel[3] = (uint8_t)Clamp(mult, 0.0f, 255.0f);
				}
			}
		}
	#endif
	}
	
	
	ConVar cvar_bg_color_r("sig_util_debugoverlay_font_v2_bg_color_r", "0x00", FCVAR_NOTIFY,
		"Utility: BG font color: red");
	ConVar cvar_bg_color_g("sig_util_debugoverlay_font_v2_bg_color_g", "0x00", FCVAR_NOTIFY,
		"Utility: BG font color: green");
	ConVar cvar_bg_color_b("sig_util_debugoverlay_font_v2_bg_color_b", "0x00", FCVAR_NOTIFY,
		"Utility: BG font color: blue");
	ConVar cvar_bg_color_a("sig_util_debugoverlay_font_v2_bg_color_a", "0xff", FCVAR_NOTIFY,
		"Utility: BG font color: alpha");
	
	
	RefCount rc_CDebugOverlay_Paint;
	DETOUR_DECL_MEMBER(void, CDebugOverlay_Paint)
	{
		SCOPED_INCREMENT(rc_CDebugOverlay_Paint);
		DETOUR_MEMBER_CALL(CDebugOverlay_Paint)();
	}
	
	DETOUR_DECL_MEMBER(int, CMatSystemSurface_DrawColoredText, vgui::HFont font, int x, int y, int r, int g, int b, int a, const char *fmt, va_list argptr)
	{
		if (rc_CDebugOverlay_Paint > 0) {
			UpdateDebugOverlayFonts();
			
			int bg_r = Clamp((int)std::strtol(cvar_bg_color_r.GetString(), nullptr, 0), 0x00, 0xff);
			int bg_g = Clamp((int)std::strtol(cvar_bg_color_g.GetString(), nullptr, 0), 0x00, 0xff);
			int bg_b = Clamp((int)std::strtol(cvar_bg_color_b.GetString(), nullptr, 0), 0x00, 0xff);
			int bg_a = Clamp((int)std::strtol(cvar_bg_color_a.GetString(), nullptr, 0), 0x00, 0xff);
			
			int rBG = DETOUR_MEMBER_CALL(CMatSystemSurface_DrawColoredText)(hFontBG, x, y, bg_r, bg_g, bg_b, bg_a, fmt, argptr);
			int rFG = DETOUR_MEMBER_CALL(CMatSystemSurface_DrawColoredText)(hFontFG, x, y,    r,     g,   b,    a, fmt, argptr);
			
			return std::max(rBG, rFG);
		}
		
		return DETOUR_MEMBER_CALL(CMatSystemSurface_DrawColoredText)(font, x, y, r, g, b, a, fmt, argptr);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:DebugOverlay_Font_v2")
		{
			MOD_ADD_DETOUR_MEMBER(CFontTextureCache_GetTextureForChars, "[client] CFontTextureCache::GetTextureForChars");
		//	MOD_ADD_DETOUR_STATIC(ApplyOutlineToTexture,                "[client] ApplyOutlineToTexture");
			MOD_ADD_DETOUR_STATIC(ApplyGaussianBlurToTexture,           "[client] ApplyGaussianBlurToTexture");
			
			MOD_ADD_DETOUR_MEMBER(CDebugOverlay_Paint,               "[client] CDebugOverlay::Paint");
			MOD_ADD_DETOUR_MEMBER(CMatSystemSurface_DrawColoredText, "[client] CMatSystemSurface::DrawColoredText [va_list]");
		}
		
		virtual bool OnLoad() override
		{
			if (g_pVGuiSurface == nullptr) return false;
			
			return true;
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_debugoverlay_font_v2", "0", FCVAR_NOTIFY,
		"Utility: allow overriding the debug overlay font",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
