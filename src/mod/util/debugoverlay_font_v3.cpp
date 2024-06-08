#include "mod.h"
#include "util/scope.h"
#include "stub/fonts.h"


#if defined _WINDOWS

namespace Mod::Util::DebugOverlay_Font_v3
{
	ConVar cvar_spew("sig_util_debugoverlay_font_v3_spew", "0", FCVAR_NOTIFY,
		"Utility: spew debugging information");
	
	
	bool bFontDirty = false;
	void SetFontDirty(IConVar *pConVar, const char *pOldValue, float flOldValue) { bFontDirty = true; }
	
	ConVar cvar_name("sig_util_debugoverlay_font_v3_name", "Consolas", FCVAR_NOTIFY,
		"Utility: font name", &SetFontDirty);
	ConVar cvar_tall("sig_util_debugoverlay_font_v3_tall", "14", FCVAR_NOTIFY,
		"Utility: font tall", &SetFontDirty);
	ConVar cvar_weight("sig_util_debugoverlay_font_v3_weight", "400", FCVAR_NOTIFY,
		"Utility: font weight", &SetFontDirty);
	ConVar cvar_scanlines("sig_util_debugoverlay_font_v3_scanlines", "0", FCVAR_NOTIFY,
		"Utility: font scanlines", &SetFontDirty);
	ConVar cvar_blur("sig_util_debugoverlay_font_v3_blur", "0", FCVAR_NOTIFY,
		"Utility: font blur", &SetFontDirty);
	ConVar cvar_italic("sig_util_debugoverlay_font_v3_italic", "0", FCVAR_NOTIFY,
		"Utility: font italic", &SetFontDirty);
	ConVar cvar_underline("sig_util_debugoverlay_font_v3_underline", "0", FCVAR_NOTIFY,
		"Utility: font underline", &SetFontDirty);
	ConVar cvar_strikeout("sig_util_debugoverlay_font_v3_strikeout", "0", FCVAR_NOTIFY,
		"Utility: font strikeout", &SetFontDirty);
	ConVar cvar_antialias("sig_util_debugoverlay_font_v3_antialias", "0", FCVAR_NOTIFY,
		"Utility: font antialias", &SetFontDirty);
	ConVar cvar_dropshadow("sig_util_debugoverlay_font_v3_dropshadow", "0", FCVAR_NOTIFY,
		"Utility: font dropshadow", &SetFontDirty);
	ConVar cvar_outline("sig_util_debugoverlay_font_v3_outline", "0", FCVAR_NOTIFY,
		"Utility: font outline", &SetFontDirty);
	ConVar cvar_box_outline("sig_util_debugoverlay_font_v3_box_outline", "0", FCVAR_NOTIFY,
		"Utility: use box algorithm instead of circle for outline", &SetFontDirty);
	
	
	vgui::HFont hFont = vgui::INVALID_FONT;
	
