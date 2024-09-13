#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/tfentities.h"
#include "util/rtti.h"


namespace Mod::Canteen::Share_Recall_Canteen
{
	RefCount rc_CTFPowerupBottle_ReapplyProvision;
	
	
	bool got_cond;
	ETFCond cond;
	float duration;
	CTFPlayer *patient = nullptr;
	int canteen_specialist = 0;
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond, ETFCond nCond, float flDuration, CBaseEntity *pProvider)
	{
		if (rc_CTFPowerupBottle_ReapplyProvision > 0 && !got_cond && nCond == TF_COND_SPEED_BOOST) {
			cond     = nCond;
			flDuration += canteen_specialist;
			duration = flDuration;
			
			got_cond = true;
		}
		
		DETOUR_MEMBER_CALL(nCond, flDuration, pProvider);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPowerupBottle_ReapplyProvision)
	{
		/* we have to get info about the patient up front, because if we are
		 * indeed using a recall canteen, the force-respawn will change our
		 * active weapon and therefore nullify our patient info by the time the
		 * call actually returns */
		auto canteen = reinterpret_cast<CTFPowerupBottle *>(this);
		CTFPlayer *medic = ToTFPlayer(canteen->GetOwnerEntity());
		
		patient = nullptr;
		canteen_specialist = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(medic, canteen_specialist, canteen_specialist);
		int iRecall = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(canteen, iRecall, recall);
		if (medic != nullptr) {
			auto medigun = rtti_cast<CWeaponMedigun *>(medic->GetActiveWeapon());
			if (medigun != nullptr) {
				patient = ToTFPlayer(medigun->GetHealTarget());
			}
		}
		
		got_cond = false;
		++rc_CTFPowerupBottle_ReapplyProvision;
		DETOUR_MEMBER_CALL();
		--rc_CTFPowerupBottle_ReapplyProvision;
		
		if (!got_cond) return;
		if (canteen_specialist <= 0) return;
		if (iRecall <= 0) return;
		if (medic == nullptr)        return;
		if (patient == nullptr)      return;
		
		patient->ForceRespawn();
		patient->m_Shared->AddCond(TF_COND_SPEED_BOOST, duration, medic);
		
		IGameEvent *event = gameeventmanager->CreateEvent("mvm_medic_powerup_shared");
		if (event != nullptr) {
			event->SetInt("player", ENTINDEX(medic));
			gameeventmanager->FireEvent(event);
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Canteen:Share_Recall_Canteen")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_AddCond,               "CTFPlayerShared::AddCond");
			MOD_ADD_DETOUR_MEMBER(CTFPowerupBottle_ReapplyProvision,     "CTFPowerupBottle::ReapplyProvision");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_canteen_share_recall_canteen", "0", FCVAR_NOTIFY,
		"Mod: allow Recall Canteens to be shared with the Canteen Specialist upgrade",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
