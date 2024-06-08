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
#include "stub/particles.h"
#include "stub/usermessages_sv.h"

namespace Mod::AI::NPC_Nextbot
{
    inline MyNextbotModule *GetNextbotModuleLoco(NextBotGroundLocomotion *loco)
    {
        return *((MyNextbotModule **)&loco->m_eyePos);
    }

    VHOOK_DECL(float, MyNextbotLocomotion_GetGravity)
    {
        return GetNextbotModuleLoco(reinterpret_cast<NextBotGroundLocomotion *>(this))->m_flGravity;
    }

    VHOOK_DECL(float, MyNextbotLocomotion_GetRunSpeed)
    {
        auto mod = GetNextbotModuleLoco(reinterpret_cast<NextBotGroundLocomotion *>(this));
        return mod->m_posture == IBody::PostureType::STAND ? mod->m_flMaxRunningSpeed : mod->m_flDuckSpeed;
    }

    VHOOK_DECL(float, MyNextbotLocomotion_GetWalkSpeed)
    {
        auto mod = GetNextbotModuleLoco(reinterpret_cast<NextBotGroundLocomotion *>(this));
        return mod->m_posture == IBody::PostureType::STAND ? mod->m_flMaxRunningSpeed : mod->m_flDuckSpeed;
    }

    VHOOK_DECL(float, MyNextbotLocomotion_GetMaxAcceleration)
    {
        return GetNextbotModuleLoco(reinterpret_cast<NextBotGroundLocomotion *>(this))->m_flAcceleration;
    }

    VHOOK_DECL(float, MyNextbotLocomotion_GetMaxDeceleration)
    {
        return GetNextbotModuleLoco(reinterpret_cast<NextBotGroundLocomotion *>(this))->m_flDeceleration;
    }

    VHOOK_DECL(float, MyNextbotLocomotion_GetMaxJumpHeight)
    {
        return GetNextbotModuleLoco(reinterpret_cast<NextBotGroundLocomotion *>(this))->m_flJumpHeight;
    }

    VHOOK_DECL(float, MyNextbotLocomotion_GetStepHeight)
    {
        return GetNextbotModuleLoco(reinterpret_cast<NextBotGroundLocomotion *>(this))->m_flStepHeight;
    }

    VHOOK_DECL(float, MyNextbotLocomotion_GetDeathDropHeight)
    {
        return 1000.0f;
    }


    VHOOK_DECL(void, MyNextbotLocomotion_FaceTowards, const Vector &target)
    {
        auto loco = reinterpret_cast<NextBotGroundLocomotion *>(this);
        Vector look( target.x, target.y, loco->GetBot()->GetEntity()->EyePosition().z );
        loco->GetBot()->GetBodyInterface()->AimHeadTowards(look, IBody::LookAtPriorityType::BORING, 1.0f, nullptr, "Looking in move direction");
    }
    
    VHOOK_DECL(void, MyNextbotLocomotion_Reset)
    {
        VHOOK_CALL();
        auto loco = reinterpret_cast<NextBotGroundLocomotion *>(this);
        auto entity = loco->GetBot()->GetEntity();
        MyNextbotModule *mod = entity->GetEntityModule<MyNextbotModule>("mynextbotmodule");
        // Reuse existing no longer used field for something else
        *((MyNextbotModule **)&loco->m_eyePos) = mod;
    }

    VHOOK_DECL(void, MyNextbotLocomotion_Upkeep)
    {
        auto loco = reinterpret_cast<NextBotGroundLocomotion *>(this);
        upkeep_current_nextbot = true;
        loco->SetUpdateInterval(gpGlobals->interval_per_tick);
        loco->Update();
        upkeep_current_nextbot = false;

    }

