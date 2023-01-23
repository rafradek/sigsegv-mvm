#include "mod.h"
#include "mod/bot/interrupt_action.h"
#include "util/iterate.h"

ActionResult<CTFBot> CTFBotMoveTo::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
    this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
    
    return ActionResult<CTFBot>::Continue();
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

ActionResult<CTFBot> CTFBotMoveTo::Update(CTFBot *actor, float dt)
{
    if (!actor->IsAlive())
        return ActionResult<CTFBot>::Done();
        
    const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
    if (threat != nullptr) {
        actor->EquipBestWeaponForThreat(threat);
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
    
    bool inRange = actor->GetAbsOrigin().DistToSqr(pos) < m_fDistanceSq && !(look != vec3_origin && !actor->GetVisionInterface()->IsLineOfSightClearToEntity(this->m_hTargetAim, &look));
    if (!inRange) {
        if (this->m_ctRecomputePath.IsElapsed()) {
            this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
            
            if (pos != vec3_origin) {
                CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
                this->m_PathFollower.Compute(nextbot, pos, cost_func, 0.0f, true);
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
            actor->FireCustomOutput<"onactiondone">(actor, actor, variant);
        }
        if (this->m_pNext != nullptr) {
            auto nextAction = this->m_pNext;
            this->m_pNext = nullptr;
            return ActionResult<CTFBot>::ChangeTo(nextAction, "Switch to next interrupt action in queue");
        }
        return ActionResult<CTFBot>::Done( "Successfully moved to area" );
    }

    this->m_PathFollower.Update(nextbot);
    
    if (look != vec3_origin) {
        
        bool look_now = !m_bKillLook || m_bAlwaysLook;
        if (m_bKillLook) {
            look_now |= actor->HasAttribute(CTFBot::ATTR_IGNORE_ENEMIES);
            if ((this->m_hTargetAim != nullptr && actor->GetVisionInterface()->IsLineOfSightClearToEntity(this->m_hTargetAim, &look))) {
                look_now = true;
                if (actor->GetBodyInterface()->IsHeadAimingOnTarget()) {
                    actor->EquipBestWeaponForThreat(nullptr);
			        actor->PressFireButton();
                }
            }
        }

        if (look_now) {
            actor->GetBodyInterface()->AimHeadTowards(look, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.2f, NULL, "Aiming at target we need to destroy to progress");
        }
    }

    return ActionResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotMoveTo::OnCommandString(CTFBot *actor, const char *cmd)
{

    if (V_stricmp(cmd, "stop interrupt action") == 0) {
        return EventDesiredResult<CTFBot>::Done("Stopping interrupt action");
    }
    else if (V_strnicmp(cmd, "interrupt_action_queue", strlen("interrupt_action_queue")) == 0) {
        CTFBotMoveTo *action = this;
        while(action->m_pNext != nullptr) {
            action = action->m_pNext;
        }
        action->m_pNext = CreateInterruptAction(actor, cmd);
        return EventDesiredResult<CTFBot>::Sustain("Add to queue");
    }
    else if (V_stricmp(cmd, "clear_interrupt_action_queue") == 0) {
        delete this->m_pNext;
        return EventDesiredResult<CTFBot>::Sustain("Clear queue");
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
        return EventDesiredResult<CTFBot>::Sustain("Delete from queue");
    }
    
    return EventDesiredResult<CTFBot>::Continue();
}

CBaseEntity *SelectTargetByName(CTFBot *actor, const char *name)
{
    CBaseEntity *target = servertools->FindEntityByName(nullptr, name, actor);
    if (target == nullptr && FStrEq(name,"RandomEnemy")) {
        target = actor->SelectRandomReachableEnemy();
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

CTFBotMoveTo *CreateInterruptAction(CTFBot *actor, const char *cmd) {
    CCommand command = CCommand();
    command.Tokenize(cmd);
    
    const char *other_target = "";

    auto interrupt_action = new CTFBotMoveTo();
    for (int i = 1; i < command.ArgC(); i++) {
        if (strcmp(command[i], "-name") == 0) {
            interrupt_action->SetName(command[i+1]);
            i++;
        }
        else if (strcmp(command[i], "-pos") == 0) {
            Vector pos;
            pos.x = strtof(command[i+1], nullptr);
            pos.y = strtof(command[i+2], nullptr);
            pos.z = strtof(command[i+3], nullptr);

            interrupt_action->SetTargetPos(pos);
            i += 3;
        }
        else if (strcmp(command[i], "-lookpos") == 0) {
            Vector pos;
            pos.x = strtof(command[i+1], nullptr);
            pos.y = strtof(command[i+2], nullptr);
            pos.z = strtof(command[i+3], nullptr);

            interrupt_action->SetTargetAimPos(pos);
            i += 3;
        }
        else if (strcmp(command[i], "-posent") == 0) {
            if (strcmp(other_target, command[i+1]) == 0) {
                interrupt_action->SetTargetPosEntity(interrupt_action->GetTargetAimPosEntity());
            }
            else {
                CBaseEntity *target = SelectTargetByName(actor, command[i+1]);
                other_target = command[i+1];
                interrupt_action->SetTargetPosEntity(target);
            }
            i++;
        }
        else if (strcmp(command[i], "-lookposent") == 0) {
            if (strcmp(other_target, command[i+1]) == 0) {
                interrupt_action->SetTargetAimPosEntity(interrupt_action->GetTargetPosEntity());
            }
            else {
                CBaseEntity *target = SelectTargetByName(actor, command[i+1]);
                other_target = command[i+1];
                interrupt_action->SetTargetAimPosEntity(target);
            }
            i++;
        }
        else if (strcmp(command[i], "-duration") == 0) {
            interrupt_action->SetDuration(strtof(command[i+1], nullptr));
            i++;
        }
        else if (strcmp(command[i], "-waituntildone") == 0) {
            interrupt_action->SetWaitUntilDone(true);
        }
        else if (strcmp(command[i], "-killlook") == 0) {
            interrupt_action->SetKillLook(true);
        }
        else if (strcmp(command[i], "-alwayslook") == 0) {
            interrupt_action->SetAlwaysLook(true);
        }
        else if (strcmp(command[i], "-ondoneattributes") == 0) {
            interrupt_action->SetOnDoneAttributes(command[i+1]);
            i++;
        }
        else if (strcmp(command[i], "-distance") == 0) {
            interrupt_action->SetMaxDistance(strtof(command[i+1], nullptr));
            i++;
        }
    }	
    return interrupt_action;
}

namespace Mod::Common::Interrupt_Action
{

	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotTacticalMonitor_OnCommandString, CTFBot *actor, const char *cmd)
	{
		if (actor->IsAlive() && V_strnicmp(cmd, "interrupt_action", strlen("interrupt_action")) == 0) {
			
			auto action = reinterpret_cast<Action<CTFBot> *>(this);

			return EventDesiredResult<CTFBot>::SuspendFor(CreateInterruptAction(actor, cmd), "Executing interrupt task");
		}
		
		return DETOUR_MEMBER_CALL(CTFBotTacticalMonitor_OnCommandString)(actor, cmd);
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