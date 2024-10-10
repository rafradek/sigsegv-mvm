#include "mod.h"
#include "stub/baseentity.h"
#include "stub/econ.h"
#include "stub/extraentitydata.h"
#include "stub/tfplayer.h"
#include "stub/projectiles.h"
#include "stub/player_util.h"
#include "stub/tfweaponbase.h"
#include "stub/objects.h"
#include "stub/tfentities.h"
#include "stub/gamerules.h"
#include "stub/usermessages_sv.h"
#include "stub/particles.h"
#include "stub/misc.h"
#include "stub/nextbot_cc.h"
#include "stub/trace.h"
#include "stub/team.h"
#include "stub/upgrades.h"
#include "stub/tf_objective_resource.h"
#include "stub/server.h"
#include "mod/pop/common.h"
#include "mod/pop/popmgr_extensions.h"
#include "util/iterate.h"
#include "util/clientmsg.h"
#include "mem/protect.h"
#include <gamemovement.h>
#include <boost/tokenizer.hpp>
#include "mod/attr/custom_attributes.h"
#include "mod/item/item_common.h"
#include "mod/etc/mapentity_additions.h"
#include "mod/common/weapon_shoot.h"
#include "mod/common/commands.h"
#include <fmt/core.h>
#include "util/vi.h"


class CDmgAccumulator;

class IPredictionSystem
{
public:
	void *vtable;
	IPredictionSystem *m_pNextSystem;
	int m_bSuppressEvent;
	CBaseEntity *m_pSuppressHost;

	int m_nStatusPushed;
};

namespace Mod::Attr::Custom_Attributes
{

	std::set<std::string> precached;

	GlobalThunk<void *> g_pFullFileSystem("g_pFullFileSystem");
	GlobalThunk<IPredictionSystem> g_RecipientFilterPredictionSystem("g_RecipientFilterPredictionSystem");

	float *fast_attribute_cache[2048] {};

	CBaseEntity *last_fast_attrib_entity = nullptr;
	float *CreateNewAttributeCache(CBaseEntity *entity) {

		IHasAttributes *attributes = entity->m_pAttributes;
		if (attributes == nullptr) return nullptr;
		
		int count = entity->IsPlayer() ? (int)ATTRIB_COUNT_PLAYER : (int)ATTRIB_COUNT_ITEM;
		float *attrib_cache = new float[count];
		
		for(int i = 0; i < count; i++) {
			attrib_cache[i] = FLT_MIN;
		}

		//GetExtraEntityDataWithAttributes(entity)->fast_attribute_cache = attrib_cache;
		fast_attribute_cache[ENTINDEX(entity)] = attrib_cache;
		return attrib_cache;
	}

	float SetAttributeCacheEntry(CBaseEntity *entity, float value, int name, float *attrib_cache) {
		IHasAttributes *attributes = entity->m_pAttributes;
		if (attributes == nullptr) 
			return value;
		
		CAttributeManager *mgr = attributes->GetAttributeManager();

		float result = mgr->ApplyAttributeFloat(value, entity, AllocPooledString_StaticConstantStringPointer(entity->IsPlayer() ? fast_attribute_classes_player[name] : fast_attribute_classes_item[name]));
		attrib_cache[name] = result;
		return result;
	}

	// Fast Attribute Cache, for every tick attribute querying. The value parameter must be a static value, unlike the CALL_ATTRIB_HOOK_ calls;
	inline float GetFastAttributeFloat(CBaseEntity *entity, float value, int name) {
		
		if (entity == nullptr)
			return value;

		//auto data = static_cast<ExtraEntityDataWithAttributes *>(entity->GetExtraEntityData());
		float *attrib_cache = fast_attribute_cache[ENTINDEX(entity)];
		if (attrib_cache == nullptr && (attrib_cache = CreateNewAttributeCache(entity)) == nullptr) {
			return value;
		}
		
		float result = attrib_cache[name];
			
		if (result != FLT_MIN) {
			return result;
		}

		return SetAttributeCacheEntry(entity, value, name, attrib_cache);
	}

	inline int GetFastAttributeInt(CBaseEntity *entity, int value, int name) {
		return RoundFloatToInt(GetFastAttributeFloat(entity, value, name));
	}

	extern ConVar cvar_enable;
	float GetFastAttributeFloatExternal(CBaseEntity *entity, float value, int name) {
		return cvar_enable.GetBool() ? GetFastAttributeFloat(entity, value, name) : value;
	}

	int GetFastAttributeIntExternal(CBaseEntity *entity, int value, int name) {
		return cvar_enable.GetBool() ? GetFastAttributeInt(entity, value, name) : value;
	}

#define GET_STRING_ATTRIBUTE_LIST(attrlist, name, varname) \
	static int inddef_##varname = GetItemSchema()->GetAttributeDefinitionByName(name)->GetIndex(); \
	const char * varname = GetStringAttribute(attrlist, inddef_##varname);

#define GET_STRING_ATTRIBUTE(entity, name, varname) \
	const char * varname = entity->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, entity, PStrT<#name>()).ToCStr(); \
	if (varname[0] == '\0') varname = nullptr;

#define GET_STRING_ATTRIBUTE_NO_CACHE(entity, name, varname) \
	const char * varname = entity->GetAttributeManager()->ApplyAttributeString(NULL_STRING, entity, PStrT<#name>()).ToCStr(); \
	if (varname[0] == '\0') varname = nullptr;
	
	const char *GetStringAttribute(CAttributeList &attrlist, int index) {
		auto attr = attrlist.GetAttributeByID(index);
		const char *value = nullptr;
		if (attr != nullptr && attr->GetValuePtr()->m_String != nullptr) {
			CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &value);
		}
		return value;
	}

	const char *GetStringAttribute(CAttributeList &attrlist, const char* name) {
		auto attr = attrlist.GetAttributeByName(name);
		const char *value = nullptr;
		if (attr != nullptr && attr->GetValuePtr()->m_String != nullptr) {
			CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &value);
		}
		return value;
	}

	inline void PrecacheSound(const char *name) {
		if (name != nullptr && name[0] != '\0' && precached.count(name) == 0) {
			if (!enginesound->PrecacheSound(name, true))
				CBaseEntity::PrecacheScriptSound(name);
			precached.insert(name);
		}
	}

	enum class AttributeChangeType
	{
		NONE,
		ADD,
		UPDATE,
		REMOVE
	};
	void OnAttributeChanged(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType);

	DETOUR_DECL_MEMBER(bool, CTFPlayer_CanAirDash)
	{
		bool ret = DETOUR_MEMBER_CALL();
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
		bool ret = DETOUR_MEMBER_CALL(target);
		auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
		auto owner = medigun->GetOwnerEntity();

		if (!ret && target != nullptr && target->IsBaseObject()) {
			float attackEnemy = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( medigun, attackEnemy, medigun_attack_enemy );
			if (owner != nullptr && (target->GetTeamNumber() == owner->GetTeamNumber() || attackEnemy != 0)) {
				int can_heal = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( medigun, can_heal, medic_machinery_beam );
				return can_heal != 0;
			}
			
		}
		if (!ret && target != nullptr && target->IsPlayer() && target->GetTeamNumber() != owner->GetTeamNumber()) {
			float attackEnemy = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( medigun, attackEnemy, medigun_attack_enemy );
			if (attackEnemy != 0) {
				return true;
			}
		}
		return ret;
	}
	class MedigunAttackModule : public EntityModule
	{
	public:
		MedigunAttackModule(CBaseEntity *entity) : EntityModule(entity) {}

		CHandle<CBaseEntity> lastTarget;
		float attackStartTime;
		float lastAttackTime;
		float maxOverheal = 1.5f;

	};

	DETOUR_DECL_MEMBER(void, CWeaponMedigun_HealTargetThink)
	{
		auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
		auto owner = medigun->GetTFPlayerOwner();
		CBaseEntity *healobject = medigun->GetHealTarget();
		if (healobject != nullptr && healobject->GetTeamNumber() != owner->GetTeamNumber()) {
			auto attackModule = medigun->GetOrCreateEntityModule<MedigunAttackModule>("medigunattack");
			if (attackModule->lastTarget != healobject) {
				attackModule->lastTarget = healobject;
				attackModule->attackStartTime = gpGlobals->curtime;
				attackModule->lastAttackTime = gpGlobals->curtime - 0.1f;
			}
			float attackEnemy = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( medigun, attackEnemy, medigun_attack_enemy );
			float attackEnemyHealMult = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( medigun, attackEnemyHealMult, medigun_attack_enemy_heal_mult );
			float mult = RemapValClamped(gpGlobals->curtime - attackModule->attackStartTime, 0, 5, 1, 3);
			int healthPre = healobject->GetHealth();
			CTakeDamageInfo info(owner, owner, medigun, vec3_origin, vec3_origin, medigun->GetHealRate() * attackEnemy * mult * (gpGlobals->curtime - attackModule->lastAttackTime), owner->m_Shared->IsCritBoosted() ? (DMG_GENERIC | DMG_CRITICAL) : DMG_GENERIC);
			healobject->TakeDamage(info);
			if (healthPre > healobject->GetHealth()) {
				int maxBuff = owner->GetMaxHealthForBuffing() * attackModule->maxOverheal;
				if (maxBuff - owner->GetHealth() > 0) {
					owner->TakeHealth(Min(maxBuff - owner->GetHealth(), (int)((healthPre - healobject->GetHealth()) * attackEnemyHealMult)), DMG_IGNORE_MAXHEALTH);
				}
			}

			
			attackModule->lastAttackTime = gpGlobals->curtime;
		}


		if (healobject != nullptr && !healobject->IsPlayer()) {
			medigun->SetCustomVariable("healingnonplayer", Variant(true));
		}

		if (medigun->GetCustomVariableBool<"healingnonplayer">() && healobject == nullptr) {
			medigun->SetHealTarget(nullptr);
			medigun->SetCustomVariable("healingnonplayer", Variant(false));
		}

		DETOUR_MEMBER_CALL();
		if (healobject != nullptr && healobject->IsBaseObject() && healobject->GetHealth() < healobject->GetMaxHealth() && healobject->GetTeamNumber() == owner->GetTeamNumber()) {
			int can_heal = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( medigun, can_heal, medic_machinery_beam );
			auto object = ToBaseObject(healobject);
			object->SetHealth( object->GetHealth() + ( (medigun->GetHealRate() / 10.f) * can_heal ) );
			
		}
	}


	void CreateExtraArrow(CTFCompoundBow *bow, CTFProjectile_Arrow *main_arrow, const QAngle& angles, float speed, bool ignite) {

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
			if (ignite) {
				pExtraArrow->m_bArrowAlight = true;
			}
			pExtraArrow->SetCollisionGroup( main_arrow->GetCollisionGroup() );
		}
	}

	float GetRandomSpreadOffset( CTFCompoundBow *bow, int iLevel, float angle )
	{
		float flMaxRandomSpread = 2.5f;// sv_arrow_max_random_spread_angle.GetFloat();
		float flRandom = RemapValClamped( bow->GetCurrentCharge(), 0.f, bow->GetChargeMaxTime(), RandomFloat( -flMaxRandomSpread, flMaxRandomSpread ), 0.f );
		return angle/*sv_arrow_spread_angle.GetFloat()*/ * iLevel + flRandom;
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
				if ( pProjectile->ClassMatches("tf_projectile*") && pProjectile->IsDestroyable(false) )
				{
					pProjectile->Destroy( false, true );

					weapon->EmitSound( "Halloween.HeadlessBossAxeHitWorld" );
				}
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_ThrowableRepel_SetCustomPipebombModel)
	{
		auto me = reinterpret_cast<CTFProjectile_ThrowableRepel *>(this);
		me->SetModel("models/weapons/w_models/w_baseball.mdl");
	}

	void FireInputAttribute(const char *input, const char *filter, const variant_t &defValue, CBaseEntity *inflictor, CBaseEntity *activator, CBaseEntity *caller, float delay)
	{
		if (input != nullptr && (filter == nullptr || caller->NameMatches(filter) || caller->ClassMatches(filter))) {
			char input_tokenized[512];
			V_strncpy(input_tokenized, input, sizeof(input_tokenized));
			
			char *target = strtok(input_tokenized,"^");
			char *input = strtok(NULL,"^");
			char *param = strtok(NULL,"^");
			
			if (target != nullptr && input != nullptr) {
				variant_t variant1;
				if (param != nullptr) {
					string_t m_iParameter = AllocPooledString(param);
					variant1.SetString(m_iParameter);
				}
				else {
					variant1 = defValue;
				}
				
				if (FStrEq(target, "!self")) {
					caller->AcceptInput(input, activator, caller, variant1,-1);
				}
				else if (FStrEq(target, "!projectile") && inflictor != nullptr) {
					inflictor->AcceptInput(input, activator, caller, variant1,-1);
				}
				else {
					CEventQueue &que = g_EventQueue;
					que.AddEvent(STRING(AllocPooledString(target)),STRING(AllocPooledString(input)),variant1,delay,activator, caller,-1);
				}
			}
		}
	}
	
	CBaseAnimating *SpawnCustomProjectile(const char *name, CTFWeaponBaseGun *weapon, CTFPlayer *player, bool doEffect)
	{
		CBaseAnimating *retval = nullptr;
		
		Vector vecSrc;
		QAngle angForward;
		Vector vecOffset( 23.5f, 12.0f, -3.0f );
		if ( player->GetFlags() & FL_DUCKING )
		{
			vecOffset.z = 8.0f;
		}
		weapon->GetProjectileFireSetup( player, vecOffset, &vecSrc, &angForward, false ,2000);

		float mult_speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, mult_speed, mult_projectile_speed);
		if (strcmp(name, "mechanicalarmorb") == 0) {

			auto projectile = rtti_cast<CTFProjectile_MechanicalArmOrb *>(CBaseEntity::CreateNoSpawn("tf_projectile_mechanicalarmorb", vecSrc, player->EyeAngles(), player));
			if (projectile != nullptr) {
				projectile->SetOwnerEntity(player);
				projectile->SetLauncher   (weapon);

				Vector eye_angles_fwd;
				AngleVectors(angForward, &eye_angles_fwd);
				projectile->SetAbsVelocity(mult_speed * 700.0f * eye_angles_fwd);
				
				projectile->ChangeTeam(player->GetTeamNumber());
				
				if (projectile->IsCritical()) {
					projectile->SetCritical(false);
				}
				
				projectile->SetDamage( weapon->GetProjectileDamage() );
				DispatchSpawn(projectile);
				projectile->SetCustomVariable("applydmgmult", Variant(true));
			}
			retval = projectile;
		}
		else if (strcmp(name, "stunball") == 0) {

			auto projectile = CTFStunBall::Create(vecSrc, player->EyeAngles(), player);
			if (projectile != nullptr) {

				Vector eye_angles_fwd;
				AngleVectors(angForward, &eye_angles_fwd);
				static ConVarRef tf_scout_stunball_base_speed("tf_scout_stunball_base_speed");
				Vector velocity = mult_speed * tf_scout_stunball_base_speed.GetFloat() * eye_angles_fwd;
				
				projectile->InitGrenade(velocity, AngularImpulse( 0, RandomFloat( 0, 100 ), 0 ), player, weapon->GetTFWpnData());
				projectile->SetLauncher(weapon);
				projectile->SetOwnerEntity(player);
				projectile->m_nSkin = player->GetTeamNumber() == TF_TEAM_BLUE ? 1 : 0;
				if (weapon->m_bCurrentAttackIsCrit) {
					projectile->m_bCritical = true;
				}
				projectile->SetDamage( weapon->GetProjectileDamage() );
			}
			retval = projectile;
		}
		else if (strcmp(name, "ornament") == 0) {

			auto projectile = CTFBall_Ornament::Create(vecSrc, player->EyeAngles(), player);
			if (projectile != nullptr) {

				Vector eye_angles_fwd;
				AngleVectors(angForward, &eye_angles_fwd);
				static ConVarRef tf_scout_stunball_base_speed("tf_scout_stunball_base_speed");
				Vector velocity = mult_speed * tf_scout_stunball_base_speed.GetFloat() * eye_angles_fwd;
				
				projectile->InitGrenade(velocity, AngularImpulse( 0, RandomFloat( 0, 100 ), 0 ), player, weapon->GetTFWpnData());
				projectile->SetLauncher(weapon);
				projectile->SetOwnerEntity(player);
				projectile->m_nSkin = player->GetTeamNumber() == TF_TEAM_BLUE ? 1 : 0;
				if (weapon->m_bCurrentAttackIsCrit) {
					projectile->m_bCritical = true;
				}
				projectile->SetDamage( weapon->GetProjectileDamage() );
				projectile->SetCustomVariable("applydmgmult", Variant(true));
			}
			retval = projectile;
		}
		else if (strcmp(name, "jarate") == 0 || strcmp(name, "madmilk") == 0 || strcmp(name, "cleaver") == 0 || strcmp(name, "gas") == 0) {
			Vector eye_angles_fwd, eye_angles_up, eye_angles_right;
			AngleVectors(angForward, &eye_angles_fwd, &eye_angles_right, &eye_angles_up);

			float speed = strcmp(name, "cleaver") == 0 ? 7000 : 1000;
			AngularImpulse angImp = strcmp(name, "cleaver") == 0 ? AngularImpulse( 0, 500, 0 ) : AngularImpulse( 300, 0, 0 );

			Vector fwd( ( eye_angles_fwd * speed * mult_speed ) + ( eye_angles_up * 200.0f ) + ( RandomFloat( -10.0f, 10.0f ) * eye_angles_right ) +		
			( RandomFloat( -10.0f, 10.0f ) * eye_angles_up ) );

			CTFProjectile_Jar *projectile = nullptr;
			if (strcmp(name, "jarate") == 0) {
				projectile = CTFProjectile_Jar::Create(vecSrc, player->EyeAngles(), fwd, angImp, player, weapon->GetTFWpnData());
			}
			else if (strcmp(name, "madmilk") == 0) {
				projectile = CTFProjectile_JarMilk::Create(vecSrc, player->EyeAngles(), fwd, angImp, player, weapon->GetTFWpnData());
			}
			else if (strcmp(name, "cleaver") == 0) {
				projectile = CTFProjectile_Cleaver::Create(vecSrc, player->EyeAngles(), fwd, angImp, player, weapon->GetTFWpnData(), weapon->m_nSkin);
			}
			else if (strcmp(name, "gas") == 0) {
				projectile = CTFProjectile_JarGas::Create(vecSrc, player->EyeAngles(), fwd, angImp, player, weapon->GetTFWpnData());
			}
			if (projectile != nullptr) {
				projectile->SetLauncher(weapon);
				projectile->m_bCritical = weapon->m_bCurrentAttackIsCrit;
				projectile->SetDamage( weapon->GetProjectileDamage() );
			}
			retval = projectile;
		}
		else if (strcmp(name, "brick") == 0 || strcmp(name, "repel") == 0 || strcmp(name, "breadmonster") == 0 || strcmp(name, "throwable") == 0) {
			const char *classname;
			if (strcmp(name, "brick") == 0) 
				classname = "tf_projectile_throwable_brick";
			else if (strcmp(name, "repel") == 0) {
				CBaseEntity::PrecacheModel("models/weapons/c_models/c_balloon_default.mdl");
				classname = "tf_projectile_throwable_repel";
			}
			else if (strcmp(name, "breadmonster") == 0) 
				classname = "tf_projectile_throwable_breadmonster";
			else if (strcmp(name, "throwable") == 0) 
				classname = "tf_projectile_throwable";
			CTFProjectile_Throwable *projectile = rtti_cast<CTFProjectile_Throwable *>(CBaseEntity::CreateNoSpawn(classname, vecSrc, player->EyeAngles(), player));
			if (projectile != nullptr) {
				projectile->SetPipebombMode();
				projectile->m_bCritical = weapon->m_bCurrentAttackIsCrit;
				
				DispatchSpawn(projectile);

				
				Vector eye_angles_fwd, eye_angles_up, eye_angles_right;
				AngleVectors(angForward, &eye_angles_fwd, &eye_angles_right, &eye_angles_up);
				Vector vecVelocity = projectile->GetVelocityVector( eye_angles_fwd, eye_angles_right, eye_angles_up, 0 );
				vecVelocity *= mult_speed;
				AngularImpulse angVelocity = projectile->GetAngularImpulse();

				projectile->InitGrenade(vecVelocity, angVelocity, player, weapon->GetTFWpnData());
				projectile->SetLauncher(weapon);
				projectile->SetDamage( weapon->GetProjectileDamage() );
			}
			retval = projectile;
		}
		else if (strcmp(name, "spellfireball") == 0 || strcmp(name, "spelllightningorb") == 0 || strcmp(name, "spellkartorb") == 0) {
			const char *classname;
			float speed = 1000;
			if (strcmp(name, "spellfireball") == 0) {
				classname = "tf_projectile_spellfireball";
			}
			else if (strcmp(name, "spelllightningorb") == 0) {
				speed = 400;
				classname = "tf_projectile_lightningorb";
			}
			else if (strcmp(name, "spellkartorb") == 0) {
				classname = "tf_projectile_spellkartorb";
			}
			speed *= mult_speed;
			CTFProjectile_Rocket *pRocket = static_cast<CTFProjectile_Rocket*>( CBaseEntity::CreateNoSpawn(classname , vecSrc, player->EyeAngles(), player ) );
			if ( pRocket )
			{
				pRocket->SetOwnerEntity( player );
				pRocket->SetLauncher( weapon ); 

				Vector vForward;
				AngleVectors( angForward, &vForward, NULL, NULL );
				vForward *= speed;
				pRocket->SetAbsVelocity( vForward );

				pRocket->SetDamage( weapon->GetProjectileDamage() );
				pRocket->ChangeTeam( player->GetTeamNumber() );

				IPhysicsObject *pPhysicsObject = pRocket->VPhysicsGetObject();
				if ( pPhysicsObject )
				{
					pPhysicsObject->AddVelocity( &vForward, &vec3_origin );
				}

				DispatchSpawn( pRocket );
				pRocket->SetCustomVariable("applydmgmult", Variant(true));
			}
			retval = pRocket;
		}
		
		else if (strcmp(name, "spellbats") == 0 || strcmp(name, "spellmirv") == 0 || strcmp(name, "spelltransposeteleport") == 0 || strcmp(name, "spellmeteorshower") == 0 || strcmp(name, "spellspawnboss") == 0 || strcmp(name, "spellspawnhorde") == 0) {
			std::string classname = "tf_projectile_"s + name;
			float speed = 1000;
			speed *= mult_speed;
			CTFProjectile_Jar *pGrenade = static_cast<CTFProjectile_Jar*>( CBaseEntity::CreateNoSpawn( classname.c_str() , vecSrc, player->EyeAngles(), player ) );
			if ( pGrenade )
			{
				// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
				pGrenade->SetPipebombMode();
				DispatchSpawn( pGrenade );

				IPhysicsObject *pPhys = pGrenade->VPhysicsGetObject();
				if ( pPhys )
				{
					pPhys->SetMass( 5.0f );
				}
				Vector vForward;
				AngleVectors( angForward, &vForward, NULL, NULL );
				vForward *= speed;
				pGrenade->InitGrenade( vForward, vec3_origin, player, weapon->GetTFWpnData() );
				pGrenade->SetDamage(weapon->GetProjectileDamage());
				pGrenade->m_flFullDamage = 0;
				pGrenade->SetCustomVariable("applydmgmult", Variant(true));
			}
			retval = pGrenade;
		}
		if (doEffect) {
			if (weapon->ShouldPlayFireAnim()) {
				player->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
			}
		
			weapon->RemoveProjectileAmmo(player);
			weapon->m_flLastFireTime = gpGlobals->curtime;
			weapon->DoFireEffects();
			weapon->UpdatePunchAngles(player);
		
			if (player->m_Shared->IsStealthed() && weapon->ShouldRemoveInvisibilityOnPrimaryAttack()) {
				player->RemoveInvisibility();
			}
		}
		return retval;
	}	

	THINK_FUNC_DECL(ProjectileHitRadius) {
		if (this->CollisionProp()->IsSolidFlagSet(FSOLID_NOT_SOLID)) {
			this->SetNextThink(gpGlobals->curtime+0.001f, "ProjectileHitRadius");
			return;
		}
		float size = this->GetCustomVariableFloat<"hitradius">();
		Ray_t ray;
		ray.Init( this->GetAbsOrigin() - this->GetAbsVelocity() * gpGlobals->frametime, this->GetAbsOrigin(), Vector(-size,-size,-size), Vector(size,size,size) );
		CBaseEntity *ents[256];
		int count = UTIL_EntitiesAlongRay(ents, sizeof(ents), ray, FL_CLIENT | FL_OBJECT | FL_NPC);
		for (int i = 0; i < count; ++i) {

			if (ents[i] == this || ents[i] == this->GetOwnerEntity() || ents[i]->m_takedamage != DAMAGE_YES || !ents[i]->CollisionProp()->IsSolid()) continue;
			this->Touch(ents[i]);
		}
		this->SetNextThink(gpGlobals->curtime+0.001f, "ProjectileHitRadius");
	};


	THINK_FUNC_DECL(ProjectileLifetime) {
		this->Remove();
	};

	THINK_FUNC_DECL(ProjectileDetonate) {
		auto proj = reinterpret_cast<CTFBaseRocket *>(this);
		trace_t tr;
		Vector vecSpot = proj->GetAbsOrigin();
		UTIL_TraceLine(vecSpot, vecSpot + Vector (0, 0, -32), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr);

		proj->Explode(&tr, GetContainingEntity(INDEXENT(0)));
	};


	THINK_FUNC_DECL(ProjectileSoundDelay) {
		this->Remove();
	};

	THINK_FUNC_DECL(ProjectileDamageable) {
		this->m_takedamage = DAMAGE_YES;
		this->m_fFlags |= FL_OBJECT;
		this->SetCustomVariable("takesdamage", Variant(true));
	};

	bool fire_projectile_multi = true;
	int old_clip = 0;

	DETOUR_DECL_MEMBER(void, CTFJar_TossJarThink)
	{
		auto weapon = reinterpret_cast<CTFWeaponBaseGun *>(this);
		GET_STRING_ATTRIBUTE(weapon, override_projectile_type_extra, projectilename);
		if (projectilename != nullptr) {
			SpawnCustomProjectile(projectilename, weapon, weapon->GetTFPlayerOwner(), false);
			return;
		}
		DETOUR_MEMBER_CALL();
	}

	RefCount rc_AltFireAttack;

	void ApplyPunch(CTFPlayer *player, CTFWeaponBase *weapon) {
		QAngle punch {vec3_angle};
		GET_STRING_ATTRIBUTE(weapon, shoot_view_punch_angle, punchNormal);
		if (punchNormal != nullptr) {
			UTIL_StringToAnglesAlt(punch, punchNormal);
		}
		QAngle random {vec3_angle};
		GET_STRING_ATTRIBUTE(weapon, shoot_view_punch_angle_random, punchRandom);
		if (punchRandom != nullptr) {
			UTIL_StringToAnglesAlt(random, punchRandom);
		}
		player->m_Local->m_vecPunchAngle += punch + QAngle(RandomFloat(-random.x, random.x),RandomFloat(-random.y, random.y),RandomFloat(-random.z, random.z));
	}

	void AddMedigunAttributes(CAttributeList *target, const char *attribs);
	void RemoveOnActiveAttributes(CEconEntity *weapon, const char *attributes);
	
	class AttackPatternModule : public EntityModule
	{
	public:
		AttackPatternModule(CBaseEntity *entity) : EntityModule(entity) {}

		int shootNum = 0;
		float resetPatternTime = 0;
	};


	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFWeaponBaseGun_FireProjectile, CTFPlayer *player)
	{
		auto weapon = reinterpret_cast<CTFWeaponBaseGun *>(this);

		int attr_projectile_count = 1;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, attr_projectile_count, mult_projectile_count);

		int attr_fire_all_at_once = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, attr_fire_all_at_once, fire_full_clip_at_once);

		int num_shots = attr_projectile_count;
		if (attr_fire_all_at_once != 0) {
			attr_projectile_count *= (weapon->IsEnergyWeapon() ? (weapon->Energy_GetMaxEnergy() / weapon->Energy_GetShotCost()) : weapon->m_iClip1);

			auto pcannon = rtti_cast<CTFParticleCannon *>(weapon);
			if (pcannon != nullptr && pcannon->m_flChargeBeginTime > 0) {
				attr_projectile_count = num_shots;
			}
		}

		int patternNoRollback = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, patternNoRollback, shoot_pattern_no_rollback);

		GET_STRING_ATTRIBUTE(weapon, spread_angle_pattern, spreadPattern);
		GET_STRING_ATTRIBUTE(weapon, spread_offset_pattern, offsetPattern);
		GET_STRING_ATTRIBUTE(weapon, proj_attribute_pattern, attributePattern);
		
		CBaseAnimating *proj = nullptr;
		//int seed = CBaseEntity::GetPredictionRandomSeed() & 255;
		for (int i = 0; i < attr_projectile_count; i++) {

			fire_projectile_multi = (i % num_shots) == 0;

			//if (i != 0) {
			//	RandomSeed(gpGlobals->tickcount + i);
			//}

			QAngle angleOffset = vec3_angle;
			Vector originOffset = vec3_origin;
			std::string attributePatternCurrent;

			if (attributePattern != nullptr || spreadPattern != nullptr || offsetPattern != nullptr) {
				auto spreadMod = weapon->GetOrCreateEntityModule<AttackPatternModule>("attackpattern");
				if (gpGlobals->curtime > spreadMod->resetPatternTime) {
					spreadMod->shootNum = 0;
				}

				if (spreadPattern != nullptr) {
					const auto v{vi::split_str(spreadPattern, "|")};
					int counter = patternNoRollback != 0 ? MIN(v.size() - 1, spreadMod->shootNum) : spreadMod->shootNum % v.size();
					UTIL_StringToAnglesAlt(angleOffset,v[counter].data());
				}

				if (offsetPattern != nullptr) {
					const auto v{vi::split_str(offsetPattern, "|")};
					int counter = patternNoRollback != 0 ? MIN(v.size() - 1, spreadMod->shootNum) : spreadMod->shootNum % v.size();
					Vector orig;
					UTIL_StringToVectorAlt(orig,v[counter].data());
					VectorRotate(orig, player->pl->v_angle, originOffset);
				}

				if (attributePattern != nullptr) {
					const auto v{vi::split_str(attributePattern, "&")};
					int counter = patternNoRollback != 0 ? MIN(v.size() - 1, spreadMod->shootNum) : spreadMod->shootNum % v.size();
					attributePatternCurrent = v[counter];
				}

				spreadMod->shootNum++;
			}

			if (rtti_cast<CTFJar *>(weapon) != nullptr) {
				weapon->StartEffectBarRegen();
			}

			GET_STRING_ATTRIBUTE(weapon, override_projectile_type_extra, projectilename);

			player->pl->v_angle += angleOffset; 
			player->SetLocalOrigin(player->GetLocalOrigin() + originOffset);
			if (!attributePatternCurrent.empty()) {
				AddMedigunAttributes(&weapon->GetItem()->GetAttributeList(), attributePatternCurrent.c_str());
			}
			
			if (projectilename != nullptr) {
				proj = SpawnCustomProjectile(projectilename, weapon, player, true);

			}
			else {
				proj = DETOUR_MEMBER_CALL(player);
			}
			player->SetLocalOrigin(player->GetLocalOrigin() - originOffset);
			player->pl->v_angle -= angleOffset; 
			
			fire_projectile_multi = true;

			if (proj != nullptr) {

				int projType = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, projType, override_projectile_type);
				if (projType != 0) {
					auto grenadeProj = rtti_cast<CTFWeaponBaseGrenadeProj *>(proj);
					if (grenadeProj != nullptr && grenadeProj->m_DmgRadius == 0) {
						grenadeProj->m_DmgRadius = 146.0f;
					}
				}
				float attr_lifetime = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, attr_lifetime, projectile_lifetime);

				if (attr_lifetime != 0.0f) {
					THINK_FUNC_SET(proj, ProjectileLifetime, gpGlobals->curtime + attr_lifetime);
					//proj->ThinkSet(&ProjectileLifetime::Update, gpGlobals->curtime + attr_lifetime, "ProjLifetime");
				}
				float attr_hit_radius = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, attr_hit_radius, projectile_hit_radius);

				if (attr_hit_radius > 0.0f) {
					proj->SetCustomVariable("hitradius", Variant(attr_hit_radius));
					THINK_FUNC_SET(proj, ProjectileHitRadius, gpGlobals->curtime);
				}

				float detonateTime = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, detonateTime, projectile_detonate_time);

				float explodeTime = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, explodeTime, projectile_explode_time);
				if (explodeTime != 0) {
					proj->SetCustomVariable("explodetime", Variant(explodeTime));
				}

				float health = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, health, projectile_health);
				if (health != 0) {
					proj->SetMaxHealth(health);
					proj->SetHealth(health);
					int takeDamageType = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, takeDamageType, projectile_take_damage_type);
					proj->SetCustomVariable("takesdamagetype", Variant(takeDamageType));
					float healthDelay = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, healthDelay, projectile_take_damage_time);
					if (healthDelay != 0.0f) {
						THINK_FUNC_SET(proj, ProjectileDamageable, gpGlobals->curtime + healthDelay);
					} else {
						proj->m_takedamage = DAMAGE_YES;
						proj->m_fFlags |= FL_OBJECT;
						proj->SetCustomVariable("takesdamage", Variant(true));
					}
				}
				
				int noclip = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, noclip, noclip_projectiles);
				if (noclip != 0) {
					proj->SetMoveType(MOVETYPE_NOCLIP);
				}

				if (detonateTime != 0.0f) {
					if (rtti_cast<CTFBaseRocket *>(proj) != nullptr) {
						THINK_FUNC_SET(proj, ProjectileDetonate, gpGlobals->curtime + detonateTime);
					}
					else {
						auto grenade = rtti_cast<CTFWeaponBaseGrenadeProj *>(proj);
						if (grenade != nullptr) {
							grenade->SetDetonateTimerLength(detonateTime);
						}
					}
					//proj->ThinkSet(&ProjectileLifetime::Update, gpGlobals->curtime + attr_lifetime, "ProjLifetime");
				}
				GET_STRING_ATTRIBUTE(weapon, projectile_trail_particle, particlename);
				if (particlename != nullptr) {

					force_send_client = true;
					CRecipientFilter filter;
					filter.AddAllPlayers();
					Vector color0 = weapon->GetParticleColor(1);
					Vector color1 = weapon->GetParticleColor(2);
					if (*particlename == '~') {
						StopParticleEffects(proj);
						//DispatchParticleEffect(particlename + 1, PATTACH_ABSORIGIN_FOLLOW, proj, INVALID_PARTICLE_ATTACHMENT, false);
						DispatchParticleEffect(particlename + 1, PATTACH_ABSORIGIN_FOLLOW, proj, nullptr, vec3_origin, false, color0, color1, true, false, nullptr, &filter);
					} else {
						DispatchParticleEffect(particlename, PATTACH_ABSORIGIN_FOLLOW, proj, nullptr, vec3_origin, false, color0, color1, true, false, nullptr, &filter);
					}
					force_send_client = false;
				}
				if (i < attr_projectile_count - 1)
					weapon->ModifyProjectile(proj);
				
				IPhysicsObject *physics = proj->VPhysicsGetObject();

				float gravity = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, gravity, projectile_gravity_native);
				if (gravity != 0.0f) {
					proj->SetGravity(gravity);
				}
				
				if (physics != nullptr) {
					if (gravity != 0.0f) {
						physics->EnableGravity(false);
					}
					float bounce_speed = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, bounce_speed, grenade_bounce_speed);
					if (bounce_speed != 0.0f) {
						physics->SetInertia({10000.0f,10000.0f,10000.0f});
					}
					int drag = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, drag, grenade_no_drag);
					if (drag != 0) {
						physics->EnableDrag(false);
					}
				}

				int ignoresOtherProjectiles = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, ignoresOtherProjectiles, ignores_other_projectiles);
				if (ignoresOtherProjectiles != 0) {
					proj->SetCollisionGroup(TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS);
				}

				//proj->SetCustomVariable("colfilter", );
				auto arrow = rtti_cast<CTFProjectile_Arrow *>(proj);
				if (arrow != nullptr) {
					int arrowIgnite = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, arrowIgnite, arrow_ignite);
					if (arrowIgnite != 0) {
						arrow->m_bArrowAlight = true;
					}
				}

				GET_STRING_ATTRIBUTE(weapon, projectile_sound, soundname);
				if (soundname != nullptr) {
					PrecacheSound(soundname);
					proj->EmitSound(soundname);
				}
				if (rc_AltFireAttack) {
					proj->SetCustomVariable("isaltfire", Variant(true));
				}

				if (!attributePatternCurrent.empty()){
					proj->SetCustomVariable("projattribs", Variant(AllocPooledString(attributePatternCurrent.c_str())));
				}
			}
			GET_STRING_ATTRIBUTE(weapon, fire_input_on_attack, input);
			FireInputAttribute(input, nullptr, Variant((CBaseEntity *)proj), proj, player, weapon, -1.0f);
			
			if (!attributePatternCurrent.empty())
				RemoveOnActiveAttributes(weapon, attributePatternCurrent.c_str());
		}

		int shoot_projectiles = 0;
		
		ApplyPunch(player, weapon);
		
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, shoot_projectiles, attack_projectiles);
		if (shoot_projectiles > 0) {
			auto weapon = reinterpret_cast<CTFWeaponBase*>(this);
			if (weapon->GetWeaponID() != TF_WEAPON_MINIGUN)
				AttackEnemyProjectiles(player, reinterpret_cast<CTFWeaponBase*>(this), shoot_projectiles);
		}
		old_clip = 0;
		projectile_arrow = proj;
		return proj;
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_RemoveProjectileAmmo, CTFPlayer *player)
	{
		if (fire_projectile_multi)
			DETOUR_MEMBER_CALL(player);
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_UpdatePunchAngles, CTFPlayer *player)
	{
		if (fire_projectile_multi)
			DETOUR_MEMBER_CALL(player);
	}

	THINK_FUNC_DECL(RelightBowArrow)
	{
		reinterpret_cast<CTFCompoundBow *>(this)->m_bArrowAlight = true;
	}
	
	DETOUR_DECL_MEMBER(void, CTFCompoundBow_LaunchGrenade)
	{
		auto bow = reinterpret_cast<CTFCompoundBow *>(this);
		int arrowIgnite = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(bow, arrowIgnite, arrow_ignite);

		if (arrowIgnite != 0) {
			bow->m_bArrowAlight = true;
		}

		DETOUR_MEMBER_CALL();

		int attib_arrow_mastery = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( bow, attib_arrow_mastery, arrow_mastery );
		
		if (arrowIgnite != 0) {
			THINK_FUNC_SET(bow, RelightBowArrow, gpGlobals->curtime + 0.1f);
		}

		if (attib_arrow_mastery != 0 && projectile_arrow != nullptr && projectile_arrow->GetOwnerEntity() != nullptr) {

			Vector vecMainVelocity = projectile_arrow->GetAbsVelocity();
			float flMainSpeed = vecMainVelocity.Length();

			CTFProjectile_Arrow *arrow = static_cast<CTFProjectile_Arrow *>(projectile_arrow);

			float angle = attib_arrow_mastery > 0 ? 5.0f : - 360.0f / (attib_arrow_mastery - 1);

			int count = attib_arrow_mastery > 0 ? attib_arrow_mastery : -attib_arrow_mastery;
			if (arrow != nullptr) {
				for (int i = 0; i < count; i++) {
					
					QAngle qOffset1 = projectile_arrow->GetAbsAngles() + QAngle( 0, GetRandomSpreadOffset( bow, i + 1, angle ), 0 );
					CreateExtraArrow( bow, arrow, qOffset1, flMainSpeed, arrowIgnite != 0 );
					if (attib_arrow_mastery > 0 ) {
						QAngle qOffset2 = projectile_arrow->GetAbsAngles() + QAngle( 0, -GetRandomSpreadOffset( bow, i + 1, angle ), 0 );
						CreateExtraArrow( bow, arrow, qOffset2, flMainSpeed, arrowIgnite != 0 );
					}

				}
			}
		}
	}
	THINK_FUNC_DECL(BowIgniteDelay)
	{
		auto bow = reinterpret_cast<CTFCompoundBow *>(this);
		bow->m_bArrowAlight = true;
	}

	VHOOK_DECL(bool, CTFCompoundBow_Deploy)
	{
		auto result = VHOOK_CALL();
		
		auto bow = reinterpret_cast<CTFCompoundBow *>(this);
		int arrowIgnite = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(bow, arrowIgnite, arrow_ignite);
		if (arrowIgnite != 0)
			THINK_FUNC_SET(bow, BowIgniteDelay, gpGlobals->curtime);
		return result;
	}

	RefCount rc_stop_our_team_deflect;
	DETOUR_DECL_MEMBER(void, CTFWeaponBaseMelee_Swing, CTFPlayer *player)
	{
		DETOUR_MEMBER_CALL(player);
		auto weapon = reinterpret_cast<CTFWeaponBaseMelee*>(this);
		float smacktime = weapon->m_flSmackTime;
		float time = gpGlobals->curtime;
		float attr_smacktime = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, attr_smacktime, mult_smack_time);
		weapon->m_flSmackTime = time + (smacktime - time) * attr_smacktime;
		if (!player->IsBot() && weapon->m_flSmackTime > weapon->m_flNextPrimaryAttack)
			weapon->m_flSmackTime = weapon->m_flNextPrimaryAttack - 0.02;
		
		int airblast = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( weapon, airblast, melee_airblast);
		if (airblast > 0) {
			SCOPED_INCREMENT(rc_stop_our_team_deflect);
			weapon->DeflectProjectiles();
		}
		float protection = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, protection, melee_grants_protection);
		if (protection > 0) {
			variant_t var;
			var.SetFloat(gpGlobals->curtime + protection);
			weapon->SetCustomVariable("swingtime", var);
		}
	}

	DETOUR_DECL_MEMBER(int, CBaseObject_OnTakeDamage, CTakeDamageInfo &info)
	{
		int damage = DETOUR_MEMBER_CALL(info);
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

	CBaseEntity *killer_weapon = nullptr;
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
			
		killer_weapon = info.GetWeapon();
		DETOUR_MEMBER_CALL(info);
		
		int destroyBuildingsOnDeath = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, destroyBuildingsOnDeath, destroy_buildings_on_death);
		if (destroyBuildingsOnDeath != 0) {
			for (int i = player->GetObjectCount() - 1; i >= 0 ; i--) {
				auto obj = player->GetObject(i);
				if (obj != nullptr) {
					obj->DetonateObject();
				}
			}
		}
		killer_weapon = nullptr;

		ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
			static int attachment_name_def = GetItemSchema()->GetAttributeDefinitionByName("attachment name")->GetIndex();
			if (entity->GetItem() != nullptr && entity->GetItem()->GetAttributeList().GetAttributeByID(attachment_name_def) != nullptr) {
				entity->AddEffects(EF_NODRAW);
			}
		});
		is_ice = false;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_CreateRagdollEntity, bool bShouldGib, bool bBurning, bool bUberDrop, bool bOnGround, bool bYER, bool bGold, bool bIce, bool bAsh, int iCustom, bool bClassic)
	{
		bIce |= is_ice;
		
		DETOUR_MEMBER_CALL(bShouldGib, bBurning, bUberDrop, bOnGround, bYER, bGold, bIce, bAsh, iCustom, bClassic);
	}

	void GetExplosionParticle(CTFPlayer *player, CTFWeaponBase *weapon, int &particle, int &particleDirectHit) {
		int no_particle = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, no_particle, no_explosion_particles);
		if (no_particle) {
			particle = -1;
			return;
		}

		GET_STRING_ATTRIBUTE(weapon, explosion_particle, particlename);
		if (particlename != nullptr) {
			if (precached.find(particlename) == precached.end()) {
				PrecacheParticleSystem(particlename);
				precached.insert(particlename);
			}
			particle = GetParticleSystemIndex(particlename);
		}

		GET_STRING_ATTRIBUTE(weapon, explosion_particle_on_direct_hit, particlenameDirect);
		if (particlenameDirect != nullptr) {
			if (precached.find(particlenameDirect) == precached.end()) {
				PrecacheParticleSystem(particlenameDirect);
				precached.insert(particlenameDirect);
			}
			particleDirectHit = GetParticleSystemIndex(particlenameDirect);
		}
	}

	int particle_to_use = 0;
	int particle_to_use_direct_hit = 0;

	CTFWeaponBaseGun *stickbomb = nullptr;
	DETOUR_DECL_MEMBER(void, CTFStickBomb_Smack)
	{	
		stickbomb = reinterpret_cast<CTFWeaponBaseGun*>(this);
		particle_to_use = 0;
		particle_to_use_direct_hit = 0;
		GetExplosionParticle(stickbomb->GetTFPlayerOwner(), stickbomb,particle_to_use, particle_to_use_direct_hit);
		DETOUR_MEMBER_CALL();
		stickbomb = nullptr;
		particle_to_use_direct_hit = 0;
		particle_to_use = 0;
	}

	RefCount rc_CTFGameRules_RadiusDamage;
	
	int hit_entities_explosive = 0;
	int hit_entities_explosive_max = 0;

	bool minicrit = false;
	CBaseEntity *hit_explode_direct = nullptr;
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
				DETOUR_MEMBER_CALL(info);
				stickbomb->GetTFPlayerOwner()->m_Shared->RemoveCond(TF_COND_NOHEALINGDAMAGEBUFF);
				return;
			}
		}
		
		CBaseEntity *weapon = info.m_DmgInfo->GetWeapon();
		hit_entities_explosive = 0;
		hit_entities_explosive_max = GetFastAttributeInt(weapon, 0, MAX_AOE_TARGETS);

		if (hit_explode_direct != nullptr && hit_explode_direct->MyCombatCharacterPointer() != nullptr) {
			float radiusMult = 1;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, radiusMult, mult_explosion_radius_direct_hit );
			info.m_flRadius *= radiusMult;
			
			float dmg_mult = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, dmg_mult, mult_dmg_direct_hit);
			info.m_DmgInfo->SetDamage(info.m_DmgInfo->GetDamage() * dmg_mult);
		}
		float damagePerTarget = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, damagePerTarget, add_damage_per_target );
		float dmgBase = info.m_DmgInfo->GetDamage() * damagePerTarget;
		if (damagePerTarget != 0 && info.m_DmgInfo->GetAttacker() != nullptr) {
			const int maxCollectedEntities = 128;
			CBaseEntity	*pObjects[ maxCollectedEntities ];
			CFlaggedEntitiesEnum iter = CFlaggedEntitiesEnum(pObjects, maxCollectedEntities, 0 );
			partition->EnumerateElementsInSphere(PARTITION_ENGINE_NON_STATIC_EDICTS, info.m_vecOrigin, info.m_flRadius, false, &iter);
			int count = iter.GetCount();

			for ( int i = 0; i < count; i++ )
			{
				CBaseEntity *target = pObjects[i];
				if (target == nullptr) continue;
				if (target == info.m_DmgInfo->GetAttacker()) continue;
				if (target->m_takedamage == DAMAGE_NO) continue;
				if (target->GetTeamNumber() == info.m_DmgInfo->GetAttacker()->GetTeamNumber() ) continue;
				Vector closestPoint;
				target->CollisionProp()->CalcNearestPoint(info.m_vecOrigin, &closestPoint);
				if ((info.m_vecOrigin - closestPoint).LengthSqr() > info.m_flRadius * info.m_flRadius ) continue;
				info.m_DmgInfo->AddDamage(dmgBase);
			}
		}

		DETOUR_MEMBER_CALL(info);
	}

	CBasePlayer *process_movement_player = nullptr;
	DETOUR_DECL_MEMBER(void, CTFGameMovement_ProcessMovement, CBasePlayer *player, void *data)
	{
		process_movement_player = player;
		DETOUR_MEMBER_CALL(player, data);
		process_movement_player = nullptr;
	}

	DETOUR_DECL_MEMBER(bool, CTFGameMovement_CheckJumpButton)
	{
		bool restoreDucking = false;
		if (process_movement_player != nullptr) {
			auto player = ToTFPlayer(process_movement_player);
			if (!player->IsPlayerClass(TF_CLASS_SCOUT) && player->GetGroundEntity() == nullptr && player->GetFlags() & FL_DUCKING) {
				player->m_fFlags &= ~(FL_DUCKING);
				restoreDucking = true;
			}
		}
		bool ret = DETOUR_MEMBER_CALL();
		if (restoreDucking) {
			process_movement_player->m_fFlags |= FL_DUCKING;
		}
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

	class MedigunCustomBeamModule : public EntityModule, public AutoList<MedigunCustomBeamModule>
	{
	public:
		MedigunCustomBeamModule(CBaseEntity *entity) : EntityModule(entity), medigun(static_cast<CWeaponMedigun *>(entity)) { 
			RecalcEffects();
		}
		
		~MedigunCustomBeamModule() {
			if (particles != nullptr) {
				particles->Remove();
			}
			if (particlesAttach != nullptr) {
				particlesAttach->Remove();
			}
			if (chargeEffect != nullptr) {
				chargeEffect->Remove();
			}
		} 

		void RecalcEffects() {
			
			GET_STRING_ATTRIBUTE(medigun, medigun_particle, medigunParticle);
			GET_STRING_ATTRIBUTE(medigun, medigun_particle_enemy, medigunParticleEnemy);
			GET_STRING_ATTRIBUTE(medigun, medigun_particle_release, medigunParticleRelease);
			GET_STRING_ATTRIBUTE(medigun, medigun_particle_spark, medigunParticleSpark);

			if (medigunParticle != nullptr && medigunParticle[0] == '~') {
				effectImmediateDestroy = true;
				medigunParticle++; 
			}
			if (medigunParticleEnemy != nullptr && medigunParticleEnemy[0] == '~') {
				effectImmediateDestroy = true;
				medigunParticleEnemy++; 
			}
			if (medigunParticleRelease != nullptr && medigunParticleRelease[0] == '~') {
				effectImmediateDestroy = true;
				medigunParticleRelease++; 
			}

			effectBlu = PStrT<"medicgun_beam_blue">();
			effectRed = PStrT<"medicgun_beam_red">();
			effectTargetBlu = PStrT<"medicgun_beam_blue_targeted">();
			effectTargetRed = PStrT<"medicgun_beam_red_targeted">();
			effectReleaseBlu = PStrT<"medicgun_beam_blue_invun">();
			effectReleaseRed = PStrT<"medicgun_beam_red_invun">();
			effectObjects = PStrT<"medicgun_beam_machinery">();
			effectEnemyBlu = PStrT<"medicgun_beam_blue_targeted">();
			effectEnemyRed = PStrT<"medicgun_beam_red_targeted">();
			effectSparkBlu = PStrT<"medicgun_invulnstatus_fullcharge_blue">();
			effectSparkRed = PStrT<"medicgun_invulnstatus_fullcharge_red">();
			if (medigunParticleEnemy == nullptr && medigunParticle != nullptr) {
				medigunParticleEnemy = medigunParticle;
			}
			if (medigunParticleRelease == nullptr && medigunParticle != nullptr) {
				medigunParticleRelease = medigunParticle;
			}
			
			if (medigunParticle != nullptr) {
				effectObjects = AllocPooledString(CFmtStr("%s_machinery", medigunParticle));
				if (GetParticleSystemIndex(STRING(effectObjects)) == 0) {
					effectObjects = AllocPooledString(medigunParticle);
				}
				effectRed = AllocPooledString(CFmtStr("%s_red", medigunParticle));
				if (GetParticleSystemIndex(STRING(effectRed)) == 0) {
					effectRed = AllocPooledString(medigunParticle);
				}
				effectTargetRed = AllocPooledString(CFmtStr("%s_red_targeted", medigunParticle));
				if (GetParticleSystemIndex(STRING(effectTargetRed)) == 0) {
					effectTargetRed = effectRed;
				}
				effectBlu = AllocPooledString(CFmtStr("%s_blue", medigunParticle));
				if (GetParticleSystemIndex(STRING(effectBlu)) == 0) {
					effectBlu = AllocPooledString(medigunParticle);
				}
				effectTargetBlu = AllocPooledString(CFmtStr("%s_blue_targeted", medigunParticle));
				if (GetParticleSystemIndex(STRING(effectTargetBlu)) == 0) {
					effectTargetBlu = effectRed;
				}
			}
			if (medigunParticleRelease != nullptr) {
				effectReleaseRed = AllocPooledString(CFmtStr("%s_red_invun", medigunParticleRelease));
				if (GetParticleSystemIndex(STRING(effectReleaseRed)) == 0) {
					effectReleaseRed = AllocPooledString(medigunParticleRelease);
				}
				effectReleaseBlu = AllocPooledString(CFmtStr("%s_blue_invun", medigunParticleRelease));
				if (GetParticleSystemIndex(STRING(effectReleaseBlu)) == 0) {
					effectReleaseRed = AllocPooledString(medigunParticleRelease);
				}
			}
			if (medigunParticleEnemy != nullptr) {
				effectEnemyRed = AllocPooledString(CFmtStr("%s_red", medigunParticleEnemy));
				if (GetParticleSystemIndex(STRING(effectEnemyRed)) == 0) {
					effectEnemyRed = AllocPooledString(medigunParticleEnemy);
				}
				effectEnemyBlu = AllocPooledString(CFmtStr("%s_blue", medigunParticleEnemy));
				if (GetParticleSystemIndex(STRING(effectEnemyBlu)) == 0) {
					effectEnemyBlu = AllocPooledString(medigunParticleEnemy);
				}
			}
			if (medigunParticleSpark != nullptr) {
				effectSparkRed = AllocPooledString(CFmtStr("%s_red", medigunParticleSpark));
				if (GetParticleSystemIndex(STRING(effectSparkRed)) == 0) {
					effectSparkRed = AllocPooledString(medigunParticleSpark);
				}
				effectSparkBlu = AllocPooledString(CFmtStr("%s_blue", medigunParticleSpark));
				if (GetParticleSystemIndex(STRING(effectSparkBlu)) == 0) {
					effectSparkBlu = AllocPooledString(medigunParticleSpark);
				}
			}
		}

		CWeaponMedigun *medigun;
		string_t lastEffect = NULL_STRING;
		string_t effectBlu;
		string_t effectRed;
		string_t effectTargetBlu;
		string_t effectTargetRed;
		string_t effectReleaseBlu;
		string_t effectReleaseRed;
		string_t effectObjects;
		string_t effectEnemyBlu;
		string_t effectEnemyRed;
		string_t effectSparkBlu;
		string_t effectSparkRed;
		bool effectImmediateDestroy = false;

		CHandle<CParticleSystem> particles;
		CHandle<CParticleSystem> particlesAttach;
		CHandle<CParticleSystem> chargeEffect;
		CHandle<CBaseEntity> prevTarget;
	};

	struct CustomModelEntry
	{
		CHandle<CTFWeaponBase> weapon;
		CHandle<CEconEntity> wearable;
		CHandle<CBaseEntity> wearable_vm;
		int model_index;
		bool temporaryVisible = false;
		bool createVMWearable = true;
	};
	std::vector<CustomModelEntry> model_entries;

	CustomModelEntry *FindCustomModelEntry(CTFWeaponBase *entity) {
		for (auto &entry : model_entries) {
			if (entry.weapon == entity) {
				return &entry;
			} 
		}
		return nullptr;
	}

	CBaseAnimating *FindVisibleEntity(CTFPlayer *owner, CEconEntity *econEntity)
	{
		if (model_entries.empty()) return econEntity;

		auto weapon = static_cast<CTFWeaponBase *>(ToBaseCombatWeapon(econEntity));
		if (weapon != nullptr) {
			auto entry = FindCustomModelEntry(weapon);
			if (entry != nullptr && entry->wearable != nullptr) {
				return entry->wearable;
			}
		}
		return econEntity;
	}
	
	void ApplyAttachmentAttributesToEntity(CTFPlayer *owner, CBaseAnimating *entity, CEconEntity *econEntity)
	{
		int color = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( econEntity, color, item_color_rgb);
		if (color != 0) {
			entity->SetRenderColorR(color >> 16);
			entity->SetRenderColorG((color >> 8) & 255);
			entity->SetRenderColorB(color & 255);
		}
		int invisible = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( econEntity, invisible, is_invisible);
		if (invisible != 0) {
			DevMsg("SetInvisible\n");
			servertools->SetKeyValue(entity, "rendermode", "10");
			entity->AddEffects(EF_NODRAW);
			entity->SetRenderColorA(0);
		}

		CAttributeList &attrlist = econEntity->GetItem()->GetAttributeList();

		GET_STRING_ATTRIBUTE(econEntity, attachment_name, attachmentname);

		if (owner != nullptr && attachmentname != nullptr) {
			int attachment = owner->LookupAttachment(attachmentname);
			entity->SetEffects(entity->GetEffects() & ~(EF_BONEMERGE));
			if (attachment > 0) {
				entity->SetParent(owner, attachment);
				
			}

			Vector pos = vec3_origin;
			QAngle ang = vec3_angle;
			float scale = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( econEntity, scale, attachment_scale);
			
			GET_STRING_ATTRIBUTE(econEntity, attachment_offset, offsetstr);
			GET_STRING_ATTRIBUTE(econEntity, attachment_angles, anglesstr);

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

	void CreateWeaponWearables(CustomModelEntry &entry)
	{
		if (entry.weapon->GetTFPlayerOwner() == nullptr) return;

		if ((entry.wearable_vm == nullptr || entry.wearable_vm->IsMarkedForDeletion()) && entry.createVMWearable) {
			auto wearable_vm = static_cast<CTFWearable *>(CreateEntityByName("tf_wearable_vm"));
			CopyVisualAttributes(entry.weapon->GetTFPlayerOwner(), entry.weapon, wearable_vm);
			DispatchSpawn(wearable_vm);
			wearable_vm->GiveTo(entry.weapon->GetTFPlayerOwner());
			wearable_vm->SetModelIndex(entry.model_index);
			wearable_vm->m_bValidatedAttachedEntity = true;
			//wearable_vm->SetParent(wearable_vm->GetMoveParent(), entry.weapon->GetTFPlayerOwner()->GetViewModel()->LookupAttachment("weapon_bone"));

			entry.wearable_vm = wearable_vm;
		}

		if (entry.wearable == nullptr || entry.wearable->IsMarkedForDeletion()) {
			auto wearable = CreateCustomWeaponModelPlaceholder(entry.weapon->GetTFPlayerOwner(), entry.weapon, entry.model_index);
			ApplyAttachmentAttributesToEntity(entry.weapon->GetTFPlayerOwner(), wearable, entry.weapon);
			entry.wearable = wearable;
			entry.weapon->SetCustomVariable("custommodelwearable", Variant((CBaseEntity *) wearable));
		}
	}

	DETOUR_DECL_MEMBER(const char *, CTFWeaponInvis_GetViewModel, int index)
	{
		auto me = reinterpret_cast<CTFWeaponInvis *>(this);
		GET_STRING_ATTRIBUTE(me, custom_item_model, modelname);
		if (modelname != nullptr) {
			return modelname;
		}
		return DETOUR_MEMBER_CALL(index);
	}
	void UpdateCustomModel(CTFPlayer *owner, CEconEntity *entity, CAttributeList &attrlist) {
		

		GET_STRING_ATTRIBUTE(entity, custom_item_model, modelname);
		if (modelname != nullptr) {

			int model_index = CBaseEntity::PrecacheModel(modelname);
			auto weapon = static_cast<CTFWeaponBase *>(ToBaseCombatWeapon(entity));
			if (owner != nullptr && rtti_cast<CTFWeaponInvis *>(weapon) != nullptr && owner->GetViewModel(weapon->m_nViewModelIndex) != nullptr) {
				//owner->GetViewModel(weapon->m_nViewModelIndex)->SetWeaponModel(modelname, weapon);
				// owner->GetViewModel(weapon->m_nViewModelIndex)->SetModel(modelname);
				// owner->GetViewModel(weapon->m_nViewModelIndex)->SpawnControlPanels();
				// owner->GetViewModel(weapon->m_nViewModelIndex)->SetControlPanelsActive(true);

			}
			// else if (owner != nullptr && rtti_cast<CTFWeaponBuilder *>(weapon) != nullptr) {
			// 	for (int i = 0; i < MAX_VISION_MODES; ++i) {
			// 		entity->SetModelIndexOverride(i, model_index);
			// 	}
			// }
			else if (weapon != nullptr && owner != nullptr && !owner->IsFakeClient()) {
				auto entry = FindCustomModelEntry(weapon);
				if (entry != nullptr) {
					if (entry->wearable != nullptr && entry->wearable->GetModelIndex() != model_index) {
						entry->wearable->Remove();
						entry->wearable = nullptr;
					}
					if (entry->wearable_vm != nullptr && entry->wearable_vm->GetModelIndex() != model_index) {
						entry->wearable_vm->Remove();
						entry->wearable_vm = nullptr;
					}
					entry->model_index = model_index;
				}
				else {
					entity->SetRenderMode(kRenderTransAlpha);
					entity->AddEffects(EF_NOSHADOW);
					entity->SetRenderColorA(0);
					auto mod = entity->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
					mod->props["m_bBeingRepurposedForTaunt"] = {Variant(true), Variant(true)};
					model_entries.push_back({weapon, nullptr, nullptr, model_index, false, entity->GetItem()->GetItemDefinition()->GetKeyValues()->GetInt("attach_to_hands", 0) != 0});
					if (rtti_cast<CWeaponMedigun *>(entity) != nullptr) {
						entity->GetOrCreateEntityModule<MedigunCustomBeamModule>("mediguncustombeam");
					}
					entry = &model_entries.back();
					CreateWeaponWearables(*entry);
				}
			}
			else {
				for (int i = 0; i < MAX_VISION_MODES; ++i) {
					entity->SetModelIndexOverride(i, model_index);
				}
			}
		}
		else {
			auto weapon = static_cast<CTFWeaponBase *>(ToBaseCombatWeapon(entity));
			if (weapon != nullptr ) {
				auto entry = FindCustomModelEntry(weapon);
				if (entry != nullptr) {
					weapon->SetRenderMode(kRenderNormal);
					entity->SetRenderColorA(255);
					auto mod = entity->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
					mod->props.erase("m_bBeingRepurposedForTaunt");
					entry->weapon = nullptr;
				}
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBase_UpdateHands)
	{
		DETOUR_MEMBER_CALL();
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		GET_STRING_ATTRIBUTE(weapon, custom_view_model, viewmodel);
		
		if (viewmodel != nullptr && weapon->GetTFPlayerOwner() != nullptr && IsCustomViewmodelAllowed(weapon->GetTFPlayerOwner())) {
			weapon->SetCustomViewModel(viewmodel);
			weapon->m_iViewModelIndex = CBaseEntity::PrecacheModel(viewmodel);
			weapon->SetModel(viewmodel);
		}
	}

	DETOUR_DECL_MEMBER(void, CEconEntity_UpdateModelToClass)
	{
		DETOUR_MEMBER_CALL();
		auto entity = reinterpret_cast<CEconEntity *>(this);

		auto owner = ToTFPlayer(entity->GetOwnerEntity());
		CAttributeList &attrlist = entity->GetItem()->GetAttributeList();
		UpdateCustomModel(owner, entity, attrlist);
		ApplyAttachmentAttributesToEntity(owner, entity, entity);
	}

	DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_Equip, CBaseCombatCharacter *owner)
	{
		DETOUR_MEMBER_CALL(owner);
		auto ent = reinterpret_cast<CBaseCombatWeapon *>(this);

		GET_STRING_ATTRIBUTE_NO_CACHE(ent, attachment_name, attachmentname);
		if (attachmentname != nullptr && ToTFPlayer(owner) != nullptr) {
			ApplyAttachmentAttributesToEntity(ToTFPlayer(owner), ent, ent);
		}
	}
	
	float bounce_damage_bonus = 0.0f;

	void ExplosionCustomSet(CBaseProjectile *proj)
	{
		auto launcher = static_cast<CTFWeaponBase *>(ToBaseCombatWeapon(proj->GetOriginalLauncher()));
		if (launcher != nullptr) {
			GetExplosionParticle(launcher->GetTFPlayerOwner(), launcher,particle_to_use, particle_to_use_direct_hit);
			GET_STRING_ATTRIBUTE(launcher, custom_impact_sound, sound);
			
			if (sound != nullptr) {
				PrecacheSound(sound);
				proj->EmitSound(sound);
			}
		}
	}

	THINK_FUNC_DECL(DelayExplodeGrenade) {
		this->SetCustomVariable("explodedelaynow", Variant(true));
		trace_t tr;
		Vector vecSpot = this->GetAbsOrigin();
		UTIL_TraceLine(vecSpot, vecSpot + Vector (0, 0, -32), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr);
		reinterpret_cast<CTFWeaponBaseGrenadeProj *>(this)->Explode(&tr, this->GetDamageType());
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGrenadeProj_Explode, trace_t *pTrace, int bitsDamageType)
	{
		auto proj = reinterpret_cast<CTFWeaponBaseGrenadeProj *>(this);
		if (proj->GetCustomVariableFloat<"explodetime">() != 0.0f && !proj->GetCustomVariableBool<"explodedelaynow">()) {
			if (proj->GetNextThink("DelayExplodeGrenade") <= 0)
				THINK_FUNC_SET(proj, DelayExplodeGrenade, gpGlobals->curtime + proj->GetCustomVariableFloat<"explodetime">());
			return;
		} 
		particle_to_use = 0;
		particle_to_use_direct_hit = 0;
		
		if (bounce_damage_bonus != 0.0f) {
			proj->SetDamage(proj->GetDamage() * (1 + bounce_damage_bonus));
		}

		ExplosionCustomSet(proj);
		DETOUR_MEMBER_CALL(pTrace, bitsDamageType);
		particle_to_use = 0;
		particle_to_use_direct_hit = 0;
	}

	THINK_FUNC_DECL(DelayExplodeRocket) {
		this->SetCustomVariable("explodedelaynow", Variant(true));
		trace_t tr;
		Vector vecSpot = this->GetAbsOrigin();
		UTIL_TraceLine(vecSpot, vecSpot + Vector (0, 0, -32), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr);
		reinterpret_cast<CTFBaseRocket *>(this)->Explode(&tr, GetWorldEntity());
	}

	DETOUR_DECL_MEMBER(void, CTFBaseRocket_Explode, trace_t *pTrace, CBaseEntity *pOther)
	{
		auto proj = reinterpret_cast<CTFBaseRocket *>(this);
		if (proj->GetCustomVariableFloat<"explodetime">() != 0.0f && !proj->GetCustomVariableBool<"explodedelaynow">()) {
			
			if (proj->GetNextThink("DelayExplodeRocket") <= 0)
				THINK_FUNC_SET(proj, DelayExplodeRocket, gpGlobals->curtime + proj->GetCustomVariableFloat<"explodetime">());
			return;
		} 
		particle_to_use = 0;
		particle_to_use_direct_hit = 0;
		ExplosionCustomSet(proj);
		hit_explode_direct = pOther;
		DETOUR_MEMBER_CALL(pTrace, pOther);
		hit_explode_direct = nullptr;

		particle_to_use = 0;
		particle_to_use_direct_hit = 0;
	}

	RefCount rc_CTFProjectile_EnergyBall_Explode;
	DETOUR_DECL_MEMBER(void, CTFProjectile_EnergyBall_Explode, trace_t *pTrace, CBaseEntity *pOther)
	{
		SCOPED_INCREMENT(rc_CTFProjectile_EnergyBall_Explode);
		auto proj = reinterpret_cast<CTFProjectile_EnergyBall *>(this);
		if (proj->GetCustomVariableFloat<"explodetime">() != 0.0f && !proj->GetCustomVariableBool<"explodedelaynow">()) {
			
			if (proj->GetNextThink("DelayExplodeRocket") <= 0)
				THINK_FUNC_SET(proj, DelayExplodeRocket, gpGlobals->curtime + proj->GetCustomVariableFloat<"explodetime">());
			return;
		} 
		particle_to_use = 0;
		particle_to_use_direct_hit = 0;
		ExplosionCustomSet(proj);
		hit_explode_direct = pOther;
		DETOUR_MEMBER_CALL(pTrace, pOther);
		hit_explode_direct = nullptr;
		particle_to_use = 0;
		particle_to_use_direct_hit = 0;
	}

	DETOUR_DECL_STATIC(void, TE_TFExplosion, IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, int nEntIndex, int nDefID, int nSound, int iCustomParticle)
	{
		//CBasePlayer *playerbase;
		//if (nEntIndex > 0 && (playerbase = UTIL_PlayerByIndex(nEntIndex)) != nullptr) {
		if (particle_to_use != 0)
			iCustomParticle = particle_to_use;

		if (particle_to_use_direct_hit != 0 && nEntIndex > 0 && UTIL_PlayerByIndex(nEntIndex) != nullptr)
			iCustomParticle = particle_to_use_direct_hit;

		if (particle_to_use == -1) return;

		DETOUR_STATIC_CALL(filter, flDelay, vecOrigin, vecNormal, iWeaponID, nEntIndex, nDefID, nSound, iCustomParticle);
	}

	DETOUR_DECL_STATIC(void, TE_TFParticleEffect, IRecipientFilter& recipement, float value, char const* name, Vector vector, Vector normal, QAngle angles, CBaseEntity* entity)
	{
		if (rc_CTFProjectile_EnergyBall_Explode && StringStartsWith(name, "drg_cow")) {
			if (particle_to_use_direct_hit != 0 && hit_explode_direct != nullptr && hit_explode_direct->MyCombatCharacterPointer() != nullptr) {
				name = GetParticleSystemNameFromIndex(particle_to_use_direct_hit);
			}
			else if (particle_to_use) {
				name = GetParticleSystemNameFromIndex(particle_to_use);
			}
		}
		
		DETOUR_STATIC_CALL(recipement, value, name, vector, normal, angles, entity);
	}
	RefCount rc_CTFPlayer_TraceAttack;

	const char *weapon_sound_override = nullptr;
	CBasePlayer *weapon_sound_override_owner = nullptr;
	DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_WeaponSound, int index, float soundtime) 
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		if ((index == SINGLE || index == BURST || index == MELEE_MISS || index == MELEE_HIT || index == MELEE_HIT_WORLD || index == RELOAD || index == DEPLOY || index == SPECIAL1 || index == SPECIAL3)) {

			int weaponid = weapon->GetWeaponID();
			// Allow SPECIAL1 sound for Cow Mangler and SPECIAL3 sound for short circuit
			if (index == SPECIAL1 && weaponid != TF_WEAPON_PARTICLE_CANNON) return DETOUR_MEMBER_CALL(index, soundtime);
			if (index == SPECIAL3 && weaponid != TF_WEAPON_MECHANICAL_ARM) return DETOUR_MEMBER_CALL(index, soundtime);

			int attr_name = -1;

			static int custom_weapon_fire_sound = GetItemSchema()->GetAttributeDefinitionByName("custom weapon fire sound")->GetIndex();
			static int custom_weapon_impact_sound = GetItemSchema()->GetAttributeDefinitionByName("custom impact sound")->GetIndex();
			static int custom_weapon_reload_sound = GetItemSchema()->GetAttributeDefinitionByName("custom weapon reload sound")->GetIndex();
			static int custom_weapon_deploy_sound = GetItemSchema()->GetAttributeDefinitionByName("custom weapon deploy sound")->GetIndex();
			switch (index) {
				case SINGLE: case BURST: case MELEE_MISS: case SPECIAL1: case SPECIAL3: attr_name = custom_weapon_fire_sound; break;
				case MELEE_HIT: case MELEE_HIT_WORLD: attr_name = custom_weapon_impact_sound; break;
				case RELOAD: attr_name = custom_weapon_reload_sound; break;
				case DEPLOY: attr_name = custom_weapon_deploy_sound; break;
			}
			
			const char *modelname = GetStringAttribute(weapon->GetItem()->GetAttributeList(), attr_name);
			if (weapon->GetOwner() != nullptr && modelname != nullptr) {
				if (rc_CTFPlayer_TraceAttack && index == BURST) return;
				PrecacheSound(modelname);
				weapon_sound_override_owner = ToTFPlayer(weapon->GetOwner());
				weapon_sound_override = modelname;

				weapon->GetOwner()->EmitSound(modelname, soundtime);
				return;
			}
			else if (modelname != nullptr) {
				if (rc_CTFPlayer_TraceAttack && index == BURST) return;
				PrecacheSound(modelname);
				weapon->EmitSound(modelname, soundtime);
				return;
			}
		}
		if (rc_CTFPlayer_TraceAttack && index == BURST && rtti_cast<CTFMinigun *>(reinterpret_cast<CTFWeaponBase *>(this)) != nullptr) {
			return;
		}
		auto oldOwner = weapon->GetOwner();
		DETOUR_MEMBER_CALL(index, soundtime);
		weapon_sound_override = nullptr;
	}

	DETOUR_DECL_STATIC(void, CBaseEntity_EmitSound, IRecipientFilter& filter, int iEntIndex, const char *sound, const Vector *pOrigin, float start, float *duration )
	{
		if (weapon_sound_override != nullptr) {
			//reinterpret_cast<CRecipientFilter &>(filter).AddRecipient(weapon_sound_override_owner);
			sound = weapon_sound_override;
		}
		DETOUR_STATIC_CALL(filter, iEntIndex, sound, pOrigin, start, duration);
	}

	RefCount rc_CTFWeaponFlameBall_FireProjectile;
	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFWeaponFlameBall_FireProjectile, CTFPlayer *player)
	{
		SCOPED_INCREMENT(rc_CTFWeaponFlameBall_FireProjectile);
		return DETOUR_MEMBER_CALL(player);
	}

	DETOUR_DECL_MEMBER(void, CBaseEntity_EmitSound_member, const char *sound, float start, float *duration)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);
		if (rc_CTFWeaponFlameBall_FireProjectile && (FStrEq(sound, "Weapon_DragonsFury.Single") || FStrEq(sound, "Weapon_DragonsFury.SingleCrit"))) {
			auto fury = rtti_cast<CEconEntity *>(entity);
			GET_STRING_ATTRIBUTE(fury, custom_weapon_fire_sound, soundfiring);
			if (soundfiring != nullptr) {
				sound = soundfiring;
			}
		}
		DETOUR_MEMBER_CALL(sound, start, duration);
	}

	int fire_bullet_num_shot = 0;
	RefCount rc_CTFPlayer_FireBullet;
	DETOUR_DECL_MEMBER(void, CTFPlayer_FireBullet, CTFWeaponBase *weapon, FireBulletsInfo_t& info, bool bDoEffects, int nDamageType, int nCustomDamageType)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_FireBullet);
		if (fire_bullet_num_shot == 0) {
			float rangeLimit = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, rangeLimit, max_bullet_range);
			if (rangeLimit != 0.0f) {
				info.m_flDistance = rangeLimit;
			}
		}
		DETOUR_MEMBER_CALL(weapon, info, bDoEffects, nDamageType, nCustomDamageType);
		fire_bullet_num_shot++;
	}

	DETOUR_DECL_STATIC(void, FX_FireBullets, CTFWeaponBase *pWpn, int iPlayer, const Vector &vecOrigin, const QAngle &vecAngles,
					 int iWeapon, int iMode, int iSeed, float flSpread, float flDamage, bool bCritical)
	{
		fire_bullet_num_shot = 0;
		DETOUR_STATIC_CALL(pWpn, iPlayer, vecOrigin, vecAngles, iWeapon, iMode, iSeed, flSpread, flDamage, bCritical);
		fire_bullet_num_shot = 0;
	}
	
	bool BounceArrow(CTFProjectile_Arrow *arrow, float bounce_speed) {
		trace_t &tr = CBaseEntity::GetTouchTrace();
		if (tr.DidHit()) {
			Vector pre_vel = arrow->GetAbsVelocity();
			Vector &normal = tr.plane.normal;
			Vector mirror_vel = (pre_vel - 2 * (pre_vel.Dot(normal)) * normal) * bounce_speed;
			arrow->SetAbsVelocity(mirror_vel);
			QAngle angles;
			VectorAngles(mirror_vel, angles);
			arrow->SetAbsAngles(angles);
			int resetHits = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(arrow->GetOriginalLauncher(), resetHits, reset_arrow_hits_on_bounce);
			if (resetHits != 0) {
				arrow->SetCustomVariable("HitEntities", Variant(arrow->GetCustomVariableInt<"HitEntities">() + Max(0, arrow->m_HitEntities->Count()-1)));
				arrow->m_HitEntities->RemoveAll();
			}
			
			if (!arrow->GetCustomVariableBool<"bounced">()) {
				float bounce_damage = 0;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(arrow->GetOriginalLauncher(), bounce_damage, grenade_bounce_damage);
				if (bounce_damage != 0) {
					arrow->SetCustomVariable("bounced", Variant(true));
					arrow->SetDamage(arrow->GetDamage() * (1 + bounce_damage));
				}
			}

			return true;
		}
		return false;
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Arrow_StrikeTarget, mstudiobbox_t *bbox, CBaseEntity *ent)
	{
		int no_headshot = 0;
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		CALL_ATTRIB_HOOK_INT_ON_OTHER(arrow->GetOriginalLauncher(), no_headshot, cannot_be_headshot);
		if (no_headshot != 0 && bbox->group == HITGROUP_HEAD) {
			bbox->group = HITGROUP_CHEST;
		}
		
		float bounce_speed_target = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(arrow->GetOriginalLauncher(), bounce_speed_target, arrow_target_bounce_speed);
		if (bounce_speed_target != 0) {
			BounceArrow(arrow, bounce_speed_target);
		}

		auto ret = DETOUR_MEMBER_CALL(bbox, ent);
		if (!ret && bounce_speed_target == 0) {
			float bounce_speed = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(arrow->GetOriginalLauncher(), bounce_speed, grenade_bounce_speed);
			if (bounce_speed != 0) {
				BounceArrow(arrow, bounce_speed);
			}
		}
		return ret;
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
			auto weapon = static_cast<CTFWeaponBase *>(ToBaseCombatWeapon(info.GetWeapon()));
			if (weapon != nullptr) {
				GET_STRING_ATTRIBUTE(weapon, custom_impact_sound, sound);
				if (sound != nullptr) {
					PrecacheSound(sound);
					CRecipientFilter filter;
					filter.AddRecipientsByPAS(ptr->endpos);
					EmitSound_t params;
                    params.m_pSoundName = sound;
                    params.m_flSoundTime = 0.0f;
                    params.m_pflSoundDuration = nullptr;
                    params.m_bWarnOnDirectWaveReference = true;
					params.m_pOrigin = &ptr->endpos;
					params.m_nChannel = CHAN_WEAPON;
                    CBaseEntity::EmitSound(filter, ENTINDEX(ptr->DidHit() ? ptr->m_pEnt : ent), params);

					//CBaseEntity::EmitSound(filter, 0, sound, &ptr->endpos);
				}

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
					int customparticleDirectHit = INVALID_STRING_INDEX;
					GetExplosionParticle(ToTFPlayer(weapon->GetOwnerEntity()), weapon, customparticle, customparticleDirectHit);

					if (customparticle != -1)
						TE_TFExplosion( filter, 0.0f, ptr->endpos, Vector(0,0,1), TF_WEAPON_GRENADELAUNCHER, ENTINDEX(info.GetAttacker()), -1, 11/*SPECIAL1*/, ToBaseCombatCharacter(ptr->m_pEnt) != nullptr ? customparticleDirectHit : customparticle);

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
					hit_explode_direct = ptr->m_pEnt;
					weapon->SetAbsOrigin(ptr->endpos - (weapon->WorldSpaceCenter() - origpos));
					TFGameRules()->RadiusDamage( radiusinfo );
					weapon->SetAbsOrigin(origpos);
					hit_explode_direct = nullptr;
					DETOUR_MEMBER_CALL(info2, vecDir, ptr, pAccumulator);
					return;
				}
			}
		}
		if (stickbomb != nullptr) {
			if (rc_CTFGameRules_RadiusDamage == 0) {
				
				minicrit = (info.GetDamageType() & DMG_RADIUS_MAX) != 0;
				stickbomb->GetTFPlayerOwner()->m_Shared->AddCond(TF_COND_NOHEALINGDAMAGEBUFF, 99.0f);
				DETOUR_MEMBER_CALL(info, vecDir, ptr, pAccumulator);
				stickbomb->GetTFPlayerOwner()->m_Shared->RemoveCond(TF_COND_NOHEALINGDAMAGEBUFF);
				DevMsg("mini crit set: %d\n",minicrit);
				return;
			}
		}

		DETOUR_MEMBER_CALL(useinfomod ? infomod : info, vecDir, ptr, pAccumulator);
	}

	DETOUR_DECL_MEMBER(bool, CRecipientFilter_IgnorePredictionCull)
	{
		return force_send_client || DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(float, CTFCompoundBow_GetProjectileSpeed)
	{
		auto bow = reinterpret_cast<CTFCompoundBow *>(this);
		float speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(bow, speed, mult_projectile_speed);
		speed *= DETOUR_MEMBER_CALL();
		return speed;
	}

	DETOUR_DECL_MEMBER(float, CTFProjectile_EnergyRing_GetInitialVelocity)
	{
		auto proj = reinterpret_cast<CTFProjectile_EnergyRing *>(this);
		float speed = 1.0f;
		if (proj->GetOriginalLauncher() != nullptr) {
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(proj->GetOriginalLauncher(), speed, mult_projectile_speed);
		}
		return speed * DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(CBaseEntity *, CTFWeaponBaseGun_FireEnergyBall, CTFPlayer *player, bool ring)
	{
		auto proj = DETOUR_MEMBER_CALL(player, ring);
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
		return speed * DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(float, CTFGrapplingHook_GetProjectileSpeed)
	{
		auto bow = reinterpret_cast<CTFWeaponBase *>(this);
		float speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(bow, speed, mult_projectile_speed);
		return speed * DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(float, CTFShotgunBuildingRescue_GetProjectileSpeed)
	{
		auto bow = reinterpret_cast<CTFWeaponBase *>(this);
		float speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(bow, speed, mult_projectile_speed);
		return speed * DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(float, CTFWeaponBaseGun_GetProjectileSpeed)
	{
		auto bow = reinterpret_cast<CTFWeaponBase *>(this);
		float speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(bow, speed, mult_projectile_speed);
		return speed;
	}

	int GetDamageType(CEconEntity *weapon, int value)
	{
		int headshot = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, headshot, can_headshot);
		if (headshot) {
			value |= DMG_USE_HITLOCATIONS;
		}
		
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

		int iAddDamageType = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, iAddDamageType, add_damage_type);
		value |= iAddDamageType;

		int iRemoveDamageType = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, iRemoveDamageType, remove_damage_type);
		value &= ~iRemoveDamageType;

		return value;
	}

	DETOUR_DECL_MEMBER(int, CTFWeaponBase_GetDamageType)
	{
		return GetDamageType(reinterpret_cast<CTFWeaponBase *>(this), DETOUR_MEMBER_CALL());
	}
	DETOUR_DECL_MEMBER(int, CTFSniperRifle_GetDamageType)
	{
		return GetDamageType(reinterpret_cast<CTFWeaponBase *>(this), DETOUR_MEMBER_CALL());
	}
	DETOUR_DECL_MEMBER(int, CTFSniperRifleClassic_GetDamageType)
	{
		return GetDamageType(reinterpret_cast<CTFWeaponBase *>(this), DETOUR_MEMBER_CALL());
	}
	DETOUR_DECL_MEMBER(int, CTFSMG_GetDamageType)
	{
		return GetDamageType(reinterpret_cast<CTFWeaponBase *>(this), DETOUR_MEMBER_CALL());
	}
	DETOUR_DECL_MEMBER(int, CTFRevolver_GetDamageType)
	{
		return GetDamageType(reinterpret_cast<CTFWeaponBase *>(this), DETOUR_MEMBER_CALL());
	}
	DETOUR_DECL_MEMBER(int, CTFPistol_ScoutSecondary_GetDamageType)
	{
		return GetDamageType(reinterpret_cast<CTFWeaponBase *>(this), DETOUR_MEMBER_CALL());
	}

	bool CanHeadshot(CBaseProjectile *projectile) {
		CBaseEntity *shooter = projectile->GetOriginalLauncher();

		if (shooter != nullptr) {
			int headshot = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(shooter, headshot, can_headshot);
			return headshot != 0;
		}
		return false;
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_HealingBolt_CanHeadshot)
	{
		return CanHeadshot(reinterpret_cast<CBaseProjectile *>(this)) || DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_EnergyRing_CanHeadshot)
	{
		return CanHeadshot(reinterpret_cast<CBaseProjectile *>(this)) || DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_GrapplingHook_CanHeadshot)
	{
		return CanHeadshot(reinterpret_cast<CBaseProjectile *>(this)) || DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_EnergyBall_CanHeadshot)
	{
		return CanHeadshot(reinterpret_cast<CBaseProjectile *>(this)) || DETOUR_MEMBER_CALL();
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

		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(int, CBaseCombatCharacter_OnTakeDamage, const CTakeDamageInfo& info)
	{
		if (info.GetWeapon() != nullptr) {
			auto character = reinterpret_cast<CBaseCombatCharacter *>(this);
			if (character->MyNextBotPointer() != nullptr && !character->IsPlayer() && !character->ClassMatches("tank_boss")) {
				float dmg_mult = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(info.GetWeapon(), dmg_mult, mult_dmg_vs_npc);
				if (dmg_mult != 1.0f) {
					CTakeDamageInfo newinfo = info;
					newinfo.SetDamage(newinfo.GetDamage() * dmg_mult);
					return DETOUR_MEMBER_CALL(newinfo);
				}
			}
		}
		return DETOUR_MEMBER_CALL(info);
	}

	float GetDamageMult(CBaseProjectile *proj)
	{
		if (proj->GetOriginalLauncher() == nullptr) return 1.0f;

		auto weapon = rtti_cast<CTFWeaponBaseGun *>(proj->GetOriginalLauncher());

		if (weapon == nullptr) return 1.0f; 

		int damage = weapon->GetTFWpnData().m_nDamage;

		if (damage == 0) return 1.0f;
		
		return weapon->GetProjectileDamage() / weapon->GetTFWpnData().m_nDamage;
	}

	void ApplyAttributesFromString(CTFPlayer *player, const char *attributes) {
		if (attributes != nullptr) {
			std::string str(attributes);
			boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

			auto it = tokens.begin();
			while (it != tokens.end()) {
				auto attribute = *it;
				if (++it == tokens.end())
					break;
				auto value = *it;
				if (++it == tokens.end())
					break;
				auto duration = stof(*it);
				player->AddCustomAttribute(attribute.c_str(), value, duration);
				it++;
			}
		}
	}
	
	
	RefCount rc_ApplyOnHitAttributes;
	bool IsApplyingOnHitAttributes()
	{
		return rc_ApplyOnHitAttributes;
	}

	void ApplyOnHitAttributes(CTFWeaponBase *weapon, CBaseEntity *victim, CBaseEntity *attacker, const CTakeDamageInfo &info) {
		SCOPED_INCREMENT(rc_ApplyOnHitAttributes);
		GET_STRING_ATTRIBUTE(weapon, custom_hit_sound, str);
		if (str != nullptr) {
			PrecacheSound(str);
			victim->EmitSound(str);
		}
		CTFPlayer *playerVictim = ToTFPlayer(victim);
		CTFPlayer *playerAttacker = ToTFPlayer(attacker);
		if (victim != nullptr && victim != attacker) {
			float damageReturnsAsHealth = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, damageReturnsAsHealth, damage_returns_as_health);
			if (damageReturnsAsHealth != 0.0f) {
				float health = damageReturnsAsHealth * info.GetDamage();
				if (health >= 0) {
					attacker->TakeHealth(health, DMG_GENERIC);
				} 
				else {
					attacker->TakeDamage(CTakeDamageInfo(attacker, attacker, weapon, vec3_origin, vec3_origin, (health * -1), DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE));
				}
			}

			std::vector<CTFPlayer *> condVictims; if (playerVictim != nullptr) condVictims.push_back(playerVictim);
			std::vector<CTFPlayer *> condAllies; if (playerAttacker != nullptr) condAllies.push_back(playerAttacker);
			float radialCondRadiusSq = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, radialCondRadiusSq, radial_cond);
			radialCondRadiusSq *= radialCondRadiusSq;
			if (radialCondRadiusSq != 0) {
				ForEachTFPlayer([&](CTFPlayer *playerl) {
					if (playerl != attacker && playerl->GetTeamNumber() == attacker->GetTeamNumber() && radialCondRadiusSq > playerl->GetAbsOrigin().DistToSqr(attacker->GetAbsOrigin())) {
						condAllies.push_back(playerl);
					}
					else if (playerl != victim && playerl->GetTeamNumber() != attacker->GetTeamNumber() && radialCondRadiusSq > playerl->GetAbsOrigin().DistToSqr(victim->GetAbsOrigin())) {
						condVictims.push_back(playerl);
					}
				});
			}

			int removecond_attr = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, removecond_attr, remove_cond_on_hit);
			if (removecond_attr != 0) {
				for (int i = 0; i < 4; i++) {
					int removecond = (removecond_attr >> (i * 8)) & 255;
					if (removecond != 0) {
						for (auto victim : condVictims) victim->m_Shared->RemoveCond((ETFCond)removecond);
					}
				}
			}

			int addcond_attr = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, addcond_attr, add_cond_on_hit);
			if (addcond_attr != 0) {
				float addcond_duration = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, addcond_duration, add_cond_on_hit_duration);
				if (addcond_duration == 0.0f) {
					addcond_duration = -1.0f;
				}
				for (int i = 0; i < 4; i++) {
					int addcond = (addcond_attr >> (i * 8)) & 255;
					if (addcond != 0) {
						for (auto victim : condVictims) victim->m_Shared->AddCond((ETFCond)addcond, addcond_duration, playerAttacker);
					}
				}
			}

			int self_addcond_attr = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, self_addcond_attr, self_add_cond_on_hit);
			if (self_addcond_attr != 0) {
				float addcond_duration = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, addcond_duration, self_add_cond_on_hit_duration);
				if (addcond_duration == 0.0f) {
					addcond_duration = -1.0f;
				}
				for (int i = 0; i < 4; i++) {
					int addcond = (self_addcond_attr >> (i * 8)) & 255;
					if (addcond != 0) {
						for (auto ally : condAllies) ally->m_Shared->AddCond((ETFCond)addcond, addcond_duration, playerAttacker);
					}
				}
			}
			GET_STRING_ATTRIBUTE(weapon, add_attributes_on_hit, attributes_string);
			for (auto victim : condVictims) ApplyAttributesFromString(victim, attributes_string);

			GET_STRING_ATTRIBUTE(weapon, self_add_attributes_on_hit, attributes_string_self);
			for (auto ally : condAllies) ApplyAttributesFromString(ally, attributes_string_self);
		}

		float stunDuration = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, stunDuration, stun_on_hit);

		int stunNoGiants = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, stunNoGiants, stun_on_hit_no_giants);

		if (playerVictim != nullptr && stunDuration != 0.0f && (stunNoGiants == 0 || !playerVictim->IsMiniBoss())) {
			GET_STRING_ATTRIBUTE(weapon, stun_on_hit_type, stunType);
			int stunId = TF_STUNFLAG_BONKSTUCK | TF_STUNFLAG_NOSOUNDOREFFECT;
			if (stunType != nullptr) {
				if (FStrEq(stunType, "movement")) {
					stunId = TF_STUNFLAG_SLOWDOWN;
				}
				else if (FStrEq(stunType, "panic")) {
					stunId = TF_STUNFLAG_SLOWDOWN | TF_STUNFLAG_NOSOUNDOREFFECT | TF_STUNFLAG_THIRDPERSON;
				}
				else if (FStrEq(stunType, "bonk")) {
					stunId = TF_STUNFLAG_BONKSTUCK;
				}
				else if (FStrEq(stunType, "ghost")) {
					stunId = TF_STUNFLAG_SLOWDOWN | TF_STUNFLAG_GHOSTEFFECT | TF_STUNFLAG_THIRDPERSON;
				}
				else if (FStrEq(stunType, "bigbonk")) {
					stunId = TF_STUNFLAG_CHEERSOUND | TF_STUNFLAG_BONKSTUCK;
				}
			}
			float stunSlow = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, stunSlow, stun_on_hit_slow);
			playerVictim->m_Shared->StunPlayer(stunDuration, stunSlow, stunId, playerAttacker);
		}
	}

	DETOUR_DECL_MEMBER(int, CTFGameRules_ApplyOnDamageModifyRules, CTakeDamageInfo& info, CBaseEntity *pVictim, bool b1)
	{
		bool restoreMinicritBoostOnKill = false;
		auto playerVictim = ToTFPlayer(pVictim);
		auto weapon = rtti_cast<CTFWeaponBase *>(info.GetWeapon());
		CBaseEntity *oldInflictor = nullptr;
		if (info.GetInflictor() != nullptr && info.GetInflictor()->GetOwnerEntity() != nullptr && info.GetInflictor()->GetOwnerEntity()->IsBaseObject() && info.GetInflictor()->GetCustomVariableBool<"customsentryweapon">()) {
			oldInflictor = info.GetInflictor();
			info.SetInflictor(info.GetInflictor()->GetOwnerEntity());
		}
		//Allow halloween kart to do more damage based on attributes

		if (info.GetAttacker() != nullptr && info.GetAttacker()->IsPlayer() && info.GetDamageCustom() == TF_DMG_CUSTOM_KART)
		{
			float dmg = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( info.GetAttacker(), dmg, mult_dmg );
			if (pVictim->IsPlayer())
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( info.GetAttacker(), dmg, mult_dmg_vs_players );

			info.SetDamage(info.GetDamage() * dmg);
		}

		//Allow mantreads to do more damage based on attributes

		if (info.GetAttacker() != nullptr && info.GetAttacker()->IsPlayer() && info.GetDamageCustom() == TF_DMG_CUSTOM_BOOTS_STOMP && info.GetWeapon() != nullptr && info.GetWeapon()->IsWearable())
		{
			float dmg = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( info.GetWeapon(), dmg, mult_dmg );
			if (pVictim->IsPlayer())
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( info.GetWeapon(), dmg, mult_dmg_vs_players );

			info.SetDamage(info.GetDamage() * dmg);
		}

		//Allow some attacks to do more damage based on attributes

		if (info.GetInflictor() != nullptr && info.GetInflictor()->GetCustomVariableBool<"applydmgmult">())
		{
			auto proj = rtti_cast<CBaseProjectile *>(info.GetInflictor());
			if (proj != nullptr) {
				info.SetDamage(info.GetDamage() * GetDamageMult(proj));
			}
		}
		
		if ((info.GetDamageType() & (DMG_BLAST | DMG_BULLET | DMG_BUCKSHOT | DMG_MELEE)) && ToTFPlayer(pVictim) != nullptr) {
			auto melee = rtti_cast<CTFWeaponBaseMelee *>(ToTFPlayer(pVictim)->GetActiveTFWeapon());
			if (melee != nullptr) {
				float swingTime = melee->GetCustomVariableFloat<"swingtime">();
				if (swingTime > gpGlobals->curtime) {
					Vector fwd;
					AngleVectors(pVictim->EyeAngles(), &fwd);
					float dot = info.GetDamageForce().Dot(fwd);
					return 0;
					//ClientMsg(ToTFPlayer(pVictim), "dot %f\n", dot);
					//if (dot < 0) {
					//	return 0;
					//}
				}
			}
		}
		
		if (weapon != nullptr) {
			
			float dmg = info.GetDamage();

			// Allow mult_dmg_bonus_while_half_dead mult_dmg_penalty_while_half_alive mult_dmg_with_reduced_health
			// to work on non melee weapons
			//
			if (info.GetAttacker() != nullptr && !weapon->IsMeleeWeapon()) {
				float maxHealth = info.GetAttacker()->GetMaxHealth();
				float health = info.GetAttacker()->GetHealth();
				float halfHealth = maxHealth * 0.5f;
				float dmgmult = 1.0f;
				if ( health < halfHealth )
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, dmgmult, mult_dmg_bonus_while_half_dead );
				}
				else
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, dmgmult, mult_dmg_penalty_while_half_alive );
				}

				// Some weapons change damage based on player's health
				float flReducedHealthBonus = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, flReducedHealthBonus, mult_dmg_with_reduced_health );
				if ( flReducedHealthBonus != 1.0f )
				{
					float flHealthFraction = clamp( health / maxHealth, 0.0f, 1.0f );
					flReducedHealthBonus = Lerp( flHealthFraction, flReducedHealthBonus, 1.0f );

					dmgmult *= flReducedHealthBonus;
				}
				dmg *= dmgmult;

				// Crit from behind should not work on tanks
				if (rtti_cast<CTFTankBoss *>(pVictim) == nullptr) {
					int critfromback = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, critfromback, crit_from_behind);
					Vector toEnt = pVictim->GetAbsOrigin() - info.GetAttacker()->GetAbsOrigin();
					if (critfromback != 0) {
						Vector entForward;
						AngleVectors(pVictim->EyeAngles(), &entForward);
						toEnt.z = 0;
						toEnt.NormalizeInPlace();
						if (DotProduct(toEnt, entForward) > 0.7071f)	// 75 degrees from center (total of 150)
						{
							info.SetCritType(CTakeDamageInfo::CRIT_FULL);
							info.AddDamageType(DMG_CRITICAL);
						}
					}
				}
			}
			

			//Allow some attacks to do more damage based on attributes

			if (info.GetAttacker() != nullptr) {
				float dmg_mult = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, dmg_mult, mult_dmg_before_distance);
				if (dmg_mult != 1.0f) {
					float maxDist = 2048.0f;
					float maxDistSpec = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, maxDistSpec, mult_dmg_before_distance_specify);
					if (maxDistSpec != 0.0f) {
						maxDist = maxDistSpec;
					}
					float distance = info.GetAttacker()->GetAbsOrigin().DistTo(pVictim->GetAbsOrigin());
					dmg_mult = RemapValClamped(distance, 0, maxDist, dmg_mult, 1.0f);
				}

				dmg *= dmg_mult;
			}

			float iDmgCurrentHealth = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, iDmgCurrentHealth, dmg_current_health);
			if (iDmgCurrentHealth != 0.0f)
			{
				dmg += pVictim->GetHealth() * iDmgCurrentHealth;
			}

			float iDmgMaxHealth = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, iDmgMaxHealth, dmg_max_health);
			if (iDmgMaxHealth != 0.0f)
			{
				dmg += pVictim->GetMaxHealth() * iDmgMaxHealth;
			}

			float iDmgMissingHealth = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, iDmgMissingHealth, dmg_missing_health);
			if (iDmgMissingHealth != 0.0f)
			{
				dmg += (pVictim->GetMaxHealth() - pVictim->GetHealth()) * iDmgMissingHealth;
			}

			if (info.GetAttacker() != nullptr && info.GetAttacker()->GetGroundEntity() == nullptr) {
				float dmg_mult = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, dmg_mult, mult_dmg_while_midair);
				dmg *= dmg_mult;
			}

			if (playerVictim != nullptr) {
				int iDmgType = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, iDmgType, special_damage_type);
				float dmg_mult = 1.0f;
				if (iDmgType == 1) {
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, dmg_mult, dmg_taken_mult_from_special_damage_type_1);
				}
				else if (iDmgType == 2) {
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, dmg_mult, dmg_taken_mult_from_special_damage_type_2);
				}
				else if (iDmgType == 3) {
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, dmg_mult, dmg_taken_mult_from_special_damage_type_3);
				}
				if (playerVictim->IsMiniBoss()) {
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, dmg_mult, mult_dmg_vs_giants);
				}

				if (playerVictim->InAirDueToKnockback()) {
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( weapon, dmg_mult, mult_dmg_vs_airborne );
				}

				GET_STRING_ATTRIBUTE(weapon, crit_on_cond, critOnCond);
				if (critOnCond != nullptr) {
					while (*critOnCond != '\0') {
						int cond = 0;
						critOnCond = ParseToInt(critOnCond, cond);
						if (playerVictim->m_Shared->InCond((ETFCond)cond)) {
							info.SetCritType(CTakeDamageInfo::CRIT_FULL);
							info.AddDamageType(DMG_CRITICAL);
							break;
						}
						if (*critOnCond != '\0')
							critOnCond++;
					}
				}

				GET_STRING_ATTRIBUTE(weapon, minicrit_on_cond, miniCritOnCond);
				if (miniCritOnCond != nullptr && info.GetCritType() == CTakeDamageInfo::CRIT_NONE) {
					while (*miniCritOnCond != '\0') {
						int cond = 0;
						miniCritOnCond = ParseToInt(miniCritOnCond, cond);
						if (playerVictim->m_Shared->InCond((ETFCond)cond)) {
							restoreMinicritBoostOnKill = !ToTFPlayer(info.GetAttacker())->m_Shared->GetCondData().InCond(TF_COND_NOHEALINGDAMAGEBUFF);
							ToTFPlayer(info.GetAttacker())->m_Shared->GetCondData().AddCondBit(TF_COND_NOHEALINGDAMAGEBUFF);
							break;
						}
						if (*miniCritOnCond != '\0')
							miniCritOnCond++;
					}
				}

				dmg *= dmg_mult;
			}
			else if (pVictim->MyNextBotPointer() != nullptr) {
				if (pVictim->ClassMatches("tank_boss")) {
					float dmg_mult = 1.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, dmg_mult, mult_dmg_vs_tanks);
					dmg *= dmg_mult;
				}
			}
			info.SetDamage(dmg);
		}
		
		if (info.GetAttacker() == pVictim) {
			float dmg_mult = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pVictim, dmg_mult, mult_dmgtaken_from_self);
			info.SetDamage(info.GetDamage() * dmg_mult);
		}

		int ret = DETOUR_MEMBER_CALL(info, pVictim, b1);

		// ApplyOnHit for non players and vs buildings
		if ((info.GetAttacker() == nullptr || !info.GetAttacker()->IsPlayer() || pVictim->IsBaseObject()) && weapon != nullptr) {
			ApplyOnHitAttributes(weapon, pVictim, info.GetAttacker(), info);
		}
		if (restoreMinicritBoostOnKill) {
			ToTFPlayer(info.GetAttacker())->m_Shared->GetCondData().RemoveCondBit(TF_COND_NOHEALINGDAMAGEBUFF);
		}
		if (oldInflictor != nullptr) {
			info.SetInflictor(oldInflictor);
		}

		if ((info.GetDamageType() & DMG_CRITICAL) && info.GetWeapon() != nullptr) {
			float crit = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(info.GetWeapon(), crit, mult_crit_dmg);
			float dmg = info.GetDamage();
			info.SetDamageBonus(dmg * crit - (dmg - info.GetDamageBonus()));
			info.SetDamage(dmg * crit);
		}

		// Fix razorback infinite healing exploit
		if (info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB && info.GetDamage() == 0.0f && info.GetDamageBonus() != 0.0f) {
			info.SetDamageBonus(0.0f);
		}

		if (info.GetWeapon() != nullptr && info.GetWeapon()->MyCombatWeaponPointer() != nullptr) {
			float dmg = info.GetDamage();

			if (info.GetAttacker() != nullptr && info.GetAttacker()->GetTeamNumber() == pVictim->GetTeamNumber()) {
				float dmg_mult = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(info.GetWeapon(), dmg_mult, mult_dmg_friendly_fire);
				dmg *= dmg_mult;
				if (dmg < 0.0f && pVictim->IsPlayer()) {
					HealPlayer(ToTFPlayer(pVictim), ToTFPlayer(info.GetAttacker()), rtti_cast<CEconEntity *>(info.GetWeapon()), -dmg, true, DMG_GENERIC);
					return 0;
				}
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
		bool ret = DETOUR_MEMBER_CALL();
		if (firing)
			minigun->m_iWeaponState = CTFMinigun::AC_STATE_FIRING;
			
		return ret;
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CObjectSapper_ApplyRoboSapperEffects, CTFPlayer *target, float duration) {
		int cannotApply = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(target, cannotApply, cannot_be_sapped);
		if (!cannotApply)
			DETOUR_MEMBER_CALL(target, duration);
	}

	DETOUR_DECL_MEMBER(bool, CObjectSapper_IsParentValid) {
		bool ret = DETOUR_MEMBER_CALL();
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
		bool ret = DETOUR_MEMBER_CALL();
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

	VHOOK_DECL(void, CUpgrades_StartTouch, CBaseEntity *pOther)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			CTFPlayer *player = ToTFPlayer(pOther);
			if (player != nullptr) {
				int cannotUpgrade = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(player,cannotUpgrade,cannot_upgrade);
				if (cannotUpgrade) {
#ifndef NO_MVM
                    if (Mod::Pop::PopMgr_Extensions::HasExtraLoadoutItems(player->GetPlayerClass()->GetClassIndex()) && menus->GetDefaultStyle()->GetClientMenu(ENTINDEX(player), nullptr) != MenuSource_BaseMenu) {
                        Mod::Pop::PopMgr_Extensions::DisplayExtraLoadoutItemsClass(player, player->GetPlayerClass()->GetClassIndex(), true);
					}
#endif
				}
			}
		}
		
		VHOOK_CALL(pOther);
	}
	
	DETOUR_DECL_MEMBER(void, CUpgrades_UpgradeTouch, CBaseEntity *pOther)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			CTFPlayer *player = ToTFPlayer(pOther);
			if (player != nullptr) {
				int cannotUpgrade = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(player,cannotUpgrade,cannot_upgrade);
				if (cannotUpgrade) {
					gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, TranslateText(player, "The Upgrade Station is disabled!"));
					return;
				}
			}
		}
		
		DETOUR_MEMBER_CALL(pOther);
	}

	DETOUR_DECL_MEMBER(void, CUpgrades_EndTouch, CBaseEntity *pOther)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			CTFPlayer *player = ToTFPlayer(pOther);
			if (player != nullptr) {
				int cannotUpgrade = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(player,cannotUpgrade,cannot_upgrade);
				if (cannotUpgrade) {
					gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, TranslateText(player, "The Upgrade Station is disabled!"));
#ifndef NO_MVM
					if (Mod::Pop::PopMgr_Extensions::HasExtraLoadoutItems(player->GetPlayerClass()->GetClassIndex())) {
        				CancelClientMenu(player);
					}
#endif
				}
			}
		}
		
		DETOUR_MEMBER_CALL(pOther);
	}

	class WeaponModule : public EntityModule
    {
    public:
	WeaponModule(CBaseEntity *entity) : EntityModule(entity) {}

	float crouchAccuracy = 1.0f;
	bool crouchAccuracyApplied = false;
	bool lastMoveAccuracyApplied = 1.0f;
	float consecutiveShotsScore = 0.0f;
	float lastConsecutiveShotsApplied = 1.0f;
	float totalAccuracyApplied = 1.0f;
	bool midairAccuracyApplied = false;

	float lastShotTime = 0.0f;
	int burstShotNumber = 0;
	float lastAttackCooldown = 0.0f;
	float attackTimeSave = 0.0f;
    };

	void OnWeaponUpdate(CTFWeaponBase *weapon) {
		CTFPlayer *owner = ToTFPlayer(weapon->GetOwnerEntity());
		if (owner != nullptr && (gpGlobals->tickcount % 5) == 3) {
			int alwaysCrit = GetFastAttributeInt(weapon, 0, ALWAYS_CRIT);

			if (alwaysCrit) {
				owner->m_Shared->AddCond(TF_COND_CRITBOOSTED_USER_BUFF, 0.5f, nullptr);
			}
			
			int addcond = GetFastAttributeInt(weapon, 0, ADD_COND_ON_ACTIVE);
			if (addcond != 0) {
				for (int i = 0; i < 3; i++) {
					int addcond_single = (addcond >> (i * 8)) & 255;
					if (addcond_single != 0) {
						owner->m_Shared->AddCond((ETFCond)addcond_single, 0.5f, owner);
					}
				}
			}
			float crouch_accuracy = GetFastAttributeFloat(weapon, 1.0f, DUCK_ACCURACY_MULT);
			float consecutive_accuracy = GetFastAttributeFloat(weapon, 1.0f, CONTINOUS_ACCURACY_MULT);
			float move_accuracy = GetFastAttributeFloat(weapon, 1.0f, MOVE_ACCURACY_MULT);
			float midair_accuracy = GetFastAttributeFloat(weapon, 1.0f, MIDAIR_ACCURACY_MULT);

			if (crouch_accuracy != 1.0f || consecutive_accuracy != 1.0f || move_accuracy != 1.0f || midair_accuracy != 1.0f) {
				auto mod = weapon->GetOrCreateEntityModule<WeaponModule>("weapon");
				static int accuracy_penalty_id = GetItemSchema()->GetAttributeDefinitionByName("spread penalty")->GetIndex();
				static int spread_angle_mult_id = GetItemSchema()->GetAttributeDefinitionByName("projectile spread angle mult")->GetIndex();
				float applyAccuracy = 1.0f;
				bool doApplyAccuracy = false;
				//auto attr = weapon->GetItem()->GetAttributeList().GetAttributeByID(accuracy_penalty_id);

				if (crouch_accuracy != 1.0f) {
					mod->crouchAccuracyApplied = owner->m_Local->m_bDucked;
					if (mod->crouchAccuracyApplied) {
						applyAccuracy *= crouch_accuracy;
					}
				}

				if (consecutive_accuracy != 1.0f) {
					float consecutive_accuracy_time = GetFastAttributeFloat(weapon, 0.0f, CONTINOUS_ACCURACY_TIME);
					float consecutiveAccuracyApply = RemapValClamped(mod->consecutiveShotsScore, 0.0f, consecutive_accuracy_time, 1.0f, consecutive_accuracy);
					applyAccuracy *= consecutiveAccuracyApply;
					if (weapon->m_flNextPrimaryAttack <= gpGlobals->curtime || weapon->m_bInReload) {
						mod->consecutiveShotsScore = Clamp(mod->consecutiveShotsScore - gpGlobals->frametime * 5 * 5, 0.0f, consecutive_accuracy_time);
					}
					mod->lastConsecutiveShotsApplied = consecutiveAccuracyApply;
				}

				if (move_accuracy != 1.0f) {
					float moveAccuracyApply = RemapValClamped(owner->GetAbsVelocity().Length(), 0.0f, owner->TeamFortress_CalculateMaxSpeed() * 0.5f, 1.0f, move_accuracy);
					applyAccuracy *= moveAccuracyApply;
					mod->lastMoveAccuracyApplied = moveAccuracyApply;
				}

				if (midair_accuracy != 1.0f) {
					mod->midairAccuracyApplied = owner->GetGroundEntity() == nullptr;
					if (mod->midairAccuracyApplied) {
						applyAccuracy *= midair_accuracy;
					}
				}
				doApplyAccuracy = mod->totalAccuracyApplied != applyAccuracy;
				if (doApplyAccuracy) {
					if (applyAccuracy == 1.0f) {
						weapon->GetItem()->GetAttributeList().RemoveAttributeByDefID(accuracy_penalty_id);
						weapon->GetItem()->GetAttributeList().RemoveAttributeByDefID(spread_angle_mult_id);
					}
					else {
						weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(accuracy_penalty_id, applyAccuracy);
						weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(spread_angle_mult_id, applyAccuracy);
					}
				}
				mod->totalAccuracyApplied = applyAccuracy;
			}
		}
	}

	inline bool UseBuilderAttributes(CBaseObject *object)
	{
		return object->GetBuilder() != nullptr && object->GetCustomVariableInt<"attributeoverride">() == 0;
	}

	template<FixedString lit>
	inline float GetBuildingAttributeFloat(CBaseObject *object, const char *attribute, bool overrideDefaultLogic)
	{
		float rate = object->GetCustomVariableFloat<lit>(1.0f);
		if (overrideDefaultLogic && !UseBuilderAttributes(object)) {
			rate /= CAttributeManager::AttribHookValue(1.0f, attribute, object->GetBuilder());
		}
		if (!overrideDefaultLogic && UseBuilderAttributes(object)) {
			rate *= CAttributeManager::AttribHookValue(1.0f, attribute, object->GetBuilder());
		}
		return rate;
	}

	template<FixedString lit>
	inline float GetBuildingAttributeInt(CBaseObject *object, const char *attribute, bool overrideDefaultLogic)
	{
		float rate = object->GetCustomVariableInt<lit>(0);
		if (overrideDefaultLogic && !UseBuilderAttributes(object)) {
			rate -= CAttributeManager::AttribHookValue(0, attribute, object->GetBuilder());
		}
		if (!overrideDefaultLogic && UseBuilderAttributes(object)) {
			rate += CAttributeManager::AttribHookValue(0, attribute, object->GetBuilder());
		}
		return rate;
	}

	template<FixedString lit, FixedString attribute>
	inline const char *GetBuildingAttributeString(CBaseObject *object)
	{
		//if (!UseBuildingKeyvalues(object)) return 1.0f;
		const char *str = object->GetCustomVariable<lit>("");
		auto player = ToTFPlayer(object->GetBuilder());
		if ((str == nullptr || str[0] == '\0') && player != nullptr) {
			str = player->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, player, PStrT<attribute>()).ToCStr();
		}
		return str;
	}

	CTFBaseRocket *sentry_gun_rocket = nullptr;
	RefCount rc_CObjectSentrygun_FireRocket;
	DETOUR_DECL_MEMBER(bool, CObjectSentrygun_FireRocket)
	{
		SCOPED_INCREMENT(rc_CObjectSentrygun_FireRocket);

		sentry_gun_rocket = nullptr;
		bool ret = DETOUR_MEMBER_CALL();
		if (ret) {
			auto sentrygun = reinterpret_cast<CObjectSentrygun *>(this);
			CTFPlayer *owner = sentrygun->GetBuilder();
			if (owner != nullptr) {
				float fireRocketRate = GetBuildingAttributeFloat<"rocketfireratemult">(sentrygun, "mult_firerocket_rate", false);
				if (fireRocketRate != 1.0f) {
					sentrygun->m_flNextRocketFire = (sentrygun->m_flNextRocketFire - gpGlobals->curtime) * fireRocketRate + gpGlobals->curtime;
				}
			}
			if (sentry_gun_rocket != nullptr) {
				sentry_gun_rocket->SetDamage(sentry_gun_rocket->GetDamage()*GetBuildingAttributeFloat<"damagemult">(sentrygun, "mult_engy_sentry_damage", true));
				sentry_gun_rocket->SetAbsVelocity(sentry_gun_rocket->GetAbsVelocity()*GetBuildingAttributeFloat<"projspeedmult">(sentrygun, "mult_sentry_rocket_projectile_speed", false));
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CBaseObject_CanBeUpgraded, CTFPlayer *player)
	{
		bool ret = DETOUR_MEMBER_CALL(player);
		if (ret) {
			auto obj = reinterpret_cast<CBaseObject *>(this);
			CTFPlayer *owner = obj->GetBuilder();
			if (owner != nullptr) {
				int maxLevel = GetBuildingAttributeInt<"maxlevel">(obj, "building_max_level", false);
				if (maxLevel > 0) {
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
			int maxLevel = GetBuildingAttributeInt<"maxlevel">(obj, "building_max_level", false);
			if (maxLevel > 0 && obj->m_iUpgradeLevel >= maxLevel) {
				servertools->SetKeyValue(obj, "defaultupgrade", CFmtStr("%d", maxLevel - 1));
				return;
			}
		}
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_SentryThink)
	{
		auto obj = reinterpret_cast<CObjectSentrygun *>(this);
		CTFPlayer *owner = obj->GetBuilder();
		DETOUR_MEMBER_CALL();
		if (owner != nullptr) {
			int rapidTick = 0;
			if (GetBuildingAttributeInt<"rapidfire">(obj, "sentry_rapid_fire", false) != 0) {
				//static BASEPTR addr = reinterpret_cast<BASEPTR> (AddrManager::GetAddr("CObjectSentrygun::SentryThink"));
				obj->ThinkSet(&CObjectSentrygun::SentryThink, gpGlobals->curtime, "SentrygunContext");
			}
		}
	}

	void SetBuildingStuff(CBaseObject *obj, CTFPlayer *builder)
	{
		int color = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( builder, color, building_color_rgb);
		if (color != 0) {
			obj->SetRenderColorR(color >> 16);
			obj->SetRenderColorG((color >> 8) & 255);
			obj->SetRenderColorB(color & 255);
		}
		float scale = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( builder, scale, building_scale);
		if (scale != 1.0f) {
			obj->SetModelScale(scale);
		}
	}
	DETOUR_DECL_MEMBER(void, CBaseObject_StartBuilding, CBaseEntity *builder)
	{
		DETOUR_MEMBER_CALL(builder);
		auto obj = reinterpret_cast<CBaseObject *>(this);
		CTFPlayer *owner = obj->GetBuilder();
		if (owner != nullptr) {
			SetBuildingStuff(obj, owner);
		}
	}

	DETOUR_DECL_MEMBER(void, CBaseObject_StartPlacement, CTFPlayer *builder)
	{
		DETOUR_MEMBER_CALL(builder);
		auto obj = reinterpret_cast<CBaseObject *>(this);
		if (builder != nullptr) {
			SetBuildingStuff(obj, builder);
		}
	}

	DETOUR_DECL_MEMBER(void, CObjectTeleporter_RecieveTeleportingPlayer, CTFPlayer *player)
	{
		auto tele = reinterpret_cast<CObjectTeleporter *>(this);
		if ( player == nullptr || tele->IsMarkedForDeletion() )
			return;

		Vector prepos = player->GetAbsOrigin();
		DETOUR_MEMBER_CALL(player);
		Vector postpos = player->GetAbsOrigin();
		if (tele->GetModelScale() != 1.0f && prepos != postpos) {
			postpos.z = tele->WorldSpaceCenter().z + tele->CollisionProp()->OBBMaxs().z + 1.0f;
			player->Teleport(&postpos, &(player->GetAbsAngles()), &(player->GetAbsVelocity()));
		}
	}

	inline int HasAllowFriendlyFire(CBaseEntity *launcher, CBaseEntity *player)
	{
		return GetFastAttributeInt(launcher != nullptr ? launcher : player, 0, launcher != nullptr ? (int)ALLOW_FRIENDLY_FIRE : (int)ALLOW_FRIENDLY_FIRE_PLAYER);
	}

	DETOUR_DECL_MEMBER(bool, CTFGameRules_FPlayerCanTakeDamage, CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo& info)
	{
		if (pPlayer != nullptr && pAttacker != nullptr && pPlayer->GetTeamNumber() == pAttacker->GetTeamNumber()) {
			int friendly = HasAllowFriendlyFire(info.GetWeapon(), pAttacker);
			if (friendly != 0)
				return true;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, friendly, receive_friendly_fire);
			if (friendly != 0)
				return true;
		}
		
		return DETOUR_MEMBER_CALL(pPlayer, pAttacker, info);
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_WantsLagCompensationOnEntity, const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		auto result = DETOUR_MEMBER_CALL(pPlayer, pCmd, pEntityTransmitBits);
		
		if (!result && player->GetTeamNumber() == pPlayer->GetTeamNumber()) {
			int friendly = HasAllowFriendlyFire(player->GetActiveWeapon(), player);
			result = friendly != 0;
		}
		return result;
	}

	RefCount rc_stop_stun;
	bool addcond_overridden = false;
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_StunPlayer, float duration, float slowdown, int flags, CTFPlayer *attacker)
	{
		if (rc_stop_stun) return;

		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		auto player = shared->GetOuter();

		float stun = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( player, stun, mult_stun_resistance);
		if (stun != 1.0f) {
			slowdown *= stun;
		}

		DETOUR_MEMBER_CALL(duration, slowdown, flags, attacker);
	}

	struct gamevcollisionevent_t : public vcollisionevent_t
	{
		Vector			preVelocity[2];
		Vector			postVelocity[2];
		AngularImpulse	preAngularVelocity[2];
		CBaseEntity		*pEntities[2];
	};

	CTFGrenadePipebombProjectile *grenade_proj;
	DETOUR_DECL_MEMBER(void, CTFGrenadePipebombProjectile_VPhysicsCollision, int index, gamevcollisionevent_t *pEvent)
	{
		grenade_proj = reinterpret_cast<CTFGrenadePipebombProjectile *>(this);

		int no_stick = 0;
		CBaseEntity *hit_ent = pEvent->pEntities[!index];
		CALL_ATTRIB_HOOK_INT_ON_OTHER(grenade_proj->GetOriginalLauncher(), no_stick, stickbomb_no_stick);
		if (no_stick != 0) {
			pEvent->pEntities[!index] = grenade_proj;
		}

		DETOUR_MEMBER_CALL(index, pEvent);

		if (no_stick != 0) {
			pEvent->pEntities[!index] = hit_ent;
			float flFizzle = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(grenade_proj->GetOriginalLauncher(), flFizzle, stickybomb_fizzle_time);
			if (flFizzle > 0) {
				grenade_proj->SetDetonateTimerLength(flFizzle);
			}
		}

		/*DevMsg("pre %d post %d touch\n", touched, grenade_proj->m_bTouched + 0);
		if (!touched && grenade_proj->m_bTouched) {
			float damage_bonus = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(grenade_proj->GetOriginalLauncher(), damage_bonus, grenade_bounce_damage);
			DevMsg("Add bounce %f\n", grenade_proj->GetDamage() * damage_bonus);

			if (damage_bonus != 0.0f) {
				grenade_proj->SetDamage(grenade_proj->GetDamage() * damage_bonus);
			}
		}*/

		float bounce_speed = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(grenade_proj->GetOriginalLauncher(), bounce_speed, grenade_bounce_speed);
		float bounce_speed_xy = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(grenade_proj->GetOriginalLauncher(), bounce_speed_xy, grenade_bounce_speed_xy);
		if (bounce_speed != 0.0f || bounce_speed_xy != 0.0f) {
			Vector &pre_vel = pEvent->preVelocity[index];
			Vector normal;
			pEvent->pInternalData->GetSurfaceNormal(normal);
			if (bounce_speed_xy != 0) {
				normal *= bounce_speed_xy;
			}
			Vector mirror_vel = (pre_vel - 2 * (pre_vel.Dot(normal)) * normal) * bounce_speed;
			AngularImpulse angularVelocity;
			grenade_proj->VPhysicsGetObject()->GetVelocity( &normal, &angularVelocity );

			grenade_proj->VPhysicsGetObject()->SetVelocity( &mirror_vel, &angularVelocity );
		}

		grenade_proj = nullptr;
	}

	DETOUR_DECL_MEMBER(void, CTFGrenadePipebombProjectile_PipebombTouch, CBaseEntity *ent)
	{
		auto proj = reinterpret_cast<CTFGrenadePipebombProjectile *>(this);

		bounce_damage_bonus = 0.0f;
		if (proj->m_bTouched) {
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(proj->GetOriginalLauncher(), bounce_damage_bonus, grenade_bounce_damage);
			if (bounce_damage_bonus != 0.0f) {
				proj->m_bTouched = false;
			}
		}
		hit_explode_direct = ent;
		
		DETOUR_MEMBER_CALL(ent);
		hit_explode_direct = nullptr;
	}

	DETOUR_DECL_STATIC(bool, PropDynamic_CollidesWithGrenades, CBaseEntity *ent)
	{
		if (grenade_proj != nullptr && grenade_proj->GetOriginalLauncher() != nullptr) {
			int explode_impact = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(grenade_proj->GetOriginalLauncher(), explode_impact, grenade_explode_on_impact);
			if (explode_impact != 0)
				return true;
		}

		return DETOUR_STATIC_CALL(ent);
	}
	
	RefCount rc_CTFPlayer_RegenThink;
	DETOUR_DECL_MEMBER(void, CTFPlayer_RegenThink)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_RegenThink);
		DETOUR_MEMBER_CALL();
		
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);

		int iSuicideCounter = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER ( player, iSuicideCounter, is_suicide_counter );
		if (iSuicideCounter != 0) {
			Vector pos = player->GetAbsOrigin();
			CTakeDamageInfo info = CTakeDamageInfo(player, player, nullptr, vec3_origin, pos, iSuicideCounter, DMG_PREVENT_PHYSICS_FORCE, 0, &pos);
			info.SetDamageCustom(TF_DMG_CUSTOM_TELEFRAG);
			player->TakeDamage(info);
		}
	}

	CTFPlayer *player_taking_damage = nullptr;

	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage, CTakeDamageInfo &info)
	{
		player_taking_damage = reinterpret_cast<CTFPlayer *>(this);
		int damage = DETOUR_MEMBER_CALL(info);
		player_taking_damage = nullptr;

		//Non sniper rifle explosive headshot
		auto weapon = info.GetWeapon();
		if (weapon != nullptr && info.GetAttacker() != nullptr && (info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT || info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT_DECAPITATION)) {
			auto tfweapon = rtti_cast<CTFWeaponBase *>(weapon->MyCombatWeaponPointer());
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

	DETOUR_DECL_MEMBER(bool, CSchemaAttributeType_Default_ConvertStringToEconAttributeValue , const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, attribute_data_union_t *out_pValue, bool floatforce)
	{
		if (pszValue[0] == 'i' || pszValue[0] == 'I') {
			out_pValue->m_UInt = (uint32)V_atoui64(pszValue + 1);
			return true;
		}
		else if (pszValue[0] == 'x' || pszValue[0] == 'X') {
			out_pValue->m_UInt = (uint32)strtoll(pszValue + 1, nullptr, 16);
			return true;
		}

		return DETOUR_MEMBER_CALL(pAttrDef, pszValue, out_pValue, floatforce);
	}

	DETOUR_DECL_MEMBER(bool, static_attrib_t_BInitFromKV_SingleLine, const char *context, KeyValues *attribute, CUtlVector<CUtlString> *errors, bool b)
	{
		if (V_strnicmp(attribute->GetName(), "SET BONUS: ", strlen("SET BONUS: ")) == 0) {
			attribute->SetName(attribute->GetName() + strlen("SET BONUS: "));
		}

		return DETOUR_MEMBER_CALL(context, attribute, errors, b);
	}

	std::vector<CHandle<CTFPlayer>> displacePlayers;

	DETOUR_DECL_MEMBER(bool, CTraceFilterObject_ShouldHitEntity, IHandleEntity *pServerEntity, int contentsMask)
	{
		CTraceFilterSimple *filter = reinterpret_cast<CTraceFilterSimple*>(this);
		
        // Always a player so ok to cast directly
        CBaseEntity *entityme = reinterpret_cast<CBaseEntity *>(const_cast<IHandleEntity *>(filter->GetPassEntity()));
		CBaseEntity *entityhit = EntityFromEntityHandle(pServerEntity);
		if (entityhit == nullptr) return true;
		if (entityme == nullptr) return DETOUR_MEMBER_CALL(pServerEntity, contentsMask);

		bool entityhit_player = entityhit->IsPlayer();

		if (entityhit_player || entityhit->IsBaseObject()) {
			bool me_collide = true;

			int not_solid = GetFastAttributeInt(entityme, 0, NOT_SOLID_TO_PLAYERS);
			me_collide = not_solid == 0;
			if (!me_collide)
				return false;

			if (entityhit_player) {
				int not_solid = GetFastAttributeInt(entityhit, 0, NOT_SOLID_TO_PLAYERS);
				if (not_solid != 0)
					return false;
			}
		}

		if (entityhit_player && entityhit->GetTeamNumber() != entityme->GetTeamNumber()) {
			int displace = GetFastAttributeFloat(entityme, 0.0f, DISPLACE_TOUCHED_ENEMIES);
			if (displace != 0.0f) {
				displacePlayers.push_back(static_cast<CTFPlayer *>(entityhit));
				return false;
			}
		}

		return DETOUR_MEMBER_CALL(pServerEntity, contentsMask);
	}

	RefCount rc_CTFPlayer_TFPlayerThink;
	DETOUR_DECL_MEMBER(void, CTFPlayer_StopTaunt, bool var1)
	{
		if (rc_CTFPlayer_TFPlayerThink && var1) {
			int iCancelTaunt = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER ( reinterpret_cast<CTFPlayer *>(this), iCancelTaunt, always_allow_taunt );
			if (iCancelTaunt != 0) {
				return;
			}
		}
		return DETOUR_MEMBER_CALL(var1);
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsAllowedToInitiateTauntWithPartner, CEconItemView *pEconItemView, char *pszErrorMessage, int cubErrorMessage)
	{
		int iAllowTaunt = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER ( reinterpret_cast<CTFPlayer *>(this), iAllowTaunt, always_allow_taunt );
		if (iAllowTaunt != 0) {
			return true;
		}
		return DETOUR_MEMBER_CALL( pEconItemView, pszErrorMessage, cubErrorMessage);
	}

	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_DeflectEntity, CBaseEntity *pTarget, CTFPlayer *pOwner, Vector &vecForward, Vector &vecCenter, Vector &vecSize)
	{
		int team = pTarget->GetTeamNumber();
		CBaseEntity *projOwner = pTarget->GetOwnerEntity();
		auto projEntity = rtti_cast<CBaseProjectile *>(pTarget);
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);

		if (rc_stop_our_team_deflect && pTarget->GetTeamNumber() == pOwner->GetTeamNumber()) {
			return false;
		}
		if (projEntity != nullptr && projEntity->IsDestroyable(false)) {
			int destroyProj = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, destroyProj, airblast_destroy_projectile);
			if (destroyProj != 0) {
				weapon->EmitSound("Halloween.HeadlessBossAxeHitWorld");
				projEntity->Destroy(false, true);
				return true;
			}
		}
		auto result = DETOUR_MEMBER_CALL(pTarget, pOwner, vecForward, vecCenter, vecSize);
		if (result) {
			int deflectKeepTeam = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER ( weapon, deflectKeepTeam, reflect_keep_team );
			if (deflectKeepTeam != 0) {
				pTarget->ChangeTeam(team);
				pTarget->SetOwnerEntity(projOwner);
			}

			int reflectMagnet = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, reflectMagnet, reflect_magnet);
			if (reflectMagnet != 0) {
				IPhysicsObject *physics = pTarget->VPhysicsGetObject();

				if (physics != nullptr) {
					AngularImpulse ang_imp;
					Vector vel;
					physics->GetVelocity(&vel, &ang_imp);
					float len = vel.Length();
					vel = pOwner->WorldSpaceCenter() - pTarget->GetAbsOrigin();
					vel.NormalizeInPlace();
					vel *= len;
					physics->SetVelocity(&vel, &ang_imp);
				}
				else {
					Vector vel = pTarget->GetAbsVelocity();
					float len = vel.Length();
					vel = pOwner->WorldSpaceCenter() - pTarget->GetAbsOrigin();
					vel.NormalizeInPlace();
					vel *= len;
					pTarget->SetAbsVelocity(vel);
				}
			}

			float deflectStrength = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, deflectStrength, mult_reflect_velocity);
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
			CBaseCombatWeapon *weapon = info.GetWeapon()->MyCombatWeaponPointer();
			if (weapon != nullptr && weapon->GetItem() != nullptr) {
				
				GET_STRING_ATTRIBUTE(weapon, custom_kill_icon, str);
				if (str != nullptr)
					return str;
			}
		}
		return DETOUR_MEMBER_CALL(info, pVictim, iWeaponID);
	}

	class PlayerTouchModule : public EntityModule
	{
	public:
		PlayerTouchModule(CBaseEntity *entity) {}

		std::unordered_map<CBaseEntity *, float> touchTimes;
	};

	DETOUR_DECL_MEMBER(void, CTFPlayer_Touch, CBaseEntity *toucher)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (toucher == nullptr || player == nullptr) return;

		DETOUR_MEMBER_CALL(toucher);

		if (toucher->GetTeamNumber() == player->GetTeamNumber()) return;

		if (toucher->IsBaseObject()) {
			float stomp = GetFastAttributeFloat(player, 0.0f, STOMP_BUILDING_DAMAGE);
			if (stomp != 0.0f) {
				
				float stompTime = GetFastAttributeFloat(player, 0.0f, STOMP_PLAYER_TIME);

				if (stompTime == 0.0f || (gpGlobals->curtime - player->GetOrCreateEntityModule<PlayerTouchModule>("playertouch")->touchTimes[toucher]) > stompTime) {
					CTakeDamageInfo info(player, player, player->GetActiveTFWeapon(), vec3_origin, vec3_origin, stomp, DMG_BLAST);
					toucher->TakeDamage(info);
					if (stompTime != 0.0f)
						player->GetOrCreateEntityModule<PlayerTouchModule>("playertouch")->touchTimes[toucher] = gpGlobals->curtime;
				}
			}
		}
		else if (toucher->IsPlayer()) {
			
			float stompTime = GetFastAttributeFloat(player, 0.0f, STOMP_PLAYER_TIME);
			if (stompTime == 0.0f || (gpGlobals->curtime - player->GetOrCreateEntityModule<PlayerTouchModule>("playertouch")->touchTimes[toucher]) > stompTime) {
				float stomp = GetFastAttributeFloat(player, 0.0f, STOMP_PLAYER_DAMAGE);
				if (stomp != 0.0f) {
					CTakeDamageInfo info(player, player, player->GetActiveTFWeapon(), vec3_origin, vec3_origin, stomp, DMG_BLAST);
					toucher->TakeDamage(info);
				}

				float knockback = GetFastAttributeFloat(player, 0.0f, STOMP_PLAYER_FORCE);
				if (knockback != 0.0f) {
					Vector vec = toucher->GetAbsOrigin() - player->GetAbsOrigin();
					vec.NormalizeInPlace();
					vec.z = 1.0f;
					vec *= knockback;
					ToTFPlayer(toucher)->ApplyGenericPushbackImpulse(vec, player);
				}
				if (stompTime != 0.0f)
					player->GetOrCreateEntityModule<PlayerTouchModule>("playertouch")->touchTimes[toucher] = gpGlobals->curtime;
			}
		}
	}

	void InspectAttributes(CTFPlayer *target, CTFPlayer *player, bool force, int slot = -2);

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CUpgrades_PlayerPurchasingUpgrade, CTFPlayer *player, int itemslot, int upgradeslot, bool sell, bool free, bool refund)
	{
		if (!refund) {
			auto upgrade = reinterpret_cast<CUpgrades *>(this);
			
			if (itemslot >= 0 && upgradeslot >= 0 && upgradeslot < CMannVsMachineUpgradeManager::Upgrades().Count()) {
				
				CEconEntity *entity = GetEconEntityAtLoadoutSlot(player, itemslot);

				if (entity != nullptr) {
					int iCannotUpgrade = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER ( entity, iCannotUpgrade, cannot_be_upgraded );
					if (iCannotUpgrade > 0) {
						if (!sell) {
							gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, TranslateText(player, "This weapon is not upgradeable"));
						}
						return;
					}
				}
			}
		}
		DETOUR_MEMBER_CALL(player, itemslot, upgradeslot, sell, free, refund);
		
		if (!refund) {
			InspectAttributes(player, player , true, itemslot);
		}

		// Delete old dropped weapons if a player refunded an upgrade
		if (sell && !refund) {
			if (itemslot >= 0) {
				CEconEntity *entity = GetEconEntityAtLoadoutSlot(player, itemslot);
				if (entity != nullptr) {
					ForEachEntityByRTTI<CTFDroppedWeapon>([&](CTFDroppedWeapon *weapon) {
						if (weapon->m_Item->m_iItemID == entity->GetItem()->m_iItemID) {
							weapon->Remove();
						}
					});
				}
			}
		}
	}
	
	RefCount rc_CTFPlayer_ReapplyItemUpgrades;
	DETOUR_DECL_MEMBER(void, CTFPlayer_ReapplyItemUpgrades, CEconItemView *item_view)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		bool no_upgrade = false;
		if (item_view != nullptr) {
			auto attr = item_view->GetAttributeList().GetAttributeByName("cannot be upgraded");
			no_upgrade = attr != nullptr && attr->GetValuePtr()->m_Float > 0.0f;
		}
		SCOPED_INCREMENT_IF(rc_CTFPlayer_ReapplyItemUpgrades, no_upgrade);
		DETOUR_MEMBER_CALL(item_view);
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, unsigned short, CUpgrades_ApplyUpgradeToItem, CTFPlayer* player, CEconItemView *item, int upgrade, int cost, bool downgrade, bool fresh)
	{
		if (rc_CTFPlayer_ReapplyItemUpgrades)
		{
			return INVALID_ATTRIB_DEF_INDEX;
		}

        return DETOUR_MEMBER_CALL(player, item, upgrade, cost, downgrade, fresh);
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

		return DETOUR_MEMBER_CALL() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Arrow_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_Arrow *>(this);

		return DETOUR_MEMBER_CALL() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Flare_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_Flare *>(this);

		return DETOUR_MEMBER_CALL() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_EnergyBall_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_EnergyBall *>(this);

		return DETOUR_MEMBER_CALL() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFGrenadePipebombProjectile_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFGrenadePipebombProjectile *>(this);

		return DETOUR_MEMBER_CALL() && IsDeflectable(ent);
	}

	DETOUR_DECL_MEMBER(int, CTFGrenadePipebombProjectile_OnTakeDamage, CTakeDamageInfo &info)
	{
		auto ent = reinterpret_cast<CTFGrenadePipebombProjectile *>(this);
		if (!IsDeflectable(ent)) {
			return 0;
		}
		return DETOUR_MEMBER_CALL(info);
	}

	RefCount rc_CTFGrenadePipebombProjectile_DetonateStickies;
	DETOUR_DECL_MEMBER(void, CTFGrenadePipebombProjectile_DetonateStickies)
	{
		SCOPED_INCREMENT(rc_CTFGrenadePipebombProjectile_DetonateStickies);
		return DETOUR_MEMBER_CALL();
	}

    DETOUR_DECL_STATIC(int, UTIL_EntitiesInSphere, const Vector& center, float radius, CFlaggedEntitiesEnum *pEnum)
	{
		if (!rc_CTFGrenadePipebombProjectile_DetonateStickies) return DETOUR_STATIC_CALL(center, radius, pEnum);

		auto result = DETOUR_STATIC_CALL(center, radius, pEnum);
		auto list = pEnum->GetList();
		for (auto i = 0; i < pEnum->GetCount(); i++) {
			auto ent = rtti_cast<CBaseProjectile *>(list[i]);
			if (ent != nullptr && !IsDeflectable(ent)) {
				for (auto j = i + 1; j < pEnum->GetCount(); j++) {
					list[j-1] = list[j];
				}
				result--;
			}
		}
        return result;
	}
	
	// Stop short circuit from deflecting the projectile
	
	RefCount rc_CTFProjectile_MechanicalArmOrb_CheckForProjectiles;
	DETOUR_DECL_MEMBER(void, CTFProjectile_MechanicalArmOrb_CheckForProjectiles)
	{
		SCOPED_INCREMENT(rc_CTFProjectile_MechanicalArmOrb_CheckForProjectiles);
		DETOUR_MEMBER_CALL();
	}

	RefCount rc_CTFProjectile_Arrow_ArrowTouch;

	RefCount rc_CTFFlameManager_OnCollide;
	DETOUR_DECL_MEMBER(bool, CBaseEntity_InSameTeam, CBaseEntity *other)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		if (rc_CTFProjectile_MechanicalArmOrb_CheckForProjectiles)
		{
			if (!IsDeflectable(static_cast<CBaseProjectile *>(ent))) {
				return true;
			}
		}
		if (rc_CTFFlameManager_OnCollide) {
			return false;
		}
/*
		if ((ent->m_fFlags & FL_GRENADE) && (other->m_fFlags & FL_GRENADE) && strncmp(ent->GetClassname(), "tf_projectile", strlen("tf_projectile")) == 0) {
			if (!IsDeflectable(static_cast<CBaseProjectile *>(ent))) {
				return true;
			}
		}*/

		return DETOUR_MEMBER_CALL(other);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnAddBalloonHead)
	{
        DETOUR_MEMBER_CALL();
		CTFPlayer *player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();

		float gravity = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, gravity, player_gravity_ballon_head);
		if (gravity != 0.0f)
			player->SetGravity(gravity);
	}


	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ApplyOnHitAttributes, CBaseEntity *ent, CTFPlayer *player, const CTakeDamageInfo& info)
	{
		DETOUR_MEMBER_CALL(ent, player, info);
		if (ent != nullptr)
			ApplyOnHitAttributes(reinterpret_cast<CTFWeaponBase *>(this), ent, player, info);
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_MakeScaledBuilding, CTFPlayer *player)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
		//DevMsg("sentry deploy %d %d\n", sentry->m_bCarryDeploy + 0, sentry->m_bCarried + 0);
		if (sentry->m_bCarried)
			return;
		DETOUR_MEMBER_CALL(player);
		
	}

	DETOUR_DECL_MEMBER(void, CObjectTeleporter_TeleporterTouch, CBaseEntity *player)
	{
		auto tele = reinterpret_cast<CObjectTeleporter *>(this);
		
		if (player->IsPlayer()) {
			int iCannotTeleport = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, iCannotTeleport, cannot_be_teleported);
			if (iCannotTeleport != 0)
				return;
		}
		bool restore = false;
		if (tele->m_iTeleportType == 2 && GetBuildingAttributeInt<"bidirectional">(tele, "bidirectional_teleport", true)) {
			tele->m_iTeleportType = 1;
			restore = true;
		}
		DETOUR_MEMBER_CALL(player);
		if (restore) {
			tele->m_iTeleportType = 2;
		}
	}

	DETOUR_DECL_MEMBER(float, CWeaponMedigun_GetTargetRange)
	{
		auto weapon = reinterpret_cast<CWeaponMedigun *>(this);
		
		float range = DETOUR_MEMBER_CALL();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, range, mult_medigun_range);
		return range;
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBotTacticalMonitor_ShouldOpportunisticallyTeleport, CTFPlayer *bot)
	{
		bool result = DETOUR_MEMBER_CALL(bot);
		if (result) {
			int iCannotTeleport = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(bot, iCannotTeleport, cannot_be_teleported);
			result = iCannotTeleport == 0;
		}
		return result;
	}
	

	DETOUR_DECL_MEMBER(void, CTFProjectile_Arrow_ArrowTouch, CBaseEntity *pOther)
	{
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		auto launcher = arrow->GetOriginalLauncher();
		if (pOther->IsBaseObject() && pOther->GetTeamNumber() == arrow->GetTeamNumber() && ToBaseObject(pOther)->HasSapper()) {
			int can_damage_sappers = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(launcher, can_damage_sappers, set_dmg_apply_to_sapper);

			if (can_damage_sappers != 0) {
				if (rtti_cast<CObjectSapper *>(pOther->FirstMoveChild()) != nullptr) {
					pOther = pOther->FirstMoveChild();
				}
			}
		}

		if (gpGlobals->curtime - arrow->m_flTimeInit >= 10.0f) {
			float lifetime = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(launcher, lifetime, projectile_lifetime);
			float bounce = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(launcher, bounce, grenade_bounce_speed);
			if (lifetime != 0 || bounce != 0) {
				arrow->m_flTimeInit = gpGlobals->curtime - 9.0f;
			}
		}

		SCOPED_INCREMENT(rc_CTFProjectile_Arrow_ArrowTouch);
		DETOUR_MEMBER_CALL(pOther);

		if (pOther->IsAlive() && pOther->GetTeamNumber() != arrow->GetTeamNumber() && pOther->MyCombatCharacterPointer() != nullptr) {
			float snapRadius = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(launcher, snapRadius, arrow_snap_to_next_target_radius);
			if (snapRadius > 0) {
				const int maxCollectedEntities = 128;
				CBaseEntity	*pObjects[ maxCollectedEntities ];
				
				CFlaggedEntitiesEnum iter = CFlaggedEntitiesEnum(pObjects, maxCollectedEntities, FL_CLIENT | FL_FAKECLIENT | FL_NPC );

				partition->EnumerateElementsInSphere(PARTITION_ENGINE_NON_STATIC_EDICTS, arrow->GetAbsOrigin(), snapRadius, false, &iter);
				int count = iter.GetCount();

				CBaseEntity *bestTarget = nullptr;
				float bestTargetDist = FLT_MAX;
				Vector pos = arrow->GetAbsOrigin();
				Vector forward;
				AngleVectors(arrow->GetAbsAngles(), &forward);
				for ( int i = 0; i < count; i++ )
				{
					CBaseEntity *pObject = pObjects[i];
					if (bestTarget != nullptr && bestTarget->IsPlayer() && !pObject->IsPlayer()) continue;
					if (pObject == pOther) continue;
					if (!pObject->IsAlive()) continue;
					if (pObject->GetTeamNumber() == arrow->GetTeamNumber()) continue;
					if (pObject->MyCombatCharacterPointer() == nullptr) continue;
					if (arrow->m_HitEntities->Find(pObject->entindex()) != -1) continue;

					auto dist = pObject->WorldSpaceCenter().DistToSqr(pos);
					if (dist < bestTargetDist) {
						
						CTraceFilterIgnoreFriendlyCombatItems filter(arrow, COLLISION_GROUP_NONE, false);
						trace_t result;
						UTIL_TraceLine(pos, pos + sqrt(dist) * forward, MASK_SHOT, &filter, &result);
						if (!result.DidHit() || result.m_pEnt == pObject) {
							bestTargetDist = dist;
							bestTarget = pObject;
						}
					}
				}
				if (bestTarget != nullptr) {
					float velocity = arrow->GetAbsVelocity().Length();
					forward = (bestTarget->WorldSpaceCenter() - pos).Normalized() * velocity;
					QAngle angles;
					VectorAngles(forward, angles);
					arrow->SetAbsVelocity(forward);
					arrow->SetAbsAngles(angles);
				}
			}
		}

		int iPenetrateLimit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(launcher, iPenetrateLimit, projectile_penetration_limit);
		if (iPenetrateLimit != 0 && arrow->m_HitEntities->Count() + arrow->GetCustomVariableInt<"HitEntities">() >= iPenetrateLimit + 1) {
			arrow->Remove();
		}
	}

	THINK_FUNC_DECL(UpdateArrowTrail)
	{
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		auto launcher = arrow->GetOriginalLauncher();

		float bounce = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(launcher, bounce, grenade_bounce_speed);
		float lifetime = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(launcher, lifetime, projectile_lifetime);
		if (bounce != 0) {
			arrow->SetNextThink(-1, "FadeTrail");
		}
		float targetBounce = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(launcher, targetBounce, arrow_target_bounce_speed);
		if (targetBounce != 0) {
			arrow->CollisionProp()->SetSolidFlags(arrow->CollisionProp()->GetSolidFlags() & ~(FSOLID_NOT_SOLID | FSOLID_TRIGGER));
		}
		
		else if (lifetime != 0) {
			arrow->SetNextThink(gpGlobals->curtime + lifetime, "FadeTrail");
		}
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_Arrow_CreateTrail)
	{
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		DETOUR_MEMBER_CALL();
		THINK_FUNC_SET(arrow, UpdateArrowTrail, gpGlobals->curtime+0.01f);
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, int, CTFRadiusDamageInfo_ApplyToEntity, CBaseEntity *ent)
	{
		auto info = reinterpret_cast<CTFRadiusDamageInfo *>(this);
		if (hit_entities_explosive_max != 0 && hit_entities_explosive >= hit_entities_explosive_max)
			return 0;
		int healthpre = ent->GetHealth();
		//DevMsg("Applytoentity damage %f %d\n", info->m_DmgInfo->GetDamage(), ent->CollisionProp()->IsPointInBounds(info->m_vecOrigin));
		auto result = DETOUR_MEMBER_CALL(ent);
		if (ent->GetHealth() != healthpre) {
			hit_entities_explosive++;
		}
		return result;
	}
	
	class PenetrationNumberModule : public EntityModule
	{
	public:
		PenetrationNumberModule(CBaseEntity *entity) {}

		int penetrationCount = 0;
	};
	
	DETOUR_DECL_MEMBER(bool, CTFFlameManager_BCanBurnEntityThisFrame, CBaseEntity* entity)
	{
		auto flamemgr = reinterpret_cast<CBaseEntity *>(this);

		bool ret = DETOUR_MEMBER_CALL(entity);
		if (ret) {
			int iMaxAoe = GetFastAttributeInt(flamemgr->GetOwnerEntity(), 0, MAX_AOE_TARGETS);
			if (iMaxAoe != 0) {
				int &counter = flamemgr->GetOrCreateEntityModule<PenetrationNumberModule>("penetrationnumber")->penetrationCount; 
				int min_delay;
				switch(iMaxAoe) {
					case 1: min_delay = 5; break;
					case 2: min_delay = 3; break;
					case 3: min_delay = 2; break;
					default: min_delay = 1; 
				}
				
				ret = gpGlobals->tickcount - counter >= min_delay;
				if (ret)
					counter = gpGlobals->tickcount;
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_BallOfFire_RocketTouch, CBaseEntity *pOther)
	{
		auto arrow = reinterpret_cast<CBaseProjectile *>(this);

		if (pOther == nullptr) {
			DETOUR_MEMBER_CALL(pOther);
			return;
		}

		int health_pre = pOther->GetHealth();
		DETOUR_MEMBER_CALL(pOther);
		
		if (pOther != arrow && health_pre != pOther->GetHealth()) {
			int &counter = arrow->GetOrCreateEntityModule<PenetrationNumberModule>("penetrationnumber")->penetrationCount;
			counter += 1;
			int iPenetrateLimit = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(arrow->GetOriginalLauncher(), iPenetrateLimit, projectile_penetration_limit);
			if (iPenetrateLimit != 0 && counter >= iPenetrateLimit) {
				arrow->Remove();
			}
		}
	}

	bool bison_projectile_touch = false;
	DETOUR_DECL_MEMBER(void, CTFProjectile_EnergyRing_ProjectileTouch, CBaseEntity *pOther)
	{
		auto arrow = reinterpret_cast<CBaseProjectile *>(this);

		if (pOther == nullptr) {
			DETOUR_MEMBER_CALL(pOther);
			return;
		}

		int health_pre = pOther->GetHealth();
		bison_projectile_touch = pOther->GetTeamNumber() == arrow->GetTeamNumber() && gpGlobals->tickcount % 2 == 0;
		DETOUR_MEMBER_CALL(pOther);	
		bison_projectile_touch = false;
		
		if (pOther != arrow && health_pre != pOther->GetHealth()) {
			int &counter = arrow->GetOrCreateEntityModule<PenetrationNumberModule>("penetrationnumber")->penetrationCount;
			counter += 1;
			int iPenetrateLimit = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(arrow->GetOriginalLauncher(), iPenetrateLimit, projectile_penetration_limit);
			if (iPenetrateLimit != 0 && counter >= iPenetrateLimit) {
				arrow->Remove();
			}
		}
	}

	DETOUR_DECL_MEMBER(bool, CTFProjectile_EnergyRing_ShouldPenetrate)
	{
		if (bison_projectile_touch) {
			bison_projectile_touch = false;
			return false;
		}
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayerShared_CanRecieveMedigunChargeEffect, int eType)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		int immune = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, immune, effect_immunity);
		if (immune != 0) {
			return false;
		}

		return DETOUR_MEMBER_CALL(eType);
	} 

	RefCount rc_CWeaponMedigun_StartHealingTarget;
	CTFPlayer *startHealingTargetHealer;
	CBaseEntity *startHealingTarget;
	
	RefCount rc_CTFPlayerShared_AddCondIn;
	RefCount rc_CTFPlayerShared_AddCond;
	RefCount rc_CTFPlayerShared_RemoveCond;
	RefCount rc_CTFPlayerShared_InCond;
	CBaseEntity *addcond_provider = nullptr;
	CBaseEntity *addcond_provider_item = nullptr;
	ETFCond addcond_specific_cond = TF_COND_INVALID;
	RefCount rc_CTFPlayerShared_PulseRageBuff;

	int aoe_in_sphere_max_hit_count = 0;
	int aoe_in_sphere_hit_count = 0;


	class AddCondAttributeImmunity : public EntityModule
	{
	public:
		AddCondAttributeImmunity(CBaseEntity *entity) : EntityModule(entity) {}

		std::vector<int> conds;
		std::vector<CEconItemAttributeDefinition *> attributes;
	};

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond2, ETFCond nCond, float flDuration, CBaseEntity *pProvider)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		if (pProvider != player && (nCond == TF_COND_URINE || nCond == TF_COND_MAD_MILK || nCond == TF_COND_MARKEDFORDEATH || nCond == TF_COND_MARKEDFORDEATH_SILENT)) {
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, flDuration, mult_debuff_duration);
		}

		auto mod = player->GetEntityModule<AddCondAttributeImmunity>("addcondimmunity");
		if (mod != nullptr && std::find(mod->conds.begin(), mod->conds.end(), (int) nCond) != mod->conds.end()) return;

		DETOUR_MEMBER_CALL(nCond, flDuration, pProvider);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond, ETFCond nCond, float flDuration, CBaseEntity *pProvider)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCondIn);
		CTFPlayer *player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();

		if (pProvider != player && pProvider != nullptr && pProvider->IsPlayer()) {
			int immune = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, immune, effect_immunity);
			if (immune != 0) {
				return;
			}
		}

		if (rc_CTFPlayerShared_AddCond)
        {
			if (addcond_specific_cond != TF_COND_INVALID && addcond_specific_cond != nCond) return DETOUR_MEMBER_CALL(nCond, flDuration, pProvider);

			if (aoe_in_sphere_max_hit_count != 0 && ++aoe_in_sphere_hit_count > aoe_in_sphere_max_hit_count) {
				return;
			}

			// If one condition was added due to another condition, ignore it
			if (rc_CTFPlayerShared_AddCondIn > 1) return DETOUR_MEMBER_CALL(nCond, flDuration, pProvider);

			auto attribProvider = addcond_provider_item != nullptr ? addcond_provider_item : addcond_provider;
			addcond_overridden = false;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(attribProvider, flDuration, mult_effect_duration);
			int iCondOverride = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(attribProvider, iCondOverride, effect_cond_override);

			//DevMsg("add cond pre %d\n", iCondOverride);
			// Allow up to 4 addconds with bit shifting
			if (iCondOverride != 0) {
				for (int i = 0; i < 4; i++) {
					int addcond = (iCondOverride >> (i * 8)) & 255;
					//DevMsg("add cond post %d\n", addcond);
					if (addcond != 0) {
						addcond_overridden = true;
						DETOUR_MEMBER_CALL((ETFCond) addcond, flDuration, pProvider);
						
					}
				}
			}

			auto weapon = rtti_cast<CEconEntity *>(addcond_provider_item);
			//Msg(CFmtStr("provider item, %d %d %d\n", weapon, nCond, rc_CTFPlayerShared_AddCond));
			if (weapon != nullptr) {
				GET_STRING_ATTRIBUTE(weapon, effect_add_attributes, attribs);
				
				if (attribs != nullptr) {
					std::string str(attribs);
					//Msg(CFmtStr("attribs, %s\n", attribs));
					boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

					auto it = tokens.begin();
					while (it != tokens.end()) {
						auto attribute = *it;
						if (++it == tokens.end())
							break;
						auto &value = *it;
						//Msg(CFmtStr("provide, %s %f %f\n", attribute.c_str(), strtof(value.c_str(),nullptr), flDuration));
						player->AddCustomAttribute(attribute.c_str(), value, flDuration);
						it++;
					}
				}
				GET_STRING_ATTRIBUTE(weapon, fire_input_on_effect, input);
				if (input != nullptr) {
					FireInputAttribute(input, nullptr, Variant((int)nCond), nullptr, pProvider, player, 0.0f);
				}
			}
			if (iCondOverride != 0) return;
        }
		DETOUR_MEMBER_CALL(nCond, flDuration, pProvider);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_RemoveCond, ETFCond nCond, bool bool1)
	{
		auto *shared = reinterpret_cast<CTFPlayerShared *>(this);

		if (rc_CTFPlayerShared_RemoveCond)
        {
			if (addcond_specific_cond != TF_COND_INVALID && addcond_specific_cond != nCond) return DETOUR_MEMBER_CALL(nCond, bool1);

			auto attribProvider = addcond_provider_item != nullptr ? addcond_provider_item : addcond_provider;
			CTFPlayer *player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
			int iCondOverride = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(attribProvider, iCondOverride, effect_cond_override);
			addcond_overridden = false;

			// Allow up to 4 addconds with bit shifting
			if (iCondOverride != 0) {
				for (int i = 0; i < 4; i++) {
					int addcond = (iCondOverride >> (i * 8)) & 255;
					if (addcond != 0) {
						nCond = (ETFCond) addcond;
						DETOUR_MEMBER_CALL(nCond, bool1);
						addcond_overridden = true;
					}
				}
			}

			auto *weapon = rtti_cast<CEconEntity *>(addcond_provider_item);
			//Msg("remove cond item, %d\n", weapon);
			if (weapon != nullptr) {
				GET_STRING_ATTRIBUTE(weapon, effect_add_attributes, attribs);
				
				if (attribs != nullptr) {
					std::string str(attribs);
					//Msg("attribs, %s\n", attribs);
					boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

					auto it = tokens.begin();
					while (it != tokens.end()) {
						auto attribute = *it;
						if (++it == tokens.end())
							break;
						auto &value = *it;
						//Msg("provide, %s %f\n", attribute.c_str());
						player->RemoveCustomAttribute(attribute.c_str());
						it++;
					}
				}
			}
			if (iCondOverride != 0) return;
        }
		DETOUR_MEMBER_CALL(nCond, bool1);
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayerShared_InCond, ETFCond nCond)
	{
		return false;
		// if (rc_CTFPlayerShared_InCond) {
		// 	if (rc_CTFPlayerShared_AddCondWatch && nCond != TF_COND_STEALTHED) return DETOUR_MEMBER_CALL(nCond);

		// 	auto attribProvider = addcond_provider_item != nullptr ? addcond_provider_item : addcond_provider;
		// 	int iCondOverride = 0;
		// 	CALL_ATTRIB_HOOK_INT_ON_OTHER(attribProvider, iCondOverride, effect_cond_override);

		// 	// Allow up to 4 addconds with bit shifting
		// 	if (iCondOverride != 0) {
		// 		for (int i = 0; i < 4; i++) {
		// 			int addcond = (iCondOverride >> (i * 8)) & 255;
		// 			if (addcond != 0) {
		// 				nCond = (ETFCond) addcond;
		// 				if (DETOUR_MEMBER_CALL(nCond)) return true;
		// 			}
		// 		}
		// 		return false;
		// 	}
		// }
		// return DETOUR_MEMBER_CALL(nCond);
	}

	RefCount rc_ReplaceCond;
	void ReplaceCond(CTFPlayerShared &shared, ETFCond cond) {
		rc_ReplaceCond.Increment();
		if (rc_ReplaceCond > 1) return;

		auto condData = shared.GetCondData();
		if (!condData.InCond(cond)) {
			auto attribProvider = addcond_provider_item != nullptr ? addcond_provider_item : addcond_provider;
			int iCondOverride = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(attribProvider, iCondOverride, effect_cond_override);

			// Allow up to 4 addconds with bit shifting
			if (iCondOverride != 0) {
				for (int i = 0; i < 4; i++) {
					int addcond = (iCondOverride >> (i * 8)) & 255;
					if (addcond != 0) {
						if (condData.InCond(addcond)) {
							condData.AddCondBit(cond);
							return;
						}
					}
				}
			}
		}
	}

	void ReplaceBackCond(CTFPlayerShared &shared, ETFCond cond) {
		rc_ReplaceCond.Decrement();
		if (rc_ReplaceCond > 0) return;
		auto condData = shared.GetCondData();
		if (condData.InCond(cond)) {
			auto attribProvider = addcond_provider_item != nullptr ? addcond_provider_item : addcond_provider;
			int iCondOverride = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(attribProvider, iCondOverride, effect_cond_override);

			// Allow up to 4 addconds with bit shifting
			if (iCondOverride != 0) {
				for (int i = 0; i < 4; i++) {
					int addcond = (iCondOverride >> (i * 8)) & 255;
					if (addcond != 0) {
						if (condData.InCond(addcond)) {
							condData.RemoveCondBit(cond);
							return;
						}
					}
				}
			}
		}
	}
	DETOUR_DECL_MEMBER(bool, CTFWeaponInvis_ActivateInvisibilityWatch)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		SCOPED_INCREMENT(rc_CTFPlayerShared_InCond);
		
		auto wep = reinterpret_cast<CTFWeaponInvis *>(this);
		addcond_provider = wep->GetTFPlayerOwner();
		addcond_provider_item = wep;
		
		int iCondOverride = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(wep, iCondOverride, effect_cond_override);

		ReplaceCond(wep->GetTFPlayerOwner()->m_Shared.Get(), TF_COND_STEALTHED);
		addcond_specific_cond = TF_COND_STEALTHED;
		auto result = DETOUR_MEMBER_CALL();
		addcond_specific_cond = TF_COND_INVALID;
		ReplaceBackCond(wep->GetTFPlayerOwner()->m_Shared.Get(), TF_COND_STEALTHED);
		if (result && iCondOverride != 0) {
			int mode = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(wep, mode, set_weapon_mode);
			if (mode != 1)
				wep->GetTFPlayerOwner()->SetOffHandWeapon(wep);

			wep->GetTFPlayerOwner()->m_Shared->m_bMotionCloak = mode == 2;
		}
		return result;
	}

	RefCount rc_CTFWeaponInvis_CleanupInvisibilityWatch;
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_FadeInvis, float mult)
	{
		auto me = reinterpret_cast<CTFPlayerShared *>(this);
		addcond_provider = me->GetOuter();
		if (!rc_CTFWeaponInvis_CleanupInvisibilityWatch)
			addcond_provider_item = GetEconEntityAtLoadoutSlot(me->GetOuter(), LOADOUT_POSITION_PDA2);
		
		bool shouldFadeOverride = addcond_provider_item != nullptr;

		SCOPED_INCREMENT_IF(rc_CTFPlayerShared_RemoveCond, shouldFadeOverride);
		SCOPED_INCREMENT_IF(rc_CTFPlayerShared_InCond, shouldFadeOverride);
		if (shouldFadeOverride) {
			int iCondOverride = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(addcond_provider_item, iCondOverride, effect_cond_override);
			if (iCondOverride) {
				me->GetOuter()->HolsterOffHandWeapon();
			}
			ReplaceCond(*me, TF_COND_STEALTHED);
			addcond_specific_cond = TF_COND_STEALTHED;
		}
		DETOUR_MEMBER_CALL(mult);
		if (shouldFadeOverride) {
			addcond_specific_cond = TF_COND_INVALID;
			ReplaceBackCond(*me, TF_COND_STEALTHED);
		}
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CTFPlayerShared_UpdateCloakMeter)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_RemoveCond);
		SCOPED_INCREMENT(rc_CTFPlayerShared_InCond);
		auto me = reinterpret_cast<CTFPlayerShared *>(this);
		bool isSpy = me->GetOuter()->IsPlayerClass(TF_CLASS_SPY);
		if (isSpy) {
			addcond_provider = me->GetOuter();
			addcond_provider_item = GetEconEntityAtLoadoutSlot(me->GetOuter(), LOADOUT_POSITION_PDA2);
			if (me->m_bMotionCloak && me->m_flCloakMeter <= 0 && addcond_provider_item != nullptr) {
				
				int iCondOverride = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(addcond_provider_item, iCondOverride, effect_cond_override);
				if (iCondOverride != 0)
					me->m_bMotionCloak = false;
			} 
			ReplaceCond(*me, TF_COND_STEALTHED);
		}
		addcond_specific_cond = TF_COND_STEALTHED;
		DETOUR_MEMBER_CALL();
		addcond_specific_cond = TF_COND_INVALID;
		if (isSpy) {
			ReplaceBackCond(*me, TF_COND_STEALTHED);
		}
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponInvis_CleanupInvisibilityWatch)
	{
		SCOPED_INCREMENT(rc_CTFWeaponInvis_CleanupInvisibilityWatch);
		auto wep = reinterpret_cast<CTFWeaponInvis *>(this);
		addcond_provider = wep->GetTFPlayerOwner();
		addcond_provider_item = wep;
		
		ReplaceCond(wep->GetTFPlayerOwner()->m_Shared.Get(), TF_COND_STEALTHED);
		DETOUR_MEMBER_CALL();
		ReplaceBackCond(wep->GetTFPlayerOwner()->m_Shared.Get(), TF_COND_STEALTHED);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_SpyDeadRingerDeath, const CTakeDamageInfo& info)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		SCOPED_INCREMENT(rc_CTFPlayerShared_InCond);
		auto me = reinterpret_cast<CTFPlayer *>(this);
		addcond_provider = me;
		addcond_provider_item = GetEconEntityAtLoadoutSlot(me, LOADOUT_POSITION_PDA2);
		ReplaceCond(me->m_Shared.Get(), TF_COND_STEALTHED);
		DETOUR_MEMBER_CALL(info);
		ReplaceBackCond(me->m_Shared.Get(), TF_COND_STEALTHED);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_PulseRageBuff, int rage)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		CTFPlayer *player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();;
		addcond_provider = player;
		int primaryHasRage = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_PRIMARY), primaryHasRage, set_buff_type); 
		addcond_provider_item = GetEconEntityAtLoadoutSlot(player, primaryHasRage != 0 ? LOADOUT_POSITION_PRIMARY : LOADOUT_POSITION_SECONDARY);

		DETOUR_MEMBER_CALL(rage);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_DoTauntAttack)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		
		auto player = reinterpret_cast<CTFPlayer *>(this);

		addcond_provider = player;
		addcond_provider_item = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_SECONDARY);
		
		int attackWhenInterrupted = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player->GetActiveTFWeapon(), attackWhenInterrupted, taunt_attack_after_end);
		bool removeTauntAfter = false;
		if (attackWhenInterrupted != 0) {
			removeTauntAfter = (player->m_Shared->m_nPlayerCond & TF_COND_TAUNTING) == 0;
			player->m_Shared->m_nPlayerCond |= 1 << TF_COND_TAUNTING;
		}

		DETOUR_MEMBER_CALL();

		if (removeTauntAfter) {
			player->m_Shared->m_nPlayerCond &= ~(1 << TF_COND_TAUNTING);
		}

		if (player->m_flTauntAttackTime > gpGlobals->curtime) {
			float attackDelayMult = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player->GetActiveTFWeapon(), attackDelayMult, taunt_attack_time_mult);
			if (attackDelayMult != 1.0f) {
				float attackDelay = player->m_flTauntAttackTime - gpGlobals->curtime;
				attackDelay *= attackDelayMult;
				player->m_flTauntAttackTime = gpGlobals->curtime + attackDelay;
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_ClearTauntAttack)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		int attackWhenInterrupted = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player->GetActiveTFWeapon(), attackWhenInterrupted, taunt_attack_after_end);
		if (attackWhenInterrupted != 0) {
			return;
		}

		DETOUR_MEMBER_CALL();
	}

	#define WEAPON_USE_DETOUR \
	SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond); \
	SCOPED_INCREMENT(rc_CTFPlayerShared_RemoveCond); \
	addcond_provider = reinterpret_cast<CBaseEntity *>(this)->GetOwnerEntity(); \
	addcond_provider_item = reinterpret_cast<CBaseEntity *>(this); \
	
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CTFPlayerShared_UpdateEnergyDrinkMeter)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		auto player = shared->GetOuter();
		addcond_provider = player;
		addcond_provider_item = rtti_cast<CTFSodaPopper *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_PRIMARY));
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond); \
		SCOPED_INCREMENT(rc_CTFPlayerShared_RemoveCond); \
		if (player->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT && shared->GetConditionProvider(TF_COND_SODAPOPPER_HYPE) != nullptr) {
			shared->m_flHypeMeter = Max(shared->m_flHypeMeter.Get(), 1.0f);
		}
		
		if (addcond_provider_item != nullptr) {
			ReplaceCond(*reinterpret_cast<CTFPlayerShared *>(this), TF_COND_SODAPOPPER_HYPE);
		}
		addcond_specific_cond = TF_COND_SODAPOPPER_HYPE;
		DETOUR_MEMBER_CALL();
		addcond_specific_cond = TF_COND_INVALID;
		if (addcond_provider_item != nullptr) {
			ReplaceBackCond(*reinterpret_cast<CTFPlayerShared *>(this), TF_COND_SODAPOPPER_HYPE);
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_SetScoutHypeMeter, float meter)
	{
		auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		addcond_provider = player;
		addcond_provider_item = rtti_cast<CTFSodaPopper *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_PRIMARY));
		SCOPED_INCREMENT_IF(rc_CTFPlayerShared_RemoveCond, addcond_provider_item != nullptr);
		if (addcond_provider_item != nullptr) {
			ReplaceCond(*reinterpret_cast<CTFPlayerShared *>(this), TF_COND_SODAPOPPER_HYPE);
		}
		DETOUR_MEMBER_CALL(meter);
		if (addcond_provider_item != nullptr) {
			ReplaceBackCond(*reinterpret_cast<CTFPlayerShared *>(this), TF_COND_SODAPOPPER_HYPE);
		}
	}

	DETOUR_DECL_MEMBER(void, CTFSodaPopper_SecondaryAttack)
	{
		WEAPON_USE_DETOUR
		auto wep = reinterpret_cast<CTFWeaponBase *>(this);
		ReplaceCond(wep->GetTFPlayerOwner()->m_Shared.Get(), TF_COND_SODAPOPPER_HYPE);
		DETOUR_MEMBER_CALL();
		ReplaceBackCond(wep->GetTFPlayerOwner()->m_Shared.Get(), TF_COND_SODAPOPPER_HYPE);
	}

	DETOUR_DECL_STATIC(void, JarExplode, int iEntIndex, CTFPlayer *pAttacker, CBaseEntity *pOriginalWeapon, CBaseEntity *pWeapon, const Vector& vContactPoint, int iTeam, float flRadius, ETFCond cond, float flDuration, const char *pszImpactEffect, const char *text2 )
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		addcond_provider = pAttacker;
		addcond_provider_item = pOriginalWeapon;
		
		CBaseCombatWeapon *econ_entity = ToBaseCombatWeapon(pOriginalWeapon);
		if (econ_entity != nullptr) {

			aoe_in_sphere_max_hit_count = GetFastAttributeInt(econ_entity, 0, MAX_AOE_TARGETS);
			aoe_in_sphere_hit_count = 0;

			GET_STRING_ATTRIBUTE(econ_entity, explosion_particle, particlename);
			if (particlename != nullptr) {
				pszImpactEffect = particlename;
			}
			
			GET_STRING_ATTRIBUTE(econ_entity, custom_impact_sound, sound);
			if (sound != nullptr) {
				PrecacheSound(sound);
				text2 = sound;
			}
		}

		DETOUR_STATIC_CALL(iEntIndex, pAttacker, pOriginalWeapon, pWeapon, vContactPoint, iTeam, flRadius, cond, flDuration, pszImpactEffect, text2);
		aoe_in_sphere_max_hit_count = 0;
	}
	DETOUR_DECL_MEMBER(void, CTFGasManager_OnCollide, CBaseEntity *entity, int id )
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		addcond_provider_item = addcond_provider = reinterpret_cast<CBaseEntity *>(this)->GetOwnerEntity();
		DETOUR_MEMBER_CALL(entity, id);
	}

	struct MedigunEffects_t
	{
		ETFCond eCondition;
		ETFCond eWearingOffCondition;
		const char *pszChargeOnSound;
		const char *pszChargeOffSound;
	};

	class ChargeOverrideModule : public EntityModule
	{
	public:
		ChargeOverrideModule(CBaseEntity *entity) {}

		int condOverride[6] = {-1, -1, -1, -1, -1, -1};
		EHANDLE condOverrideProvider[6];
		const char *condOverrideAttributes[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	};

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_SetChargeEffect, int iCharge, bool bState, bool bInstant, MedigunEffects_t& effects, float flWearOffTime, CTFPlayer *pProvider)
	{
		addcond_provider = pProvider;
		addcond_provider_item = pProvider != nullptr ? GetEconEntityAtLoadoutSlot(pProvider, LOADOUT_POSITION_SECONDARY) : nullptr;
		int iCondOverride = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(addcond_provider_item != nullptr ? addcond_provider_item : addcond_provider, iCondOverride, effect_cond_override);
		const char *effectAttributes = addcond_provider_item != nullptr ? ((CEconEntity *)addcond_provider_item)->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, addcond_provider_item, PStrT<"effect_add_attributes">()).ToCStr(): "";
		
		SCOPED_INCREMENT_IF(rc_CTFPlayerShared_AddCond, iCondOverride != 0 || effectAttributes[0] != '\0');
		SCOPED_INCREMENT_IF(rc_CTFPlayerShared_RemoveCond, iCondOverride != 0 || effectAttributes[0] != '\0');
		
		ETFCond old_cond = effects.eCondition;
		ETFCond old_wearing_cond = effects.eWearingOffCondition;
		auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		if (iCondOverride == 0 && effectAttributes[0] != '\0') {
			iCondOverride = old_cond;
		}
		if (bState && iCondOverride == 0) {
			auto mod = player->GetEntityModule<ChargeOverrideModule>("chargeoverride");
			if (mod != nullptr && iCharge >= 0 && iCharge < (int) ARRAY_SIZE(mod->condOverride)) {
				mod->condOverride[iCharge] = -1;
				mod->condOverrideAttributes[iCharge] = nullptr;
			}
		}
		int medigunKeepChargedEffect = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(addcond_provider_item != nullptr ? addcond_provider_item : addcond_provider, medigunKeepChargedEffect, medigun_keep_charged_effect);
		
		if (iCondOverride != 0) {
			auto mod = player->GetOrCreateEntityModule<ChargeOverrideModule>("chargeoverride");
			if (iCharge >= 0 && iCharge < (int) ARRAY_SIZE(mod->condOverride) && medigunKeepChargedEffect == 0) {
				mod->condOverride[iCharge] = bState ? iCondOverride : -1;
				mod->condOverrideProvider[iCharge] = addcond_provider_item != nullptr ? addcond_provider_item : addcond_provider;
				if (effectAttributes[0] != '\0')
					mod->condOverrideAttributes[iCharge] = effectAttributes;
			}
			effects.eCondition = (ETFCond) (iCondOverride & 255);
			effects.eWearingOffCondition = (ETFCond) GetNumberOfTFConds();
		}
		DETOUR_MEMBER_CALL(iCharge, bState, bInstant, effects, flWearOffTime, pProvider);
		if (iCondOverride != 0) {
			effects.eCondition = old_cond;
			effects.eWearingOffCondition = old_wearing_cond;
		}
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CTFPlayerShared_PulseMedicRadiusHeal)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		addcond_provider = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		addcond_provider_item = GetEconEntityAtLoadoutSlot(reinterpret_cast<CTFPlayerShared *>(this)->GetOuter(), LOADOUT_POSITION_MELEE);
		DETOUR_MEMBER_CALL();
	}

	GlobalThunk<MedigunEffects_t[]> g_MedigunEffects("g_MedigunEffects");
	

	void RemoveMedigunAttributes(CTFPlayer *target, const char *attribs)
	{
		std::string str(attribs);
		boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

		auto it = tokens.begin();
		while (it != tokens.end()) {
			auto attribute = *it;
			if (++it == tokens.end())
				break;

			auto attr_def = GetItemSchema()->GetAttributeDefinitionByName(attribute.c_str());
			if (attr_def != nullptr) {
				target->GetAttributeList()->RemoveAttribute(attr_def);
				target->TeamFortress_SetSpeed();
			}
			++it;
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_TestAndExpireChargeEffect, int type)
	{
		auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		auto mod = player->GetEntityModule<ChargeOverrideModule>("chargeoverride");
		auto &effect = g_MedigunEffects.GetRef()[type];
		ETFCond old_cond = effect.eCondition;
		ETFCond old_wearing_cond = effect.eWearingOffCondition;
		if (mod != nullptr && mod->condOverride[type] != -1) {
			effect.eCondition = (ETFCond) (mod->condOverride[type] & 255);
			effect.eWearingOffCondition = (ETFCond) GetNumberOfTFConds();
		}
		
		DETOUR_MEMBER_CALL(type);
		if (mod != nullptr && mod->condOverride[type] != -1) {
			auto weapon = rtti_cast<CWeaponMedigun *>(mod->condOverrideProvider[type].Get());
			auto condOverride = mod->condOverride[type];
			bool removeAttrs = false;
			for (int i = 0; i < 4; i++) {
				
				ETFCond addcond = (ETFCond) ((condOverride >> (i * 8)) & 255);
				//DevMsg("add cond post %d\n", addcond);
				if (addcond != 0) {
					if (player->m_Shared->InCond(addcond)) {
						if (weapon == nullptr || (weapon->GetHealTarget() != player && weapon->GetOwner() != player) || weapon->GetTFPlayerOwner()->GetActiveTFWeapon() != weapon || !weapon->IsChargeReleased()) {
							player->m_Shared->RemoveCond(addcond);
							removeAttrs = true;
						}
					}
					if (!player->m_Shared->InCond(addcond)) {
						mod->condOverride[type] = -1;
					}
				}
			}
			if (removeAttrs && mod->condOverrideAttributes[type] != nullptr)
				RemoveMedigunAttributes(player, mod->condOverrideAttributes[type]);
			effect.eCondition = old_cond;
			effect.eWearingOffCondition = old_wearing_cond;
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_SetRevengeCrits, int crits)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		SCOPED_INCREMENT(rc_CTFPlayerShared_RemoveCond);
		addcond_provider = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		addcond_provider_item = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter()->GetActiveWeapon();
		DETOUR_MEMBER_CALL(crits);
	}
	
	DETOUR_DECL_MEMBER(void, CTFWearableDemoShield_DoCharge, CTFPlayer *player)
	{
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		addcond_provider = player;
		addcond_provider_item = reinterpret_cast<CTFWearableDemoShield *>(this);
		addcond_overridden = false;
		DETOUR_MEMBER_CALL(player);
		if (addcond_overridden) {
			player->m_Shared->m_flChargeMeter = 0;
		}
	}
	
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CObjectSapper_ApplyRoboSapperEffects_Last, CTFPlayer *target, float duration) {
		SCOPED_INCREMENT(rc_CTFPlayerShared_AddCond);
		auto sapper = reinterpret_cast<CObjectSapper *>(this);
		addcond_provider = sapper->GetBuilder();
		addcond_provider_item = GetEconEntityAtLoadoutSlot(sapper->GetBuilder(), LOADOUT_POSITION_BUILDING);
		int iCondOverride = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(addcond_provider_item, iCondOverride, effect_cond_override);
		
		SCOPED_INCREMENT_IF(rc_stop_stun, iCondOverride != 0);
		DETOUR_MEMBER_CALL(target, duration);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFFlareGun_Revenge_Holster, CBaseCombatWeapon *weapon)
	{
		WEAPON_USE_DETOUR
		return DETOUR_MEMBER_CALL(weapon);
	}

	DETOUR_DECL_MEMBER(bool, CTFShotgun_Revenge_Holster, CBaseCombatWeapon *weapon)
	{
		WEAPON_USE_DETOUR
		return DETOUR_MEMBER_CALL(weapon);
	}

	DETOUR_DECL_MEMBER(bool, CTFRevolver_Holster, CBaseCombatWeapon *weapon)
	{
		WEAPON_USE_DETOUR
		return DETOUR_MEMBER_CALL(weapon);
	}

	DETOUR_DECL_MEMBER(bool, CTFFlareGun_Revenge_Deploy)
	{
		WEAPON_USE_DETOUR
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFShotgun_Revenge_Deploy)
	{
		WEAPON_USE_DETOUR
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFRevolver_Deploy)
	{
		WEAPON_USE_DETOUR
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFChargedSMG_SecondaryAttack)
	{
		WEAPON_USE_DETOUR
		return DETOUR_MEMBER_CALL();
	}

	RefCount rc_AllowOverheal;
	DETOUR_DECL_MEMBER(void, CTFPlayer_OnKilledOther_Effects, CBaseEntity *other, const CTakeDamageInfo& info)
	{
		CBaseEntity *ent = info.GetWeapon();
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		int overheal_allow = 0;
		if (info.GetWeapon() != nullptr) {
			// Allow Restore health on kill for wearables
			if (info.GetWeapon()->IsWearable()) {
				int iRestoreHealthToPercentageOnKill = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iRestoreHealthToPercentageOnKill, restore_health_on_kill );

				if ( iRestoreHealthToPercentageOnKill > 0 )
				{
					// This attribute should ignore runes
					int iRestoreMax = player->GetMaxHealth();
					// We add one here to deal with a bizarre problem that comes up leaving you one health short sometimes
					// due to bizarre floating point rounding or something equally silly.
					int iTargetHealth = ( int )( ( ( float )iRestoreHealthToPercentageOnKill / 100.0f ) * ( float )iRestoreMax ) + 1;

					int iBaseMaxHealth =  player->GetMaxHealth() * 1.5,
						iNewHealth = Min(  player->GetHealth() + iTargetHealth, iBaseMaxHealth ),
						iDeltaHealth = Max(iNewHealth -  player->GetHealth(), 0);

					 player->TakeHealth( iDeltaHealth, DMG_IGNORE_MAXHEALTH );
				}
			}
			std::vector<CTFPlayer *> condAllies {player};
			float radialCondRadiusSq = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(info.GetWeapon(), radialCondRadiusSq, radial_cond);
			radialCondRadiusSq *= radialCondRadiusSq;
			if (radialCondRadiusSq != 0) {
				ForEachTFPlayer([&](CTFPlayer *playerl) {
					if (playerl != player && playerl->GetTeamNumber() == player->GetTeamNumber() && radialCondRadiusSq > playerl->GetAbsOrigin().DistToSqr(player->GetAbsOrigin())) {
						condAllies.push_back(playerl);
					}
				});
			}

			int addcond_attr = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(info.GetWeapon(), addcond_attr, add_cond_on_kill);
			if (addcond_attr != 0) {
				float addcond_duration = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(info.GetWeapon(), addcond_duration, add_cond_on_kill_duration);
				if (addcond_duration == 0.0f) {
					addcond_duration = -1.0f;
				}
				for (int i = 0; i < 4; i++) {
					int addcond = (addcond_attr >> (i * 8)) & 255;
					if (addcond != 0) {
						for (auto ally : condAllies) ally->m_Shared->AddCond((ETFCond)addcond, addcond_duration, player);
					}
				}
			}
			CALL_ATTRIB_HOOK_INT_ON_OTHER(info.GetWeapon(), overheal_allow, overheal_from_heal_on_kill);
			
			auto weapon = rtti_cast<CEconEntity *>(info.GetWeapon());
			if (weapon != nullptr) {
				GET_STRING_ATTRIBUTE(weapon, add_attributes_on_kill, attributes_string);
				for (auto ally : condAllies) ApplyAttributesFromString(ally, attributes_string);
			}
		}

		SCOPED_INCREMENT_IF(rc_AllowOverheal, overheal_allow != 0);

		DETOUR_MEMBER_CALL(other, info);
	}

	DETOUR_DECL_MEMBER(float, CTFPlayer_TeamFortress_CalculateMaxSpeed, bool flag)
	{
		float ret = DETOUR_MEMBER_CALL(flag);

		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		if (player->HasTheFlag())
		{
			float value = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, value, mult_flag_carrier_move_speed);
			if (value != 1.0f) {
				ret *= value;
			}
		}
		
		float speedHp = GetFastAttributeFloat(player, 1.0f, MOVE_SPEED_AS_HEALTH_DECREASES);
		if (speedHp != 1.0f) {
			speedHp = RemapValClamped(player->GetHealth(), player->GetMaxHealth() * 0.15f, player->GetMaxHealth() * 0.85f, speedHp, 1.0f);
			ret *= speedHp;
		}

		return ret;
	}
	
	DETOUR_DECL_MEMBER(void, CTFSniperRifle_ExplosiveHeadShot, CTFPlayer *player1, CTFPlayer *player2)
	{
		aoe_in_sphere_max_hit_count = GetFastAttributeInt(reinterpret_cast<CBaseEntity *>(this), 0, MAX_AOE_TARGETS);
		aoe_in_sphere_hit_count = 0;
		DETOUR_MEMBER_CALL(player1, player2);
		aoe_in_sphere_max_hit_count = 0;
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_MakeBleed, CTFPlayer *attacker, CTFWeaponBase *weapon, float bleedTime, int bleeddmg, bool perm, int val)
	{
		auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		if (attacker == player) {
			int noSelfEffect = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( weapon != nullptr ? (CBaseEntity*)weapon : attacker, noSelfEffect, no_self_effect);
			if (noSelfEffect != 0) {
				return;
			}
		}
		if (aoe_in_sphere_max_hit_count != 0 && ++aoe_in_sphere_hit_count > aoe_in_sphere_max_hit_count) {
			return;
		}
		float multDmg = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon != nullptr ? (CBaseEntity*)weapon : attacker, multDmg, mult_bleeding_dmg);
		if (multDmg != 1.0f) {
			bleeddmg = (bleeddmg * multDmg);
		}
		DETOUR_MEMBER_CALL(attacker, weapon, bleedTime, bleeddmg, perm, val);
	}

	void RemoveOnActiveAttributes(CEconEntity *weapon, const char *attributes)
	{
		if (attributes != nullptr) {
			std::string str(attributes);
			boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

			auto it = tokens.begin();
			while (it != tokens.end()) {
				auto attribute = GetItemSchema()->GetAttributeDefinitionByName((*it).c_str());
				if (++it == tokens.end())
					break;
				auto value = *it;
				if (attribute != nullptr) {
					weapon->GetItem()->GetAttributeList().RemoveAttribute(attribute);
				}
				it++;
			}
		}
	}

	void AddOnActiveAttributes(CEconEntity *weapon, const char *attributes)
	{
		if (attributes != nullptr) {
			std::string str(attributes);
			boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

			auto it = tokens.begin();
			while (it != tokens.end()) {
				auto attribute = GetItemSchema()->GetAttributeDefinitionByName(it->c_str());
				if (++it == tokens.end())
					break;
				auto value = *it;
				
				if (attribute != nullptr) {
					weapon->GetItem()->GetAttributeList().AddStringAttribute(attribute, value);
				}
				it++;
			}
		}
	}

	class AltFireAttributesModule : public EntityModule
	{
	public:
		AltFireAttributesModule(CBaseEntity *entity) : EntityModule(entity) {
		}

		string_t attributes;
		bool active = false;
	};

	CBaseEntity *takeDamageAttacker = nullptr;
	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, CTakeDamageInfo &info)
	{
		//DevMsg("Take damage damage %f\n", info.GetDamage());
		takeDamageAttacker = info.GetAttacker();
		CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
		bool was_alive = entity->IsAlive();

		CTFWeaponBase *weaponAltFire = nullptr;
		if (info.GetInflictor() != nullptr && info.GetAttacker() != nullptr && info.GetInflictor()->GetCustomVariableBool<"isaltfire">()) {
			auto proj = rtti_cast<CBaseProjectile *>(info.GetInflictor());
			if (proj != nullptr && proj->GetOriginalLauncher() != nullptr) {
				auto weapon = rtti_cast<CTFWeaponBase *>(proj->GetOriginalLauncher());
				auto mod = proj->GetOriginalLauncher()->GetEntityModule<AltFireAttributesModule>("altfireattrs");
				if (weapon != nullptr && mod != nullptr && !mod->active) {
					weaponAltFire = weapon;
					AddOnActiveAttributes(weaponAltFire, STRING(mod->attributes));
					mod->active = true;
				}
			}
		}
		CTFWeaponBase *weaponProjAttrs = nullptr;
		if (info.GetInflictor() != nullptr && info.GetInflictor()->GetCustomVariable<"projattribs">() != nullptr) {
			auto proj = rtti_cast<CBaseProjectile *>(info.GetInflictor());
			if (proj != nullptr && proj->GetOriginalLauncher() != nullptr) {
				auto weapon = rtti_cast<CTFWeaponBase *>(proj->GetOriginalLauncher());
				if (weapon != nullptr && !weapon->GetCustomVariableBool<"projattribsapplied">()) {
					weaponProjAttrs = weapon;
					AddOnActiveAttributes(weaponProjAttrs, info.GetInflictor()->GetCustomVariable<"projattribs">());
					weapon->SetCustomVariable<"projattribsapplied">(Variant(true));
				}
			}
		}

		//auto weapon = ToBaseCombatWeapon(info.GetWeapon());
		auto econEntity = rtti_cast<CEconEntity *>(info.GetWeapon());
		
		if (econEntity != nullptr && info.GetAttacker() != nullptr && econEntity->GetItem() != nullptr) {
			info.SetDamageType(GetDamageType(econEntity, info.GetDamageType()));

			int iAddDamageType = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(econEntity, iAddDamageType, add_damage_type);
			info.AddDamageType(iAddDamageType);

			int iRemoveDamageType = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(econEntity, iRemoveDamageType, remove_damage_type);
			info.SetDamageType(info.GetDamageType() & ~(iRemoveDamageType));

			int customDmgType = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(econEntity, customDmgType, custom_damage_type_override);
			if (customDmgType != 0) {
				if (customDmgType == -1) customDmgType = 0;
				info.SetDamageCustom(customDmgType);
			}
		}
		// Use construction pda as sentry weapon
		auto sentry = ToBaseObject(info.GetInflictor());
		if (sentry == nullptr && info.GetInflictor() != nullptr) {
			sentry = ToBaseObject(info.GetInflictor()->GetOwnerEntity());
		}
		if (sentry != nullptr && info.GetWeapon() == nullptr && sentry->GetBuilder() != nullptr) {
			info.SetWeapon(sentry->GetBuilder()->GetEntityForLoadoutSlot(LOADOUT_POSITION_PDA));
		}

		int damage = DETOUR_MEMBER_CALL(info);

		CBaseProjectile *proj;
		if (entity->m_takedamage == DAMAGE_YES && entity->GetCustomVariableBool<"takesdamage">() && (proj = rtti_cast<CBaseProjectile *>(entity)) != nullptr) {
			int type = entity->GetCustomVariableInt<"takesdamagetype">();
			CBaseEntity *thrower = proj->GetOwnerEntity();
			CBaseGrenade *grenade = rtti_cast<CBaseGrenade *>(proj);
			if (grenade != nullptr) {
				thrower = grenade->GetThrower();
			}
			if (thrower == nullptr) thrower = proj;

			if (type == 0 || (type == 1 && info.GetAttacker() == thrower) || (type == 2 && info.GetAttacker() != nullptr && info.GetAttacker()->GetTeamNumber() == thrower->GetTeamNumber()) 
					|| (type == 3 && (info.GetAttacker() == nullptr || info.GetAttacker()->GetTeamNumber() != thrower->GetTeamNumber()))) {
				proj->SetHealth(entity->GetHealth() - info.GetDamage());

				if (proj->GetHealth() <= 0) {
					if (proj->IsDestroyable(false)) {
						proj->Destroy();
					}
					else {
						proj->Remove();
					}
				}
				damage = info.GetDamage();
			}
		}

		//Fire input on hit
		if (econEntity != nullptr && info.GetAttacker() != nullptr && econEntity->GetItem() != nullptr) {
			if (entity->IsPlayer() && entity->GetTeamNumber() == info.GetAttacker()->GetTeamNumber()) {
				GET_STRING_ATTRIBUTE(econEntity, fire_input_on_hit_ally, input_ally);
				FireInputAttribute(input_ally, nullptr, Variant(damage), info.GetInflictor(), info.GetAttacker(), entity, 0.0f);
			}
			{
				GET_STRING_ATTRIBUTE(econEntity, fire_input_on_hit, input);
				GET_STRING_ATTRIBUTE(econEntity, fire_input_on_hit_name_restrict, filter);
				
				FireInputAttribute(input, filter, Variant(damage), info.GetInflictor(), info.GetAttacker(), entity, 0.0f);
			}

			if (was_alive && !entity->IsAlive()) {
				GET_STRING_ATTRIBUTE(econEntity, fire_input_on_kill, input);
				GET_STRING_ATTRIBUTE(econEntity, fire_input_on_kill_name_restrict, filter);

				FireInputAttribute(input, filter, Variant(damage), info.GetInflictor(), info.GetAttacker(), entity, 0.0f);
			}
			
		}

		if (weaponAltFire != nullptr) {
			auto mod = weaponAltFire->GetEntityModule<AltFireAttributesModule>("altfireattrs");
			if (mod != nullptr) {
				RemoveOnActiveAttributes(weaponAltFire, STRING(mod->attributes));
				mod->active = false;
			}
		}
		if (weaponProjAttrs != nullptr) {
			RemoveOnActiveAttributes(weaponProjAttrs, info.GetInflictor()->GetCustomVariable<"projattribs">());
			weaponProjAttrs->SetCustomVariable<"projattribsapplied">(Variant(false));
		}
		takeDamageAttacker = nullptr;
		return damage;
	}
	
	DETOUR_DECL_MEMBER(void, CTFGameRules_DeathNotice, CBasePlayer *pVictim, const CTakeDamageInfo &info, const char* eventName)
	{
		// Restore sentry damage weapon to null if it was previously set to pda, for correct killstreak counting
		CTakeDamageInfo infoc = info;
		auto sentry = ToBaseObject(info.GetInflictor());
		if (sentry == nullptr && info.GetInflictor() != nullptr) {
			sentry = ToBaseObject(info.GetInflictor()->GetOwnerEntity());
		}
		if (sentry != nullptr && sentry->GetBuilder() != nullptr && info.GetWeapon() == sentry->GetBuilder()->GetEntityForLoadoutSlot(LOADOUT_POSITION_PDA)) {
			infoc.SetWeapon(nullptr);
		}
		DETOUR_MEMBER_CALL(pVictim, infoc, eventName);
	}

	DETOUR_DECL_MEMBER(int, CTFPlayerShared_GetMaxBuffedHealth, bool flag1, bool flag2)
	{
		static ConVarRef tf_max_health_boost("tf_max_health_boost");
		float old_value = tf_max_health_boost.GetFloat();
		float value = old_value;
		value *= GetFastAttributeFloat(reinterpret_cast<CTFPlayerShared *>(this)->GetOuter(), 1.0f, MULT_MAX_OVERHEAL_SELF);
		
		auto ret = DETOUR_MEMBER_CALL(flag1, flag2);

		if (value != old_value)
			tf_max_health_boost.SetValue(old_value);
		return ret;
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_TakeHealth, float flHealth, int bitsDamageType)
	{
		if (rc_AllowOverheal > 0) {
			bitsDamageType |= DMG_IGNORE_MAXHEALTH;
		}
		
		return DETOUR_MEMBER_CALL(flHealth, bitsDamageType);
	}

	CObjectDispenser *dispenser_provider = nullptr;
	DETOUR_DECL_MEMBER(bool, CObjectDispenser_DispenseAmmo, CTFPlayer *player)
	{
		dispenser_provider = reinterpret_cast<CObjectDispenser *>(this);
		return DETOUR_MEMBER_CALL(player);
		dispenser_provider = nullptr;
	}

	DETOUR_DECL_MEMBER(int, CObjectDispenser_DispenseMetal, CTFPlayer *player)
	{
		dispenser_provider = reinterpret_cast<CObjectDispenser *>(this);
		auto ret = DETOUR_MEMBER_CALL(player);
		dispenser_provider = nullptr;
		return ret;
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_GiveAmmo, int amount, int type, bool sound, int source)
	{
		if (dispenser_provider != nullptr && type == TF_AMMO_METAL) {
			float mult = GetBuildingAttributeFloat<"ratemult">(dispenser_provider, "mult_dispenser_rate", false);
			amount = amount * mult;
		}
		if (dispenser_provider != nullptr && type != TF_AMMO_METAL) {
			amount *= GetBuildingAttributeFloat<"ratemult">(dispenser_provider, "mult_dispenser_rate", true);
		}
		return DETOUR_MEMBER_CALL(amount, type, sound, source);
	}
	
	struct StickInfo
	{
		CHandle<CBaseEntity> sticky;
		CHandle<CBaseEntity> sticked;
		Vector offset;
	};
	std::vector<StickInfo> stick_info;
	DETOUR_DECL_MEMBER(void, CTFGrenadePipebombProjectile_StickybombTouch, CBaseEntity *other)
	{
		auto proj = reinterpret_cast<CTFGrenadePipebombProjectile *>(this);

		if (!proj->m_bTouched && other->MyCombatCharacterPointer() != nullptr && other->GetTeamNumber() != proj->GetTeamNumber()) {
			int stick = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(proj->GetOriginalLauncher(), stick, stickbomb_stick_to_enemies);

			if (stick != 0) {
				proj->m_bTouched = true;
				
				auto phys = proj->VPhysicsGetObject();
				if (phys != nullptr) {
					phys->EnableMotion(false);
				}

				if (other->IsPlayer()) {
					stick_info.push_back({proj, other, proj->GetAbsOrigin() - other->GetAbsOrigin()});
				}
				else {
					proj->SetParent(other, -1);
				}
			}
			float flFizzle = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(proj->GetOriginalLauncher(), flFizzle, stickybomb_fizzle_time);
			if (flFizzle > 0) {
				proj->SetDetonateTimerLength(flFizzle);
			}
			
		}
		DETOUR_MEMBER_CALL(other);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_DropCurrencyPack, int pack, int amount, bool forcedistribute, CTFPlayer *moneymaker )
	{
		if (killer_weapon != nullptr) {
			int distribute = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(killer_weapon, distribute, collect_currency_on_kill);
			if (distribute != 0) {
				forcedistribute = true;
				moneymaker = ToTFPlayer(killer_weapon->GetOwnerEntity());
			}
		}
		DETOUR_MEMBER_CALL(pack, amount, forcedistribute, moneymaker);

	}
	
	std::map<CSteamID, float> respawnTimesForId;
	DETOUR_DECL_MEMBER(float, CTeamplayRoundBasedRules_GetMinTimeWhenPlayerMaySpawn, CBasePlayer *player)
	{
		if (!(player->IsBot() && TFGameRules()->IsMannVsMachineMode()) && !(TFGameRules()->IsMannVsMachineMode() && TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS)) {
			float respawntime = GetFastAttributeFloat(player, 0.0f, MIN_RESPAWN_TIME);
			if (respawntime != 0) {
				return player->GetDeathTime() + respawntime;
			}
			auto find = respawnTimesForId.find(player->GetSteamID());
			if (find != respawnTimesForId.end() && find->second > gpGlobals->curtime) {
				Msg("time now this\n");
				return find->second;
			}
		}

		return DETOUR_MEMBER_CALL(player);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ShouldGainInstantSpawn)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (!(player->IsBot() && TFGameRules()->IsMannVsMachineMode()) && !(TFGameRules()->IsMannVsMachineMode() && TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS)) {
			
			auto find = respawnTimesForId.find(player->GetSteamID());
			if (find != respawnTimesForId.end() && find->second > gpGlobals->curtime) {
				Msg("time now this\n");
				return false;
			}
		}
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsReadyToSpawn)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (!(player->IsBot() && TFGameRules()->IsMannVsMachineMode()) && !(TFGameRules()->IsMannVsMachineMode() && TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS)) {
			
			auto find = respawnTimesForId.find(player->GetSteamID());
			if (find != respawnTimesForId.end() && find->second > gpGlobals->curtime) {
				Msg("time now this\n");
				return false;
			}
		}
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, CTFGameRules_OnPlayerSpawned, CTFPlayer *player)
	{
		DETOUR_MEMBER_CALL(player);
		for (size_t i = 0; i < stick_info.size(); ) {
			auto &entry = stick_info[i];
			if (entry.sticked == player && entry.sticky != nullptr) {
				stick_info.erase(stick_info.begin() + i);
				auto phys = entry.sticky->VPhysicsGetObject();
				if (phys != nullptr) {
					phys->EnableMotion(true);
				}
				continue;
			}
			i++;
		}
		int isMiniboss = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, isMiniboss, is_miniboss);
		if (isMiniboss != 0)
			player->SetMiniBoss(isMiniboss);

		float playerScale = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, playerScale, model_scale);

		if (playerScale != 1.0f)
			player->SetModelScale(playerScale);
		else if (isMiniboss != 0) {
			static ConVarRef miniboss_scale("tf_mvm_miniboss_scale");
			player->SetModelScale(miniboss_scale.GetFloat());
		}
		
		float noclip = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, noclip, no_clip);
		if (noclip != 0.0f) {
			player->SetMoveType(MOVETYPE_NOCLIP);
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFMinigun_WindDown)
	{
		auto minigun = reinterpret_cast<CTFMinigun *>(this);
		if (minigun->GetItem() != nullptr) {
			GET_STRING_ATTRIBUTE(minigun, custom_wind_down_sound, str);
			if (str != nullptr) {
				PrecacheSound(str);
				minigun->EmitSound(str);
			}
		}
        DETOUR_MEMBER_CALL();
    }
	
	DETOUR_DECL_MEMBER(void, CTFMinigun_WindUp)
	{
		auto minigun = reinterpret_cast<CTFMinigun *>(this);
		if (minigun->GetItem() != nullptr) {
			GET_STRING_ATTRIBUTE(minigun, custom_wind_down_sound, str);
			if (str != nullptr) {
				PrecacheSound(str);
				minigun->EmitSound(str);
			}
		}
        DETOUR_MEMBER_CALL();
    }

	DETOUR_DECL_MEMBER(void, CTFPlayer_DropAmmoPack, const CTakeDamageInfo& info, bool b1, bool b2)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		for(int i = 0; i < player->WeaponCount(); i++ ) {
			CBaseCombatWeapon *weapon = player->GetWeapon(i);
			if (weapon == nullptr || weapon == player->GetActiveTFWeapon() || weapon->GetItem() == nullptr) continue;

			int droppedWeapon = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, droppedWeapon, is_dropped_weapon);
			
			if (droppedWeapon != 0) {
				auto dropped = CTFDroppedWeapon::Create(player, player->EyePosition(), vec3_angle, weapon->GetWorldModel(), weapon->GetItem());
				if (dropped != nullptr)
					dropped->InitDroppedWeapon(player, static_cast<CTFWeaponBase *>(weapon), info.GetAttacker() != nullptr && info.GetAttacker()->GetTeamNumber() == player->GetTeamNumber(), false);
			}
		}
		DETOUR_MEMBER_CALL(info, b1, b2);
	}

	DETOUR_DECL_STATIC(CTFDroppedWeapon *, CTFDroppedWeapon_Create, CTFPlayer *pOwner, const Vector& vecOrigin, const QAngle& vecAngles, const char *pszModelName, const CEconItemView *pItemView)
	{
		if (pItemView != nullptr) {
			CAttributeList &list = pItemView->GetAttributeList();
			GET_STRING_ATTRIBUTE_LIST(list, "custom item model", model);
			if (model != nullptr) {
				pszModelName = model;
			}
		}
		auto wep = DETOUR_STATIC_CALL(pOwner, vecOrigin, vecAngles, pszModelName, pItemView);
		if (wep != nullptr && StringStartsWith(pItemView->GetItemDefinition()->GetItemClass(""), "tf_weapon_medigun")) {
			wep->m_nBody = 1;
		}
		return wep;
	}
	
	RefCount rc_CTFPlayer_Regenerate;
	DETOUR_DECL_MEMBER(void, CTFPlayer_Regenerate, bool ammo)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_Regenerate);
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		
		int noRegenerate = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, noRegenerate, no_resupply);
		if (!noRegenerate) {
			DETOUR_MEMBER_CALL(ammo);
		}
	}
	bool CTFPlayer_ItemsMatch_Func(bool ret, CTFPlayer *player, CEconItemView *pCurWeaponItem, CEconItemView *pNewWeaponItem, CTFWeaponBase *pWpnEntity) {
		//if (pCurWeaponItem != nullptr && pNewWeaponItem != nullptr) {
		//	DevMsg("itemsmatch %lld %lld %d %s %s %d %d\n", pCurWeaponItem->m_iItemID + 0LL, pNewWeaponItem->m_iItemID + 0LL, ret, GetItemNameForDisplay(pCurWeaponItem), GetItemNameForDisplay(pNewWeaponItem), pCurWeaponItem->m_iItemDefinitionIndex.Get(), pNewWeaponItem->m_iItemDefinitionIndex.Get());
		//}
		if (!ret && rc_CTFPlayer_Regenerate) {
			if (pWpnEntity != nullptr) {
				int stay = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(pWpnEntity, stay, stay_after_regenerate);
				if (stay != 0) {
					return true;
				} 
			}
			// For cosmetics
			else if (pCurWeaponItem != nullptr) {
				auto attr = pCurWeaponItem->GetAttributeList().GetAttributeByName("stay after regenerate");
				if (attr != nullptr && attr->GetValue().m_Float != 0) {
					return true;
				}
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, bool, CTFPlayer_ItemsMatch, CEconItemView *pCurWeaponItem, CEconItemView *pNewWeaponItem, CTFWeaponBase *pWpnEntity)
	{
		return CTFPlayer_ItemsMatch_Func(DETOUR_MEMBER_CALL(pCurWeaponItem, pNewWeaponItem, pWpnEntity), reinterpret_cast<CTFPlayer *>(this), pCurWeaponItem, pNewWeaponItem, pWpnEntity);
	}

	THINK_FUNC_DECL(MinigunClearSounds)
	{
		auto minigun = reinterpret_cast<CTFMinigun *>(this);
		minigun->StopSound(minigun->GetShootSound(WPN_DOUBLE));
		minigun->StopSound(minigun->GetShootSound(BURST));
		minigun->StopSound(minigun->GetShootSound(SPECIAL3));
		
	}

	THINK_FUNC_DECL(FlameThrowerClearSounds)
	{
		auto flamethrower = reinterpret_cast<CTFFlameThrower *>(this);
		flamethrower->StopSound(flamethrower->GetShootSound(SINGLE));
		flamethrower->StopSound(flamethrower->GetShootSound(BURST));
		flamethrower->StopSound(flamethrower->GetShootSound(SPECIAL1));
		
	}

	DETOUR_DECL_MEMBER(void, CTFMinigun_SetWeaponState, CTFMinigun::MinigunState_t state)
	{
		auto minigun = reinterpret_cast<CTFMinigun *>(this);
		if (state != minigun->m_iWeaponState) {
			GET_STRING_ATTRIBUTE(minigun, custom_weapon_fire_sound, soundfiring);
			GET_STRING_ATTRIBUTE(minigun, custom_minigun_spin_sound, soundspinning);

			if (soundfiring != nullptr) {
				if (state == CTFMinigun::AC_STATE_FIRING) {
					minigun->EmitSound(soundfiring);
					THINK_FUNC_SET(minigun, MinigunClearSounds, gpGlobals->curtime);
				}
				else {
					minigun->StopSound(soundfiring);
				}
			}
			if (soundspinning != nullptr) {
				if (state == CTFMinigun::AC_STATE_SPINNING) {
					minigun->EmitSound(soundspinning);
					THINK_FUNC_SET(minigun, MinigunClearSounds, gpGlobals->curtime);
				}
				else {
					minigun->StopSound(soundspinning);
				}
			}
		}
		DETOUR_MEMBER_CALL(state);
	}
	
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CTFFlameThrower_SetWeaponState, int state)
	{
		auto flamethrower = reinterpret_cast<CTFFlameThrower *>(this);
		if (state != flamethrower->m_iWeaponState) {
			GET_STRING_ATTRIBUTE(flamethrower, custom_weapon_fire_sound, soundfiring);

			if (soundfiring != nullptr) {
				if ((state == 1 || state == 2) && flamethrower->m_iWeaponState != 1 && flamethrower->m_iWeaponState != 2) {
					flamethrower->EmitSound(soundfiring);
				}
				else if (state != 1 && state != 2) {
					flamethrower->StopSound(soundfiring);
				}
				THINK_FUNC_SET(flamethrower, FlameThrowerClearSounds, gpGlobals->curtime);
			}
		}
		DETOUR_MEMBER_CALL(state);
	}

	
	DETOUR_DECL_MEMBER(void, CTFProjectile_Arrow_FadeOut, int time)
	{
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		DETOUR_MEMBER_CALL(time);
		float remove = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(arrow->GetOriginalLauncher(), remove, arrow_hit_kill_time);
		if (remove != 0) {
			arrow->SetNextThink(gpGlobals->curtime + remove, "ARROW_REMOVE_THINK");
		}

	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_Arrow_CheckSkyboxImpact, CBaseEntity *pOther)
	{
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		float bounce_speed = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(arrow->GetOriginalLauncher(), bounce_speed, grenade_bounce_speed);
		if (bounce_speed != 0) {
			if (BounceArrow(arrow, bounce_speed)) {
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(pOther);
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_Arrow_BreakArrow)
	{
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		float bounce_speed = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(arrow->GetOriginalLauncher(), bounce_speed, grenade_bounce_speed);
		if (bounce_speed != 0) return;

		float target_bounce_speed = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(arrow->GetOriginalLauncher(), target_bounce_speed, arrow_target_bounce_speed);
		if (target_bounce_speed != 0) return;

		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(int, CTFPlayerShared_CalculateObjectCost, CTFPlayer *builder, int object)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);

		int result = DETOUR_MEMBER_CALL(builder, object);

		if (object == OBJ_SENTRYGUN) {
			float sentry_cost = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(builder, sentry_cost, mod_sentry_cost);
			result *= sentry_cost;
			if (sentry_cost > 1.0f && result > builder->GetAmmoCount( TF_AMMO_METAL )) {
				gamehelpers->TextMsg(ENTINDEX(builder), TEXTMSG_DEST_CENTER, TranslateText(builder, "You need metal to build a sentry gun", 1, &result));
			}
		}
		else if (object == OBJ_DISPENSER) {
			float dispenser_cost = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(builder, dispenser_cost, mod_dispenser_cost);
			result *= dispenser_cost;
			if (dispenser_cost > 1.0f && result > builder->GetAmmoCount( TF_AMMO_METAL )) {
				gamehelpers->TextMsg(ENTINDEX(builder), TEXTMSG_DEST_CENTER, TranslateText(builder, "You need metal to build a dispenser", 1, &result));
			}
		}
		return result;
	}

	DETOUR_DECL_MEMBER(ETFDmgCustom, CTFWeaponBase_GetPenetrateType)
	{
		auto result = DETOUR_MEMBER_CALL();

		if (result == TF_DMG_CUSTOM_NONE) {
			int penetrate = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(reinterpret_cast<CTFWeaponBase *>(this), penetrate, penetrate_teammates);
			if (penetrate != 0)
				return TF_DMG_CUSTOM_PENETRATE_MY_TEAM;
		}
		return result;
	}

	DETOUR_DECL_MEMBER(float, CBaseProjectile_GetCollideWithTeammatesDelay)
	{
		int penetrate = 0;
		auto launcher = reinterpret_cast<CBaseProjectile *>(this)->GetOriginalLauncher();
		CALL_ATTRIB_HOOK_INT_ON_OTHER(launcher, penetrate, penetrate_teammates);
		if (penetrate) {
			return 9999.0f;
		}

		int friendlyfire = HasAllowFriendlyFire(launcher, reinterpret_cast<CBaseProjectile *>(this)->GetOwnerEntity());
		if (friendlyfire) {
			return 0.0f;
		}

		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_TFPlayerThink)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_TFPlayerThink);
		if (gpGlobals->tickcount % 8 == 6) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			static ConVarRef stepsize("sv_stepsize");
			player->m_Local->m_flStepSize = GetFastAttributeFloat(player, stepsize.GetFloat(), MULT_STEP_HEIGHT);
		}

		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(unsigned int, CTFGameMovement_PlayerSolidMask, bool brushonly)
	{
		CBasePlayer *player = reinterpret_cast<CGameMovement *>(this)->player;
		unsigned int mask = DETOUR_MEMBER_CALL(brushonly);
		if (GetFastAttributeInt(player, 0, IGNORE_PLAYER_CLIP) != 0)
			mask &= ~CONTENTS_PLAYERCLIP;

		if (GetFastAttributeInt(player, 0, NOT_SOLID) != 0)
			mask = 0;

		return mask;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_PlayerRunCommand, CUserCmd* cmd, IMoveHelper* moveHelper)
	{
		CTFPlayer* player = reinterpret_cast<CTFPlayer*>(this);
		if( (cmd->buttons & IN_JUMP) && (player->GetGroundEntity() == nullptr) && player->IsAlive() 
				/*&& (player->GetFlags() & 1) */ && GetFastAttributeInt(player, 0, ALLOW_BUNNY_HOP) == 1
				 ){
			cmd->buttons &= ~IN_JUMP;
		}
		DETOUR_MEMBER_CALL(cmd, moveHelper);
	}

	DETOUR_DECL_MEMBER(void, CTFGameMovement_PreventBunnyJumping)
	{
		if(GetFastAttributeInt(reinterpret_cast<CGameMovement *>(this)->player, 0, ALLOW_BUNNY_HOP) == 0){
			DETOUR_MEMBER_CALL();
		}
	}

	RefCount rc_CTFWeaponBase_Reload;
	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_Reload)
	{
		SCOPED_INCREMENT(rc_CTFWeaponBase_Reload);
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		int iWeaponMod = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, iWeaponMod, reload_full_clip_at_once );
		if (iWeaponMod == 1 && !weapon->IsEnergyWeapon())
		{
			weapon->m_bReloadsSingly = false;
		}

		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_Holster)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);

		// Remove add cond on active effects
		CTFPlayer *owner = ToTFPlayer(weapon->GetOwnerEntity());
		if (owner != nullptr) {
			int alwaysCrit = GetFastAttributeInt(weapon, 0, ALWAYS_CRIT);

			if (alwaysCrit && owner->m_Shared->GetConditionDuration(TF_COND_CRITBOOSTED_USER_BUFF) < 0.5f) {
				owner->m_Shared->RemoveCond(TF_COND_CRITBOOSTED_USER_BUFF);
			}
			
			int addcond = GetFastAttributeInt(weapon, 0, ADD_COND_ON_ACTIVE);
			if (addcond != 0) {
				for (int i = 0; i < 3; i++) {
					int addcond_single = (addcond >> (i * 8)) & 255;
					if (addcond_single != 0) {
						if (alwaysCrit && owner->m_Shared->GetConditionDuration((ETFCond)addcond_single) < 0.5f) {
							owner->m_Shared->RemoveCond((ETFCond)addcond_single);
						}
					}
				}
			}
			
			GET_STRING_ATTRIBUTE(weapon, add_attributes_when_active, attributes);
			RemoveOnActiveAttributes(weapon, attributes);
		}
		weapon->GetOrCreateEntityModule<WeaponModule>("weapon")->consecutiveShotsScore = 0.0f;
		auto result = DETOUR_MEMBER_CALL();
		if (GetFastAttributeInt(weapon, 0, PASSIVE_RELOAD) != 0) {
			weapon->m_bInReload = true;
		}
		
		GET_STRING_ATTRIBUTE(weapon, alt_fire_attributes, altattribs);
		if (altattribs != nullptr) {
			RemoveOnActiveAttributes(weapon, altattribs);
			weapon->GetOrCreateEntityModule<AltFireAttributesModule>("altfireattrs")->active = false;
		}

		if (weapon->GetCustomVariableBool<"noattackreset">()) {
			static int noAttackId = GetItemSchema()->GetAttributeDefinitionByName("no_attack")->GetIndex();
			weapon->SetCustomVariable("noattackreset", Variant(false));
			weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(noAttackId, 0);
		}
		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ItemHolsterFrame)
	{
		DETOUR_MEMBER_CALL();
		
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		if (weapon->GetMaxClip1() != -1 && GetFastAttributeInt(weapon, 0, PASSIVE_RELOAD) != 0) {
			weapon->CheckReload();
		}
	}
	
	DETOUR_DECL_MEMBER(void, CBaseProjectile_D2)
	{
		auto projectile = reinterpret_cast<CBaseProjectile *>(this);
		auto weapon = ToBaseCombatWeapon(projectile->GetOriginalLauncher());
		if (weapon != nullptr) {
			GET_STRING_ATTRIBUTE(weapon, projectile_sound, sound);
			if (sound != nullptr) {
				projectile->StopSound(sound);
			}
		}
        DETOUR_MEMBER_CALL();
    }

	std::map<CHandle<CTFWeaponBaseGun>, float> applyGunDelay;
	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_PrimaryAttack)
	{
		auto weapon = reinterpret_cast<CTFWeaponBaseGun *>(this);
		auto mod = weapon->GetOrCreateEntityModule<WeaponModule>("weapon");

		auto oldHost = g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost;
		// Burst mode 

		//if (mod->lastAttackCooldown != 0.0f && gpGlobals->curtime < mod->lastShotTime + mod->lastAttackCooldown + gpGlobals->frametime) {
		//	g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost = nullptr;
		//}

		DETOUR_MEMBER_CALL();
		if (weapon->m_flNextPrimaryAttack > gpGlobals->curtime) {
			float attackTime = weapon->m_flNextPrimaryAttack - gpGlobals->curtime;
			mod->consecutiveShotsScore += (weapon->m_flNextPrimaryAttack - gpGlobals->curtime);
			
			static int auto_fires_full_clip_id = GetItemSchema()->GetAttributeDefinitionByName("auto fires full clip")->GetIndex();
			int burst_num = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, burst_num, burst_fire_count);
			if (burst_num != 0) {
				bool enforced = burst_num < 0;
				int burst_num_real = abs(burst_num) - 1;
				mod->lastAttackCooldown = attackTime;
				if (gpGlobals->curtime > mod->lastShotTime + attackTime + gpGlobals->frametime ) {
					mod->burstShotNumber = 0;
				}
				mod->lastShotTime = gpGlobals->curtime;

				if (mod->burstShotNumber < burst_num_real) {
					if (enforced && mod->burstShotNumber == 0) {
						weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(auto_fires_full_clip_id, 1.0f);
					}
					float ping = 0;
					auto netinfo = engine->GetPlayerNetInfo(ENTINDEX(weapon->GetTFPlayerOwner()));
					if (netinfo != nullptr) {
						ping = netinfo->GetLatency(FLOW_OUTGOING) - 0.03f;
					}
					int shotsinping = ping / attackTime;
					if (mod->burstShotNumber >= burst_num_real - shotsinping) {
						applyGunDelay[weapon] = gpGlobals->curtime + attackTime;
					} 
					mod->burstShotNumber += 1;
				}
				else {
					mod->burstShotNumber = 0;
					float burst_fire_rate = 1.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, burst_fire_rate, burst_fire_rate_mult);
					weapon->m_flNextPrimaryAttack = gpGlobals->curtime + attackTime * burst_fire_rate;
					if (enforced) {
						weapon->GetItem()->GetAttributeList().RemoveAttributeByDefID(auto_fires_full_clip_id);
					}
				}
				if (enforced && (weapon->m_iClip1 == 0 || weapon->m_flEnergy < weapon->Energy_GetShotCost())) {
					weapon->GetItem()->GetAttributeList().RemoveAttributeByDefID(auto_fires_full_clip_id);
				}
			}
			
			int fire_full_clip = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, fire_full_clip, force_fire_full_clip);
			if (fire_full_clip != 0) {
				if (weapon->m_iClip1 != 0 && weapon->m_flEnergy >= weapon->Energy_GetShotCost()) {
					weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(auto_fires_full_clip_id, 1.0f);
				}
				else {
					weapon->GetItem()->GetAttributeList().RemoveAttributeByDefID(auto_fires_full_clip_id);
				}
			}
			
			auto spreadMod = weapon->GetEntityModule<AttackPatternModule>("attackpattern");
			if (spreadMod != nullptr) {
				float patternResetTime = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, patternResetTime, shoot_pattern_reset_time);
				spreadMod->resetPatternTime = gpGlobals->curtime + (patternResetTime != 0 ? patternResetTime : attackTime + gpGlobals->interval_per_tick);
			}
		}
		g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost = oldHost;

	}

	DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount,  void **clients, void *snapshot)
	{
		if (!applyGunDelay.empty()) {
			for (auto it = applyGunDelay.begin(); it != applyGunDelay.end();) {
				if (it->first == nullptr || it->second < gpGlobals->curtime) {
					it = applyGunDelay.erase(it);
					continue;
				}
				it->first->GetOrCreateEntityModule<WeaponModule>("weapon")->attackTimeSave = it->first->m_flNextPrimaryAttack;
				it->first->m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;
				it++;
			}
		}
		DETOUR_STATIC_CALL(clientCount, clients, snapshot);
		if (!applyGunDelay.empty()) {
			for (auto it = applyGunDelay.begin(); it != applyGunDelay.end(); it++) {
				it->first->m_flNextPrimaryAttack = it->first->GetOrCreateEntityModule<WeaponModule>("weapon")->attackTimeSave;
			}
		} 
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_Spawn)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this); 
		
		// // Reapply attributes from items that stay after respawn
		// std::unordered_set<CEconEntity *> preSpawnItems;
		// ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
		// 	preSpawnItems.insert(entity);
		// });

		for (int i = 0; i < MAX_WEAPONS; i++) {
			auto weapon = player->GetWeapon(i);
			if (weapon != nullptr) {
				int fire_full_clip = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, fire_full_clip, force_fire_full_clip);
				int burst_num = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, burst_num, burst_fire_count);
				int auto_full_clip = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, auto_full_clip, auto_fires_full_clip);
				if ((burst_num < 0 || fire_full_clip != 0) && auto_full_clip != 0) {
					static int auto_fires_full_clip_id = GetItemSchema()->GetAttributeDefinitionByName("auto fires full clip")->GetIndex();
					weapon->GetItem()->GetAttributeList().RemoveAttributeByDefID(auto_fires_full_clip_id);
				}
			}
		}
		
		DETOUR_MEMBER_CALL();
		
		int fov = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, fov, fov_override);
		if (fov != 0) {
			player->SetFOV(player, fov, 0.1f, 0);
		}
		// for(auto entity : preSpawnItems) {
		// 	if (!entity->IsMarkedForDeletion()) {
				
		// 	}
		// }
	}

	DETOUR_DECL_MEMBER(QAngle, CTFWeaponBase_GetSpreadAngles)
	{
		auto ret = DETOUR_MEMBER_CALL();
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		auto eye = weapon->GetOwner()->EyeAngles();
		if (eye != ret) {
			auto diff = ret - eye;
			auto mod = weapon->GetEntityModule<WeaponModule>("weapon");
			if (mod != nullptr && mod->totalAccuracyApplied != 1.0f) {
				diff *= mod->totalAccuracyApplied;
			}
			ret = eye + diff;
		}
		return ret;
	}

	inline int IsCustomAttribute(CEconItemAttribute &attr) {
		return (attr.GetAttributeDefinitionIndex() > 4200 && attr.GetAttributeDefinitionIndex() < 5000);
	}

	void DefaultValueForAttribute(attribute_data_union_t &value, const CEconItemAttributeDefinition *attribute)
	{
		if (attribute->IsType<CSchemaAttributeType_String>()) {
			LoadAttributeDataUnionFromString(attribute, value, ""s);
		}
		else {
			value.m_Float = attribute->GetDefaultValue();
		}
	}
	CTFPlayer *GetPlayerOwnerOfAttributeList(CAttributeList *list, bool ifProviding = true);

	bool attribute_manager_no_clear_cache;
	
#ifdef PLATFORM_64BITS
	DETOUR_DECL_MEMBER(bool, CSchemaAttributeType_String_BConvertStringToEconAttributeValue,const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, attribute_data_union_t *out_pValue, bool compat)
	{
		auto ret = DETOUR_MEMBER_CALL(pAttrDef, pszValue, out_pValue, compat);
		if (out_pValue != nullptr) {
			last_parsed_string_attribute_value = out_pValue->m_String;
		}
		return ret;
	}
#endif

	DETOUR_DECL_MEMBER(void, CAttributeList_SetRuntimeAttributeValue, const CEconItemAttributeDefinition *pAttrDef, float flValue)
	{	
		if (pAttrDef == nullptr) return;

		auto list = reinterpret_cast<CAttributeList *>(this);
		
		auto owner = GetPlayerOwnerOfAttributeList(list, false);
		if (owner != nullptr) {
			auto mod = owner->GetEntityModule<AddCondAttributeImmunity>("addcondimmunity");
			if (mod != nullptr && std::find(mod->attributes.begin(), mod->attributes.end(), pAttrDef) != mod->attributes.end()) return;
		}
		auto &attrs = list->Attributes();
		int countpre = attrs.Count();

		AttributeChangeType changeType = AttributeChangeType::UPDATE;
		attribute_data_union_t oldValue;

		attribute_data_union_t newValue;
#if 0
		if (!pAttrDef->IsType<CSchemaAttributeType_String>()) {
			newValue.m_Float = flValue;
		}
		else{
			Msg("try to add ptr %zu %zu\n", flValue, last_parsed_string_attribute_value);
			newValue.m_String = last_parsed_string_attribute_value;
		}
#else
		newValue.m_Float = flValue;
#endif

		bool found = false;
		for (int i = 0; i < countpre; i++) {
			CEconItemAttribute &pAttribute = attrs[i];

			if (pAttribute.GetAttributeDefinitionIndex() == pAttrDef->GetIndex())
			{
				// Found existing attribute -- change value.
				oldValue = pAttribute.GetValue();
				found = true;
				if (memcmp(&oldValue.m_Float, &flValue, sizeof(float)) != 0) {
					*pAttribute.GetValuePtr() = newValue;
					list->NotifyManagerOfAttributeValueChanges();
					changeType = AttributeChangeType::UPDATE;
				}
				else {
					changeType = AttributeChangeType::NONE;
				}
			}
		}

		// Couldn't find an existing attribute for this definition -- make a new one.
		if (!found) {
			changeType = AttributeChangeType::ADD;
			auto attribute = CEconItemAttribute::Create(pAttrDef->GetIndex());
			*attribute->GetValuePtr() = newValue;
			attrs.AddToTail(*attribute);
			CEconItemAttribute::Destroy(attribute);
			list->NotifyManagerOfAttributeValueChanges();
		}

		// DETOUR_MEMBER_CALL(pAttrDef, flValue);
		// Move around attributes so that the custom attributes appear at the end of the list
		if (pAttrDef != nullptr && countpre != attrs.Count() && attrs.Count() > 20 && (pAttrDef->GetIndex() < 4200 || pAttrDef->GetIndex() > 5000)) {
			int count = attrs.Count();
			int i = 1;
			while (i < count) {
				auto x = attrs[i];
				int cmp = IsCustomAttribute(x);
				int j = i - 1;
				while (j >= 0 && IsCustomAttribute(attrs[j]) > cmp) {
						attrs[j+1] = attrs[j];
					j = j - 1;
				}
				attrs[j+1] = x;
				i = i + 1;
			}
			/*std::sort(attrs.begin(), attrs.end(), [](auto a, auto b) {
				return IsCustomAttribute(a) - IsCustomAttribute(b);
			}); */
			list->NotifyManagerOfAttributeValueChanges();
		}
		
		if (changeType != AttributeChangeType::NONE) {
			OnAttributeChanged(list, pAttrDef, oldValue, newValue, changeType);
		}
	}

	DETOUR_DECL_MEMBER(void, CAttributeList_RemoveAttribute, const CEconItemAttributeDefinition *pAttrDef)
	{
		if (pAttrDef == nullptr) return;
		
		auto list = reinterpret_cast<CAttributeList *>(this);
		
		attribute_data_union_t oldValue;
		AttributeChangeType changeType = AttributeChangeType::NONE;

		auto attr = list->GetAttributeByID(pAttrDef->GetIndex());
		if (attr != nullptr) {
			oldValue = attr->GetValue();
			changeType = AttributeChangeType::REMOVE;
		}

		DETOUR_MEMBER_CALL(pAttrDef);

		if (changeType != AttributeChangeType::NONE) {
			attribute_data_union_t newValue;
			OnAttributeChanged(list, pAttrDef, oldValue, newValue, changeType);
		}
	}

	DETOUR_DECL_MEMBER(void, CAttributeList_RemoveAttributeByIndex, int index)
	{
		auto list = reinterpret_cast<CAttributeList *>(this);
		auto &attrs = list->Attributes();
		CEconItemAttributeDefinition *pAttrDef = nullptr;
		attribute_data_union_t oldValue;
		AttributeChangeType changeType = AttributeChangeType::NONE;
		
		if (index >= 0 && index < attrs.Count()) {
			oldValue = attrs[index].GetValue();
			pAttrDef = attrs[index].GetStaticData();
		}

		DETOUR_MEMBER_CALL(index);

		if (pAttrDef != nullptr) {
			changeType = AttributeChangeType::REMOVE;
			attribute_data_union_t newValue;
			OnAttributeChanged(list, pAttrDef, oldValue, newValue, changeType);
		}
	}

	DETOUR_DECL_MEMBER(void, CAttributeList_AddAttribute, CEconItemAttribute *attr)
	{
		auto list = reinterpret_cast<CAttributeList *>(this);

		attribute_data_union_t oldValue;
		auto attrOld = list->GetAttributeByID(attr->GetAttributeDefinitionIndex());
		AttributeChangeType changeType = AttributeChangeType::NONE;
		if (attrOld != nullptr) {
			oldValue = attr->GetValue();
		}

		DETOUR_MEMBER_CALL(attr);

		if (oldValue.m_UInt != attr->GetValue().m_UInt) {
			changeType = AttributeChangeType::ADD;
			OnAttributeChanged(list, attr->GetStaticData(), oldValue, attr->GetValue(), changeType);
		}
	}

	DETOUR_DECL_MEMBER(void, CAttributeList_DestroyAllAttributes)
	{
		auto list = reinterpret_cast<CAttributeList *>(this);
		auto &attrs = list->Attributes();
		if (attrs.Count()) {
			for (int i = attrs.Count() - 1; i >= 0; i--) {
				auto &attr = attrs[i];
				CEconItemAttributeDefinition *pAttrDef = attr.GetStaticData();
				attribute_data_union_t oldValue = attr.GetValue();
				attrs.Remove(i);

				attribute_data_union_t newValue;
				OnAttributeChanged(list, pAttrDef, oldValue, newValue, AttributeChangeType::REMOVE);
			}
			list->NotifyManagerOfAttributeValueChanges();
		}
	}

	DETOUR_DECL_MEMBER(void, CAttributeList_NotifyManagerOfAttributeValueChanges)
	{
		auto list = reinterpret_cast<CAttributeList *>(this);
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(float, CTFKnife_GetMeleeDamage, CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage)
	{
		float ret = DETOUR_MEMBER_CALL(pTarget, piDamageType, piCustomDamage);
		auto knife = reinterpret_cast<CTFKnife *>(this);

		if (*piCustomDamage == TF_DMG_CUSTOM_BACKSTAB && knife->GetTFPlayerOwner() != nullptr && !knife->GetTFPlayerOwner()->IsBot() && pTarget->IsPlayer() && ToTFPlayer(pTarget)->IsMiniBoss() ) {
			
			//int backstabs = backstab_count[pTarget];

			float armor_piercing_attr = 25.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( knife, armor_piercing_attr, armor_piercing );
			if (armor_piercing_attr > 125 || armor_piercing_attr < 25) {
				ret = 250 * (armor_piercing_attr / 100.0f);
			}
		}
		return ret;
	}
	
	class MedigunDrainModule : public EntityModule
	{
	public:
		MedigunDrainModule(CBaseEntity *entity) : EntityModule(entity) {}

		float maxOverheal = 1.0f;
		bool healingReduced = false;
		float healFraction = 0.0f;
	};

	DETOUR_DECL_MEMBER(void, CWeaponMedigun_RemoveHealingTarget, bool flag)
	{
		auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
		auto target = ToTFPlayer(medigun->GetHealTarget());
		if (target != nullptr) {
			GET_STRING_ATTRIBUTE(medigun, medigun_passive_attributes, attribs);
			if (attribs != nullptr) {
				RemoveMedigunAttributes(target, attribs);
			}
			GET_STRING_ATTRIBUTE(medigun, medigun_passive_attributes_owner, attribsOwner);
			if (attribsOwner != nullptr && medigun->GetTFPlayerOwner() != nullptr) {
				RemoveMedigunAttributes(medigun->GetTFPlayerOwner(), attribsOwner);
			}
		}
		auto attackModule = medigun->GetEntityModule<MedigunAttackModule>("medigunattack");
		if (attackModule != nullptr) {
			if (attackModule->lastTarget != nullptr && medigun->GetTFPlayerOwner() != nullptr) {
				medigun->GetTFPlayerOwner()->m_Shared->StopHealing(medigun->GetTFPlayerOwner());
			}
			attackModule->lastTarget = nullptr;
		}

		if (medigun->GetCustomVariableBool<"healingnonplayer">() && medigun->GetHealTarget() == nullptr) {
			medigun->SetHealTarget(nullptr);
			medigun->SetCustomVariable("healingnonplayer", Variant(false));
		}

		if (medigun->GetCustomVariableBool<"healingnonplayer">()) {
			medigun->SetCustomVariable("healingnonplayer", Variant(false));
		}
	
        DETOUR_MEMBER_CALL(flag);
    }

	void AddMedigunAttributes(CAttributeList *target, const char *attribs)
	{
		std::string str(attribs);
		boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

		auto it = tokens.begin();
		while (it != tokens.end()) {
			auto attribute = *it;
			if (++it == tokens.end())
				break;
			auto &value = *it;
			auto attr_def = GetItemSchema()->GetAttributeDefinitionByName(attribute.c_str());
			if (attr_def != nullptr) {
				target->AddStringAttribute(attr_def, value);
			}
			++it;
		}
	}

	RefCount rc_CWeaponMedigun_StartHealingTarget_Drain;
	DETOUR_DECL_MEMBER(void, CWeaponMedigun_StartHealingTarget, CBaseEntity *targete)
	{
		auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
		
		auto target = ToTFPlayer(targete);
		auto owner = medigun->GetTFPlayerOwner();
		if (target != nullptr) {
			GET_STRING_ATTRIBUTE(medigun, medigun_passive_attributes, attribs);
			if (attribs != nullptr) {
				AddMedigunAttributes(target->GetAttributeList(), attribs);
				target->TeamFortress_SetSpeed();
			}
			GET_STRING_ATTRIBUTE(medigun, medigun_passive_attributes_owner, attribsOwner);
			if (attribsOwner != nullptr && owner != nullptr) {
				AddMedigunAttributes(owner->GetAttributeList(), attribsOwner);
				owner->TeamFortress_SetSpeed();
			}
		}
		float attacking = 0;
		if (owner != nullptr && target != nullptr && target != owner && target->GetTeamNumber() != owner->GetTeamNumber()) {
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(medigun, attacking, medigun_attack_enemy);
			if (attacking != 0) {
				targete = owner;
			}
		}
		float drain = 0;
		if (owner != nullptr && target != nullptr && target != owner && target->GetTeamNumber() == owner->GetTeamNumber()) {
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(medigun, drain, medigun_self_drain);
			if (drain != 0) {
				auto mod = medigun->GetOrCreateEntityModule<MedigunDrainModule>("medigundrain");
				mod->healingReduced = false;
				mod->healFraction = 0;
			}
		}
		

		SCOPED_INCREMENT_IF(rc_CWeaponMedigun_StartHealingTarget, attacking != 0 && owner->m_Shared->FindHealerIndex(owner) == -1);
		SCOPED_INCREMENT_IF(rc_CWeaponMedigun_StartHealingTarget_Drain, drain != 0);
		startHealingTargetHealer = owner;
		startHealingTarget = targete;
        DETOUR_MEMBER_CALL(targete);
		startHealingTarget = nullptr;
		startHealingTargetHealer = nullptr;
    }

	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_Energy_Recharge)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		
		int iWeaponMod = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, iWeaponMod, reload_full_clip_at_once );
		if (iWeaponMod == 1)
		{
			while(true) {
				if (DETOUR_MEMBER_CALL()) {
					break;
				}
			}
			//weapon->m_flEnergy = weapon->Energy_GetMaxEnergy();
			//return true;
		}

		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, CBasePlayer_ItemPostFrame)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		auto weapon = player->GetActiveTFWeapon();
		bool pressedM2 = false;
		if (weapon != nullptr) {
			if (GetFastAttributeInt(weapon, 0, ALT_FIRE_DISABLED) != 0) {
				weapon->m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
				if (player->m_nButtons & IN_ATTACK2) {
					player->m_nButtons &= ~IN_ATTACK2;
					pressedM2 = true;
				}
			}
			if ((player->m_nButtons & IN_ATTACK2) && GetFastAttributeFloat(weapon, 0, ALT_FIRE_ATTACK) != 0) {
				auto mod = player->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
				player->SetCustomVariable("nextattackreset", Variant(gpGlobals->tickcount));
				mod->props["m_flNextAttack"] = {Variant(gpGlobals->curtime + 0.4f), Variant(gpGlobals->curtime + 0.4f)};
			}
			
			auto mod = weapon->GetEntityModule<AltFireAttributesModule>("altfireattrs");
			if (mod != nullptr && player->m_nButtons & IN_ATTACK2 && !mod->active) {
				GET_STRING_ATTRIBUTE(weapon, alt_fire_attributes, attribs);
				if (attribs != nullptr) {
					AddMedigunAttributes(&weapon->GetItem()->GetAttributeList(), attribs);
				}
				mod->active = true;
			}
			else if (mod != nullptr && !(player->m_nButtons & IN_ATTACK2) && mod->active) {
				GET_STRING_ATTRIBUTE(weapon, alt_fire_attributes, attribs);
				if (attribs != nullptr) {
					RemoveOnActiveAttributes(weapon, attribs);
				}
				mod->active = false;
			}

			int ammoPerShot = GetFastAttributeInt(weapon, 0, MOD_AMMO_PER_SHOT);
			if (ammoPerShot > 1) {
				bool hasAmmo = weapon->GetMaxClip1() != -1 ? weapon->m_iClip1 >= ammoPerShot : player->GetAmmoCount(weapon->m_iPrimaryAmmoType) >= ammoPerShot;
				int noAttack = GetFastAttributeInt(weapon, 0, NO_ATTACK);
				static int noAttackId = GetItemSchema()->GetAttributeDefinitionByName("no_attack")->GetIndex();
				if (noAttack != 0 && hasAmmo) {
					weapon->SetCustomVariable("noattackreset", Variant(false));
					weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(noAttackId, 0);
				}
				else if (noAttack == 0 && !hasAmmo) {
					weapon->SetCustomVariable("noattackreset", Variant(true));
					weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(noAttackId, 1);
				}
			}

			int holdFire = GetFastAttributeInt(weapon, 0, HOLD_FIRE_UNTIL_FULL_RELOAD);
			if (holdFire != 0)
			{
				bool hasFullClip = weapon->IsEnergyWeapon() ? weapon->m_flEnergy >= weapon->Energy_GetMaxEnergy() : weapon->m_iClip1 >= weapon->GetMaxClip1();
				bool isReloading = weapon->m_iReloadMode > 0;
				int noAttack = GetFastAttributeInt(weapon, 0, NO_ATTACK);
				static int noAttackId = GetItemSchema()->GetAttributeDefinitionByName("no_attack")->GetIndex();
				bool autoFiresClip = GetFastAttributeInt(weapon, 0, AUTO_FIRES_FULL_CLIP) != 0;
				if (isReloading) {
					if (autoFiresClip) {
						if (weapon->m_iClip1 > 0 && weapon->m_iClip1 < weapon->GetMaxClip1()) {
							player->m_nButtons |= IN_ATTACK;
						}
					}
					else {
						player->m_nButtons &= ~IN_ATTACK;
					}
				}
				//bool setNoAttack = isReloading && (weapon->m_iClip1 < weapon->GetMaxClip1() - 1 || gpGlobals->curtime + gpGlobals->interval_per_tick < weapon->m_flNextPrimaryAttack);
				if (noAttack != 0 && !isReloading) {
					weapon->SetCustomVariable("noattackreset", Variant(false));
					weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(noAttackId, 0);
				}
				else if (noAttack == 0 && isReloading) {
					weapon->SetCustomVariable("noattackreset", Variant(true));
					weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValueByDefID(noAttackId, 1);
				}
			}
		}
		

		DETOUR_MEMBER_CALL();
		if (weapon != nullptr) {
			OnWeaponUpdate(weapon);
			// if ((player->m_nButtons & IN_ATTACK2) && GetFastAttributeFloat(weapon, 0, ALT_FIRE_ATTACK) != 0) {
			// 	player->m_flNextAttack = gpGlobals->curtime + 0.1f;
			// }
		}

		if (gpGlobals->tickcount % 3 == 2) {
			for (int i = 0; i < player->GetNumWearables(); i++) {
				auto wearable = player->GetWearable(i);
				if (wearable != nullptr) {
					
					int addcond = GetFastAttributeInt(wearable, 0, ADD_COND_ON_ACTIVE);
					if (addcond != 0) {
						for (int i = 0; i < 3; i++) {
							int addcond_single = (addcond >> (i * 8)) & 255;
							if (addcond_single != 0) {
								player->m_Shared->AddCond((ETFCond)addcond_single, 0.3f, player);
							}
						}
					}
				}
			}
		}

		auto nextAttackReset = player->GetCustomVariableInt<"nextattackreset">();
		if (nextAttackReset != 0 && nextAttackReset != gpGlobals->tickcount) {
			player->SetCustomVariable("nextattackreset", Variant(0));
			auto mod = player->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
			mod->props.erase("m_flNextAttack");
		}
		if (pressedM2) {
			player->m_nButtons |= IN_ATTACK2;
		}
	}

	DETOUR_DECL_MEMBER(float, CWeaponMedigun_GetHealRate)
	{
		auto weapon = reinterpret_cast<CWeaponMedigun *>(this);

		auto healRate = DETOUR_MEMBER_CALL();
		if (rtti_cast<CTFReviveMarker *>(weapon->GetHealTarget()) != nullptr) {
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, healRate, revive_rate);
		}
		return healRate;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_Taunt, taunts_t index, int taunt_concept)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		DETOUR_MEMBER_CALL(index, taunt_concept);

		if (player->m_flTauntAttackTime > gpGlobals->curtime) {
			float attackDelayMult = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, attackDelayMult, taunt_attack_time_mult);
			if (attackDelayMult != 1.0f) {
				float attackDelay = player->m_flTauntAttackTime - gpGlobals->curtime;
				attackDelay *= attackDelayMult;
				player->m_flTauntAttackTime = gpGlobals->curtime + attackDelay;
			}
		}
	}

	DETOUR_DECL_MEMBER(float, CTFWeaponBase_ApplyFireDelay, float delay)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);

		delay = DETOUR_MEMBER_CALL(delay);

		if (weapon->IsMeleeWeapon()) {
			float flReducedHealthBonus = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, flReducedHealthBonus, mult_postfiredelay_with_reduced_health );
			if ( flReducedHealthBonus != 1.0f )
			{
				flReducedHealthBonus = RemapValClamped( weapon->GetOwner()->GetHealth() / weapon->GetOwner()->GetMaxHealth(), 0.2f, 0.9f, flReducedHealthBonus, 1.0f );
				delay *= flReducedHealthBonus;
			}
		}
		return delay;
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_ApplyPunchImpulseX, float amount)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		int noDamageFlinch = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, noDamageFlinch, no_damage_view_flinch);

		if (noDamageFlinch != 0) {
			return false;
		}

		return DETOUR_MEMBER_CALL(amount);
	}

    DETOUR_DECL_MEMBER(void, CTFPlayer_ForceRespawn)
	{
		if (player_taking_damage != nullptr && player_taking_damage == reinterpret_cast<CTFPlayer *>(this)) {
			
			int teleport = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player_taking_damage, teleport, teleport_instead_of_die);
			if (teleport != 0) {
				PrecacheSound("Halloween.Merasmus_TP_In");
				player_taking_damage->EmitSound("Halloween.Merasmus_TP_In");
				Vector tele_pos = player_taking_damage->GetAbsOrigin();
				CPVSFilter filter(tele_pos);
				bf_write *msg = engine->UserMessageBegin(&filter, usermessages->LookupUserMessage("PlayerTeleportHomeEffect"));
				msg->WriteByte(ENTINDEX(player_taking_damage));
				engine->MessageEnd();
				
				TE_TFParticleEffect(filter, 0.0f, "teleported_blue", tele_pos, vec3_angle);
				TE_TFParticleEffect(filter, 0.0f, "player_sparkles_blue", tele_pos, vec3_angle);
			}
		}
        return DETOUR_MEMBER_CALL();
    }

	void Misfire(CTFWeaponBaseGun *weapon)
	{
		weapon->CalcIsAttackCritical();
		//DETOUR_MEMBER_CALL();

		CTFPlayer *player = weapon->GetTFPlayerOwner();
		if (!player) return;

		CBaseEntity *entity = weapon->FireProjectile(player);
		CTFBaseRocket *rocket = rtti_cast<CTFBaseRocket *>(entity);
		CTFWeaponBaseGrenadeProj *grenade = rtti_cast<CTFWeaponBaseGrenadeProj *>(entity);
		if (rocket != nullptr && rtti_cast<CTFProjectile_Arrow *>(entity) == nullptr) {
			trace_t tr;
			UTIL_TraceLine(rocket->GetAbsOrigin(), player->EyePosition(), MASK_SOLID, rocket, COLLISION_GROUP_NONE, &tr);
			rocket->Explode(&tr, player);
		}
		if (grenade != nullptr) {
			trace_t tr;
			UTIL_TraceLine(grenade->GetAbsOrigin(), player->EyePosition(), MASK_SOLID, grenade, COLLISION_GROUP_NONE, &tr);
			grenade->Explode(&tr, weapon->GetDamageType());
		}
	}

	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_CheckReloadMisfire)
	{
		bool result = DETOUR_MEMBER_CALL();

		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		int canOverload = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, canOverload, can_overload);

		if (canOverload != 0 && weapon->m_iClip1 >= weapon->GetMaxClip1()) {
			auto weapongun = rtti_cast<CTFWeaponBaseGun *>(weapon);
			if (weapongun == nullptr) return result;
			Misfire(weapongun);
			return true;
		}
		return result;
	}

    DETOUR_DECL_MEMBER(bool, CObjectSentrygun_FindTarget)
    {
        bool ret{DETOUR_MEMBER_CALL()};
        auto sentry{reinterpret_cast<CObjectSentrygun*>(this)};
        CTFPlayer* builder{sentry->GetBuilder()};
        if(builder){
            int value = 0;
            CALL_ATTRIB_HOOK_INT_ON_OTHER(builder, value, disable_wrangler_shield);
            if(value > 0){
                sentry->m_nShieldLevel = 0;
                return ret;
            }
            // CTFWeaponBase* weapon{builder->GetActiveTFWeapon()};
            // if(weapon){
			// 	int value = 0;
            //     CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, value, disable_wrangler_shield);
            //     if(value > 0)
            //         sentry->m_nShieldLevel = 0;
            // }
        }
        return ret;
    }

	DETOUR_DECL_MEMBER(void, CTFGameMovement_ToggleParachute)
    {
		CTFPlayer *player = ToTFPlayer(reinterpret_cast<CGameMovement *>(this)->player);
		//if ((player->GetFlags() & FL_ONGROUND) || (reinterpret_cast<CGameMovement *>(this)->GetMoveData()->m_nOldButtons & IN_JUMP)) return;
		//ClientMsg(player, "redepl\n");
        DETOUR_MEMBER_CALL();
		if (player->m_Shared->InCond(TF_COND_PARACHUTE_DEPLOYED)) {
			int parachuteRedeploy = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, parachuteRedeploy, parachute_redeploy);
			if (parachuteRedeploy != 0) {
				player->m_Shared->RemoveCond(TF_COND_PARACHUTE_DEPLOYED);
			}
		}
    }
	
	DETOUR_DECL_MEMBER(void, CTFGameMovement_HandleDuckingSpeedCrop)
    {
		auto movement = reinterpret_cast<CGameMovement *>(this);
		CTFPlayer *player = ToTFPlayer(movement->player);

		float preSpeedFw = movement->GetMoveData()->m_flForwardMove;
		float preSpeedSide = movement->GetMoveData()->m_flSideMove;
		float preSpeedUp = movement->GetMoveData()->m_flUpMove;
		
		
        DETOUR_MEMBER_CALL();
		if (preSpeedFw != movement->GetMoveData()->m_flForwardMove || preSpeedSide != movement->GetMoveData()->m_flSideMove || preSpeedUp != movement->GetMoveData()->m_flUpMove) {
			float mult = GetFastAttributeFloat(player, 1, MULT_DUCK_SPEED);
			movement->GetMoveData()->m_flForwardMove *= mult;
			movement->GetMoveData()->m_flSideMove *= mult;
			movement->GetMoveData()->m_flUpMove *= mult;
		}
    }
	
	RefCount rc_ProvidingAttributes;
	DETOUR_DECL_MEMBER(void, CAttributeManager_ProvideTo, CBaseEntity *entity)
    {
		SCOPED_INCREMENT(rc_ProvidingAttributes);
		auto manager = reinterpret_cast<CAttributeManager *>(this);
        DETOUR_MEMBER_CALL(entity);
		auto item = rtti_cast<CEconEntity *>(manager->m_hOuter.Get().Get());
		if (item != nullptr) {
			auto &attrs = item->GetItem()->GetAttributeList().Attributes(); 

			attribute_data_union_t oldValue;
			
			ForEachItemAttribute(item->GetItem(), [&](const CEconItemAttributeDefinition *def,  attribute_data_union_t val){
				OnAttributeChanged(&item->GetItem()->GetAttributeList(), def, oldValue, val, AttributeChangeType::ADD);
				return true;
			});
		}
    }
	
	CBaseEntity *stop_provider_entity = nullptr;
	DETOUR_DECL_MEMBER(void, CAttributeManager_StopProvidingTo, CBaseEntity *entity)
    {
		SCOPED_INCREMENT(rc_ProvidingAttributes);
        DETOUR_MEMBER_CALL(entity); 
		auto manager = reinterpret_cast<CAttributeManager *>(this);
		auto item = rtti_cast<CEconEntity *>(manager->m_hOuter.Get().Get());
		if (item != nullptr) {
			auto &attrlist = item->GetItem()->GetAttributeList();
			auto &attrs = attrlist.Attributes(); 
			
			attribute_data_union_t newValue;
			stop_provider_entity = entity;
			
			ForEachItemAttribute(item->GetItem(), [&](const CEconItemAttributeDefinition *def,  attribute_data_union_t val){
				OnAttributeChanged(&attrlist, def, val, newValue, AttributeChangeType::REMOVE);
				return true;
			});

			stop_provider_entity = nullptr;
		}
    }

	DETOUR_DECL_MEMBER(void, CTFPlayer_HandleAnimEvent, animevent_t *pEvent)
    {
		DETOUR_MEMBER_CALL(pEvent);
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if ((pEvent->event == AE_WPN_HIDE || pEvent->event == AE_WPN_UNHIDE) && player->GetActiveTFWeapon() != nullptr) {
			GET_STRING_ATTRIBUTE(player->GetActiveTFWeapon(), custom_item_model, model);
			if (model != nullptr) {
				if (pEvent->event == AE_WPN_HIDE) {
					player->GetActiveTFWeapon()->AddEffects(EF_NODRAW);
				}
				else if (pEvent->event == AE_WPN_UNHIDE) {
					player->GetActiveTFWeapon()->RemoveEffects(EF_NODRAW);
				}
			}
		}
	}

	DETOUR_DECL_MEMBER(Vector, CTFWeaponBase_GetParticleColor, int color)
    {
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		int particleColor = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, particleColor, particle_color_rgb);
		if (particleColor != 0) {
			Color clr = Color( ((particleColor & 0xFF0000) >> 16), ((particleColor & 0xFF00) >> 8), (particleColor & 0xFF) );

			float fColorMod = 1.f;
			if ( color == 2 )
			{
				fColorMod = 0.5f;
			}

			Vector vResult;
			vResult.x = clamp( fColorMod * clr.r() * (1.f/255), 0.f, 1.0f );
			vResult.y = clamp( fColorMod * clr.g() * (1.f/255), 0.f, 1.0f );
			vResult.z = clamp( fColorMod * clr.b() * (1.f/255), 0.f, 1.0f );
			return vResult;
		}
		float particleRainbow = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, particleRainbow, particle_color_rainbow);
		if (particleRainbow != 0) {

			Vector vResult;
			HSVtoRGB(Vector((int)(gpGlobals->curtime * particleRainbow) % 360, 1 , 1), vResult);

			return vResult;
		}
		return DETOUR_MEMBER_CALL(color);
	}

	DETOUR_DECL_STATIC(CTFProjectile_EnergyRing *, CTFProjectile_EnergyRing_Create, CTFWeaponBaseGun *pLauncher, const Vector &vecOrigin, const QAngle& vecAngles, float fSpeed, float fGravity, 
			CBaseEntity *pOwner, CBaseEntity *pScorer, Vector vColor1, Vector vColor2, bool bCritical)
    {
		auto ring = DETOUR_STATIC_CALL(pLauncher, vecOrigin, vecAngles, fSpeed, fGravity, pOwner, pScorer, vColor1, vColor2, bCritical);
		if (ring != nullptr && pLauncher != nullptr) {
			int particleColor = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(pLauncher, particleColor, particle_color_rgb);
			float particleRainbow = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pLauncher, particleRainbow, particle_color_rainbow);
			if (particleColor != 0 || particleRainbow != 0) {
				const char *particle;
				int penetrate = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(pLauncher, penetrate, energy_weapon_penetration);
				if (penetrate && bCritical) {
					particle = "drg_bison_projectile_crit";
				}
				else if (penetrate && !bCritical) {
					particle = "drg_bison_projectile";
				}
				else if (!penetrate && bCritical) {
					particle = "drg_pomson_projectile_crit";
				}
				else {
					particle = "drg_pomson_projectile";
				}

				force_send_client = true;
				CRecipientFilter filter;
				filter.AddAllPlayers();
				Vector color0 = pLauncher->GetParticleColor(1);
				Vector color1 = pLauncher->GetParticleColor(2);
				DispatchParticleEffect(particle, PATTACH_ABSORIGIN_FOLLOW, ring, nullptr, vec3_origin, false, color0, color1, true, true, nullptr, &filter);
				force_send_client = false;
			}
		}
		return ring;
	}

	DETOUR_DECL_MEMBER(void, CTFBat_Wood_SecondaryAttack)
    {
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		bool good = weapon->m_flNextPrimaryAttack <= gpGlobals->curtime;
		DETOUR_MEMBER_CALL();
		if (good && weapon->m_flNextPrimaryAttack > gpGlobals->curtime) {
			float fireRateMult = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, fireRateMult, mult_postfiredelay);
			weapon->m_flNextPrimaryAttack = (weapon->m_flNextPrimaryAttack - gpGlobals->curtime) * fireRateMult + (gpGlobals->curtime);
			weapon->SetNextThink(-1, "LAUNCH_BALL_THINK");
			weapon->SetNextThink(gpGlobals->curtime + 0.01, "LAUNCH_BALL_THINK");
		}
	}

	DETOUR_DECL_MEMBER(bool, CObjectSentrygun_ValidTargetPlayer, CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd)
    {
		int ignore = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pPlayer, ignore, ignored_by_enemy_sentries);
		if (ignore != 0) return false;

		return DETOUR_MEMBER_CALL(pPlayer, vecStart, vecEnd);
	}

	ConVar sig_attr_burn_time_faster_burn("sig_attr_burn_time_faster_burn", "1", FCVAR_NOTIFY | FCVAR_GAMEDLL, 
		"Mod: For non bot players in mvm mode, burn time bonus increases afterburn damage frequency instead of duration");

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_Burn, CTFPlayer *igniter, CTFWeaponBase *weapon, float duration)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		//Msg("Igniter: %d Weapon: %d %s Duration: %f\n", igniter, weapon, weapon != nullptr ? weapon->GetClassname() : "", duration);
		float remainingFlameTime = shared->m_flFlameRemoveTime;
		
		if (igniter == shared->GetOuter()) {
			int noSelfEffect = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( weapon != nullptr ? (CBaseEntity*)weapon : igniter, noSelfEffect, no_self_effect);
			if (noSelfEffect != 0) {
				return;
			}
		}
		if (remainingFlameTime < 0.0f) {
			remainingFlameTime = 0.0f;
		}
		// Don't remove exisiting afterburn if the current one deals more damage
		if (remainingFlameTime > 0.5f && shared->m_hBurnWeapon != nullptr && shared->m_hBurnWeapon != weapon) {

			float currentFlameDmg = 4;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(shared->m_hBurnWeapon, currentFlameDmg, mult_wpn_burndmg);
			float newFlameDmg = 4;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, newFlameDmg, mult_wpn_burndmg);

			if (currentFlameDmg > newFlameDmg) {
				return;
			}
		}
		DETOUR_MEMBER_CALL(igniter, weapon, duration);
		if (weapon != nullptr && remainingFlameTime != shared->m_flFlameRemoveTime) {
			float mult = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, mult, mult_afterburn_delay);
			if (sig_attr_burn_time_faster_burn.GetBool() && TFGameRules()->IsMannVsMachineMode() && igniter != nullptr && !igniter->IsBot()) {
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, mult, mult_wpn_burntime);
			}
			else {
				float multBurnTime = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, multBurnTime, mult_wpn_burntime);
				float durationReal = (duration < 0.0f && weapon != nullptr ? weapon->GetAfterburnRateOnHit() : duration ) * multBurnTime;
				if (multBurnTime > 1.0f && shared->m_flFlameRemoveTime > 9.9f && durationReal * multBurnTime > 10.0f) {
					shared->m_flFlameRemoveTime = Min(10 * multBurnTime, remainingFlameTime + durationReal * multBurnTime);
					//Msg("Set remove time to %f\n", shared->m_flFlameRemoveTime);
				}
			}

			if (mult > 1.0f) {
				shared->m_flFlameBurnTime -= ((1.0f - 1.0f/mult) * 0.5f);
			}
			
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_ConditionGameRulesThink)
	{
		//TIME_SCOPE2(GameRulesThink);
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		float nextFlameTime = shared->m_flFlameBurnTime;

		DETOUR_MEMBER_CALL();

		auto &bleedVec = shared->m_BleedInfo.Get();
		FOR_EACH_VEC(bleedVec, i) {
			auto &info = bleedVec[i];
			if (info.flBleedingTime == gpGlobals->curtime + 0.5f) {
				
				float mult = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.hBleedingWeapon != nullptr ? (CBaseEntity*)info.hBleedingWeapon.Get() : info.hBleedingAttacker.Get(), mult, mult_bleeding_delay);
				if (mult != 1.0f) {
					info.flBleedingTime = gpGlobals->curtime + 0.5f * mult;
				}
			}
		}
		if (nextFlameTime != shared->m_flFlameBurnTime && shared->m_hBurnWeapon != nullptr) {
			float mult = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(shared->m_hBurnWeapon, mult, mult_afterburn_delay);
			
			if (sig_attr_burn_time_faster_burn.GetBool() && TFGameRules()->IsMannVsMachineMode() && shared->m_hBurnWeapon->GetTFPlayerOwner() != nullptr && !shared->m_hBurnWeapon->GetTFPlayerOwner()->IsBot()) {
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(shared->m_hBurnWeapon, mult, mult_wpn_burntime);
			}
			if (mult > 1.0f) {
				shared->m_flFlameRemoveTime += ((1.0f - 1.0f/mult) * 0.5f);
				shared->m_flFlameBurnTime -= ((1.0f - 1.0f/mult) * 0.5f);
			}
		}
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CGameMovement_CheckFalling)
	{
		auto me = reinterpret_cast<CGameMovement *>(this);
		auto player = reinterpret_cast<CTFPlayer *>(me->player);
		float fall = player->m_Local->m_flFallVelocity;
		if (player->GetGroundEntity() != nullptr && player->IsAlive() && fall > 0) {
			float fallMinVel = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, fallMinVel, kb_fall_min_velocity);
			float kbRadius = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, kbRadius, kb_fall_radius);
			float kbStunTime = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, kbStunTime, kb_fall_stun_time);
			float kbStrength = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, kbStrength, kb_fall_force);
			float kbDamage = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, kbDamage, kb_fall_damage);
			if (fallMinVel != 0 && fall > fallMinVel) {
				if (kbRadius == 0) {
					kbRadius = 230;
				}
				if (kbStunTime == 0) {
					kbStunTime = 5;
				}
				if (kbStrength == 0) {
					kbStrength = 300;
				}
				if (kbDamage == 0) {
					kbDamage = 50;
				}
				Vector point = player->GetAbsOrigin();
				ForEachTFPlayer([&](CTFPlayer *playerl){
					if (!playerl->IsAlive() || playerl->GetTeamNumber() == player->GetTeamNumber()) return;

					Vector toPlayer = playerl->EyePosition() - point;

					if ( toPlayer.LengthSqr() < kbRadius * kbRadius )
					{
						// send the player flying
						// make sure we push players up and away
						toPlayer.z = 0.0f;
						toPlayer.NormalizeInPlace();
						toPlayer.z = 1.0f;

						Vector vPush = kbStrength * toPlayer;

						playerl->ApplyAbsVelocityImpulse( vPush );
						playerl->TakeDamage(CTakeDamageInfo(player, player, nullptr, vec3_origin, player->GetAbsOrigin(), kbDamage, DMG_FALL, TF_DMG_CUSTOM_BOOTS_STOMP));
						if (!playerl->IsMiniBoss() && kbStunTime > 0) {
							playerl->m_Shared->StunPlayer(kbStunTime, 0.85, 2, player);
						}
					}
				});
			}
		}
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CCurrencyPack_MyTouch, CBasePlayer *player)
	{
		if (GetFastAttributeFloat(player, 1.0f, MULT_CREDIT_COLLECT_RANGE) <= 0) return false;
		
		int nCurHealth = player->GetHealth();
		int nMaxHealth = player->GetMaxHealth();
		bool ret = DETOUR_MEMBER_CALL(player);
		if (ret) {
			float health = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, health, health_from_credits);
			if (health != 0) {
				float nHealth = nCurHealth < nMaxHealth ? health : health * 0.5f;

				int nHealthCap = nMaxHealth * 4;
				if ( nCurHealth > nHealthCap )
				{
					nHealth = RemapValClamped( nCurHealth, nHealthCap, (nHealthCap * 1.5f), health * 0.4f, health * 0.1f );
				}

				if (nHealth > 0) {
					player->TakeHealth( nHealth, DMG_IGNORE_MAXHEALTH );
				}
				else {
					player->TakeDamage(CTakeDamageInfo(player, player, nullptr, vec3_origin, vec3_origin, (nHealth * -1), DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE));
				}
			}
		}
		return ret;
	}

	CBasePlayer *playerAnimSender = nullptr;
	DETOUR_DECL_MEMBER(void, CVEngineServer_PlaybackTempEntity, IRecipientFilter& filter, float delay, const void *pSender, const SendTable *pST, int classID)
	{
		if (playerAnimSender != nullptr) {
			CRecipientFilter filter2;
			filter2.CopyFrom(filter);
			filter2.AddRecipient(playerAnimSender);
			DETOUR_MEMBER_CALL(filter2, delay, pSender, pST, classID);
			return;
		}
		DETOUR_MEMBER_CALL(filter, delay, pSender, pST, classID);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_TraceAttack, const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, void *pAccumulator)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_TraceAttack);
		DETOUR_MEMBER_CALL(info, vecDir, ptr, pAccumulator);
	}

	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_IsPassiveWeapon)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		return GetFastAttributeInt(weapon, 0, IS_PASSIVE_WEAPON) != 0;
		//return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(int, CBaseCombatWeapon_GetMaxClip1)
	{
		auto weapon = reinterpret_cast<CBaseCombatWeapon *>(this);
		int clipAttr = GetFastAttributeInt(weapon, 0, MOD_MAX_PRIMARY_CLIP_OVERRIDE);
		if (clipAttr != 0) {
			return clipAttr;
		}
		return weapon->GetWpnData().iMaxClip1;
		//return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(int, CTFWeaponBase_AutoFiresFullClip)
	{
		auto weapon = reinterpret_cast<CBaseCombatWeapon *>(this);
		return GetFastAttributeInt(weapon, 0, AUTO_FIRES_FULL_CLIP) != 0;
		//return DETOUR_MEMBER_CALL();
	}


	DETOUR_DECL_MEMBER(float, CTFWeaponBase_GetAfterburnRateOnHit)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		float burn_duration = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, burn_duration, set_dmgtype_ignite);
		if (burn_duration != 0.0f) {
			return 7.5f;
		}
		return DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		IVision *vision = reinterpret_cast<IVision *>(this);

		if (ent->IsPlayer() && GetFastAttributeInt(ent, 0, IGNORED_BY_BOTS) ) {
			return true;
		}
		if (ent->IsBaseObject() && GetBuildingAttributeInt<"ignoredbybots">(static_cast<CBaseObject *>(ent), "ignored_by_bots", false) != 0) {
			return true;
		}
		
		return DETOUR_MEMBER_CALL(ent);
	}

	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_SendWeaponAnim, int activity)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);

		if (rc_CTFWeaponBase_Reload && GetFastAttributeInt(weapon, 0, PASSIVE_RELOAD) && weapon->GetTFPlayerOwner() != nullptr && weapon->GetTFPlayerOwner()->GetActiveTFWeapon() != weapon) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL(activity);
	}
	
    DETOUR_DECL_MEMBER(bool, CTFPlayer_CanDisguise)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		int alwaysAllowDisguise = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, alwaysAllowDisguise, always_allow_disguise);
		if (alwaysAllowDisguise != 0) {
			return true;
		}
        return DETOUR_MEMBER_CALL();
    }
	
    DETOUR_DECL_MEMBER(bool, CTFPlayer_CanGoInvisible, bool flagcheck)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		int alwaysAllowCloak = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, alwaysAllowCloak, always_allow_cloak);
		if (alwaysAllowCloak != 0) {
			return true;
		}
        return DETOUR_MEMBER_CALL(flagcheck);
    }
	
    DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, bool, CObjectTeleporter_PlayerCanBeTeleported, CTFPlayer *player)
	{
		int alwaysAllowTeleport = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, alwaysAllowTeleport, always_allow_teleport);
		if (alwaysAllowTeleport != 0) {
			return true;
		}
        return DETOUR_MEMBER_CALL(player);
    }
	
    DETOUR_DECL_MEMBER(bool, CSpellPickup_ItemCanBeTouchedByPlayer, CBasePlayer *player)
	{
		int cannotPickup = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, cannotPickup, cannot_pickup_spells);
        return DETOUR_MEMBER_CALL(player);
    }
	void ChangeBuildingProperties(CTFPlayer *player, CBaseObject *obj);

	DETOUR_DECL_MEMBER(void, CTFPlayer_RemoveAllOwnedEntitiesFromWorld, bool explode)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		int destroyBuildingsOnDeath = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, destroyBuildingsOnDeath, destroy_buildings_on_death);
		if (destroyBuildingsOnDeath != 0) {
			for (int i = player->GetObjectCount() - 1; i >= 0 ; i--) {
				auto obj = player->GetObject(i);
				if (obj != nullptr) {
					obj->DetonateObject();
				}
			}
		}
		
		DETOUR_MEMBER_CALL(explode);
	}

	DETOUR_DECL_MEMBER(void, CTFBot_Event_Killed, const CTakeDamageInfo& info)
	{
		// Make it so that if the buildings still persist after bot is removed, copy the attributes over
		auto player = reinterpret_cast<CTFBot *>(this);
		for (int i = player->GetObjectCount() - 1; i >= 0 ; i--) {
			auto obj = player->GetObject(i);
			if (obj != nullptr && obj->GetCustomVariableInt<"attributeoverride">() == 0) {
				ChangeBuildingProperties(player, obj);
			}
		}
		DETOUR_MEMBER_CALL(info);
	}

	class SentryWeaponModule : public EntityModule
	{
	public:
        SentryWeaponModule() {}
        SentryWeaponModule(CBaseEntity *entity) : EntityModule(entity) {}
        virtual ~SentryWeaponModule() {
			if (weapon != nullptr)
				weapon->Remove();
		}

		CHandle<CTFWeaponBaseGun> weapon;
		std::string lastWeaponName;
	};

	CBaseEntity *ShootSentryWeaponProjectile(CBaseObject *sentry, CTFPlayer *player, const char *weaponName, const char *moduleName, const Vector &vecOrigin, const QAngle &vecAngles)
	{
		bool tempPlayer = false;
		auto sentryTeam = sentry->GetTeamNumber() != 0 ? sentry->GetTeamNumber() : TF_TEAM_BLUE;

		auto mod = sentry->GetOrCreateEntityModule<SentryWeaponModule>(moduleName);
		CTFWeaponBaseGun *weapon = mod->weapon;
		// Create new weapon if not created yet
		if (weapon == nullptr || mod->lastWeaponName != weaponName) {
			if (weapon != nullptr) {
				weapon->Remove();
			}
			auto item = CreateItemByName(nullptr, weaponName);
			weapon = rtti_cast<CTFWeaponBaseGun *>(item);
			if (weapon != nullptr) {
				mod->lastWeaponName = weaponName;
				mod->weapon = weapon;
			}
			else if (item != nullptr) {
				item->Remove();
			}
		}
		if (weapon == nullptr) return nullptr;
			
		// Fire the weapon

		// Move the owner player to mimic position
		auto projectile = Mod::Common::Weapon_Shoot::FireWeapon(sentry, weapon, vecOrigin, vecAngles, false, false, 0, sentry->GetCustomVariableBool<"allowplayerattributes">());
		if (projectile != nullptr) {
			projectile->SetCustomVariable("customsentryweapon", Variant(true));
		}
		return projectile;
	}

	// Must use CTFBaseRocket::Create because CTFProjectile_SentryRocket::Create is inlined in x64
	RefCount rc_CTFBaseRocket_Create;
	DETOUR_DECL_STATIC(CTFBaseRocket *, CTFBaseRocket_Create, CBaseEntity *launcher, const char *classname, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner)
	{
		SCOPED_INCREMENT(rc_CTFBaseRocket_Create);
		auto object = ToBaseObject(pOwner);
		bool isSentryRocket = rc_CObjectSentrygun_FireRocket && object != nullptr && rc_CTFBaseRocket_Create <= 1;
		if (isSentryRocket) {
			auto weaponName = GetBuildingAttributeString<"rocketweapon", "sentry_rocket_weapon">(object);
			if (weaponName[0] != '\0') {
				auto player = object->GetBuilder();
				auto proj = ShootSentryWeaponProjectile(object, player, weaponName, "weaponrocket", vecOrigin, vecAngles);
        		pOwner->FireCustomOutput<"onshootweaponrocket">(proj != nullptr ? proj : pOwner, pOwner, Variant());
				return nullptr;
			}
		}
		auto ret = DETOUR_STATIC_CALL(launcher, classname, vecOrigin, vecAngles, pOwner);
		sentry_gun_rocket = isSentryRocket ? ret : nullptr;
		return ret;
	}

	void ChangeBuildingProperties(CTFPlayer *player, CBaseObject *obj);

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CTFPlayer_AddObject, CBaseObject *object)
	{
        DETOUR_MEMBER_CALL(object);
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_RemoveObject, CBaseObject *object)
	{
        DETOUR_MEMBER_CALL(object);
    }

    DETOUR_DECL_MEMBER(float, CObjectDispenser_GetHealRate)
	{
        auto ret = DETOUR_MEMBER_CALL();
		auto dispenser = reinterpret_cast<CObjectDispenser *>(this);
		ret *= GetBuildingAttributeFloat<"ratemult">(dispenser, "mult_dispenser_rate", true);
		return ret;
    }

    DETOUR_DECL_MEMBER(void, CObjectTeleporter_TeleporterThink)
	{
		auto teleporter = reinterpret_cast<CObjectTeleporter *>(this);
		int prestate = teleporter->m_iState;
		bool restore = false;
		if (teleporter->m_iTeleportType == 2 && GetBuildingAttributeInt<"bidirectional">(teleporter, "bidirectional_teleport", true)) {
			teleporter->m_iTeleportType = 1;
			restore = true;
		}
        DETOUR_MEMBER_CALL();
		if (prestate == 3 && teleporter->m_iState == 6) {
			float rechargeDurationPre = teleporter->m_flCurrentRechargeDuration;
			float rate = GetBuildingAttributeFloat<"rechargeratemult">(teleporter, "mult_teleporter_recharge_rate", true);
			teleporter->m_flCurrentRechargeDuration *= rate;
			teleporter->m_flRechargeTime += rechargeDurationPre * (rate - 1);
		}
		if (restore) {
			teleporter->m_iTeleportType = 2;
		}
    }

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_StartUpgrading)
	{
		auto obj = reinterpret_cast<CObjectSentrygun *>(this);
		CTFPlayer *owner = obj->GetBuilder();

		DETOUR_MEMBER_CALL();
		obj->m_iMaxAmmoShells *= GetBuildingAttributeFloat<"ammomult">(obj, "mvm_sentry_ammo", true);
		if (!obj->m_bCarryDeploy) {
			obj->m_iAmmoShells = obj->m_iMaxAmmoShells;
		}
		obj->m_iAmmoRockets = obj->m_iMaxAmmoRockets;
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_Spawn)
	{
		auto obj = reinterpret_cast<CObjectSentrygun *>(this);

		DETOUR_MEMBER_CALL();
		obj->m_iMaxAmmoShells *= GetBuildingAttributeFloat<"ammomult">(obj, "mvm_sentry_ammo", true);
		obj->m_iMaxAmmoRockets *= GetBuildingAttributeFloat<"ammomult">(obj, "mvm_sentry_ammo", true);
		obj->m_iMaxAmmoRockets *= GetBuildingAttributeFloat<"rocketammomult">(obj, "mult_sentry_rocket_ammo", false);
		obj->m_iAmmoShells = obj->m_iMaxAmmoShells;
		obj->m_iAmmoRockets = obj->m_iMaxAmmoRockets;
	}

	DETOUR_DECL_MEMBER(void, CObjectTeleporter_TeleporterSend, CTFPlayer *player)
	{
		auto obj = reinterpret_cast<CObjectTeleporter *>(this);

		DETOUR_MEMBER_CALL(player);
		if (player != nullptr && obj->GetCustomVariableInt<"speedboost">() != 0) {
			player->m_Shared->AddCond(TF_COND_SPEED_BOOST, 4.0f);
		}
	}

	VHOOK_DECL(void, CObjectSentrygun_FireBullets, FireBulletsInfo_t &info)
	{
		auto obj = reinterpret_cast<CObjectSentrygun *>(this);
		auto player = ToTFPlayer(info.m_pAttacker);
		auto weaponName = GetBuildingAttributeString<"bulletweapon", "sentry_bullet_weapon">(obj);
		if (weaponName[0] != '\0') {
			QAngle ang;
			VectorAngles(info.m_vecDirShooting, ang);
			auto proj = ShootSentryWeaponProjectile(obj, player, weaponName, "weaponbullet", info.m_vecSrc, ang);
        	obj->FireCustomOutput<"onshootweaponbullet">(proj != nullptr ? proj : proj, obj, Variant());
			return;
		}
		info.m_flDamage *= GetBuildingAttributeFloat<"damagemult">(obj, "mult_engy_sentry_damage", true);
		VHOOK_CALL(info);
	}

	DETOUR_DECL_MEMBER(float, CObjectDispenser_GetDispenserRadius)
	{
		auto ret = DETOUR_MEMBER_CALL();
		auto dispenser = reinterpret_cast<CObjectDispenser *>(this);
		ret *= GetBuildingAttributeFloat<"radiusmult">(dispenser, "mult_dispenser_radius", true);
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_SentryRotate)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
		sentry->m_flSentryRange *= GetBuildingAttributeFloat<"rangemult">(sentry, "mult_sentry_range", true);
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_Attack)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
		sentry->m_flSentryRange *= GetBuildingAttributeFloat<"rangemult">(sentry, "mult_sentry_range", true);
		float nextAttackPre = sentry->m_flNextAttack;
		DETOUR_MEMBER_CALL();
		if (nextAttackPre < sentry->m_flNextAttack) {
			float rate = GetBuildingAttributeFloat<"fireratemult">(sentry, "mult_sentry_firerate", true);
			sentry->m_flFireRate *= rate;
			if (rate != 1.0f) {
				sentry->m_flNextAttack = gpGlobals->curtime + ((sentry->m_iUpgradeLevel == 1 ? 0.2f : 0.1f) * sentry->m_flFireRate);
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_FoundTarget, CBaseEntity *entity, const Vector &vecSoundCenter, bool noSound)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
		float nextAttackPre = sentry->m_flNextAttack;
		float nextAttackRocket = sentry->m_flNextRocketAttack;
		DETOUR_MEMBER_CALL(entity, vecSoundCenter, noSound);
		float rate = GetBuildingAttributeFloat<"fireratemult">(sentry, "mult_sentry_firerate", false);
		if (rate > 1) {
			sentry->m_flNextAttack = nextAttackPre;
		}
		float rateRocket = GetBuildingAttributeFloat<"rocketfireratemult">(sentry, "mult_firerocket_rate", false);
		if (rateRocket > 1) {
			sentry->m_flNextRocketAttack = nextAttackRocket;
		}
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_SetModel, const char *model)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
		const char *newModelPrefix = GetBuildingAttributeString<"sentrymodelprefix", "custom_sentry_model">(sentry);
		std::string oldModelPrefix = "models/buildables/sentry"s;
		std::string newModel;
		if (newModelPrefix[0] != '\0') {
			newModel = newModelPrefix;
			if (!StringEndsWith(newModelPrefix, ".mdl")) {
				if (oldModelPrefix + "1_blueprint.mdl"s == model) {
					newModel += "1_blueprint.mdl";
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = model;
					}
				}
				else if (oldModelPrefix + "1.mdl"s == model) {
					newModel += "1.mdl"s;
				}
				else if (oldModelPrefix + "1_heavy.mdl"s == model) {
					newModel += "1_heavy.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "1.mdl"s;
					}
				}
				else if (oldModelPrefix + "2.mdl"s == model) {
					newModel += "2.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "1.mdl"s;
					}
				}
				else if (oldModelPrefix + "2_heavy.mdl"s == model) {
					newModel += "2_heavy.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "2.mdl"s;
						if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
							newModel = newModelPrefix + "1.mdl"s;
						}
					}
				}
				else if (oldModelPrefix + "3.mdl"s == model) {
					newModel += "3.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "1.mdl"s;
					}
				}
				else if (oldModelPrefix + "3_heavy.mdl"s == model) {
					newModel += "3_heavy.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "3.mdl"s;
						if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
							newModel = newModelPrefix + "1.mdl"s;
						}
					}
				}
			}
			model = newModel.c_str();
			CBaseEntity::PrecacheModel(model);
		}
		DETOUR_MEMBER_CALL(model);
	}

	DETOUR_DECL_MEMBER(void, CObjectTeleporter_SetModel, const char *model)
	{
		auto tele = reinterpret_cast<CObjectTeleporter *>(this);
		const char *newModelPrefix = GetBuildingAttributeString<"teleportermodelprefix", "custom_teleporter_model">(tele);
		std::string oldModelPrefix = "models/buildables/teleporter"s;
		std::string newModel;
		if (newModelPrefix[0] != '\0') {
			newModel = newModelPrefix;
			if (!StringEndsWith(newModelPrefix, ".mdl")) {
				if (oldModelPrefix + "_blueprint_enter.mdl"s == model) {
					newModel += "_blueprint_enter.mdl";
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = model;
					}
				}
				else if (oldModelPrefix + "_blueprint_exit.mdl"s == model) {
					newModel += "_blueprint_exit.mdl";
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = model;
					}
				}
				else if (oldModelPrefix + "_light.mdl"s == model) {
					newModel += "_light.mdl"s;
				}
				else if (oldModelPrefix + ".mdl"s == model) {
					newModel += ".mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "_light.mdl"s;
					}
				}
			}
			model = newModel.c_str();
			CBaseEntity::PrecacheModel(model);
		}
		DETOUR_MEMBER_CALL(model);
	}

	DETOUR_DECL_MEMBER(void, CObjectDispenser_SetModel, const char *model)
	{
		auto disp = reinterpret_cast<CObjectDispenser *>(this);
		const char *newModelPrefix = GetBuildingAttributeString<"dispensermodelprefix", "custom_dispenser_model">(disp);
		std::string oldModelPrefix = "models/buildables/dispenser"s;
		std::string newModel;
		if (newModelPrefix[0] != '\0') {
			newModel = newModelPrefix;
			
			if (!StringEndsWith(newModelPrefix, ".mdl")) {
				if (oldModelPrefix + "_blueprint.mdl"s == model) {
					newModel += "_blueprint.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = model;
					}
				}
				else if (oldModelPrefix + "_light.mdl"s == model) {
					newModel += "_light.mdl"s;
				}
				else if (oldModelPrefix + ".mdl"s == model) {
					newModel += ".mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "_light.mdl"s;
					}
				}
				else if (oldModelPrefix + "_lvl2_light.mdl"s == model) {
					newModel += "_lvl2_light.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "_light.mdl"s;
					}
				}
				else if (oldModelPrefix + "_lvl2.mdl"s == model) {
					newModel += "_lvl2.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "_lvl2_light.mdl"s;
						if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
							newModel = newModelPrefix + "_light.mdl"s;
						}
					}
				}
				else if (oldModelPrefix + "_lvl3_light.mdl"s == model) {
					newModel += "_lvl3_light.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "_light.mdl"s;
					}
				}
				else if (oldModelPrefix + "_lvl3.mdl"s == model) {
					newModel += "_lvl3.mdl"s;
					if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
						newModel = newModelPrefix + "_lvl3_light.mdl"s;
						if (!filesystem->FileExists(newModel.c_str(), "GAME")) {
							newModel = newModelPrefix + "_light.mdl"s;
						}
					}
				}
			}
			model = newModel.c_str();
			CBaseEntity::PrecacheModel(model);
		}
		DETOUR_MEMBER_CALL(model);
	}


    VHOOK_DECL(void, CObjectSentrygun_UpdateOnRemove)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
		auto modbullet = sentry->GetEntityModule<SentryWeaponModule>("weaponbullet");
		if (modbullet != nullptr && modbullet->weapon != nullptr) {
			modbullet->weapon->Remove();
		}
		auto modrocket = sentry->GetEntityModule<SentryWeaponModule>("weaponrocket");
		if (modrocket != nullptr && modrocket->weapon != nullptr) {
			modrocket->weapon->Remove();
		}
		VHOOK_CALL();
	}

	DETOUR_DECL_MEMBER(void, CObjectSentrygun_EmitSentrySound, const char *sound)
	{
		auto sentry = reinterpret_cast<CObjectSentrygun *>(this);
		if (strcmp(sound, "Building_Sentrygun.FireRocket") == 0) {
			auto rocketSound = GetBuildingAttributeString<"rocketweapon", "sentry_rocket_weapon">(sentry);
			if (rocketSound[0] != '\0') {
				return;
			}
		}
		else if (StringStartsWith(sound, "Building_Sentrygun.Fire") || StringStartsWith(sound, "Building_Sentrygun.Shaft")) {
			auto bulletSound = GetBuildingAttributeString<"bulletweapon", "sentry_bullet_weapon">(sentry);
			if (bulletSound[0] != '\0') {
				return;
			}
		}
		DETOUR_MEMBER_CALL(sound);
	}

	bool OverrideGib(CTFPlayer *player, const CTakeDamageInfo& info, bool &result)
	{
		int alwaysGib = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, alwaysGib, always_gib);
		if (alwaysGib != 0) {
			result = true;
			return true;
		}

		int neverGib = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, neverGib, never_gib);
		if (neverGib != 0) { 
			result = false;
			return true;
		}
		
		if (info.GetWeapon() != nullptr) {
			int shouldGib = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(info.GetWeapon(), shouldGib, weapon_always_gib);
			if (shouldGib != 0) { 
				result = true;
				return true;
			}

			int shouldNotGib = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(info.GetWeapon(), shouldNotGib, weapon_never_gib);
			if (shouldNotGib != 0) { 
				result = false;
				return true;
			}
		}
		return false;
	}

	DETOUR_DECL_MEMBER(bool, CTFBot_ShouldGib, const CTakeDamageInfo& info)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		bool result = false;
		if (OverrideGib(bot, info, result)) return result;
		return DETOUR_MEMBER_CALL(info);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ShouldGib, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		bool result = false;
		if (OverrideGib(player, info, result)) return result;
		return DETOUR_MEMBER_CALL(info);
	}
	
	std::string sapperModelName;
	DETOUR_DECL_MEMBER(const char *, CObjectSapper_GetSapperModelName, int nModel, const char *pchModelName)
	{
		auto sapper = reinterpret_cast<CObjectSapper *>(this);
		
		if (sapper->GetBuilder() != nullptr && sapper->GetBuilder()->IsPlayer()) {
			//const char *value = entity->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, sapper->GetBuilder(), PStrT<"custom_sapper_model">()).ToCStr();
			GET_STRING_ATTRIBUTE_NO_CACHE(sapper->GetBuilder(), custom_sapper_model, model);
			if (model != nullptr) {
				if (StringEndsWith(model, ".mdl", false)) {
					return model;
				}
				if (nModel > 0) {
					sapperModelName = fmt::format("{}_placement.mdl", model);
					return sapperModelName.c_str();
				}
				else {
					sapperModelName = fmt::format("{}_placed.mdl", model);
					return sapperModelName.c_str();
				}
			}
		}
		return DETOUR_MEMBER_CALL(nModel, pchModelName);
	}
	
	DETOUR_DECL_MEMBER(const char *, CObjectSapper_GetSapperSoundName)
	{
		auto sapper = reinterpret_cast<CObjectSapper *>(this);
		
		if (sapper->GetBuilder() != nullptr) {
			GET_STRING_ATTRIBUTE(sapper->GetBuilder(), custom_sapper_sound, sound);
			if (sound != nullptr) {
				sapper->SetCustomVariable("customsound", Variant(AllocPooledString(sound)));
				PrecacheSound(sound);
				return sound;
			}
		}
		return DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(const char *, CObjectSapper_UpdateOnRemove)
	{
		auto sapper = reinterpret_cast<CObjectSapper *>(this);
		sapper->StopSound(sapper->GetCustomVariable<"customsound">(""));
		return DETOUR_MEMBER_CALL();
	}

	RefCount rc_CBaseObject_FindSnapToBuildPos;
	DETOUR_DECL_MEMBER(bool, CBaseObject_FindSnapToBuildPos, CBaseObject *pObjectOverride)
	{
		auto me = reinterpret_cast<CBaseObject *>(this);
		int ally = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(me->GetBuilder(), ally, sapper_sap_allies);
		if (ally != 0) {
			Vector vecNearestBuildPoint = vec3_origin;
			float flNearestPoint = 9999;
			bool found = false;
			CTFPlayer *victim = nullptr;
			ForEachTFPlayerOnTeam(TFTeamMgr()->GetTeam(me->GetBuilder()->GetTeamNumber()), [&](CTFPlayer *playerInTeam){
				if (playerInTeam != me->GetBuilder() && me->FindBuildPointOnPlayer(playerInTeam, me->GetBuilder(), flNearestPoint, vecNearestBuildPoint)) {
					victim = playerInTeam;
					found = true;
				}
			});
			if (!found) {
				me->AddEffects(EF_NODRAW);
			}
			else {
				me->RemoveEffects(EF_NODRAW);
				me->AttachObjectToObject(victim, 0, vecNearestBuildPoint);
				me->m_vecBuildOrigin = vecNearestBuildPoint;
			}
			return found;
		}
		SCOPED_INCREMENT_IF(rc_CBaseObject_FindSnapToBuildPos, ally != 0);
		return DETOUR_MEMBER_CALL(pObjectOverride);
	}

	DETOUR_DECL_MEMBER(CTFTeam *, CTFPlayer_GetOpposingTFTeam)
	{
		auto me = reinterpret_cast<CTFPlayer *>(this);
		if (rc_CBaseObject_FindSnapToBuildPos) {
			return TFTeamMgr()->GetTeam(me->GetTeamNumber());
		}
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CObjectSapper_IsValidRoboSapperTarget, CTFPlayer *player)
	{
		auto builder = reinterpret_cast<CObjectSapper *>(this)->GetBuilder();
		int ally = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(builder, ally, sapper_sap_allies);
		if (ally) {
			return player->GetTeamNumber() == builder->GetTeamNumber() && player->IsAlive();
		}
		return DETOUR_MEMBER_CALL(player);
	}

#define DETOUR_DECL_MEMBER_PROJ_DAMAGE_MULT(className) \
\
	DETOUR_DECL_MEMBER(float, className##_GetDamage) \
	{ \
		auto me = reinterpret_cast<CBaseProjectile *>(this); \
		return DETOUR_MEMBER_CALL() * GetDamageMult(me); \
	} \
	
	DETOUR_DECL_MEMBER_PROJ_DAMAGE_MULT(CTFProjectile_Cleaver);
	DETOUR_DECL_MEMBER_PROJ_DAMAGE_MULT(CTFProjectile_ThrowableRepel);
	DETOUR_DECL_MEMBER_PROJ_DAMAGE_MULT(CTFProjectile_ThrowableBrick);
	DETOUR_DECL_MEMBER_PROJ_DAMAGE_MULT(CTFProjectile_ThrowableBreadMonster);
	DETOUR_DECL_MEMBER_PROJ_DAMAGE_MULT(CTFStunBall);

	VHOOK_DECL(bool, PlayerLocomotion_IsOnGround, const CCommand& args)
	{
        auto loco = reinterpret_cast<ILocomotion *>(this);
		if (loco->GetBot()->GetEntity()->GetMoveType() == MOVETYPE_NOCLIP) return true;
        return VHOOK_CALL(args);
    }

	DETOUR_DECL_MEMBER(void, CGameMovement_FullNoClipMove, float speed, float accel)
	{
        auto movement = reinterpret_cast<CGameMovement *>(this);
		float noclip = GetFastAttributeFloat(movement->player, 0.0, NO_CLIP);
		if (noclip != 0) {
			if (noclip == 1) {
				speed = movement->player->MaxSpeed() / 300;
			}
			else {
				speed = noclip / 300;
			}
		}
        DETOUR_MEMBER_CALL(speed, accel);
    }
	
	DETOUR_DECL_MEMBER(void, CTFPlayerMove_SetupMove, CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move)
	{
		bool restoreCond = false;
		if ((ucmd->buttons & IN_JUMP) || (player->GetFlags() & FL_DUCKING)) {
			auto tfplayer = ToTFPlayer(player);
			if (tfplayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_HEAVYWEAPONS && tfplayer->m_Shared->GetCondData().InCond(TF_COND_AIMING)) {
				int minigunFullMovement = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(tfplayer, minigunFullMovement, minigun_full_movement);
				if (minigunFullMovement != 0) {
					tfplayer->m_Shared->GetCondData().RemoveCondBit(TF_COND_AIMING);
					restoreCond = true;
				}
			}
		}

        DETOUR_MEMBER_CALL(player, ucmd, pHelper, move);
		
		if (restoreCond) {
			auto tfplayer = ToTFPlayer(player);
			tfplayer->m_Shared->GetCondData().AddCondBit(TF_COND_AIMING);
		}
    }

	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_Deploy)
	{
		auto wep = reinterpret_cast<CTFWeaponBase *>(this);
		GET_STRING_ATTRIBUTE(wep, add_attributes_when_active, attributes);
		AddOnActiveAttributes(wep, attributes);
		return DETOUR_MEMBER_CALL();
	}

	bool CanSapBuilding(CBaseObject *obj) {
		return GetBuildingAttributeInt<"cannotbesapped">(obj, "buildings_cannot_be_sapped", false) == 0;
	}

	DETOUR_DECL_MEMBER(bool, CBaseObject_FindNearestBuildPoint, CBaseEntity *pEntity, CBasePlayer *pBuilder, float &flNearestPoint, Vector &vecNearestBuildPoint, bool bIgnoreChecks)
	{
		auto wep = reinterpret_cast<CTFWeaponBase *>(this);

		auto obj = ToBaseObject(pEntity);
		if (obj != nullptr) {
			if (!CanSapBuilding(obj)) {
				return false;
			}
		}

		return DETOUR_MEMBER_CALL(pEntity, pBuilder, flNearestPoint, vecNearestBuildPoint, bIgnoreChecks);
	}

	DETOUR_DECL_MEMBER(bool,CTFBotDeliverFlag_UpgradeOverTime, CTFBot *bot)
	{
		int cannotUpgrade = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(bot, cannotUpgrade, cannot_upgrade_bomb);
		if (cannotUpgrade) return false;

        return DETOUR_MEMBER_CALL(bot);
	}

	IForward *viewmodels_enabled_forward;
	IForward *viewmodels_toggle_forward;
	std::string disallowed_viewmodel_path;
	bool players_informed_about_viewmodel[ABSOLUTE_PLAYER_LIMIT];
	bool players_viewmodel_disallowed[ABSOLUTE_PLAYER_LIMIT];
	bool players_viewmodel_informed_about_disallowed[ABSOLUTE_PLAYER_LIMIT];
	bool IsCustomViewmodelAllowed(CTFPlayer *player) {
		if (players_viewmodel_disallowed[player->entindex()]) {
			if (!players_viewmodel_informed_about_disallowed[player->entindex()]) {
				PrintToChatSM(player, 1, "%t\n", "Custom hand models disabled");
				players_viewmodel_informed_about_disallowed[player->entindex()] = true;
			}
			return false;
		}
		if (players_informed_about_viewmodel[player->entindex()]) return true;

		players_informed_about_viewmodel[player->entindex()] = true;
		char pathinfo[512];
		snprintf(pathinfo, 512, "%si%llu", disallowed_viewmodel_path.c_str(), player->GetSteamID().ConvertToUint64());
		auto file = fopen(pathinfo, "w");
		if (file) {
			fclose(file);
		}

		PrintToChatSM(player, 1, "%t\n", "Custom hand models notify");

		return true;
	}

	DETOUR_DECL_MEMBER(void, CServerGameClients_ClientPutInServer, edict_t *edict, const char *playername)
	{
        DETOUR_MEMBER_CALL(edict, playername);
		char path[512];
		auto player = ((CTFPlayer*) edict->GetUnknown());
		snprintf(path, 512, "%s%llu", disallowed_viewmodel_path.c_str(), player->GetSteamID().ConvertToUint64());
		char pathinfo[512];
		snprintf(pathinfo, 512, "%si%llu", disallowed_viewmodel_path.c_str(), player->GetSteamID().ConvertToUint64());

		cell_t result = access(path, F_OK) != 0;
		viewmodels_enabled_forward->PushCell(edict->m_EdictIndex);
		viewmodels_enabled_forward->Execute(&result);
        players_viewmodel_disallowed[edict->m_EdictIndex] = !result;
        players_informed_about_viewmodel[edict->m_EdictIndex] = access(path, F_OK) == 0;
		players_viewmodel_informed_about_disallowed[edict->m_EdictIndex] = true;
    }

	DETOUR_DECL_MEMBER(int, CTFPlayer_GetMaxAmmo, int ammoIndex, int classIndex)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		int ammoValueChange = 0;
		if (ammoIndex == TF_AMMO_PRIMARY) {
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, ammoValueChange, max_ammo_primary_additive);
		}
		else if (ammoIndex == TF_AMMO_SECONDARY) {
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, ammoValueChange, max_ammo_secondary_additive);
		}
		if (ammoValueChange != 0) {
			GetPlayerClassData(classIndex == -1 ? player->GetPlayerClass()->GetClassIndex() : classIndex)->m_aAmmoMax[ammoIndex] += ammoValueChange;
		}
		auto ret = DETOUR_MEMBER_CALL(ammoIndex, classIndex);
		if (ammoValueChange != 0) {
			GetPlayerClassData(classIndex == -1 ? player->GetPlayerClass()->GetClassIndex() : classIndex)->m_aAmmoMax[ammoIndex] -= ammoValueChange;
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBuilder_StartPlacement)
	{
		DETOUR_MEMBER_CALL();
		auto builder = reinterpret_cast<CTFWeaponBuilder *>(this);

		string_t attrTest = NULL_STRING;
		switch (builder->m_iObjectType) {
			case OBJ_SENTRYGUN: attrTest = PStrT<"sentry_toolbox_model">(); break;
			case OBJ_DISPENSER: attrTest = PStrT<"dispenser_toolbox_model">(); break;
			case OBJ_TELEPORTER: attrTest = PStrT<"teleporter_toolbox_model">(); break;
		}
		
		if (attrTest != NULL_STRING) {
		 	auto model = builder->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, builder->GetTFPlayerOwner(), attrTest).ToCStr();
			
			auto kv = builder->GetItem()->GetStaticData()->GetKeyValues()->FindKey("used_by_classes");
			if (kv != nullptr && kv->FindKey("spy") != nullptr && (model == nullptr || model[0] == '\0')) {
				model = "models/weapons/c_models/c_toolbox/c_toolbox.mdl";
			}
		 	if (model != nullptr && model[0] != '\0') {
		 		builder->SetCustomVariable("custombuildingviewmodel", Variant(true));
				auto oldCustomModel = builder->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, builder, PStrT<"custom_item_model">());
		 		builder->SetCustomVariable("custombuildingviewmodelrestore", Variant(oldCustomModel));
				builder->GetItem()->GetAttributeList().AddStringAttribute(GetItemSchema()->GetAttributeDefinitionByName("custom item model"), model);
		// 		builder->SetCustomViewModel(model);
		// 		builder->m_iViewModelIndex = CBaseEntity::PrecacheModel(model);
		// 		builder->SetModel(model);
			}
		}
		
	}
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBuilder_StopPlacement)
	{
		DETOUR_MEMBER_CALL();
		auto builder = reinterpret_cast<CTFWeaponBuilder *>(this);

		if (builder->GetCustomVariableBool<"custombuildingviewmodel">()) {
			builder->SetCustomVariable("custombuildingviewmodel", Variant(false));
			auto modelRestore = builder->GetCustomVariable<"custombuildingviewmodelrestore">();
			if (modelRestore != nullptr) {
				builder->GetItem()->GetAttributeList().AddStringAttribute(GetItemSchema()->GetAttributeDefinitionByName("custom item model"), modelRestore);
			}
			builder->SetCustomVariable("custombuildingviewmodel", Variant(false));
		// 	builder->SetCustomViewModel("");
		// 	builder->SetViewModel();
		// 	builder->m_iViewModelIndex = CBaseEntity::PrecacheModel(builder->GetViewModel());
		// 	builder->SetModel(builder->GetViewModel());
		// 	builder->SetCustomVariable("custombuildingviewmodel", Variant(false));
		}
	}

	VHOOK_DECL(Activity, CTFWeaponBuilder_TranslateViewmodelHandActivityInternal, Activity base)
	{
		auto ret = VHOOK_CALL(base);
		//Msg("Base %s Res %s\n", CAI_BaseNPC::GetActivityName(base), CAI_BaseNPC::GetActivityName(ret));
		return ret;
	}

	VHOOK_DECL(bool, CTFWeaponBuilder_SendWeaponAnim, Activity act)
	{
		auto builder = reinterpret_cast<CTFWeaponBuilder *>(this);
		if (builder->m_iObjectType != OBJ_ATTACHMENT_SAPPER) {
			auto kv = builder->GetItem()->GetStaticData()->GetKeyValues()->FindKey("used_by_classes");
			if (kv != nullptr && kv->FindKey("spy") != nullptr) {
				if (act == CAI_BaseNPC::GetActivityID("ACT_VM_IDLE")) {
					act = (Activity) CAI_BaseNPC::GetActivityID("ACT_ENGINEER_BLD_VM_IDLE");
				}
				if (act == CAI_BaseNPC::GetActivityID("ACT_VM_DRAW")) {
					act = (Activity) CAI_BaseNPC::GetActivityID("ACT_ENGINEER_BLD_VM_DRAW");
				}
			}
		}
		auto ret = VHOOK_CALL(act);
		return ret;
	}
	
	
	DETOUR_DECL_MEMBER(int, CEconItemView_GetAnimationSlot)
	{
		auto ret = DETOUR_MEMBER_CALL();
		//Msg("Slot %d\n", ret);
		return ret;
	}
	
	DETOUR_DECL_MEMBER(int, CTFWeaponBase_GetViewModelWeaponRole)
	{
		auto ret = DETOUR_MEMBER_CALL();
		//Msg("SlotRole %d\n", ret);
		return ret;
	}
	
	DETOUR_DECL_MEMBER(Activity, CEconItemDefinition_GetActivityOverride, int team, Activity base)
	{
		auto ret = DETOUR_MEMBER_CALL(team, base);
		//Msg("ActivityOverride %d %d\n", base, ret);
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CTFFlameManager_OnCollide, CBaseEntity* entity, int value)
	{
		auto flame = reinterpret_cast<CBaseEntity *>(this);
		SCOPED_INCREMENT_IF(rc_CTFFlameManager_OnCollide, flame->GetOwnerEntity() != nullptr && HasAllowFriendlyFire(flame->GetOwnerEntity(), flame->GetOwnerEntity()->GetOwnerEntity()));

		DETOUR_MEMBER_CALL(entity, value);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_Heal, CBaseEntity *pHealer, float flAmount, float flOverhealBonus, float flOverhealDecayMult, bool bDispenserHeal, CTFPlayer *pHealScorer)
	{
		if (rc_CWeaponMedigun_StartHealingTarget) { 
			flAmount = 0.01f;
			if (ToTFPlayer(pHealer) != nullptr && ToTFPlayer(pHealer)->GetActiveTFWeapon() != nullptr) {
				auto attackModule = ToTFPlayer(pHealer)->GetActiveTFWeapon()->GetEntityModule<MedigunAttackModule>("medigunattack");
				if (attackModule != nullptr) {
					attackModule->maxOverheal = flOverhealBonus;
				}
			}
		}
		if (rc_CWeaponMedigun_StartHealingTarget_Drain && ToTFPlayer(pHealer) != nullptr && ToTFPlayer(pHealer)->GetActiveTFWeapon() != nullptr) {
			auto mod = ToTFPlayer(pHealer)->GetActiveTFWeapon()->GetEntityModule<MedigunDrainModule>("medigundrain");
			if (mod != nullptr) {
				mod->maxOverheal = flOverhealBonus;
			}
		}
		DETOUR_MEMBER_CALL(pHealer, flAmount, flOverhealBonus, flOverhealDecayMult, bDispenserHeal, pHealScorer);
	}

	DETOUR_DECL_MEMBER(void, CWeaponMedigun_SecondaryAttack)
	{
		auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
		auto owner = medigun->GetTFPlayerOwner();
		CBaseEntity *healRestore = nullptr;
		if (owner != nullptr && medigun->GetHealTarget() != nullptr && medigun->GetHealTarget()->GetTeamNumber() != owner->GetTeamNumber()) {
			float attacking = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(medigun, attacking, medigun_attack_enemy);
			if (attacking != 0) {
				healRestore = medigun->GetHealTarget();
				medigun->SetHealTarget(nullptr);
			}
		}
		DETOUR_MEMBER_CALL();
		if (healRestore != nullptr) {
			medigun->SetHealTarget(healRestore);
		}
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CWeaponMedigun_CycleResistType)
	{
		auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
		auto owner = medigun->GetTFPlayerOwner();
		CBaseEntity *healRestore = nullptr;
		if (owner != nullptr && medigun->GetHealTarget() != nullptr && medigun->GetHealTarget()->GetTeamNumber() != owner->GetTeamNumber()) {
			float attacking = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(medigun, attacking, medigun_attack_enemy);
			if (attacking != 0) {
				healRestore = medigun->GetHealTarget();
				medigun->SetHealTarget(nullptr);
			}
		}
		DETOUR_MEMBER_CALL();
		if (healRestore != nullptr) {
			medigun->SetHealTarget(healRestore);
		}
	}

	DETOUR_DECL_MEMBER(bool, CWeaponMedigun_FindAndHealTargets)
	{
		auto medigun = reinterpret_cast<CWeaponMedigun *>(this);
		auto owner = medigun->GetTFPlayerOwner();
		auto result = DETOUR_MEMBER_CALL();
		auto target = ToTFPlayer(medigun->GetHealTarget());
		if (result && owner != nullptr && target != nullptr && target->GetTeamNumber() == owner->GetTeamNumber()) {
			float drain = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(medigun, drain, medigun_self_drain);
			auto mod = medigun->GetEntityModule<MedigunDrainModule>("medigundrain");
			if (drain != 0 && mod != nullptr) {
				float heal = medigun->GetHealRate() * drain * gpGlobals->frametime * RemapValClamped( gpGlobals->curtime - target->m_flMvMLastDamageTime, 10, 15, 1.0, 3.0 ) + mod->healFraction;
				float maxbuff = target->GetMaxHealthForBuffing() * mod->maxOverheal;
				int healInt = heal;
				mod->healFraction = heal - healInt;
				if (healInt > 0 && target->GetHealth() < maxbuff * 0.98 && owner->GetHealth() > 1) {
					owner->SetHealth(Max(owner->GetHealth() - healInt, 1));
				}
				drain = Min(drain, 0.999f);
				if (owner->GetHealth() <= 1 && !mod->healingReduced) {
					mod->healingReduced = true;
					int healIndex = target->m_Shared->FindHealerIndex(owner);
					if (healIndex != -1) {
						target->m_Shared->m_aHealers.Get()[healIndex].flAmount *= (1.0f-drain);
					}
				}
				if (owner->GetHealth() > 1 && mod->healingReduced) {
					mod->healingReduced = false;
					int healIndex = target->m_Shared->FindHealerIndex(owner);
					if (healIndex != -1) {
						target->m_Shared->m_aHealers.Get()[healIndex].flAmount *= 1/(1.0f-drain);
					}
				}
			}
		}
		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseMelee_Smack)
	{
		auto weapon = reinterpret_cast<CTFWeaponBaseMelee*>(this);
		auto player = weapon->GetTFPlayerOwner();
		GET_STRING_ATTRIBUTE(weapon, fire_input_on_attack, input);
		FireInputAttribute(input, nullptr, Variant(), nullptr, player, weapon, -1.0f);
		ApplyPunch(player, weapon);
 		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(const char *, CTFPlayer_GetSceneSoundToken)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		int humanVoice = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, humanVoice, use_human_voice);
		if (humanVoice != 0) {
			return "";
		}
		int robotVoice = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, robotVoice, use_robot_voice);
		if (robotVoice != 0) {
			return "MVM_";
		}
		return DETOUR_MEMBER_CALL();
	}

	THINK_FUNC_DECL(SetActivityThink)
	{
		static int activityAttack = CAI_BaseNPC::GetActivityID("ACT_VM_PRIMARYATTACK");
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		weapon->SetIdealActivity(weapon->TranslateViewmodelHandActivityInternal((Activity) activityAttack));
	};

	bool idealActivitySet = false;
	GlobalThunkRW<CBasePlayer *> CBaseEntity_m_pPredictionPlayer("CBaseEntity::m_pPredictionPlayer");
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ItemPostFrame)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		auto player = weapon->GetTFPlayerOwner();
		if (player != nullptr && player->m_nButtons & IN_ATTACK2 && weapon->m_flNextPrimaryAttack <= gpGlobals->curtime && weapon->m_flNextSecondaryAttack <= gpGlobals->curtime) {
			
			float duration = GetFastAttributeFloat(weapon, 0.0f, ALT_FIRE_ATTACK);
			if (duration != 0.0f) {
				CBasePlayer *oldPredictPlayer = CBaseEntity_m_pPredictionPlayer;
				
				g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost = nullptr;
				CBaseEntity_m_pPredictionPlayer.GetRef() = nullptr;
				static int activityIdle = CAI_BaseNPC::GetActivityID("ACT_VM_IDLE");
				Activity activityPre = weapon->m_IdealActivity;
				{
					SCOPED_INCREMENT(rc_AltFireAttack);
					weapon->PrimaryAttack();
				}
				// To fix attack animation not playing, enforce idle animation for a tick before switching to attack
				if (idealActivitySet) {
					weapon->SetIdealActivity(weapon->TranslateViewmodelHandActivityInternal((Activity) activityIdle));
					THINK_FUNC_SET(weapon, SetActivityThink, gpGlobals->curtime);
					idealActivitySet = false;
				}
				weapon->m_flNextSecondaryAttack = gpGlobals->curtime + (weapon->m_flNextPrimaryAttack - gpGlobals->curtime) * duration;
				g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost = oldPredictPlayer;
				CBaseEntity_m_pPredictionPlayer.GetRef() = oldPredictPlayer;
			}
		}
		
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CBaseCombatWeapon_SetIdealActivity, Activity act)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		static int activityAttack = CAI_BaseNPC::GetActivityID("ACT_VM_PRIMARYATTACK");
		if (rc_AltFireAttack && weapon->m_IdealActivity == act && act == weapon->TranslateViewmodelHandActivityInternal((Activity) activityAttack)) {
			idealActivitySet = true;
		}
		return DETOUR_MEMBER_CALL(act);
	}
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ItemBusyFrame)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		auto player = weapon->GetTFPlayerOwner();

		bool restore = false;
		if (player != nullptr && player->m_nButtons & IN_ATTACK2) {
			if (GetFastAttributeFloat(weapon, 0.0f, ALT_FIRE_ATTACK) != 0.0f) {
				restore = !(player->m_nButtons & IN_ATTACK);
				player->m_nButtons |= IN_ATTACK;
			}
		}
		
		DETOUR_MEMBER_CALL();

		if (restore) {
			player->m_nButtons &= ~(IN_ATTACK);
		}
	}
	
	DETOUR_DECL_STATIC(CTFReviveMarker *, CTFReviveMarker_Create, CTFPlayer *player)
	{
		int noRevive = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, noRevive, no_revive);
		if (noRevive != 0) {
			return nullptr;
		}
		
		return DETOUR_STATIC_CALL(player);
	}

	DETOUR_DECL_STATIC(void, TE_PlayerAnimEvent, CBasePlayer *player, int anim, int data)
	{
		if (rc_AltFireAttack) {
			playerAnimSender = player;
		}
		DETOUR_STATIC_CALL(player, anim, data);
		playerAnimSender = nullptr;
	}
	
    DETOUR_DECL_MEMBER(void, CBaseEntity_Touch, CBaseEntity *other)
	{
		CTFWeaponBase *weaponAltFire = nullptr;
		CTFWeaponBase *weaponProjAttrs = nullptr;
        auto ent = reinterpret_cast<CBaseEntity *>(this);
		if (ent->GetExtraEntityData() != nullptr && (ent->GetCustomVariableBool<"isaltfire">() || ent->GetCustomVariable<"projattribs">() != nullptr)) {
			auto proj = rtti_cast<CBaseProjectile *>(ent);
			if (proj != nullptr && proj->GetOriginalLauncher() != nullptr) {
				auto weapon = rtti_cast<CTFWeaponBase *>(proj->GetOriginalLauncher());
				if (weapon != nullptr) {
					auto mod = weapon->GetEntityModule<AltFireAttributesModule>("altfireattrs");
					if (mod != nullptr && ent->GetCustomVariableBool<"isaltfire">() && !mod->active) {
						weaponAltFire = weapon;
						AddOnActiveAttributes(weaponAltFire, STRING(mod->attributes));
						mod->active = true;
					}
					if (proj->GetCustomVariable<"projattribs">() != nullptr && !weapon->GetCustomVariableBool<"projattribsapplied">()) {
						weaponProjAttrs = weapon;
						AddOnActiveAttributes(weaponProjAttrs, proj->GetCustomVariable<"projattribs">());
						weapon->SetCustomVariable<"projattribsapplied">(Variant(true));
					}
				}
			}
		}
		
		DETOUR_MEMBER_CALL(other);

		if (weaponAltFire != nullptr) {
			auto mod = weaponAltFire->GetEntityModule<AltFireAttributesModule>("altfireattrs");
			if (mod != nullptr) {
				RemoveOnActiveAttributes(weaponAltFire, STRING(mod->attributes));
				mod->active = false;
			}
		}
		if (weaponProjAttrs != nullptr) {
			RemoveOnActiveAttributes(weaponProjAttrs, ent->GetCustomVariable<"projattribs">());
			weaponProjAttrs->SetCustomVariable<"projattribsapplied">(Variant(false));
		}
    }
	
    DETOUR_DECL_MEMBER(void, CCollisionEvent_PostCollision, vcollisionevent_t *event)
	{
		CBaseEntity *grenade = nullptr;
		for (int i = 0; i < 2; i++) {
			auto entity = reinterpret_cast<CBaseEntity *>(event->pObjects[i]->GetGameData());
			if (entity != nullptr && (entity->GetExtraEntityData() != nullptr && (entity->GetCustomVariableBool<"isaltfire">() || entity->GetCustomVariable<"projattribs">() != nullptr))) {
				grenade = entity;
				break;
			}
		}
		CTFWeaponBase *weaponAltFire = nullptr;
		CTFWeaponBase *weaponProjAttrs = nullptr;

		if (grenade != nullptr) {
			auto proj = rtti_cast<CBaseProjectile *>(grenade);
			if (proj != nullptr && proj->GetOriginalLauncher() != nullptr) {
				auto weapon = rtti_cast<CTFWeaponBase *>(proj->GetOriginalLauncher());
				if (weapon != nullptr) {
					auto mod = weapon->GetEntityModule<AltFireAttributesModule>("altfireattrs");
					if (mod != nullptr && proj->GetCustomVariableBool<"isaltfire">() && !mod->active) {
						weaponAltFire = weapon;
						AddOnActiveAttributes(weaponAltFire, STRING(mod->attributes));
						mod->active = true;
					}
					if (proj->GetCustomVariable<"projattribs">() != nullptr && !weapon->GetCustomVariableBool<"projattribsapplied">()) {
						weaponProjAttrs = weapon;
						AddOnActiveAttributes(weaponProjAttrs, proj->GetCustomVariable<"projattribs">());
						weapon->SetCustomVariable<"projattribsapplied">(Variant(true));
					}
				}
			}
		}
        DETOUR_MEMBER_CALL(event);

		if (weaponAltFire != nullptr) {
			auto mod = weaponAltFire->GetEntityModule<AltFireAttributesModule>("altfireattrs");
			if (mod != nullptr) {
				RemoveOnActiveAttributes(weaponAltFire, STRING(mod->attributes));
				mod->active = false;
			}
		}
		if (weaponProjAttrs != nullptr) {
			RemoveOnActiveAttributes(weaponProjAttrs, grenade->GetCustomVariable<"projattribs">());
			weaponProjAttrs->SetCustomVariable<"projattribsapplied">(Variant(false));
		}
    }

    DETOUR_DECL_MEMBER(bool, CBaseCombatWeapon_HasPrimaryAmmo)
	{
		auto entity = reinterpret_cast<CBaseCombatWeapon *>(this);

		int modAmmoCount = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(entity, modAmmoCount, mod_ammo_per_shot);
		if (modAmmoCount > 1) {
			if (entity->GetMaxClip1() != -1 && entity->m_iClip1 >= modAmmoCount) {
				return true;
			}
			auto owner = ToTFPlayer(entity->GetOwner());
			if (owner != nullptr && owner->GetAmmoCount( entity->m_iPrimaryAmmoType ) >= modAmmoCount) {
				return true;
			}
		}

        return DETOUR_MEMBER_CALL();
    }

	DETOUR_DECL_MEMBER(void, CBasePlayer_PlayStepSound, Vector& vecOrigin, surfacedata_t *psurface, float fvol, bool force)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		GET_STRING_ATTRIBUTE(player, additional_step_sound, sound);
		if (sound != nullptr) {
			player->EmitSound(sound);
		}
		
		DETOUR_MEMBER_CALL(vecOrigin, psurface, fvol, force);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_OnTauntSucceeded, const char* pszSceneName, int iTauntIndex, int iTauntConcept)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		DETOUR_MEMBER_CALL(pszSceneName, iTauntIndex, iTauntConcept);
		GET_STRING_ATTRIBUTE(player, fire_input_on_taunt, input);
		FireInputAttribute(input, nullptr, Variant(AllocPooledString(pszSceneName)), nullptr, player, player, -1.0f);
	}

	DETOUR_DECL_MEMBER(void, CGameMovement_PlayerMove)
	{
		auto move = reinterpret_cast<CGameMovement *>(this);
		auto player = move->player;

		displacePlayers.clear();
		DETOUR_MEMBER_CALL();
		if (displacePlayers.empty()) return;

		for (auto &toucherH : displacePlayers) {
			CTFPlayer *toucher = toucherH;

			Vector bossGlobalMins = player->CollisionProp()->OBBMins() + player->GetAbsOrigin();
			Vector bossGlobalMaxs = player->CollisionProp()->OBBMaxs() + player->GetAbsOrigin();

			Vector playerGlobalMins = toucher->CollisionProp()->OBBMins() + toucher->GetAbsOrigin();
			Vector playerGlobalMaxs = toucher->CollisionProp()->OBBMaxs() + toucher->GetAbsOrigin();

			Vector newPlayerPos = toucher->GetAbsOrigin();


			if ( playerGlobalMins.x > bossGlobalMaxs.x ||
				playerGlobalMaxs.x < bossGlobalMins.x ||
				playerGlobalMins.y > bossGlobalMaxs.y ||
				playerGlobalMaxs.y < bossGlobalMins.y ||
				playerGlobalMins.z > bossGlobalMaxs.z ||
				playerGlobalMaxs.z < bossGlobalMins.z )
			{
				// no overlap
				return;
			}

			Vector toPlayer = toucher->WorldSpaceCenter() - player->WorldSpaceCenter();

			Vector overlap;
			float signX, signY, signZ;

			if ( toPlayer.x >= 0 )
			{
				overlap.x = bossGlobalMaxs.x - playerGlobalMins.x;
				signX = 1.0f;
			}
			else
			{
				overlap.x = playerGlobalMaxs.x - bossGlobalMins.x;
				signX = -1.0f;
			}

			if ( toPlayer.y >= 0 )
			{
				overlap.y = bossGlobalMaxs.y - playerGlobalMins.y;
				signY = 1.0f;
			}
			else
			{
				overlap.y = playerGlobalMaxs.y - bossGlobalMins.y;
				signY = -1.0f;
			}

			if ( toPlayer.z >= 0 )
			{
				overlap.z = bossGlobalMaxs.z - playerGlobalMins.z;
				signZ = 1.0f;
			}
			else
			{
				// don't push player underground
				overlap.z = 99999.9f; // playerGlobalMaxs.z - bossGlobalMins.z;
				signZ = -1.0f;
			}

			float bloat = 5.0f;

			if ( overlap.x < overlap.y )
			{
				if ( overlap.x < overlap.z )
				{
					// X is least overlap
					newPlayerPos.x += signX * ( overlap.x + bloat );
				}
				else
				{
					// Z is least overlap
					newPlayerPos.z += signZ * ( overlap.z + bloat );
				}
			}
			else if ( overlap.z < overlap.y )
			{
				// Z is least overlap
				newPlayerPos.z += signZ * ( overlap.z + bloat );
			}
			else
			{
				// Y is least overlap
				newPlayerPos.y += signY * ( overlap.y + bloat );
			}

			// check if new location is valid
			trace_t result;
			Ray_t ray;
			ray.Init( newPlayerPos, newPlayerPos, toucher->CollisionProp()->OBBMins(), toucher->CollisionProp()->OBBMaxs() );
			UTIL_TraceRay( ray, MASK_PLAYERSOLID, toucher, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

			if ( result.DidHit() )
			{
				// Trace down from above to find safe ground
				ray.Init( newPlayerPos + Vector( 0.0f, 0.0f, 32.0f ), newPlayerPos, toucher->CollisionProp()->OBBMins(), toucher->CollisionProp()->OBBMaxs() );
				UTIL_TraceRay( ray, MASK_PLAYERSOLID, toucher, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

				if ( result.startsolid )
				{
					// player was crushed against something
					toucher->TakeDamage( CTakeDamageInfo( player, player, nullptr, vec3_origin, vec3_origin, GetFastAttributeFloat(player, 0.0f, DISPLACE_TOUCHED_ENEMIES), DMG_CRUSH ) );
					return;
				}
				else
				{
					// Use the trace end position
					newPlayerPos = result.endpos;
				}
			}

			toucher->SetAbsOrigin( newPlayerPos );
		}
	}
	
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, bool, CBaseObject_FindBuildPointOnPlayer, CTFPlayer *pTFPlayer, CBasePlayer *pBuilder, float &flNearestPoint, Vector &vecNearestBuildPoint)
	{
		auto object = reinterpret_cast<CBaseObject *>(this);
		
		int cannotApply = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pTFPlayer, cannotApply, cannot_be_sapped);
		if (cannotApply)
			return false;
		return DETOUR_MEMBER_CALL(pTFPlayer, pBuilder, flNearestPoint, vecNearestBuildPoint);
	}
	
	DETOUR_DECL_MEMBER(void, CTFBaseRocket_Destroy)
	{
		auto rocket = reinterpret_cast<CTFBaseRocket *>(this);
		int explode = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(rocket->GetOriginalLauncher(), explode, projectile_explode_on_destroy);
		if (explode == 3 && takeDamageAttacker != nullptr && takeDamageAttacker->GetTeamNumber() != (rocket->GetOwnerEntity() != nullptr ? rocket->GetOwnerEntity() : rocket)->GetTeamNumber()) {
			DETOUR_MEMBER_CALL();
			return;
		}
		if (explode == 2 && takeDamageAttacker != nullptr) {
			rocket->SetOwnerEntity(takeDamageAttacker);
		}
		if (explode != 0) {
			trace_t tr;
			Vector vecSpot = rocket->GetAbsOrigin();
			UTIL_TraceLine(vecSpot, vecSpot + Vector (0, 0, -32), MASK_SHOT_HULL, rocket, COLLISION_GROUP_NONE, &tr);

			rocket->Explode(&tr, GetContainingEntity(INDEXENT(0)));
			return;
		}
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGrenadeProj_Destroy)
	{
		auto rocket = reinterpret_cast<CTFWeaponBaseGrenadeProj *>(this);
		int explode = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(rocket->GetOriginalLauncher(), explode, projectile_explode_on_destroy);
		if (explode == 3 && (takeDamageAttacker == nullptr || takeDamageAttacker->GetTeamNumber() != (rocket->GetThrower() != nullptr ? rocket->GetThrower() : rocket)->GetTeamNumber())) {
			DETOUR_MEMBER_CALL();
			return;
		}
		if (explode == 2 && takeDamageAttacker != nullptr) {
			rocket->SetOwnerEntity(takeDamageAttacker);
		}
		if (explode != 0) {
			trace_t tr;
			Vector vecSpot = rocket->GetAbsOrigin();
			UTIL_TraceLine(vecSpot, vecSpot + Vector (0, 0, -32), MASK_SHOT_HULL, rocket, COLLISION_GROUP_NONE, &tr);

			rocket->Explode(&tr, rocket->GetDamageType());
			return;
		}
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_CanHolster)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		int holdFire = GetFastAttributeInt(weapon, 0, HOLD_FIRE_UNTIL_FULL_RELOAD);
		if (holdFire != 0)
		{	
			bool hasFullClip = weapon->IsEnergyWeapon() ? weapon->m_flEnergy >= weapon->Energy_GetMaxEnergy() : weapon->m_iClip1 >= weapon->GetMaxClip1();
			bool isReloading = weapon->m_iReloadMode > 0;
			if (isReloading) return false;

			if (!hasFullClip && holdFire == 2) return false;
		}
		return DETOUR_MEMBER_CALL();
	}

	bool flareCritBefore = false;
	DETOUR_DECL_MEMBER(void, CTFProjectile_Flare_Explode, CGameTrace *tr, CBaseEntity *ent)
	{
		flareCritBefore = reinterpret_cast<CTFProjectile_Flare *>(this)->m_bCritical; 
		DETOUR_MEMBER_CALL(tr, ent);
	}

	DETOUR_DECL_MEMBER(int, CTFProjectile_Flare_GetDamageType)
	{
		auto flare = reinterpret_cast<CTFProjectile_Flare *>(this);
		if (!flareCritBefore && flare->m_bCritical) {
			int noCritOnBurning = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(flare->GetOriginalLauncher(), noCritOnBurning, flare_no_crit_burning);
			if (noCritOnBurning != 0) {
				flare->m_bCritical = false;
			}
		}
		return DETOUR_MEMBER_CALL();
	}
	
	// inline int GetMaxHealthForBuffing(CTFPlayer *player) {
	// 	int iMax = GetPlayerClassData(player->GetPlayerClass()->GetClassIndex())->m_nMaxHealth;
	// 	iMax += GetFastAttributeInt(player, 0, ADD_MAXHEALTH);

	// 	CTFWeaponBase *pWeapon = player->GetActiveTFWeapon();
	// 	if (pWeapon != nullptr)
	// 	{
	// 		iMax += pWeapon->GetMaxHealthMod();
	// 	}
	// 	auto sword = rtti_cast<CTFSword *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_MELEE));
	// 	if (sword != nullptr) {
	// 		iMax += sword->GetSwordHealthMod();
	// 	}

	// 	auto &shared = player->m_Shared.Get();
	// 	// Some Powerup Runes increase your Max Health
	// 	if (shared.GetCarryingRuneType() != -1) {
	// 		iMax += player->GetRuneHealthBonus();
	// 	}

	// 	if (shared.InCond(TF_COND_HALLOWEEN_GIANT))
	// 	{
	// 		static ConVarRef tf_halloween_giant_health_scale("tf_halloween_giant_health_scale");
	// 		return iMax * tf_halloween_giant_health_scale.GetFloat();
	// 	}

	// 	return iMax;
	// }
	// DETOUR_DECL_MEMBER(int, CTFPlayer_GetMaxHealthForBuffing)
	// {
	// 	CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
	// 	return GetMaxHealthForBuffing(player);
	// }
	
	// DETOUR_DECL_MEMBER(int, CTFPlayer_GetMaxHealth)
	// {
	// 	CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
	// 	int iMax = GetMaxHealthForBuffing(player);
	// 	iMax += GetFastAttributeInt(player, 0, ADD_MAXHEALTH_NONBUFFED);
	// 	return MAX(iMax, 1);
	// }

	ConVar cvar_display_attrs("sig_attr_display", "1", FCVAR_GAMEDLL,	
		"Enable displaying custom attributes on the right side of the screen");	

	class AttributeInfoModule : public EntityModule
	{
	public:
		AttributeInfoModule(CBaseEntity *entity) : EntityModule(entity) {}

		std::vector<std::string> m_Strings;
		float m_flDisplayTime;
		int m_iCurrentPage = 0;
	};

	bool DisplayAttributeString(CTFPlayer *player)
	{
		auto mod = player->GetEntityModule<AttributeInfoModule>("attributeinfo");
		if (mod == nullptr) return false;

		auto &vec = mod->m_Strings;

		CRecipientFilter filter;
		filter.AddRecipient(player);
		bf_write *msg = engine->UserMessageBegin(&filter, usermessages->LookupUserMessage("KeyHintText"));
		msg->WriteByte(1);
		int num = mod->m_iCurrentPage++;
		bool display = num < (int) vec.size();
		if (display) {
			std::string str = vec[num].substr(0, 253);
			msg->WriteString(str.c_str());
		}
		else {
			msg->WriteString("");
			vec.clear();
			mod->m_iCurrentPage = 0;
		}

		engine->MessageEnd();
		return display;
	}

	void ClearAttributeDisplay(CTFPlayer *player)
	{
		CRecipientFilter filter;
		filter.AddRecipient(player);
		bf_write *msg = engine->UserMessageBegin(&filter, usermessages->LookupUserMessage("KeyHintText"));
		msg->WriteByte(1);
		msg->WriteString("");
		engine->MessageEnd();
	}

	THINK_FUNC_DECL(DisplayAttributeStringThink) {
		if (DisplayAttributeString(reinterpret_cast<CTFPlayer *>(this))) {
        	this->SetNextThink(gpGlobals->curtime + 5.0f, "DisplayAttributeStringThink");
		}
	}

	void DisplayAttributes(int &indexstr, std::vector<std::string> &attribute_info_vec, CUtlVector<CEconItemAttribute> &attrs, CTFPlayer *player, CEconItemView *item_def, bool display_stock)
	{
		bool added_item_name = false;
		int slot = 0;//reinterpret_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinition(item_def))->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex());
		if (display_stock && (item_def == nullptr || slot < LOADOUT_POSITION_PDA2 || slot == LOADOUT_POSITION_ACTION) ) {
			added_item_name = true;
			std::string str = std::string(CFmtStr("\n%s:\n\n", item_def != nullptr ? GetItemNameForDisplay(item_def, player) : TranslateText(player, "Character Attributes:")));
			if (attribute_info_vec.back().size() + str.size() > 252) {
				attribute_info_vec.push_back(str);
			}
			else {
				attribute_info_vec.back() += str;
			}
		}
		for (int i = 0; i < attrs.Count(); i++) {
			CEconItemAttribute &attr = attrs[i];
			CEconItemAttributeDefinition *attr_def = attr.GetStaticData();
			
			if (attr_def == nullptr || (!display_stock && attr_def->GetIndex() < 4000))
				continue;

			std::string format_str;
			if (!FormatAttributeString(format_str, attr.GetStaticData(), *attr.GetValuePtr(), player))
				continue;

			// break lines
			size_t space_pos = 0;
			size_t last_space_pos = 0;
			size_t find_newline_pos = 0;
			while(space_pos < format_str.size()) {
				space_pos = format_str.find(" ", space_pos);
				if (space_pos == std::string::npos) {
					space_pos = format_str.size();
				}
				if (space_pos - find_newline_pos > 28 /*25*/ && last_space_pos > 0) {
					format_str.insert(last_space_pos - 1, "\n");
					space_pos++;
					find_newline_pos = last_space_pos;
				}
				space_pos++;
				last_space_pos = space_pos;
			}

			// replace \n with newline
			size_t newline_pos;
			while((newline_pos = format_str.find("\\n")) != std::string::npos) {
				format_str.replace(newline_pos, 2, "\n");
			} 

			// Replace percent symbols as HintKeyText parses them as keys
			size_t percent_pos;
			while((percent_pos = format_str.find("%")) != std::string::npos) {
				format_str.replace(percent_pos, 1, "％");
			} 

			if (!added_item_name) {
				format_str.insert(0, CFmtStr("\n%s:\n\n", item_def != nullptr ? GetItemNameForDisplay(item_def, player) : TranslateText(player, "Character Attributes:")));

				added_item_name = true;
			}

			if (attribute_info_vec.back().size() + format_str.size() + 1 > 251 /*220*/) {
				++indexstr;
				attribute_info_vec.push_back("");
			}
			if (indexstr > 5)
				break;

			attribute_info_vec.back() += format_str + '\n';
		}
	}

	void InspectAttributes(CTFPlayer *target, CTFPlayer *player, bool force, int slot)
	{
		if (!cvar_display_attrs.GetBool())
			return;
			
		bool display_stock = player != target && TFGameRules()->IsMannVsMachineMode();

		CTFWeaponBase *weapon = target->GetActiveTFWeapon();
		CEconItemView *view = nullptr;
		if (weapon != nullptr)
			view = weapon->GetItem();

		if (slot >= 0) {
			auto ent = GetEconEntityAtLoadoutSlot(target, slot);
			view = ent != nullptr ? ent->GetItem() : nullptr;
		}

		auto mod = player->GetOrCreateEntityModule<AttributeInfoModule>("attributeinfo");

		auto &attribute_info_vec = mod->m_Strings;

		ClearAttributeDisplay(player);

		if (!force && !attribute_info_vec.empty()) {
			attribute_info_vec.clear();
			return;
		}

		attribute_info_vec.clear();

		mod->m_flDisplayTime = gpGlobals->curtime;

		attribute_info_vec.push_back("");

		int indexstr = 0;

		bool display_stock_item = display_stock || (view != nullptr && view->GetStaticData()->GetLoadoutSlot(target->GetPlayerClass()->GetClassIndex()) == -1);
		if (slot == -1) {
			DisplayAttributes(indexstr, attribute_info_vec, target->GetAttributeList()->Attributes(), player, nullptr, display_stock);
			
			if (view != nullptr)
				DisplayAttributes(indexstr, attribute_info_vec, view->GetAttributeList().Attributes(), player, view, display_stock_item);
		}
		else {
			if (view != nullptr)
				DisplayAttributes(indexstr, attribute_info_vec, view->GetAttributeList().Attributes(), player, view, display_stock_item);

			DisplayAttributes(indexstr, attribute_info_vec, target->GetAttributeList()->Attributes(), player, nullptr, display_stock);
		}

		ForEachTFPlayerEconEntity(target, [&](CEconEntity *entity){
			if (entity->GetItem() != nullptr && entity->GetItem() != view && entity->GetItem()->GetStaticData()->m_iItemDefIndex != 0) {
				DisplayAttributes(indexstr, attribute_info_vec, entity->GetItem()->GetAttributeList().Attributes(), player, entity->GetItem(), display_stock || entity->GetItem()->GetStaticData()->GetLoadoutSlot(target->GetPlayerClass()->GetClassIndex()) == -1);
			}
		});

		if (player != target && !attribute_info_vec.empty()) {
			attribute_info_vec.insert(attribute_info_vec.begin(), TranslateText(player, "Inspecting player", 1, target->GetPlayerName()));
		}
		
		/*hudtextparms_t textparms;
		textparms.channel = 2;
		textparms.x = 1.0f;
		textparms.y = 0.0f;
		textparms.effect = 0;
		textparms.r1 = 255;
		textparms.r2 = 255;
		textparms.b1 = 255;
		textparms.b2 = 255;
		textparms.g1 = 255;
		textparms.g2 = 255;
		textparms.a1 = 0;
		textparms.a2 = 0; 
		textparms.fadeinTime = 0.f;
		textparms.fadeoutTime = 0.f;
		textparms.holdTime = 4.0f;
		textparms.fxTime = 1.0f;
		UTIL_HudMessage(player, textparms, attribute_info_vec[0].c_str());

		if (attribute_info_vec.size() > 1) {
			textparms.channel = 3;
			textparms.y = 0.45f;
			UTIL_HudMessage(player, textparms, attribute_info_vec[1].c_str());
		}*/
	
		if (!attribute_info_vec.back().empty()) {
			mod->m_iCurrentPage = 0;
			if (DisplayAttributeString(player)) {
				THINK_FUNC_SET(player, DisplayAttributeStringThink, gpGlobals->curtime + 5.0f);
			}
		}
	}

	THINK_FUNC_DECL(HideInvalidTarget)
	{
		gamehelpers->TextMsg(ENTINDEX(this), TEXTMSG_DEST_CENTER, " ");
	}
	DETOUR_DECL_MEMBER(void, CTFPlayer_InspectButtonPressed)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);

		Vector forward;
		AngleVectors(player->EyeAngles(), &forward);

		trace_t result;
		UTIL_TraceLine(player->EyePosition(), player->EyePosition() + 4000.0f * forward, MASK_SOLID, player, COLLISION_GROUP_NONE, &result);

		CTFPlayer *target = ToTFPlayer(result.m_pEnt);
		if (target == nullptr || target->GetTeamNumber() != player->GetTeamNumber()) {
			target = player;
		}
		else {
			THINK_FUNC_SET(player, HideInvalidTarget, gpGlobals->curtime + 0.05f);
		}
		InspectAttributes(target, player, false);

		DETOUR_MEMBER_CALL();

	}

    void RemoveAttributeManager(CBaseEntity *entity) {
        
        int index = ENTINDEX(entity);
        if (entity == last_fast_attrib_entity) {
            last_fast_attrib_entity = nullptr;
        }
		if (fast_attribute_cache[index] != nullptr) {
			delete fast_attribute_cache[index];
			fast_attribute_cache[index] = nullptr;
		}
    }

	DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateOnRemove)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		float respawntime = GetFastAttributeFloat(player, 0.0f, MIN_RESPAWN_TIME);

		if (!player->IsAlive() && respawntime != 0.0f && !(player->IsBot() && TFGameRules()->IsMannVsMachineMode()) && !(TFGameRules()->IsMannVsMachineMode() && TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS)) {
			respawnTimesForId[player->GetSteamID()] = player->GetDeathTime() + respawntime;
		}

        DETOUR_MEMBER_CALL();
        RemoveAttributeManager(reinterpret_cast<CBaseEntity *>(this));
    }

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CAttributeManager_ClearCache)
	{
        DETOUR_MEMBER_CALL();

        auto mgr = reinterpret_cast<CAttributeManager *>(this);

        if (mgr->m_hOuter != nullptr) {
			auto cache = fast_attribute_cache[ENTINDEX(mgr->m_hOuter)];
            if (cache != nullptr) {
				int count = mgr->m_hOuter->IsPlayer() ? (int)ATTRIB_COUNT_PLAYER : (int)ATTRIB_COUNT_ITEM;
				for(int i = 0; i < count; i++) {
					cache[i] = FLT_MIN;
				}
            }
        }
	}

    DETOUR_DECL_MEMBER(void, CEconEntity_UpdateOnRemove)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);
        DETOUR_MEMBER_CALL();
        RemoveAttributeManager(entity);
    }

	VHOOK_DECL(bool, CBaseProjectile_ShouldCollide, int collisionGroup, int contentsMask)
	{
		Msg("should\n");
		return false;

		return VHOOK_CALL(collisionGroup, contentsMask);
	}

#ifdef PLATFORM_64BITS
    DETOUR_DECL_MEMBER(bool, CEconItemAttributeIterator_ApplyAttributeString_OnIterateAttributeValue, const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& value)
	{
		const char *pstr = "";
		auto pvalue = (CAttribute_String *)(((uintptr_t)&value) & 0xFFFFFFFF);
        return DETOUR_MEMBER_CALL(pAttrDef, *pvalue);
    }
    DETOUR_DECL_MEMBER(void, CAttributeList_SetRuntimeAttributeRefundableCurrency, CEconItemAttributeDefinition *pAttrDef, int value)
	{
		// Do not write to m_nRefundableCurrency in case it is read as upper 32 bits for string attribute value
		if (pAttrDef->IsType<CSchemaAttributeType_String>()) return;
        return DETOUR_MEMBER_CALL(pAttrDef, value);
    }
	
#endif

	RefCount rc_HandleRageGain;
	DETOUR_DECL_STATIC(void, HandleRageGain, CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale)
	{
		if (pPlayer != nullptr) {
			float rageScale = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pPlayer, rageScale,rage_receive_scale);
			flDamage *= rageScale;
		}
		DETOUR_STATIC_CALL(pPlayer, iRequiredBuffFlags, flDamage, fInverseRageGainScale);
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
        DETOUR_MEMBER_CALL();
        auto mgr = reinterpret_cast<CAttributeManager *>(this);
		OnAttributesChange(mgr);
	}
	DETOUR_DECL_MEMBER(void, CAttributeContainer_OnAttributeValuesChanged)
	{
        DETOUR_MEMBER_CALL();
        auto mgr = reinterpret_cast<CAttributeManager *>(this);
		OnAttributesChange(mgr);
	}
	*/
	DETOUR_DECL_MEMBER(void, CAttributeContainerPlayer_OnAttributeValuesChanged)
	{
		auto mgr = reinterpret_cast<CAttributeManager *>(this);
		auto player = ToTFPlayer(mgr->m_hOuter);
		if (player != nullptr) {
			DETOUR_MEMBER_CALL();
			/*auto &rec = mgr->m_Providers.Get();
			FOR_EACH_VEC(rec, i) {
				//Msg("Receiver %s\n", ent->GetClassname());
				auto econ = reinterpret_cast<CEconEntity *>(rec[i].Get());
				econ->GetAttributeContainer()->ClearCache();
			}*/
			
			ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
				entity->GetAttributeContainer()->ClearCache();
			});

		}
	}

	CTFPlayer *GetPlayerOwnerOfAttributeList(CAttributeList *list, bool ifProviding)
	{
		auto manager = list->GetManager();
		if (manager != nullptr) {
			auto player = ToTFPlayer(manager->m_hOuter);
			if (player == nullptr && manager->m_hOuter != nullptr && (!ifProviding || manager->IsProvidingTo(manager->m_hOuter->GetOwnerEntity()))) {
				player = ToTFPlayer(manager->m_hOuter->GetOwnerEntity());
			}
			if (player == nullptr && stop_provider_entity != nullptr) {
				player = ToTFPlayer(stop_provider_entity);
			}
			return player;
		}
		return nullptr;
	}

	using AttributeCallback = void (*)(CAttributeList *, const CEconItemAttributeDefinition *, attribute_data_union_t, attribute_data_union_t, AttributeChangeType);
	std::vector<std::pair<unsigned short, AttributeCallback>> attribute_callbacks;
	
	void RegisterCallback(const char *attribute_class, AttributeCallback callback)
	{
		for (int i = 0; i < 30000; i++) {
			auto attr = GetItemSchema()->GetAttributeDefinition(i);
			if (attr != nullptr && attr->GetAttributeClass() != nullptr && strcmp(attr->GetAttributeClass(), attribute_class) == 0) {
				attribute_callbacks.push_back({i, callback});
			}
		}
	}

	void ClearAttributeManagerCachedAttribute(const char *class_name, CAttributeManager *manager)
	{
		auto &cached = manager->m_CachedResults.Get();
		for(int i = cached.Count() - 1; i >= 0; i--) {
			if (strcmp(STRING(cached[i].attrib), class_name) == 0) {
				cached.Remove(i);
			}
		}
	}

	void ClearAttributeManagerCachedAttributeRecurse(const char *class_name, CAttributeManager *manager)
	{
		ClearAttributeManagerCachedAttribute(class_name, manager);
		auto player = ToTFPlayer(manager->m_hOuter);
		if (player != nullptr) {
			ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
				ClearAttributeManagerCachedAttribute(class_name, entity->GetAttributeContainer());
			});
		}
		else if (manager->m_hOuter != nullptr) {
			auto player = ToTFPlayer(manager->m_hOuter->GetOwnerEntity());
			ClearAttributeManagerCachedAttribute(class_name, player->GetAttributeManager());
		}
	}

	void OnAttributeChanged(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		if (pAttrDef == nullptr) return;

		if (list->GetManager() == nullptr) return;
		
		// auto manager = list->GetManager();
		// if (manager != nullptr) {
		// 	auto className = pAttrDef->GetAttributeClass();
		// 	ClearAttributeManagerCachedAttributeRecurse(class_name, manager);
		// }

		int index = pAttrDef->GetIndex();
		
		if (changeType == AttributeChangeType::REMOVE)
			DefaultValueForAttribute(new_value, pAttrDef);
		if (changeType == AttributeChangeType::ADD)
			DefaultValueForAttribute(old_value, pAttrDef);

		for (auto &pair : attribute_callbacks) {
			if (pair.first == index) {
				(*pair.second)(list, pAttrDef, old_value, new_value, changeType);
			}
		}
	}
	
	ConVar sig_attr_speed_health_ammo_recalculate("sig_attr_speed_health_ammo_recalculate", "1", FCVAR_NOTIFY,
		"Recalculate current health, ammo, move speed when attribute changes. May affect other plugins. Requires sig_attr_custom 1");

	void OnMoveSpeedChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		if (!sig_attr_speed_health_ammo_recalculate.GetBool()) return;

		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			player->TeamFortress_SetSpeed();
		}
	}

	void OnMaxHealthChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		if (!sig_attr_speed_health_ammo_recalculate.GetBool()) return;

		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr && player->GetHealth() > 0) {
			float change = pAttrDef->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_ADDITIVE ? new_value.m_Float - old_value.m_Float : player->GetMaxHealth() * (1 - (old_value.m_Float / new_value.m_Float));
			float maxHealth = player->GetMaxHealth();
			float preMaxHealth = maxHealth - change;
			float overheal = MAX(0, player->GetHealth() - preMaxHealth);
			float preHealthRatio = MIN(1, player->GetHealth() / preMaxHealth);
			player->SetHealth(MAX(1,round(maxHealth * preHealthRatio + overheal)));
		}
	}

	void OnItemColorChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto manager = list->GetManager();
		if (manager != nullptr) {
			auto player = GetPlayerOwnerOfAttributeList(list);
			CBaseEntity *ent = manager->m_hOuter;
			auto econentity = rtti_cast<CEconEntity *>(ent);
			if (econentity != nullptr) {
				ApplyAttachmentAttributesToEntity(player, FindVisibleEntity(player, econentity), econentity);
			}
		}
	}

	void OnCustomModelChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto manager = list->GetManager();
		if (manager != nullptr) {
			auto player = GetPlayerOwnerOfAttributeList(list);
			CBaseEntity *ent = manager->m_hOuter;
			auto econentity = rtti_cast<CEconEntity *>(ent);
			if (econentity != nullptr && new_value.m_String != nullptr) {
				UpdateCustomModel(player, econentity, *list);
			}
		}
	}

	void OnCustomViewModelChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto manager = list->GetManager();
		if (manager != nullptr) {
			CBaseEntity *ent = manager->m_hOuter;
			auto weapon = rtti_cast<CTFWeaponBase *>(ent);
			auto player = GetPlayerOwnerOfAttributeList(list);
			if (weapon != nullptr && new_value.m_String != nullptr && player != nullptr) {
				const char *value;
				CopyStringAttributeValueToCharPointerOutput(new_value.m_String, &value);
				bool allowed = IsCustomViewmodelAllowed(player);
				weapon->SetCustomViewModel(changeType == AttributeChangeType::REMOVE || value == nullptr || !allowed ? "" : value);
				if (changeType == AttributeChangeType::REMOVE || value == nullptr || value[0] == '\0' || !allowed) {
					weapon->SetViewModel();
					weapon->m_iViewModelIndex = CBaseEntity::PrecacheModel(weapon->GetViewModel());
					weapon->SetModel(weapon->GetViewModel());
				}
				else {
					weapon->SetCustomViewModel(value);
					weapon->m_iViewModelIndex = CBaseEntity::PrecacheModel(value);
					weapon->SetModel(value);
				}
			}
		}
	}

	void OnPaintkitChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto attr = list->GetAttributeByID(pAttrDef->GetIndex());

		if (attr != nullptr && attr->GetValuePtr()->m_UInt > 0x40000000 ) {
			attr->GetValuePtr()->m_UInt = attr->GetValuePtr()->m_Float;
		}
	}

	void OnMiniBossChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			player->SetMiniBoss(new_value.m_Float != 0.0f);

			float playerScale = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, playerScale, model_scale);
			if (playerScale == 1.0f) {
				if (new_value.m_Float != 0.0f) {
					static ConVarRef miniboss_scale("tf_mvm_miniboss_scale");
					player->SetModelScale(miniboss_scale.GetFloat());
				}
				else {
					player->SetModelScale(1.0f);
				}
			}
#ifndef NO_MVM
			Mod::Pop::PopMgr_Extensions::ApplyOrClearRobotModel(player);
#endif
		}
	}

	void OnScaleChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			player->SetModelScale(new_value.m_Float);
		}
	}

	void OnReloadFullClipAtOnceChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto manager = list->GetManager();
		if (manager != nullptr) {
			CBaseEntity *ent = manager->m_hOuter;
			auto econentity = rtti_cast<CTFWeaponBase *>(ent);
			if (econentity != nullptr) {
				econentity->m_bReloadsSingly = new_value.m_Float == 0.0;
			}
		}
	}
	class AmmoFractionModule : public EntityModule
	{
	public:
		AmmoFractionModule(CBaseEntity *entity) {}

		float ammoFraction[TF_AMMO_COUNT] {0.0f};
	};

	void AdjustAmmo(CTFPlayer *player, int ammoType, attribute_data_union_t old_value, attribute_data_union_t new_value, bool additive)
	{
		if (!sig_attr_speed_health_ammo_recalculate.GetBool()) return;

		if (additive) {
			float oldVal = (player->GetMaxAmmo(ammoType) - (new_value.m_Float - old_value.m_Float));
			if (oldVal == 0) {
				player->SetAmmoCount(player->GetMaxAmmo(ammoType), ammoType);
				return;
			}
		}
		float newAmmoRatio = additive ? player->GetMaxAmmo(ammoType) : new_value.m_Float;
		float oldAmmoRatio = additive ? (player->GetMaxAmmo(ammoType) - (new_value.m_Float - old_value.m_Float)) : old_value.m_Float;

		float multIncrease = newAmmoRatio / oldAmmoRatio;
		auto mod = player->GetOrCreateEntityModule<AmmoFractionModule>("amraction");
		// Old max ammo count was 0, restrore ammo to full
		if (oldAmmoRatio == 0) {
			mod->ammoFraction[ammoType] = 0;
			player->SetAmmoCount(ammoType, player->GetMaxAmmo(ammoType));
			return;
		}
		if (mod->ammoFraction[ammoType] != 0 && player->GetAmmoCount(ammoType) * multIncrease >= player->GetMaxAmmo(ammoType)) {
			mod->ammoFraction[ammoType] = 0;
		}
		float ammo = (player->GetAmmoCount(ammoType) +  mod->ammoFraction[ammoType]) * multIncrease;
		float fraction = ammo - (int) ammo;
		player->SetAmmoCount(ammo, ammoType);
		mod->ammoFraction[ammoType] = fraction;
	}

	void OnPrimaryAmmoChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			int maxAmmoPre = player->GetMaxAmmo(TF_AMMO_PRIMARY);
			int ammoPre = player->GetAmmoCount(TF_AMMO_PRIMARY);

			AdjustAmmo(player, TF_AMMO_PRIMARY, old_value, new_value, !pAttrDef->IsMultiplicative());
		}
	}

	void OnSecondaryAmmoChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			AdjustAmmo(player, TF_AMMO_SECONDARY, old_value, new_value, !pAttrDef->IsMultiplicative());
		}
	}

	void OnGrenadeAmmoChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			AdjustAmmo(player, TF_AMMO_GRENADES1, old_value, new_value, !pAttrDef->IsMultiplicative());
		}
	}

	void OnMetalChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			AdjustAmmo(player, TF_AMMO_METAL, old_value, new_value, !pAttrDef->IsMultiplicative());
		}
	}

	void OnNoClipChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			if (changeType == AttributeChangeType::REMOVE || new_value.m_Float == 0)
				player->SetMoveType(MOVETYPE_WALK);
			else if (new_value.m_Float != 0)
				player->SetMoveType(MOVETYPE_NOCLIP);
		}
	}

	void OnAddAttributesWhenActive(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto manager = list->GetManager();
		if (manager != nullptr) {
			CBaseEntity *ent = manager->m_hOuter;
			auto econentity = rtti_cast<CEconEntity *>(ent);
			if (econentity != nullptr) {
				if (changeType == AttributeChangeType::REMOVE || changeType == AttributeChangeType::UPDATE) {
					const char *oldValue;
					CopyStringAttributeValueToCharPointerOutput(old_value.m_String, &oldValue);
					RemoveOnActiveAttributes(econentity, oldValue);
				}
				if (changeType == AttributeChangeType::ADD || changeType == AttributeChangeType::UPDATE) {
					const char *newValue;
					CopyStringAttributeValueToCharPointerOutput(new_value.m_String, &newValue);
					auto weapon = rtti_cast<CTFWeaponBase *>(ent);
					if (weapon != nullptr && weapon->m_iState == WEAPON_IS_ACTIVE) {
						AddOnActiveAttributes(econentity, newValue);
					}
				}
			}
		}
	}

	void OnMedigunBeamChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto manager = list->GetManager();
		if (manager != nullptr) {
			CBaseEntity *ent = manager->m_hOuter;
			auto econentity = rtti_cast<CWeaponMedigun *>(ent);
			if (econentity != nullptr) {
				econentity->GetOrCreateEntityModule<MedigunCustomBeamModule>("mediguncustombeam")->RecalcEffects();
			}
		}
	}

	void OnAltFireAttributesChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto manager = list->GetManager();
		if (manager != nullptr) {
			CBaseEntity *ent = manager->m_hOuter;
			auto econentity = rtti_cast<CEconEntity *>(ent);
			if (econentity != nullptr) {
				if (changeType == AttributeChangeType::ADD || changeType == AttributeChangeType::UPDATE) {
					const char *newValue = nullptr;
					CopyStringAttributeValueToCharPointerOutput(new_value.m_String, &newValue);
					if (newValue != nullptr && newValue[0] != '\0') {
						econentity->GetOrCreateEntityModule<AltFireAttributesModule>("altfireattrs")->attributes = MAKE_STRING(newValue);
						return;
					}
				}
				econentity->RemoveEntityModule("altfireattrs");
			}
		}
	}

	void OnFOVChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			if (changeType == AttributeChangeType::REMOVE || new_value.m_Float == 0)
				player->SetFOV(player, 0, 0.1f, 0);
			else if (new_value.m_Float != 0)
				player->SetFOV(player, new_value.m_Float, 0.1f, 0);
		}
	}

	void OnOverlayChange(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			if (changeType == AttributeChangeType::ADD || changeType == AttributeChangeType::UPDATE) {
				const char *newValue = nullptr;
				CopyStringAttributeValueToCharPointerOutput(new_value.m_String, &newValue);
				if (newValue != nullptr && newValue[0] != '\0') {
					V_strncpy(player->m_Local->m_szScriptOverlayMaterial.Get(), newValue, 260);
					return;
				}
			}
			player->m_Local->m_szScriptOverlayMaterial.Get()[0] = '\0';
		}
	}

	void OnAddCondImmunity(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			auto mod = player->GetOrCreateEntityModule<AddCondAttributeImmunity>("addcondimmunity");

			if (changeType == AttributeChangeType::REMOVE || changeType == AttributeChangeType::UPDATE) {
				const char *oldValue = nullptr;
				CopyStringAttributeValueToCharPointerOutput(old_value.m_String, &oldValue);
				if (oldValue != nullptr && oldValue[0] != '\0') {

					const std::string_view input{oldValue};
					const auto v{vi::split_str(input, "|")};
					for(auto &val : v) {
						auto cond = vi::from_str<int>(val);
						if (cond.has_value()) {
							auto find = std::find(mod->conds.begin(), mod->conds.end(), cond.value());
							if (find != mod->conds.end())
								mod->conds.erase(find);
						}
					}
				}
			}

			if (changeType == AttributeChangeType::ADD || changeType == AttributeChangeType::UPDATE) {
				const char *newValue = nullptr;
				CopyStringAttributeValueToCharPointerOutput(new_value.m_String, &newValue);
				if (newValue != nullptr && newValue[0] != '\0') {
					
					const std::string_view input{newValue};
					const auto v{vi::split_str(input, "|")};
					for(auto &val : v) {
						auto cond = vi::from_str<int>(val);
						if (cond.has_value()) {
							if (cond >= 0) {
								player->m_Shared->RemoveCond((ETFCond)cond.value());
							}
							mod->conds.push_back(cond.value());
						}
					}
				}
			}
		}
	}

	void OnAttributeImmunity(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		if (player != nullptr) {
			auto mod = player->GetOrCreateEntityModule<AddCondAttributeImmunity>("addcondimmunity");

			if (changeType == AttributeChangeType::REMOVE || changeType == AttributeChangeType::UPDATE) {
				const char *oldValue = nullptr;
				CopyStringAttributeValueToCharPointerOutput(old_value.m_String, &oldValue);
				if (oldValue != nullptr && oldValue[0] != '\0') {
					std::string str(oldValue);
					boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

					auto it = tokens.begin();
					while (it != tokens.end()) {
						auto attr = GetItemSchema()->GetAttributeDefinitionByName(it->c_str());
						if (attr != nullptr) {
							auto find = std::find(mod->attributes.begin(), mod->attributes.end(), attr);
							if (find != mod->attributes.end())
								mod->attributes.erase(find);
						}
						it++;
					}
				}
			}

			if (changeType == AttributeChangeType::ADD || changeType == AttributeChangeType::UPDATE) {
				const char *newValue = nullptr;
				CopyStringAttributeValueToCharPointerOutput(new_value.m_String, &newValue);
				if (newValue != nullptr && newValue[0] != '\0') {
					std::string str(newValue);
					boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>("|"));

					auto it = tokens.begin();
					while (it != tokens.end()) {
						auto attr = GetItemSchema()->GetAttributeDefinitionByName(it->c_str());
						if (attr != nullptr) {
							player->GetAttributeList()->RemoveAttribute(attr);
							mod->attributes.push_back(attr);
						}
						it++;
					}
				}
			}
		}
	}

	class CustomModelAttachment : public EntityModule, public AutoList<CustomModelAttachment>
	{
	public:
		CustomModelAttachment(CBaseEntity *entity) : EntityModule(entity), me((CTFWeaponBase *)entity) {}

		~CustomModelAttachment() {
			for (auto &[def, ent] : models) {
				if (ent != nullptr) {
					ent->Remove();
				}
			}
		}

		CTFWeaponBase *me;
		bool prevStateActive = false;
		std::unordered_multimap<const CEconItemAttributeDefinition *, CHandle<CBaseEntity>> models;
	};

	void OnCustomItemModelAttachment(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto manager = list->GetManager();
		bool isVM = FStrEq(pAttrDef->GetAttributeClass(), "custom_item_model_attachment_viewmodel");
		if (manager != nullptr) {
			CBaseEntity *ent = manager->m_hOuter;
			auto econentity = rtti_cast<CTFWeaponBase *>(ent);
			if (econentity != nullptr) {
				auto player = ToTFPlayer(econentity->GetOwnerEntity());
				if (player == nullptr) return;

				auto mod = ent->GetOrCreateEntityModule<CustomModelAttachment>("custommodelattachment");
				auto range = mod->models.equal_range(pAttrDef);
				for (auto it = range.first; it != range.second; it++) {
					CBaseEntity *ent = it->second;
					if (ent != nullptr) {
						ent->Remove();
					}
				}

				if (changeType == AttributeChangeType::ADD || changeType == AttributeChangeType::UPDATE) {
					const char *newValue = nullptr;
					CopyStringAttributeValueToCharPointerOutput(new_value.m_String, &newValue);
					if (newValue != nullptr && newValue[0] != '\0') {
						
						vi::for_each_split_str(newValue, "|", [econentity, player, isVM, mod, pAttrDef](auto str){
							auto params = vi::split_str(str, ",");
							if (params.empty()) return;

							auto wearable = static_cast<CTFWearable *>(CreateEntityByName(isVM ? "tf_wearable_vm": "tf_wearable"));
							DispatchSpawn(wearable);
							wearable->GiveTo(player);
							wearable->m_hWeaponAssociatedWith = econentity;
							wearable->m_spawnflags |= (1 << 30);
							wearable->m_bAlwaysAllow = true;
							std::string model {params[0]};
							wearable->SetModelIndex(CBaseEntity::PrecacheModel(model.c_str()));
							wearable->m_bValidatedAttachedEntity = true;

							size_t paramIndexOrigin = isVM ? 1 : 3;
							size_t paramIndexAngles = isVM ? 2 : 4;
							size_t paramIndexScale = isVM ? 3 : 5;
							if (params.size() > paramIndexOrigin) {
								auto paramsOrigin = vi::split_str(params[paramIndexOrigin], " ");
								if (paramsOrigin.size() >= 3) {
									wearable->RemoveEffects(EF_BONEMERGE);
									wearable->SetLocalOrigin(Vector(vi::from_str<float>(paramsOrigin[0]).value_or(0), vi::from_str<float>(paramsOrigin[1]).value_or(0), vi::from_str<float>(paramsOrigin[2]).value_or(0)));
								}
							}
							if (params.size() > paramIndexAngles) {
								auto paramsAngles = vi::split_str(params[paramIndexAngles], " ");
								if (paramsAngles.size() >= 3) {
									wearable->RemoveEffects(EF_BONEMERGE);
									wearable->SetLocalAngles(QAngle(vi::from_str<float>(paramsAngles[0]).value_or(0), vi::from_str<float>(paramsAngles[1]).value_or(0), vi::from_str<float>(paramsAngles[2]).value_or(0)));
								}
							}
							if (params.size() > paramIndexScale) {
								wearable->RemoveEffects(EF_BONEMERGE);
								wearable->SetModelScale(vi::from_str<float>(params[paramIndexScale]).value_or(1));
							}
							if (!isVM) {
								if (params.size() > 1 && !params[1].empty()) {
									wearable->RemoveEffects(EF_BONEMERGE);
									std::string attach {params[1]};
									wearable->m_iParentAttachment = MAX(0, player->LookupAttachment(attach.c_str()));
								}
								if (params.size() > 2 && !params[2].empty()) {
									auto paramsColor = vi::split_str(params[2], " ");
									if (paramsColor.size() >= 3) {
										wearable->SetRenderColorR(vi::from_str<int>(paramsColor[0]).value_or(0));
										wearable->SetRenderColorG(vi::from_str<int>(paramsColor[1]).value_or(0));
										wearable->SetRenderColorB(vi::from_str<int>(paramsColor[2]).value_or(0));
									}
								}
							}
							mod->models.emplace(pAttrDef, wearable);
						});
					}
				}
			}
		}
	}

	void OnArrowIgnite(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		auto player = GetPlayerOwnerOfAttributeList(list);
		auto manager = list->GetManager();
		if (player != nullptr && manager != nullptr) {
			CBaseEntity *ent = manager->m_hOuter;
			auto econentity = rtti_cast<CTFCompoundBow *>(ent);
			if (econentity != nullptr && player->GetActiveTFWeapon() == econentity) {
				econentity->m_bArrowAlight = new_value.m_Float != 0;
			}
		}
	}

	void OnProvideOnActive(CAttributeList *list, const CEconItemAttributeDefinition *pAttrDef, attribute_data_union_t old_value, attribute_data_union_t new_value, AttributeChangeType changeType)
	{
		if (!rc_ProvidingAttributes) {
			auto manager = list->GetManager();
			if (manager != nullptr) {
				CBaseEntity *ent = manager->m_hOuter;
				auto econentity = rtti_cast<CEconEntity *>(ent);
				if (econentity != nullptr) {
					econentity->ReapplyProvision();
				}
			}
		}
	}

	void ChangeBuildingProperties(CTFPlayer *player, CBaseObject *obj)
	{
		if (obj != nullptr) {
			obj->SetCustomVariable("fireratemult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_sentry_firerate", player)));
			obj->SetCustomVariable("rangemult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_sentry_range", player)));
			obj->SetCustomVariable("radiusmult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_dispenser_radius", player)));
			obj->SetCustomVariable("damagemult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_engy_sentry_damage", player)));
			obj->SetCustomVariable("bidirectional", Variant(CAttributeManager::AttribHookValue(0, "bidirectional_teleport", player)));
			obj->SetCustomVariable("maxlevel", Variant(CAttributeManager::AttribHookValue(0, "building_max_level", player)));
			obj->SetCustomVariable("rapidfire", Variant(CAttributeManager::AttribHookValue(0, "sentry_rapid_fire", player)));
			obj->SetCustomVariable("ammomult", Variant(CAttributeManager::AttribHookValue(1.0f, "mvm_sentry_ammo", player)));
			obj->SetCustomVariable("rocketfireratemult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_firerocket_rate", player)));
			obj->SetCustomVariable("ratemult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_dispenser_rate", player)));
			obj->SetCustomVariable("rechargeratemult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_teleporter_recharge_rate", player)));
			obj->SetCustomVariable("speedboost", Variant(CAttributeManager::AttribHookValue(0, "mod_teleporter_speed_boost", player)));
			obj->SetCustomVariable("rocketammomult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_sentry_rocket_ammo", player)));
			obj->SetCustomVariable("projspeedmult", Variant(CAttributeManager::AttribHookValue(1.0f, "mult_sentry_rocket_projectile_speed", player)));
			obj->SetCustomVariable("cannotbesapped", Variant(CAttributeManager::AttribHookValue(0, "buildings_cannot_be_sapped", player)));
			obj->SetCustomVariable("ignoredbybots", Variant(CAttributeManager::AttribHookValue(0, "ignored_by_bots", player)));
			obj->SetCustomVariable("bulletweapon", Variant(player->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, player, PStrT<"sentry_bullet_weapon">())));
			obj->SetCustomVariable("rocketweapon", Variant(player->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, player, PStrT<"sentry_rocket_weapon">())));
			obj->SetCustomVariable("sentrymodelprefix", Variant(player->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, player, PStrT<"custom_sentry_model">())));
			obj->SetCustomVariable("dispensermodelprefix", Variant(player->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, player, PStrT<"custom_dispenser_model">())));
			obj->SetCustomVariable("teleportermodelprefix", Variant(player->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, player, PStrT<"custom_teleporter_model">())));
		}
	}

	void ChangeBuildingsProperties(CTFPlayer *player)
	{
		for (int i = 0; i < player->GetObjectCount(); i++) {
			ChangeBuildingProperties(player, player->GetObject(i));
		}
	}

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Attr:Custom_Attributes")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanAirDash, "CTFPlayer::CanAirDash");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_AllowedToHealTarget, "CWeaponMedigun::AllowedToHealTarget");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_HealTargetThink, "CWeaponMedigun::HealTargetThink");
			MOD_ADD_DETOUR_MEMBER(CTFCompoundBow_LaunchGrenade, "CTFCompoundBow::LaunchGrenade");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFWeaponBaseGun_FireProjectile, "CTFWeaponBaseGun::FireProjectile", HIGHEST);
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
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_GetProjectileSpeed,        "CTFWeaponBaseGun::GetProjectileSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGrenadeProj_Explode,    "CTFWeaponBaseGrenadeProj::Explode");
			MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Explode,    "CTFBaseRocket::Explode");

			MOD_ADD_DETOUR_MEMBER(CRecipientFilter_IgnorePredictionCull,    "CRecipientFilter::IgnorePredictionCull");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetDamageType,    "CTFWeaponBase::GetDamageType");
			MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_GetDamageType,    "CTFSniperRifle::GetDamageType");
			MOD_ADD_DETOUR_MEMBER(CTFSniperRifleClassic_GetDamageType,    "CTFSniperRifleClassic::GetDamageType");
			MOD_ADD_DETOUR_MEMBER(CTFRevolver_GetDamageType,    "CTFRevolver::GetDamageType");
			MOD_ADD_DETOUR_MEMBER(CTFSMG_GetDamageType,    "CTFSMG::GetDamageType");
			MOD_ADD_DETOUR_MEMBER(CTFPistol_ScoutSecondary_GetDamageType,    "CTFPistol_ScoutSecondary::GetDamageType");
			
			MOD_ADD_DETOUR_MEMBER(CTFMinigun_CanHolster,    "CTFMinigun::CanHolster");
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_ApplyRoboSapperEffects,    "CObjectSapper::ApplyRoboSapperEffects [clone]");
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_IsParentValid,    "CObjectSapper::IsParentValid");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsAllowedToTaunt,    "CTFPlayer::IsAllowedToTaunt");
			MOD_ADD_VHOOK(CUpgrades_StartTouch, TypeName<CUpgrades>(), "CBaseTrigger::StartTouch");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_UpgradeTouch, "CUpgrades::UpgradeTouch");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_EndTouch, "CUpgrades::EndTouch");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_FireRocket, "CObjectSentrygun::FireRocket");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_StartUpgrading, "CBaseObject::StartUpgrading");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_CanBeUpgraded, "CBaseObject::CanBeUpgraded");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_SentryThink, "CObjectSentrygun::SentryThink");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_StartBuilding, "CBaseObject::StartBuilding");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_StartPlacement, "CBaseObject::StartPlacement");
			MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_RecieveTeleportingPlayer, "CObjectTeleporter::RecieveTeleportingPlayer");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_WantsLagCompensationOnEntity, "CTFPlayer::WantsLagCompensationOnEntity");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FPlayerCanTakeDamage, "CTFGameRules::FPlayerCanTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_StunPlayer, "CTFPlayerShared::StunPlayer");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_HealingBolt_CanHeadshot, "CTFProjectile_HealingBolt::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_GrapplingHook_CanHeadshot, "CTFProjectile_GrapplingHook::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_CanHeadshot, "CTFProjectile_EnergyBall::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyRing_CanHeadshot, "CTFProjectile_EnergyRing::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_CanHeadshot, "CTFProjectile_Arrow::CanHeadshot");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatCharacter_OnTakeDamage, "CBaseCombatCharacter::OnTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageModifyRules, "CTFGameRules::ApplyOnDamageModifyRules");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_Equip, "CBaseCombatWeapon::Equip");
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_VPhysicsCollision, "CTFGrenadePipebombProjectile::VPhysicsCollision");
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_PipebombTouch, "CTFGrenadePipebombProjectile::PipebombTouch");
			MOD_ADD_DETOUR_STATIC(PropDynamic_CollidesWithGrenades, "PropDynamic_CollidesWithGrenades");
			MOD_ADD_DETOUR_MEMBER(CTraceFilterObject_ShouldHitEntity, "CTraceFilterObject::ShouldHitEntity");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_RegenThink, "CTFPlayer::RegenThink");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_StopTaunt, "CTFPlayer::StopTaunt");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsAllowedToInitiateTauntWithPartner, "CTFPlayer::IsAllowedToInitiateTauntWithPartner");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_DeflectEntity, "CTFWeaponBase::DeflectEntity");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetKillingWeaponName, "CTFGameRules::GetKillingWeaponName");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Touch, "CTFPlayer::Touch");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_PlayerPurchasingUpgrade, "CUpgrades::PlayerPurchasingUpgrade [clone]");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ReapplyItemUpgrades, "CTFPlayer::ReapplyItemUpgrades");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CUpgrades_ApplyUpgradeToItem, "CUpgrades::ApplyUpgradeToItem [clone]", LOWEST);
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_WeaponSound, "CBaseCombatWeapon::WeaponSound");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_IsDeflectable, "CTFProjectile_Rocket::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_IsDeflectable, "CTFProjectile_Arrow::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Flare_IsDeflectable, "CTFProjectile_Flare::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_IsDeflectable, "CTFProjectile_EnergyBall::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_IsDeflectable, "CTFGrenadePipebombProjectile::IsDeflectable");
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_OnTakeDamage, "CTFGrenadePipebombProjectile::OnTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_InSameTeam, "CBaseEntity::InSameTeam");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnAddBalloonHead, "CTFPlayerShared::OnAddBalloonHead");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ApplyOnHitAttributes,          "CTFWeaponBase::ApplyOnHitAttributes");
			MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_TeleporterTouch,          "CObjectTeleporter::TeleporterTouch");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_GetTargetRange,          "CWeaponMedigun::GetTargetRange");
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_ShouldOpportunisticallyTeleport, "CTFBotTacticalMonitor::ShouldOpportunisticallyTeleport");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_ArrowTouch, "CTFProjectile_Arrow::ArrowTouch");
			MOD_ADD_DETOUR_MEMBER(CTFRadiusDamageInfo_ApplyToEntity, "CTFRadiusDamageInfo::ApplyToEntity [clone]");
			MOD_ADD_DETOUR_MEMBER(CTFFlameManager_BCanBurnEntityThisFrame,        "CTFFlameManager::BCanBurnEntityThisFrame");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_BallOfFire_RocketTouch, "CTFProjectile_BallOfFire::RocketTouch");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyRing_ProjectileTouch, "CTFProjectile_EnergyRing::ProjectileTouch");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyRing_ShouldPenetrate, "CTFProjectile_EnergyRing::ShouldPenetrate");
			

			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayerShared_AddCond, "CTFPlayerShared::AddCond", HIGHEST);
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayerShared_AddCond2, "CTFPlayerShared::AddCond", HIGH);
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayerShared_RemoveCond, "CTFPlayerShared::RemoveCond", HIGHEST);
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_PulseRageBuff, "CTFPlayerShared::PulseRageBuff");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DoTauntAttack, "CTFPlayer::DoTauntAttack");
			MOD_ADD_DETOUR_MEMBER(CTFSodaPopper_SecondaryAttack, "CTFSodaPopper::SecondaryAttack");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_SetScoutHypeMeter, "CTFPlayerShared::SetScoutHypeMeter");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_UpdateEnergyDrinkMeter, "CTFPlayerShared::UpdateEnergyDrinkMeter [clone]");
			MOD_ADD_DETOUR_STATIC(JarExplode, "JarExplode");
			MOD_ADD_DETOUR_MEMBER(CTFGasManager_OnCollide, "CTFGasManager::OnCollide");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_SetChargeEffect, "CTFPlayerShared::SetChargeEffect");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_PulseMedicRadiusHeal, "CTFPlayerShared::PulseMedicRadiusHeal [clone]");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_SetRevengeCrits, "CTFPlayerShared::SetRevengeCrits");
			MOD_ADD_DETOUR_MEMBER(CTFFlareGun_Revenge_Deploy , "CTFFlareGun_Revenge::Deploy");
			MOD_ADD_DETOUR_MEMBER(CTFShotgun_Revenge_Deploy, "CTFShotgun_Revenge::Deploy");
			MOD_ADD_DETOUR_MEMBER(CTFRevolver_Deploy, "CTFRevolver::Deploy");
			MOD_ADD_DETOUR_MEMBER(CTFFlareGun_Revenge_Holster, "CTFFlareGun_Revenge::Holster");
			MOD_ADD_DETOUR_MEMBER(CTFShotgun_Revenge_Holster, "CTFShotgun_Revenge::Holster");
			MOD_ADD_DETOUR_MEMBER(CTFRevolver_Holster ,"CTFRevolver::Holster");
			MOD_ADD_DETOUR_MEMBER(CTFChargedSMG_SecondaryAttack ,"CTFChargedSMG::SecondaryAttack");

			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnKilledOther_Effects ,"CTFPlayer::OnKilledOther_Effects");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_TeamFortress_CalculateMaxSpeed ,"CTFPlayer::TeamFortress_CalculateMaxSpeed");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseEntity_TakeDamage ,"CBaseEntity::TakeDamage", HIGH);
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_GetMaxBuffedHealth ,"CTFPlayerShared::GetMaxBuffedHealth");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_TakeHealth ,"CTFPlayer::TakeHealth");
			MOD_ADD_DETOUR_MEMBER(CObjectDispenser_DispenseMetal ,"CObjectDispenser::DispenseMetal");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GiveAmmo ,"CTFPlayer::GiveAmmo");
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_StickybombTouch ,"CTFGrenadePipebombProjectile::StickybombTouch");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DropCurrencyPack ,"CTFPlayer::DropCurrencyPack");
			MOD_ADD_DETOUR_MEMBER(CTeamplayRoundBasedRules_GetMinTimeWhenPlayerMaySpawn ,"CTeamplayRoundBasedRules::GetMinTimeWhenPlayerMaySpawn");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_OnPlayerSpawned ,"CTFGameRules::OnPlayerSpawned");
			MOD_ADD_DETOUR_MEMBER(CTFMinigun_WindDown ,"CTFMinigun::WindDown");
			MOD_ADD_DETOUR_MEMBER(CTFMinigun_WindUp ,"CTFMinigun::WindUp");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DropAmmoPack, "CTFPlayer::DropAmmoPack");
			MOD_ADD_DETOUR_STATIC(CTFDroppedWeapon_Create, "CTFDroppedWeapon::Create");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Regenerate ,"CTFPlayer::Regenerate");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ItemsMatch ,"CTFPlayer::ItemsMatch [clone]");
			MOD_ADD_DETOUR_MEMBER(CTFMinigun_SetWeaponState ,"CTFMinigun::SetWeaponState");
// Older GCC has trouble compiling it, easiest way is to just disable it
#if !(defined(__GNUC__) && (__GNUC__ < 12))
			MOD_ADD_DETOUR_MEMBER(CTFFlameThrower_SetWeaponState ,"CTFFlameThrower::SetWeaponState");
#endif
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_StrikeTarget ,"CTFProjectile_Arrow::StrikeTarget");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_FadeOut, "CTFProjectile_Arrow::FadeOut");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_CheckSkyboxImpact, "CTFProjectile_Arrow::CheckSkyboxImpact");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_CalculateObjectCost, "CTFPlayerShared::CalculateObjectCost");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetPenetrateType, "CTFWeaponBase::GetPenetrateType");
			MOD_ADD_DETOUR_MEMBER(CBaseProjectile_GetCollideWithTeammatesDelay, "CBaseProjectile::GetCollideWithTeammatesDelay");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_TFPlayerThink,           "CTFPlayer::TFPlayerThink");
			MOD_ADD_DETOUR_MEMBER(CTFGameMovement_PlayerSolidMask, "CTFGameMovement::PlayerSolidMask");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_PlayerRunCommand,					 "CTFPlayer::PlayerRunCommand");
			MOD_ADD_DETOUR_MEMBER(CTFGameMovement_PreventBunnyJumping,			 "CTFGameMovement::PreventBunnyJumping");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Reload,			 "CTFWeaponBase::Reload");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Holster,			 "CTFWeaponBase::Holster");
			MOD_ADD_DETOUR_MEMBER(CBaseProjectile_D2, "CBaseProjectile::~CBaseProjectile [D2]");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_PrimaryAttack, "CTFWeaponBaseGun::PrimaryAttack");
			MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks, "SV_ComputeClientPacks");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Spawn, "CTFPlayer::Spawn");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetSpreadAngles, "CTFWeaponBase::GetSpreadAngles");

// #ifdef PLATFORM_64BITS
// 			// SetRuntimeAttributeValue supports only 4 byte attributes, and on 64 bits, strings are 8 byte
// 			MOD_ADD_DETOUR_MEMBER(CSchemaAttributeType_String_BConvertStringToEconAttributeValue, "CSchemaAttributeType_String::BConvertStringToEconAttributeValue");
// #endif
			MOD_ADD_DETOUR_MEMBER(CAttributeList_SetRuntimeAttributeValue, "CAttributeList::SetRuntimeAttributeValue");
			MOD_ADD_DETOUR_MEMBER(CAttributeList_RemoveAttribute, "CAttributeList::RemoveAttribute");
			MOD_ADD_DETOUR_MEMBER(CAttributeList_RemoveAttributeByIndex, "CAttributeList::RemoveAttributeByIndex");
			MOD_ADD_DETOUR_MEMBER(CAttributeList_AddAttribute, "CAttributeList::AddAttribute");
 			MOD_ADD_DETOUR_MEMBER(CAttributeList_DestroyAllAttributes, "CAttributeList::DestroyAllAttributes");

			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_RemoveHealingTarget, "CWeaponMedigun::RemoveHealingTarget");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_StartHealingTarget, "CWeaponMedigun::StartHealingTarget");
			MOD_ADD_DETOUR_STATIC(FX_FireBullets, "FX_FireBullets");
            MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Energy_Recharge, "CTFWeaponBase::Energy_Recharge");
            MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_GetHealRate, "CWeaponMedigun::GetHealRate");
            MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_Explode, "CTFProjectile_EnergyBall::Explode");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_Taunt, "CTFPlayer::Taunt");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClearTauntAttack, "CTFPlayer::ClearTauntAttack");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyPunchImpulseX, "CTFPlayer::ApplyPunchImpulseX");			

            MOD_ADD_DETOUR_MEMBER(CTFPlayer_ForceRespawn, "CTFPlayer::ForceRespawn");	
			
            MOD_ADD_DETOUR_MEMBER(CTFGameMovement_ToggleParachute, "CTFGameMovement::ToggleParachute");	
            MOD_ADD_DETOUR_MEMBER(CTFGameMovement_HandleDuckingSpeedCrop, "CTFGameMovement::HandleDuckingSpeedCrop");	
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_HandleAnimEvent, "CTFPlayer::HandleAnimEvent");
            MOD_ADD_DETOUR_MEMBER(CAttributeManager_ProvideTo, "CAttributeManager::ProvideTo");
            MOD_ADD_DETOUR_MEMBER(CAttributeManager_StopProvidingTo, "CAttributeManager::StopProvidingTo");
            MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetParticleColor, "CTFWeaponBase::GetParticleColor");
            MOD_ADD_DETOUR_STATIC(CTFProjectile_EnergyRing_Create, "CTFProjectile_EnergyRing::Create");
            MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_ValidTargetPlayer, "CObjectSentrygun::ValidTargetPlayer");
            MOD_ADD_DETOUR_MEMBER(CTFJar_TossJarThink, "CTFJar::TossJarThink");
            MOD_ADD_DETOUR_MEMBER(CTFProjectile_ThrowableRepel_SetCustomPipebombModel, "CTFProjectile_ThrowableRepel::SetCustomPipebombModel");
            MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ItemHolsterFrame, "CTFWeaponBase::ItemHolsterFrame");
            MOD_ADD_DETOUR_MEMBER(CGameMovement_CheckFalling, "CGameMovement::CheckFalling [clone]");
            MOD_ADD_DETOUR_MEMBER(CTFWeaponInvis_ActivateInvisibilityWatch, "CTFWeaponInvis::ActivateInvisibilityWatch");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_FadeInvis, "CTFPlayerShared::FadeInvis");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_UpdateCloakMeter, "CTFPlayerShared::UpdateCloakMeter [clone]");
            //MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_InCond, "CTFPlayerShared::InCond");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_SpyDeadRingerDeath, "CTFPlayer::SpyDeadRingerDeath");
            MOD_ADD_DETOUR_MEMBER(CTFWeaponInvis_CleanupInvisibilityWatch, "CTFWeaponInvis::CleanupInvisibilityWatch");
            MOD_ADD_DETOUR_MEMBER(CTFWeaponInvis_GetViewModel, "CTFWeaponInvis::GetViewModel");
            MOD_ADD_DETOUR_MEMBER(CTFWearableDemoShield_DoCharge, "CTFWearableDemoShield::DoCharge");
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CObjectSapper_ApplyRoboSapperEffects_Last, "CObjectSapper::ApplyRoboSapperEffects [clone]", LOWEST);
            MOD_ADD_DETOUR_MEMBER(CCurrencyPack_MyTouch, "CCurrencyPack::MyTouch");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_TraceAttack, "CTFPlayer::TraceAttack");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_MakeBleed, "CTFPlayerShared::MakeBleed");
            MOD_ADD_DETOUR_MEMBER(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored");
            MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_SendWeaponAnim, "CTFWeaponBase::SendWeaponAnim");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_DeathNotice, "CTFGameRules::DeathNotice");
            MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_CreateTrail, "CTFProjectile_Arrow::CreateTrail");
            MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_BreakArrow, "CTFProjectile_Arrow::BreakArrow");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanDisguise, "CTFPlayer::CanDisguise");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanGoInvisible, "CTFPlayer::CanGoInvisible");
            MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_PlayerCanBeTeleported, "CObjectTeleporter::PlayerCanBeTeleported [clone]");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_RemoveAllOwnedEntitiesFromWorld, "CTFPlayer::RemoveAllOwnedEntitiesFromWorld");
			MOD_ADD_DETOUR_STATIC(CTFBaseRocket_Create, "CTFBaseRocket::Create");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_AddObject, "CTFPlayer::AddObject [clone]");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_RemoveObject, "CTFPlayer::RemoveObject");
            MOD_ADD_DETOUR_MEMBER(CObjectDispenser_GetHealRate, "CObjectDispenser::GetHealRate");
            MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_TeleporterThink, "CObjectTeleporter::TeleporterThink");
            MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_StartUpgrading, "CObjectSentrygun::StartUpgrading");
            MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_TeleporterSend, "CObjectTeleporter::TeleporterSend");
            MOD_ADD_VHOOK(CObjectSentrygun_FireBullets, TypeName<CObjectSentrygun>(), "CBaseEntity::FireBullets");
            MOD_ADD_DETOUR_MEMBER(CObjectDispenser_GetDispenserRadius, "CObjectDispenser::GetDispenserRadius");
            MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_SentryRotate, "CObjectSentrygun::SentryRotate");
            MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_Attack, "CObjectSentrygun::Attack");
            MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_SetModel, "CObjectSentrygun::SetModel");
            MOD_ADD_DETOUR_MEMBER(CObjectDispenser_SetModel, "CObjectDispenser::SetModel");
            MOD_ADD_DETOUR_MEMBER(CObjectTeleporter_SetModel, "CObjectTeleporter::SetModel");
			MOD_ADD_DETOUR_MEMBER(CTFBot_Event_Killed, "CTFBot::Event_Killed");
            MOD_ADD_VHOOK(CObjectSentrygun_UpdateOnRemove, TypeName<CObjectSentrygun>(), "CBaseEntity::UpdateOnRemove");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_EmitSentrySound, "CObjectSentrygun::EmitSentrySound");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_Spawn,    "CObjectSentrygun::Spawn");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_TestAndExpireChargeEffect,    "CTFPlayerShared::TestAndExpireChargeEffect");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFBot_ShouldGib,    "CTFBot::ShouldGib", HIGH);
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayer_ShouldGib,    "CTFPlayer::ShouldGib", HIGH);
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_GetSapperModelName,    "CObjectSapper::GetSapperModelName");
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_GetSapperSoundName,    "CObjectSapper::GetSapperSoundName");
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_UpdateOnRemove,    "CObjectSapper::UpdateOnRemove");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_FindSnapToBuildPos,    "CBaseObject::FindSnapToBuildPos");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetOpposingTFTeam,    "CTFPlayer::GetOpposingTFTeam");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Cleaver_GetDamage, "CTFProjectile_Cleaver::GetDamage");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_ThrowableRepel_GetDamage, "CTFProjectile_ThrowableRepel::GetDamage");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_ThrowableBrick_GetDamage, "CTFProjectile_ThrowableBrick::GetDamage");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_ThrowableBreadMonster_GetDamage, "CTFProjectile_ThrowableBreadMonster::GetDamage");
			MOD_ADD_DETOUR_MEMBER(CTFStunBall_GetDamage, "CTFStunBall::GetDamage");
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_IsValidRoboSapperTarget, "CObjectSapper::IsValidRoboSapperTarget");
			MOD_ADD_VHOOK(PlayerLocomotion_IsOnGround, "16PlayerLocomotion", "PlayerLocomotion::IsOnGround");
			MOD_ADD_DETOUR_MEMBER(CGameMovement_FullNoClipMove, "CGameMovement::FullNoClipMove");
			MOD_ADD_VHOOK(CTFCompoundBow_Deploy, TypeName<CTFCompoundBow>(), "CTFPipebombLauncher::Deploy");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerMove_SetupMove, "CTFPlayerMove::SetupMove");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Deploy, "CTFWeaponBase::Deploy");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_UpdateHands, "CTFWeaponBase::UpdateHands");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_FindNearestBuildPoint, "CBaseObject::FindNearestBuildPoint");
			MOD_ADD_DETOUR_MEMBER(CTFBotDeliverFlag_UpgradeOverTime, "CTFBotDeliverFlag::UpgradeOverTime");
			MOD_ADD_DETOUR_MEMBER(CServerGameClients_ClientPutInServer, "CServerGameClients::ClientPutInServer");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetMaxAmmo, "CTFPlayer::GetMaxAmmo");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBuilder_StartPlacement, "CTFWeaponBuilder::StartPlacement");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBuilder_StopPlacement, "CTFWeaponBuilder::StopPlacement");
			MOD_ADD_VHOOK2(CTFWeaponBuilder_TranslateViewmodelHandActivityInternal, TypeName<CTFWeaponBuilder>(), TypeName<CEconEntity>(), "CEconEntity::TranslateViewmodelHandActivityInternal");
			MOD_ADD_VHOOK(CTFWeaponBuilder_SendWeaponAnim, TypeName<CTFWeaponBuilder>(), "CTFWeaponBase::SendWeaponAnim");
			MOD_ADD_DETOUR_MEMBER(CEconItemView_GetAnimationSlot, "CEconItemView::GetAnimationSlot");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetViewModelWeaponRole, "CTFWeaponBase::GetViewModelWeaponRole");
			MOD_ADD_DETOUR_MEMBER(CEconItemDefinition_GetActivityOverride, "CEconItemDefinition::GetActivityOverride");
			MOD_ADD_DETOUR_MEMBER(CTFFlameManager_OnCollide, "CTFFlameManager::OnCollide");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_Heal, "CTFPlayerShared::Heal");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_SecondaryAttack, "CWeaponMedigun::SecondaryAttack");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_CycleResistType, "CWeaponMedigun::CycleResistType [clone]");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_FindAndHealTargets, "CWeaponMedigun::FindAndHealTargets");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_Smack, "CTFWeaponBaseMelee::Smack");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetSceneSoundToken, "CTFPlayer::GetSceneSoundToken");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ItemPostFrame, "CTFWeaponBase::ItemPostFrame");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ItemBusyFrame, "CTFWeaponBase::ItemBusyFrame");
			MOD_ADD_DETOUR_STATIC(CTFReviveMarker_Create, "CTFReviveMarker::Create");
			MOD_ADD_DETOUR_STATIC(TE_PlayerAnimEvent, "TE_PlayerAnimEvent");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_SetIdealActivity, "CBaseCombatWeapon::SetIdealActivity");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_Touch, "CBaseEntity::Touch");
			MOD_ADD_DETOUR_MEMBER(CCollisionEvent_PostCollision, "CCollisionEvent::PostCollision");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_HasPrimaryAmmo, "CBaseCombatWeapon::HasPrimaryAmmo");
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_PlayStepSound, "CBasePlayer::PlayStepSound");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTauntSucceeded, "CTFPlayer::OnTauntSucceeded");
			MOD_ADD_DETOUR_MEMBER(CGameMovement_PlayerMove, "CGameMovement::PlayerMove");
			MOD_ADD_DETOUR_MEMBER(CBaseObject_FindBuildPointOnPlayer, "CBaseObject::FindBuildPointOnPlayer [clone]");
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_FoundTarget, "CObjectSentrygun::FoundTarget");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGrenadeProj_Destroy, "CTFWeaponBaseGrenadeProj::Destroy");
			MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Destroy, "CTFBaseRocket::Destroy");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_CanHolster, "CTFWeaponBase::CanHolster");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Flare_Explode, "CTFProjectile_Flare::Explode");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Flare_GetDamageType, "CTFProjectile_Flare::GetDamageType");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_EmitSound_member, "CBaseEntity::EmitSound [member: normal]");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponFlameBall_FireProjectile, "CTFWeaponFlameBall::FireProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldGainInstantSpawn, "CTFPlayer::ShouldGainInstantSpawn");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsReadyToSpawn, "CTFPlayer::IsReadyToSpawn");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_MechanicalArmOrb_CheckForProjectiles, "CTFProjectile_MechanicalArmOrb::CheckForProjectiles");
			MOD_ADD_DETOUR_STATIC(HandleRageGain, "HandleRageGain");
			MOD_ADD_DETOUR_STATIC(TE_TFParticleEffect, "TE_TFParticleEffect [No attachment]");
			MOD_ADD_DETOUR_STATIC(UTIL_EntitiesInSphere, "UTIL_EntitiesInSphere");
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_DetonateStickies, "CTFGrenadePipebombProjectile::DetonateStickies");
			
            //MOD_ADD_VHOOK_INHERIT(CBaseProjectile_ShouldCollide, TypeName<CBaseProjectile>(), "CBaseEntity::ShouldCollide");
			
			
			
			
            //MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_GetCarryingRuneType, "CTFPlayerShared::GetCarryingRuneType");
            MOD_ADD_DETOUR_MEMBER(CVEngineServer_PlaybackTempEntity, "CVEngineServer::PlaybackTempEntity");
			

		//  Some optimization for attributes
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_IsPassiveWeapon, "CTFWeaponBase::IsPassiveWeapon");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_GetMaxClip1, "CBaseCombatWeapon::GetMaxClip1");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_AutoFiresFullClip, "CTFWeaponBase::AutoFiresFullClip");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetMaxHealthForBuffing, "CTFPlayer::GetMaxHealthForBuffing");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetMaxHealth, "CTFPlayer::GetMaxHealth");

		//  Fix burn time mult not working by making fire deal damage faster
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_Burn, "CTFPlayerShared::Burn");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_ConditionGameRulesThink, "CTFPlayerShared::ConditionGameRulesThink");
			
		//  Allow fire rate bonus on ball secondary attack
            MOD_ADD_DETOUR_MEMBER(CTFBat_Wood_SecondaryAttack, "CTFBat_Wood::SecondaryAttack");
			
		//  Allow fire rate bonus with reduced health on melee weapons
            MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ApplyFireDelay, "CTFWeaponBase::ApplyFireDelay");

		//  Implement disable alt fire
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_ItemPostFrame, "CBasePlayer::ItemPostFrame");

		//  Implement can overload for non rocket launchers
            MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_CheckReloadMisfire, "CTFWeaponBase::CheckReloadMisfire");
        //  Implement disable wrangler shield attribute
            MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_FindTarget, "CObjectSentrygun::FindTarget");
			

		//	Remove knife armor penetration limit
			MOD_ADD_DETOUR_MEMBER(CTFKnife_GetMeleeDamage, "CTFKnife::GetMeleeDamage");
			
			//Inspect custom attributes
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_InspectButtonPressed ,"CTFPlayer::InspectButtonPressed");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_UpdateOnRemove, "CTFPlayer::UpdateOnRemove");

			//MOD_ADD_DETOUR_MEMBER(CAttributeManager_OnAttributeValuesChanged, "CAttributeManager::OnAttributeValuesChanged");
			//MOD_ADD_DETOUR_MEMBER(CAttributeManager_OnAttributeValuesChanged, "CAttributeManager::OnAttributeValuesChanged");
			//MOD_ADD_DETOUR_MEMBER(CAttributeContainer_OnAttributeValuesChanged, "CAttributeContainer::OnAttributeValuesChanged");
			MOD_ADD_DETOUR_MEMBER(CAttributeContainerPlayer_OnAttributeValuesChanged, "CAttributeContainerPlayer::OnAttributeValuesChanged");
			//MOD_ADD_DETOUR_STATIC(CBaseEntity_EmitSound, "CBaseEntity::EmitSound [static: normal]");
			
		//	Allow explosive headshots on anything
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage, "CTFPlayer::OnTakeDamage");

		//	Allow parsing attributes stored as integer
			MOD_ADD_DETOUR_MEMBER(CSchemaAttributeType_Default_ConvertStringToEconAttributeValue, "CSchemaAttributeType_Default::ConvertStringToEconAttributeValue");

		//	Allow set bonus attributes on items, with the help of custom attributes
			MOD_ADD_DETOUR_MEMBER(static_attrib_t_BInitFromKV_SingleLine, "static_attrib_t::BInitFromKV_SingleLine");
			
		//	Fix build small sentries attribute reaplly max health on redeploy bug
			MOD_ADD_DETOUR_MEMBER(CObjectSentrygun_MakeScaledBuilding, "CObjectSentrygun::MakeScaledBuilding");

		//	Fix set dmgtype ignite
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetInitialAfterburnDuration, "CTFWeaponBase::GetInitialAfterburnDuration");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetAfterburnRateOnHit, "CTFWeaponBase::GetAfterburnRateOnHit");

		//	Fix reading string attribute on 64 bit
#ifdef PLATFORM_64BITS
			MOD_ADD_DETOUR_MEMBER(CEconItemAttributeIterator_ApplyAttributeString_OnIterateAttributeValue, "CEconItemAttributeIterator_ApplyAttributeString::OnIterateAttributeValue");
			MOD_ADD_DETOUR_MEMBER(CAttributeList_SetRuntimeAttributeRefundableCurrency, "CAttributeList::SetRuntimeAttributeRefundableCurrency");
#endif

		//  Fast attribute cache
			MOD_ADD_DETOUR_MEMBER(CAttributeManager_ClearCache,            "CAttributeManager::ClearCache [clone]");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CEconEntity_UpdateOnRemove,     "CEconEntity::UpdateOnRemove", LOWEST);
		}

		void LoadAttributes()
		{
			KeyValues *kv = new KeyValues("attributes");
			char path[PLATFORM_MAX_PATH];
			g_pSM->BuildPath(Path_SM,path,sizeof(path),"gamedata/sigsegv/custom_attributes.txt");
			if (kv->LoadFromFile(filesystem, path)) {
				DevMsg("Loaded attrs\n");
				CUtlVector<CUtlString> err;
				GetItemSchema()->BInitAttributes(kv, &err);
			}
			static bool attributeCallbackInstalled = false;
			if (!attributeCallbackInstalled) {
				attributeCallbackInstalled = true;
				RegisterCallback("mult_player_movespeed", OnMoveSpeedChange);
				RegisterCallback("mult_player_aiming_movespeed", OnMoveSpeedChange);
				RegisterCallback("major_mult_player_movespeed", OnMoveSpeedChange);
				RegisterCallback("mult_player_movespeed_shieldrequired", OnMoveSpeedChange);
				RegisterCallback("mult_player_movespeed_active", OnMoveSpeedChange);
				RegisterCallback("add_maxhealth", OnMaxHealthChange);
				RegisterCallback("add_maxhealth_nonbuffed", OnMaxHealthChange);
				RegisterCallback("item_color_rgb", OnItemColorChange);
				RegisterCallback("is_invisible", OnItemColorChange);
				RegisterCallback("attachment_name", OnItemColorChange);
				RegisterCallback("attachment_scale", OnItemColorChange);
				RegisterCallback("attachment_offset", OnItemColorChange);
				RegisterCallback("attachment_angles", OnItemColorChange);
				RegisterCallback("custom_item_model", OnCustomModelChange);
				RegisterCallback("is_miniboss", OnMiniBossChange);
				RegisterCallback("model_scale", OnScaleChange);
				RegisterCallback("reload_full_clip_at_once", OnReloadFullClipAtOnceChange);
				RegisterCallback("mult_maxammo_primary", OnPrimaryAmmoChange);
				RegisterCallback("mult_maxammo_secondary", OnSecondaryAmmoChange);
				RegisterCallback("mult_maxammo_grenades1", OnGrenadeAmmoChange);
				RegisterCallback("mult_maxammo_metal", OnMetalChange);
				RegisterCallback("custom_view_model", OnCustomViewModelChange);
				RegisterCallback("paintkit_proto_def_index", OnPaintkitChange);
				RegisterCallback("no_clip", OnNoClipChange);
				RegisterCallback("add_attributes_when_active", OnAddAttributesWhenActive);
				RegisterCallback("medigun_particle", OnMedigunBeamChange);
				RegisterCallback("medigun_particle_enemy", OnMedigunBeamChange);
				RegisterCallback("medigun_particle_release", OnMedigunBeamChange);
				RegisterCallback("medigun_particle_spark", OnMedigunBeamChange);
				RegisterCallback("alt_fire_attributes", OnAltFireAttributesChange);
				RegisterCallback("fov_override", OnFOVChange);
				RegisterCallback("hud_overlay", OnOverlayChange);
				RegisterCallback("addcond_immunity", OnAddCondImmunity);
				RegisterCallback("attribute_immunity", OnAttributeImmunity);
				RegisterCallback("custom_item_model_attachment", OnCustomItemModelAttachment);
				RegisterCallback("custom_item_model_attachment_viewmodel", OnCustomItemModelAttachment);
				RegisterCallback("arrow_ignite", OnArrowIgnite);
				RegisterCallback("provide_on_active", OnProvideOnActive);
				
			}
		}

		virtual bool OnLoad() override
		{
			if (GetItemSchema() != nullptr)
				LoadAttributes();
			
			const char *game_path = g_SMAPI->GetBaseDir();
			char path_sm[PLATFORM_MAX_PATH];
        	g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/disallowed_viewmodels/");
			disallowed_viewmodel_path = path_sm;
			mkdir(path_sm, 0766);

			viewmodels_enabled_forward = forwards->CreateForward("SIG_IsCustomViewmodelEnabled", ET_Hook, 1, NULL, Param_Cell);
			viewmodels_toggle_forward = forwards->CreateForward("SIG_ToggleCustomViewmodel", ET_Hook, 2, NULL, Param_Cell, Param_Cell);
			return true;
		}

		virtual void LevelInitPostEntity() override
		{
			precached.clear();
		}

		virtual void LevelInitPreEntity() override
		{
			precached.clear();
			LoadAttributes();
			respawnTimesForId.clear();
		}
		virtual bool ShouldReceiveCallbacks() const override { return true; }

		virtual void OnUnload() override
		{
			forwards->ReleaseForward(viewmodels_enabled_forward);
			forwards->ReleaseForward(viewmodels_toggle_forward);
			precached.clear();
		}
		
		virtual void OnDisable() override
		{
			precached.clear();
		}

		virtual void OnEnable() override
		{
			
		}

		virtual void FrameUpdatePostEntityThink() override
		{
			if (!respawnTimesForId.empty() && TFGameRules()->State_Get() != GR_STATE_RND_RUNNING) {
				respawnTimesForId.clear();
			}

			if (gpGlobals->tickcount % 16 == 0) { 
				ForEachTFPlayer([&](CTFPlayer *player){
					static bool in_upgrade_zone[MAX_PLAYERS + 1];

					int index = ENTINDEX(player) - 1;

					if (player->IsBot() || index > MAX_PLAYERS) return;

					if (player->m_Shared->m_bInUpgradeZone) {
						in_upgrade_zone[index] = true;
						if (player->GetOrCreateEntityModule<AttributeInfoModule>("attributeinfo")->m_Strings.empty()) {
							InspectAttributes(player, player, false);
						} 
					}
					else if (in_upgrade_zone[index]) {
						in_upgrade_zone[index] = false;
						player->GetOrCreateEntityModule<AttributeInfoModule>("attributeinfo")->m_Strings.clear();
						ClearAttributeDisplay(player);
					}
				});
			}

			if (gpGlobals->tickcount % 16 == 5) { 
				ForEachTFPlayer([&](CTFPlayer *player){
					auto speed = GetFastAttributeFloat(player, 1.0, MOVE_SPEED_AS_HEALTH_DECREASES);
					if (speed != 1.0f) {
						player->TeamFortress_SetSpeed();
					}
				});
			}

			for (size_t i = 0; i < model_entries.size(); ) {
				auto &entry = model_entries[i];
				if (entry.weapon == nullptr || entry.weapon->IsMarkedForDeletion() || entry.weapon->GetOwnerEntity() == nullptr) {
					if (entry.wearable != nullptr)
						entry.wearable->Remove();
					if (entry.wearable_vm != nullptr)
						entry.wearable_vm->Remove();
					
					model_entries.erase(model_entries.begin() + i);
					continue;
				}
				
				if (entry.weapon->m_bBeingRepurposedForTaunt) {
					entry.weapon->SetRenderMode(kRenderNormal);
					entry.weapon->SetRenderColorA(255);
					entry.temporaryVisible = true;
				}
				else if (entry.temporaryVisible) {
					entry.weapon->SetRenderMode(kRenderTransAlpha);
					entry.weapon->SetRenderColorA(0);
					entry.temporaryVisible = false;
				}

				if ((entry.wearable_vm == nullptr && entry.createVMWearable) || entry.wearable == nullptr) {
					if (entry.wearable != nullptr)
						entry.wearable->Remove();
					if (entry.wearable_vm != nullptr)
						entry.wearable_vm->Remove();

					CreateWeaponWearables(entry);
				}
				if (entry.weapon->IsEffectActive(EF_NODRAW) || entry.temporaryVisible || entry.weapon->m_iState != WEAPON_IS_ACTIVE) {
					/*if (entry.wearable != nullptr)
						entry.wearable->Remove();
					if (entry.wearable_vm != nullptr)
						entry.wearable_vm->Remove();*/
					if (entry.wearable != nullptr && !entry.wearable->IsEffectActive(EF_NODRAW))
						entry.wearable->SetEffects(entry.wearable->GetEffects() | EF_NODRAW);
					if (entry.wearable_vm != nullptr && !entry.wearable_vm->IsEffectActive(EF_NODRAW))
						entry.wearable_vm->SetEffects(entry.wearable_vm->GetEffects() | EF_NODRAW);
				}
				else {
					/*if (entry.wearable == nullptr) {
						CreateWeaponWearables(entry);
					}*/
					if (entry.wearable != nullptr && entry.wearable->IsEffectActive(EF_NODRAW))
						entry.wearable->SetEffects(entry.wearable->GetEffects() & ~(EF_NODRAW));
					if (entry.wearable_vm != nullptr && entry.wearable_vm->IsEffectActive(EF_NODRAW))
						entry.wearable_vm->SetEffects(entry.wearable_vm->GetEffects() & ~(EF_NODRAW));
				}
				i++;
			}
			for (size_t i = 0; i < stick_info.size(); ) {
				auto &entry = stick_info[i];
				if (entry.sticky == nullptr) {
					stick_info.erase(stick_info.begin() + i);
					continue;
				}
				auto phys = entry.sticky->VPhysicsGetObject();
				if (entry.sticked == nullptr || !entry.sticked->IsAlive()) {
					if (phys != nullptr) {
						phys->EnableMotion(true);
					}
					stick_info.erase(stick_info.begin() + i);
					continue;
				}
				if (phys != nullptr && phys->IsMotionEnabled()) {
					stick_info.erase(stick_info.begin() + i);
					continue;
				}
				entry.sticky->SetAbsOrigin(entry.sticked->GetAbsOrigin() + entry.offset);
				i++;
			}
			for (auto entry : AutoList<CustomModelAttachment>::List()) {
				bool visible = !entry->me->IsEffectActive(EF_NODRAW) && !entry->me->m_bBeingRepurposedForTaunt && entry->me->m_iState == WEAPON_IS_ACTIVE;
				if (entry->prevStateActive != visible) {
					for (auto &[def, ent] : entry->models) {
						if (ent != nullptr) {
							ent->SetEffects(visible ? ent->GetEffects() & (~EF_NODRAW) : ent->GetEffects() | EF_NODRAW);
						}
					}
				}
				entry->prevStateActive = visible;
			}

			for (auto entry : AutoList<MedigunCustomBeamModule>::List()) {
				auto medigun = entry->medigun;
				auto owner = medigun->GetTFPlayerOwner();
				auto healTarget = medigun->GetHealTarget();
				if (owner == nullptr) {
					continue;
				}

				string_t preferredParticle = NULL_STRING;
				if (healTarget != nullptr) {

					bool enemy = false;
					if (owner->GetTeamNumber() != healTarget->GetTeamNumber()) {
						float attackEnemy = 0;
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( medigun, attackEnemy, medigun_attack_enemy );
						enemy = attackEnemy != 0;
					}
					if (healTarget->IsBaseObject()) {
						preferredParticle = entry->effectObjects;
					}
					else if (enemy) {
						preferredParticle = owner->GetTeamNumber() == TF_TEAM_BLUE ? entry->effectEnemyBlu : entry->effectEnemyRed;
					}
					else if (medigun->IsChargeReleased()) {
						preferredParticle = owner->GetTeamNumber() == TF_TEAM_BLUE ? entry->effectReleaseBlu : entry->effectReleaseRed;
					}
					else if (owner->IsRealPlayer() && healTarget->IsPlayer()) {
						preferredParticle = owner->GetTeamNumber() == TF_TEAM_BLUE ? entry->effectTargetBlu : entry->effectTargetRed;
					}
					else {
						preferredParticle = owner->GetTeamNumber() == TF_TEAM_BLUE ? entry->effectBlu : entry->effectRed;
					}
				}

				if (entry->particles != nullptr && (healTarget != entry->prevTarget || healTarget == nullptr || preferredParticle != entry->lastEffect)) {
					if (entry->effectImmediateDestroy) {
						entry->particles->SetParent(nullptr, -1);
						entry->particles->SetAbsOrigin(Vector(16384,16384,16384));
						THINK_FUNC_SET(entry->particles, ProjectileLifetime, gpGlobals->curtime + 0.1f);
					}
					else {
						entry->particles->Remove();
					}	
					entry->particles = nullptr;

					if (entry->particlesAttach != nullptr) {
						if (entry->effectImmediateDestroy) {
							entry->particlesAttach->SetParent(nullptr, -1);
							entry->particlesAttach->SetAbsOrigin(Vector(16384,16384,16384));
							THINK_FUNC_SET(entry->particlesAttach, ProjectileLifetime, gpGlobals->curtime + 0.1f);
							entry->particlesAttach = nullptr;
						}
						else {
							entry->particlesAttach->Remove();
						}	
					}
				}
				if (healTarget != nullptr && entry->particles == nullptr) {
					
					auto particlesAttach = static_cast<CParticleSystem *>(CreateEntityByName("info_particle_system"));
					particlesAttach->m_iszEffectName = preferredParticle;
					particlesAttach->SetAbsOrigin(healTarget->WorldSpaceCenter() + Vector(0, 0, 4));
					particlesAttach->SetParent(healTarget, -1);
					particlesAttach->SetLocalAngles(vec3_angle);
					entry->particlesAttach = particlesAttach;
					DispatchSpawn(particlesAttach);
					particlesAttach->Activate();

					auto particles = static_cast<CParticleSystem *>(CreateEntityByName("info_particle_system"));
					particles->m_iszEffectName = preferredParticle;
					particles->m_hControlPointEnts.SetIndex(particlesAttach, 0);
					particles->m_bStartActive = true;
					DispatchSpawn(particles);
					particles->Activate();
					entry->particles = particles;
					particles->SetParent(owner, -1);
					particles->SetLocalOrigin(Vector(0,-8.0f,owner->CollisionProp()->OBBMaxs().z * 0.65f));
					particles->SetLocalAngles(vec3_angle);
					entry->prevTarget = healTarget;
					StopParticleEffects(entry->medigun);
					if (owner->GetViewModel() != nullptr)
						StopParticleEffects(owner->GetViewModel());
					entry->lastEffect = preferredParticle;
				}
				bool displayCharge = !owner->m_Shared->InCond(TF_COND_TAUNTING) && owner->GetActiveWeapon() == entry->medigun && (entry->medigun->IsChargeReleased() || entry->medigun->GetCharge() >= 1.0f);
				if (entry->chargeEffect != nullptr && !displayCharge) {
					entry->chargeEffect->Remove();
					entry->chargeEffect = nullptr;
				}
				if (entry->chargeEffect == nullptr && displayCharge) {
					auto particles = static_cast<CParticleSystem *>(CreateEntityByName("info_particle_system"));
					particles->m_iszEffectName = owner->GetTeamNumber() == TF_TEAM_BLUE ? entry->effectSparkBlu : entry->effectSparkRed;
					particles->m_bStartActive = true;
					DispatchSpawn(particles);
					particles->Activate();
					entry->chargeEffect = particles;
					particles->SetParent(entry->medigun, -1);
					particles->SetLocalOrigin(Vector(27.0f,-8.0f,owner->CollisionProp()->OBBMaxs().z * 0.22f));
					particles->SetLocalAngles(vec3_angle);
				}
				if (entry->particles != nullptr || entry->chargeEffect != nullptr) {
					StopParticleEffects(entry->medigun);
					if (owner->GetViewModel() != nullptr)
						StopParticleEffects(owner->GetViewModel());
				}
				
			}
			// The function does not make use of this pointer, so its safe to convert to CAttributeManager
			static int last_cache_version = 0;
            int cache_version = reinterpret_cast<CAttributeManager *>(this)->GetGlobalCacheVersion();

            if(last_cache_version != cache_version) {
                for (int i = 0; i < 2048; i++) {
					auto cache = fast_attribute_cache[i];
					if (cache != nullptr) {
						int count = i <= gpGlobals->maxClients ? (int)ATTRIB_COUNT_PLAYER : (int)ATTRIB_COUNT_ITEM;
						for(int i = 0; i < count; i++) {
							cache[i] = FLT_MIN;
						}
					}
                }
                last_cache_version = cache_version;
            }
		}
		
		virtual std::vector<std::string> GetRequiredMods() { return {"Common:Weapon_Shoot", "Item:Item_Common"};}
	};
	CMod s_Mod;

	ModCommandClient sig_defaulthands("sig_defaulthands", [](CCommandPlayer *player, const CCommand& args){
		players_informed_about_viewmodel[player->entindex()] = true;
		auto &disallowed = players_viewmodel_disallowed[player->entindex()];
		disallowed = !disallowed;

		char path[512];
		snprintf(path, 512, "%s%llu", disallowed_viewmodel_path.c_str(), player->GetSteamID().ConvertToUint64());
		if (disallowed) {
			auto file = fopen(path, "w");
			if (file) {
				fclose(file);
			}
			ForEachTFPlayerEconEntity(player, [](CEconEntity *ent){
				if (ent->MyCombatWeaponPointer() != nullptr) {
					ent->MyCombatWeaponPointer()->SetCustomViewModel("");
					ent->MyCombatWeaponPointer()->m_iViewModelIndex = CBaseEntity::PrecacheModel(ent->MyCombatWeaponPointer()->GetViewModel());
					ent->MyCombatWeaponPointer()->SetModel(ent->MyCombatWeaponPointer()->GetViewModel());
				}
			});
		}
		else {
			remove(path);
		}

		players_viewmodel_informed_about_disallowed[player->entindex()] = true;
		viewmodels_toggle_forward->PushCell(player->entindex());
		viewmodels_toggle_forward->PushCell(disallowed);
		viewmodels_toggle_forward->Execute();
		ModCommandResponse("%s\n", TranslateText(player, disallowed ? "Custom hand models disabled apply" : "Custom hand models enabled apply"));
	}, &s_Mod);
	
	ConVar cvar_enable("sig_attr_custom", "0", FCVAR_NOTIFY,
		"Mod: enable custom attributes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

}
