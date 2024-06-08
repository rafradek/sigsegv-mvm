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
#include "stub/objects.h"

namespace Mod::AI::NPC_Nextbot
{
    VHOOK_DECL(void, MyNextbotVision_Update)
    {
        auto entity = my_current_nextbot_entity;
        auto mod = my_current_nextbot_module;
        if (!mod->m_VisionScanTime.IsElapsed()) {
            return;
        }
        // Increase vision cooldown, for performace
        mod->m_VisionScanTime.Start(RandomFloat(0.8f, 1.2f));
        VHOOK_CALL();
    }

    VHOOK_DECL(void, MyNextbotVision_CollectPotentiallyVisibleEntities, CUtlVector<CBaseEntity *> *visible)
    {
        VHOOK_CALL(visible);
        for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
            auto obj = rtti_scast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
            if (obj != nullptr && !obj->m_bPlacing && !obj->m_bCarried) {
                visible->AddToTail(obj);
            }
        }
    }

    VHOOK_DECL(bool, MyNextbotVision_IsIgnored, CBaseEntity *subject)
    {
        auto entity = reinterpret_cast<IVision *>(this)->GetBot()->GetEntity();
        auto isEnemy = subject->GetTeamNumber() != entity->GetTeamNumber();
        // Ignore friends, for performance
        if (!isEnemy) {
            return true;
        }
        auto player = ToTFPlayer(subject);
        if (player != nullptr) {
            if (isEnemy) {
                if (player->m_Shared->IsStealthed()) {
                    return true;
                }
                if (player->m_Shared->InCond(TF_COND_DISGUISED) && player->m_Shared->GetDisguiseTeam() == entity->GetTeamNumber()) {
                    return true;
                }
            }
        }
        return false;
    }

    bool IsOnNav(CBaseCombatCharacter *entity, float maxZDistance, float maxXYDistance)
    {
        auto subjectCombat = entity->MyCombatCharacterPointer();
        if (subjectCombat != nullptr) {
            auto area = subjectCombat->GetLastKnownArea();
            if (area != nullptr) {
                
                auto &vec = entity->GetAbsOrigin();
                bool contains = area->IsOverlapping(vec,maxXYDistance);
                float z = area->GetZ(vec.x, vec.y);
                if (!contains || z - vec.z > maxZDistance) {
                    return false;
                }
            }
        }
        return true;
    }

    VHOOK_DECL(bool, MyNextbotVision_IsAbleToSee, CBaseEntity *subject, int checkFOV, Vector *visibleSpot)
    {
        // For performance, entities on nav area that is fully visible from our nav area are considered visible
        auto entity = reinterpret_cast<IVision *>(this)->GetBot()->GetEntity();
        auto subjectCombat = subject->MyCombatCharacterPointer();
        if (subjectCombat != nullptr && IsOnNav(subjectCombat, 20, 4) && IsOnNav(entity, 20, 4) && entity->GetLastKnownArea() != nullptr && entity->GetLastKnownArea()->IsCompletelyVisible(subjectCombat->GetLastKnownArea())) {
            return true;
        }
        return VHOOK_CALL(subject, checkFOV, visibleSpot);
    }

    void LoadVisionHooks(void **vtable) {
        
        CVirtualHook hooks[] = {
            CVirtualHook("7IVision", "5IBody", "IBody::Update", GET_VHOOK_CALLBACK(MyNextbotVision_Update), GET_VHOOK_INNERPTR(MyNextbotVision_Update)),
            CVirtualHook("7IVision", "12CTFBotVision", "CTFBotVision::IsIgnored", GET_VHOOK_CALLBACK(MyNextbotVision_IsIgnored), GET_VHOOK_INNERPTR(MyNextbotVision_IsIgnored)),
            CVirtualHook("7IVision", "12CTFBotVision", "CTFBotVision::CollectPotentiallyVisibleEntities", GET_VHOOK_CALLBACK(MyNextbotVision_CollectPotentiallyVisibleEntities), GET_VHOOK_INNERPTR(MyNextbotVision_CollectPotentiallyVisibleEntities)),
            CVirtualHook("7IVision", "IVision::IsAbleToSee2", GET_VHOOK_CALLBACK(MyNextbotVision_IsAbleToSee), GET_VHOOK_INNERPTR(MyNextbotVision_IsAbleToSee)),
        };
        for (auto &hook : hooks) {
            hook.DoLoad();
            hook.AddToVTable(vtable);
        }
    }

    // static MemberFuncThunk<NextBotGroundLocomotion *, void, INextBot *> ft_NextBotGroundLocomotion_ctor("NextBotGroundLocomotion::NextBotGroundLocomotion");
    // class MyNextbotLocomotion : public NextBotGroundLocomotion
    // {
    // public:
    //     static MyNextbotLocomotion *New(INextBot *bot) {
    //         auto loco = reinterpret_cast<MyNextbotLocomotion *>(::operator new(sizeof(MyNextbotLocomotion)));
	//         ft_NextBotGroundLocomotion_ctor(loco, bot);
    //         //
    //         return loco;
    //     } 
    // };
}