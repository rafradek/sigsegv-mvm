
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
#include "mod/bot/interrupt_action.h"
#include "mod/bot/guard_action.h"
#include "stub/usermessages_sv.h"
#include "stub/particles.h"
#include "stub/objects.h"
#include "util/entity.h"

namespace Mod::AI::NPC_Nextbot
{

    Action<MyNextbotEntity> *CMyNextbotMainAction::InitialContainedAction(MyNextbotEntity *actor)
    {
        return new CMyNextbotScenarioMonitor();
    }

    ActionResult<MyNextbotEntity> CMyNextbotMainAction::OnStart(MyNextbotEntity *actor, Action<MyNextbotEntity> *action)
    {
        return ActionResult<MyNextbotEntity>::Continue();
    }

    ActionResult<MyNextbotEntity> CMyNextbotMainAction::Update(MyNextbotEntity *actor, float interval)
    {
        auto mod = GetNextbotModule(actor);
        auto vision = actor->GetVisionInterface();
        auto known = vision->GetPrimaryKnownThreat(false);
        if (known != nullptr && known->GetEntity() != nullptr && !mod->m_bIgnoreEnemies) {
            auto target = known->GetEntity();
            if (known->IsVisibleInFOVNow()) {
                actor->GetBodyInterface()->AimHeadTowards(target, IBody::LookAtPriorityType::CRITICAL, 1.0f, nullptr, "Aiming at threat");
            }
            else if (vision->IsLineOfSightClearToEntity(target, nullptr)) {
                actor->GetBodyInterface()->AimHeadTowards(target, IBody::LookAtPriorityType::IMPORTANT, 1.0f, nullptr, "Aiming at threat");
            }
            bool canSee = ((CTFBot *)actor)->IsLineOfFireClear(target->EyePosition()) 
                || ((CTFBot *)actor)->IsLineOfFireClear(target->WorldSpaceCenter())
                || ((CTFBot *)actor)->IsLineOfFireClear(target->GetAbsOrigin());

            if (canSee) {
                auto range = (target->GetAbsOrigin() - actor->GetAbsOrigin()).Length();
                //Msg("Aimingat %d\n", actor->GetBodyInterface()->IsHeadAimingOnTarget());
                if (mod->m_flHandAttackTime > 0.0f) {
                    if (range < mod->m_flHandAttackRange + 30.0f)
                        mod->SetShooting(true);
                }
                else if (actor->GetBodyInterface()->IsHeadAimingOnTarget()) {
                    mod->SetShooting(true);
                }
            }
        }

        return ActionResult<MyNextbotEntity>::Continue();
    }


	EventDesiredResult< MyNextbotEntity > CMyNextbotMainAction::OnKilled(MyNextbotEntity *actor, const CTakeDamageInfo &info) {
        CPVSFilter filter(actor->GetAbsOrigin() );
        auto mod = GetNextbotModule(actor);

        if (mod->m_bIsRobot) {
            StopParticleEffects(actor);
        }
        if (mod->m_hZombieCosmetic != nullptr) {
            mod->m_hZombieCosmetic->Remove();
        }

        bool gib = actor->ShouldGib(info);
        bool ragdoll = !gib && mod->m_DeathEffectType != MyNextbotModule::NONE;
        Vector force = actor->CalcDamageForceVector(info);
        if (ragdoll || gib) {
            for (auto child = actor->FirstMoveChild(); child != nullptr; child = child->NextMovePeer()) {
                if (rtti_cast<CEconEntity *>(child) != nullptr) {
                    
                    //auto physprop = CreateEntityByName("prop_physics_multiplayer");
                    //if (physprop != nullptr) {
                    //    
                    //}

                    // bf_write *msg = engine->UserMessageBegin(&filter, usermessages->LookupUserMessage("BreakModel"));
                    // msg->WriteShort(child->GetModelIndex());
                    // msg->WriteBitVec3Coord(actor->EyePosition());
                    // msg->WriteBitAngles(actor->GetAbsAngles());
                    // engine->MessageEnd();

                    

                    TE_BreakModel(filter, 0.0f, actor->EyePosition(), actor->EyeAngles(), Vector(0,0,0), force*0.03f, child->GetModelIndex(), 1, 1, 15.0f,0);
                    //TE_PhysicsProp(filter, 0.0f, child->GetModelIndex(), ((CBaseAnimating *)child)->m_nSkin, actor->EyePosition(),actor->EyeAngles(),actor->GetAbsVelocity(), 0, 0);
                    child->Remove();
                }
            }
        }
            
        if (gib) {
            bf_write *msg = engine->UserMessageBegin(&filter, usermessages->LookupUserMessage("BreakModel"));
            msg->WriteShort(actor->GetModelIndex());
            msg->WriteBitVec3Coord(actor->GetAbsOrigin());
            msg->WriteBitAngles(actor->GetAbsAngles());
            engine->MessageEnd();
            actor->Remove();
        }
        if (ragdoll) {
            actor->BecomeRagdoll(info, force);
        }

        if (!ragdoll && !gib) {
            actor->Remove();
        }

        return EventDesiredResult<MyNextbotEntity>::Done();
    }

