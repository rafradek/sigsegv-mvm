#include "mod.h"


// this is a rebalance mod based on some ideas from these forum posts:
// https://steamcommunity.com/app/440/discussions/1/618463738382900637/?ctp=860#c1639788130265953000
// https://steamcommunity.com/app/440/discussions/1/618463738382900637/?ctp=861#c1639788130267750449


namespace Mod::MvM::NonExplosive_Headshot
{
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:NonExplosive_Headshot")
		{
			// TODO: selectively modify the mult_damage attr amount granted by the upgrade station to sniper rifle weapons
			// TODO: mod out the existing explosive_sniper_shot code
			// TODO: mod in new code to apply a damage_all_connected style effect on headshots
			
			// TODO: figure out balance... ideas?
			// explosive_sniper_shot 0 ==> no Third Degree effect
			// explosive_sniper_shot 1 ==> Third Degree effect on 100% charged headshots
			// explosive_sniper_shot 2 ==> Third Degree effect on >=50% charged headshots
			// explosive_sniper_shot 3 ==> Third Degree effect on all headshots
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_nonexplosive_headshot", "0", FCVAR_NOTIFY,
		"Mod: rebalance sniper with doubled damage per upgrade and Third Degree effect on headshots rather than EH",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
