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
#include "util/clientmsg.h"
#include "stub/trace.h"
#include "mod/common/weapon_shoot.h"

namespace Mod::AI::NPC_Nextbot
{
    inline MyNextbotModule *GetNextbotModuleBody(CBotNPCBody *body)
    {
        return *((MyNextbotModule **)&body->m_desiredAimAngles);
    }

    void UpdateAim(CBotNPCBody *body, MyNextbotEntity *me, MyNextbotModule* mod) {
        // update aim angles
        const float deltaT = gpGlobals->frametime;
        if ( deltaT < 0.00001f )
        {
            return;
        }

        // if our current look-at has expired, don't change our aim further
        if ( mod->m_hasBeenSightedIn && mod->m_lookAtExpireTimer.IsElapsed() )
        {
            if ( !mod->m_headSteadyTimer.HasStarted() )
            {
                mod->m_headSteadyTimer.Start();
            }
            return;
        }

        // get current view angles
        QAngle &currentAngles = mod->m_currentAimAngles;
        //Msg("anglepre %f %f %f\n", mod->m_currentAimAngles.x, mod->m_currentAimAngles.y, mod->m_currentAimAngles.z);

        // track when our head is "steady"
        bool isSteady = true;

        float actualPitchRate = AngleDiff( currentAngles.x, mod->m_priorAngles.x );
        if ( abs( actualPitchRate ) > nb_head_aim_steady_max_rate->GetFloat() * deltaT )
        {
            isSteady = false;
        }
        else
        {
            float actualYawRate = AngleDiff( currentAngles.y, mod->m_priorAngles.y );

            if ( abs( actualYawRate ) > nb_head_aim_steady_max_rate->GetFloat() * deltaT )
            {
                isSteady = false;
            }
        }

        if ( isSteady )
        {
            if ( !mod->m_headSteadyTimer.HasStarted() )
            {
                mod->m_headSteadyTimer.Start();
            }
        }
        else
        {
            mod->m_headSteadyTimer.Invalidate();
        }

        mod->m_priorAngles = currentAngles;

        // simulate limited range of mouse movements
        // compute the angle change from "center"
        Vector &forward = mod->m_currentAimForward;
        
        float deltaAngle = RAD2DEG( acos( DotProduct( forward, mod->m_anchorForward ) ) );

        if ( deltaAngle > nb_head_aim_resettle_angle->GetFloat() )
        {
            // time to recenter our 'virtual mouse'
            mod->m_anchorRepositionTimer.Start( RandomFloat( 0.9f, 1.1f ) * nb_head_aim_resettle_time->GetFloat() );
            mod->m_anchorForward = forward;
            return;
        }

        // if we're currently recentering our "virtual mouse", wait
        if ( mod->m_anchorRepositionTimer.HasStarted() && !mod->m_anchorRepositionTimer.IsElapsed() )
        {
            return;
        }
        mod->m_anchorRepositionTimer.Invalidate();

        // if we have a subject, update lookat point
        CBaseEntity *subject = mod->m_lookAtSubject;
        if ( subject )
        {
            if ( mod->m_lookAtTrackingTimer.IsElapsed() )
            {
                // update subject tracking by periodically estimating linear aim velocity, allowing for "slop" between updates
                Vector desiredLookAtPos;

                if ( subject->MyCombatCharacterPointer() ) 
                {
                    desiredLookAtPos = me->GetIntentionInterface()->SelectTargetPoint( body->GetBot(), subject->MyCombatCharacterPointer() );
                }
                else
                {
                    desiredLookAtPos = subject->WorldSpaceCenter();
                }

                //desiredLookAtPos += body->GetHeadAimSubjectLeadTime() * subject->GetAbsVelocity();

                Vector errorVector = desiredLookAtPos - mod->m_lookAtPos;
                float error = errorVector.NormalizeInPlace();

                float trackingInterval = mod->m_flHeadTrackingInterval;
                if ( trackingInterval < deltaT )
                {
                    trackingInterval = deltaT;
                }

                float errorVel = error / trackingInterval;

                mod->m_lookAtVelocity = ( errorVel * errorVector ) + subject->GetAbsVelocity();

                mod->m_lookAtTrackingTimer.Start( RandomFloat( 0.8f, 1.2f ) * trackingInterval );
            }

            mod->m_lookAtPos += deltaT * mod->m_lookAtVelocity;
        }

        // aim view towards last look at point
        Vector to = mod->m_lookAtPos - me->EyePosition();
        to.NormalizeInPlace();

        QAngle desiredAngles;
        VectorAngles( to, desiredAngles );

        QAngle angles;
        
        const float onTargetTolerance = 0.98f;
        float dot = DotProduct( forward, to );
        if ( dot > onTargetTolerance )
        {
            // on target
            mod->m_isSightedIn = true;

            if ( !mod->m_hasBeenSightedIn )
            {
                mod->m_hasBeenSightedIn = true;
            }

            if ( mod->m_lookAtReplyWhenAimed )
            {
                mod->m_lookAtReplyWhenAimed->OnSuccess( body->GetBot() );
                mod->m_lookAtReplyWhenAimed = NULL;
            }
        }
        else
        {
            // off target
            mod->m_isSightedIn = false;
        }


        // rotate view at a rate proportional to how far we have to turn
        // max rate if we need to turn around
        // want first derivative continuity of rate as our aim hits to avoid pop
        float approachRate = mod->m_flMaxHeadAngularVelocity;

        const float easeOut = 0.7f;
        if ( dot > easeOut )
        {
            float t = RemapVal( dot, easeOut, 1.0f, 1.0f, 0.02f );
            const float halfPI = 1.57f;
            approachRate *= sin( halfPI * t );
        }

        const float easeInTime = 0.25f;
        if ( mod->m_lookAtDurationTimer.GetElapsedTime() < easeInTime )
        {
            approachRate *= mod->m_lookAtDurationTimer.GetElapsedTime() / easeInTime;
        }

        angles.y = ApproachAngle( desiredAngles.y, currentAngles.y, approachRate * deltaT );
        angles.x = ApproachAngle( desiredAngles.x, currentAngles.x, 0.5f * approachRate * deltaT );
        angles.z = 0.0f;

        angles.x = AngleNormalize( angles.x );
        angles.y = AngleNormalize( angles.y );

        mod->m_currentAimAngles = angles;
        AngleVectors(angles, &mod->m_currentAimForward);
        //Msg("anglepost %f %f %f %f %f %f\n", mod->m_currentAimAngles.x, mod->m_currentAimAngles.y, desiredAngles.x, desiredAngles.y, approachRate, deltaT);
    }

