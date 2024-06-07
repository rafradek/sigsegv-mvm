#ifdef SE_IS_TF2
#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/tfbot.h"
#include "stub/tfbot_behavior.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include <in_buttons.h>


namespace Mod::Perf::Squad_Escort_Optimize
{
    struct NextBotData
    {
        void *vtable;
        INextBotComponent *m_ComponentList;              // +0x04
        PathFollower *m_CurrentPath;                     // +0x08
        int m_iManagerIndex;                             // +0x0c
        bool m_bScheduledForNextTick;                    // +0x10
        int m_iLastUpdateTick;                           // +0x14
        int m_Dword18;                                   // +0x18 (reset to 0 in INextBot::Reset)
        int m_iDebugTextOffset;                          // +0x1c
        Vector m_vecLastPosition;                        // +0x20
        CountdownTimer m_ctImmobileCheck;                // +0x2c
        IntervalTimer m_itImmobileEpoch;                 // +0x38
        ILocomotion *m_LocoInterface;                    // +0x3c
        IBody *m_BodyInterface;                          // +0x40
        IIntention *m_IntentionInterface;                // +0x44
        IVision *m_VisionInterface;                      // +0x48
        CUtlVector<void *> m_DebugLines; // +0x4c
        int off;
        int inputButtons;
        int prevInputButtons;

    };

    std::map<CTFBot *, CTFBotEscortSquadLeader *> update_mark;
    
    bool updatecall = false;

    THINK_FUNC_DECL(SquadAIUpdate)
    {
        auto bot = reinterpret_cast<CTFBot *>(this);

        auto find = update_mark.find(bot);

        if (!bot->IsAlive()){
            if (find != update_mark.end())
                update_mark.erase(find);
            return;
        }

        if (find == update_mark.end())
            return;

        auto action = find->second;

        bot->ReleaseForwardButton();
        bot->ReleaseBackwardButton();
        bot->ReleaseLeftButton();
        bot->ReleaseRightButton();

        auto nextbotdata = reinterpret_cast<NextBotData *>(bot->MyNextBotPointer());
        int bits = ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK);
        nextbotdata->inputButtons &= bits;
        nextbotdata->prevInputButtons &= bits;

        updatecall = true;
        auto result = action->Update(bot, 0.03f);
        updatecall = false;

        if (result.transition != ActionTransition::CONTINUE) {
            if (result.action != nullptr && result.action != action && result.action != action->m_actionToDoAfterSquadDisbands) {
                delete action->m_actionToDoAfterSquadDisbands;
                action->m_actionToDoAfterSquadDisbands = result.action;
            }
            update_mark.erase(find);
            return;
        }

    }

    DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotEscortSquadLeader_Update, CTFBot *actor, float dt)
	{
        auto nextbot = reinterpret_cast<NextBotData *>(actor->MyNextBotPointer());

        bool nexttick = nextbot->m_bScheduledForNextTick;
        
        auto action = reinterpret_cast<CTFBotEscortSquadLeader *>(this);

        auto result = DETOUR_MEMBER_CALL(CTFBotEscortSquadLeader_Update)(actor, dt);

        if (!updatecall) {
            if (result.transition == ActionTransition::CONTINUE)
            {
                update_mark[actor] = action;
                actor->ThinkSet(&ThinkFunc_SquadAIUpdate::Update, gpGlobals->curtime + 0.03f, "SquadAIUpdate1");
                actor->ThinkSet(&ThinkFunc_SquadAIUpdate::Update, gpGlobals->curtime + 0.06f, "SquadAIUpdate2");
                actor->ThinkSet(&ThinkFunc_SquadAIUpdate::Update, gpGlobals->curtime + 0.09f, "SquadAIUpdate3");
            }
            else {
                update_mark.erase(actor);
            }
            
        }
        nextbot->m_bScheduledForNextTick = nexttick;

        return result;
    }

    void OnDestroy(CTFBotEscortSquadLeader *ai)
    {
        CTFBot *actor = ai->GetActor();
        if (actor != nullptr)
        {
            update_mark.erase(actor);
        }
    }
    
    DETOUR_DECL_MEMBER(void, CTFBotEscortSquadLeader_dtor0)
	{
        auto ai = reinterpret_cast<CTFBotEscortSquadLeader *>(this);
        OnDestroy(ai);
        DETOUR_MEMBER_CALL(CTFBotEscortSquadLeader_dtor0)();
    }

    DETOUR_DECL_MEMBER(void, CTFBotEscortSquadLeader_dtor2)
	{
        auto ai = reinterpret_cast<CTFBotEscortSquadLeader *>(this);
        OnDestroy(ai);
        DETOUR_MEMBER_CALL(CTFBotEscortSquadLeader_dtor2)();
    }

	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Perf:Squad_Escort_Optimize")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotEscortSquadLeader_Update,               "CTFBotEscortSquadLeader::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotEscortSquadLeader_dtor0,               "CTFBotEscortSquadLeader::~CTFBotEscortSquadLeader [D0]");
			MOD_ADD_DETOUR_MEMBER(CTFBotEscortSquadLeader_dtor2,               "CTFBotEscortSquadLeader::~CTFBotEscortSquadLeader [D2]");
        
		}
    
		virtual void OnUnload() override
		{
            update_mark.clear();
		}
		
		virtual void OnDisable() override
		{
            update_mark.clear();
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void LevelInitPreEntity() override
		{
			update_mark.clear();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			update_mark.clear();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_squad_escort_optimize", "0", FCVAR_NOTIFY,
		"Mod: improve squad escort performance by reducing update frequency",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#endif