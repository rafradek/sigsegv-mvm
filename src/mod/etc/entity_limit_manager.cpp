#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/projectiles.h"
#include "stub/tfweaponbase.h"
#include "stub/tfentities.h"
#include "stub/server.h"
#include "stub/extraentitydata.h"
#include "util/pooled_string.h"
#include "util/iterate.h"
#include "util/clientmsg.h"
#include "util/admin.h"
#include "re/nextbot.h"
#include "mod/etc/entity_limit_manager.h"

namespace Mod::Etc::Entity_Limit_Manager
{
    class DisposableEntityModule : public EntityModule, public AutoList<DisposableEntityModule>
    {
    public:
        DisposableEntityModule(CBaseEntity *entity) : EntityModule(entity), me(entity) {}

        CBaseEntity *me;
    };

    void MarkEntityAsDisposable(CBaseEntity *entity) {
        entity->GetOrCreateEntityModule<DisposableEntityModule>("disposableentity");
    }

    ConVar sig_etc_entity_limit_manager_viewmodel("sig_etc_entity_limit_manager_viewmodel", "1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Remove usused offhand viewmodel entity for other classes than spy");

    ConVar sig_etc_entity_limit_manager_remove_expressions("sig_etc_entity_limit_manager_remove_expressions", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Remove expressions for bots (like yelling when firing minigun). Reduces entity count by 1 per bot");

    DETOUR_DECL_MEMBER(void, CTFPlayer_Spawn)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this); 
		
