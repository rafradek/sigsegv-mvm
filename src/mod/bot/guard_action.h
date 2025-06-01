#ifndef _INCLUDE_SIGSEGV_MOD_COMMON_GUARD_ACTION_H_
#define _INCLUDE_SIGSEGV_MOD_COMMON_GUARD_ACTION_H_

#include "util/autolist.h"
#include "stub/tfbot.h"
#include "stub/strings.h"
#include "util/misc.h"
#include "mod/item/item_common.h"
#include "mod/pop/pointtemplate.h"

#include "stub/tfbot_behavior.h"
#include "re/path.h"

struct EventInfo;

struct GuardPathNode
{

    std::string m_sName;
    Vector m_Target = vec3_origin;
    std::string m_TargetEntity;
    Vector m_TargetLook = vec3_origin;
    std::string m_TargetLookEntity;
    float m_flWait = -1;
    bool m_bHasTargetLook = false;
    std::vector<int> m_NextNodes;
    std::vector<std::string> m_NextNodesStr;
    std::vector<InputInfoTemplate> m_onReachedOutput;
};

class GuardPath
{
public:

    std::vector<GuardPathNode> m_nodes;
    float m_flMaxPursueRange = 400;
    float m_flMaxPursueRangeSq = m_flMaxPursueRange * m_flMaxPursueRange;
    float m_flMinDistanceToNode = 100;
    float m_flMinDistanceToNodeSq = 100 * 100;
    std::string m_TrackEntity;
    std::vector<InputInfoTemplate> m_onTrackEndOutput;
    bool m_bFireOnPassOutput = true;
    bool m_bTeleportToTrack = false;
};

class GuardPathModule : public EntityModule
{
public:
    GuardPathModule(CBaseEntity *entity) : EntityModule(entity) {}

    std::shared_ptr<GuardPath> m_Path;
};

template<class Actor>
class CTFBotGuardAction : public IHotplugAction<Actor>
{
public:
    CTFBotGuardAction();
    virtual ~CTFBotGuardAction()
    {
        if (this->m_Attack != nullptr) {
            delete this->m_Attack;
        }
    }

    virtual const char *GetName() const override { return "Guard"; }
    virtual ActionResult<Actor> OnStart(Actor *actor, Action<Actor> *action) override;
    virtual void OnEnd(Actor *actor, Action<Actor> *action) override;
    virtual ActionResult<Actor> Update(Actor *actor, float dt) override;
    void PathNodeSet(Actor *actor);

private:
    Action<Actor> *m_Attack = nullptr;
    
    EHANDLE m_hTarget;
    EHANDLE m_hTargetLook;
    CHandle<CPathTrack> m_hTrackTarget;

    std::shared_ptr<GuardPath> m_Path;
    size_t m_nCurrentNodeIndex = 0;
    float m_flGoalReachedTime = 0;
    CountdownTimer m_ctWaitAfterDone;
    bool m_bNodeCompleted = false;
    bool m_bTrackCompleted = false;

    float m_fFinalNavAreaReachTime = 0.0f;

    Vector m_PursueStartPos;
    CountdownTimer m_ctPursueCooldown;

    PathFollower m_PathFollower;
    ChasePath m_ChasePath;
    CountdownTimer m_ctRecomputePath;
};

std::shared_ptr<GuardPath> Parse_GuardPath(KeyValues *kv);
#endif