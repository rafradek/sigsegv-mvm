#include "mod.h"
#include "stub/projectiles.h"
#include "stub/tfplayer.h"
#include "util/iterate.h"
#include "util/clientmsg.h"
#include "stub/extraentitydata.h"
#include "stub/tfweaponbase.h"
#include "stub/trace.h"


namespace Mod::Etc::Heat_Seeking_Rockets
{
#if 1 && 0
	class CTFProjectile_Flare : public CTFProjectile_Rocket
	{
	public:
		
		
	private:
		// 288 float m_flGravity
		// ...
		// 4ac CHandle<T> m_hLauncher
		// ...
		// 4e4 float m_flHeatSeekTime
		// ...
	};
	
	
	float CTFProjectile_Flare::GetHeatSeekPower() const
	{
		float flAttr = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(this->m_hLauncher, flAttr, mod_projectile_heat_seek_power);
		return flAttr;
	}
	
	void CTFProjectile_Flare::Spawn()
	{
		if (this->GetHeatSeekPower() != 0.0f) {
			this->SetMoveType(MOVETYPE_CUSTOM, MOVECOLLIDE_DEFAULT);
			this->m_flGravity = 0.3f;
		} else {
			this->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM);
			this->m_flGravity = 0.3f;
		}
	}
	
	void CTFProjectile_Flare::PerformCustomPhysics(Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity)
	{
		if (gpGlobals->curtime > this->m_flHeatSeekTime) {
			CTFPlayer *target_player = nullptr;
			float target_distsqr     = FLT_MAX;
			
			ForEachTFPlayer([&](CTFPlayer *player){
				if (!player->m_Shared.InCond(TF_COND_BURNING)) return;
				if (player->InSameTeam(this))                  return;
				
				// ???: player->m_Shared.GetDisguiseTeam()
				// ???: this->GetTeamNumber()
				// ???: player->m_Shared.IsStealthed()
				
				if (player->GetTeamNumber() == TEAM_SPECTATOR) return;
				if (!player->IsAlive())                        return;
				
				 // TODO: use projectile's WSC
				Vector delta = player->WorldSpaceCenter() - this->GetAbsOrigin();
				
				if (DotProduct(delta.Normalized(), pNewVelocity->Normalized()) < -0.25f) return;
				
				 // TODO: use projectile's WSC
				float distsqr = this->GetAbsOrigin().DistToSqr(player->WorldSpaceCenter());
				if (distsqr < target_distsqr) {
					trace_t tr;
				 	// TODO: use projectile's WSC
					UTIL_TraceLine(player->WorldSpaceCenter(), this->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, player, COLLISION_GROUP_NONE, &tr);
					
					if (!tr.DidHit() || tr.m_pEnt == this) {
						target_player  = player;
						target_distsqr = distsqr;
					}
				}
			});
			
			float power = this->GetHeatSeekPower();
			
			QAngle angToTarget = *pNewAngles;
			if (target_player != nullptr) {
				// TODO: use projectile's WSC
				VectorAngles(target_player->WorldSpaceCenter() - this->GetAbsOrigin, angToTarget);
			}
			
			if (angToTarget != *pNewAngles) {
				pNewAngVelocity->x = Clamp(Approach(AngleDiff(angToTarget.x, pNewAngles->x) * 4.0f, pNewAngVelocity->x, power), -360.0f, 360.0f);
				pNewAngVelocity->y = Clamp(Approach(AngleDiff(angToTarget.y, pNewAngles->y) * 4.0f, pNewAngVelocity->y, power), -360.0f, 360.0f);
				pNewAngVelocity->z = Clamp(Approach(AngleDiff(angToTarget.z, pNewAngles->z) * 4.0f, pNewAngVelocity->z, power), -360.0f, 360.0f);
			}
			
			this->m_flHeatSeekTime = gpGlobals->curtime + 0.25f;
		}
		
		*pNewAngles += (*pNewAngVelocity * gpGlobals->frametime);
		
		Vector vecOrientation;
		AngleVectors(*pNewAngles, &vecOrientation);
		
		*pNewVelocity = vecOrientation * this->GetProjectileSpeed();
		
		*pNewPosition += (*pNewVelocity * gpGlobals->frametime);
	}
