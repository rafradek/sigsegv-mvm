#include "mod.h"
#include "stub/nav.h"
#include "stub/tfentities.h"
#include "stub/gamerules.h"
#include "stub/tf_objective_resource.h"
#include "util/scope.h"


namespace Mod::Credits::Spawn_AutoCollect
{
	/* which team is allowed to collect credits? */
	int GetCreditTeamNum()
	{
		/* HACK: make behavior consistent with another mod */
		static ConVarRef sig_mvm_set_credit_team("sig_mvm_set_credit_team");
		if (sig_mvm_set_credit_team.IsValid() && sig_mvm_set_credit_team.GetBool()) {
			return sig_mvm_set_credit_team.GetInt();
		}
		
		return TF_TEAM_RED;
	}
	
	
	DETOUR_DECL_MEMBER(void, CCurrencyPack_ComeToRest)
	{
		auto pack = reinterpret_cast<CCurrencyPack *>(this);
		
		DETOUR_MEMBER_CALL();
		
		if (!pack->IsMarkedForDeletion() && !pack->IsBeingPulled() && !pack->IsDistributed()) {
			int credit_team = GetCreditTeamNum();
			if (credit_team == TF_TEAM_RED || credit_team == TF_TEAM_BLUE) {
				TFNavAttributeType spawn_attr = ((credit_team == TF_TEAM_RED) ? BLUE_SPAWN_ROOM : RED_SPAWN_ROOM);
				
				auto area = reinterpret_cast<CTFNavArea *>(TheNavMesh->GetNavArea(pack->GetAbsOrigin()));
				if (area != nullptr && area->HasTFAttributes(spawn_attr)) {
					DevMsg("Non-distributed CCurrencyPack (#%d / $%d) landed in spawn room nav area; distributing now.\n",
						ENTINDEX(pack), pack->GetAmount());
					
					pack->SetDistributed(true);
					
					TFGameRules()->DistributeCurrencyAmount(pack->GetAmount(), nullptr, true, false, false);
					TFObjectiveResource()->m_nMvMWorldMoney -= pack->GetAmount();
					
					StopParticleEffects(pack);
					DispatchParticleEffect("mvm_cash_embers_red", PATTACH_ABSORIGIN_FOLLOW, pack);
				}
			}
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Credits:Spawn_AutoCollect")
		{
			MOD_ADD_DETOUR_MEMBER(CCurrencyPack_ComeToRest, "CCurrencyPack::ComeToRest");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_credits_spawn_autocollect", "0", FCVAR_NOTIFY,
		"Mod: auto-collect credits that land in the bots' spawn area",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
