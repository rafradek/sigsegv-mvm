#include "mod.h"
#include "mod/bot/interrupt_action.h"
#include "stub/nextbot_cc.h"
#include "util/iterate.h"
#include "mod/ai/npc_nextbot/npc_nextbot.h"
#include "util/entity.h"

template <class Actor>
ActionResult<Actor> CTFBotMoveTo<Actor>::OnStart(Actor *actor, Action<Actor> *action)
{
    auto tfbot = ToTFBot(actor);
    actor->GetLocomotionInterface()->Reset();
    this->m_PathFollower.SetMinLookAheadDistance(tfbot != nullptr ? tfbot->GetDesiredPathLookAheadRange() : 250.0f);
    
    return ActionResult<Actor>::Continue();
}

std::map<CHandle<CBaseEntity>, std::string> change_attributes_delay;

THINK_FUNC_DECL(ChangeAttributes) {
    CTFBot *bot = reinterpret_cast<CTFBot *>(this);

    std::string str = change_attributes_delay[bot];
    auto attribs = bot->GetEventChangeAttributes(str.c_str());
    if (attribs != nullptr)
        bot->OnEventChangeAttributes(attribs);

    change_attributes_delay.erase(bot);
}

template <class Actor>
ActionResult<Actor> CTFBotMoveTo<Actor>::Update(Actor *actor, float dt)
{
    if (!actor->IsAlive())
        return ActionResult<Actor>::Done();
    
    auto vision = actor->GetVisionInterface();
    auto tfbot = ToTFBot(actor);
    const CKnownEntity *threat = vision->GetPrimaryKnownThreat(false);
    if (threat != nullptr && tfbot != nullptr) {
        tfbot->EquipBestWeaponForThreat(threat);
    }
    
    Vector pos = vec3_origin;
    if (this->m_hTarget != nullptr) {
        pos = this->m_hTarget->WorldSpaceCenter();
    }
    else {
        pos = this->m_TargetPos;
    }

    Vector look = vec3_origin;
    if (this->m_hTargetAim != nullptr) {
        look = this->m_hTargetAim->WorldSpaceCenter();
    }
    else {
        look = this->m_TargetAimPos;
    }

    auto nextbot = actor->MyNextBotPointer();
    
    bool inRange = actor->GetAbsOrigin().DistToSqr(pos) < m_fDistanceSq && !(look != vec3_origin && this->m_hTargetAim != nullptr && !vision->IsLineOfSightClearToEntity(this->m_hTargetAim, &look));
    if (!inRange) {
        if (this->m_hTarget != nullptr) {
            if (tfbot != nullptr) {
                CTFBotPathCost cost_func(tfbot, DEFAULT_ROUTE);
                this->m_ChasePath.Update(nextbot, this->m_hTarget, cost_func, nullptr);
            }
            else {
                CZombiePathCost cost_func((CZombie *) actor);
                this->m_ChasePath.Update(nextbot, this->m_hTarget, cost_func, nullptr);
            }
        }
        else if (this->m_ctRecomputePath.IsElapsed()) {
            this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
            
            if (pos != vec3_origin) {
                this->m_ChasePath.Invalidate();
                if (tfbot != nullptr) {
                    CTFBotPathCost cost_func(tfbot, DEFAULT_ROUTE);
                    this->m_PathFollower.Compute(nextbot, pos, cost_func, 0.0f, true);
                }
                else {
                    CZombiePathCost cost_func((CZombie *) actor);
                    this->m_PathFollower.Compute(nextbot, pos, cost_func, 0.0f, true);
                }
            }
        }
    }
    else {
        this->m_PathFollower.Invalidate();
    }
    if (!m_bDone) {

        if (!m_bWaitUntilDone) {
            m_bDone = true;
        }
        else if (m_bKillLook) {
            m_bDone = this->m_hTargetAim == nullptr || !this->m_hTargetAim->IsAlive();
        }
        else if (pos == vec3_origin || TheNavMesh->GetNavArea(pos) == actor->GetLastKnownArea() || inRange) {
            m_bDone = true;
        }

        this->m_ctDoneAction.Start(m_fDuration);
    }
    if (m_bDone && this->m_ctDoneAction.IsElapsed()) {
        if (this->m_strOnDoneAttributes != "") {
            change_attributes_delay[actor] = this->m_strOnDoneAttributes;
            THINK_FUNC_SET(actor, ChangeAttributes, gpGlobals->curtime);
        }
        if (this->m_strName != "") {
            variant_t variant;
            variant.SetString(AllocPooledString(this->m_strName.c_str()));
            ((CBaseEntity *)actor)->FireCustomOutput<"onactiondone">(actor, actor, variant);
        }
        if (this->m_pNext != nullptr) {
            auto nextAction = this->m_pNext;
            this->m_pNext = nullptr;
            return ActionResult<Actor>::ChangeTo(nextAction, "Switch to next interrupt action in queue");
        }
        return ActionResult<Actor>::Done( "Successfully moved to area" );
    }
    
    if (this->m_hTarget == nullptr) {
        this->m_ChasePath.Invalidate();
        this->m_PathFollower.Update(nextbot);
    }
    
    if (look != vec3_origin) {
        
        bool look_now = !m_bKillLook || m_bAlwaysLook;
        if (m_bKillLook) {
            look_now |= tfbot != nullptr && tfbot->HasAttribute(CTFBot::ATTR_IGNORE_ENEMIES);
            if (this->m_hTargetAim != nullptr && vision->IsLineOfSightClearToEntity(this->m_hTargetAim, &look)) {
                look_now = true;
                if (actor->GetBodyInterface()->IsHeadAimingOnTarget()) {
                    if (tfbot != nullptr) {
                        tfbot->EquipBestWeaponForThreat(nullptr);
			            tfbot->PressFireButton();
                    }
                    else {
                        auto mod = ((CBaseEntity *)actor)->GetEntityModule<Mod::AI::NPC_Nextbot::MyNextbotModule>("mynextbotmodule");
                        if (mod != nullptr) {
                            mod->SetShooting(true);
                        }
                    }
                }
            }
        }

        if (look_now) {
            if (this->m_hTargetAim != nullptr) {
                actor->GetBodyInterface()->AimHeadTowards(this->m_hTargetAim, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.2f, NULL, "Aiming at target we need to destroy to progress");
            }
            else {
                actor->GetBodyInterface()->AimHeadTowards(look, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.2f, NULL, "Aiming at target we need to destroy to progress");
            }
        }
    }

    return ActionResult<Actor>::Continue();
}

