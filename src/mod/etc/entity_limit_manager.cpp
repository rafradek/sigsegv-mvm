#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/projectiles.h"
#include "stub/tfweaponbase.h"
#include "stub/server.h"
#include "util/pooled_string.h"
#include "util/iterate.h"
#include "util/clientmsg.h"
#include "util/admin.h"
#include "re/nextbot.h"

namespace Mod::Etc::Entity_Limit_Manager
{
    ConVar sig_etc_entity_limit_manager_viewmodel("sig_etc_entity_limit_manager_viewmodel", "1", FCVAR_NOTIFY,
		"Remove usused offhand viewmodel entity for other classes than spy");

    ConVar sig_etc_entity_limit_manager_remove_expressions("sig_etc_entity_limit_manager_remove_expressions", "0", FCVAR_NOTIFY,
		"Remove expressions for bots (like yelling when firing minigun). Reduces entity count by 1 per bot");

    DETOUR_DECL_MEMBER(void, CTFPlayer_Spawn)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this); 
		
		DETOUR_MEMBER_CALL(CTFPlayer_Spawn)();
		// Remove unused offhand viewmodel for other classes than spy
		if (sig_etc_entity_limit_manager_viewmodel.GetBool() && !player->IsPlayerClass(TF_CLASS_SPY) && player->GetViewModel(1) != nullptr) {
			player->GetViewModel(1)->Remove();
		}
    }

    DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_Equip, CBaseCombatCharacter *owner)
	{
		auto ent = reinterpret_cast<CBaseCombatWeapon *>(this);
        DETOUR_MEMBER_CALL(CBaseCombatWeapon_Equip)(owner);
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
        DETOUR_MEMBER_CALL(CBaseCombatWeapon_SetViewModelIndex)(index);
    }
    
    RefCount rc_CTFPlayer_UpdateExpression;
    DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateExpression)
	{
        auto player = reinterpret_cast<CTFPlayer *>(this);
        SCOPED_INCREMENT_IF(rc_CTFPlayer_UpdateExpression, sig_etc_entity_limit_manager_remove_expressions.GetBool() && player->IsBot());
        DETOUR_MEMBER_CALL(CTFPlayer_UpdateExpression)();
    }

    CBaseEntity *remove_this_scene_entity = nullptr;
    DETOUR_DECL_STATIC(float, InstancedScriptedScene, CBaseFlex *pActor, const char *pszScene, EHANDLE *phSceneEnt,
							 float flPostDelay, bool bIsBackground, AI_Response *response,
							 bool bMultiplayer, IRecipientFilter *filter)
	{
        //Msg("pActor %d scene %s\n", ENTINDEX(pActor), pszScene);
        //if (strstr(pszScene,"idleloop") != nullptr) { return 10;}
        if (rc_CTFPlayer_UpdateExpression) return 10;
        
        //auto oldSceneEnt = *phSceneEnt;
        auto ret = DETOUR_STATIC_CALL(InstancedScriptedScene)(pActor, pszScene, phSceneEnt, flPostDelay, bIsBackground, response, bMultiplayer, filter);
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
        if (reinterpret_cast<CGameServer *>(sv)->GetNumEdicts() > 2044) {
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
                if (g_DeleteList.GetRef().Count() > 0) {

                    g_bDisableEhandleAccess.GetRef() = true;
                    auto edict = g_DeleteList.GetRef()[0]->GetEdict();
                    g_DeleteList.GetRef()[0]->Release();
                    g_DeleteList.GetRef().Remove(0);
                    g_bDisableEhandleAccess.GetRef() = false;
                    if (edict != nullptr) {
                        edict->freetime = 0;
                    }
                    success = true;
                }
            }
        }
        if (!success && entityCount > 2044) {
            // Remove scripted scene first
            CBaseEntity *entityToDelete = nullptr;
            auto scriptedScene = servertools->FindEntityByClassname(nullptr, "instanced_scripted_scene");
            if (scriptedScene != nullptr) {
                entityToDelete = scriptedScene;
            }
            if (entityToDelete == nullptr) {
                // Delete trails
                ForEachEntityByClassname("env_spritetrail", [&](CBaseEntity *trail) {
                    if (rtti_cast<CBaseProjectile *>(trail->GetMoveParent()) != nullptr) {
                        entityToDelete = trail;
                        return false;
                    }
                    return true;
                });
            }
            bool notifyDelete = false;
            if (entityToDelete == nullptr) {
                // Delete projectiles, starting from oldest. Also delete grounded arrows

                notifyDelete = true;
                float oldestProjectileTime = -1;
                for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {

                    auto proj = rtti_scast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
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
                //ClientMsgAll("deleted %d\n", notifyDelete);
                if (notifyDelete) {
                    SendConsoleMessageToAdmins("ENTITY limit reached, removing entity %s\n", entityToDelete->GetClassname());
                }
                // Need to do this to allow the entity to be reused immediately
                int oldRemoveImmediate = s_RemoveImmediateSemaphore.GetRef();
                if (engine->GetEntityCount() == 2048 && s_RemoveImmediateSemaphore.GetRef() > 0) {
                    s_RemoveImmediateSemaphore.GetRef() = 0;
                    removed_entities_immediate.insert(entityToDelete);
                }
                servertools->RemoveEntityImmediate(entityToDelete);
                s_RemoveImmediateSemaphore.GetRef() = oldRemoveImmediate;
                edict->freetime = 0;
            }
        }
		return DETOUR_STATIC_CALL(CreateEntityByName)(className, iForceEdictIndex);
	}

    DETOUR_DECL_STATIC(void , Physics_SimulateEntity, CBaseEntity * entity) 
    {
        // Needed so that the game will not crash simulating immediately removed entities
        if (!removed_entities_immediate.empty() && removed_entities_immediate.count(entity)) {
            return;
        } 
        DETOUR_STATIC_CALL(Physics_SimulateEntity)(entity);
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

}