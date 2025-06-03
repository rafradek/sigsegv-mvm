#include "mod.h"
#include "stub/extraentitydata.h"
#include "stub/tfbot_behavior.h"
#include "stub/nextbot_cc.h"
#include "mod/ai/npc_nextbot/npc_nextbot.h"
#include "mod/ai/npc_nextbot/npc_nextbot_actions.h"
#include "mod/bot/guard_action.h"
#include "mod/bot/interrupt_action.h"
#include "util/iterate.h"
#include "util/entity.h"
#include "util/clientmsg.h"
#include "stub/populators.h"

template <>
CTFBotGuardAction<CTFBot>::CTFBotGuardAction(): m_ChasePath(0) {

    this->m_Attack = CTFBotAttack::New();
}

template <>
CTFBotGuardAction<CBotNPCArcher>::CTFBotGuardAction(): m_ChasePath(0) {

    this->m_Attack = new Mod::AI::NPC_Nextbot::CMyNextbotAttack();
}

template <class Actor>
ActionResult<Actor> CTFBotGuardAction<Actor>::OnStart(Actor *actor, Action<Actor> *action)
{
    CTFBot *tfbot = ToTFBot(actor);
    this->m_PathFollower.SetMinLookAheadDistance(tfbot != nullptr ? tfbot->GetDesiredPathLookAheadRange() : 300);

    this->m_hTarget = nullptr;

    
    this->m_Attack->OnStart(actor, action);
    if (tfbot != nullptr) {
        tfbot->SetCustomVariable<"ispassiveaction">(Variant(true));
    }
    return ActionResult<Actor>::Continue();
}

template <class Actor>
void CTFBotGuardAction<Actor>::OnEnd(Actor *actor, Action<Actor> *action)
{
    CTFBot *tfbot = ToTFBot(actor);
    if (tfbot != nullptr) {
        tfbot->SetCustomVariable<"ispassiveaction">(Variant(false));
    }
}

template <class Actor>
void CTFBotGuardAction<Actor>::PathNodeSet(Actor *actor) {
    GuardPathNode &node = this->m_Path->m_nodes[this->m_nCurrentNodeIndex];
    if (this->m_bTrackCompleted) return;

    if (!this->m_Path->m_TrackEntity.empty()) {
        if (m_hTrackTarget == nullptr) {
            m_hTrackTarget = rtti_cast<CPathTrack *>(servertools->FindEntityByName(nullptr, this->m_Path->m_TrackEntity.c_str()));
        }
        m_hTarget = m_hTrackTarget;
    }
    else if (!node.m_TargetEntity.empty() && m_hTarget == nullptr) {
        m_hTarget = SelectTargetByName(actor, node.m_TargetEntity.c_str());
    }
    if (!node.m_TargetLookEntity.empty() && m_hTargetLook == nullptr) {
        m_hTargetLook = SelectTargetByName(actor, node.m_TargetLookEntity.c_str());
    }
    this->m_ctRecomputePath.Invalidate();
    m_bNodeCompleted = false;
}

