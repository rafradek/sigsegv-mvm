#include "mod.h"
#include "mod/ai/mvm_defender_bots/helpers.h"
#include "mod/ai/mvm_defender_bots/actions/attack_tank.h"


namespace Mod::AI::MvM_Defender_Bots
{
	ActionResult<CTFBot> CTFBotAttackTank::OnStart(CTFBot *actor, Action<CTFBot> *action)
	{
		this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
		
		if (!this->SelectTarget()) {
			return ActionResult<CTFBot>::Done("No tanks to attack.");
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	ActionResult<CTFBot> CTFBotAttackTank::Update(CTFBot *actor, float dt)
	{
		if (this->m_hTarget == nullptr || !this->m_hTarget->IsAlive()) {
			return ActionResult<CTFBot>::Done("Tank is no longer valid.");
		}
		
		this->DrawOverlays();
		
		this->EquipBestTankWeapon();
		
		float dist_to_tank;
		{
			Ray_t ray;
			ray.Init(actor->EyePosition(), this->m_hTarget->WorldSpaceCenter());
			
			trace_t tr;
			enginetrace->ClipRayToCollideable(ray, CONTENTS_SOLID, this->m_hTarget->GetCollideable(), &tr);
			
			if ((tr.contents & CONTENTS_SOLID) != 0) {
				dist_to_tank = tr.startpos.DistTo(tr.endpos);
			} else {
				dist_to_tank = FLT_MAX;
			}
		}
		
		NDebugOverlay::EntityText(ENTINDEX(actor), 2, CFmtStrN<64>("IDEAL:  %.0f HU", this->GetIdealTankRange()), 0.1f, 0xff, 0xff, 0xff, 0xff);
		NDebugOverlay::EntityText(ENTINDEX(actor), 3, CFmtStrN<64>("ACTUAL: %.0f HU", dist_to_tank), 0.1f, 0xff, 0xff, 0xff, 0xff);
		
		if (!actor->IsLineOfFireClear(this->m_hTarget) || dist_to_tank > this->GetIdealTankRange()) {
			auto nextbot = actor->MyNextBotPointer();
			
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(0.3f, 0.4f));
				
				CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
				this->m_PathFollower.Compute(nextbot, this->m_hTarget->GetAbsOrigin(), cost_func, 0.0f, true);
			}
			
			this->m_PathFollower.Update(nextbot);
		}
		
		// look @ tank if line of sight is clear and we are near it
		
		return ActionResult<CTFBot>::Continue();
	}
	