    GlobalThunk<acttable_t[33]> s_acttableMelee("s_acttableMelee");

    Activity TranslateActivity(CBotNPCBody *body, MyNextbotEntity *me, MyNextbotModule* mod, Activity act) {
        if (act == ACT_JUMP) act = ACT_MP_JUMP_START;

        auto weapon = me->GetActiveWeapon();
        if (weapon == nullptr) {
            for (int i = 0; i < 33; i++) {
                auto &acttable = s_acttableMelee.GetRef()[i];
                if (acttable.baseAct == act) {
                    act = (Activity)acttable.weaponAct;
                    break;
                }
            }
        }
        else {
            bool req;
            act = weapon->ActivityOverride(act, &req);
        }
        return act;
        // switch (act) {
        //     case ACT_JUMP: return ACT_MP_JUMP_START_ITEM2;
        //     case ACT_MP_AIRWALK: return ACT_MP_AIRWALK_ITEM2;
        //     case ACT_MP_RUN: return ACT_MP_RUN_ITEM2;
        //     case ACT_MP_STAND_IDLE: return ACT_MP_STAND_ITEM2;
        //     case ACT_MP_JUMP_FLOAT: return ACT_MP_JUMP_FLOAT_ITEM2;
        //     case ACT_MP_CROUCH_IDLE: return ACT_MP_CROUCH_ITEM2;
        //     case ACT_MP_CROUCHWALK: return ACT_MP_CROUCHWALK_ITEM2;
        //     case ACT_MP_JUMP_LAND: return ACT_MP_JUMP_LAND_ITEM2;
        //     default: return act;
        // }
    }

    bool SwitchToActivity(CBotNPCBody *body, MyNextbotEntity *me, MyNextbotModule* mod, Activity act, CStudioHdr *modelptr, bool reset = true) {
        if (act == ACT_INVALID) {
            mod->m_untranslatedActivity = act;
            body->m_currentActivity = act;
            return true;
        }
        Activity untranslated = act;
        act = TranslateActivity(body, me, mod, act);
        if (!reset && act == body->m_currentActivity) return false;
        
        int animSequence = ::SelectWeightedSequence(modelptr, act, me->m_nSequence);

        if ( animSequence )
        {
            mod->m_untranslatedActivity = untranslated;
            body->m_currentActivity = act;
            me->m_nSequence = animSequence;
            me->m_flPlaybackRate = 1.0f;
            me->SetCycle( 0 );
            me->ResetSequenceInfo();
        }
        return animSequence != 0;
    }

    void SelectDesiredActivity(CBotNPCBody *body, MyNextbotEntity *me, MyNextbotModule* mod, CStudioHdr *modelptr) {
        //TIME_SCOPE2(desired)
        if (mod->m_forcedActivity != ACT_INVALID) {
            //Msg("Forced activity %s %d %d\n", CAI_BaseNPC::GetActivityName(mod->m_forcedActivity), me->m_bSequenceFinished.Get(), me->m_bSequenceLoops.Get());
            if (me->m_bSequenceFinished && !me->m_bSequenceLoops) {
                mod->m_forcedActivity = ACT_INVALID;
            }
            else {
                mod->m_bInAirWalk = false;
                return;
            }
        }
        Activity act = ACT_MP_STAND_IDLE;
        auto loco = me->GetLocomotionInterface();
        auto &velocity = loco->GetVelocity();
        float speed = loco->GetGroundSpeed();
        bool running = speed > 0.01f;
        bool onground = me->GetFlags() & FL_ONGROUND;
        bool ducking = body->GetActualPosture() == IBody::PostureType::CROUCH;
        bool jumping = loco->IsClimbingOrJumping();

        if (!onground) {
            if ((velocity.z > 300.0f || mod->m_bInAirWalk) && !ducking) {
                act = ACT_MP_AIRWALK;
                mod->m_bInAirWalk = true;
            }
            else if (jumping) {
                act = ACT_MP_JUMP_FLOAT;
            }
        }
        else {
            mod->m_bInAirWalk = false;
            if (ducking) {
                act = running ? ACT_MP_CROUCHWALK : ACT_MP_CROUCH_IDLE;
            }
            else if (running) {
                act = ACT_MP_RUN;
            }
        }
        if (act == ACT_INVALID) return;

        SwitchToActivity(body, me, mod, act, modelptr, false);
    }

