#include "mod.h"
#include "stub/tfplayer.h"
#include "util/iterate.h"
#include "util/clientmsg.h"


namespace Mod::Debug::Medigun_Speed
{
	ConVar cvar_filter_name("sig_debug_medigun_speed_filter_name", "", FCVAR_NOTIFY,
		"Debug: list of strings against which to filter player names");
	ConVar cvar_filter_class("sig_debug_medigun_speed_filter_class", "", FCVAR_NOTIFY,
		"Debug: list of integers against which to filter player classes");
	
	
	bool PlayerMatchesFilter(CTFPlayer *player)
	{
		if (strlen(cvar_filter_name.GetString()) != 0) {
			if (V_stristr(cvar_filter_name.GetString(), player->GetPlayerName()) == nullptr) {
				return false;
			}
		}
		
		const char *class_str = "";
		switch (player->GetPlayerClass()->GetClassIndex()) {
		case TF_CLASS_SCOUT:        class_str = "scout";        break;
		case TF_CLASS_SOLDIER:      class_str = "soldier";      break;
		case TF_CLASS_PYRO:         class_str = "pyro";         break;
		case TF_CLASS_DEMOMAN:      class_str = "demoman";      break;
		case TF_CLASS_HEAVYWEAPONS: class_str = "heavyweapons"; break;
		case TF_CLASS_ENGINEER:     class_str = "engineer";     break;
		case TF_CLASS_MEDIC:        class_str = "medic";        break;
		case TF_CLASS_SNIPER:       class_str = "sniper";       break;
		case TF_CLASS_SPY:          class_str = "spy";          break;
		}
		
		if (strlen(cvar_filter_class.GetString()) != 0) {
			if (V_stristr(cvar_filter_class.GetString(), class_str) == nullptr) {
				return false;
			}
		}
		
		return true;
	}
	
	
	struct SpeedData
	{
		float TeamFortress_CalculateMaxSpeed = NAN;
		float MaxSpeed                       = NAN;
		float GetPlayerMaxSpeed              = NAN;
	};
	std::unordered_map<int, SpeedData> s_PlayerSpeeds;
	
	
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Medigun_Speed") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			ForEachTFPlayer([](CTFPlayer *player){
				SpeedData spOld = s_PlayerSpeeds[ENTINDEX(player)];
				
				SpeedData spNew = {
					player->TeamFortress_CalculateMaxSpeed(),
					player->MaxSpeed(),
					player->GetPlayerMaxSpeed(),
				};
				
				if (PlayerMatchesFilter(player)) {
					if (spOld.TeamFortress_CalculateMaxSpeed != spNew.TeamFortress_CalculateMaxSpeed) {
						ClientMsgAll("[%8.3f] %s: %30s: %6.1f -> %6.1f\n",
							gpGlobals->curtime, player->GetPlayerName(), "TeamFortress_CalculateMaxSpeed",
							spOld.TeamFortress_CalculateMaxSpeed, spNew.TeamFortress_CalculateMaxSpeed);
					}
					
					if (spOld.MaxSpeed != spNew.MaxSpeed) {
						ClientMsgAll("[%8.3f] %s: %30s: %6.1f -> %6.1f\n",
							gpGlobals->curtime, player->GetPlayerName(), "MaxSpeed",
							spOld.MaxSpeed, spNew.MaxSpeed);
					}
					
					if (spOld.GetPlayerMaxSpeed != spNew.GetPlayerMaxSpeed) {
						ClientMsgAll("[%8.3f] %s: %30s: %6.1f -> %6.1f\n",
							gpGlobals->curtime, player->GetPlayerName(), "GetPlayerMaxSpeed",
							spOld.GetPlayerMaxSpeed, spNew.GetPlayerMaxSpeed);
					}
				}
				
				s_PlayerSpeeds[ENTINDEX(player)] = spNew;
			});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_medigun_speed", "0", FCVAR_NOTIFY,
		"Debug: investigate bugginess of medigun movement speed matching",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
