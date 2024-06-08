#include "mod.h"
#include "stub/gamerules.h"


namespace Mod::MvM::PlayerAttributeOverride
{
    std::map<int,std::string> indexmap;
    std::map<std::string,std::string> classnamemap;

	DETOUR_DECL_MEMBER(CBaseEntity *, CTFPlayer_GiveNamedItem, const char *classname, int i1, const CEconItemView *item_view, bool b1)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
        CBaseEntity *entity = DETOUR_MEMBER_CALL(classname, i1, item_view, b1);
		if (entity != nullptr && TFGameRules()->IsMannVsMachineMode() && player->GetTeamNumber() == TF_TEAM_RED) {
            
        }
		return entity;
    }
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:No_Halloween_Souls")
		{
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_DropHalloweenSoulPack, "CTFGameRules::DropHalloweenSoulPack");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_no_halloween_souls", "0", FCVAR_NOTIFY,
		"Mod: disable those stupid Halloween soul drop things in MvM mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}