    EventDesiredResult<MyNextbotEntity> CMyNextbotScenarioMonitor::OnCommandString(MyNextbotEntity *actor, const char *cmd)
    {
        if (actor->IsAlive() && V_strnicmp(cmd, "interrupt_action", strlen("interrupt_action")) == 0) {
            
            auto action = reinterpret_cast<Action<CTFBot> *>(this);

            return EventDesiredResult<MyNextbotEntity>::SuspendFor(CreateInterruptAction(actor, cmd), "Executing interrupt task");
        }
        
        return EventDesiredResult<MyNextbotEntity>::Continue();
    }

    /* mobber AI, based on MyNextbotEntityAttackFlagDefenders */
	class MyNextbotEntityMobber : public IHotplugAction<MyNextbotEntity>
	{
	public:
		MyNextbotEntityMobber()
		{
			this->m_Attack = new CMyNextbotAttack();
		}
		virtual ~MyNextbotEntityMobber()
		{
			if (this->m_Attack != nullptr) {
				delete this->m_Attack;
			}
			DevMsg("Remove mobber\n");
		}
		
		virtual void OnEnd(MyNextbotEntity *actor, Action<MyNextbotEntity> *action) override
		{
			DevMsg("On end mobber\n");
		}

		virtual const char *GetName() const override { return "Mobber"; }
		
		virtual ActionResult<MyNextbotEntity> OnStart(MyNextbotEntity *actor, Action<MyNextbotEntity> *action) override
		{
			this->m_PathFollower.SetMinLookAheadDistance(300.0f);
			
			this->m_hTarget = nullptr;

			return this->m_Attack->OnStart(actor, action);
		}
		
