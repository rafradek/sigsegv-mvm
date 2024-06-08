#include "mod.h"
#include "stub/tfentities.h"


namespace Mod::Sound::Tele_Activate
{
	DETOUR_DECL_MEMBER(void, CTFBotHintEngineerNest_HintTeleporterThink)
	{
		auto hint = reinterpret_cast<CTFBotHintEngineerNest *>(this);
		
		bool active_pre = hint->m_bHasActiveTeleporter;
		DETOUR_MEMBER_CALL();
		bool active_post = hint->m_bHasActiveTeleporter;
		
		if (!active_pre && active_post) {
			enginesound->PrecacheSound(")mvm/mvm_tele_activate.wav", true);
		//	DevMsg("Teleporter going active!\n");
			hint->EmitSound("MVM.Robot_Teleporter_Activate");
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Sound:Tele_Activate")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotHintEngineerNest_HintTeleporterThink, "CTFBotHintEngineerNest::HintTeleporterThink");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_sound_tele_activate", "0", FCVAR_NOTIFY,
		"Mod: enable the engiebot teleporter activation sound effect",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