    void UpdateAnim(CBotNPCBody *body, MyNextbotEntity *me, MyNextbotModule* mod) {

        const float deltaT = gpGlobals->frametime * 2;

        auto modelptr = me->GetModelPtr();
        SelectDesiredActivity(body, me, mod, modelptr);

        if (modelptr != nullptr) {
            if (body->m_moveXPoseParameter < 0) {
                body->m_moveXPoseParameter = me->LookupPoseParameter(modelptr, "move_x" );
            }

            if (body->m_moveYPoseParameter < 0) {
                body->m_moveYPoseParameter = me->LookupPoseParameter(modelptr, "move_y" );
            }

            if (mod->m_iBodyPitchParam < 0) {
                mod->m_iBodyPitchParam  = me->LookupPoseParameter(modelptr, "body_pitch" );
            }

            if (mod->m_iBodyYawParam < 0) {
                mod->m_iBodyYawParam = me->LookupPoseParameter(modelptr, "body_yaw" );
            }

            if (mod->m_iBodyMoveYawParam < 0) {
                mod->m_iBodyMoveYawParam = me->LookupPoseParameter(modelptr, "move_yaw" );
            }

            if (mod->m_iBodyMoveScaleParam < 0) {
                mod->m_iBodyMoveScaleParam = me->LookupPoseParameter(modelptr, "move_scale" );
            }
        }

        // Update the pose parameters
        auto loco = me->GetLocomotionInterface();
        float speed = loco->GetGroundSpeed(); // me->GetAbsVelocity().Length();

        QAngle botAngles = me->GetAbsAngles();

        if ( speed < 0.01f )
        {
            // stopped
            if ( body->m_moveXPoseParameter >= 0 ) {
                me->SetPoseParameter(modelptr, body->m_moveXPoseParameter, 0.0f );
            }
            if ( body->m_moveYPoseParameter >= 0 ) {
                me->SetPoseParameter(modelptr, body->m_moveYPoseParameter, 0.0f );
            }
        }
        else
        {
            Vector forward, right, up;
            me->GetVectors( &forward, &right, &up );

            const Vector &motionVector = loco->GetGroundMotionVector();

            // move_x == 1.0 at full forward motion and -1.0 in full reverse
            if ( body->m_moveXPoseParameter >= 0 )
            {
                float forwardVel = DotProduct( motionVector, forward );

                me->SetPoseParameter(modelptr, body->m_moveXPoseParameter, forwardVel );
            }

            if ( body->m_moveYPoseParameter >= 0 )
            {
                float sideVel = DotProduct( motionVector, right );

                me->SetPoseParameter(modelptr, body->m_moveYPoseParameter, sideVel );
            }
            
            if ( mod->m_iBodyMoveYawParam >= 0 ) {
                QAngle ang;
                VectorAngles(motionVector, ang);
                me->SetPoseParameter(modelptr, mod->m_iBodyMoveYawParam, AngleDiff(ang.y, botAngles.y) );
            }
        }
        
        if ( mod->m_iBodyMoveScaleParam >= 0 ) {
            me->SetPoseParameter(modelptr, mod->m_iBodyMoveScaleParam, 1.0f );
        }

        // adjust animation speed to actual movement speed
        if ( me->m_flGroundSpeed > 0.0f && me->GetGroundEntity() != nullptr && !loco->IsClimbingOrJumping())
        {
            // Clamp playback rate to avoid datatable warnings.  Anything faster would look silly, anyway.
            float playbackRate = clamp( speed / me->m_flGroundSpeed, -4.f, 12.f );
            me->m_flPlaybackRate = playbackRate;
        }
        float preCycle = me->GetCycle();
        // move the animation ahead in time	
        me->StudioFrameAdvance();
        me->DispatchAnimEvents( me );

        if (speed > 0.01f && me->GetGroundEntity() != nullptr && !mod->m_bNoFootsteps) {
            // Hardcode footstep sound every half animation cycle
            if ((preCycle < 0.5f && me->GetCycle() >= 0.5f) || (preCycle >= 0.5f && me->GetCycle() < 0.5f)) {
                const char *sound = mod->m_strFootstepSound;
                // Fire a trace to determine what kind of surface the ground is
                if (sound[0] == '\0') {
                    trace_t trace;
                    UTIL_TraceHull(me->GetAbsOrigin(),
                        me->GetAbsOrigin() - Vector(0,0,5), 
                        me->CollisionProp()->OBBMins(), 
                        me->CollisionProp()->OBBMaxs(), 
                        me->GetBodyInterface()->GetSolidMask(), me, me->GetCollisionGroup(), &trace);
                    
                    if (trace.DidHit()) {
                        auto data = physprops->GetSurfaceData(trace.surface.surfaceProps);
                        sound = physprops->GetString(me->GetCycle() >= 0.5f ? data->sounds.stepleft : data->sounds.stepright);
                    }
                }
                if (sound[0] != '\0'){
                    me->EmitSound(sound);
                }
            }
        }

        
        if ( mod->m_iBodyPitchParam >= 0 )
        {
            //Msg("SetParamPitch %f\n", -mod->m_currentAimAngles.x);
            me->SetPoseParameter(modelptr, mod->m_iBodyPitchParam, -mod->m_currentAimAngles.x );
        }

        float extra = fmodf(mod->m_currentAimAngles.y - botAngles.y, 360.0f);
        float diffyaw = Max(abs(AngleDiff(mod->m_currentAimAngles.y, botAngles.y))-44.0f,0.0f);
        if (speed > 0.01f) {
            diffyaw += 90.0f * deltaT;
        }
        botAngles.y = ApproachAngle( mod->m_currentAimAngles.y, botAngles.y, diffyaw);
        me->SetAbsAngles(botAngles);


        if ( mod->m_iBodyYawParam >= 0 )
        {
            me->SetPoseParameter(modelptr, mod->m_iBodyYawParam, Clamp(mod->m_currentAimAngles.y == botAngles.y ? 0 : -AngleDiff(mod->m_currentAimAngles.y, botAngles.y), -44.0f, 44.0f) );
        }
    }

    float ResetFiringReloadGestures(MyNextbotEntity *me, MyNextbotModule* mod, Activity newActivity, float speedMult) {
        me->RemoveGesture(mod->m_LastAttackGestureActivity);
        int layer = mod->m_LastAttackGestureLayer = me->AddGesture(newActivity);
        mod->m_LastAttackGestureActivity = newActivity;
        float duration = me->GetLayerDuration(layer);
        if (speedMult != 1.0f) {
            me->SetLayerDuration(layer, duration * speedMult);
        }
        return duration * speedMult;
    }

    bool HasAmmo(CTFWeaponBaseGun *gun, MyNextbotEntity *me) {
        return gun->IsEnergyWeapon() ? gun->m_flEnergy >= gun->Energy_GetShotCost() : (gun->m_iClip1 == -1 ? me->GetAmmoCount(gun->m_iPrimaryAmmoType) >= gun->GetAmmoPerShot() : gun->m_iClip1 >= gun->GetAmmoPerShot());
    }

