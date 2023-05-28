#include "mod.h"
#include "re/nextbot.h"
#include "stub/nextbot_cc.h"
#include "stub/nextbot_cc_behavior.h"
#include "stub/nextbot_etc.h"
#include "re/path.h"
#include "stub/tfbot.h"
#include "stub/gamerules.h"
#include "stub/tfentities.h"
#include "util/iterate.h"
#include "mod/common/commands.h"
#include "stub/extraentitydata.h"
#include "util/misc.h"
#include "util/value_override.h"
#include "mod/ai/npc_nextbot/npc_nextbot_actions.h"
#include "mod/ai/npc_nextbot/npc_nextbot.h"
#include "mod/etc/mapentity_additions.h"
#include "mod/item/item_common.h"
#include "stub/server.h"
#include "stub/usermessages_sv.h"
#include "stub/particles.h"

namespace Mod::AI::NPC_Nextbot
{
    
    // VHOOK_DECL(void, CBaseEntity_GetServerClass, int team)
    // {
    //     VHOOK_CALL(CBotNPCArcher_ChangeTeam)(team);
    //     auto me = reinterpret_cast<CBotNPCArcher *>(this);
    //     if (team == TF_TEAM_RED) {
    //         me->m_nSkin = 0;
    //     }
    //     else if (team == TF_TEAM_BLUE) {
    //         me->m_nSkin = 1;
    //     }
    // }
    // CVirtualHook 
    GlobalThunk<ServerClass> g_CBaseAnimating_ClassReg("g_CBaseAnimating_ClassReg");
    Mod::Etc::Mapentity_Additions::ClassnameFilter bot_filter("$bot_npc", {
		{"GiveWeapon"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            if (Value.Entity() == nullptr) {
                auto item = CreateItemByName(nullptr, Value.String());
                auto weapon = rtti_cast<CTFWeaponBaseGun *>(item);
                if (item != nullptr) {
                    
                    item->NetworkProp()->m_pServerClass = &g_CBaseAnimating_ClassReg.GetRef();
                    item->m_nSkin = bot->GetTeamNumber() == TF_TEAM_RED ? 0 : 1;
                    item->ChangeTeam(bot->GetTeamNumber());
                    if (weapon != nullptr) {
                        bot->Weapon_Equip(weapon);
                        bot->Weapon_Switch(weapon);
                    }
                    else {
                        item->SetParent(bot, -1);
                        item->SetLocalOrigin(vec3_origin);
                        item->SetOwnerEntity(bot);
                        item->AddEffects(EF_BONEMERGE|EF_BONEMERGE_FASTCULL);
                    }
                    item->UpdateBodygroups(bot, true);
                    //((CBasePlayer *)pActivator)->Weapon_Equip(weapon);
                    //((CBasePlayer *)pActivator)->Weapon_Switch(weapon);
                    //weapon->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop")->props["m_hOwnerEntity"] = {Variant(pActivator), Variant(pActivator)};
                    //weapon->SetOwnerEntity(pActivator);
                    //weapon->SetOwner(pActivator);
                    //weapon->m_bValidatedAttachedEntity = true;
                }
            }
            else if (Value.Entity()->MyCombatWeaponPointer() != nullptr){
                Value.Entity()->NetworkProp()->m_pServerClass = &g_CBaseAnimating_ClassReg.GetRef();
                bot->Weapon_Equip(Value.Entity()->MyCombatWeaponPointer());
                bot->Weapon_Switch(Value.Entity()->MyCombatWeaponPointer());
                Value.Entity()->MyCombatWeaponPointer()->m_nSkin = bot->GetTeamNumber() == TF_TEAM_RED ? 0 : 1;
                Value.Entity()->MyCombatWeaponPointer()->UpdateBodygroups(bot, true);
            }
        }},
		{"RemoveWeapon"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            if (Value.Entity() == nullptr) {
                for (int i = 0; i < 48; i++) {
                    auto weapon = bot->GetWeapon(i);
                    if (weapon != nullptr) {
                        if (FStrEq(GetItemName(weapon->GetItem()), Value.String())) {
                            weapon->UpdateBodygroups(bot, false);
                            weapon->Remove();
                        }
                    }
                }
            }
            else {
                for (int i = 0; i < 48; i++) {
                    auto weapon = bot->GetWeapon(i);
                    if (weapon == Value.Entity()) {
                        weapon->UpdateBodygroups(bot, false);
                        weapon->Remove();
                    }
                }
            }
        }},
		{"RemoveAllWeapons"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            for (int i = 0; i < 48; i++) {
                auto weapon = bot->GetWeapon(i);
                if (weapon != nullptr) {
                    weapon->UpdateBodygroups(bot, false);
                    weapon->Remove();
                }
            }
        }},
		{"SetHealth"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->SetHealth(Value.Int());
        }},
		{"SetMaxHealth"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->SetMaxHealth(Value.Int());
        }},
		{"StartActivity"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            bot->GetBodyInterface()->StartActivity((Activity)CAI_BaseNPC::GetActivityID(Value.String()), 0);
        }},
		{"StartSequence"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            auto mod = bot->GetOrCreateEntityModule<MyNextbotModule>("mynextbotmodule");
            auto seq = bot->LookupSequence(Value.String());
            if (seq >= 0) {
                mod->m_forcedActivity = ACT_HL2MP_IDLE_RPG;
                mod->m_untranslatedActivity = ACT_HL2MP_IDLE_RPG;
                ((CBotNPCBody *)bot->GetBodyInterface())->m_currentActivity = ACT_HL2MP_IDLE_RPG;
                bot->m_nSequence = seq;
                bot->m_flPlaybackRate = 1.0f;
                bot->SetCycle( 0 );
                bot->ResetSequenceInfo();
            }
        }},
		{"AddGestureActivity"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            Msg("add %d\n", CAI_BaseNPC::GetActivityID(Value.String()));
            bot->AddGesture((Activity)CAI_BaseNPC::GetActivityID(Value.String()));
        }},
		{"RestartGestureActivity"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            bot->RestartGesture((Activity)CAI_BaseNPC::GetActivityID(Value.String()));
        }},
		{"AddGestureSequence"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            auto seq = bot->LookupSequence(Value.String());
            if (seq >= 0) {
                bot->AddGestureSequence(seq);
            }
        }},
		{"RemoveGesture"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            bot->RemoveGesture((Activity)CAI_BaseNPC::GetActivityID(Value.String()));
        }},
		{"RemoveAllGestures"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            bot->RemoveAllGestures();
        }},
		{"StopActivityOrSequence"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            auto mod = bot->GetOrCreateEntityModule<MyNextbotModule>("mynextbotmodule");
            mod->m_forcedActivity = ACT_INVALID;
        }},
		{"SwitchClass"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto *bot = static_cast<CBotNPCArcher *>(ent);
            auto mod = bot->GetOrCreateEntityModule<MyNextbotModule>("mynextbotmodule");
            bot->GetEntityModule<MyNextbotModule>("mynextbotmodule")->TransformToClass
                (GetClassIndexFromString(Value.String()));
        }}
    });

	INextBot *my_current_nextbot = nullptr;
	IBody *my_current_nextbot_body = nullptr;
	ILocomotion *my_current_nextbot_locomotion = nullptr;
	IVision *my_current_nextbot_vision = nullptr;
	MyNextbotModule *my_current_nextbot_module = nullptr;
	MyNextbotEntity *my_current_nextbot_entity = nullptr;
    bool upkeep_current_nextbot = false;
    const char *current_kill_icon = nullptr;

	ConVar *nb_head_aim_steady_max_rate;
	ConVar *nb_head_aim_resettle_angle;
	ConVar *nb_head_aim_resettle_time;
	ConVar *nb_head_aim_settle_duration;

    void *MyBotVTable[512];

    void *MyBodyVTable[160];

    void *MyLocomotionVTable[160];

    void *MyVisionVTable[160];

    bool create_my_nextbot = false;
    CBotNPCArcher *CreateMyNextbot()
    {
        CBotNPCArcher *mybot;
        {
            mybot = reinterpret_cast<CBotNPCArcher *>(CreateEntityByName("$bot_npc"));
            DispatchSpawn(mybot);
            mybot->Activate();
        }
        return mybot;
    }

    ModCommandAdmin spawn_nextbot("sig_spawn_nextbot", [](CCommandPlayer *player, const CCommand &command){
        auto times = atoi(command[1]);
        for (int i = 0; i < times; i++) {
            auto mybot = CreateMyNextbot();
            if (player != nullptr) {
                trace_t trace;
                FireEyeTrace(trace, player);
                Vector pos = trace.endpos + Vector(RandomFloat(-sqrt(20 * times), sqrt(20 * times)), RandomFloat(-sqrt(20 * times), sqrt(20 * times)), 0);
                mybot->Teleport(&pos, nullptr, nullptr);
            }
        }
    });

	DETOUR_DECL_MEMBER(void, CBotNPCArcher_CBotNPCArcher)
	{
		DETOUR_MEMBER_CALL(CBotNPCArcher_CBotNPCArcher)();
        auto bot = reinterpret_cast<CBotNPCArcher *>(this);
        if (create_my_nextbot) {
            bot->SetCollisionGroup(COLLISION_GROUP_PLAYER_MOVEMENT);
            auto mod = bot->GetOrCreateEntityModule<MyNextbotModule>("mynextbotmodule");
            bot->m_bow = (CBaseAnimating *) mod;
            uintptr_t ptrdif = *((void ***)bot->MyNextBotPointer()) -*((void ***)bot);
            *((void ***)bot) = MyBotVTable+4;
            *((void ***)bot->GetBodyInterface()) = MyBodyVTable+4;
            *((void ***)bot->GetLocomotionInterface()) = MyLocomotionVTable+4;
            *((void ***)bot->GetVisionInterface()) = MyVisionVTable+4;
            *((void ***)bot->MyNextBotPointer()) = MyBotVTable+4+ptrdif;
        }
	}

	DETOUR_DECL_MEMBER(void, CBotNPCArcher_Spawn)
	{
        auto bot = reinterpret_cast<CBotNPCArcher *>(this);
        auto preHealth = bot->GetHealth();
        auto preMaxHealth = bot->GetMaxHealth();
		DETOUR_MEMBER_CALL(CBotNPCArcher_Spawn)();
        if (FStrEq(bot->GetClassname(), "$bot_npc")) {
            if (bot->m_bow != nullptr) {
                bot->m_bow.Get()->Remove();
            }
            auto mod = bot->GetEntityModule<MyNextbotModule>("mynextbotmodule");
            bot->m_bow = (CBaseAnimating *) mod;

            if (preHealth > 0) {
                bot->SetHealth(preHealth);
                bot->SetMaxHealth(preMaxHealth);
            }

            for (int i = 0; i < TF_AMMO_COUNT; i++) {
                bot->SetAmmoCount(5000, i);
            }
            mod->TransformToClass(GetClassIndexFromString(bot->GetCustomVariable<"class">("sniper")));

	        bot->m_fFlags |= FL_NPC;
            UTIL_SetSize(bot, &VEC_HULL_MIN, &VEC_HULL_MAX);
        }
	}

	DETOUR_DECL_MEMBER(void, CBotNPCArcherIntention_CBotNPCArcherIntention, CBotNPCArcher *zombie)
	{
		DETOUR_MEMBER_CALL(CBotNPCArcherIntention_CBotNPCArcherIntention)(zombie);
        if (create_my_nextbot) {
            auto intention = reinterpret_cast<CBotNPCArcherIntention *>(this);
            intention->m_behavior->SetAction(new CMyNextbotMainAction());
        }
    }

	DETOUR_DECL_MEMBER(void, CBotNPCArcherIntention_Reset)
	{
		DETOUR_MEMBER_CALL(CBotNPCArcherIntention_Reset)();
        auto intention = reinterpret_cast<CBotNPCArcherIntention *>(this);
        if (intention->GetBot()->GetEntity()->GetEntityModule<MyNextbotModule>("mynextbotmodule") != nullptr) {
            intention->m_behavior->SetAction(new CMyNextbotMainAction());
        }
    }

	DETOUR_DECL_MEMBER(bool, CTraceFilterIgnorePlayers_ShouldHitEntity, IHandleEntity *pServerEntity, int contentsMask)
	{
		auto result = DETOUR_MEMBER_CALL(CTraceFilterIgnorePlayers_ShouldHitEntity)(pServerEntity, contentsMask);
        if (result) {
            auto entity = EntityFromEntityHandle(pServerEntity);
            if (entity != nullptr && PStr<"$bot_npc">() == entity->GetClassname()) return false;
        }
        return result;
    }

	DETOUR_DECL_MEMBER(const char *, CTFGameRules_GetKillingWeaponName, const CTakeDamageInfo &info, CTFPlayer *pVictim, int *iWeaponID)
	{
		
		if (current_kill_icon != nullptr) {
			return current_kill_icon;
		}
		auto result = DETOUR_MEMBER_CALL(CTFGameRules_GetKillingWeaponName)(info, pVictim, iWeaponID);
		return result;
	}
    
	DETOUR_DECL_MEMBER(void, NextBotGroundLocomotion_ApplyAccumulatedApproach)
	{
        DETOUR_MEMBER_CALL(NextBotGroundLocomotion_ApplyAccumulatedApproach)();
        auto loco = reinterpret_cast<NextBotGroundLocomotion *>(this);
        Msg("apply %f %f\n", loco->m_moveVector.x, loco->m_moveVector.y);
    }
		
    

    VHOOK_DECL(int, CBotNPCArcher_OnTakeDamage, const CTakeDamageInfo& rawInfo)
	{
		auto ent = reinterpret_cast<MyNextbotEntity *>(this);
		auto mod = ent->GetEntityModule<MyNextbotModule>("mynextbotmodule");
        if (!mod->m_bAllowTakeFriendlyFire && rawInfo.GetAttacker() != nullptr && rawInfo.GetAttacker()->GetTeamNumber() == ent->GetTeamNumber()) {
            return 0;
        }
        CTakeDamageInfo info = rawInfo;
        TFGameRules()->ApplyOnDamageModifyRules(info, ent, true);

        // On damage Rage
        // Give the soldier/pyro some rage points for dealing/taking damage.
        if ( info.GetDamage() && info.GetAttacker() != ent )
        {
            CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );

            // Buff flag 1: we get rage when we deal damage. Here, that means the soldier that attacked
            // gets rage when we take damage.
            HandleRageGain( pAttacker, 1, info.GetDamage(), 6.0f );

            // Buff 5: our pyro attacker get rage when we're damaged by fire
            if ( ( info.GetDamageType() & DMG_BURN ) != 0 || ( info.GetDamageType() & DMG_PLASMA ) != 0 )
            {
                HandleRageGain( pAttacker, 8, info.GetDamage(), 30.f );
            }

            if ( pAttacker && info.GetWeapon() )
            {
                CTFWeaponBase *pWeapon = rtti_cast<CTFWeaponBase *>(info.GetWeapon());
                if ( pWeapon )
                {
                    pWeapon->ApplyOnHitAttributes(ent, pAttacker, info);
                }
            }
        }

        return VHOOK_CALL(CBotNPCArcher_OnTakeDamage)(info);
    }

    VHOOK_DECL(int, CBotNPCArcher_OnTakeDamage_Alive, const CTakeDamageInfo& rawInfo)
	{
		auto ent = reinterpret_cast<MyNextbotEntity *>(this);
        auto mod = ent->GetEntityModule<MyNextbotModule>("mynextbotmodule");
		float preHealth = ent->GetHealth();

	    CTakeDamageInfo info = rawInfo;
        CTFGameRules::DamageModifyExtras_t outParams;
		info.SetDamage( TFGameRules()->ApplyOnDamageAliveModifyRules( info, ent, outParams ) );

		int dmg = VHOOK_CALL(CBotNPCArcher_OnTakeDamage_Alive)(info);
        
		if (preHealth - ent->GetHealth() > 0) {
			IGameEvent *event = gameeventmanager->CreateEvent("npc_hurt");
			if ( event )
			{

				event->SetInt("entindex", ENTINDEX(ent));
				event->SetInt("health", Max(0, ent->GetHealth()));
				event->SetInt("damageamount", preHealth - ent->GetHealth());
				event->SetBool("crit", (info.GetDamageType() & DMG_CRITICAL) ? true : false);

				CTFPlayer *attackerPlayer = ToTFPlayer(info.GetAttacker());
				if (attackerPlayer)
				{
					event->SetInt("attacker_player", attackerPlayer->GetUserID());

					if ( attackerPlayer->GetActiveTFWeapon() )
					{
						event->SetInt("weaponid", attackerPlayer->GetActiveTFWeapon()->GetWeaponID());
					}
					else
					{
						event->SetInt("weaponid", 0);
					}
				}
				else
				{
					// hurt by world
					event->SetInt("attacker_player", 0);
					event->SetInt("weaponid", 0);
				}

				gameeventmanager->FireEvent(event);
			}
            float duration;
            if (gpGlobals->curtime > mod->m_flNextHurtSoundTime) {
                
                CPASFilter filter( ent->GetAbsOrigin() );

                EmitSound_t params;
                params.m_pSoundName = mod->m_strHurtSound;
                params.m_flSoundTime = 0.0f;
                params.m_pflSoundDuration = &duration;
                params.m_bWarnOnDirectWaveReference = true;
                params.m_nChannel = CHAN_VOICE;
                params.m_nSpeakerEntity = ENTINDEX(ent);
                CBaseEntity::EmitSound(filter, ENTINDEX(ent), params);
                //ent->EmitSound(mod->m_strHurtSound, 0, &duration);
                mod->m_flNextHurtSoundTime = gpGlobals->curtime + duration + 1.0f;
            }
            static Activity ACT_MP_GESTURE_FLINCH_CHEST = (Activity) CAI_BaseNPC::GetActivityID("ACT_MP_GESTURE_FLINCH_CHEST");
            ent->AddGesture(ACT_MP_GESTURE_FLINCH_CHEST);
		}
        if (ent->GetHealth() <= 0) {
            ent->EmitSound(mod->m_strDeathSound);
        }
		return dmg;
	}

    
    inline void SetCurrentNextbot(INextBot *nextbot)
    {
        my_current_nextbot = nextbot;
        my_current_nextbot_body = nextbot != nullptr ? nextbot->GetBodyInterface() : nullptr;
        my_current_nextbot_locomotion = nextbot != nullptr ? nextbot->GetLocomotionInterface() : nullptr;
        my_current_nextbot_vision = nextbot != nullptr ? nextbot->GetVisionInterface() : nullptr;
        my_current_nextbot_module = nextbot != nullptr ? nextbot->GetEntity()->GetEntityModule<MyNextbotModule>("mynextbotmodule") : nullptr;
        my_current_nextbot_entity = nextbot != nullptr ? (MyNextbotEntity *)nextbot->GetEntity() : nullptr;
    }
    
    VHOOK_DECL(void, CBotNPCArcher_Upkeep)
    {
        //Msg("Main Upkeep\n");
        auto nextbot = reinterpret_cast<INextBot *>(this);
        VHOOK_CALL(CBotNPCArcher_Upkeep)();
    }

    
    VHOOK_DECL(void, CBotNPCArcher_Update)
    {
        //Msg("Main Update\n");
        auto nextbot = reinterpret_cast<INextBot *>(this);
        SetCurrentNextbot(nextbot);
        auto mod = nextbot->GetEntity()->GetEntityModule<MyNextbotModule>("mynextbotmodule");
        mod->Update();
        if (!mod->m_bFreeze) {
            VHOOK_CALL(CBotNPCArcher_Update)();
        }
        SetCurrentNextbot(nullptr);
    }
    
    VHOOK_DECL(uint, CBotNPCArcher_PhysicsSolidMaskForEntity)
    {
        int contents = MASK_PLAYERSOLID & ~CONTENTS_PLAYERCLIP;
        auto mod = reinterpret_cast<CBotNPCArcher *>(this)->GetEntityModule<MyNextbotModule>("mynextbotmodule");
        if (!mod->m_bIgnoreClips) {
            contents |= CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP;
        }
        return contents;
    }
    
    VHOOK_DECL(bool, CBotNPCArcher_ShouldCollide, int collisionGroup, int contentsMask)
    {
        if (collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT) {
            auto mod = reinterpret_cast<CBotNPCArcher *>(this)->GetEntityModule<MyNextbotModule>("mynextbotmodule");
            if (mod->m_bNotSolidToPlayers) {
                return false;
            }

            auto team = reinterpret_cast<CBotNPCArcher *>(this)->GetTeamNumber();
            if (team != TF_TEAM_RED && team != TF_TEAM_BLUE && (contentsMask & (CONTENTS_REDTEAM | CONTENTS_BLUETEAM)) == (CONTENTS_REDTEAM | CONTENTS_BLUETEAM)) {
                return false;
            }
            if (team != TF_TEAM_RED && team != TF_TEAM_BLUE && (contentsMask & (CONTENTS_REDTEAM | CONTENTS_BLUETEAM)) != 0) {
                return true;
            }
            // Don't collide with other non teamed
        }
        return VHOOK_CALL(CBotNPCArcher_ShouldCollide)(collisionGroup,contentsMask );
    }

    
    VHOOK_DECL(void, CBotNPCArcher_ChangeTeam, int team)
    {
        VHOOK_CALL(CBotNPCArcher_ChangeTeam)(team);
        auto me = reinterpret_cast<CBotNPCArcher *>(this);
        bool zombie = GetNextbotModule(me)->m_bIsZombie;
        if (team == TF_TEAM_RED) {
            me->m_nSkin = zombie ? 4 : 0;
        }
        else if (team == TF_TEAM_BLUE) {
            me->m_nSkin = zombie ? 5 : 1;
        }
        
        if (GetNextbotModule(me)->m_hZombieCosmetic != nullptr) {
            GetNextbotModule(me)->m_hZombieCosmetic->m_nSkin = team == TF_TEAM_RED ? 0 : 1;
        }
    }
    
    VHOOK_DECL(void, CBotNPCArcher_TraceAttack, const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, void *pAccumulator)
    {
		auto ent = reinterpret_cast<MyNextbotEntity *>(this);
        auto mod = ent->GetEntityModule<MyNextbotModule>("mynextbotmodule");
        if (!mod->m_bAllowTakeFriendlyFire && info.GetAttacker() != nullptr && info.GetAttacker()->GetTeamNumber() == ent->GetTeamNumber()) {
            return;
        }
        CTakeDamageInfo info_modified = info;
        if (info_modified.GetDamageType() & DMG_USE_HITLOCATIONS && ptr->hitgroup == HITGROUP_HEAD)
	    {
            info_modified.AddDamageType(DMG_CRITICAL);
        }
        return VHOOK_CALL(CBotNPCArcher_TraceAttack)(info_modified,vecDir, ptr, pAccumulator );
    }
    
    VHOOK_DECL(const QAngle &, CBotNPCArcher_EyeAngles)
    {
        return GetNextbotModule(reinterpret_cast<MyNextbotEntity *>(this))->m_currentAimAngles;
    }

    VHOOK_DECL(void, CBotNPCArcher_RefreshCollisionBounds)
    {
        VHOOK_CALL(CBotNPCArcher_RefreshCollisionBounds)();
        auto me = reinterpret_cast<CBotNPCArcher *>(this);
        GetNextbotModule(me)->SetEyeOffset();
    }

    VHOOK_DECL(bool, CBotNPCArcher_ShouldGib, const CTakeDamageInfo &info)
    {
        auto me = reinterpret_cast<CBotNPCArcher *>(this);
        return GetNextbotModule(me)->m_DeathEffectType == MyNextbotModule::GIB;
    }
    
    
    // DETOUR_DECL_MEMBER(bool, CTraceFilterSimple_ShouldHitEntity, IHandleEntity *pServerEntity, int contentsMask)
	// {
	// 	if (rc_CTFWeaponBase_DeflectEntity > 0 && cvar_fix.GetBool()) {
	// 		auto filter = reinterpret_cast<CTraceFilterSimple *>(this);
			
	// 		CTFPlayer *player = ToTFPlayer(EntityFromEntityHandle(const_cast<IHandleEntity *>(filter->GetPassEntity())));
	// 		if (player != nullptr) {
	// 			CBaseEntity *ent = EntityFromEntityHandle(pServerEntity);
	// 			if (ent != nullptr && (ent->IsPlayer() || ent->IsBaseObject() || ent->IsCombatItem()) &&
	// 				ent->GetTeamNumber() == player->GetTeamNumber()) {
	// 				return false;
	// 			}
	// 		}
	// 	}
		
	// 	return DETOUR_MEMBER_CALL(CTraceFilterSimple_ShouldHitEntity)(pServerEntity, contentsMask);
	// }

    template<FixedString lit>
    inline void LoadStringSoundFromVariable(CBaseEntity *entity, const char *&target, const char *defaultValue)
    {
        const char *prevSound = target;
		target = entity->GetCustomVariable<lit>(defaultValue);
        if (prevSound != target && target[0] != '\0') {
            if (!enginesound->PrecacheSound(target, true))
				CBaseEntity::PrecacheScriptSound(target);
        }
    }

    inline void LoadStringSoundFromVariable2(const char *&target, variant_t &var)
    {
        const char *prevSound = target;
		target = GetVariantValueConvert<const char *>(var);
        if (prevSound != target && target[0] != '\0') {
            if (!enginesound->PrecacheSound(target, true))
				CBaseEntity::PrecacheScriptSound(target);
        }
    }
    
    const char *player_hurt_sounds[] = {"", "Scout.PainSevere01", "Sniper.PainSevere01", "Soldier.PainSevere01", "Demoman.PainSevere01"
        , "Medic.PainSevere01", "Heavy.PainSevere01", "Pyro.PainSevere01", "Spy.PainSevere01", "Engineer.PainSevere01", "Scout.PainSevere01"};

    const char *player_death_sounds[] = {"", "Scout.Death", "Sniper.Death", "Soldier.Death", "Demoman.Death"
        , "Medic.Death", "Heavy.Death", "Pyro.Death", "Spy.Death", "Engineer.Death", "Scout.Death"};

    const char *giant_step_sounds[] = {"", "MVM.GiantScoutStep", "MVM.GiantSniperStep", "MVM.GiantSoldierStep", "MVM.GiantDemomanStep"
        , "MVM.GiantMedicStep", "MVM.GiantHeavyStep", "MVM.GiantPyroStep", "MVM.GiantSpyStep", "MVM.GiantEngineerStep", "MVM.GiantScoutStep"};

    const char *robot_hurt_sounds[] = {"", "Scout.MVM_PainSevere01", "Sniper.MVM_PainSevere01", "Soldier.MVM_PainSevere01", "Demoman.MVM_PainSevere01"
        , "Medic.MVM_PainSevere01", "Heavy.MVM_PainSevere01", "Pyro.MVM_PainSevere01", "Spy.MVM_PainSevere01", "Engineer.MVM_PainSevere01", "Scout.MVM_PainSevere01"};

    const char *robot_death_sounds[] = {"", "Scout.MVM_Death", "Sniper.MVM_Death", "Soldier.MVM_Death", "Demoman.MVM_Death"
        , "Medic.MVM_Death", "Heavy.MVM_Death", "Pyro.MVM_Death", "Spy.MVM_Death", "Engineer.MVM_Death", "Scout.MVM_Death"};
        
    void MyNextbotModule::SetEyeOffset()
    {
        if (this->m_bForceEyeOffset) {
            this->m_pEntity->m_eyeOffset = this->m_bForceEyeOffset;
        }
        else {
            this->m_pEntity->m_eyeOffset = this->m_posture == IBody::PostureType::CROUCH ? VEC_DUCK_VIEW_SCALED(this->m_pEntity)  : g_TFClassViewVectors[this->m_iPlayerClass] * this->m_pEntity->GetModelScale();
        }
    }

    void MyNextbotModule::TransformToClass(int classIndex)
    {
        if (classIndex == 0) {
            classIndex = RandomInt(1, TF_CLASS_COUNT-1);
        }
        if (classIndex >= 1 && classIndex < TF_CLASS_COUNT) {
            this->m_bIsRobot = this->m_pEntity->GetCustomVariableBool<"robot">();
            this->m_bIsRobotGiant = this->m_pEntity->GetCustomVariableBool<"giant">();
            this->m_bIsZombie = this->m_pEntity->GetCustomVariableBool<"zombie">();
            auto data = GetPlayerClassData(classIndex);
            this->m_iPlayerClass = classIndex;
            this->m_strPlayerClass = AllocPooledString(g_aPlayerClassNames_NonLocalized[classIndex]);
            this->m_flMaxRunningSpeed = data->m_flMaxSpeed;
            this->m_pEntity->SetHealth(data->m_nMaxHealth);
            this->m_pEntity->SetMaxHealth(data->m_nMaxHealth);
            auto oldPosture = this->m_pEntity->GetBodyInterface()->GetActualPosture();
            this->m_pEntity->GetBodyInterface()->SetDesiredPosture(IBody::PostureType::STAND);
            const char *model = data->m_szModelName;
            if (!this->m_bIsZombie && this->m_bIsRobot) {
                model = g_szBotModels[classIndex];
                this->m_strFootstepSound = "MVM.BotStep";
                CBaseEntity::PrecacheScriptSound(this->m_strFootstepSound);
            }
            if (this->m_bIsRobotGiant) {
                if (!this->m_bIsZombie) {
                    model = g_szBotBossModels[classIndex];
                }
                this->m_strFootstepSound = giant_step_sounds[classIndex];
                CBaseEntity::PrecacheScriptSound(this->m_strFootstepSound);
            }
            if (this->m_hZombieCosmetic != nullptr) {
                this->m_hZombieCosmetic->Remove();
            }
            CBaseEntity::PrecacheModel(model);
            
            if (this->m_bIsZombie) {
                this->m_hZombieCosmetic = (CBaseAnimating *) CreateEntityByName("prop_dynamic");
                this->m_hZombieCosmetic->SetModel(CFmtStr("models/player/items/%s/%s_zombie.mdl", g_aRawPlayerClassNamesShort[classIndex], g_aRawPlayerClassNamesShort[classIndex]));
                this->m_hZombieCosmetic->SetParent(this->m_pEntity, -1);
                this->m_hZombieCosmetic->AddEffects(EF_BONEMERGE | EF_BONEMERGE_FASTCULL);
                this->m_hZombieCosmetic->SetLocalOrigin(vec3_origin);
                this->m_hZombieCosmetic->SetLocalAngles(vec3_angle);
                this->m_hZombieCosmetic->m_nSkin = this->m_pEntity->GetTeamNumber() == TF_TEAM_RED ? 0 : 1;
                
                this->m_pEntity->m_nSkin = this->m_pEntity->GetTeamNumber() == TF_TEAM_RED ? 4 : 5;
            }

            this->m_pEntity->SetModel(model);
            UTIL_SetSize(this->m_pEntity, &VEC_HULL_MIN, &VEC_HULL_MAX);
            this->SetEyeOffset();
            this->m_pEntity->GetBodyInterface()->SetDesiredPosture(oldPosture);
            this->m_flAcceleration = data->m_flMaxSpeed*10;
            this->m_flDeceleration = data->m_flMaxSpeed*10;
            Msg("acceleration set to %f\n", this->m_flAcceleration);
            this->m_strHurtSound = this->m_bIsRobot ? robot_hurt_sounds[classIndex] : player_hurt_sounds[classIndex];
            enginesound->PrecacheSound(this->m_strHurtSound);
            this->m_strDeathSound = this->m_bIsRobot ? robot_death_sounds[classIndex] : player_death_sounds[classIndex];
            enginesound->PrecacheSound(this->m_strDeathSound);

            this->m_pEntity->SetCustomVariable("class", Variant(m_strPlayerClass));
            this->m_pEntity->SetCustomVariable("classindex", Variant(classIndex));

            if (this->m_bIsRobot) {
                auto skill = this->m_pEntity->GetCustomVariable<"skill">(PStr<"normal">());
                Vector color = (skill == PStr<"normal">() || skill == PStr<"easy">() ? Vector( 0, 240, 255 ) : Vector( 255, 180, 36));
                Vector colorNorm = color / 255;
                StopParticleEffects(this->m_pEntity);
                CRecipientFilter filter;
                filter.AddAllPlayers();
                te_tf_particle_effects_control_point_t cp = { PATTACH_ABSORIGIN, color };

                const char *eye1 = this->m_bIsRobotGiant ? "eye_boss_1" : "eye_1";
                DispatchParticleEffect("bot_eye_glow", PATTACH_POINT_FOLLOW, this->m_pEntity, eye1, vec3_origin, false, colorNorm, colorNorm, true, false, &cp, &filter);
                const char *eye2 = this->m_bIsRobotGiant ? "eye_boss_2" : "eye_2";
                DispatchParticleEffect("bot_eye_glow", PATTACH_POINT_FOLLOW, this->m_pEntity, eye2, vec3_origin, false, colorNorm, colorNorm, true, false, &cp, &filter);
            }

            this->Update();
        }
    }

    void MyNextbotModule::Update()
    {
        auto entity = this->m_pEntity;
        bool trackingset = false;
        bool forceDuck = false;
        string_t skill = PStrT<"normal">();
        auto prevNotSolid = this->m_bNotSolidToPlayers;
        for (auto &var : entity->GetExtraEntityData()->GetCustomVariables()) {
            if (var.key == PStrT<"movespeed">()) this->m_flMaxRunningSpeed = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"duckmovespeed">()) this->m_flDuckSpeed = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"acceleration">()) this->m_flAcceleration = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"deceleration">()) this->m_flDeceleration =GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"jumpheight">()) this->m_flJumpHeight = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"gravity">()) this->m_flGravity = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"skill">()) skill = GetVariantValueConvert<string_t>(var.value);
            else if (var.key == PStrT<"aimtrackinginterval">()) {trackingset = true; this->m_flHeadTrackingInterval = GetVariantValueConvert<float>(var.value);}
            else if (var.key == PStrT<"aimleadtime">()) this->m_flHeadAimLeadTime = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"headrotatespeed">()) this->m_flMaxHeadAngularVelocity = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"autojumpmintime">()) this->m_flAutoJumpMinTime = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"autojumpmaxtime">()) this->m_flAutoJumpMaxTime = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"forceduck">()) forceDuck = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"forceshoot">()) this->m_bShootingForced = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"disableshoot">()) this->m_bShootingDisabled = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"freeze">()) this->m_bFreeze = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"ignoreenemies">()) this->m_bIgnoreEnemies = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"dodge">()) this->m_bDodge = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"notsolidtoplayers">()) this->m_bNotSolidToPlayers = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"ignoreclips">()) this->m_bIgnoreClips = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"allowtakefriendlyfire">()) this->m_bAllowTakeFriendlyFire = GetVariantValueConvert<bool>(var.value);
            
            else if (var.key == PStrT<"handattackdamage">()) this->m_flHandAttackDamage = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"handattacktime">()) this->m_flHandAttackTime = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"handattackrange">()) this->m_flHandAttackRange = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"handattacksize">()) this->m_flHandAttackSize = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"handattackforce">()) this->m_flHandAttackForce = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"handattacksmacktime">()) this->m_flHandAttackSmackTime = GetVariantValueConvert<float>(var.value);
            else if (var.key == PStrT<"handattackcleave">()) this->m_bHandAttackCleave = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"handattackicon">()) this->m_strHandAttackIcon = GetVariantValueConvert<const char *>(var.value);
            else if (var.key == PStrT<"handattacksound">()) LoadStringSoundFromVariable2(this->m_strHandAttackSound, var.value);
            else if (var.key == PStrT<"handattacksoundmiss">()) LoadStringSoundFromVariable2(this->m_strHandAttackSoundMiss, var.value);

            else if (var.key == PStrT<"crits">()) this->m_bCrit = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"hurtsound">()) LoadStringSoundFromVariable2(this->m_strHurtSound, var.value);
            else if (var.key == PStrT<"deathsound">()) LoadStringSoundFromVariable2(this->m_strDeathSound, var.value);
            else if (var.key == PStrT<"nofootsteps">()) this->m_bNoFootsteps = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"footstepsound">()) LoadStringSoundFromVariable2(this->m_strFootstepSound, var.value);

            else if (var.key == PStrT<"waituntilfullreload">()) this->m_bWaitUntilFullReload = GetVariantValueConvert<bool>(var.value);
            else if (var.key == PStrT<"deatheffecttype">()) {
                auto str = GetVariantValueConvert<string_t>(var.value);
                if (str == PStrT<"gib">()) {
                    this->m_DeathEffectType = GIB;
                }
                else if (str == PStrT<"ragdoll">()) {
                    this->m_DeathEffectType = RAGDOLL;
                }
                else if (str == PStrT<"default">()) {
                    this->m_DeathEffectType = DEFAULT;
                }
                else if (str == PStrT<"none">()) {
                    this->m_DeathEffectType = NONE;
                }
            }
            
        }
        if (!trackingset) {
            if (skill == PStrT<"normal">()) {
                this->m_flHeadTrackingInterval = 0.25f;
            }
            else if (skill == PStrT<"easy">()) {
                this->m_flHeadTrackingInterval = 1.0f;
            }
            else if (skill == PStrT<"hard">()) {
                this->m_flHeadTrackingInterval = 0.1f;
            }
            else if (skill == PStrT<"expert">()) {
                this->m_flHeadTrackingInterval = 0.05f;
            }
        }

        this->m_flAutoJumpMaxTime = Max(this->m_flAutoJumpMaxTime, this->m_flAutoJumpMinTime);

        if (forceDuck) {
            entity->GetBodyInterface()->SetDesiredPosture(IBody::PostureType::CROUCH);
            this->m_bDuckForced = true;
        }
        else if (this->m_bDuckForced && !forceDuck) {
            this->m_bDuckForced = false;
            entity->GetBodyInterface()->SetDesiredPosture(IBody::PostureType::STAND);
        }

        this->SetShooting(this->m_bShootingForced && !this->m_bShootingDisabled);

        if (this->m_bNotSolidToPlayers && !prevNotSolid) {
            entity->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop")->props["m_CollisionGroup"] = {Variant((int)COLLISION_GROUP_WEAPON), Variant((int)COLLISION_GROUP_WEAPON)};
        }
        else if (prevNotSolid && entity->GetEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop") != nullptr){
            entity->GetEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop")->props.erase("m_CollisionGroup");
        }
		this->m_flHandAttackSmackTime = Min(this->m_flHandAttackTime - gpGlobals->interval_per_tick, this->m_flHandAttackSmackTime);
    }

    float MyNextbotModule::GetAttackRange() {
        if (this->m_flHandAttackTime > 0.0f) {
            return this->m_flHandAttackRange;
        }
        else {
            return 1024.0f;
        }
    }

    class MyNPCFactory : public IEntityFactory
    {
    public:
        MyNPCFactory(IEntityFactory *orig) : orig(orig) {}

        virtual IServerNetworkable *Create( const char *pClassName ) {
            
            create_my_nextbot = true;
            auto result = orig->Create(pClassName);
            create_my_nextbot = false;
            return result;

        }
        virtual void Destroy( IServerNetworkable *pNetworkable ) {orig->Destroy(pNetworkable);}
        virtual size_t GetEntitySize() {return orig->GetEntitySize();}

        IEntityFactory *orig;
    };

    class CEntityFactoryDictionary : public IEntityFactoryDictionary
    {
    public:
        CUtlDict< IEntityFactory *, unsigned short > m_Factories;
    };

    class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:My_Nextbot")
		{
			MOD_ADD_DETOUR_MEMBER(CBotNPCArcher_CBotNPCArcher,                   "CBotNPCArcher::CBotNPCArcher");
			MOD_ADD_DETOUR_MEMBER(CBotNPCArcher_Spawn,                   "CBotNPCArcher::Spawn");
			MOD_ADD_DETOUR_MEMBER(CBotNPCArcherIntention_CBotNPCArcherIntention, "CBotNPCArcherIntention::CBotNPCArcherIntention");
			MOD_ADD_DETOUR_MEMBER(CBotNPCArcherIntention_Reset,            "CBotNPCArcherIntention::Reset");
			MOD_ADD_DETOUR_MEMBER(CTraceFilterIgnorePlayers_ShouldHitEntity,            "CTraceFilterIgnorePlayers::ShouldHitEntity");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetKillingWeaponName,            "CTFGameRules::GetKillingWeaponName");
			//MOD_ADD_DETOUR_MEMBER(NextBotGroundLocomotion_ApplyAccumulatedApproach,            "NextBotGroundLocomotion::ApplyAccumulatedApproach");
            
            
            
		}
        virtual bool EnableByDefault() override { return true; }

        virtual bool OnLoad() override
        {

            nb_head_aim_steady_max_rate = g_pCVar->FindVar("nb_head_aim_steady_max_rate");
            nb_head_aim_resettle_angle = g_pCVar->FindVar("nb_head_aim_resettle_angle");
            nb_head_aim_resettle_time = g_pCVar->FindVar("nb_head_aim_resettle_time");
            nb_head_aim_settle_duration = g_pCVar->FindVar("nb_head_aim_settle_duration");

            auto botVtable = RTTI::GetVTable(TypeName<CBotNPCArcher>());
            memcpy(MyBotVTable, botVtable-4, sizeof(MyBotVTable));

            CVirtualHook hooks[] = {
                CVirtualHook(TypeName<CBotNPCArcher>(), "INextBot::Upkeep", GET_VHOOK_CALLBACK(CBotNPCArcher_Upkeep), GET_VHOOK_INNERPTR(CBotNPCArcher_Upkeep)),
                CVirtualHook(TypeName<CBotNPCArcher>(), "INextBot::Update", GET_VHOOK_CALLBACK(CBotNPCArcher_Update), GET_VHOOK_INNERPTR(CBotNPCArcher_Update)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CBaseEntity>(), "CBaseEntity::PhysicsSolidMaskForEntity", GET_VHOOK_CALLBACK(CBotNPCArcher_PhysicsSolidMaskForEntity), GET_VHOOK_INNERPTR(CBotNPCArcher_PhysicsSolidMaskForEntity)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CBaseEntity>(), "CBaseEntity::ShouldCollide", GET_VHOOK_CALLBACK(CBotNPCArcher_ShouldCollide), GET_VHOOK_INNERPTR(CBotNPCArcher_ShouldCollide)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CBaseEntity>(), "CBaseEntity::ChangeTeam", GET_VHOOK_CALLBACK(CBotNPCArcher_ChangeTeam), GET_VHOOK_INNERPTR(CBotNPCArcher_ChangeTeam)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CBaseEntity>(), "CBaseEntity::OnTakeDamage", GET_VHOOK_CALLBACK(CBotNPCArcher_OnTakeDamage), GET_VHOOK_INNERPTR(CBotNPCArcher_OnTakeDamage)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CTFPlayer>(), "CTFPlayer::OnTakeDamage_Alive", GET_VHOOK_CALLBACK(CBotNPCArcher_OnTakeDamage_Alive), GET_VHOOK_INNERPTR(CBotNPCArcher_OnTakeDamage_Alive)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CBaseEntity>(), "CBaseEntity::TraceAttack", GET_VHOOK_CALLBACK(CBotNPCArcher_TraceAttack), GET_VHOOK_INNERPTR(CBotNPCArcher_TraceAttack)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CBaseEntity>(), "CBaseEntity::EyeAngles", GET_VHOOK_CALLBACK(CBotNPCArcher_EyeAngles), GET_VHOOK_INNERPTR(CBotNPCArcher_EyeAngles)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CBaseAnimating>(), "CBaseAnimating::RefreshCollisionBounds", GET_VHOOK_CALLBACK(CBotNPCArcher_RefreshCollisionBounds), GET_VHOOK_INNERPTR(CBotNPCArcher_RefreshCollisionBounds)),
                CVirtualHook(TypeName<CBotNPCArcher>(), TypeName<CBaseCombatCharacter>(), "CBaseCombatCharacter::ShouldGib", GET_VHOOK_CALLBACK(CBotNPCArcher_ShouldGib), GET_VHOOK_INNERPTR(CBotNPCArcher_ShouldGib))
                
            };
            for (auto &hook : hooks) {
                hook.DoLoad();
                hook.AddToVTable(MyBotVTable+4);
            }

            auto bodyVtable = RTTI::GetVTable(TypeName<CBotNPCBody>());
            memcpy(MyBodyVTable, bodyVtable-4, sizeof(MyBodyVTable));

            auto locomotionVtable = RTTI::GetVTable("23NextBotGroundLocomotion");
            memcpy(MyLocomotionVTable, locomotionVtable-4, sizeof(MyLocomotionVTable));

            auto visionVtable = RTTI::GetVTable("7IVision");
            memcpy(MyVisionVTable, visionVtable-4, sizeof(MyVisionVTable));

            LoadBodyHooks(MyBodyVTable+4);
            LoadLocomotionHooks(MyLocomotionVTable+4);
            LoadVisionHooks(MyVisionVTable+4);
            
            servertools->GetEntityFactoryDictionary()->InstallFactory(new MyNPCFactory(servertools->GetEntityFactoryDictionary()->FindFactory("bot_npc_archer")), "$bot_npc");
            return true;
        }

        virtual void OnUnload() override
        {
            ((CEntityFactoryDictionary *)servertools->GetEntityFactoryDictionary())->m_Factories.Remove("$bot_npc");
        }

        virtual void OnDisable() override
        {
            std::vector<CBaseEntity *> toRemove;
            ForEachEntityByClassname("$bot_npc", [&](CBaseEntity *entity){
                toRemove.push_back(entity);
            });
            for(auto entity : toRemove) {
                servertools->RemoveEntityImmediate(entity);
            }
        }

		virtual std::vector<std::string> GetRequiredMods() { return {"Common:Weapon_Shoot"};}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_my_nextbot", "0", FCVAR_NOTIFY,
		"Mod: create more nexbots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

}