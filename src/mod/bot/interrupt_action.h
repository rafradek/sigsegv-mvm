#ifndef _INCLUDE_SIGSEGV_MOD_COMMON_INTERRUPT_ACTION_H_
#define _INCLUDE_SIGSEGV_MOD_COMMON_INTERRUPT_ACTION_H_

#include "util/autolist.h"
#include "stub/tfbot.h"
#include "stub/strings.h"
#include "mod/item/item_common.h"

#include "re/path.h"

class CTFBotMoveTo : public IHotplugAction<CTFBot>
{
public:
    CTFBotMoveTo() {}
    virtual ~CTFBotMoveTo() {
        if (this->m_pNext != nullptr) {
            delete this->m_pNext;
        }
    }

    void SetTargetPos(Vector &target_pos)
    {
        this->m_TargetPos = target_pos;
    }

    void SetTargetPosEntity(CBaseEntity *target)
    {
        this->m_hTarget = target;
    }

    CBaseEntity *GetTargetPosEntity()
    {
        return this->m_hTarget;
    }

    void SetTargetAimPos(Vector &target_aim)
    {
        this->m_TargetAimPos = target_aim;
    }

    void SetTargetAimPosEntity(CBaseEntity *target)
    {
        this->m_hTargetAim = target;
    }

    CBaseEntity * GetTargetAimPosEntity()
    {
        return this->m_hTargetAim;
    }

    void SetDuration(float duration)
    {
        this->m_fDuration = duration;
    }

    void SetKillLook(bool kill_look)
    {
        this->m_bKillLook = kill_look;
    }
    
    void SetWaitUntilDone(bool wait_until_done)
    {
        this->m_bWaitUntilDone = wait_until_done;
    }

    void SetOnDoneAttributes(std::string on_done_attributes)
    {
        this->m_strOnDoneAttributes = on_done_attributes;
    }

    void SetName(const std::string &name)
    {
        this->m_strName = name;
    }

    void SetAlwaysLook(bool always_look)
    {
        this->m_bAlwaysLook = always_look;
    }

    void SetMaxDistance(float distance)
    {
        this->m_fDistanceSq = distance * distance;
    }

    virtual const char *GetName() const override { return "Interrupt Action"; }

    virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
    virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
    virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd) override;

private:

    std::string m_strName;

    Vector m_TargetPos = vec3_origin;
    Vector m_TargetAimPos = vec3_origin;
    
    CHandle<CBaseEntity> m_hTarget;
    CHandle<CBaseEntity> m_hTargetAim;

    float m_fDuration = 0.0f;
    bool m_bDone = false;
    bool m_bWaitUntilDone = false;
    float m_fDistanceSq = 0.0f;

    bool m_bKillLook = false;
    bool m_bAlwaysLook = false;
    std::string m_strOnDoneAttributes = "";
    PathFollower m_PathFollower;
    CountdownTimer m_ctRecomputePath;
    CountdownTimer m_ctDoneAction;

    CTFBotMoveTo *m_pNext = nullptr;
};

CTFBotMoveTo *CreateInterruptAction(CTFBot *actor, const char *cmd);
#endif