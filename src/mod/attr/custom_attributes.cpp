#include "mod.h"
#include "stub/baseentity.h"
#include "stub/econ.h"
#include "stub/tfplayer.h"
#include "stub/projectiles.h"
#include "stub/tfweaponbase.h"
#include "stub/objects.h"
#include "stub/entities.h"
#include "stub/gamerules.h"
#include "stub/usermessages_sv.h"
#include "stub/particles.h"
#include "stub/misc.h"
#include "stub/trace.h"
#include "stub/upgrades.h"
#include "util/iterate.h"

struct CTFRadiusDamageInfo
{
	CTakeDamageInfo *m_DmgInfo;   // +0x00
	Vector m_vecOrigin;           // +0x04: blast origin
	float m_flRadius;             // +0x10: blast radius
	CBaseEntity *m_pEntityIgnore = nullptr; // +0x14
	float m_unknown_18 = 0.0f;           // +0x18, default 0.0f, ?
	float m_unknown_1c = 1.0f;           // +0x1c, default 1.0f, scales damage force or something
	CBaseEntity *target = nullptr;             // +0x20, default 0, ?
	float m_flFalloff = 0.5f;            // +0x24: how much the damage is decreased by at the edge of the blast
};


class CDmgAccumulator;

namespace Mod::Attr::Custom_Attributes
{
	std::set<std::string> precached;

	GlobalThunk<void *> g_pFullFileSystem("g_pFullFileSystem");