	void CTFBotAttackTank::OnEnd(CTFBot *actor, Action<CTFBot> *action)
	{
		
	}
	
	
	EventDesiredResult<CTFBot> CTFBotAttackTank::OnMoveToSuccess(CTFBot *actor, const Path *path)
	{
		DevMsg("CTFBotAttackTank::OnMoveToSuccess(#%d)\n", ENTINDEX(actor));
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	EventDesiredResult<CTFBot> CTFBotAttackTank::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
	{
		DevMsg("CTFBotAttackTank::OnMoveToFailure(#%d)\n", ENTINDEX(actor));
		
	//	this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Invalidate();
		
		actor->GetLocomotionInterface()->ClearStuckStatus("");
		
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	EventDesiredResult<CTFBot> CTFBotAttackTank::OnStuck(CTFBot *actor)
	{
		DevMsg("CTFBotAttackTank::OnStuck(#%d)\n", ENTINDEX(actor));
		
	//	this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Invalidate();
		
		actor->GetLocomotionInterface()->ClearStuckStatus("");
		
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	
	bool CTFBotAttackTank::IsPossible(CTFBot *actor)
	{
#if 0
		bool is_possible = false;
		
		ForEachTank([&](CTFTankBoss *tank, bool& done){
			is_possible = true;
			done = true;
		});
		
		return is_possible;
#endif
	}
	
	
	bool CTFBotAttackTank::SelectTarget()
	{
#if 0
		CTFTankBoss *best_tank = nullptr;
		float best_progress = FLT_MIN;
		
		ForEachTank([&](CTFTankBoss *tank, bool& done){
			float progress = GetTankProgress(tank);
			
			if (progress > best_progress) {
				best_progress = progress;
				best_tank = tank;
			}
		});
		
		if (best_tank == nullptr) {
			return false;
		}
		
		this->m_hTarget = best_tank;
		return true;
#endif
	}
	
	
	ConVar cvar_overlay_origin("sig_tank_overlay_origin", "0", FCVAR_NOTIFY,
		"Tank overlay: origin");
	ConVar cvar_overlay_wsc("sig_tank_overlay_wsc", "0", FCVAR_NOTIFY,
		"Tank overlay: world space center");
	ConVar cvar_overlay_bbox("sig_tank_overlay_bbox", "0", FCVAR_NOTIFY,
		"Tank overlay: bounding box");
	ConVar cvar_overlay_angles("sig_tank_overlay_angles", "0", FCVAR_NOTIFY,
		"Tank overlay: orientation");
	
	void CTFBotAttackTank::DrawOverlays()
	{
		if (cvar_overlay_origin.GetBool()) {
			NDebugOverlay::Axis(this->m_hTarget->GetAbsOrigin(), vec3_angle, 15.0f, true, 0.1f);
		}
		
		if (cvar_overlay_wsc.GetBool()) {
			NDebugOverlay::Axis(this->m_hTarget->WorldSpaceCenter(), vec3_angle, 15.0f, true, 0.1f);
		}
		
		if (cvar_overlay_bbox.GetBool()) {
			const Vector& center = this->m_hTarget->CollisionProp()->GetCollisionOrigin();
			const Vector& mins   = this->m_hTarget->CollisionProp()->OBBMins();
			const Vector& maxs   = this->m_hTarget->CollisionProp()->OBBMaxs();
			
			NDebugOverlay::Box(center, mins, maxs, 0xff, 0xff, 0x00, 0x00, 0.1f);
		}
		
		if (cvar_overlay_angles.GetBool()) {
			NDebugOverlay::Axis(this->m_hTarget->WorldSpaceCenter(), this->m_hTarget->GetAbsAngles(), 200.0f, true, 0.1f);
			
			QAngle ang = this->m_hTarget->GetAbsAngles();
			
			Vector fwd, rt, up;
			AngleVectors(ang, &fwd, &rt, &up);
			
			{
				Ray_t ray;
				ray.Init(this->m_hTarget->WorldSpaceCenter() + (1000.0f * fwd), this->m_hTarget->WorldSpaceCenter());
				
				trace_t tr;
				enginetrace->ClipRayToCollideable(ray, CONTENTS_SOLID, this->m_hTarget->GetCollideable(), &tr);
				
				Vector pos = tr.endpos;
				Vector dir = -fwd;
				
			//	NDebugOverlay::Sphere(pos, vec3_angle, 10.0f, 0xff, 0x00, 0x00, 0x80, true, 0.1f);
				NDebugOverlay::HorzArrow(pos - (30.0f * dir), pos - (10.0f * dir), 5.0f, 0xff, 0x00, 0x00, 0xff, true, 0.1f);
			}
		}
	}
	
	
	ConVar cvar_range_melee("sig_ai_mvm_defender_bots_tank_range_melee", "1.0", FCVAR_NOTIFY,
		"AttackTank: ideal attack range for melee weapons (multiplier)");
	ConVar cvar_range_splash("sig_ai_mvm_defender_bots_tank_range_splash", "400", FCVAR_NOTIFY,
		"AttackTank: ideal attack range for splash weapons");
	ConVar cvar_range_default("sig_ai_mvm_defender_bots_tank_range_default", "100", FCVAR_NOTIFY,
		"AttackTank: ideal attack range for normal weapons");
	
	float CTFBotAttackTank::GetIdealTankRange()
	{
		CTFWeaponBase *weapon = this->GetActor()->GetActiveTFWeapon();
		if (weapon != nullptr) {
			auto melee = rtti_cast<CTFWeaponBaseMelee *>(weapon);
			if (melee != nullptr) {
				// TODO: also factor in these multiplier attrs:
				// "melee_bounds_multiplier" (float, on weapon)
				// "melee_range_multiplier"  (float, on weapon)
				
				return cvar_range_melee.GetFloat() * melee->GetSwingRange();
			}
			
			switch (weapon->GetWeaponID()) {
			case TF_WEAPON_ROCKETLAUNCHER:
			case TF_WEAPON_GRENADELAUNCHER:
			case TF_WEAPON_FLAREGUN:
			case TF_WEAPON_DIRECTHIT:
			case TF_WEAPON_PARTICLE_CANNON:
			case TF_WEAPON_CANNON:
				return cvar_range_splash.GetFloat();
			}
		}
		
		return cvar_range_default.GetFloat();
	}
	
	
/*	bool CompareTankWeapons_Scout(const CTFWeaponBase *lhs, const CTFWeaponBase *rhs)
	{
		
	}*/
	
	
	int EvalTankWeapon_Scout(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_SCATTERGUN:
		case TF_WEAPON_PEP_BRAWLER_BLASTER:
		case TF_WEAPON_SODA_POPPER:
		case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
			return 100;
			
		case TF_WEAPON_BAT:
		case TF_WEAPON_BAT_FISH:
		case TF_WEAPON_BAT_WOOD:
			return 80;
			
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_HANDGUN_SCOUT_SEC:
			return 60;
			
		case TF_WEAPON_BAT_GIFTWRAP:
			return 20;
			
		case TF_WEAPON_CLEAVER:
		case TF_WEAPON_LUNCHBOX:
		case TF_WEAPON_JAR:
		case TF_WEAPON_JAR_MILK:
			return 0;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 100;
		case 1:  return 60;
		case 2:  return 80;
		default: return 10;
		}
	}
	
	int EvalTankWeapon_Soldier(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_PARTICLE_CANNON:
		case TF_WEAPON_DIRECTHIT:
			return 100;
			
		case TF_WEAPON_SHOVEL:
		case TF_WEAPON_BOTTLE:
		case TF_WEAPON_SWORD:
			return 80;
			
		case TF_WEAPON_SHOTGUN_PRIMARY:
		case TF_WEAPON_SHOTGUN_SOLDIER:
		case TF_WEAPON_SHOTGUN_HWG:
		case TF_WEAPON_SHOTGUN_PYRO:
		case TF_WEAPON_RAYGUN:
			return 60;
			
		case TF_WEAPON_BUFF_ITEM:
		case TF_WEAPON_PARACHUTE:
			return 0;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 100;
		case 1:  return 60;
		case 2:  return 80;
		default: return 10;
		}
	}
	
	int EvalTankWeapon_Pyro(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_FLAMETHROWER:
		case TF_WEAPON_FLAMETHROWER_ROCKET:
			return 100;
			
		case TF_WEAPON_FIREAXE:
			return 80;
			
		case TF_WEAPON_SHOTGUN_PRIMARY:
		case TF_WEAPON_SHOTGUN_SOLDIER:
		case TF_WEAPON_SHOTGUN_HWG:
		case TF_WEAPON_SHOTGUN_PYRO:
			return 60;
			
		case TF_WEAPON_FLAREGUN:
		case TF_WEAPON_RAYGUN_REVENGE:
			return 20;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 100;
		case 1:  return 20;
		case 2:  return 80;
		default: return 10;
		}
	}
	