template <class Actor>
ActionResult<Actor> CTFBotGuardAction<Actor>::Update(Actor *actor, float dt)
{
    actor->GetVisionInterface()->Update();
    const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
    bool hasThreat = false;
    bool pursuing = false;
    
    CTFBot *tfbot = ToTFBot(actor);
    auto nextbot = actor->MyNextBotPointer();
    if (threat != nullptr && !threat->IsObsolete() && !(tfbot != nullptr && tfbot->HasAttribute(CTFBot::ATTR_IGNORE_ENEMIES)) && actor->GetIntentionInterface()->ShouldAttack(nextbot, threat) == QueryResponse::YES) {
        hasThreat = true;
        if (tfbot != nullptr) {
            tfbot->EquipBestWeaponForThreat(threat);
        }
        if (this->m_PursueStartPos != vec3_origin && actor->GetAbsOrigin().DistToSqr(this->m_PursueStartPos) > this->m_Path->m_flMaxPursueRangeSq) {
            this->m_PursueStartPos = vec3_origin;
            this->m_ctPursueCooldown.Start(12.0f);
        }
        if (this->m_Path->m_flMaxPursueRange != 0 && this->m_ctPursueCooldown.IsElapsed()) {
            if (this->m_PursueStartPos == vec3_origin) {
                this->m_PursueStartPos = actor->GetAbsOrigin();
            }
            this->m_Attack->Update(actor, dt);
            pursuing = true;
        }
    }
    CBaseEntity *target = m_hTarget;
    CBaseEntity *ent = actor;
    auto newPath = ent->GetOrCreateEntityModule<GuardPathModule>("guardpath")->m_Path;
    if (this->m_Path != newPath) {
        this->m_Path = newPath;
        this->m_nCurrentNodeIndex = 0;
        this->m_bTrackCompleted = false;
        PathNodeSet(actor);
        if (this->m_Path->m_bTeleportToTrack) {
            if (target != nullptr) {
                actor->Teleport(&target->GetAbsOrigin(), &target->GetAbsAngles(), nullptr);
            }
            else {
                actor->Teleport(&this->m_Path->m_nodes[0].m_Target, &vec3_angle, nullptr);
            }
        }
    }

    if (this->m_Path == nullptr) {
        return ActionResult<Actor>::Continue();
    }
    GuardPathNode &node = this->m_Path->m_nodes[this->m_nCurrentNodeIndex];
    if (!pursuing) {
        this->m_PursueStartPos = vec3_origin;
        Vector pos = vec3_origin;
        if (target != nullptr) {
            pos = target->WorldSpaceCenter();
        }
        else {
            pos = node.m_Target;
        }
        trace_t tr;
        UTIL_TraceLine(pos + Vector(0, 0, 10), pos + Vector(0, 0, -100), MASK_SOLID_BRUSHONLY, target, COLLISION_GROUP_NONE, &tr);
        Vector posGround = tr.endpos + Vector(0,0,10);
        bool inRange = actor->GetAbsOrigin().DistToSqr(posGround) < this->m_Path->m_flMinDistanceToNodeSq;

        if (!inRange) {
            if (target != nullptr) {
                if (tfbot != nullptr) {
                    CTFBotPathCost cost_func(tfbot, DEFAULT_ROUTE);
                    this->m_ChasePath.Update(nextbot, target, cost_func, nullptr);
                }
                else {
                    CZombiePathCost cost_func((CZombie *) actor);
                    this->m_ChasePath.Update(nextbot, target, cost_func, nullptr);
                }
                this->m_ChasePath.ShortenFailTimer(0.5f);
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
    
        if (target == nullptr) {
            this->m_ChasePath.Invalidate();
            this->m_PathFollower.Update(nextbot);

            if (!inRange) {
                bool onFinalNav = TheNavMesh->GetNavArea(pos) == actor->GetLastKnownArea();
                if (onFinalNav && this->m_fFinalNavAreaReachTime == 0) {
                    this->m_fFinalNavAreaReachTime = gpGlobals->curtime;
                }
                else if (!onFinalNav) {
                    this->m_fFinalNavAreaReachTime = 0;
                }
                if (onFinalNav && this->m_fFinalNavAreaReachTime + 5 < gpGlobals->curtime) {
                    inRange = true;
                }
            }
        }

        if (m_hTargetLook != nullptr) {
            actor->GetBodyInterface()->AimHeadTowards(m_hTargetLook, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.2f, NULL, "Aiming at target entity");
        }
        else if (node.m_TargetLook != vec3_origin) {
            actor->GetBodyInterface()->AimHeadTowards(node.m_TargetLook, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.2f, NULL, "Aiming at position");
        }

        if (pos == vec3_origin || inRange) {

            if (!m_bNodeCompleted) {
                this->m_ctWaitAfterDone.Start(node.m_flWait);
                if (this->m_hTrackTarget != nullptr){
                    m_hTrackTarget->m_OnPass->FireOutput(Variant(), actor, m_hTrackTarget);
                }
                TriggerList(actor, node.m_onReachedOutput);
            }
            m_bNodeCompleted = true;
        }
    }

    if (m_bNodeCompleted && this->m_ctWaitAfterDone.IsElapsed()) {
        if (this->m_hTrackTarget != nullptr) {
            this->m_hTrackTarget = this->m_hTrackTarget->GetNext();
            if (this->m_hTrackTarget == nullptr) {
                TriggerList(actor, this->m_Path->m_onTrackEndOutput);
                m_bTrackCompleted = true;
            }
        }
        else {
            this->m_nCurrentNodeIndex = node.m_NextNodes[RandomInt(0, node.m_NextNodes.size() - 1)];
        }
        PathNodeSet(actor);
    }

    return ActionResult<Actor>::Continue();
}

GuardPathNode Parse_GuardPathNode(KeyValues *kv) {
    GuardPathNode node;
    if (kv->GetFirstSubKey() == nullptr) {
        if (!UTIL_StringToVectorAlt(node.m_Target, kv->GetString())) {
            node.m_Target = vec3_origin;
            node.m_TargetEntity = kv->GetString();
        }
    }
    else {
        FOR_EACH_SUBKEY(kv, subkey) {
            auto name = subkey->GetName();
            if (FStrEq(name, "Target")) {
                if (!UTIL_StringToVectorAlt(node.m_Target, subkey->GetString())) {
                    node.m_Target = vec3_origin;
                    node.m_TargetEntity = subkey->GetString();
                }
            }
            else if (FStrEq(name, "AimTarget")) {
                if (!UTIL_StringToVectorAlt(node.m_TargetLook, subkey->GetString())) {
                    node.m_TargetLook = vec3_origin;
                    node.m_TargetLookEntity = subkey->GetString();
                }
                node.m_bHasTargetLook = true;
            }
            else if (FStrEq(name, "Wait")) {
                node.m_flWait = subkey->GetFloat();
            }
            else if (FStrEq(name, "Output")) {
                node.m_onReachedOutput.push_back(Parse_InputInfoTemplate(subkey));
            }
            else if (FStrEq(name, "Next")) {
                node.m_NextNodesStr.push_back(subkey->GetString());
            }
            else if (FStrEq(name, "Name")) {
                node.m_sName = subkey->GetString();
            }
        }
    }
    return node;
}

std::shared_ptr<GuardPath> Parse_GuardPath(KeyValues *kv) {
    auto path = std::make_shared<GuardPath>();
    std::unordered_map<std::string, size_t> nodeNameToId;
    FOR_EACH_SUBKEY(kv, subkey) {
        auto name = subkey->GetName();
        if (FStrEq(name, "Node")) {
            path->m_nodes.push_back(Parse_GuardPathNode(subkey));
            nodeNameToId[path->m_nodes.back().m_sName] = path->m_nodes.size()-1;
        }
        else if (FStrEq(name, "PursueRange")) {
            path->m_flMaxPursueRange = subkey->GetFloat();
            path->m_flMaxPursueRangeSq = path->m_flMaxPursueRange * path->m_flMaxPursueRange;
        }
        else if (FStrEq(name, "NodeReachRange")) {
            path->m_flMinDistanceToNode = subkey->GetFloat();
            path->m_flMinDistanceToNodeSq = path->m_flMinDistanceToNode * path->m_flMinDistanceToNode;
        }
        else if (FStrEq(name, "Track")) {
            path->m_TrackEntity = subkey->GetString();
        }
        else if (FStrEq(name, "TrackFinishOutput")) {
            path->m_onTrackEndOutput.push_back(Parse_InputInfoTemplate(subkey));
        }
        else if (FStrEq(name, "TrackFireOnPassOutput")) {
            path->m_bFireOnPassOutput = subkey->GetBool();
        }
        else if (FStrEq(name, "TeleportToTrack")) {
            path->m_bTeleportToTrack = subkey->GetBool();
        }
    }
    if (!path->m_TrackEntity.empty()) {
        path->m_nodes.clear();
        path->m_nodes.emplace_back();
    }
    else if (path->m_nodes.empty()) {
        path->m_nodes.emplace_back();
    }
    
    for(size_t id = 0; id < path->m_nodes.size(); id++) {
        auto &node = path->m_nodes[id];
        if (node.m_NextNodesStr.empty()) {
            node.m_NextNodes.push_back((id+1) % path->m_nodes.size());
        }
        else {
            for (auto &nextName : node.m_NextNodesStr) {
                node.m_NextNodes.push_back(nodeNameToId[nextName]);
            }
        }
    }
    return path;
}

template class CTFBotGuardAction<CTFBot>;
template class CTFBotGuardAction<CBotNPCArcher>;