	void UpdateDebugOverlayFont()
	{
		if (hFont == vgui::INVALID_FONT || bFontDirty) {
			DevMsg("UpdateDebugOverlayFont: regenerating font\n");
			
			hFont = g_pVGuiSurface->CreateFont();
			
			const char *name = cvar_name.GetString();
			int tall         = cvar_tall.GetInt();
			int weight       = cvar_weight.GetInt();
			int blur         = cvar_blur.GetInt();
			int scanlines    = cvar_scanlines.GetInt();
			
			int flags = vgui::ISurface::FONTFLAG_NONE;
			if (cvar_italic    .GetBool()) flags |= vgui::ISurface::FONTFLAG_ITALIC;
			if (cvar_underline .GetBool()) flags |= vgui::ISurface::FONTFLAG_UNDERLINE;
			if (cvar_strikeout .GetBool()) flags |= vgui::ISurface::FONTFLAG_STRIKEOUT;
			if (cvar_antialias .GetBool()) flags |= vgui::ISurface::FONTFLAG_ANTIALIAS;
			if (cvar_dropshadow.GetBool()) flags |= vgui::ISurface::FONTFLAG_DROPSHADOW;
			if (cvar_outline   .GetBool()) flags |= vgui::ISurface::FONTFLAG_OUTLINE;
			
			bool bOK = g_pVGuiSurface->SetFontGlyphSet(hFont, name, tall, weight, blur, scanlines, flags);
			DevMsg("UpdateDebugOverlayFont: %-3s %08lx\n", (bOK ? "OK" : "BAD"), hFont);
			
			bFontDirty = false;
		}
	}
	
	
	RefCount rc_ISurface_SetFontGlyphSet;
	DETOUR_DECL_MEMBER(bool, ISurface_SetFontGlyphSet, vgui::HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
	{
		SCOPED_INCREMENT_IF(rc_ISurface_SetFontGlyphSet, font == hFont);
		return DETOUR_MEMBER_CALL(font, windowsFontName, tall, weight, blur, scanlines, flags);
	}
	
	DETOUR_DECL_MEMBER(bool, CWin32Font_Create, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
	{
		auto result = DETOUR_MEMBER_CALL(windowsFontName, tall, weight, blur, scanlines, flags);
		
		if (result && rc_ISurface_SetFontGlyphSet > 0) {
			auto font = reinterpret_cast<CWin32Font *>(this);
			
			if (font->m_hDIB != nullptr) {
				::DeleteObject(font->m_hDIB);
				font->m_hDIB = nullptr;
			}
			
			::TEXTMETRIC tm;
			memset(&tm, 0x00, sizeof(tm));
			
			if (!::GetTextMetrics(font->m_hDC, &tm)) {
				font->m_szName = UTL_INVAL_SYMBOL;
				return false;
			}
			
			int real_outline    = cvar_outline.GetInt();
			int real_dropshadow = cvar_dropshadow.GetInt();
			
			font->m_iHeight       = tm.tmHeight + real_dropshadow + (2 * real_outline);
			font->m_iMaxCharWidth = tm.tmMaxCharWidth;
			font->m_iAscent       = tm.tmAscent;
			
			font->m_rgiBitmapSize[0] = tm.tmMaxCharWidth + real_dropshadow + (2 * real_outline);
			font->m_rgiBitmapSize[1] = tm.tmHeight       + real_dropshadow + (2 * real_outline);
			
			::BITMAPINFOHEADER header;
			memset(&header, 0x00, sizeof(header));
			header.biSize        = sizeof(header);
			header.biWidth       = font->m_rgiBitmapSize[0];
			header.biHeight      = -font->m_rgiBitmapSize[1];
			header.biPlanes      = 1;
			header.biBitCount    = 32;
			header.biCompression = BI_RGB;
			
			font->m_hDIB = ::CreateDIBSection(font->m_hDC, (BITMAPINFO *)&header, DIB_RGB_COLORS, (void **)&font->m_pBuf, nullptr, 0);
			::SelectObject(font->m_hDC, font->m_hDIB);
		}
		
		return result;
	}
	
	#if 0
	bool g_bFontFound = false;
	int CALLBACK FontEnumProc( 
		const LOGFONT *lpelfe,		// logical-font data
		const TEXTMETRIC *lpntme,	// physical-font data
		DWORD FontType,				// type of font
		LPARAM lParam )				// application-defined data
	{
		g_bFontFound = true;
		return 0;
	}
	DETOUR_DECL_MEMBER(bool, CWin32Font_Create, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags)
	{
		if (rc_CFontManager_SetFontGlyphSet <= 0) {
			return DETOUR_MEMBER_CALL(windowsFontName, tall, weight, blur, scanlines, flags);
		}
		
		auto font = reinterpret_cast<CWin32Font *>(this);
		
		font->m_szName     = windowsFontName;
		font->m_iTall      = tall;
		font->m_iWeight    = weight;
		font->m_iFlags     = flags;
		font->m_iScanLines = scanlines;
		font->m_iBlur      = blur;
		
		font->m_bUnderlined       = ((flags & vgui::ISurface::FONTFLAG_UNDERLINE ) ? 1 : 0);
		font->m_iDropShadowOffset = ((flags & vgui::ISurface::FONTFLAG_DROPSHADOW) ? 1 : 0);
		font->m_bAntialiased      = ((flags & vgui::ISurface::FONTFLAG_ANTIALIAS ) ? 1 : 0);
		font->m_iOutlineSize      = ((flags & vgui::ISurface::FONTFLAG_OUTLINE   ) ? 1 : 0);
		font->m_bRotary           = ((flags & vgui::ISurface::FONTFLAG_ROTARY    ) ? 1 : 0);
		font->m_bAdditive         = ((flags & vgui::ISurface::FONTFLAG_ADDITIVE  ) ? 1 : 0);
		
		int charset = (flags & vgui::ISurface::FONTFLAG_SYMBOL) ? SYMBOL_CHARSET : ANSI_CHARSET;
		
		font->m_hDC = ::CreateCompatibleDC(nullptr);
		
		LOGFONT logfont;
		logfont.lfCharSet = DEFAULT_CHARSET;
		logfont.lfPitchAndFamily = 0;
		V_strcpy_safe(logfont.lfFaceName, m_szName.String());
		g_bFontFound = false;
		::EnumFontFamiliesEx(font->m_hDC, &logfont, &FontEnumProc, 0, 0);
		if (!g_bFontFound) {
			font->m_szName = UTL_INVAL_SYMBOL;
			return false;
		}
		
		
		
		// TODO: g_bFontFound
		
		// TODO
	}
	#endif
	
	
#define RGBA_COORD(ptr, x, y) ((ptr) + (((x) + ((y) * rgbaWide)) * 4))
	
	void ExpandifyTexture(int rgbaWide, int rgbaTall, uint8_t *rgba, int radius)
	{
		static std::vector<uint8_t> buf;
		
		size_t size = (rgbaWide * rgbaTall * 4);
		buf.resize(size);
		
		uint8_t *tmp = buf.data();
		
		int radius_sqr = Square(radius);
		bool box_mode = cvar_box_outline.GetBool();
		
		for (int y = 0; y < rgbaTall; ++y) {
			for (int x = 0; x < rgbaWide; ++x) {
				const uint8_t *src = RGBA_COORD(rgba, x, y);
				      uint8_t *dst = RGBA_COORD(tmp, x, y);
				
				uint8_t max_alpha = 0x00;
				for (int dy = -radius; dy <= radius && max_alpha < 0xff; ++dy) {
					for (int dx = -radius; dx <= radius && max_alpha < 0xff; ++dx) {
						if (x + dx < 0 || x + dx >= rgbaWide)                  continue;
						if (y + dy < 0 || y + dy >= rgbaTall)                  continue;
						if (!box_mode && Square(dx) + Square(dy) > radius_sqr) continue;
						
						const uint8_t *test = RGBA_COORD(rgba, x + dx, y + dy);
						max_alpha = std::max(max_alpha, test[3]);
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
		
		memcpy(rgba, tmp, size);
	}
	
	
	RefCount rc_CFontTextureCache_GetTextureForChars;
	DETOUR_DECL_MEMBER(bool, CFontTextureCache_GetTextureForChars, vgui::HFont font, vgui::FontDrawType_t type, wchar_t *wch, int *textureID, float **texCoords, int numChars)
	{
		SCOPED_INCREMENT_IF(rc_CFontTextureCache_GetTextureForChars, font == hFont);
		
		return DETOUR_MEMBER_CALL(font, type, wch, textureID, texCoords, numChars);
	}
	
	DETOUR_DECL_STATIC(void, ApplyOutlineToTexture, int rgbaWide, int rgbaTall, unsigned char *rgba, int iOutlineSize)
	{
		if (rc_CFontTextureCache_GetTextureForChars > 0) {
			DevMsg("ApplyOutlineToTexture: [rgbaWide:%d] [rgbaTall:%d] [rgba:%p] [iOutlineSize:%d]\n", rgbaWide, rgbaTall, rgba, iOutlineSize);
			ExpandifyTexture(rgbaWide, rgbaTall, rgba, cvar_outline.GetInt());
			return;
		}
		
		DETOUR_STATIC_CALL(rgbaWide, rgbaTall, rgba, iOutlineSize);
	}
	
	
	RefCount rc_CDebugOverlay_Paint;
	DETOUR_DECL_MEMBER(void, CDebugOverlay_Paint)
	{
		SCOPED_INCREMENT(rc_CDebugOverlay_Paint);
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(int, CMatSystemSurface_DrawColoredText, vgui::HFont font, int x, int y, int r, int g, int b, int a, const char *fmt, va_list argptr)
	{
		if (rc_CDebugOverlay_Paint > 0) {
			UpdateDebugOverlayFont();
			return DETOUR_MEMBER_CALL(hFont, x, y, 0x00, 0x00, 0x00, 0xff, fmt, argptr);
		}
		
		return DETOUR_MEMBER_CALL(font, x, y, r, g, b, a, fmt, argptr);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:DebugOverlay_Font_v3")
		{
			MOD_ADD_DETOUR_MEMBER(ISurface_SetFontGlyphSet, "ISurface::SetFontGlyphSet");
			MOD_ADD_DETOUR_MEMBER(CWin32Font_Create,        "[client] CWin32Font::Create");
			
			MOD_ADD_DETOUR_MEMBER(CFontTextureCache_GetTextureForChars, "[client] CFontTextureCache::GetTextureForChars");
			MOD_ADD_DETOUR_STATIC(ApplyOutlineToTexture,                "[client] ApplyOutlineToTexture");
			
			MOD_ADD_DETOUR_MEMBER(CDebugOverlay_Paint,               "[client] CDebugOverlay::Paint");
			MOD_ADD_DETOUR_MEMBER(CMatSystemSurface_DrawColoredText, "[client] CMatSystemSurface::DrawColoredText [va_list]");
		}
		
		virtual bool OnLoad() override
		{
			if (g_pVGuiSurface == nullptr) return false;
			
			return true;
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_debugoverlay_font_v3", "0", FCVAR_NOTIFY,
		"Utility: allow overriding the debug overlay font",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#endif