	int EvalTankWeapon_Demo(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_CANNON:
			return 100;
			
		case TF_WEAPON_BOTTLE:
		case TF_WEAPON_SHOVEL:
		case TF_WEAPON_SWORD:
		case TF_WEAPON_STICKBOMB:
			return 80;
			
		case TF_WEAPON_BUFF_ITEM:
		case TF_WEAPON_PARACHUTE:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_STICKY_BALL_LAUNCHER:
			return 0;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 100;
		case 1:  return 0;
		case 2:  return 80;
		default: return 10;
		}
	}
	
	int EvalTankWeapon_Heavy(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_MINIGUN:
			return 100;
			
		case TF_WEAPON_FISTS:
		case TF_WEAPON_FIREAXE:
			return 80;
			
		case TF_WEAPON_SHOTGUN_PRIMARY:
		case TF_WEAPON_SHOTGUN_SOLDIER:
		case TF_WEAPON_SHOTGUN_HWG:
		case TF_WEAPON_SHOTGUN_PYRO:
			return 60;
			
		case TF_WEAPON_LUNCHBOX:
			return 0;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 100;
		case 1:  return 0;
		case 2:  return 60;
		default: return 10;
		}
	}
	
	int EvalTankWeapon_Engie(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_WRENCH:
		case TF_WEAPON_MECHANICAL_ARM:
			return 100;
			
		case TF_WEAPON_SHOTGUN_PRIMARY:
		case TF_WEAPON_SHOTGUN_SOLDIER:
		case TF_WEAPON_SHOTGUN_HWG:
		case TF_WEAPON_SHOTGUN_PYRO:
		case TF_WEAPON_SENTRY_REVENGE:
		case TF_WEAPON_DRG_POMSON:
			return 80;
			
		case TF_WEAPON_SHOTGUN_BUILDING_RESCUE:
			return 60;
			
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_REVOLVER:
			return 40;
			
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_PDA_SPY_BUILD:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_LASER_POINTER:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_DISPENSER_GUN:
			return 0;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 80;
		case 1:  return 0;
		case 2:  return 100;
		default: return 10;
		}
	}
	
	int EvalTankWeapon_Medic(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_BONESAW:
		case TF_WEAPON_HARVESTER_SAW:
			return 100;
			
		case TF_WEAPON_SYRINGEGUN_MEDIC:
		case TF_WEAPON_NAILGUN:
			return 80;
			
		case TF_WEAPON_CROSSBOW:
			return 60;
			
		case TF_WEAPON_MEDIGUN:
			return 0;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 80;
		case 1:  return 0;
		case 2:  return 100;
		default: return 10;
		}
	}
	
	int EvalTankWeapon_Sniper(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			return 100;
			
		case TF_WEAPON_COMPOUND_BOW:
			return 80;
			
		case TF_WEAPON_CLUB:
			return 60;
			
		case TF_WEAPON_CHARGED_SMG:
		case TF_WEAPON_SMG:
			return 40;
			
		case TF_WEAPON_JAR:
		case TF_WEAPON_JAR_MILK:
			return 0;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 100;
		case 1:  return 40;
		case 2:  return 60;
		default: return 10;
		}
	}
	
	int EvalTankWeapon_Spy(int slot, CTFWeaponBase *weapon)
	{
		switch (weapon->GetWeaponID()) {
		case TF_WEAPON_REVOLVER:
			return 100;
			
		case TF_WEAPON_KNIFE:
			return 80;
			
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_PDA_SPY_BUILD:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_INVIS:
			return 0;
		}
		
		/* fallback for unknown weapon IDs */
		switch (slot) {
		case 0:  return 100;
		case 1:  return 0;
		case 2:  return 80;
		default: return 10;
		}
	}
	
	
	void CTFBotAttackTank::EquipBestTankWeapon()
	{
		CTFBot *actor = this->GetActor();
		
		CTFWeaponBase *best_weapon = nullptr;
		int best_score = INT_MIN;
		
		for (int slot : { 0, 1, 2 }) {
			auto weapon = rtti_cast<CTFWeaponBase *>(actor->Weapon_GetSlot(slot));
			if (weapon == nullptr || !actor->Weapon_CanSwitchTo(weapon)) continue;
			
			int id = weapon->GetWeaponID();
			
			int score;
			switch (actor->GetPlayerClass()->GetClassIndex()) {
			case TF_CLASS_SCOUT:        score = EvalTankWeapon_Scout  (slot, weapon); break;
			case TF_CLASS_SNIPER:       score = EvalTankWeapon_Sniper (slot, weapon); break;
			case TF_CLASS_SOLDIER:      score = EvalTankWeapon_Soldier(slot, weapon); break;
			case TF_CLASS_DEMOMAN:      score = EvalTankWeapon_Demo   (slot, weapon); break;
			case TF_CLASS_MEDIC:        score = EvalTankWeapon_Medic  (slot, weapon); break;
			case TF_CLASS_HEAVYWEAPONS: score = EvalTankWeapon_Heavy  (slot, weapon); break;
			case TF_CLASS_PYRO:         score = EvalTankWeapon_Pyro   (slot, weapon); break;
			case TF_CLASS_SPY:          score = EvalTankWeapon_Spy    (slot, weapon); break;
			case TF_CLASS_ENGINEER:     score = EvalTankWeapon_Engie  (slot, weapon); break;
			}
			
			if (best_weapon == nullptr || score > best_score) {
				best_weapon = weapon;
				best_score = score;
			}
		}
		
		if (best_weapon == nullptr) {
			Warning("CTFBotAttackTank::EquipBestTankWeapon: no valid weapons!\n");
			return;
		}
		
		actor->Weapon_Switch(best_weapon);
	}
}