		DETOUR_MEMBER_CALL();
		// Remove unused offhand viewmodel for other classes than spy
		if (sig_etc_entity_limit_manager_viewmodel.GetBool() && !player->IsPlayerClass(TF_CLASS_SPY) && player->GetViewModel(1) != nullptr) {
			player->GetViewModel(1)->Remove();
		}
    }

    DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_Equip, CBaseCombatCharacter *owner)
	{
		auto ent = reinterpret_cast<CBaseCombatWeapon *>(this);
        DETOUR_MEMBER_CALL(owner);
		// Restore unused offhand viewmodel for other classes than spy

		auto playerOwner = ToTFPlayer(owner);
		if (sig_etc_entity_limit_manager_remove_expressions.GetBool() && playerOwner != nullptr && playerOwner->GetViewModel(1) == nullptr && ent->m_nViewModelIndex == 1) {
			playerOwner->CreateViewModel(1);
		}
    }

    DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_SetViewModelIndex, int index)
	{
		auto ent = reinterpret_cast<CBaseCombatWeapon *>(this);
		// Restore unused offhand viewmodel for other classes than spy
        if (index == 1) {
            auto player = ToTFPlayer(ent->GetOwnerEntity());
            if (player != nullptr && player->GetViewModel(1) == nullptr) {
                player->CreateViewModel(1);
            }
        }
        DETOUR_MEMBER_CALL(index);
    }
    
    RefCount rc_CTFPlayer_UpdateExpression;
    DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateExpression)
	{
        auto player = reinterpret_cast<CTFPlayer *>(this);
        SCOPED_INCREMENT_IF(rc_CTFPlayer_UpdateExpression, sig_etc_entity_limit_manager_remove_expressions.GetBool() && player->IsBot());
        DETOUR_MEMBER_CALL();
    }

    CTFPlayer *disguise_weapon_player = nullptr;
    DETOUR_DECL_MEMBER(void, CTFPlayerShared_DetermineDisguiseWeapon, bool forcePrimary)
	{
        auto shared = reinterpret_cast<CTFPlayerShared *>(this);
        disguise_weapon_player = shared->GetOuter();
        CHandle<CTFWeaponBase> prevDisguiseWeapon = shared->m_hDisguiseWeapon.Get();
        DETOUR_MEMBER_CALL(forcePrimary);
        if (prevDisguiseWeapon != shared->m_hDisguiseWeapon && prevDisguiseWeapon != nullptr) {
            prevDisguiseWeapon->Remove();
        }
        disguise_weapon_player = nullptr;
    }

    CTFPlayer *disguise_wearables_player = nullptr;
    DETOUR_DECL_MEMBER(void, CTFPlayerShared_DetermineDisguiseWearables)
	{
        disguise_wearables_player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
        DETOUR_MEMBER_CALL();
        disguise_wearables_player = nullptr;
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_CreateDisguiseWeaponList, CTFPlayer *target)
	{   
        disguise_weapon_player = reinterpret_cast<CTFPlayer *>(this);
        DETOUR_MEMBER_CALL(target);
        disguise_weapon_player = nullptr;
    }

    DETOUR_DECL_MEMBER(void, CTFPlayerShared_RemoveDisguiseWeapon, CTFPlayer *target)
	{

    }
   
    CBaseEntity *simulated_entity = nullptr;
    CBaseEntity *remove_this_scene_entity = nullptr;
    DETOUR_DECL_STATIC(float, InstancedScriptedScene, CBaseFlex *pActor, const char *pszScene, EHANDLE *phSceneEnt,
							 float flPostDelay, bool bIsBackground, AI_Response *response,
							 bool bMultiplayer, IRecipientFilter *filter)
	{
        //Msg("pActor %d scene %s\n", ENTINDEX(pActor), pszScene);
        //if (strstr(pszScene,"idleloop") != nullptr) { return 10;}
        if (rc_CTFPlayer_UpdateExpression) return 10;
        
        //auto oldSceneEnt = *phSceneEnt;
        auto ret = DETOUR_STATIC_CALL(pActor, pszScene, phSceneEnt, flPostDelay, bIsBackground, response, bMultiplayer, filter);
        //if (remove_this_scene_entity != nullptr) {
        //    remove_this_scene_entity->Remove();
        //    *phSceneEnt = oldSceneEnt;
        //    remove_this_scene_entity = nullptr;
        //}
        return ret;
    }

    std::unordered_set<CBaseEntity *> removed_entities_immediate;
    GlobalThunk<CBitVec< MAX_EDICTS >> g_FreeEdicts("g_FreeEdicts");
    GlobalThunk<CUtlVector<IServerNetworkable*>> g_DeleteList("g_DeleteList");
    GlobalThunk<bool> g_bDisableEhandleAccess("g_bDisableEhandleAccess");
    GlobalThunk<int> s_RemoveImmediateSemaphore("s_RemoveImmediateSemaphore");
	DETOUR_DECL_STATIC(CBaseEntity *, CreateEntityByName, const char *className, int iForceEdictIndex)
	{
        int entityCount = engine->GetEntityCount();
        // Check if there is a useable free edict. If not, try to find a recently freed edict and make it useable
        bool success = false;
        if (reinterpret_cast<CGameServer *>(sv)->GetNumEdicts() == MAX_EDICTS) {
            //CFastTimer timer;
            //timer.Start();
            int iBit = -1;
            auto worldEdict = INDEXENT(0);
            edict_t *edictToUse = nullptr;
            auto &freeEdicts = g_FreeEdicts.GetRef();
            float svtime = sv->GetTime();
            for ( ;; )
            {
                iBit = freeEdicts.FindNextSetBit( iBit + 1 );
                if ( iBit < 0 )
                    break;
                auto edict = worldEdict + iBit;
                if ((edict->freetime < 2 ) || ( svtime - edict->freetime >= 1.0)) {
                    // There is an useable edict
                    edictToUse = nullptr;
                    success = true;
                    break;
                }
                else {
                    edictToUse = edict;
                    
                }

            }
            // make it useable
            if (edictToUse != nullptr) {
                success = true;
                edictToUse->freetime = 0;
            }
            // Try to find a marked for deletion entity and clear it
            if (!success) {
                for (int i = 0; i < g_DeleteList.GetRef().Count(); i++) {
                    auto networkable = g_DeleteList.GetRef()[i];
                    if (simulated_entity == networkable->GetBaseEntity()) continue;

                    g_bDisableEhandleAccess.GetRef() = true;
                    auto edict = networkable->GetEdict();
                    removed_entities_immediate.insert(networkable->GetBaseEntity());
                    networkable->Release();
                    g_DeleteList.GetRef().Remove(i);
                    g_bDisableEhandleAccess.GetRef() = false;
                    if (edict != nullptr) {
                        edict->freetime = 0;
                    }
                    success = true;
                    break;
                }
            }
        }
        if (entityCount > MAX_EDICTS - 6) {
            // Remove scripted scene first
            bool notifyDelete = false;
            CBaseEntity *entityToDelete = nullptr;
            auto scriptedScene = servertools->FindEntityByClassname(nullptr, "instanced_scripted_scene");
            if (scriptedScene != nullptr && simulated_entity != scriptedScene) {
                entityToDelete = scriptedScene;
            }
            // Entities marked as disposable
            if (entityToDelete == nullptr && !AutoList<DisposableEntityModule>::List().empty()){
                auto entity = AutoList<DisposableEntityModule>::List()[0]->me;
                if (!entity->IsMarkedForDeletion() && simulated_entity != entity) {
                    entityToDelete = entity;
                }
            }
            if (entityToDelete == nullptr) {
                // Delete trails
                ForEachEntityByClassname("env_spritetrail", [&](CBaseEntity *trail) {
                    if (rtti_cast<CBaseProjectile *>(trail->GetMoveParent()) != nullptr && trail != simulated_entity) {
                        entityToDelete = trail;
                        return false;
                    }
                    return true;
                });
            }
            // Delete ragdolls
            if (entityToDelete == nullptr) {
                ForEachTFPlayer([&](CTFPlayer *player) {
                    if (player->m_hRagdoll != nullptr && player->m_hRagdoll != simulated_entity) {
                        entityToDelete = player->m_hRagdoll;
                        return false;
                    }
                    return true;
                });
            }
            // Delete spy bot vgui_screen
            if (entityToDelete == nullptr) {
                
                ForEachEntityByClassname("vgui_screen", [&](CBaseEntity *screen) {
                    if (screen != simulated_entity && rtti_cast<CBaseViewModel *>(screen->GetMoveParent()) != nullptr && ToTFPlayer(screen->GetMoveParent()->GetMoveParent()) != nullptr && ToTFPlayer(screen->GetMoveParent()->GetMoveParent())->IsFakeClient()) {
                        
                        entityToDelete = screen;
                        return false;
                    }
                    return true;
                });
            }
            // Delete disguise wearables
            if (entityToDelete == nullptr) {
                ForEachTFPlayer([&](CTFPlayer *player) {
                    if (player != disguise_wearables_player) {
                        for (int i = 0; i < player->GetNumWearables(); i++) {
                            auto tfwearable = static_cast<CTFWearable *>(player->GetWearable(i));
                            if (tfwearable != nullptr && tfwearable->m_bDisguiseWearable && tfwearable != simulated_entity) {
                                entityToDelete = tfwearable;
                                return false;
                            }
                        }
                    }
                    return true;
                });
            }
            // Reserve some slots for projectiles
            auto &projList = IBaseProjectileAutoList::AutoList();
            if (projList.Count() < 64) {
                // Delete disguise weapons not in main slot
                if (entityToDelete == nullptr) {
                    ForEachTFPlayer([&](CTFPlayer *player) {
                        if (player != disguise_weapon_player && player != disguise_wearables_player && !player->m_hDisguiseWeaponList->IsEmpty() && player->m_hDisguiseWeaponList->Tail() != simulated_entity) {
                            entityToDelete = player->m_hDisguiseWeaponList->Tail();
                            player->m_hDisguiseWeaponList->RemoveMultipleFromTail(1);
                            return false;
                        }
                        return true;
                    });
                }
                // Delete viewmodels for bots
                if (entityToDelete == nullptr) {
                    notifyDelete = true;
                    for (int i = 1; i >= 0; i--) {
                        ForEachTFPlayer([&](CTFPlayer *player) {
                            if (player->IsFakeClient()) {
                                auto viewmodel = player->GetViewModel(i);
                                if (viewmodel != nullptr && viewmodel != simulated_entity) {
                                    entityToDelete = viewmodel;
                                    return false;
                                }
                            }
                            return true;
                        });
                    }
                }
                
                // Remove currency packs
                auto &currencyList = ICurrencyPackAutoList::AutoList();
                if (!currencyList.IsEmpty()) {
                    entityToDelete = rtti_scast<CCurrencyPack *>(currencyList[0]);
                }
                // Delete most wearables (bots)
                if (entityToDelete == nullptr) {
                    notifyDelete = true;
                    ForEachTFPlayer([&](CTFPlayer *player) {
                        if (player->IsFakeClient()) {
                            for (int i = 0; i < player->GetNumWearables(); i++) {
                                auto tfwearable = static_cast<CTFWearable *>(player->GetWearable(i));
                                if (tfwearable != nullptr && tfwearable != simulated_entity) {
                                    int slot = tfwearable->GetItem()->GetItemDefinition()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex());
                                    if (slot >= LOADOUT_POSITION_HEAD && slot != LOADOUT_POSITION_ACTION) {
                                        entityToDelete = tfwearable;
                                        return false;
                                    }
                                }
                            }
                        }
                        return true;
                    });
                }
                // Delete most wearables (others)
                if (entityToDelete == nullptr) {
                    notifyDelete = true;
                    ForEachTFPlayer([&](CTFPlayer *player) {
                        for (int i = 0; i < player->GetNumWearables(); i++) {
                            auto tfwearable = static_cast<CTFWearable *>(player->GetWearable(i));
                            if (tfwearable != nullptr && tfwearable != simulated_entity) {
                                int slot = tfwearable->GetItem()->GetItemDefinition()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex());
                                if (slot >= LOADOUT_POSITION_HEAD && slot != LOADOUT_POSITION_ACTION) {
                                    entityToDelete = tfwearable;
                                    return false;
                                }
                            }
                        }
                        return true;
                    });
                }
            }
            if (entityToDelete == nullptr) {
                // Delete projectiles, starting from oldest. Also delete grounded arrows

                float oldestProjectileTime = -1;
                for (int i = 0; i < projList.Count(); ++i) {

                    auto proj = rtti_scast<CBaseProjectile *>(projList[i]);
                    if (proj == simulated_entity) continue;
                    
                    float time = (float)(proj->m_flSimulationTime) - (float)(proj->m_flAnimTime);

                    // Syringes are going to despawn faster
                    if (proj->GetClassname() == PStr<"tf_projectile_syringe">()) {
                        time *= 2.0f;
                    }
                    // Stickies despawn slower
                    if (proj->GetClassname() == PStr<"tf_projectile_pipe_remote">()) {
                        time *= 0.25f;
                    }

                    // Remove grounded arrow
                    if (/*rtti_cast<CTFProjectile_Arrow *>(proj) != nullptr && */proj->GetMoveType() == MOVETYPE_NONE 
                        && proj->CollisionProp()->IsSolidFlagSet(FSOLID_NOT_SOLID)
                        && proj->IsEffectActive(EF_NODRAW)) {

                        entityToDelete = proj;
                        break;
                    }

                    //Msg("AA %s %f %f\n", proj->GetClassname(), time, oldestProjectileTime);
                    if (time > oldestProjectileTime) {
                        entityToDelete = proj;
                        oldestProjectileTime = time;
                        //Msg("Oldest\n");
                    }
                }
            }
            //timer.End();
            if (entityToDelete != nullptr) {
                //Msg("Deleted entity %d %s %.9f\n", entityToDelete->entindex(), entityToDelete->GetClassname(), timer.GetDuration().GetSeconds());
                auto edict = entityToDelete->edict();
                if (notifyDelete) {
                    SendConsoleMessageToAdmins("ENTITY limit reached, removing entity %s\n", entityToDelete->GetClassname());
                }
                // Need to do this to allow the entity to be reused immediately
                int oldRemoveImmediate = s_RemoveImmediateSemaphore.GetRef();
                if (entityCount == MAX_EDICTS && s_RemoveImmediateSemaphore.GetRef() > 0) {
                    s_RemoveImmediateSemaphore.GetRef() = 0;
                    removed_entities_immediate.insert(entityToDelete);
                }
                servertools->RemoveEntityImmediate(entityToDelete);
                s_RemoveImmediateSemaphore.GetRef() = oldRemoveImmediate;
                edict->freetime = 0;
            }
        }
		return DETOUR_STATIC_CALL(className, iForceEdictIndex);
	}

    DETOUR_DECL_STATIC(void , Physics_SimulateEntity, CBaseEntity * entity) 
    {
        // Needed so that the game will not crash simulating immediately removed entities
        if (!removed_entities_immediate.empty() && removed_entities_immediate.count(entity)) {
            return;
        }
        simulated_entity = entity; 
        DETOUR_STATIC_CALL(entity);
        simulated_entity = nullptr;
    }

    class CMod : public IMod, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Etc:Entity_Limit_Manager")
		{
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_Spawn, "CTFPlayer::Spawn");
            MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_Equip, "CBaseCombatWeapon::Equip");
            MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_SetViewModelIndex, "CBaseCombatWeapon::SetViewModelIndex");
            
            MOD_ADD_DETOUR_STATIC(InstancedScriptedScene, "InstancedScriptedScene");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_UpdateExpression, "CTFPlayer::UpdateExpression");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_DetermineDisguiseWeapon, "CTFPlayerShared::DetermineDisguiseWeapon");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_DetermineDisguiseWearables, "CTFPlayerShared::DetermineDisguiseWearables");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_CreateDisguiseWeaponList, "CTFPlayer::CreateDisguiseWeaponList");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_RemoveDisguiseWeapon, "CTFPlayerShared::RemoveDisguiseWeapon");
            
			MOD_ADD_DETOUR_STATIC(CreateEntityByName, "CreateEntityByName");
			MOD_ADD_DETOUR_STATIC(Physics_SimulateEntity, "Physics_SimulateEntity");
        }
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			if (!removed_entities_immediate.empty()) {
                removed_entities_immediate.clear();
            }
		}
    };
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_entity_limit_manager", "0", FCVAR_NOTIFY,
		"Mod: automatically remove entities as the limit is reached",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});


    RefCount rc_ServerOnly;
    ConVar cvar_path_track_is_server_entity("sig_etc_path_track_is_server_entity", "1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: make path_track a server entity, which saves edicts");
    DETOUR_DECL_MEMBER(IServerNetworkable *, CEntityFactory_CPathTrack_Create, const char *classname)
	{
        SCOPED_INCREMENT_IF(rc_ServerOnly, cvar_path_track_is_server_entity.GetBool());
        return DETOUR_MEMBER_CALL(classname);
    }


    THINK_FUNC_DECL(PlaceholderThink)
    {
        variant_t val;
        this->GetCustomVariableVariant<"placeholderorig">(val);
        auto entity = val.Entity().Get();
        this->GetCustomVariableVariant<"placeholdermove">(val);
        auto move = val.Entity().Get();
        auto animating = entity != nullptr ? entity->GetBaseAnimating() : nullptr;
        bool light = this->GetCustomVariableBool<"placeholderlight">();

        if (move == nullptr || entity == nullptr || ((light && (animating == nullptr || animating->m_hLightingOrigin != this)) || (!light && entity->GetMoveParent() != this ))) {
            this->Remove();
            return;
        }
        this->SetAbsOrigin(move->GetAbsOrigin());
        this->SetAbsAngles(move->GetAbsAngles());
        this->SetNextThink(gpGlobals->curtime + 0.01f, "PlaceholderThink");
    }

    DETOUR_DECL_MEMBER(void, CBaseEntity_SetParent, CBaseEntity *parent, int iAttachment)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        if (parent != nullptr && entity->edict() != nullptr && parent->edict() == nullptr && rtti_cast<CLogicalEntity *>(parent) == nullptr) {
            auto placeholder = CreateEntityByName("point_teleport");
            variant_t val;
            val.SetEntity(entity);
            placeholder->SetCustomVariable<"placeholderorig">(Variant(entity));
            val.SetEntity(parent);
            placeholder->SetCustomVariable<"placeholdermove">(Variant(parent));
            val.SetBool(true);
            placeholder->SetCustomVariable<"placeholderparent">(Variant(true));
            THINK_FUNC_SET(placeholder, PlaceholderThink, gpGlobals->curtime+0.01f);
            placeholder->SetAbsOrigin(parent->GetAbsOrigin());
            placeholder->SetAbsAngles(parent->GetAbsAngles());
            parent = placeholder;
        }
        DETOUR_MEMBER_CALL(parent, iAttachment);
    }

    DETOUR_DECL_MEMBER(void, CBaseAnimating_SetLightingOrigin, CBaseEntity *entity)
	{
        CBaseAnimating *animating = reinterpret_cast<CBaseAnimating *>(this);
        if (entity != nullptr && entity->edict() == nullptr) {
            auto placeholder = CreateEntityByName("point_teleport");
            placeholder->SetCustomVariable<"placeholderorig">(Variant((CBaseEntity *)animating));
            placeholder->SetCustomVariable<"placeholdermove">(Variant(entity));
            placeholder->SetCustomVariable<"placeholderlight">(Variant(true));
            THINK_FUNC_SET(placeholder, PlaceholderThink, gpGlobals->curtime+0.01f);
            entity = placeholder;
        }
        DETOUR_MEMBER_CALL(entity);
    }

    DETOUR_DECL_MEMBER(void, CBaseEntity_CBaseEntity, bool serverOnly)
	{
        DETOUR_MEMBER_CALL(rc_ServerOnly || serverOnly);
        if (rc_ServerOnly) {
            auto entity = reinterpret_cast<CBaseEntity *>(this);
            entity->AddEFlags(65536); //EFL_FORCE_ALLOW_MOVEPARENT
        }
        /*auto entity = reinterpret_cast<CBaseEntity *>(this);
        if (entity->IsPlayer()) {
            entity->m_extraEntityData = new ExtraEntityDataPlayer(entity);
        }
        else if (entity->IsBaseCombatWeapon()) {
            entity->m_extraEntityData = new ExtraEntityDataCombatWeapon(entity);
        }*/
    }