	const char *GetStringAttribute(CAttributeList &attrlist, const char *name) {
		auto attr = attrlist.GetAttributeByName(name);
		const char *value = nullptr;
		if (attr != nullptr && attr->GetValuePtr()->m_String != nullptr) {
			CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &value);
		}
		return value;
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_CanAirDash)
	{
		bool ret = DETOUR_MEMBER_CALL(CTFPlayer_CanAirDash)();
		if (!ret) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			if (!player->IsPlayerClass(TF_CLASS_SCOUT)) {
				int dash = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( player, dash, air_dash_count );
				if (dash > player->m_Shared->m_iAirDash)
					return true;
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CWeaponMedigun_AllowedToHealTarget, CBaseEntity *target)
	{
		bool ret = DETOUR_MEMBER_CALL(CWeaponMedigun_AllowedToHealTarget)(target);
		if (!ret && target != nullptr && target->IsBaseObject()) {
			auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
			auto owner = medigun->GetOwnerEntity();
			
			if (owner != nullptr && target->GetTeamNumber() == owner->GetTeamNumber()) {
				int can_heal = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( medigun, can_heal, medic_machinery_beam );
				return can_heal != 0;
			}
			
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CWeaponMedigun_HealTargetThink)
	{
		DETOUR_MEMBER_CALL(CWeaponMedigun_HealTargetThink)();
		auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
		CBaseEntity *healobject = medigun->GetHealTarget();
		if (healobject != nullptr && healobject->IsBaseObject() && healobject->GetHealth() < healobject->GetMaxHealth() ) {
			int can_heal = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( medigun, can_heal, medic_machinery_beam );
			healobject->SetHealth( healobject->GetHealth() + ( (medigun->GetHealRate() / 10.f) * can_heal ) );
			
		}
	}


	void CreateExtraArrow(CTFCompoundBow *bow, CTFProjectile_Arrow *main_arrow, const QAngle& angles, float speed) {
		CTFProjectile_Arrow* pExtraArrow = CTFProjectile_Arrow::Create( main_arrow->GetAbsOrigin(), angles, speed, bow->GetProjectileGravity(), bow->GetWeaponProjectileType(), main_arrow->GetOwnerEntity(), main_arrow->GetOwnerEntity() );
		if ( pExtraArrow != nullptr )
		{
			pExtraArrow->SetLauncher( bow );
			bool critical = main_arrow->m_bCritical;
			pExtraArrow->m_bCritical = critical;
			pExtraArrow->SetDamage( (TFGameRules()->IsPVEModeControlled(bow->GetOwnerEntity()) ? 1.0f : 0.5f) * bow->GetProjectileDamage() );
			//if ( main_arrow->CanPenetrate() )
			//{
				//pExtraArrow->SetPenetrate( true );
			//}
			pExtraArrow->SetCollisionGroup( main_arrow->GetCollisionGroup() );
		}
	}

	float GetRandomSpreadOffset( CTFCompoundBow *bow, int iLevel )
	{
		float flMaxRandomSpread = 2.5f;// sv_arrow_max_random_spread_angle.GetFloat();
		float flRandom = RemapValClamped( bow->GetCurrentCharge(), 0.f, bow->GetChargeMaxTime(), RandomFloat( -flMaxRandomSpread, flMaxRandomSpread ), 0.f );
		return 5.0f/*sv_arrow_spread_angle.GetFloat()*/ * iLevel + flRandom;
	}

	CBaseAnimating *projectile_arrow = nullptr;

	bool force_send_client = false;

	void AttackEnemyProjectiles( CTFPlayer *player, CTFWeaponBase *weapon, int shoot_projectiles)
	{

		const int nSweepDist = 300;	// How far out
		const int nHitDist = ( player->IsMiniBoss() ) ? 56 : 38;	// How far from the center line (radial)

		// Pos
		const Vector &vecGunPos = ( player->IsMiniBoss() ) ? player->Weapon_ShootPosition() : player->EyePosition();
		Vector vecForward;
		AngleVectors( weapon->GetAbsAngles(), &vecForward );
		Vector vecGunAimEnd = vecGunPos + vecForward * (float)nSweepDist;

		// Iterate through each grenade/rocket in the sphere
		const int maxCollectedEntities = 128;
		CBaseEntity	*pObjects[ maxCollectedEntities ];
		
		CFlaggedEntitiesEnum iter = CFlaggedEntitiesEnum(pObjects, maxCollectedEntities, FL_GRENADE );

		partition->EnumerateElementsInSphere(PARTITION_ENGINE_NON_STATIC_EDICTS, vecGunPos, nSweepDist, false, &iter);

		int count = iter.GetCount();

		for ( int i = 0; i < count; i++ )
		{
			if ( player->GetTeamNumber() == pObjects[i]->GetTeamNumber() )
				continue;

			// Hit?
			const Vector &vecGrenadePos = pObjects[i]->GetAbsOrigin();
			float flDistToLine = CalcDistanceToLineSegment( vecGrenadePos, vecGunPos, vecGunAimEnd );
			if ( flDistToLine <= nHitDist )
			{
				if ( player->FVisible( pObjects[i], MASK_SOLID ) == false )
					continue;

				if ( ( pObjects[i]->GetFlags() & FL_ONGROUND ) )
					continue;
					
				if ( !pObjects[i]->IsDeflectable() )
					continue;

				CBaseProjectile *pProjectile = static_cast< CBaseProjectile* >( pObjects[i] );
				if ( pProjectile->ClassMatches("tf_projectile*") && pProjectile->IsDestroyable() )
				{
					pProjectile->Destroy( false, true );

					weapon->EmitSound( "Halloween.HeadlessBossAxeHitWorld" );
				}
			}
		}
	}
	
	THINK_FUNC_DECL(ProjectileLifetime) {
		this->Remove();
	};

	bool fire_projectile_multi = false;

	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFWeaponBaseGun_FireProjectile, CTFPlayer *player)
	{
		auto weapon = reinterpret_cast<CTFWeaponBaseGun *>(this);
		int attr_projectile_count = 1;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, attr_projectile_count, mult_projectile_count);
		CBaseAnimating *proj = nullptr;
		for (int i = 0; i < attr_projectile_count; i++) {

			fire_projectile_multi = i != 0;
			proj = DETOUR_MEMBER_CALL(CTFWeaponBaseGun_FireProjectile)(player);
			fire_projectile_multi = false;

			if (proj != nullptr) {
				float attr_lifetime = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, attr_lifetime, projectile_lifetime);

				if (attr_lifetime != 0.0f) {
					THINK_FUNC_SET(proj, ProjectileLifetime, gpGlobals->curtime + attr_lifetime);
					//proj->ThinkSet(&ProjectileLifetime::Update, gpGlobals->curtime + attr_lifetime, "ProjLifetime");
				}

				const char *particlename = GetStringAttribute(weapon->GetItem()->GetAttributeList(), "projectile trail particle");
				if (particlename != nullptr) {
					
					DevMsg("particlename %s\n",particlename);

					force_send_client = true;
					if (*particlename == '~') {
						StopParticleEffects(proj);
						DispatchParticleEffect(particlename + 1, PATTACH_ABSORIGIN_FOLLOW, proj, INVALID_PARTICLE_ATTACHMENT, false);
					} else {
						DispatchParticleEffect(particlename, PATTACH_ABSORIGIN_FOLLOW, proj, INVALID_PARTICLE_ATTACHMENT, false);
					}
					force_send_client = false;
				}
				if (i < attr_projectile_count - 1)
					weapon->ModifyProjectile(proj);
			}
		}
		
		int shoot_projectiles = 0;
			
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, shoot_projectiles, attack_projectiles);
		if (shoot_projectiles > 0) {
			auto weapon = reinterpret_cast<CTFWeaponBase*>(this);
			if (weapon->GetWeaponID() != TF_WEAPON_MINIGUN)
				AttackEnemyProjectiles(player, reinterpret_cast<CTFWeaponBase*>(this), shoot_projectiles);
		}
		
		projectile_arrow = proj;
		return proj;
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_RemoveProjectileAmmo, CTFPlayer *player)
	{
		if (!fire_projectile_multi)
			DETOUR_MEMBER_CALL(CTFWeaponBaseGun_RemoveProjectileAmmo)(player);
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_UpdatePunchAngles, CTFPlayer *player)
	{
		if (!fire_projectile_multi)
			DETOUR_MEMBER_CALL(CTFWeaponBaseGun_UpdatePunchAngles)(player);
	}
	
	DETOUR_DECL_MEMBER(void, CTFCompoundBow_LaunchGrenade)
	{
		DETOUR_MEMBER_CALL(CTFCompoundBow_LaunchGrenade)();

		int attib_arrow_mastery = 0;
		auto bow = reinterpret_cast<CTFCompoundBow *>(this);
		CALL_ATTRIB_HOOK_INT_ON_OTHER( bow, attib_arrow_mastery, arrow_mastery );

		if (attib_arrow_mastery >= 0 && projectile_arrow != nullptr) {

			Vector vecMainVelocity = projectile_arrow->GetAbsVelocity();
			float flMainSpeed = vecMainVelocity.Length();

			CTFProjectile_Arrow *arrow = static_cast<CTFProjectile_Arrow *>(projectile_arrow);

			if (arrow != nullptr) {
				for (int i = 0; i < attib_arrow_mastery; i++) {
					
					QAngle qOffset1 = projectile_arrow->GetAbsAngles() + QAngle( 0, GetRandomSpreadOffset( bow, i + 1 ), 0 );
					CreateExtraArrow( bow, arrow, qOffset1, flMainSpeed );
					QAngle qOffset2 = projectile_arrow->GetAbsAngles() + QAngle( 0, -GetRandomSpreadOffset( bow, i + 1 ), 0 );
					CreateExtraArrow( bow, arrow, qOffset2, flMainSpeed );

				}
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseMelee_Swing, CTFPlayer *player)
	{
		DETOUR_MEMBER_CALL(CTFWeaponBaseMelee_Swing)(player);
		auto weapon = reinterpret_cast<CTFWeaponBaseMelee*>(this);
		float smacktime = weapon->m_flSmackTime;
		float time = gpGlobals->curtime;
		float attr_smacktime = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, attr_smacktime, mult_smack_time);
		weapon->m_flSmackTime = time + (smacktime - time) * attr_smacktime;
		if (!player->IsBot() && weapon->m_flSmackTime > weapon->m_flNextPrimaryAttack)
			weapon->m_flSmackTime = weapon->m_flNextPrimaryAttack - 0.02;
	}

	DETOUR_DECL_MEMBER(int, CBaseObject_OnTakeDamage, CTakeDamageInfo &info)
	{
		int damage = DETOUR_MEMBER_CALL(CBaseObject_OnTakeDamage)(info);
		auto weapon = info.GetWeapon();
		if (weapon != nullptr) {
			auto tfweapon = ToBaseCombatWeapon(weapon);
			if (tfweapon != nullptr) {
				float attr_disabletime = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( tfweapon, attr_disabletime, disable_buildings_on_hit);
				if (attr_disabletime > 0.0f) {
					reinterpret_cast<CBaseObject*>(this)->SetPlasmaDisabled(attr_disabletime);
				}
			}
		}
		return damage;
	}

	bool is_ice = false;
	DETOUR_DECL_MEMBER(void, CTFPlayer_Event_Killed, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		if (info.GetWeapon() != nullptr) {
			int attr_ice = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), attr_ice, set_turn_to_ice );
			is_ice = attr_ice != 0;
		}
		else
			is_ice = false;

		DETOUR_MEMBER_CALL(CTFPlayer_Event_Killed)(info);
		is_ice = false;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_CreateRagdollEntity, bool bShouldGib, bool bBurning, bool bUberDrop, bool bOnGround, bool bYER, bool bGold, bool bIce, bool bAsh, int iCustom, bool bClassic)
	{
		bIce |= is_ice;
		
		return DETOUR_MEMBER_CALL(CTFPlayer_CreateRagdollEntity)(bShouldGib, bBurning, bUberDrop, bOnGround, bYER, bGold, bIce, bAsh, iCustom, bClassic);
	}

	void GetExplosionParticle(CTFPlayer *player, CTFWeaponBase *weapon, int &particle) {
		const char *particlename = GetStringAttribute(weapon->GetItem()->GetAttributeList(), "explosion particle");
		if (particlename != nullptr) {
			if (precached.find(particlename) == precached.end()) {
				PrecacheParticleSystem(particlename);
				precached.insert(particlename);
			}
			particle = GetParticleSystemIndex(particlename);
			DevMsg("Expl part %s\n",particlename);
		}
	}

	int particle_to_use = 0;

	CTFWeaponBaseGun *stickbomb = nullptr;
	DETOUR_DECL_MEMBER(void, CTFStickBomb_Smack)
	{	
		stickbomb = reinterpret_cast<CTFWeaponBaseGun*>(this);
		particle_to_use = 0;
		GetExplosionParticle(stickbomb->GetTFPlayerOwner(), stickbomb,particle_to_use);
		DETOUR_MEMBER_CALL(CTFStickBomb_Smack)();
		stickbomb = nullptr;
		particle_to_use = 0;
	}

	RefCount rc_CTFGameRules_RadiusDamage;
	
	bool minicrit = false;
	DETOUR_DECL_MEMBER(void, CTFGameRules_RadiusDamage, CTFRadiusDamageInfo& info)
	{
		SCOPED_INCREMENT(rc_CTFGameRules_RadiusDamage);
		if (stickbomb != nullptr) {
			float radius = 1.0f;
			
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( stickbomb, radius, mult_explosion_radius);

			info.m_flRadius = radius * info.m_flRadius;

			int iLargeExplosion = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( stickbomb, iLargeExplosion, use_large_smoke_explosion );
			if ( iLargeExplosion > 0 )
			{
				force_send_client = true;
				DispatchParticleEffect( "explosionTrail_seeds_mvm", info.m_vecOrigin , vec3_angle );
				DispatchParticleEffect( "fluidSmokeExpl_ring_mvm", info.m_vecOrigin , vec3_angle);
				force_send_client = false;
			}
			//DevMsg("mini crit used: %d\n",minicrit);
			//info.m_DmgInfo->SetDamageType(info.m_DmgInfo->GetDamageType() & (~DMG_USEDISTANCEMOD));
			if (minicrit) {
				stickbomb->GetTFPlayerOwner()->m_Shared->AddCond(TF_COND_NOHEALINGDAMAGEBUFF, 99.0f);
				DETOUR_MEMBER_CALL(CTFGameRules_RadiusDamage)(info);
				stickbomb->GetTFPlayerOwner()->m_Shared->RemoveCond(TF_COND_NOHEALINGDAMAGEBUFF);
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CTFGameRules_RadiusDamage)(info);
	}

	CBasePlayer *process_movement_player = nullptr;
	DETOUR_DECL_MEMBER(void, CTFGameMovement_ProcessMovement, CBasePlayer *player, void *data)
	{
		process_movement_player = player;
		DETOUR_MEMBER_CALL(CTFGameMovement_ProcessMovement)(player, data);
		process_movement_player = nullptr;
	}

	DETOUR_DECL_MEMBER(bool, CTFGameMovement_CheckJumpButton)
	{
		bool ret = DETOUR_MEMBER_CALL(CTFGameMovement_CheckJumpButton)();

		if (ret && process_movement_player != nullptr) {
			auto player = ToTFPlayer(process_movement_player);
			//CAttributeList *attrlist = player->GetAttributeList();
			//auto attr = attrlist->GetAttributeByName("custom jump particle");
			//if (attr != nullptr) {
			//	
			//	const char *particlename;
			//	CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &particlename);
				//GetItemSchema()->GetAttributeDefinitionByName("custom jump particle")->ConvertValueToString(*(attr->GetValuePtr()),particlename,255);

			//	DevMsg ("jump %s\n", particlename);
			//	DispatchParticleEffect( particlename, PATTACH_POINT_FOLLOW, player, "foot_L" );
			//	DispatchParticleEffect( particlename, PATTACH_POINT_FOLLOW, player, "foot_R" );
			//}

			if (!player->IsBot()) {
				int attr_jump = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( player, attr_jump, bot_custom_jump_particle );
				if (attr_jump) {
					const char *particlename = "rocketjump_smoke";
					force_send_client = true;
					DispatchParticleEffect( particlename, PATTACH_POINT_FOLLOW, player, "foot_L" );
					DispatchParticleEffect( particlename, PATTACH_POINT_FOLLOW, player, "foot_R" );
					force_send_client = false;
				}
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CEconEntity_UpdateModelToClass)
	{
		DETOUR_MEMBER_CALL(CEconEntity_UpdateModelToClass)();
		auto entity = reinterpret_cast<CEconEntity *>(this);
		CAttributeList &attrlist = entity->GetItem()->GetAttributeList();
		const char *modelname = GetStringAttribute(attrlist, "custom item model");

		if (modelname != nullptr) {

			int model_index = CBaseEntity::PrecacheModel(modelname);
			for (int i = 0; i < MAX_VISION_MODES; ++i) {
				entity->SetModelIndexOverride(i, model_index);
			}
		}
		int color = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( entity, color, item_color_rgb);
		if (color != 0) {
			entity->SetRenderColorR(color >> 16);
			entity->SetRenderColorG((color >> 8) & 255);
			entity->SetRenderColorB(color & 255);
		}
		int invisible = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( entity, invisible, is_invisible);
		if (invisible != 0) {
			DevMsg("SetInvisible\n");
			servertools->SetKeyValue(entity, "rendermode", "10");
			entity->AddEffects(EF_NODRAW);
			entity->SetRenderColorA(0);
		}

		const char *attachmentname = GetStringAttribute(attrlist, "attachment name");

		auto owner = ToTFPlayer(entity->GetOwnerEntity());
		
		if (owner != nullptr && attachmentname != nullptr) {
			int attachment = owner->LookupAttachment(attachmentname);
			entity->SetEffects(entity->GetEffects() & ~(EF_BONEMERGE));
			if (attachment > 0) {
				entity->SetParent(owner, attachment);
				
			}

			Vector pos = vec3_origin;
			QAngle ang = vec3_angle;
			float scale = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( entity, scale, attachment_scale);
			
			const char *offsetstr = GetStringAttribute(attrlist, "attachment offset");
			const char *anglesstr = GetStringAttribute(attrlist, "attachment angles");

			if (offsetstr != nullptr)
				sscanf(offsetstr, "%f %f %f", &pos.x, &pos.y, &pos.z);

			if (anglesstr != nullptr)
				sscanf(anglesstr, "%f %f %f", &ang.x, &ang.y, &ang.z);
				
			if (scale != 1.0f) {
				entity->SetModelScale(scale);
			}
			entity->SetLocalOrigin(pos);
			entity->SetLocalAngles(ang);
		}
	}
	DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_Equip, CBaseCombatCharacter *owner)
	{
		DETOUR_MEMBER_CALL(CBaseCombatWeapon_Equip)(owner);
		auto ent = reinterpret_cast<CBaseCombatWeapon *>(this);
		auto econent = rtti_cast<CEconEntity *>(ent);
		if (econent != nullptr) {
			const char *attachmentname = GetStringAttribute(econent->GetItem()->GetAttributeList(), "attachment name");
			if (attachmentname != nullptr) {
				econent->SetEffects(econent->GetEffects() & ~(EF_BONEMERGE));
			}
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGrenadeProj_Explode, trace_t *pTrace, int bitsDamageType)
	{
		particle_to_use = 0;
		auto proj = reinterpret_cast<CTFWeaponBaseGrenadeProj *>(this);
		auto launcher = static_cast<CTFWeaponBase *>(ToBaseCombatWeapon(proj->GetOriginalLauncher()));
		if (launcher != nullptr)
			GetExplosionParticle(launcher->GetTFPlayerOwner(), launcher,particle_to_use);
		DETOUR_MEMBER_CALL(CTFWeaponBaseGrenadeProj_Explode)(pTrace, bitsDamageType);
		particle_to_use = 0;
	}

	DETOUR_DECL_MEMBER(void, CTFBaseRocket_Explode, trace_t *pTrace, CBaseEntity *pOther)
	{
		particle_to_use = 0;
		auto proj = reinterpret_cast<CTFBaseRocket *>(this);
		auto launcher = static_cast<CTFWeaponBase *>(ToBaseCombatWeapon(proj->GetOriginalLauncher()));
		if (launcher != nullptr)
			GetExplosionParticle(launcher->GetTFPlayerOwner(), launcher,particle_to_use);
		DETOUR_MEMBER_CALL(CTFBaseRocket_Explode)(pTrace, pOther);
		particle_to_use = 0;
	}

	DETOUR_DECL_STATIC(void, TE_TFExplosion, IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, int nEntIndex, int nDefID, int nSound, int iCustomParticle)
	{
		//CBasePlayer *playerbase;
		//if (nEntIndex > 0 && (playerbase = UTIL_PlayerByIndex(nEntIndex)) != nullptr) {
		if (particle_to_use != 0)
			iCustomParticle = particle_to_use;
		DETOUR_STATIC_CALL(TE_TFExplosion)(filter, flDelay, vecOrigin, vecNormal, iWeaponID, nEntIndex, nDefID, nSound, iCustomParticle);
	}

	const char *weapon_sound_override = nullptr;
	CBasePlayer *weapon_sound_override_owner = nullptr;
	DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_WeaponSound, int index, float soundtime) 
	{
		if ((index == 1 || index == 5)) {
			auto weapon = reinterpret_cast<CTFWeaponBase *>(this);

			const char *modelname = GetStringAttribute(weapon->GetItem()->GetAttributeList(), "custom weapon fire sound");
			if (weapon->GetOwner() != nullptr && modelname != nullptr) {
				DevMsg("Sound %s\n", modelname);
				if (precached.find(modelname) == precached.end()) {
					if (!enginesound->PrecacheSound(modelname, true))
						CBaseEntity::PrecacheScriptSound(modelname);
					precached.insert(modelname);
				}
				weapon_sound_override_owner = ToTFPlayer(weapon->GetOwner());
				weapon_sound_override = modelname;

				weapon->GetOwner()->EmitSound(modelname, soundtime);
				return;
			}
		}
		DETOUR_MEMBER_CALL(CBaseCombatWeapon_WeaponSound)(index, soundtime);
		weapon_sound_override = nullptr;
	}

	DETOUR_DECL_STATIC(void, CBaseEntity_EmitSound, IRecipientFilter& filter, int iEntIndex, const char *sound, const Vector *pOrigin, float start, float *duration )
	{
		if (weapon_sound_override != nullptr) {
			//reinterpret_cast<CRecipientFilter &>(filter).AddRecipient(weapon_sound_override_owner);
			DevMsg("%s\n",weapon_sound_override);
			sound = weapon_sound_override;
		}
		DETOUR_STATIC_CALL(CBaseEntity_EmitSound)(filter, iEntIndex, sound, pOrigin, start, duration);
	}

	RefCount rc_CTFPlayer_FireBullet;
	DETOUR_DECL_MEMBER(void, CTFPlayer_FireBullet, CTFWeaponBase *weapon, const FireBulletsInfo_t& info, bool bDoEffects, int nDamageType, int nCustomDamageType)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_FireBullet);
		DETOUR_MEMBER_CALL(CTFPlayer_FireBullet)(weapon, info, bDoEffects, nDamageType, nCustomDamageType);
	}
	RefCount rc_CBaseEntity_DispatchTraceAttack;
	DETOUR_DECL_MEMBER(void, CBaseEntity_DispatchTraceAttack, const CTakeDamageInfo& info, const Vector& vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
	{
		
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		SCOPED_INCREMENT(rc_CBaseEntity_DispatchTraceAttack);
		bool useinfomod = false;
		CTakeDamageInfo infomod = info;
		if ((info.GetDamageType() & DMG_USE_HITLOCATIONS) && info.GetWeapon() != nullptr) {
			int can_headshot = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(ent, can_headshot, cannot_be_headshot);
			if (can_headshot != 0) {
				infomod.SetDamageType(info.GetDamageType() & ~DMG_USE_HITLOCATIONS);
				useinfomod = true;
			}
		}
		//int predamagecustom = info.GetDamageCustom();
		//int predamage = info.GetDamageType();

		if (rc_CTFPlayer_FireBullet > 0 && ptr != nullptr && rc_CTFGameRules_RadiusDamage == 0 && (ptr->surface.flags & SURF_SKY) == 0) {
			if (info.GetWeapon() != nullptr) {
				auto weapon = rtti_cast<CTFWeaponBase *>(info.GetWeapon());
				int attr_explode_bullet = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, attr_explode_bullet, explosive_bullets);
				if (weapon && attr_explode_bullet != 0) {
					
					CRecipientFilter filter;
					filter.AddRecipientsByPVS(ptr->endpos);

					int iLargeExplosion = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( weapon, iLargeExplosion, use_large_smoke_explosion );
					if ( iLargeExplosion > 0 )
					{
						force_send_client = true;
						DispatchParticleEffect( "explosionTrail_seeds_mvm", ptr->endpos , vec3_angle );
						DispatchParticleEffect( "fluidSmokeExpl_ring_mvm", ptr->endpos , vec3_angle);
						force_send_client = false;
					}
					int customparticle = INVALID_STRING_INDEX;
					GetExplosionParticle(ToTFPlayer(weapon->GetOwnerEntity()), weapon, customparticle);

					TE_TFExplosion( filter, 0.0f, ptr->endpos, Vector(0,0,1), TF_WEAPON_GRENADELAUNCHER, ENTINDEX(info.GetAttacker()), -1, 11/*SPECIAL1*/, customparticle );

					CTFRadiusDamageInfo radiusinfo;
					CTakeDamageInfo info2( weapon, weapon->GetOwnerEntity(), weapon, vec3_origin, ptr->endpos, info.GetDamage(), (infomod.GetDamageType() | DMG_BLAST) & (~DMG_BULLET), infomod.GetDamageCustom() );

					radiusinfo.m_DmgInfo = &info2;
					radiusinfo.m_vecOrigin = ptr->endpos;
					radiusinfo.m_flRadius = attr_explode_bullet;
					radiusinfo.m_pEntityIgnore = ent;
					radiusinfo.m_unknown_18 = 0.0f;
					radiusinfo.m_unknown_1c = 1.0f;
					radiusinfo.target = nullptr;
					radiusinfo.m_flFalloff = 0.5f;

					Vector origpos = weapon->GetAbsOrigin();
					weapon->SetAbsOrigin(ptr->endpos - (weapon->WorldSpaceCenter() - origpos));
					TFGameRules()->RadiusDamage( radiusinfo );
					weapon->SetAbsOrigin(origpos);
					DETOUR_MEMBER_CALL(CBaseEntity_DispatchTraceAttack)(info2, vecDir, ptr, pAccumulator);
					return;
				}
			}
		}
		if (stickbomb != nullptr) {
			if (rc_CTFGameRules_RadiusDamage == 0) {
				
				minicrit = (info.GetDamageType() & DMG_RADIUS_MAX) != 0;
				stickbomb->GetTFPlayerOwner()->m_Shared->AddCond(TF_COND_NOHEALINGDAMAGEBUFF, 99.0f);
				DETOUR_MEMBER_CALL(CBaseEntity_DispatchTraceAttack)(info, vecDir, ptr, pAccumulator);
				stickbomb->GetTFPlayerOwner()->m_Shared->RemoveCond(TF_COND_NOHEALINGDAMAGEBUFF);
				DevMsg("mini crit set: %d\n",minicrit);
				return;
			}
		}

		DETOUR_MEMBER_CALL(CBaseEntity_DispatchTraceAttack)(useinfomod ? infomod : info, vecDir, ptr, pAccumulator);
	}

	DETOUR_DECL_MEMBER(bool, CRecipientFilter_IgnorePredictionCull)
	{
		return force_send_client || DETOUR_MEMBER_CALL(CRecipientFilter_IgnorePredictionCull)();
	}

	DETOUR_DECL_MEMBER(float, CTFCompoundBow_GetProjectileSpeed)
	{
		auto bow = reinterpret_cast<CTFCompoundBow *>(this);
		float speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(bow, speed, mult_projectile_speed);
		speed *= DETOUR_MEMBER_CALL(CTFCompoundBow_GetProjectileSpeed)();
		return speed;
	}

	DETOUR_DECL_MEMBER(float, CTFProjectile_EnergyRing_GetInitialVelocity)
	{
		auto proj = reinterpret_cast<CTFProjectile_EnergyRing *>(this);
		float speed = 1.0f;
		if (proj->GetOriginalLauncher() != nullptr) {
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(proj->GetOriginalLauncher(), speed, mult_projectile_speed);
		}
		return speed * DETOUR_MEMBER_CALL(CTFProjectile_EnergyRing_GetInitialVelocity)();
	}

	DETOUR_DECL_MEMBER(CBaseEntity *, CTFWeaponBaseGun_FireEnergyBall, CTFPlayer *player, bool ring)
	{
		auto proj = DETOUR_MEMBER_CALL(CTFWeaponBaseGun_FireEnergyBall)(player, ring);
		if (ring && proj != nullptr) {
			auto weapon = reinterpret_cast<CTFWeaponBaseGun *>(this);
			Vector vel = proj->GetAbsVelocity();
			float speed = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, speed, mult_projectile_speed);
			proj->SetAbsVelocity(vel * speed);
		}
		return proj;
	}

	DETOUR_DECL_MEMBER(float, CTFCrossbow_GetProjectileSpeed)
	{
		auto bow = reinterpret_cast<CTFWeaponBase *>(this);
		float speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(bow, speed, mult_projectile_speed);
		return speed * DETOUR_MEMBER_CALL(CTFCrossbow_GetProjectileSpeed)();
	}

	DETOUR_DECL_MEMBER(float, CTFGrapplingHook_GetProjectileSpeed)
	{
		auto bow = reinterpret_cast<CTFWeaponBase *>(this);
		float speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(bow, speed, mult_projectile_speed);
		return speed * DETOUR_MEMBER_CALL(CTFGrapplingHook_GetProjectileSpeed)();
	}

	DETOUR_DECL_MEMBER(float, CTFShotgunBuildingRescue_GetProjectileSpeed)
	{
		auto bow = reinterpret_cast<CTFWeaponBase *>(this);
		float speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(bow, speed, mult_projectile_speed);
		return speed * DETOUR_MEMBER_CALL(CTFShotgunBuildingRescue_GetProjectileSpeed)();
	}

	DETOUR_DECL_MEMBER(int, CTFWeaponBase_GetDamageType)
	{
		int value = DETOUR_MEMBER_CALL(CTFWeaponBase_GetDamageType)();

		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		if ((value & DMG_USE_HITLOCATIONS) == 0) {
			int headshot = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, headshot, can_headshot);
			if (headshot) {
				value |= DMG_USE_HITLOCATIONS;
			}
		}
		else
		{
			int noheadshot = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, noheadshot, cannot_headshot);
			if (noheadshot) {
				value &= ~DMG_USE_HITLOCATIONS;
			}
		}

		if ((value & DMG_USEDISTANCEMOD) != 0) {
			int nofalloff= 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, nofalloff, no_damage_falloff);
			if (nofalloff) {
				value &= ~DMG_USEDISTANCEMOD;
			}
		}
		else
		{ 
			int falloff= 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, falloff, force_damage_falloff);
			if (falloff) {
				value |= DMG_USEDISTANCEMOD;
			}
		}

		if ((value & DMG_NOCLOSEDISTANCEMOD) != 0) {
			int noclosedistancemod= 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, noclosedistancemod, no_reduced_damage_rampup);
			if (noclosedistancemod) {
				value &= ~DMG_NOCLOSEDISTANCEMOD;
			}
		}
		else
		{ 
			int closedistancemod= 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, closedistancemod, reduced_damage_rampup);
			if (closedistancemod) {
				value |= DMG_NOCLOSEDISTANCEMOD;
			}
		}

		if ((value & DMG_DONT_COUNT_DAMAGE_TOWARDS_CRIT_RATE) == 0) {
			int crit_rate_damage = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, crit_rate_damage, dont_count_damage_towards_crit_rate);
			if (crit_rate_damage) {
				value |= DMG_DONT_COUNT_DAMAGE_TOWARDS_CRIT_RATE;
			}
		}

		/*if (info.GetWeapon() != nullptr) {
			auto weapon = rtti_cast<CTFWeaponBase *>(info.GetWeapon());
			if (weapon != nullptr) {
				int headshot = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, headshot, can_headshot);
				if (headshot) {
					CTakeDamageInfo info2 = info;
					info2.SetDamageType(can_headshot);
				}
			}
		}*/
		return value;
	}

	bool CanHeadshot(CBaseProjectile *projectile) {
		CBaseEntity *shooter = projectile->GetOriginalLauncher();

		if (shooter != nullptr) {
			int headshot = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(shooter, headshot, can_headshot);
			DevMsg("can headshot %d",headshot);
			return headshot != 0;
		}
		return false;
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_HealingBolt_CanHeadshot)
	{
		return CanHeadshot(reinterpret_cast<CBaseProjectile *>(this)) || DETOUR_MEMBER_CALL(CTFProjectile_HealingBolt_CanHeadshot)();
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_EnergyRing_CanHeadshot)
	{
		return CanHeadshot(reinterpret_cast<CBaseProjectile *>(this)) || DETOUR_MEMBER_CALL(CTFProjectile_EnergyRing_CanHeadshot)();
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_GrapplingHook_CanHeadshot)
	{
		return CanHeadshot(reinterpret_cast<CBaseProjectile *>(this)) || DETOUR_MEMBER_CALL(CTFProjectile_GrapplingHook_CanHeadshot)();
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_EnergyBall_CanHeadshot)
	{
		return CanHeadshot(reinterpret_cast<CBaseProjectile *>(this)) || DETOUR_MEMBER_CALL(CTFProjectile_EnergyBall_CanHeadshot)();
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Arrow_CanHeadshot)
	{
		auto projectile = reinterpret_cast<CBaseProjectile *>(this);
		if (CanHeadshot(projectile))
			return true;

		CBaseEntity *shooter = projectile->GetOriginalLauncher();

		if (shooter != nullptr) {
			int headshot = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(shooter, headshot, cannot_headshot);
			if (headshot == 1)
				return false;
		}

		return DETOUR_MEMBER_CALL(CTFProjectile_Arrow_CanHeadshot)();
	}

	DETOUR_DECL_MEMBER(int, CTFGameRules_ApplyOnDamageModifyRules, CTakeDamageInfo& info, CBaseEntity *pVictim, bool b1)
	{

		int ret = DETOUR_MEMBER_CALL(CTFGameRules_ApplyOnDamageModifyRules)(info, pVictim, b1);
		if ((info.GetDamageType() & DMG_CRITICAL) && info.GetWeapon() != nullptr) {
			auto weapon = rtti_cast<CTFWeaponBase *>(info.GetWeapon());
			if (weapon != nullptr) {
				float crit = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, crit, mult_crit_dmg);
				float dmg = info.GetDamage();
				info.SetDamageBonus(dmg * crit - (dmg - info.GetDamageBonus()));
				info.SetDamage(dmg * crit);
			}
			
		}

		return ret;
	}

	//Allow can holster while spinning to work with firing minigun
	DETOUR_DECL_MEMBER(bool, CTFMinigun_CanHolster) {
		auto minigun = reinterpret_cast<CTFMinigun *>(this);
		bool firing = minigun->m_iWeaponState == CTFMinigun::AC_STATE_FIRING;
		if (firing)
			minigun->m_iWeaponState = CTFMinigun::AC_STATE_SPINNING;
		bool ret = DETOUR_MEMBER_CALL(CTFMinigun_CanHolster)();
		if (firing)
			minigun->m_iWeaponState = CTFMinigun::AC_STATE_FIRING;
			
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CObjectSapper_ApplyRoboSapperEffects, CTFPlayer *target, float duration) {
		int cannotApply = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(target, cannotApply, cannot_be_sapped);
		if (!cannotApply)
			DETOUR_MEMBER_CALL(CObjectSapper_ApplyRoboSapperEffects)(target, duration);
	}

	DETOUR_DECL_MEMBER(bool, CObjectSapper_IsParentValid) {
		bool ret = DETOUR_MEMBER_CALL(CObjectSapper_IsParentValid)();
		if (ret) {
			CTFPlayer * player = ToTFPlayer(reinterpret_cast<CBaseObject *>(this)->GetBuiltOnEntity());
			if (player != nullptr) {
				int cannotApply = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(player, cannotApply, cannot_be_sapped);
				if (cannotApply)
					ret = false;
			}
		}

		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsAllowedToTaunt) {
		bool ret = DETOUR_MEMBER_CALL(CTFPlayer_IsAllowedToTaunt)();
		auto player = reinterpret_cast<CTFPlayer *>(this);

		if (ret) {

			int cannotTaunt = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, cannotTaunt, cannot_taunt);
			if (cannotTaunt)
				ret = false;
		}
		else if (player->IsAlive()) {

			int allowTaunt = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, allowTaunt, always_allow_taunt);

			if (allowTaunt > 1 || (allowTaunt == 1 && !player->m_Shared->InCond(TF_COND_TAUNTING)))
				ret = true;
		}

		return ret;
	}

	DETOUR_DECL_MEMBER(void, CUpgrades_UpgradeTouch, CBaseEntity *pOther)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			CTFPlayer *player = ToTFPlayer(pOther);
			if (player != nullptr) {
				int cannotUpgrade = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(player,cannotUpgrade,cannot_upgrade);
				if (cannotUpgrade) {
					gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, "The Upgrade Station is disabled!");
					return;
				}
			}
		}
		
		DETOUR_MEMBER_CALL(CUpgrades_UpgradeTouch)(pOther);
	}

	void OnWeaponUpdate(CTFWeaponBase *weapon) {
		CTFPlayer *owner = ToTFPlayer(weapon->GetOwnerEntity());
		if (owner != nullptr) {
			int alwaysCrit = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon,alwaysCrit,always_crit);
			if (alwaysCrit) {
				owner->m_Shared->AddCond(TF_COND_CRITBOOSTED_CARD_EFFECT, 0.25f, nullptr);
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ItemPostFrame)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		OnWeaponUpdate(weapon);
		
		DETOUR_MEMBER_CALL(CTFWeaponBase_ItemPostFrame)();
	}
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ItemBusyFrame)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		OnWeaponUpdate(weapon);

		DETOUR_MEMBER_CALL(CTFWeaponBase_ItemBusyFrame)();
	}

	DETOUR_DECL_MEMBER(bool, CObjectSentrygun_FireRocket)
	{
		bool ret = DETOUR_MEMBER_CALL(CObjectSentrygun_FireRocket)();
		if (ret) {
			auto sentrygun = reinterpret_cast<CObjectSentrygun *>(this);
			CTFPlayer *owner = sentrygun->GetBuilder();
			if (owner != nullptr) {
				float fireRocketRate = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(owner,fireRocketRate,mult_firerocket_rate);
				if (fireRocketRate != 1.0f) {
					sentrygun->m_flNextRocketFire = (sentrygun->m_flNextRocketFire - gpGlobals->curtime) * fireRocketRate + gpGlobals->curtime;
				}
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CBaseObject_CanBeUpgraded, CTFPlayer *player)
	{
		bool ret = DETOUR_MEMBER_CALL(CBaseObject_CanBeUpgraded)(player);
		if (ret) {
			auto obj = reinterpret_cast<CBaseObject *>(this);
			CTFPlayer *owner = obj->GetBuilder();
			if (owner != nullptr) {
				int maxLevel = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(owner,maxLevel,building_max_level);
				if (maxLevel != 0) {
					return obj->m_iUpgradeLevel < maxLevel;
				}
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CBaseObject_StartUpgrading)
	{
		auto obj = reinterpret_cast<CBaseObject *>(this);
		CTFPlayer *owner = obj->GetBuilder();
		if (owner != nullptr) {
			int maxLevel = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(owner,maxLevel,building_max_level);
			if (maxLevel > 0 && obj->m_iUpgradeLevel >= maxLevel) {
				servertools->SetKeyValue(obj, "defaultupgrade", CFmtStr("%d", maxLevel - 1));
				return;
			}
		}
		DETOUR_MEMBER_CALL(CBaseObject_StartUpgrading)();
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_SentryThink)
	{
		auto obj = reinterpret_cast<CObjectSentrygun *>(this);
		CTFPlayer *owner = obj->GetBuilder();
		DETOUR_MEMBER_CALL(CObjectSentrygun_SentryThink)();
		if (owner != nullptr) {
			int rapidTick = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(owner,rapidTick,sentry_rapid_fire);
			if (rapidTick > 0) {
				//static BASEPTR addr = reinterpret_cast<BASEPTR> (AddrManager::GetAddr("CObjectSentrygun::SentryThink"));
				obj->ThinkSet(&CObjectSentrygun::SentryThink, gpGlobals->curtime, "SentrygunContext");
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CBaseObject_StartBuilding, CBaseEntity *builder)
	{
		DETOUR_MEMBER_CALL(CBaseObject_StartBuilding)(builder);
		auto obj = reinterpret_cast<CBaseObject *>(this);
		CTFPlayer *owner = obj->GetBuilder();
		if (owner != nullptr) {
			int color = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( owner, color, building_color_rgb);
			if (color != 0) {
				obj->SetRenderColorR(color >> 16);
				obj->SetRenderColorG((color >> 8) & 255);
				obj->SetRenderColorB(color & 255);
			}
			float scale = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( owner, scale, building_scale);
			if (scale != 1.0f) {
				obj->SetModelScale(scale);
			}
		}
	}

	DETOUR_DECL_MEMBER(bool, CTFGameRules_FPlayerCanTakeDamage, CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo& info)
	{
		if (pPlayer != nullptr && pAttacker != nullptr && pPlayer->GetTeamNumber() == pAttacker->GetTeamNumber()) {
			int friendly = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, friendly, allow_friendly_fire);
			if (friendly != 0)
				return true;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, friendly, receive_friendly_fire);
			if (friendly != 0)
				return true;
		}
		
		return DETOUR_MEMBER_CALL(CTFGameRules_FPlayerCanTakeDamage)(pPlayer, pAttacker, info);
	}
	
	int friendy_fire_cache_tick = 0;
	std::unordered_map<CTFPlayer *, bool> friendy_fire_cache;

	DETOUR_DECL_MEMBER(bool, CTFPlayer_WantsLagCompensationOnEntity, const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		
		if ( gpGlobals->tickcount - friendy_fire_cache_tick > 7 || gpGlobals->tickcount < friendy_fire_cache_tick ) {
			friendy_fire_cache_tick = gpGlobals->tickcount;
			friendy_fire_cache.clear();
		}
		auto result = DETOUR_MEMBER_CALL(CTFPlayer_WantsLagCompensationOnEntity)(pPlayer, pCmd, pEntityTransmitBits);
		
		if (!result && player->GetTeamNumber() == pPlayer->GetTeamNumber()) {
			auto entry = friendy_fire_cache.find(player);

			if (entry == friendy_fire_cache.end()) {
				int friendly = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( player, friendly, allow_friendly_fire);
				result = friendly != 0;
				friendy_fire_cache[player] = result;
			}
			else{
				result = entry->second;
			}
		}
		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_StunPlayer, float duration, float slowdown, int flags, CTFPlayer *attacker)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		auto player = shared->GetOuter();

		float stun = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( player, stun, mult_stun_resistance);
		if (stun != 1.0f) {
			slowdown *= stun;
		}

		DETOUR_MEMBER_CALL(CTFPlayerShared_StunPlayer)(duration, slowdown, flags, attacker);
	}

	CTFGrenadePipebombProjectile *grenade_proj;
	DETOUR_DECL_MEMBER(void, CTFGrenadePipebombProjectile_VPhysicsCollision, int index, void *pEvent)
	{
		grenade_proj = reinterpret_cast<CTFGrenadePipebombProjectile *>(this);
		DETOUR_MEMBER_CALL(CTFGrenadePipebombProjectile_VPhysicsCollision)(index, pEvent);
		grenade_proj = nullptr;
	}

	DETOUR_DECL_STATIC(bool, PropDynamic_CollidesWithGrenades, CBaseEntity *ent)
	{
		if (grenade_proj != nullptr && grenade_proj->GetOriginalLauncher() != nullptr) {
			int explode_impact = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(grenade_proj->GetOriginalLauncher(), explode_impact, grenade_explode_on_impact);
			if (explode_impact != 0)
				return true;
		}

		return DETOUR_STATIC_CALL(PropDynamic_CollidesWithGrenades)(ent);
	}
	
	RefCount rc_CTFPlayer_RegenThink;
	DETOUR_DECL_MEMBER(void, CTFPlayer_RegenThink)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_RegenThink);
		DETOUR_MEMBER_CALL(CTFPlayer_RegenThink)();
	}

	DETOUR_DECL_MEMBER(float, CTFGameRules_ApplyOnDamageAliveModifyRules, CTakeDamageInfo &info, CBaseEntity *entity, void *extras)
	{
		if (rc_CTFPlayer_RegenThink) {
			
			int iSuicideCounter = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER ( entity, iSuicideCounter, is_suicide_counter );
			if (iSuicideCounter != 0) {
				info.AddDamageType(DMG_PREVENT_PHYSICS_FORCE);
				return info.GetDamage();
			}
		}
		return DETOUR_MEMBER_CALL(CTFGameRules_ApplyOnDamageAliveModifyRules)(info, entity, extras);
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage, CTakeDamageInfo &info)
	{
		int damage = DETOUR_MEMBER_CALL(CTFPlayer_OnTakeDamage)(info);

		//Non sniper rifle explosive headshot
		auto weapon = info.GetWeapon();
		if (weapon != nullptr && info.GetAttacker() != nullptr && (info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT || info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT_DECAPITATION)) {
			auto tfweapon = rtti_cast<CTFWeaponBase *>(weapon);
			auto player = reinterpret_cast<CTFPlayer *>(this);
			auto attacker = ToTFPlayer(info.GetAttacker());

			if (tfweapon != nullptr && !WeaponID_IsSniperRifle(tfweapon->GetWeaponID())) {
				int iExplosiveShot = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER ( tfweapon, iExplosiveShot, explosive_sniper_shot );
				if (iExplosiveShot != 0) {
					reinterpret_cast<CTFSniperRifle *>(tfweapon)->ExplosiveHeadShot(attacker, player);
				}
			}
		}
		return damage;
	}

	DETOUR_DECL_MEMBER(bool, static_attrib_t_BInitFromKV_SingleLine, const char *context, KeyValues *attribute, CUtlVector<CUtlString> *errors, bool b)
	{
		if (V_strnicmp(attribute->GetName(), "SET BONUS: ", strlen("SET BONUS: ")) == 0) {
			attribute->SetName(attribute->GetName() + strlen("SET BONUS: "));
		}

		return DETOUR_MEMBER_CALL(static_attrib_t_BInitFromKV_SingleLine)(context, attribute, errors, b);
	}
	
	int should_hit_entity_cache_tick = 0;
	std::unordered_map<CBaseEntity *, bool> should_hit_entity_cache;

	DETOUR_DECL_MEMBER(bool, CTraceFilterObject_ShouldHitEntity, IHandleEntity *pServerEntity, int contentsMask)
	{
		CTraceFilterSimple *filter = reinterpret_cast<CTraceFilterSimple*>(this);
		if ( gpGlobals->tickcount - should_hit_entity_cache_tick > 7 || gpGlobals->tickcount < should_hit_entity_cache_tick ) {
			should_hit_entity_cache_tick = gpGlobals->tickcount;
			should_hit_entity_cache.clear();
		}

		bool result = DETOUR_MEMBER_CALL(CTraceFilterObject_ShouldHitEntity)(pServerEntity, contentsMask);
		
		if (result) {
			CBaseEntity *entityme = const_cast< CBaseEntity * >(EntityFromEntityHandle(filter->GetPassEntity()));
			CBaseEntity *entityhit = EntityFromEntityHandle(pServerEntity);

			bool entityme_player = entityme->IsPlayer();
			bool entityhit_player = entityhit->IsPlayer();

			if (!entityme_player || (!entityhit_player && !entityhit->IsBaseObject()))
				return true;

			bool me_collide = true;
			bool hit_collide = true;

			auto entry = should_hit_entity_cache.find(entityme);
			if (entry == should_hit_entity_cache.end()) {
				int not_solid = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( entityme, not_solid, not_solid_to_players);
				me_collide = not_solid == 0;
				should_hit_entity_cache[entityme] = me_collide;
			}
			else{
				me_collide = entry->second;
			}

			if (!me_collide)
				return false;

			if (entityhit_player) {
				auto entry = should_hit_entity_cache.find(entityhit);
				if (entry == should_hit_entity_cache.end()) {
					int not_solid = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( entityhit, not_solid, not_solid_to_players);
					hit_collide = not_solid == 0;
					should_hit_entity_cache[entityhit] = hit_collide;
				}
				else{
					hit_collide = entry->second;
				}
			}

			return hit_collide;
		}	 

		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_CancelTaunt)
	{
		int iCancelTaunt = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER ( reinterpret_cast<CTFPlayer *>(this), iCancelTaunt, always_allow_taunt );
		if (iCancelTaunt != 0) {
			return;
		}
		return DETOUR_MEMBER_CALL(CTFPlayer_CancelTaunt)();
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsAllowedToInitiateTauntWithPartner, CEconItemView *pEconItemView, char *pszErrorMessage, int cubErrorMessage)
	{
		int iAllowTaunt = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER ( reinterpret_cast<CTFPlayer *>(this), iAllowTaunt, always_allow_taunt );
		if (iAllowTaunt != 0) {
			return true;
		}
		return DETOUR_MEMBER_CALL(CTFPlayer_IsAllowedToInitiateTauntWithPartner)( pEconItemView, pszErrorMessage, cubErrorMessage);
	}

	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_DeflectEntity, CBaseEntity *pTarget, CTFPlayer *pOwner, Vector &vecForward, Vector &vecCenter, Vector &vecSize)
	{
		auto result = DETOUR_MEMBER_CALL(CTFWeaponBase_DeflectEntity)(pTarget, pOwner, vecForward, vecCenter, vecSize);
		if (result) {
			float deflectStrength = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( reinterpret_cast<CTFPlayer *>(this), deflectStrength, mult_reflect_velocity );
			if (deflectStrength != 1.0f) {
				IPhysicsObject *physics = pTarget->VPhysicsGetObject();

				if (physics != nullptr) {
					AngularImpulse ang_imp;
					Vector vel;
					physics->GetVelocity(&vel, &ang_imp);
					vel *= deflectStrength;
					physics->SetVelocity(&vel, &ang_imp);
				}
				else {
					Vector vel = pTarget->GetAbsVelocity();
					vel *= deflectStrength;
					pTarget->SetAbsVelocity(vel);
				}

			}
		}
		return result;
	}

	DETOUR_DECL_MEMBER(const char *, CTFGameRules_GetKillingWeaponName, const CTakeDamageInfo &info, CTFPlayer *pVictim, int *iWeaponID)
	{
		if (info.GetWeapon() != nullptr && info.GetAttacker() != nullptr && info.GetAttacker()->IsPlayer()) {
			CTFWeaponBase *weapon = rtti_cast<CTFWeaponBase *>(info.GetWeapon()->MyCombatWeaponPointer());
			if (weapon != nullptr) {
				const char *str = GetStringAttribute(weapon->GetItem()->GetAttributeList(), "custom kill icon");
				if (str != nullptr)
					return str;
			}
		}
		return DETOUR_MEMBER_CALL(CTFGameRules_GetKillingWeaponName)(info, pVictim, iWeaponID);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_Touch, CBaseEntity *toucher)
	{
		DETOUR_MEMBER_CALL(CTFPlayer_Touch)(toucher);
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (toucher->IsBaseObject()) {
			float stomp = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( player, stomp, stomp_building_damage );
			if (stomp != 0.0f) {
				CTakeDamageInfo info(player, player, player->GetActiveTFWeapon(), vec3_origin, vec3_origin, stomp, DMG_BLAST);
				toucher->TakeDamage(info);
			}
		}
		else if (toucher->IsPlayer()) {
			float stomp = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( player, stomp, stomp_player_damage );
			if (stomp != 0.0f) {
				CTakeDamageInfo info(player, player, player->GetActiveTFWeapon(), vec3_origin, vec3_origin, stomp, DMG_BLAST);
				toucher->TakeDamage(info);
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CUpgrades_PlayerPurchasingUpgrade, CTFPlayer *player, int itemslot, int upgradeslot, bool sell, bool free, bool b3)
	{
		if (!b3) {
			auto upgrade = reinterpret_cast<CUpgrades *>(this);
			
			if (itemslot >= 0 && upgradeslot >= 0 && upgradeslot < CMannVsMachineUpgradeManager::Upgrades().Count()) {
				
				CEconEntity *entity = GetEconEntityAtLoadoutSlot(player, itemslot);

				if (entity != nullptr) {
					int iCannotUpgrade = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER ( entity, iCannotUpgrade, cannot_be_upgraded );
					if (iCannotUpgrade > 0) {
						if (!sell) {
							gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, "This weapon is not upgradeable");
						}
						return;
					}
				}
			}
		}
		DETOUR_MEMBER_CALL(CUpgrades_PlayerPurchasingUpgrade)(player, itemslot, upgradeslot, sell, free, b3);
	}
	
	RefCount rc_CTFPlayer_ReapplyItemUpgrades;
	DETOUR_DECL_MEMBER(void, CTFPlayer_ReapplyItemUpgrades, CEconItemView *item_view)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (item_view != nullptr) {
			auto attr = item_view->GetAttributeList().GetAttributeByName("cannot be upgraded");
			SCOPED_INCREMENT_IF(rc_CTFPlayer_ReapplyItemUpgrades, attr != nullptr && attr->GetValuePtr()->m_Float > 0.0f);
		}
		DETOUR_MEMBER_CALL(CTFPlayer_ReapplyItemUpgrades)(item_view);
	}

	DETOUR_DECL_MEMBER(attrib_definition_index_t, CUpgrades_ApplyUpgradeToItem, CTFPlayer* player, CEconItemView *item, int upgrade, int cost, bool downgrade, bool fresh)
	{
		if (rc_CTFPlayer_ReapplyItemUpgrades)
		{
			return INVALID_ATTRIB_DEF_INDEX;
		}

        return DETOUR_MEMBER_CALL(CUpgrades_ApplyUpgradeToItem)(player, item, upgrade, cost, downgrade, fresh);
    }

	bool IsDeflectable(CBaseProjectile *proj) {
		if (proj->GetOriginalLauncher() == nullptr)
			return true;

		int iCannotReflect = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(proj->GetOriginalLauncher(), iCannotReflect, projectile_no_deflect);
		if (iCannotReflect != 0)
			return false;

		return true;
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Rocket_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_Rocket *>(this);

		return DETOUR_MEMBER_CALL(CTFProjectile_Rocket_IsDeflectable)() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Arrow_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_Arrow *>(this);

		return DETOUR_MEMBER_CALL(CTFProjectile_Arrow_IsDeflectable)() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Flare_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_Flare *>(this);

		return DETOUR_MEMBER_CALL(CTFProjectile_Flare_IsDeflectable)() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_EnergyBall_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_EnergyBall *>(this);

		return DETOUR_MEMBER_CALL(CTFProjectile_EnergyBall_IsDeflectable)() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFGrenadePipebombProjectile_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFGrenadePipebombProjectile *>(this);

		return DETOUR_MEMBER_CALL(CTFGrenadePipebombProjectile_IsDeflectable)() && IsDeflectable(ent);
	}

	// Stop short circuit from deflecting the projectile
	
	RefCount rc_CTFProjectile_MechanicalArmOrb_CheckForProjectiles;
	DETOUR_DECL_MEMBER(void, CTFProjectile_MechanicalArmOrb_CheckForProjectiles)
	{
		SCOPED_INCREMENT(rc_CTFProjectile_MechanicalArmOrb_CheckForProjectiles);
		DETOUR_MEMBER_CALL(CTFProjectile_MechanicalArmOrb_CheckForProjectiles)();
	}

	DETOUR_DECL_MEMBER(bool, CBaseEntity_InSameTeam, CBaseEntity *other)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		if (rc_CTFProjectile_MechanicalArmOrb_CheckForProjectiles)
		{
			if (!IsDeflectable(static_cast<CBaseProjectile *>(ent))) {
				return true;
			}
		}
