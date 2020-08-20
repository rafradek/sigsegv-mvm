#include "mod.h"
#include "stub/projectiles.h"
#include "stub/tfplayer.h"
#include "util/iterate.h"


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
	
	
	ConVar cvar_power("sig_etc_heat_seeking_rockets_power", "1.0", FCVAR_NOTIFY,
		"Stand-in for attribute mod_projectile_heat_seek_power");
	
	DETOUR_DECL_MEMBER(void, CTFProjectile_Rocket_Spawn)
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
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_PerformCustomPhysics, Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		if (!ent->ClassMatches("tf_projectile_rocket") || !ent->ClassMatches("tf_projectile_sentryrocket")) {
			DETOUR_MEMBER_CALL(CBaseEntity_PerformCustomPhysics)(pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity);
			return;
		}
		
		auto proj = rtti_cast<CTFProjectile_Rocket *>(ent);
		assert(proj != nullptr);
		
		constexpr float interval = 0.25f;
		if (gpGlobals->tickcount % (int)(interval / gpGlobals->interval_per_tick) == 0) {
			CTFPlayer *target_player = nullptr;
			float target_distsqr     = FLT_MAX;
			
			ForEachTFPlayer([&](CTFPlayer *player){
				if (!player->IsAlive())                               return;
				if (player->GetTeamNumber() == TEAM_SPECTATOR)        return;
				if (player->GetTeamNumber() == proj->GetTeamNumber()) return;
				
				// TODO: disguise     stuff
				// TODO: invisibility stuff
				
				Vector delta = player->WorldSpaceCenter() - proj->WorldSpaceCenter();

				float mindotproduct = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(proj->GetLauncher(), mindotproduct, mod_projectile_heat_aim_error);
				mindotproduct = FastCos(DEG2RAD(Clamp(mindotproduct, 0.0f, 180.0f)));
				if (DotProduct(delta.Normalized(), pNewVelocity->Normalized()) < mindotproduct) return;
				
				float distsqr = proj->WorldSpaceCenter().DistToSqr(player->WorldSpaceCenter());
				if (distsqr < target_distsqr) {
					trace_t tr;
					UTIL_TraceLine(player->WorldSpaceCenter(), proj->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, player, COLLISION_GROUP_NONE, &tr);
					
					if (!tr.DidHit() || tr.m_pEnt == proj) {
						target_player  = player;
						target_distsqr = distsqr;
					}
				}
			});
			
			if (target_player != nullptr) {
				QAngle angToTarget;
				VectorAngles(target_player->WorldSpaceCenter() - proj->WorldSpaceCenter(), angToTarget);
				
				float power = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(proj->GetLauncher(), power, mod_projectile_heat_seek_power);

				pNewAngVelocity->x = Clamp(Approach(AngleDiff(angToTarget.x, pNewAngles->x) * 4.0f, pNewAngVelocity->x, power), -360.0f, 360.0f);
				pNewAngVelocity->y = Clamp(Approach(AngleDiff(angToTarget.y, pNewAngles->y) * 4.0f, pNewAngVelocity->y, power), -360.0f, 360.0f);
				pNewAngVelocity->z = Clamp(Approach(AngleDiff(angToTarget.z, pNewAngles->z) * 4.0f, pNewAngVelocity->z, power), -360.0f, 360.0f);
			}
		}
		
		*pNewAngles += (*pNewAngVelocity * gpGlobals->frametime);
		
		Vector vecOrientation;
		AngleVectors(*pNewAngles, &vecOrientation);
		float speed = 1100.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(proj->GetLauncher(), speed, mult_projectile_speed);//proj->GetAbsVelocity();
		*pNewVelocity = vecOrientation * speed;
		
		*pNewPosition += (*pNewVelocity * gpGlobals->frametime);
		
		if (gpGlobals->tickcount % 2 == 0) {
			NDebugOverlay::EntityText(ENTINDEX(proj), -2, CFmtStr("  AngVel: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewAngVelocity)), 0.030f);
			NDebugOverlay::EntityText(ENTINDEX(proj), -1, CFmtStr("  Angles: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewAngles)),      0.030f);
			NDebugOverlay::EntityText(ENTINDEX(proj),  1, CFmtStr("Velocity: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewVelocity)),    0.030f);
			NDebugOverlay::EntityText(ENTINDEX(proj),  2, CFmtStr("Position: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewPosition)),    0.030f);
		}
		
	//	DevMsg("[%d] PerformCustomPhysics: #%d %s\n", gpGlobals->tickcount, ENTINDEX(ent), ent->GetClassname());
	}
	
	
	class CMod : public IMod/*, public IFrameUpdateListener*/
	{
	public:
		CMod() : IMod("Etc:Heat_Seeking_Rockets")
		{
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_Spawn,       "CTFProjectile_Rocket::Spawn");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PerformCustomPhysics, "CBaseEntity::PerformCustomPhysics");
		}
		
	//	virtual bool ShouldReceiveFrameEvents() const override { return this->IsEnabled(); }
	//	
	//	virtual void FrameUpdatePostEntityThink() override
	//	{
	//		static long frame = 0;
	//		if (++frame % 2 == 0) return;
	//		
	//		ForEachEntityByRTTI<CTFProjectile_Rocket>([](CTFProjectile_Rocket *projectile){
	//			DevMsg("#%d: movetype %d movecollide %d\n", ENTINDEX(projectile), projectile->GetMoveType(), projectile->GetMoveCollide());
	//		});
	//	}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_heat_seeking_rockets", "0", FCVAR_NOTIFY,
		"Etc: enable heat-seeking rockets",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
