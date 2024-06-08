#include "mod.h"
#include "util/scope.h"


namespace Mod::Util::DebugOverlay_Font
{
	RefCount rc_CDebugOverlay_Paint;
	vgui::HScheme hScheme = 0;
	
	
	ConVar cvar_name("sig_util_debugoverlay_font_name", "", FCVAR_NOTIFY,
		"Utility: font name from the scheme file");
	
	
	void MaybeOverrideFont(vgui::HFont *pFont)
	{
		if (rc_CDebugOverlay_Paint <= 0) return;
		
		const char *font_name = cvar_name.GetString();
		if (strlen(font_name) == 0) return;
		
		if (g_pVGuiSchemeManager == nullptr) {
			Warning("DebugOverlay font override: g_pVGuiSchemeManager is nullptr!\n");
			return;
		}
		
		vgui::IScheme *scheme = g_pVGuiSchemeManager->GetIScheme(hScheme);
		if (scheme == nullptr) {
			Warning("DebugOverlay font override: GetIScheme failed!\n");
			return;
		}
		
		vgui::HFont font = scheme->GetFont(font_name);
		if (font == vgui::INVALID_FONT) {
			Warning("DebugOverlay font override: Couldn't find font named \"%s\"!\n", font_name);
			return;
		}
		
		*pFont = font;
	}
	
	
	DETOUR_DECL_MEMBER(void, CDebugOverlay_Paint)
	{
		SCOPED_INCREMENT(rc_CDebugOverlay_Paint);
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(int, CMatSystemSurface_DrawColoredText, vgui::HFont font, int x, int y, int r, int g, int b, int a, const char *fmt, va_list argptr)
	{
		MaybeOverrideFont(&font);
		
		return DETOUR_MEMBER_CALL(font, x, y, r, g, b, a, fmt, argptr);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:DebugOverlay_Font")
		{
			MOD_ADD_DETOUR_MEMBER(CDebugOverlay_Paint,               "[client] CDebugOverlay::Paint");
			MOD_ADD_DETOUR_MEMBER(CMatSystemSurface_DrawColoredText, "[client] CMatSystemSurface::DrawColoredText [va_list]");
		}
		
		virtual void OnEnable() override
		{
			if (g_pVGuiSchemeManager != nullptr) {
				KeyValuesSystem()->InvalidateCache();
				
				hScheme = g_pVGuiSchemeManager->LoadSchemeFromFile("resource/debugoverlay_fonts.res", "DebugOverlayFonts");
				DevMsg("hScheme = %08lx\n", hScheme);
			}
		}
		
		virtual void OnDisable() override
		{
			if (g_pVGuiSchemeManager != nullptr) {
				// make our next LoadSchemeFromFile actually load the scheme from file
				// by making the filename not match up
				
				vgui::HScheme handle;
				while ((handle = g_pVGuiSchemeManager->GetScheme("DebugOverlayFonts")) != 1) {
					vgui::IScheme *scheme = g_pVGuiSchemeManager->GetIScheme(handle);
					if (scheme == nullptr) continue;
					
					auto fileName = reinterpret_cast<char *>((uintptr_t)scheme + 0x4);
					auto tag = reinterpret_cast<char *>((uintptr_t)scheme + 0x104);
					
					DevMsg("Wiping out scheme #%lu (\"%s\" \"%s\")\n", handle, tag, fileName);
					
					fileName[0] = '\0';
					tag[0]      = '\0';
				}
			}
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_debugoverlay_font", "0", FCVAR_NOTIFY,
		"Utility: allow overriding the debug overlay font",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_ISchemeManager_LoadSchemeFromFile, "Call ISchemeManager::LoadSchemeFromFile(<fileName>, <tag>)")
	{
		if (g_pVGuiSchemeManager == nullptr) {
			Warning("g_pVGuiSchemeManager is nullptr!\n");
			return;
		}
		
		if (args.ArgC() != 3) {
			Warning("Expected 2 arguments.\n");
			return;
		}
		
		vgui::HScheme handle = g_pVGuiSchemeManager->LoadSchemeFromFile(args[1], args[2]);
		
		Msg("LoadSchemeFromFile returned the following handle: %08lx\n", handle);
	}
	
	
	CON_COMMAND(sig_ISchemeManager_ReloadSchemes, "Call ISchemeManager::ReloadSchemes()")
	{
		if (g_pVGuiSchemeManager == nullptr) {
			Warning("g_pVGuiSchemeManager is nullptr!\n");
			return;
		}
		
		if (args.ArgC() != 1) {
			Warning("Expected 0 arguments.\n");
			return;
		}
		
		g_pVGuiSchemeManager->ReloadSchemes();
	}
	
	
	CON_COMMAND(sig_ISchemeManager_ReloadFonts, "Call ISchemeManager::ReloadFonts()")
	{
		if (g_pVGuiSchemeManager == nullptr) {
			Warning("g_pVGuiSchemeManager is nullptr!\n");
			return;
		}
		
		if (args.ArgC() != 1) {
			Warning("Expected 0 arguments.\n");
			return;
		}
		
		g_pVGuiSchemeManager->ReloadFonts();
	}
	
	
	CON_COMMAND(sig_ISchemeManager_show_font_info, "Show font information for a scheme (by tag)")
	{
		if (g_pVGuiSchemeManager == nullptr) {
			Warning("g_pVGuiSchemeManager is nullptr!\n");
			return;
		}
		if (g_pVGuiSurface == nullptr) {
			Warning("g_pVGuiSurface is nullptr!\n");
			return;
		}
		
		if (args.ArgC() != 2) {
			Warning("Expected 1 argument.\n");
			return;
		}
		
		vgui::HScheme handle = g_pVGuiSchemeManager->GetScheme(args[1]);
		if (handle == 1) {
			Warning("Warning: ISchemeManager::GetScheme(\"%s\") returned the default scheme handle!\n", args[1]);
		}
		
		vgui::IScheme *scheme = g_pVGuiSchemeManager->GetIScheme(handle);
		if (scheme == nullptr) {
			Warning("ISchemeManager::GetIScheme returned nullptr!\n");
			return;
		}
		
		int num_fonts = scheme->GetFontCount();
		Msg("%d fonts:\n", num_fonts);
		for (int i = 0; i < num_fonts; ++i) {
			vgui::HFont font = scheme->GetFontAtIndex(i);
			if (font == vgui::INVALID_FONT) {
				Warning("- invalid font!\n");
				continue;
			}
			
			// TODO
		}
	}
}