    void IncrementAmmo(CTFWeaponBaseGun *gun, MyNextbotEntity *me) {
        if (gun->IsEnergyWeapon()) {
            gun->m_flEnergy += gun->Energy_GetShotCost();
        }
        else {
            int ammoToLoad = gun->m_bReloadsSingly ? 1 : Min(gun->GetMaxClip1() - gun->m_iClip1, me->GetAmmoCount(gun->m_iPrimaryAmmoType));
            gun->m_iClip1 += ammoToLoad;
            me->RemoveAmmo(ammoToLoad, gun->m_iPrimaryAmmoType);
        }
    }
    
    bool CanReload(CTFWeaponBaseGun *gun, MyNextbotEntity *me) {
        return (gun->GetMaxClip1() > 0 && gun->GetMaxClip1() > gun->m_iClip1 && me->GetAmmoCount(gun->m_iPrimaryAmmoType) > 0) ||
            (gun->IsEnergyWeapon() && gun->m_flEnergy < gun->Energy_GetMaxEnergy());
    }

    void StartReload(CBotNPCBody *body, CTFWeaponBaseGun *gun, MyNextbotEntity *me, MyNextbotModule* mod, bool first) {

        Activity gesture;
        bool standing = body->GetActualPosture() == IBody::PostureType::STAND;
        if (first) {
            gesture = standing ? ACT_MP_RELOAD_STAND : ACT_MP_RELOAD_CROUCH;
        }
        else {
            gesture = standing ? ACT_MP_RELOAD_STAND_LOOP : ACT_MP_RELOAD_CROUCH_LOOP;
        }
        float animTime = ResetFiringReloadGestures(me, mod, TranslateActivity(body, me, mod, gesture), 1.0f);
        float time = gun->GetTFWpnData().m_flTimeReload + (first && gun->m_bReloadsSingly ? gun->GetTFWpnData().m_flTimeReloadStart : 0);

        if (first && gun->m_bReloadsSingly) gun->m_flNextPrimaryAttack += gun->GetTFWpnData().m_flTimeReloadStart;

        if (time == 0.0f) {
            time = animTime;
        }
        gun->m_flNextPrimaryAttack = gpGlobals->curtime + time;
    }
    
    void UpdateShoot(CBotNPCBody *body, MyNextbotEntity *me, MyNextbotModule* mod) {
        auto gun = rtti_cast<CTFWeaponBaseGun *>(me->GetActiveWeapon());
        bool startReload = false;
        if (gun != nullptr) {
            bool canReload = CanReload(gun, me);
            
            if (!canReload && gun->m_bInReload) {
                gun->m_bInReload = false;
            }

            if (canReload && !gun->m_bInReload && gpGlobals->curtime >= gun->m_flNextPrimaryAttack) {
                startReload = true;
            }

            if (gun->m_bInReload && gpGlobals->curtime >= gun->m_flNextPrimaryAttack) {
                IncrementAmmo(gun, me);
                if (CanReload(gun, me)) {
                    StartReload(body, gun, me, mod, false);
                }
                else {
                    ResetFiringReloadGestures(me, mod, TranslateActivity(body, me, mod, body->GetActualPosture() == IBody::PostureType::STAND ? ACT_MP_RELOAD_STAND_END : ACT_MP_RELOAD_CROUCH_END), 1.0f);
                }
            }
        }
        if(mod->m_bShouldShoot) {
            if (mod->m_flHandAttackTime > 0.0f) {
                if (mod->m_handAttackTimer.IsElapsed()) {
                    mod->m_handAttackTimer.Start(mod->m_flHandAttackTime);
                    mod->m_handAttackSmackTimer.Start(mod->m_flHandAttackSmackTime);
                    ResetFiringReloadGestures(me, mod, TranslateActivity(body, me, mod, body->GetActualPosture() == IBody::PostureType::STAND ? ACT_MP_ATTACK_STAND_PRIMARYFIRE : ACT_MP_ATTACK_CROUCH_PRIMARYFIRE), 1.0f);
                    mod->m_bHandAttackSmack = true;
                }
            }

            if (gun != nullptr) {
                if ((gpGlobals->curtime >= gun->m_flNextPrimaryAttack || (gun->m_bInReload && !mod->m_bWaitUntilFullReload) ) && (HasAmmo(gun, me))) {
                    gun->m_bInReload = false;
                    startReload = false;
                    auto proj = Mod::Common::Weapon_Shoot::FireWeapon(me, gun, me->EyePosition(), me->EyeAngles(), mod->m_bCrit, true);
                    auto delay = gun->GetTFWpnData().m_flTimeFireDelay;

                    gun->m_flNextPrimaryAttack = gpGlobals->curtime + GetWeaponFireSpeedDelay(gun, delay);
                    ResetFiringReloadGestures(me, mod, TranslateActivity(body, me, mod, body->GetActualPosture() == IBody::PostureType::STAND ? ACT_MP_ATTACK_STAND_PRIMARYFIRE : ACT_MP_ATTACK_CROUCH_PRIMARYFIRE), 1.0f);
                }
            }

        }
        if (startReload) {
            gun->m_bInReload = true;
            StartReload(body, gun, me, mod, true);
        }

        if (mod->m_bHandAttackSmack && mod->m_handAttackSmackTimer.IsElapsed()) {
            Vector forward; 
            AngleVectors(me->EyeAngles(), &forward);
            Vector start = me->EyePosition();
            Vector end = start + forward * mod->m_flHandAttackRange;
            Vector mins = Vector(-mod->m_flHandAttackSize, -mod->m_flHandAttackSize, -mod->m_flHandAttackSize);
            Vector maxs = Vector(mod->m_flHandAttackSize, mod->m_flHandAttackSize, mod->m_flHandAttackSize);
            Ray_t ray;
            ray.Init( start, end, mins, maxs );
            CBaseEntity *pList[256];
            int nTargetCount = UTIL_EntitiesAlongRay( pList, ARRAYSIZE( pList ), ray, FL_CLIENT|FL_OBJECT|FL_NPC );
            
            int nHitCount = 0;
            for ( int i=0; i<nTargetCount; ++i )
            {
                CBaseEntity *pTarget = pList[i];
                if (pTarget == me) continue;

                if (pTarget->GetTeamNumber() == me->GetTeamNumber()) continue;

                trace_t tr;
                UTIL_TraceModel( start, end, mins, maxs, pTarget, COLLISION_GROUP_NONE, &tr );

                if (tr.m_pEnt == pTarget) {
                    current_kill_icon = mod->m_strHandAttackIcon;
                    CTakeDamageInfo info(me->GetOwnerEntity() != nullptr ? me->GetOwnerEntity() : me, me, nullptr, (forward + Vector(0,0,1)) * mod->m_flHandAttackForce, me->GetAbsOrigin(), mod->m_flHandAttackDamage, DMG_CLUB | (mod->m_bCrit ? DMG_CRITICAL : 0), TF_DMG_CUSTOM_NONE);
                    pTarget->TakeDamage(info);
                    current_kill_icon = nullptr;
                }
                if (!mod->m_bHandAttackCleave) {
                    break;
                }
                nHitCount++;
            }
            me->EmitSound(nHitCount > 0 ? mod->m_strHandAttackSound : mod->m_strHandAttackSoundMiss);
            mod->m_bHandAttackSmack = false;
        }
    }