template <class Actor>
EventDesiredResult<Actor> CTFBotMoveTo<Actor>::OnCommandString(Actor *actor, const char *cmd)
{

    if (V_stricmp(cmd, "stop interrupt action") == 0) {
        return EventDesiredResult<Actor>::Done("Stopping interrupt action");
    }
    else if (V_strnicmp(cmd, "interrupt_action_queue", strlen("interrupt_action_queue")) == 0) {
        CTFBotMoveTo *action = this;
        while(action->m_pNext != nullptr) {
            action = action->m_pNext;
        }
        action->m_pNext = CreateInterruptAction(actor, cmd);
        return EventDesiredResult<Actor>::Sustain("Add to queue");
    }
    else if (V_stricmp(cmd, "clear_interrupt_action_queue") == 0) {
        delete this->m_pNext;
        return EventDesiredResult<Actor>::Sustain("Clear queue");
    }
    else if (V_stricmp(cmd, "remove_interrupt_action_queue_name") == 0) {
        CTFBotMoveTo *action = this;
        CCommand command = CCommand();
        command.Tokenize(cmd);
        while(action->m_pNext != nullptr) {
            if (FStrEq(action->m_pNext->GetName(), command[1])) {
                auto actionToDelete = action->m_pNext;
                action->m_pNext = actionToDelete->m_pNext;
                delete actionToDelete;
                break;
            }
            action = action->m_pNext;
        }
        return EventDesiredResult<Actor>::Sustain("Delete from queue");
    }
    
    return EventDesiredResult<Actor>::Continue();
}

