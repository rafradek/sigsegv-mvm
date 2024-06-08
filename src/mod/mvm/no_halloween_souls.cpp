#include "mod.h"
#include "stub/gamerules.h"


namespace Mod::MvM::No_Halloween_Souls
{
	DETOUR_DECL_MEMBER(void, CTFGameRules_DropHalloweenSoulPack, int i1, const Vector& vec1, CBaseEntity *ent1, int i2)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			/* don't spawn "halloween_souls_pack" entities in MvM mode */
			return;
		}
		
		DETOUR_MEMBER_CALL(i1, vec1, ent1, i2);
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
