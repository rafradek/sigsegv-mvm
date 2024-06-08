#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/tfentities.h"
#include "util/rtti.h"


namespace Mod::Canteen::Share_Recall_Canteen
{
	RefCount rc_CTFPowerupBottle_ReapplyProvision;
	
	
	bool got_attr;
	int canteen_specialist;
	DETOUR_DECL_STATIC(int, CAttributeManager_AttribHookValue_int, int value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
		auto result = DETOUR_STATIC_CALL(value, attr, ent, vec, b1);
		
		if (rc_CTFPowerupBottle_ReapplyProvision > 0 && !got_attr && value == 0 && V_stricmp(attr, "canteen_specialist") == 0) {
			canteen_specialist = result;
			
			got_attr = true;
		}
		
		return result;
	}
	
	bool got_cond;
	ETFCond cond;
	float duration;
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond, ETFCond nCond, float flDuration, CBaseEntity *pProvider)
	{
		if (rc_CTFPowerupBottle_ReapplyProvision > 0 && !got_cond && nCond == TF_COND_SPEED_BOOST) {
			cond     = nCond;
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
		
		CTFPlayer *patient = nullptr;
		if (medic != nullptr) {
			auto medigun = rtti_cast<CWeaponMedigun *>(medic->GetActiveWeapon());
			if (medigun != nullptr) {
				patient = ToTFPlayer(medigun->GetHealTarget());
			}
		}
		
		got_attr = false;
		got_cond = false;
		
		++rc_CTFPowerupBottle_ReapplyProvision;
		DETOUR_MEMBER_CALL();
		--rc_CTFPowerupBottle_ReapplyProvision;
		
		if (!got_attr || !got_cond)  return;
		if (canteen_specialist <= 0) return;
		if (medic == nullptr)        return;
		if (patient == nullptr)      return;
		
		patient->ForceRespawn();
		patient->m_Shared->AddCond(cond, duration);
		
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
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_int, "CAttributeManager::AttribHookValue<int>");
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