#define MAKE_ENTITY_SERVERSIDE(name) \
    DETOUR_DECL_MEMBER(IServerNetworkable *, name, const char *classname) \
	{\
        SCOPED_INCREMENT(rc_ServerOnly);\
        return DETOUR_MEMBER_CALL(classname);\
    }\

    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTFBotHintSentrygun_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTFBotHintTeleporterExit_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTFBotHint_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CFuncNavAvoid_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CFuncNavPrefer_Create);
    // Trigger entities cannot be made serverside
    //MAKE_ENTITY_SERVERSIDE(CEntityFactory_CFuncNavPrerequisite_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CEnvEntityMaker_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CGameText_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTrainingAnnotation_Create);
    //MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTriggerMultiple_Create);
    //MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTriggerHurt_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTFHudNotify_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CRagdollMagnet_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CEnvShake_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTeamplayRoundWin_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CEnvViewPunch_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CTFForceRespawn_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CPointEntity_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CPointNavInterface_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CPointClientCommand_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CPointServerCommand_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CPointPopulatorInterface_Create);
    MAKE_ENTITY_SERVERSIDE(CEntityFactory_CPointHurt_Create);

    class CModConvertServerside : public IMod
	{
	public:
		CModConvertServerside() : IMod("Etc:Entity_Limit_Manager_Convert_Serverside")
		{
            // Make some entities server only
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_CBaseEntity, "CBaseEntity::CBaseEntity");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CPathTrack_Create, "CEntityFactory<CPathTrack>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTFBotHint_Create, "CEntityFactory<CTFBotHint>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTFBotHintSentrygun_Create, "CEntityFactory<CTFBotHintSentrygun>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTFBotHintTeleporterExit_Create,  "CEntityFactory<CTFBotHintTeleporterExit>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CFuncNavAvoid_Create,  "CEntityFactory<CFuncNavAvoid>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CFuncNavPrefer_Create,  "CEntityFactory<CFuncNavPrefer>::Create");
            //MOD_ADD_DETOUR_MEMBER(CEntityFactory_CFuncNavPrerequisite_Create,  "CEntityFactory<CFuncNavPrerequisite>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CEnvEntityMaker_Create,  "CEntityFactory<CEnvEntityMaker>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CGameText_Create,  "CEntityFactory<CGameText>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTrainingAnnotation_Create,  "CEntityFactory<CTrainingAnnotation>::Create");
            //MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTriggerMultiple_Create,  "CEntityFactory<CTriggerMultiple>::Create");
            //MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTriggerHurt_Create,  "CEntityFactory<CTriggerHurt>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTFHudNotify_Create,  "CEntityFactory<CTFHudNotify>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CRagdollMagnet_Create,  "CEntityFactory<CRagdollMagnet>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CEnvShake_Create,  "CEntityFactory<CEnvShake>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTeamplayRoundWin_Create,  "CEntityFactory<CTeamplayRoundWin>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CEnvViewPunch_Create,  "CEntityFactory<CEnvViewPunch>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CTFForceRespawn_Create,  "CEntityFactory<CTFForceRespawn>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CPointEntity_Create,  "CEntityFactory<CPointEntity>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CPointNavInterface_Create,  "CEntityFactory<CPointNavInterface>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CPointClientCommand_Create,  "CEntityFactory<CPointClientCommand>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CPointServerCommand_Create,  "CEntityFactory<CPointServerCommand>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CPointPopulatorInterface_Create,  "CEntityFactory<CPointPopulatorInterface>::Create");
            MOD_ADD_DETOUR_MEMBER(CEntityFactory_CPointHurt_Create,  "CEntityFactory<CPointHurt>::Create");
            
            MOD_ADD_DETOUR_MEMBER(CBaseAnimating_SetLightingOrigin,  "CBaseAnimating::SetLightingOrigin");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetParent, "CBaseEntity::SetParent");
        }
    };
	CModConvertServerside s_ModConvertServerside;
	
	
	ConVar cvar_enable_convert_serverside("sig_etc_entity_limit_manager_convert_server_entity", "0", FCVAR_NOTIFY,
		"Mod: convert some entities to serverside entities to save edicts",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_ModConvertServerside.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}