/*
		if ((ent->m_fFlags & FL_GRENADE) && (other->m_fFlags & FL_GRENADE) && strncmp(ent->GetClassname(), "tf_projectile", strlen("tf_projectile")) == 0) {
			if (!IsDeflectable(static_cast<CBaseProjectile *>(ent))) {
				return true;
			}
		}*/

		return DETOUR_MEMBER_CALL(CBaseEntity_InSameTeam)(other);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnAddBalloonHead)
	{
        DETOUR_MEMBER_CALL(CTFPlayerShared_OnAddBalloonHead)();
		CTFPlayer *player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();

		float gravity = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, gravity, player_gravity_ballon_head);
		if (gravity != 0.0f)
			player->SetGravity(gravity);
	}

	
	/*void OnAttributesChange(CAttributeManager *mgr)
	{
		CBaseEntity *outer = mgr->m_hOuter;

		if (outer != nullptr) {
			CTFPlayer *owner = ToTFPlayer(outer->IsPlayer() ? outer : outer->GetOwnerEntity());
			CAttributeManager ownermgr = outer->IsPlayer() ? mgr : (owner != nullptr ? owner->GetAttributeManager() : nullptr)
			if (ownermgr != nullptr)
			{
				float gravity = ownermgr->ApplyAttributeFloatWrapper(1.0f, owner, AllocPooledString_StaticConstantStringPointer("player_gravity"));
				if (gravity != 1.0f)
					outer->SetGravity(gravity);
				DevMsg("gravity %f\n", gravity);
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CAttributeManager_OnAttributeValuesChanged)
	{
        DETOUR_MEMBER_CALL(CAttributeManager_OnAttributeValuesChanged)();
        auto mgr = reinterpret_cast<CAttributeManager *>(this);
		OnAttributesChange(mgr);
	}
	DETOUR_DECL_MEMBER(void, CAttributeContainer_OnAttributeValuesChanged)
	{
        DETOUR_MEMBER_CALL(CAttributeContainer_OnAttributeValuesChanged)();
        auto mgr = reinterpret_cast<CAttributeManager *>(this);
		OnAttributesChange(mgr);
	}
	DETOUR_DECL_MEMBER(void, CAttributeContainerPlayer_OnAttributeValuesChanged)
	{
        DETOUR_MEMBER_CALL(CAttributeContainerPlayer_OnAttributeValuesChanged)();
        auto mgr = reinterpret_cast<CAttributeManager *>(this);
		OnAttributesChange(mgr);
	}*/


	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Attr:Custom_Attributes")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanAirDash, "CTFPlayer::CanAirDash");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_AllowedToHealTarget, "CWeaponMedigun::AllowedToHealTarget");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_HealTargetThink, "CWeaponMedigun::HealTargetThink");
			MOD_ADD_DETOUR_MEMBER(CTFCompoundBow_LaunchGrenade, "CTFCompoundBow::LaunchGrenade");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireProjectile, "CTFWeaponBaseGun::FireProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_RemoveProjectileAmmo, "CTFWeaponBaseGun::RemoveProjectileAmmo");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_UpdatePunchAngles, "CTFWeaponBaseGun::UpdatePunchAngles");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_Swing, "CTFWeaponBaseMelee::Swing");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_OnTakeDamage, "CBaseObject::OnTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Event_Killed, "CTFPlayer::Event_Killed");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CreateRagdollEntity, "CTFPlayer::CreateRagdollEntity [args]");
			MOD_ADD_DETOUR_MEMBER(CTFStickBomb_Smack, "CTFStickBomb::Smack");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_RadiusDamage, "CTFGameRules::RadiusDamage");
			MOD_ADD_DETOUR_MEMBER(CTFGameMovement_CheckJumpButton, "CTFGameMovement::CheckJumpButton");
			MOD_ADD_DETOUR_MEMBER(CTFGameMovement_ProcessMovement, "CTFGameMovement::ProcessMovement");
			MOD_ADD_DETOUR_MEMBER(CEconEntity_UpdateModelToClass, "CEconEntity::UpdateModelToClass");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetShootSound, "CTFWeaponBase::GetShootSound");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_DispatchTraceAttack,    "CBaseEntity::DispatchTraceAttack");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_FireBullet,    "CTFPlayer::FireBullet");
			MOD_ADD_DETOUR_STATIC(TE_TFExplosion,    "TE_TFExplosion");
			MOD_ADD_DETOUR_MEMBER(CTFCompoundBow_GetProjectileSpeed,    "CTFCompoundBow::GetProjectileSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireEnergyBall,    "CTFWeaponBaseGun::FireEnergyBall");
			MOD_ADD_DETOUR_MEMBER(CTFCrossbow_GetProjectileSpeed,    "CTFCrossbow::GetProjectileSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFGrapplingHook_GetProjectileSpeed,    "CTFGrapplingHook::GetProjectileSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFShotgunBuildingRescue_GetProjectileSpeed,    "CTFShotgunBuildingRescue::GetProjectileSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGrenadeProj_Explode,    "CTFWeaponBaseGrenadeProj::Explode");
			MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Explode,    "CTFBaseRocket::Explode");
			MOD_ADD_DETOUR_MEMBER(CRecipientFilter_IgnorePredictionCull,    "CRecipientFilter::IgnorePredictionCull");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetDamageType,    "CTFWeaponBase::GetDamageType");
			MOD_ADD_DETOUR_MEMBER(CTFMinigun_CanHolster,    "CTFMinigun::CanHolster");
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_ApplyRoboSapperEffects,    "CObjectSapper::ApplyRoboSapperEffects");
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_IsParentValid,    "CObjectSapper::IsParentValid");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsAllowedToTaunt,    "CTFPlayer::IsAllowedToTaunt");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_UpgradeTouch, "CUpgrades::UpgradeTouch");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ItemPostFrame, "CTFWeaponBase::ItemPostFrame");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ItemBusyFrame, "CTFWeaponBase::ItemBusyFrame");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_FireRocket, "CObjectSentrygun::FireRocket");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_StartUpgrading, "CBaseObject::StartUpgrading");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_CanBeUpgraded, "CBaseObject::CanBeUpgraded");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_SentryThink, "CObjectSentrygun::SentryThink");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_StartBuilding, "CBaseObject::StartBuilding");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_WantsLagCompensationOnEntity, "CTFPlayer::WantsLagCompensationOnEntity");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FPlayerCanTakeDamage, "CTFGameRules::FPlayerCanTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_StunPlayer, "CTFPlayerShared::StunPlayer");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_HealingBolt_CanHeadshot, "CTFProjectile_HealingBolt::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_GrapplingHook_CanHeadshot, "CTFProjectile_GrapplingHook::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_CanHeadshot, "CTFProjectile_EnergyBall::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyRing_CanHeadshot, "CTFProjectile_EnergyRing::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_CanHeadshot, "CTFProjectile_Arrow::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageModifyRules, "CTFGameRules::ApplyOnDamageModifyRules");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_Equip, "CBaseCombatWeapon::Equip");
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_VPhysicsCollision, "CTFGrenadePipebombProjectile::VPhysicsCollision");
			MOD_ADD_DETOUR_STATIC(PropDynamic_CollidesWithGrenades, "PropDynamic_CollidesWithGrenades");
			MOD_ADD_DETOUR_MEMBER(CTraceFilterObject_ShouldHitEntity, "CTraceFilterObject::ShouldHitEntity");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_RegenThink, "CTFPlayer::RegenThink");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageAliveModifyRules, "CTFGameRules::ApplyOnDamageAliveModifyRules");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CancelTaunt, "CTFPlayer::CancelTaunt");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsAllowedToInitiateTauntWithPartner, "CTFPlayer::IsAllowedToInitiateTauntWithPartner");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_DeflectEntity, "CTFWeaponBase::DeflectEntity");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetKillingWeaponName, "CTFGameRules::GetKillingWeaponName");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Touch, "CTFPlayer::Touch");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_PlayerPurchasingUpgrade, "CUpgrades::PlayerPurchasingUpgrade");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ReapplyItemUpgrades, "CTFPlayer::ReapplyItemUpgrades");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_ApplyUpgradeToItem, "CUpgrades::ApplyUpgradeToItem");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_WeaponSound, "CBaseCombatWeapon::WeaponSound");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_IsDeflectable, "CTFProjectile_Rocket::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_IsDeflectable, "CTFProjectile_Arrow::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Flare_IsDeflectable, "CTFProjectile_Flare::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_IsDeflectable, "CTFProjectile_EnergyBall::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_IsDeflectable, "CTFGrenadePipebombProjectile::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_InSameTeam, "CBaseEntity::InSameTeam");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnAddBalloonHead, "CTFPlayerShared::OnAddBalloonHead");
			//MOD_ADD_DETOUR_MEMBER(CAttributeManager_OnAttributeValuesChanged, "CAttributeManager::OnAttributeValuesChanged");
			//MOD_ADD_DETOUR_MEMBER(CAttributeManager_OnAttributeValuesChanged, "CAttributeManager::OnAttributeValuesChanged");
			//MOD_ADD_DETOUR_MEMBER(CAttributeContainer_OnAttributeValuesChanged, "CAttributeContainer::OnAttributeValuesChanged");
			//MOD_ADD_DETOUR_MEMBER(CAttributeContainerPlayer_OnAttributeValuesChanged, "CAttributeContainerPlayer::OnAttributeValuesChanged");
			//MOD_ADD_DETOUR_STATIC(CBaseEntity_EmitSound, "CBaseEntity::EmitSound [static: normal]");
			
			
			
		//	Allow explosive headshots on anything
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage, "CTFPlayer::OnTakeDamage");

		//	Allow set bonus attributes on items, with the help of custom attributes
			MOD_ADD_DETOUR_MEMBER(static_attrib_t_BInitFromKV_SingleLine, "static_attrib_t::BInitFromKV_SingleLine");
			
			this->Toggle(true);
		}

		virtual void LevelInitPostEntity() override
		{
			precached.clear();
		}
		
		virtual void LevelInitPreEntity() override
		{
			precached.clear();
			KeyValues *kv = new KeyValues("attributes");
			char path[PLATFORM_MAX_PATH];
			g_pSM->BuildPath(Path_SM,path,sizeof(path),"gamedata/sigsegv/custom_attributes.txt");
			if (kv->LoadFromFile(filesystem, path)) {
				DevMsg("Loaded attrs\n");
				CUtlVector<CUtlString> err;
				GetItemSchema()->BInitAttributes(kv, &err);
			}
		}
		virtual bool ShouldReceiveCallbacks() const override { return true; }

		virtual void OnUnload() override
		{
			precached.clear();
		}
		
		virtual void OnDisable() override
		{
			precached.clear();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_attr_custom", "0", FCVAR_NOTIFY,
		"Mod: enable custom attributes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