    VHOOK_DECL(void, MyNextbotBody_Upkeep)
    {
        auto body = reinterpret_cast<CBotNPCBody *>(this);
        auto mod = GetNextbotModuleBody(body);
        MyNextbotEntity *me = mod->m_pEntity;
        if (!mod->m_bFreeze) {
            UpdateAim(body, me, mod);
        }
        
        if ((me->entindex() + gpGlobals->tickcount) % 2 != 0) {
            UpdateAnim(body, me, mod);
        }

        UpdateShoot(body, me, mod);
    }

    VHOOK_DECL(void, MyNextbotBody_Update)
    {
        auto body = reinterpret_cast<CBotNPCBody *>(this);
        auto mod = GetNextbotModuleBody(body);
        if (mod->m_flAutoJumpMaxTime != 0) {
            if (mod->m_autoJumpTimer.IsElapsed()) {
                mod->m_autoJumpTimer.Start(RandomFloat(mod->m_flAutoJumpMinTime, mod->m_flAutoJumpMaxTime));
                mod->m_pEntity->GetLocomotionInterface()->Jump();
            }
        }
    }

    VHOOK_DECL(void, MyNextbotBody_AimHeadTowards_Vec, const Vector& vec, IBody::LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason)
    {
        if ( duration <= 0.0f )
        {
            duration = 0.1f;
        }
        auto body = reinterpret_cast<CBotNPCBody *>(this);
        auto entity = body->GetBot()->GetEntity();
        const float epsilon = 1.0f;
        auto mod = GetNextbotModuleBody(body);
        bool sameTarget = ( mod->m_lookAtPos - vec ).IsLengthLessThan( epsilon );
        // don't spaz our aim around
        // Msg("Steady %d %d\n", !body->IsHeadSteady(), body->GetHeadSteadyDuration() < nb_head_aim_settle_duration->GetFloat());
        if ( !sameTarget && mod->m_lookAtPriority == priority )
        {
            if ( !body->IsHeadSteady() || body->GetHeadSteadyDuration() < nb_head_aim_settle_duration->GetFloat() )
            {
                // we're still finishing a look-at at the same priority
                if ( replyWhenAimed ) 
                {
                    replyWhenAimed->OnFail( body->GetBot(), INextBotReply::FailureReason::REJECTED );
                }
                return;
            }
        }

        // don't short-circuit if "sighted in" to avoid rapid view jitter
        if ( !sameTarget && mod->m_lookAtPriority > priority && !mod->m_lookAtExpireTimer.IsElapsed() )
        {
            // higher priority lookat still ongoing
            if ( replyWhenAimed ) 
            {
                replyWhenAimed->OnFail( body->GetBot(), INextBotReply::FailureReason::REJECTED );
            }
            return;
        }

        if ( mod->m_lookAtReplyWhenAimed )
        {
            // in-process aim was interrupted
            mod->m_lookAtReplyWhenAimed->OnFail( body->GetBot(), INextBotReply::FailureReason::PREEMPTED );
        }

        mod->m_lookAtReplyWhenAimed = replyWhenAimed;
        mod->m_lookAtExpireTimer.Start( duration );

        // if given the same subject, just update priority
        if ( sameTarget )
        {
            mod->m_lookAtPriority = priority;
            return;
        }

        // new subject
        mod->m_lookAtSubject = nullptr;
        mod->m_lookAtPos = vec;

        mod->m_lookAtPriority = priority;
        mod->m_lookAtDurationTimer.Start();

        // do NOT clear this here, or continuous calls to AimHeadTowards will keep IsHeadAimingOnTarget returning false all of the time
        Vector to = mod->m_lookAtPos - entity->EyePosition();
        to.NormalizeInPlace();

        QAngle desiredAngles;
        VectorAngles( to, desiredAngles );
        float dot = DotProduct( mod->m_currentAimForward, to );
        if ( dot < 0.98f )
        {
            mod->m_isSightedIn = false;
        }

        mod->m_hasBeenSightedIn = false;
    }

