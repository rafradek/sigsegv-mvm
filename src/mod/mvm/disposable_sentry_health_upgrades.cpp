#include "mod.h"
#include "stub/objects.h"
#include "stub/tf_shareddefs.h"
#include "util/scope.h"


namespace Mod::MvM::Disposable_Sentry_Health_Upgrades
{
	/* undo the modification of m_iMaxHealth in CObjectSentrygun::MakeDisposableBuilding */
	RefCount rc_CObjectSentrygun_MakeDisposableBuilding;
	DETOUR_DECL_MEMBER(void, CObjectSentrygun_MakeDisposableBuilding, CTFPlayer *owner)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
		
		int max_health = sentry->GetMaxHealth();
		
		SCOPED_INCREMENT(rc_CObjectSentrygun_MakeDisposableBuilding);
		DETOUR_MEMBER_CALL(owner);
		
		sentry->SetMaxHealth(max_health);
	}
	
	/* disallow the call to CBaseObject::SetHealth in CObjectSentrygun::MakeDisposableBuilding */
	DETOUR_DECL_MEMBER(void, CBaseObject_SetHealth, float amt)
	{
		auto obj = reinterpret_cast<CBaseObject *>(this);
		
		if (rc_CObjectSentrygun_MakeDisposableBuilding > 0 && obj->GetType() == OBJ_SENTRYGUN && amt == 100.0f) {
			/* "nope" */
			return;
		}
		
		DETOUR_MEMBER_CALL(amt);
	}
	
	
	/* lie about being a disposable building when its max health is computed */
	DETOUR_DECL_MEMBER(int, CBaseObject_GetMaxHealthForCurrentLevel)
	{
		auto obj = reinterpret_cast<CBaseObject *>(this);
		
		bool m_bDisposableBuilding = obj->m_bDisposableBuilding;
		obj->m_bDisposableBuilding = false;
		int ret = DETOUR_MEMBER_CALL();
		obj->m_bDisposableBuilding = m_bDisposableBuilding;
		
		return ret;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Disposable_Sentry_Health_Upgrades")
		{
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_MakeDisposableBuilding, "CObjectSentrygun::MakeDisposableBuilding");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_SetHealth,                   "CBaseObject::SetHealth");
			
			MOD_ADD_DETOUR_MEMBER(CBaseObject_GetMaxHealthForCurrentLevel, "CBaseObject::GetMaxHealthForCurrentLevel");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_disposable_sentry_health_upgrades", "0", FCVAR_NOTIFY,
		"Mod: make building health upgrades apply to disposable sentries",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


/* DIFFERENCES BETWEEN MINI-SENTRY AND DISPOSABLE SENTRY


                   m_bMiniBuilding  m_bDisposableBuilding
      Mini-Sentry  1                0
Disposable Sentry  1                1


MODEL SCALE
===========
M: 0.75
D: 0.65
(SEE: CObjectSentrygun::MakeMiniBuilding, CObjectSentrygun::MakeDisposableBuilding)

BASE HEALTH
===========
M: 100
D: 100
(SEE: CBaseObject::GetMaxHealthForCurrentLevel)

STARTING HEALTH RATIO
=====================
M:  50%
D: 100%
(SEE: code near calls to CBaseObject::SetHealth in CBaseObject::StartBuilding)

FIRING INTERVAL COMPARED TO REGULAR SENTRY
==========================================
M:  75%
D: 100%
(SEE: CObjectSentrygun::Attack)

DETONATES WHEN OUT OF AMMO
==========================
M: NO
D: YES
(SEE: CObjectSentrygun::Fire)

CAN BE HEALED WITH WRENCH
=========================
M: YES
D: NO
(SEE: CObjectSentrygun::OnWrenchHit, CBaseObject::CanBeRepaired)

CAN BE HEALED BY RESCUE RANGER
==============================
M: YES
D: YES
(SEE: CTFProjectile_Arrow::BuildingHealingArrow; this is a bug, and that code really ought to call CanBeRepaired)

AFFECTED BY ATTR: mult_engy_building_health
===========================================
M: YES
D: NO
(SEE: CBaseObject::GetMaxHealthForCurrentLevel)

AFFECTED BY ATTR: mult_sentry_range
===================================
M: YES
D: NO
(SEE: CObjectSentrygun::SentryThink)

AFFECTED BY ATTR: mult_sentry_firerate
======================================
M: YES
D: YES
(SEE: CObjectSentrygun::Attack)

*/