#endif

	CBaseEntity *disallow_movetype = nullptr;
	int disallow_movetype_tick = 0;
	DETOUR_DECL_MEMBER(void, CBaseProjectile_SetLauncher, CBaseEntity *launcher)
	{
		auto proj = reinterpret_cast<CBaseProjectile *>(this);
		CBaseEntity *original = proj->GetOriginalLauncher();
		DETOUR_MEMBER_CALL(CBaseProjectile_SetLauncher)(launcher);
		
		if (launcher != nullptr && original == nullptr) {
			auto weapon = static_cast<CTFWeaponBaseGun *>(launcher->MyCombatWeaponPointer());
			if (weapon != nullptr && weapon->GetOwnerEntity() != nullptr && weapon->GetOwnerEntity()->IsPlayer()) {

				HomingRockets &homing = *(GetExtraProjectileData(proj)->homing = new HomingRockets());

				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, homing.turn_power, mod_projectile_heat_seek_power);
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, homing.acceleration, projectile_acceleration);
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, homing.gravity, projectile_gravity);
				if (homing.turn_power != 0.0f || homing.acceleration != 0.0f || homing.gravity != 0.0f) {

					proj->SetMoveType(MOVETYPE_CUSTOM, proj->GetMoveCollide());
					if (proj->VPhysicsGetObject() != nullptr) {
						proj->VPhysicsDestroyObject();
					}
					disallow_movetype = proj;
					disallow_movetype_tick = gpGlobals->tickcount;

					homing.enable = true;
					float min_dot_product = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, min_dot_product, mod_projectile_heat_aim_error);
					if (min_dot_product != 0.0f)
						homing.min_dot_product = FastCos(DEG2RAD(Clamp(min_dot_product, 0.0f, 180.0f)));

					float aim_time = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, aim_time, mod_projectile_heat_aim_time);
					if (aim_time != 0.0f)
						homing.aim_time = aim_time;

					float acceleration_time = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, acceleration_time, projectile_acceleration_time);
					if (acceleration_time != 0.0f)
						homing.acceleration_time = acceleration_time;

					float acceleration_start = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, acceleration_start, projectile_acceleration_start_time);
					if (acceleration_start != 0.0f)
						homing.acceleration_start = acceleration_start;

					int follow_crosshair = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, follow_crosshair, mod_projectile_heat_follow_crosshair);
					if (follow_crosshair != 0)
						homing.follow_crosshair = true;

					int no_predict_target_speed = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, follow_crosshair, mod_projectile_heat_no_predict_target_speed);
					if (follow_crosshair != 0)
						homing.predict_target_speed = false;
					
					homing.speed = CalculateProjectileSpeed(weapon);

					if (homing.speed < 0) {
						homing.speed = -homing.speed;
					}
				}
				else
					homing.enable = false;
			}
		}
		
	}

	DETOUR_DECL_MEMBER(void, CBaseEntity_SetMoveType, MoveType_t val, MoveCollide_t collide)
	{
		if (disallow_movetype_tick == gpGlobals->tickcount && disallow_movetype == reinterpret_cast<CBaseEntity *>(this)) {
			val = MOVETYPE_CUSTOM;
		}

		DETOUR_MEMBER_CALL(CBaseEntity_SetMoveType)(val, collide);

		//if (setmovetype_energyring) {
		//	reinterpret_cast<CTFProjectile_EnergyRing>(this)->SetMoveType(MOVETYPE_CUSTOM);
		//	setmovetype_energyring = false;
		//}

	}
	
	/*DETOUR_DECL_MEMBER(void, CTFProjectile_Rocket_Spawn)
	{
		DETOUR_MEMBER_CALL(CTFProjectile_Rocket_Spawn)();
		if (setmovetype_rocket) {
			reinterpret_cast<CTFProjectile_Rocket_Spawn>(this)->SetMoveType(MOVETYPE_CUSTOM);
			setmovetype_energyring = false;
		}

	}*/

	/*DETOUR_DECL_MEMBER(void, CTFProjectile_Rocket_Spawn)
	{
		DETOUR_MEMBER_CALL(CTFProjectile_Rocket_Spawn)();
		
		auto ent = reinterpret_cast<CTFProjectile_Rocket *>(this);
		if (ent->GetLauncher() != nullptr) {
			float power = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(ent->GetLauncher(), power, mod_projectile_heat_seek_power);
			if (power != 0.0f) {
				ent->SetMoveType(MOVETYPE_CUSTOM);
			}
		}
	}*/
	inline bool PerformCustomPhysics(CBaseEntity *ent, Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity) {
		
		//if (strncmp(ent->GetClassname(), "tf_projectile", strlen("tf_projectile")) != 0){
		//	return false;
		//}

		//Assume all "tf_projectile" entities are projectiles, for better performance
		auto proj = static_cast<CBaseProjectile *>(ent);
		float seek = 0.0f;
		if (proj == nullptr) {
			return false;
		}

		auto extra = GetExtraProjectileData(proj, false);
		if (extra == nullptr || extra->homing == nullptr)
			return false;

		HomingRockets &homing = *extra->homing;

		if (!homing.enable) {
			return false;
		}
		
		constexpr float interval = 0.25f;

		float time = (float)(ent->m_flSimulationTime) - (float)(ent->m_flAnimTime);

		float speed_calculated = homing.speed + homing.acceleration * Clamp(time - homing.acceleration_start, 0.0f, homing.acceleration_time);
		
		if (homing.turn_power != 0.0f && time < homing.aim_time && gpGlobals->tickcount % (int)(interval / gpGlobals->interval_per_tick) == 0) {
			
			Vector target_vec = vec3_origin;

			if (homing.follow_crosshair) {
				CBaseEntity *owner = proj->GetOwnerEntity();
				if (owner != nullptr) {
					Vector forward;
					AngleVectors(owner->EyeAngles(), &forward);

					CTraceFilterIgnoreFriendlyCombatItems filter(owner, COLLISION_GROUP_NONE, false);
					trace_t result;
					UTIL_TraceLine(owner->EyePosition(), owner->EyePosition() + 4000.0f * forward, MASK_SHOT, &filter, &result);

					target_vec = result.endpos;
				}
			}
			else {
			//	float target_distsqr     = FLT_MAX;
				
				float target_dotproduct  = FLT_MIN;
				CTFPlayer *target_player = nullptr;
				ForEachTFPlayer([&](CTFPlayer *player){
					if (!player->IsAlive())                               return;
					if (player->GetTeamNumber() == TEAM_SPECTATOR)        return;
					if (player->GetTeamNumber() == proj->GetTeamNumber()) return;
					
					if (homing.ignore_disguised_spies) {
						if (player->m_Shared->InCond(TF_COND_DISGUISED) && player->m_Shared->GetDisguiseTeam() == proj->GetTeamNumber()) {
							return;
						}
					}
					
					if (homing.ignore_stealthed_spies) {
						if (player->m_Shared->IsStealthed() && player->m_Shared->GetPercentInvisible() >= 0.75f &&
							!player->m_Shared->InCond(TF_COND_STEALTHED_BLINK) && !player->m_Shared->InCond(TF_COND_BURNING) && !player->m_Shared->InCond(TF_COND_URINE) && !player->m_Shared->InCond(TF_COND_BLEEDING)) {
							return;
						}
					}
					
					Vector delta = player->WorldSpaceCenter() - proj->WorldSpaceCenter();

					float mindotproduct = homing.min_dot_product;
					float dotproduct = DotProduct(delta.Normalized(), pNewVelocity->Normalized());
					if (dotproduct < mindotproduct) return;
					
					

				//	float distsqr = proj->WorldSpaceCenter().DistToSqr(player->WorldSpaceCenter());
				//	if (distsqr < target_distsqr) {
					if (dotproduct > target_dotproduct) {
						trace_t tr;
						UTIL_TraceLine(player->WorldSpaceCenter(), proj->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, player, COLLISION_GROUP_NONE, &tr);
						
						if (!tr.DidHit() || tr.m_pEnt == proj) {
							target_player  = player;
							target_dotproduct = dotproduct;
						}
					}
				});
				if (target_player != nullptr) {
					target_vec = target_player->WorldSpaceCenter();
					float target_distance = proj->WorldSpaceCenter().DistTo(target_player->WorldSpaceCenter());
					if (homing.predict_target_speed)
						target_vec += target_player->GetAbsVelocity() * target_distance / speed_calculated;
				}
			}
			
			if (target_vec != vec3_origin) {
				QAngle angToTarget;
				VectorAngles(target_vec - proj->WorldSpaceCenter(), angToTarget);

				pNewAngVelocity->x = Clamp(Approach(AngleDiff(angToTarget.x, pNewAngles->x) * 4.0f, pNewAngVelocity->x, homing.turn_power), -360.0f, 360.0f);
				pNewAngVelocity->y = Clamp(Approach(AngleDiff(angToTarget.y, pNewAngles->y) * 4.0f, pNewAngVelocity->y, homing.turn_power), -360.0f, 360.0f);
				pNewAngVelocity->z = Clamp(Approach(AngleDiff(angToTarget.z, pNewAngles->z) * 4.0f, pNewAngVelocity->z, homing.turn_power), -360.0f, 360.0f);
			}
		}

		if (time < homing.aim_time)
			*pNewAngles += (*pNewAngVelocity * gpGlobals->frametime);
		
		Vector vecOrientation;
		AngleVectors(*pNewAngles, &vecOrientation);
		*pNewVelocity = vecOrientation * (speed_calculated) + Vector(0,0,-homing.gravity * (time));
		
	//	if (homing.gravity != 0) {
	//		VectorAngles(*pNewVelocity, *pNewAngles);
	//	}
		*pNewPosition += (*pNewVelocity * gpGlobals->frametime);
		return true;
	}

	DETOUR_DECL_MEMBER(void, CBaseEntity_PerformCustomPhysics, Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		if (!PerformCustomPhysics(ent, pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity)) {
			return DETOUR_MEMBER_CALL(CBaseEntity_PerformCustomPhysics)(pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity);
		}
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_Flare_PerformCustomPhysics, Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		if (!PerformCustomPhysics(ent, pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity)) {
			return DETOUR_MEMBER_CALL(CTFProjectile_Flare_PerformCustomPhysics)(pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity);
		}
	}
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Heat_Seeking_Rockets")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetMoveType,       "CBaseEntity::SetMoveType");
			MOD_ADD_DETOUR_MEMBER(CBaseProjectile_SetLauncher,      "CBaseProjectile::SetLauncher");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PerformCustomPhysics, "CBaseEntity::PerformCustomPhysics");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Flare_PerformCustomPhysics, "CTFProjectile_Flare::PerformCustomPhysics");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_heat_seeking_rockets", "0", FCVAR_NOTIFY,
		"Etc: enable heat-seeking rockets",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