    VHOOK_DECL(void, MyNextbotLocomotion_Update)
    {
        auto loco = reinterpret_cast<NextBotGroundLocomotion *>(this);
        // Only allow updates from Upkeep function
        if (upkeep_current_nextbot) {
            auto mod = GetNextbotModuleLoco(loco);
            auto me = mod->m_pEntity;
            // Run locomotion update every second frame
            //if ((me->entindex() + gpGlobals->tickcount) % 2 == 0) {
                //Msg("UpdateTime %f\n", loco->GetUpdateInterval());
                //TIME_SCOPE2(scope);
                // Help climb up ledges
                if (loco->m_isClimbingUpToLedge) {
                    loco->m_moveVector = loco->m_ledgeJumpGoalPos - me->GetAbsOrigin();
                    //TE_BeamPointsForDebug(loco->m_ledgeJumpGoalPos, loco->m_ledgeJumpGoalPos + Vector(0,0,100), 1.0f, 255, 0, 0);
                    loco->m_moveVector.z = 0.0f;
                    loco->m_moveVector.NormalizeInPlace();
                    //TE_BeamPointsForDebug(me->GetAbsOrigin(), me->GetAbsOrigin() + loco->m_moveVector, 1.0f, 0, 255, 0);
                    
                    loco->m_acceleration = 5 * loco->m_moveVector;
                    //TE_BeamPointsForDebug(me->GetAbsOrigin(), me->GetAbsOrigin() + loco->m_acceleration, 1.0f, 0, 0, 255);
                }
                //me->SetAbsOrigin(me->GetAbsOrigin() - me->GetAbsVelocity() * gpGlobals->interval_per_tick);
                
                auto preAccumWeights = loco->m_accumApproachWeights;
                auto preAccumVectors = loco->m_accumApproachVectors;
                VHOOK_CALL();
                
                loco->m_accumApproachWeights = preAccumWeights;
                loco->m_accumApproachVectors = preAccumVectors;
            //}
            // On the another second frame, extrapolate
            //else {
            //    me->SetAbsOrigin(me->GetAbsOrigin() + me->GetAbsVelocity() * gpGlobals->interval_per_tick);
            //}
        }
        else {
            loco->m_accumApproachWeights = 0;
            loco->m_accumApproachVectors = vec3_origin;
        }
    }

    VHOOK_DECL(void, MyNextbotLocomotion_Approach, const Vector &vec, float weight)
    {
        VHOOK_CALL(vec, weight);
        TE_BeamPointsForDebug(vec, vec + Vector(0,0,100), 1.0f, 255, 255, 0);
    }

    //----------------------------------------------------------------------------------------------------
    // VHOOK_DECL(void, MyNextbotLocomotion_Update)
    // {
    // bool PlayerLocomotion::IsClimbPossible( INextBot *me, const CBaseEntity *obstacle ) const
    // {
    //     // don't jump unless we have to
    //     const PathFollower *path = GetBot()->GetCurrentPath();
    //     if ( path )
    //     {
    //         const float watchForClimbRange = 75.0f;
    //         if ( !path->IsDiscontinuityAhead( GetBot(), Path::CLIMB_UP, watchForClimbRange ) )
    //         {
    //             // we are not planning on climbing

    //             // always allow climbing over movable obstacles
    //             if ( obstacle && !const_cast< CBaseEntity * >( obstacle )->IsWorld() )
    //             {
    //                 IPhysicsObject *physics = obstacle->VPhysicsGetObject();
    //                 if ( physics && physics->IsMoveable() )
    //                 {
    //                     // movable physics object - climb over it
    //                     return true;
    //                 }
    //             }

    //             if ( !GetBot()->GetLocomotionInterface()->IsStuck() )
    //             {
    //                 // we're not stuck - don't try to jump up yet
    //                 return false;
    //             }
    //         }
    //     }

    //     return true;
    // }


    //----------------------------------------------------------------------------------------------------
    
    VHOOK_DECL(bool, MyNextbotLocomotion_ClimbUpToLedge, const Vector &landingGoal, const Vector &landingForward, const CBaseEntity *obstacle)
    {
        auto loco = reinterpret_cast<NextBotGroundLocomotion *>(this);
        loco->Jump();

        loco->m_isClimbingUpToLedge = true;
        loco->m_ledgeJumpGoalPos = landingGoal;
        return true;
    }