    VHOOK_DECL(void, MyNextbotBody_AimHeadTowards, CBaseEntity *subject, IBody::LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason)
    {
        if ( duration <= 0.0f )
        {
            duration = 0.1f;
        }

        if ( subject == NULL )
        {
            return;
        }

        auto body = reinterpret_cast<CBotNPCBody *>(this);
        auto entity = body->GetBot()->GetEntity();
        auto mod = GetNextbotModuleBody(body);
        bool sameTarget = subject == mod->m_lookAtSubject;
        // don't spaz our aim around
        if (!sameTarget && mod->m_lookAtPriority == priority )
        {
            if ( !body->IsHeadSteady() || body->GetHeadSteadyDuration() < nb_head_aim_settle_duration->GetFloat() )
            {
                // we're still finishing a look-at at the same priority
                if ( replyWhenAimed ) 
                {
                    replyWhenAimed->OnFail( body->GetBot(), INextBotReply::FailureReason::REJECTED );
                }
                return;
            }
        }

        // don't short-circuit if "sighted in" to avoid rapid view jitter
        if (!sameTarget && mod->m_lookAtPriority > priority && !mod->m_lookAtExpireTimer.IsElapsed() )
        {
            // higher priority lookat still ongoing
            if ( replyWhenAimed ) 
            {
                replyWhenAimed->OnFail( body->GetBot(), INextBotReply::FailureReason::REJECTED );
            }
            return;
        }

        if ( mod->m_lookAtReplyWhenAimed )
        {
            // in-process aim was interrupted
            mod->m_lookAtReplyWhenAimed->OnFail( body->GetBot(), INextBotReply::FailureReason::PREEMPTED );
        }

        mod->m_lookAtReplyWhenAimed = replyWhenAimed;
        mod->m_lookAtExpireTimer.Start( duration );

        // if given the same subject, just update priority
        if (sameTarget)
        {
            mod->m_lookAtPriority = priority;
            return;
        }

        // new subject
        mod->m_lookAtSubject = subject;

        mod->m_lookAtPriority = priority;
        mod->m_lookAtDurationTimer.Start();
        
        Vector to = mod->m_lookAtSubject->EyePosition() - entity->EyePosition();
        to.NormalizeInPlace();

        QAngle desiredAngles;
        VectorAngles( to, desiredAngles );
        float dot = DotProduct( mod->m_currentAimForward, to );
        if ( dot < 0.98f )
        {
            mod->m_isSightedIn = false;
        }

        mod->m_hasBeenSightedIn = false;
    }

    

    VHOOK_DECL(void, MyNextbotBody_Reset)
    {
        VHOOK_CALL();
        auto body = reinterpret_cast<CBotNPCBody *>(this);
        auto entity = body->GetBot()->GetEntity();
        MyNextbotModule *mod = entity->GetEntityModule<MyNextbotModule>("mynextbotmodule");
        mod->m_posture = IBody::PostureType::STAND;
        mod->m_lookAtPos = vec3_origin;
        mod->m_lookAtSubject = nullptr;
        mod->m_lookAtReplyWhenAimed = nullptr;
        mod->m_lookAtVelocity = vec3_origin;
        mod->m_lookAtExpireTimer.Invalidate();
        mod->m_lookAtPriority = IBody::LookAtPriorityType::BORING;
        mod->m_lookAtDurationTimer.Invalidate();
        mod->m_isSightedIn = false;
        mod->m_hasBeenSightedIn = false;
        mod->m_headSteadyTimer.Invalidate();
        mod->m_priorAngles = vec3_angle;
        mod->m_anchorRepositionTimer.Invalidate();
        mod->m_anchorForward = vec3_origin;
        body->m_moveXPoseParameter = -1;
        body->m_moveYPoseParameter = -1;
		mod->m_iBodyYawParam = -1;
		mod->m_iBodyPitchParam = -1;
		mod->m_currentAimAngles = vec3_angle;
        mod->m_currentAimForward = vec3_origin;
        mod->m_forcedActivity = ACT_INVALID;
        mod->m_untranslatedActivity = ACT_INVALID;
        mod->m_iForcedActivityFlags = 0U;
        mod->m_bShouldShoot = false;
        mod->m_bDuckForced = false;
        mod->m_bInAirWalk = false;
        // Reuse existing no longer used field for something else
        *((MyNextbotModule **)&body->m_desiredAimAngles) = mod;
    }

    VHOOK_DECL(bool, MyNextbotBody_IsHeadSteady)
    {
        return GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_headSteadyTimer.HasStarted();
    }

    VHOOK_DECL(float, MyNextbotBody_GetHeadSteadyDuration)
    {
        auto &timer = GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_headSteadyTimer;
        return timer.HasStarted() ? timer.GetElapsedTime() : 0.0f;
    }

    VHOOK_DECL(void, MyNextbotBody_ClearPendingAimReply)
    {
        GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_lookAtReplyWhenAimed = nullptr;
    }

    VHOOK_DECL(float, MyNextbotBody_GetMaxHeadAngularVelocity)
    {
        return GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_flMaxHeadAngularVelocity;//reinterpret_cast<CBotNPCBody *>(this)->GetBot()->GetEntity()->GetEntityModule<MyNextbotModule>("mynextbotmodule")->m_flHeadTrackingInterval;
    }

    constexpr float stand_to_crouch_ratio = 82.0f/62.0f;
    constexpr float crouch_to_stand_ratio = 62.0f/82.0f;
    VHOOK_DECL(void, MyNextbotBody_SetDesiredPosture, IBody::PostureType posture )
    {
        auto body = reinterpret_cast<CBotNPCBody *>(this);
        auto mod = GetNextbotModuleBody(body);
        auto &curPosture = mod->m_posture;
        if (mod->m_bDuckForced) posture = IBody::PostureType::CROUCH;

        if (curPosture != posture) {
            curPosture = posture;
            auto ent = mod->m_pEntity;
            if (posture == IBody::PostureType::STAND) {
                Vector newMaxs = ent->CollisionProp()->OBBMaxsPreScaled();
                newMaxs.z *= stand_to_crouch_ratio;
                UTIL_SetSize(ent, &ent->CollisionProp()->OBBMinsPreScaled(), &newMaxs);
	            ent->m_fFlags &= ~FL_DUCKING;
                mod->SetEyeOffset();
            }
            else {
                Vector newMaxs = ent->CollisionProp()->OBBMaxsPreScaled();
                newMaxs.z *= crouch_to_stand_ratio;
                UTIL_SetSize(ent, &ent->CollisionProp()->OBBMinsPreScaled(), &newMaxs);
	            ent->m_fFlags |= FL_DUCKING;
                mod->SetEyeOffset();
            }
        }
    }