		virtual ActionResult<MyNextbotEntity> Update(MyNextbotEntity *actor, float dt) override
		{
			const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
			
			ActionResult<MyNextbotEntity> result = this->m_Attack->Update(actor, dt);
			if (result.transition != ActionTransition::DONE) {
				return ActionResult<MyNextbotEntity>::Continue();
			}
			
			/* added teamnum check to fix some TF_COND_REPROGRAMMED quirks */
			if (this->m_hTarget == nullptr || !this->m_hTarget->IsAlive() || this->m_hTarget->GetTeamNumber() == actor->GetTeamNumber()) {
				
				this->m_hTarget = SelectRandomReachableEnemy(actor->GetTeamNumber());
				
				if (this->m_hTarget == nullptr) {
					CBaseEntity *target_ent = nullptr;
					//auto attrs = actor->ExtAttr();
					//if (!attrs[MyNextbotEntity::ExtendedAttr::IGNORE_NPC]) {
						do {
							target_ent = servertools->FindEntityByClassname(target_ent, "tank_boss");

							if (target_ent != nullptr && target_ent->GetTeamNumber() != actor->GetTeamNumber()) {
								this->m_hTarget = target_ent->MyCombatCharacterPointer();
								break;
							}

						} while (target_ent != nullptr);
						
						target_ent = nullptr;
						do {
							target_ent = servertools->FindEntityByClassname(target_ent, "headless_hatman");

							if (target_ent != nullptr && target_ent->GetTeamNumber() != actor->GetTeamNumber()) {
								this->m_hTarget = target_ent->MyCombatCharacterPointer();
								break;
							}

						} while (target_ent != nullptr);
					//}
				}
				
				if (this->m_hTarget == nullptr) {
					CBaseEntity *target_ent = nullptr;
					//auto attrs = actor->ExtAttr();
					std::vector<CBaseObject *> objs;
					//if (!attrs[MyNextbotEntity::ExtendedAttr::IGNORE_BUILDINGS]) {
						for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
							auto obj = rtti_scast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
							if (obj != nullptr && obj->GetType() == OBJ_SENTRYGUN && obj->IsTargetable() && obj->GetTeamNumber() != actor->GetTeamNumber()) {
								objs.push_back(obj);
							}
						}
					//}
				    if (!objs.empty()) {
						this->m_hTarget = objs[RandomInt(0, objs.size()-1)];
					}
				}
				if (this->m_hTarget == nullptr) {
					return ActionResult<MyNextbotEntity>::Continue();
				}
			}
			
			actor->GetVisionInterface()->AddKnownEntity(this->m_hTarget);
			
			auto nextbot = actor->MyNextBotPointer();
			
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
				
				CZombiePathCost cost_func((CZombie*) actor);
				this->m_PathFollower.Compute(nextbot, this->m_hTarget, cost_func, 0.0f, true);
			}
			
			this->m_PathFollower.Update(nextbot);
			