    void LoadLocomotionHooks(void **vtable) {
        
        CVirtualHook hooks[] = {
            CVirtualHook("23NextBotGroundLocomotion", TypeName<CZombieIntention>(), "CZombieIntention::Reset", GET_VHOOK_CALLBACK(MyNextbotLocomotion_Reset), GET_VHOOK_INNERPTR(MyNextbotLocomotion_Reset)),
            CVirtualHook("23NextBotGroundLocomotion", "21CTFBaseBossLocomotion", "CTFBaseBossLocomotion::FaceTowards", GET_VHOOK_CALLBACK(MyNextbotLocomotion_FaceTowards), GET_VHOOK_INNERPTR(MyNextbotLocomotion_FaceTowards)),
            CVirtualHook("23NextBotGroundLocomotion", "5IBody", "IBody::Update", GET_VHOOK_CALLBACK(MyNextbotLocomotion_Update), GET_VHOOK_INNERPTR(MyNextbotLocomotion_Update)),
            CVirtualHook("23NextBotGroundLocomotion", "INextBotComponent::Upkeep", GET_VHOOK_CALLBACK(MyNextbotLocomotion_Upkeep), GET_VHOOK_INNERPTR(MyNextbotLocomotion_Upkeep)),
            CVirtualHook("23NextBotGroundLocomotion", "NextBotGroundLocomotion::GetGravity", GET_VHOOK_CALLBACK(MyNextbotLocomotion_GetGravity), GET_VHOOK_INNERPTR(MyNextbotLocomotion_GetGravity)),
            CVirtualHook("23NextBotGroundLocomotion", "25CHeadlessHatmanLocomotion", "CHeadlessHatmanLocomotion::GetRunSpeed", GET_VHOOK_CALLBACK(MyNextbotLocomotion_GetRunSpeed), GET_VHOOK_INNERPTR(MyNextbotLocomotion_GetRunSpeed)),
            CVirtualHook("23NextBotGroundLocomotion", "NextBotGroundLocomotion::GetWalkSpeed", GET_VHOOK_CALLBACK(MyNextbotLocomotion_GetWalkSpeed), GET_VHOOK_INNERPTR(MyNextbotLocomotion_GetWalkSpeed)),
            CVirtualHook("23NextBotGroundLocomotion", "NextBotGroundLocomotion::GetMaxAcceleration", GET_VHOOK_CALLBACK(MyNextbotLocomotion_GetMaxAcceleration), GET_VHOOK_INNERPTR(MyNextbotLocomotion_GetMaxAcceleration)),
            CVirtualHook("23NextBotGroundLocomotion", "NextBotGroundLocomotion::GetMaxDeceleration", GET_VHOOK_CALLBACK(MyNextbotLocomotion_GetMaxDeceleration), GET_VHOOK_INNERPTR(MyNextbotLocomotion_GetMaxDeceleration)),
            CVirtualHook("23NextBotGroundLocomotion", "NextBotGroundLocomotion::GetMaxJumpHeight", GET_VHOOK_CALLBACK(MyNextbotLocomotion_GetMaxJumpHeight), GET_VHOOK_INNERPTR(MyNextbotLocomotion_GetMaxJumpHeight)),
            CVirtualHook("23NextBotGroundLocomotion", "21CTFBaseBossLocomotion", "CTFBaseBossLocomotion::GetStepHeight", GET_VHOOK_CALLBACK(MyNextbotLocomotion_GetStepHeight), GET_VHOOK_INNERPTR(MyNextbotLocomotion_GetStepHeight)),
            CVirtualHook("23NextBotGroundLocomotion", "NextBotGroundLocomotion::GetDeathDropHeight", GET_VHOOK_CALLBACK(MyNextbotLocomotion_GetDeathDropHeight), GET_VHOOK_INNERPTR(MyNextbotLocomotion_GetDeathDropHeight)),
            CVirtualHook("23NextBotGroundLocomotion", "NextBotGroundLocomotion::ClimbUpToLedge", GET_VHOOK_CALLBACK(MyNextbotLocomotion_ClimbUpToLedge), GET_VHOOK_INNERPTR(MyNextbotLocomotion_ClimbUpToLedge)),
            //CVirtualHook("23NextBotGroundLocomotion", "16CTFBotLocomotion", "CTFBotLocomotion::Approach", GET_VHOOK_CALLBACK(MyNextbotLocomotion_Approach), GET_VHOOK_INNERPTR(MyNextbotLocomotion_Approach))
            
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