CBaseEntity *SelectTargetByName(CBaseCombatCharacter *actor, const char *name)
{
    CBaseEntity *target = servertools->FindEntityByName(nullptr, name, actor);
    if (target == nullptr && FStrEq(name,"RandomEnemy")) {
        target = SelectRandomReachableEnemy(actor->GetTeamNumber());
    }
    else if (target == nullptr && FStrEq(name, "ClosestPlayer")) {
        float closest_dist = FLT_MAX;
        ForEachTFPlayer([&](CTFPlayer *player){
            if (player->IsAlive() && !player->IsBot()) {
                float dist = player->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
                if (dist < closest_dist) {
                    closest_dist = dist;
                    target = player;
                }
            }
        });
    }
    else if (target == nullptr) {
        ForEachTFBot([&](CTFBot *bot) {
            if (bot->IsAlive() && FStrEq(bot->GetPlayerName(), name)) {
                target = bot; 
            }
        });
    }
    if (target == nullptr) {
        float closest_dist = FLT_MAX;
        ForEachEntityByClassname(name, [&](CBaseEntity *entity) {
            float dist = entity->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
            if (dist < closest_dist) {
                closest_dist = dist;
                target = entity;
            }
        });
    }
    return target;

}
template<class Actor>
void CTFBotMoveTo<Actor>::Init(Actor *actor, const char *cmd) {
    CCommand command = CCommand();
    command.Tokenize(cmd);
    
    const char *other_target = "";

    for (int i = 1; i < command.ArgC(); i++) {
        if (strcmp(command[i], "-name") == 0) {
            this->SetName(command[i+1]);
            i++;
        }
        else if (strcmp(command[i], "-pos") == 0) {
            Vector pos;
            pos.x = strtof(command[i+1], nullptr);
            pos.y = strtof(command[i+2], nullptr);
            pos.z = strtof(command[i+3], nullptr);

            this->SetTargetPos(pos);
            i += 3;
        }
        else if (strcmp(command[i], "-lookpos") == 0) {
            Vector pos;
            pos.x = strtof(command[i+1], nullptr);
            pos.y = strtof(command[i+2], nullptr);
            pos.z = strtof(command[i+3], nullptr);

            this->SetTargetAimPos(pos);
            i += 3;
        }
        else if (strcmp(command[i], "-posent") == 0) {
            if (strcmp(other_target, command[i+1]) == 0) {
                this->SetTargetPosEntity(this->GetTargetAimPosEntity());
            }
            else {
                CBaseEntity *target = SelectTargetByName(actor, command[i+1]);
                other_target = command[i+1];
                this->SetTargetPosEntity(target);
            }
            i++;
        }
        else if (strcmp(command[i], "-lookposent") == 0) {
            if (strcmp(other_target, command[i+1]) == 0) {
                this->SetTargetAimPosEntity(this->GetTargetPosEntity());
            }
            else {
                CBaseEntity *target = SelectTargetByName(actor, command[i+1]);
                other_target = command[i+1];
                this->SetTargetAimPosEntity(target);
            }
            i++;
        }
        else if (strcmp(command[i], "-duration") == 0) {
            this->SetDuration(strtof(command[i+1], nullptr));
            i++;
        }
        else if (strcmp(command[i], "-waituntildone") == 0) {
            this->SetWaitUntilDone(true);
        }
        else if (strcmp(command[i], "-killlook") == 0) {
            this->SetKillLook(true);
        }
        else if (strcmp(command[i], "-alwayslook") == 0) {
            this->SetAlwaysLook(true);
        }
        else if (strcmp(command[i], "-ondoneattributes") == 0) {
            this->SetOnDoneAttributes(command[i+1]);
            i++;
        }
        else if (strcmp(command[i], "-distance") == 0) {
            this->SetMaxDistance(strtof(command[i+1], nullptr));
            i++;
        }
    }	
}
template class CTFBotMoveTo<CTFBot>;
template class CTFBotMoveTo<CBotNPCArcher>;

namespace Mod::Common::Interrupt_Action
{

	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotTacticalMonitor_OnCommandString, CTFBot *actor, const char *cmd)
	{
		if (actor->IsAlive() && V_strnicmp(cmd, "interrupt_action", strlen("interrupt_action")) == 0) {
			
			auto action = reinterpret_cast<Action<CTFBot> *>(this);

			return EventDesiredResult<CTFBot>::SuspendFor(CreateInterruptAction(actor, cmd), "Executing interrupt task");
		}
		
		return DETOUR_MEMBER_CALL(actor, cmd);
	}

    class CMod : public IMod
    {
    public:
        CMod() : IMod("Common:Interrupt_Action")
        {
            MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_OnCommandString, "CTFBotTacticalMonitor::OnCommandString");
        }
        
        virtual bool EnableByDefault() override { return true; }
    };
    CMod s_Mod;

}
