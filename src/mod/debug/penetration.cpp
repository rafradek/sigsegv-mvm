#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "util/scope.h"


namespace Mod::Debug::Penetration
{
	struct penetrated_target_list
	{
		CBaseEntity *ent; // +0x00
		float fraction;   // +0x04
	};
	
	struct CBulletPenetrateEnum
	{
		void **vtable;     // +0x00
		Ray_t *ray;        // +0x04
		int pen_type;      // +0x08
		CTFPlayer *player; // +0x0c
		bool bool_0x10;    // +0x10
		CUtlVector<penetrated_target_list> vec; // +0x14, actually: CUtlSortVector<penetrated_target_list, PenetratedTargetLess>
		int dword_0x28;    // +0x28
		bool bool_0x2c;    // +0x2c
	};
	
	
	RefCount rc_CTFPlayer_FireBullet;
	DETOUR_DECL_MEMBER(void, CTFPlayer_FireBullet, CTFWeaponBase *weapon, const FireBulletsInfo_t& info, bool bDoEffects, int nDamageType, int nCustomDamageType)
	{
		DevMsg("\nCTFPlayer::FireBullet BEGIN\n");
		
		DevMsg("  nCustomDamageType = %d\n", nCustomDamageType);
		
		int pen_type = nCustomDamageType;
		if (weapon != nullptr && weapon->GetPenetrateType() != 0) {
			pen_type = weapon->GetPenetrateType();
		}
		DevMsg("  pen_type = %d\n", pen_type);
		
		bool has_pen = ((unsigned int)pen_type - 0xbu <= 1u);
		DevMsg("  has_pen = %s\n", (has_pen ? "true" : "false"));
		
		SCOPED_INCREMENT(rc_CTFPlayer_FireBullet);
		DETOUR_MEMBER_CALL(weapon, info, bDoEffects, nDamageType, nCustomDamageType);
		
		DevMsg("CTFPlayer::FireBullet END\n");
	}
	
	RefCount rc_IEngineTrace_EnumerateEntities;
	DETOUR_DECL_MEMBER(void, IEngineTrace_EnumerateEntities_ray, const Ray_t& ray, bool triggers, IEntityEnumerator *pEnumerator)
	{
		DevMsg("  IEngineTrace::EnumerateEntities BEGIN\n");
		
		SCOPED_INCREMENT(rc_IEngineTrace_EnumerateEntities);
		DETOUR_MEMBER_CALL(ray, triggers, pEnumerator);
		
		auto pen = reinterpret_cast<CBulletPenetrateEnum *>(pEnumerator);
		FOR_EACH_VEC(pen->vec, i) {
			DevMsg("  pen[%d]: %.2f #%d (class '%s' name '%s')\n",
				i, pen->vec[i].fraction, ENTINDEX(pen->vec[i].ent),
				pen->vec[i].ent->GetClassname(), STRING(pen->vec[i].ent->GetEntityName()));
		}
		
		DevMsg("  IEngineTrace::EnumerateEntities END\n");
	}
	
	DETOUR_DECL_MEMBER(bool, CBulletPenetrateEnum_EnumEntity, IHandleEntity *pHandleEntity)
	{
		auto pen = reinterpret_cast<CBulletPenetrateEnum *>(this);
		auto ent = reinterpret_cast<CBaseEntity *>(pHandleEntity);
		
		int count_before = pen->vec.Count();
		
		bool result = DETOUR_MEMBER_CALL(pHandleEntity);
		
		if (rc_CTFPlayer_FireBullet > 0 && rc_IEngineTrace_EnumerateEntities > 0) {
			bool was_added = (pen->vec.Count() != count_before);
			
			if (was_added) {
				DevMsg("    CBulletPenetrateEnum::EnumEntity: ADDED #%d (class '%s' name '%s')\n",
					ENTINDEX(ent), ent->GetClassname(), STRING(ent->GetEntityName()));
			} else {
				DevMsg("    CBulletPenetrateEnum::EnumEntity: SKIPPED #%d (class '%s' name '%s')\n",
					ENTINDEX(ent), ent->GetClassname(), STRING(ent->GetEntityName()));
			}
		}
		
		return result;
	}
	
	class CDmgAccumulator;
	DETOUR_DECL_MEMBER(void, CBaseEntity_DispatchTraceAttack, const CTakeDamageInfo& info, const Vector& vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (rc_CTFPlayer_FireBullet > 0) {
			DevMsg("  CBaseEntity::DispatchTraceAttack: ent #%d (m_iPlayerPenetrationCount = %d)\n",
				ENTINDEX(ent), info.GetPlayerPenetrationCount());
		}
		
		DETOUR_MEMBER_CALL(info, vecDir, ptr, pAccumulator);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Penetration")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_FireBullet,               "CTFPlayer::FireBullet");
			MOD_ADD_DETOUR_MEMBER(IEngineTrace_EnumerateEntities_ray, "IEngineTrace::EnumerateEntities_ray");
			MOD_ADD_DETOUR_MEMBER(CBulletPenetrateEnum_EnumEntity,    "CBulletPenetrateEnum::EnumEntity");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_DispatchTraceAttack,    "CBaseEntity::DispatchTraceAttack");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_penetration", "0", FCVAR_NOTIFY,
		"Debug: penetration",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
