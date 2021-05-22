#include "mod.h"
#include "stub/baseentity.h"
#include "stub/projectiles.h"
#include "stub/objects.h"
#include "stub/entities.h"
#include "stub/gamerules.h"
#include "stub/nav.h"
#include "util/backtrace.h"
#include "tier1/CommandBuffer.h"


namespace Mod::Etc::Misc
{

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Rocket_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_Rocket *>(this);

		if (strcmp(ent->GetClassname(), "tf_projectile_balloffire") == 0) {
			return false;
		}

		return DETOUR_MEMBER_CALL(CTFProjectile_Rocket_IsDeflectable)();
	}

	bool AllowHit(CBaseEntity *proj, CBaseEntity *other)
	{
		bool penetrates = proj->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER);
		if (penetrates && !(other->entindex() == 0 || other->MyCombatCharacterPointer() != nullptr || other->IsCombatItem())) {
			Vector start = proj->GetAbsOrigin();
			Vector vel = proj->GetAbsVelocity();
			trace_t tr;
			UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_MONSTER|CONTENTS_SOLID, other, COLLISION_GROUP_NONE, &tr );
			if (tr.m_pEnt == nullptr) {
				return false;
			}
		}
		return true;
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_Arrow_ArrowTouch, CBaseEntity *pOther)
	{
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		
		if (AllowHit(arrow, pOther))
			DETOUR_MEMBER_CALL(CTFProjectile_Arrow_ArrowTouch)(pOther);
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_BallOfFire_RocketTouch, CBaseEntity *pOther)
	{
		auto arrow = reinterpret_cast<CBaseEntity *>(this);
		
		if (AllowHit(arrow, pOther))
			DETOUR_MEMBER_CALL(CTFProjectile_BallOfFire_RocketTouch)(pOther);
	}
	
	DETOUR_DECL_MEMBER(void, CTFBaseRocket_Explode, trace_t *pTrace, CBaseEntity *pOther)
	{
		auto proj = reinterpret_cast<CTFBaseRocket *>(this);

		auto owner = proj->GetOwnerEntity();
		if (owner != nullptr && owner->IsBaseObject()) {

			if (ToBaseObject(owner)->GetBuilder() == nullptr) {
				proj->SetOwnerEntity(GetContainingEntity(INDEXENT(0)));
			}
		}
		DETOUR_MEMBER_CALL(CTFBaseRocket_Explode)(pTrace, pOther);
	}
	
	RefCount rc_SendProxy_PlayerObjectList;
	bool playerobjectlist = false;
	DETOUR_DECL_STATIC(void, SendProxy_PlayerObjectList, const void *pProp, const void *pStruct, const void *pData, void *pOut, int iElement, int objectID)
	{
		SCOPED_INCREMENT(rc_SendProxy_PlayerObjectList);
		bool firstminisentry = true;
		CTFPlayer *player = (CTFPlayer *)(pStruct);
		for (int i = 0; i <= iElement; i++) {
			CBaseObject *obj = player->GetObject(i);
			if (obj != nullptr && obj->m_bDisposableBuilding) {
				if (!firstminisentry)
					iElement++;
				firstminisentry = false;
			}
		}
		DETOUR_STATIC_CALL(SendProxy_PlayerObjectList)(pProp, pStruct, pData, pOut, iElement, objectID);
	}

	RefCount rc_SendProxyArrayLength_PlayerObjects;
	DETOUR_DECL_STATIC(int, SendProxyArrayLength_PlayerObjects, const void *pStruct, int objectID)
	{
		int count = DETOUR_STATIC_CALL(SendProxyArrayLength_PlayerObjects)(pStruct, objectID);
		CTFPlayer *player = (CTFPlayer *)(pStruct);
		bool firstminisentry = true;
		for (int i = 0; i < count; i++) {
			CBaseObject *obj = player->GetObject(i);
			if (obj != nullptr && obj->m_bDisposableBuilding) {
				if (!firstminisentry)
					count--;
				firstminisentry = false;
			}
		}
		return count;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_StartBuildingObjectOfType, int type, int mode)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (type == 2) {
			int buildcount = player->GetObjectCount();
			int sentrycount = 0;
			int maxdisposables = 0;
			
			int minanimtimeid = -1;
			float minanimtime = std::numeric_limits<float>::max();
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, maxdisposables, engy_disposable_sentries);

			for (int i = buildcount - 1; i >= 0; i--) {
				CBaseObject *obj = player->GetObject(i);
				if (obj != nullptr && obj->GetType() == OBJ_SENTRYGUN) {
					sentrycount++;
					if (obj->m_flSimulationTime < minanimtime && obj->m_bDisposableBuilding) {
						minanimtime = obj->m_flSimulationTime;
						minanimtimeid = i;
					}
				}
			}
			if (sentrycount >= maxdisposables + 1 && minanimtimeid != -1) {
				player->GetObject(minanimtimeid)->DetonateObject();
			}
		}
		DETOUR_MEMBER_CALL(CTFPlayer_StartBuildingObjectOfType)(type, mode);
	}

	DETOUR_DECL_MEMBER(void, CCaptureFlag_Drop, CTFPlayer *pPlayer, bool bVisible, bool bThrown, bool bMessage)
	{
		DETOUR_MEMBER_CALL(CCaptureFlag_Drop)(pPlayer, bVisible, bThrown, bMessage);
		auto flag = reinterpret_cast<CCaptureFlag *>(this);
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			Vector pos = flag->GetAbsOrigin();
			if (pPlayer != nullptr && TheNavMesh->GetNavArea(pos, 99999.9f ) == nullptr) {
				CNavArea *area = pPlayer->GetLastKnownArea();
				if (area != nullptr) {
					area->GetClosestPointOnArea( pos, &pos );
					pos.z += 5.0f;

					flag->SetAbsOrigin( pos );
				}
			}
		}

	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Misc")
		{
            // Make dragons fury projectile non reflectable
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_IsDeflectable, "CTFProjectile_Rocket::IsDeflectable");

			// Make unowned sentry rocket deal damage
			MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Explode,    "CTFBaseRocket::Explode");

			// Makes penetration arrows not collide with bounding boxes of various entities
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_ArrowTouch, "CTFProjectile_Arrow::ArrowTouch");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_BallOfFire_RocketTouch, "CTFProjectile_BallOfFire::RocketTouch");

			// Allow to construct disposable sentries by destroying the oldest ones
			MOD_ADD_DETOUR_STATIC(SendProxy_PlayerObjectList,    "SendProxy_PlayerObjectList");
			MOD_ADD_DETOUR_STATIC(SendProxyArrayLength_PlayerObjects,    "SendProxyArrayLength_PlayerObjects");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_StartBuildingObjectOfType, "CTFPlayer::StartBuildingObjectOfType");

			// Drop flag to last bot nav area
			MOD_ADD_DETOUR_MEMBER(CCaptureFlag_Drop, "CCaptureFlag::Drop");
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_misc", "0", FCVAR_NOTIFY,
		"Mod: Stuff i am lazy to make into separate mods",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}