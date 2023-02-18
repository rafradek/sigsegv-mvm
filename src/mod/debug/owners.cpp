#include "mod.h"
#include "stub/tfentities.h"
#include "stub/objects.h"
#include "stub/tfplayer.h"
#include "util/stringpool.h"
#include "util/iterate.h"


namespace Mod::Debug::Owners
{
	StringPool strpool(false);
	
	std::map<int, std::vector<std::pair<Color, const char *>>> ov_text;
	std::vector<std::pair<Vector, Vector>> ov_arrow;
	
	void GenerateOverlays(CBaseEntity *ent)
	{
		CBaseEntity *owner = ent->GetOwnerEntity();
		
		if (ov_text.find(ENTINDEX(ent)) == ov_text.end()) {
			const char *str = strpool(CFmtStr("ENTITY: #%d %s \"%s\"", ENTINDEX(ent), ent->GetClassname(), STRING(ent->GetEntityName())));
			ov_text[ENTINDEX(ent)].emplace_back(std::make_pair(Color(0xc0, 0xc0, 0xc0, 0xff), str));
		}
		if (owner != nullptr) {
			const char *str = strpool(CFmtStr(" OWNER: #%d %s \"%s\"", ENTINDEX(owner), owner->GetClassname(), STRING(owner->GetEntityName())));
			ov_text[ENTINDEX(ent)].emplace_back(std::make_pair(Color(0xff, 0xff, 0xff, 0xff), str));
		} else {
			const char *str = strpool(" OWNER: nullptr");
			ov_text[ENTINDEX(ent)].emplace_back(std::make_pair(Color(0x80, 0x80, 0x80, 0xff), str));
		}
		
		if (owner != nullptr && ov_text.find(ENTINDEX(owner)) == ov_text.end()) {
			const char *str = strpool(CFmtStr("ENTITY: #%d %s \"%s\"", ENTINDEX(owner), owner->GetClassname(), STRING(owner->GetEntityName())));
			ov_text[ENTINDEX(owner)].emplace_back(std::make_pair(Color(0xff, 0xff, 0xff, 0xff), str));
		}
		
		if (owner != nullptr) {
			ov_arrow.emplace_back(std::make_pair(ent->WorldSpaceCenter(), owner->WorldSpaceCenter()));
		}
	}
	
	void DrawOverlays()
	{
		for (const auto& pair : ov_text) {
			int entindex        = pair.first;
			const auto& strings = pair.second;
			
			for (size_t i = 0; i < strings.size(); ++i) {
				const Color& color = strings[i].first;
				const char *str    = strings[i].second;
				
				int r = color.r();
				int g = color.g();
				int b = color.b();
				int a = color.a();
				
				NDebugOverlay::EntityTextAtPosition(UTIL_EntityByIndex(entindex)->WorldSpaceCenter(), i + 1, str, 0.030, r, g, b, a);
			}
		}
		
		for (const auto& pair : ov_arrow) {
			const Vector& v_from = pair.first;
			const Vector& v_to   = pair.second;
			
			NDebugOverlay::HorzArrow(v_from, v_to, 2.0f, 0xc0, 0xc0, 0xc0, 0xff, true, 0.030f);
		}
		
		ov_text .clear();
		ov_arrow.clear();
	}
	
	
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Owners") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			static long frame = 0;
			if (frame++ % 2 == 0) return;
			
			ForEachEntity([](CBaseEntity *ent){
				if (ent->ClassMatches("bot_hint_sentrygun") ||
					ent->ClassMatches("bot_hint_teleporter_exit") ||
					ent->ClassMatches("bot_hint_engineer_nest") ||
					rtti_cast<CBaseObject *>(ent) != nullptr) {
					GenerateOverlays(ent);
				}
			});
			
			DrawOverlays();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_owners", "0", FCVAR_NOTIFY,
		"Debug: show info about the owner relationships between objects/hints/players",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