    VHOOK_DECL(IBody::PostureType, MyNextbotBody_GetDesiredPosture)
    {
        return GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_posture;
    }

    VHOOK_DECL(bool, MyNextbotBody_IsDesiredPosture, IBody::PostureType posture)
    {
        return ( posture == GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_posture );
    }

    VHOOK_DECL(IBody::PostureType, MyNextbotBody_GetActualPosture)
    {
        return GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_posture;
    }

    VHOOK_DECL(bool, MyNextbotBody_IsActualPosture, IBody::PostureType posture)
    {
        return ( posture == GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_posture );
    }

    VHOOK_DECL(float, MyNextbotBody_GetHeadAimTrackingInterval)
    {
        return GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_flHeadTrackingInterval ;
    }

    VHOOK_DECL(float, MyNextbotBody_GetHeadAimSubjectLeadTime)
    {
        return GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_flHeadAimLeadTime;
    }

    VHOOK_DECL(float, MyNextbotBody_GetStandHullHeight)
    {
        auto mod = GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this));
        auto prop = mod->m_pEntity->CollisionProp();
        float mult = mod->m_posture == IBody::PostureType::STAND ? 1.0f : stand_to_crouch_ratio;
        return (prop->OBBMaxs().z - prop->OBBMins().z) * mult;
    }

    VHOOK_DECL(float, MyNextbotBody_GetCrouchHullHeight)
    {
        
        auto mod = GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this));
        auto prop = mod->m_pEntity->CollisionProp();
        float mult = mod->m_posture == IBody::PostureType::CROUCH ? 1.0f : crouch_to_stand_ratio;
        return (prop->OBBMaxs().z - prop->OBBMins().z) * mult;
    }

    VHOOK_DECL(float, MyNextbotBody_GetHullWidth)
    {
        auto mod = GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this));
        auto prop = mod->m_pEntity->CollisionProp();
        return Max(prop->OBBMaxs().x - prop->OBBMins().x, prop->OBBMaxs().y - prop->OBBMins().y);
    }

    VHOOK_DECL(float, MyNextbotBody_GetHullHeight)
    {
        auto mod = GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this));
        auto prop = mod->m_pEntity->CollisionProp();
        return prop->OBBMaxs().z - prop->OBBMins().z;
    }

    VHOOK_DECL(const Vector &, MyNextbotBody_GetHullMins)
    {
        auto mod = GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this));
        return  mod->m_pEntity->CollisionProp()->OBBMins();
    }

    VHOOK_DECL(const Vector &, MyNextbotBody_GetHullMaxs)
    {
        auto mod = GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this));
        return mod->m_pEntity->CollisionProp()->OBBMaxs();
    }

    VHOOK_DECL(uint, MyNextbotBody_GetSolidMask)
    {
        auto mod = GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this));
        int contents = MASK_PLAYERSOLID & ~CONTENTS_PLAYERCLIP;
        if (!mod->m_bIgnoreClips) {
            contents |= CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP;
        }
        if (!mod->m_bNotSolidToPlayers) {
            auto numteam = mod->m_pEntity->GetTeamNumber();
            if (numteam != TF_TEAM_BLUE) {
                contents |= CONTENTS_BLUETEAM;
            }
            if (numteam != TF_TEAM_RED) {
                contents |= CONTENTS_REDTEAM;
            }
            // auto team = mod->m_pEntity->GetTeamNumber();
            // if (team != TF_TEAM_RED && team != TF_TEAM_BLUE) && (contentsMask & (CONTENTS_REDTEAM | CONTENTS_BLUETEAM)) != 0) {
            // //     return true;
            // //)
        }
        return contents;
    }

    

    VHOOK_DECL(bool, MyNextbotBody_StartActivity, Activity activity, uint flags)
    {
        auto body = reinterpret_cast<CBotNPCBody *>(this);
        auto mod = GetNextbotModuleBody(body);
        auto result = SwitchToActivity(body, (MyNextbotEntity *)body->GetBot()->GetEntity(), mod, activity, body->GetBot()->GetEntity()->GetModelPtr());
        if (result) {
            mod->m_forcedActivity = activity;
            mod->m_iForcedActivityFlags = flags;
        }
        return result;
    }

    VHOOK_DECL(const Vector &, MyNextbotBody_GetViewVector)
    {
        return GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_currentAimForward;
    }

    VHOOK_DECL(void, MyNextbotBody_OnLandOnGround, CBaseEntity *ground)
    {
        auto body = reinterpret_cast<CBotNPCBody *>(this);
        auto mod = GetNextbotModuleBody(body);
        mod->m_pEntity->AddGesture(TranslateActivity(body, mod->m_pEntity, mod, ACT_MP_JUMP_LAND));
        VHOOK_CALL(ground);
    }
    
    VHOOK_DECL(bool, MyNextbotBody_IsHeadAimingOnTarget)
    {
        return GetNextbotModuleBody(reinterpret_cast<CBotNPCBody *>(this))->m_isSightedIn;
    }
    
    void LoadBodyHooks(void **vtable) {
        CVirtualHook hooks[] = {
            CVirtualHook(TypeName<CBotNPCBody>(), "INextBotComponent::Upkeep", GET_VHOOK_CALLBACK(MyNextbotBody_Upkeep), GET_VHOOK_INNERPTR(MyNextbotBody_Upkeep)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::Update", GET_VHOOK_CALLBACK(MyNextbotBody_Update), GET_VHOOK_INNERPTR(MyNextbotBody_Update)),
            CVirtualHook(TypeName<CBotNPCBody>(), "CBotNPCBody::AimHeadTowards [Vector]", GET_VHOOK_CALLBACK(MyNextbotBody_AimHeadTowards_Vec), GET_VHOOK_INNERPTR(MyNextbotBody_AimHeadTowards_Vec)),
            CVirtualHook(TypeName<CBotNPCBody>(), "CBotNPCBody::AimHeadTowards [CBaseEntity *]", GET_VHOOK_CALLBACK(MyNextbotBody_AimHeadTowards), GET_VHOOK_INNERPTR(MyNextbotBody_AimHeadTowards)),
            CVirtualHook(TypeName<CBotNPCBody>(), TypeName<CZombieIntention>(), "CZombieIntention::Reset", GET_VHOOK_CALLBACK(MyNextbotBody_Reset), GET_VHOOK_INNERPTR(MyNextbotBody_Reset)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::IsHeadSteady", GET_VHOOK_CALLBACK(MyNextbotBody_IsHeadSteady), GET_VHOOK_INNERPTR(MyNextbotBody_IsHeadSteady)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetHeadSteadyDuration", GET_VHOOK_CALLBACK(MyNextbotBody_GetHeadSteadyDuration), GET_VHOOK_INNERPTR(MyNextbotBody_GetHeadSteadyDuration)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::ClearPendingAimReply", GET_VHOOK_CALLBACK(MyNextbotBody_ClearPendingAimReply), GET_VHOOK_INNERPTR(MyNextbotBody_ClearPendingAimReply)),
            CVirtualHook(TypeName<CBotNPCBody>(), "10PlayerBody", "PlayerBody::GetMaxHeadAngularVelocity", GET_VHOOK_CALLBACK(MyNextbotBody_GetMaxHeadAngularVelocity), GET_VHOOK_INNERPTR(MyNextbotBody_GetMaxHeadAngularVelocity)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::SetDesiredPosture", GET_VHOOK_CALLBACK(MyNextbotBody_SetDesiredPosture), GET_VHOOK_INNERPTR(MyNextbotBody_SetDesiredPosture)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetDesiredPosture", GET_VHOOK_CALLBACK(MyNextbotBody_GetDesiredPosture), GET_VHOOK_INNERPTR(MyNextbotBody_GetDesiredPosture)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetActualPosture", GET_VHOOK_CALLBACK(MyNextbotBody_GetActualPosture), GET_VHOOK_INNERPTR(MyNextbotBody_GetActualPosture)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::IsActualPosture", GET_VHOOK_CALLBACK(MyNextbotBody_IsActualPosture), GET_VHOOK_INNERPTR(MyNextbotBody_IsActualPosture)),
            CVirtualHook(TypeName<CBotNPCBody>(), "10CTFBotBody", "CTFBotBody::GetHeadAimTrackingInterval", GET_VHOOK_CALLBACK(MyNextbotBody_GetHeadAimTrackingInterval), GET_VHOOK_INNERPTR(MyNextbotBody_GetHeadAimTrackingInterval)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetStandHullHeight", GET_VHOOK_CALLBACK(MyNextbotBody_GetStandHullHeight), GET_VHOOK_INNERPTR(MyNextbotBody_GetStandHullHeight)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetCrouchHullHeight", GET_VHOOK_CALLBACK(MyNextbotBody_GetCrouchHullHeight), GET_VHOOK_INNERPTR(MyNextbotBody_GetCrouchHullHeight)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetCrouchHullHeight", GET_VHOOK_CALLBACK(MyNextbotBody_GetCrouchHullHeight), GET_VHOOK_INNERPTR(MyNextbotBody_GetCrouchHullHeight)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetHullWidth", GET_VHOOK_CALLBACK(MyNextbotBody_GetHullWidth), GET_VHOOK_INNERPTR(MyNextbotBody_GetHullWidth)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetHullHeight", GET_VHOOK_CALLBACK(MyNextbotBody_GetHullHeight), GET_VHOOK_INNERPTR(MyNextbotBody_GetHullHeight)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetHullMins", GET_VHOOK_CALLBACK(MyNextbotBody_GetHullMins), GET_VHOOK_INNERPTR(MyNextbotBody_GetHullMins)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetHullMaxs", GET_VHOOK_CALLBACK(MyNextbotBody_GetHullMaxs), GET_VHOOK_INNERPTR(MyNextbotBody_GetHullMaxs)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::StartActivity", GET_VHOOK_CALLBACK(MyNextbotBody_StartActivity), GET_VHOOK_INNERPTR(MyNextbotBody_StartActivity)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody", "IBody::GetViewVector", GET_VHOOK_CALLBACK(MyNextbotBody_GetViewVector), GET_VHOOK_INNERPTR(MyNextbotBody_GetViewVector)),
            CVirtualHook(TypeName<CBotNPCBody>(), "CBotNPCBody::GetSolidMask", GET_VHOOK_CALLBACK(MyNextbotBody_GetSolidMask), GET_VHOOK_INNERPTR(MyNextbotBody_GetSolidMask)),
            CVirtualHook(TypeName<CBotNPCBody>(), "22INextBotEventResponder","INextBotEventResponder::OnLandOnGround", GET_VHOOK_CALLBACK(MyNextbotBody_OnLandOnGround), GET_VHOOK_INNERPTR(MyNextbotBody_OnLandOnGround)),
            CVirtualHook(TypeName<CBotNPCBody>(), "5IBody","IBody::IsHeadAimingOnTarget", GET_VHOOK_CALLBACK(MyNextbotBody_IsHeadAimingOnTarget), GET_VHOOK_INNERPTR(MyNextbotBody_IsHeadAimingOnTarget))
        };
        for (auto &hook : hooks) {
            hook.DoLoad();
            hook.AddToVTable(vtable);
        }
    }

    // static MemberFuncThunk<CBotNPCBody *, void, INextBot *> ft_CBotNPCBody_ctor("CBotNPCBody::CBotNPCBody");
    // class MyNextbotBody : public CBotNPCBody
    // {
    // public:

    //     static MyNextbotBody *New(INextBot *bot) {
    //         auto body = reinterpret_cast<MyNextbotBody *>(::operator new(sizeof(MyNextbotBody)));
	//         ft_CBotNPCBody_ctor(body, bot);
    //         //*((void ***)body) = MyBodyVTable+4;
    //         return body;
    //     } 
    // };
}