			return ActionResult<MyNextbotEntity>::Continue();
		}
		
	private:
		CMyNextbotAttack *m_Attack = nullptr;
		
		CHandle<CBaseCombatCharacter> m_hTarget;
		
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
	};

    
    class CMyNextbotPassive : public IHotplugAction<MyNextbotEntity>
	{
	public:
		CMyNextbotPassive()
		{
			this->m_Attack = new CMyNextbotAttack();
		}
		virtual ~CMyNextbotPassive()
		{
			if (this->m_Attack != nullptr) {
				delete this->m_Attack;
			}
			DevMsg("Remove mobber\n");
		}
		
		virtual void OnEnd(MyNextbotEntity *actor, Action<MyNextbotEntity> *action) override
		{
			DevMsg("On end mobber\n");
		}

		virtual const char *GetName() const override { return "Passive"; }
		
		virtual ActionResult<MyNextbotEntity> OnStart(MyNextbotEntity *actor, Action<MyNextbotEntity> *action) override
		{	
			return this->m_Attack->OnStart(actor, action);
		}
		
		virtual ActionResult<MyNextbotEntity> Update(MyNextbotEntity *actor, float dt) override
		{
			DevMsg("Passive update\n");
			ActionResult<MyNextbotEntity> result = this->m_Attack->Update(actor, dt);
			
			return ActionResult<MyNextbotEntity>::Continue();
		}
		
	private:
		CMyNextbotAttack *m_Attack = nullptr;
	};

    Action<MyNextbotEntity> *GetCurrentAction(MyNextbotEntity *actor) {
        auto mod = GetNextbotModule(actor);
        auto str = actor->GetCustomVariable<"action">(PStr<"guard">());
        mod->m_strCurAction = str;
        if (str == PStr<"guard">()) {
            return nullptr;
        }
        else if (str == PStr<"passive">()) {
            return new CMyNextbotPassive();
        }
        else if (str == PStr<"mobber">()) {
            return new MyNextbotEntityMobber();
        }
        else if (str == PStr<"followpath">()) {
            return new CTFBotGuardAction<MyNextbotEntity>();
        }
        return nullptr;
    }

    Action<MyNextbotEntity> *CMyNextbotScenarioMonitor::InitialContainedAction(MyNextbotEntity *actor)
    {
        return GetCurrentAction(actor);
    }

    ActionResult<MyNextbotEntity> CMyNextbotScenarioMonitor::Update(MyNextbotEntity *actor, float interval)
    {
        auto mod = GetNextbotModule(actor);
        auto str = actor->GetCustomVariable<"action">(PStr<"guard">());
        if (str != mod->m_strCurAction) {
            return ActionResult<MyNextbotEntity>::ChangeTo(new CMyNextbotScenarioMonitor(), "Change to new action");
        }
        return ActionResult<MyNextbotEntity>::Continue();
    }

	bool IsRangeLessThan( CBaseEntity *bot, const Vector &pos, float range)
	{
		Vector to = pos - bot->GetAbsOrigin();
		return to.IsLengthLessThan(range);
	}
    
	bool IsRangeGreaterThan( CBaseEntity *bot, const Vector &pos, float range)
	{
		Vector to = pos - bot->GetAbsOrigin();
		return to.IsLengthGreaterThan(range);
	}
    
    ActionResult<MyNextbotEntity> CMyNextbotAttack::OnStart( MyNextbotEntity *actor, Action< MyNextbotEntity > *priorAction )
    {
        Msg("Attack\n");
        m_path.SetMinLookAheadDistance(300.0f);

        return ActionResult<MyNextbotEntity>::Continue();
    }


    ActionResult<MyNextbotEntity> CMyNextbotAttack::Update(MyNextbotEntity *actor, float interval)
    {
        auto mod = GetNextbotModule(actor);
        auto nextBot = actor->MyNextBotPointer();
        auto vision = actor->GetVisionInterface();
        auto known = vision->GetPrimaryKnownThreat(false);
        if (known == nullptr || known->IsObsolete() || actor->GetIntentionInterface()->ShouldAttack(nextBot, known) == QueryResponse::NO) {
            return ActionResult<MyNextbotEntity>::Done("No threat");
        }
        auto target = known->GetEntity();

        if (mod->m_bIgnoreEnemies)
            return ActionResult<MyNextbotEntity>::Continue();

        auto range = mod->GetAttackRange();
        if (!known->IsVisibleRecently() || IsRangeGreaterThan(actor, target->GetAbsOrigin(), range) || !((CTFBot *)actor)->IsLineOfFireClear(target->EyePosition())) {
            if (known->IsVisibleRecently()) {
                CZombiePathCost cost_func((CZombie *) actor);
                m_chasePath.Update(nextBot, target, cost_func, nullptr);
                m_chasePath.ShortenFailTimer(0.5f);
            }
            else {
                auto &lastKnownPosition = *known->GetLastKnownPosition();
                // if we're at the threat's last known position and he's still not visible, we lost him
                m_chasePath.Invalidate();

                if (IsRangeLessThan(actor, lastKnownPosition, 20.0f)) {
                    actor->GetVisionInterface()->ForgetEntity( target );
                    return ActionResult<MyNextbotEntity>::Done( "I lost my target!" );
                }

                // look where we last saw him as we approach
                if (IsRangeLessThan(actor, lastKnownPosition, range) ) {
                    actor->GetBodyInterface()->AimHeadTowards( lastKnownPosition + Vector( 0, 0, 62 ), IBody::LookAtPriorityType::IMPORTANT, 0.2f, nullptr, "Looking towards where we lost sight of our victim" );
                }

                m_path.Update( nextBot );

                if (m_repathTimer.IsElapsed()) {
                    //m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );
                    m_repathTimer.Start( RandomFloat( 3.0f, 5.0f ) );
                    
                    CZombiePathCost cost_func((CZombie *) actor);
                    float maxPathLength = 0.0f;
                    m_path.Compute( nextBot, lastKnownPosition, cost_func, maxPathLength, true );
                }
            }
        }

        return ActionResult<MyNextbotEntity>::Continue();
    }
}