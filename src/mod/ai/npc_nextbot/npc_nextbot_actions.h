#ifndef _INCLUDE_SIGSEGV_MOD_AI_NPC_NEXTBOT_ACTIONS_H_
#define _INCLUDE_SIGSEGV_MOD_AI_NPC_NEXTBOT_ACTIONS_H_

#include "mod/ai/npc_nextbot/npc_nextbot.h"

namespace Mod::AI::NPC_Nextbot
{
	class CMyNextbotMainAction : public Action<MyNextbotEntity>
	{
	public:
		CMyNextbotMainAction()
		{
		}
		virtual ~CMyNextbotMainAction()
		{
		}
		virtual Action<MyNextbotEntity> *InitialContainedAction(MyNextbotEntity *actor) override;
		
		virtual void OnEnd(MyNextbotEntity *actor, Action<MyNextbotEntity> *action) override
		{
		}

		virtual const char *GetName() const override { return "NPC main action"; }
		
		virtual ActionResult<MyNextbotEntity> OnStart(MyNextbotEntity *actor, Action<MyNextbotEntity> *action) override;
		
		virtual ActionResult<MyNextbotEntity> Update(MyNextbotEntity *actor, float dt) override;
		
		virtual EventDesiredResult< MyNextbotEntity > OnKilled(MyNextbotEntity *actor, const CTakeDamageInfo &info) override;

		virtual QueryResponse ShouldAttack(const INextBot *bot, const CKnownEntity *them) const override { return QueryResponse::YES; }
		
	private:
        CHandle<CBaseEntity *> target;
	};

	class CMyNextbotScenarioMonitor : public Action<MyNextbotEntity>
	{
	public:
		CMyNextbotScenarioMonitor()
		{
		}
		virtual ~CMyNextbotScenarioMonitor()
		{
		}

		virtual Action<MyNextbotEntity> *InitialContainedAction(MyNextbotEntity *actor) override;
		
		virtual void OnEnd(MyNextbotEntity *actor, Action<MyNextbotEntity> *action) override
		{
		}

		virtual const char *GetName() const override { return "NPC scenario monitor"; }
		
		virtual ActionResult<MyNextbotEntity> Update(MyNextbotEntity *actor, float dt) override;

        virtual EventDesiredResult<MyNextbotEntity> OnCommandString(MyNextbotEntity *actor, const char *cmd) override;
		
	private:
        CHandle<CBaseEntity *> target;
	};

	class CMyNextbotAttack : public Action<MyNextbotEntity>
	{
	public:
		CMyNextbotAttack() : m_chasePath(0)
		{
		}
		virtual ~CMyNextbotAttack()
		{
		}
		
		virtual ActionResult<MyNextbotEntity> OnStart(MyNextbotEntity *actor, Action<MyNextbotEntity> *priorAction) override;
		
		virtual void OnEnd(MyNextbotEntity *actor, Action<MyNextbotEntity> *action) override
		{
		}

		virtual const char *GetName() const override { return "NPC attack"; }
		
		virtual ActionResult<MyNextbotEntity> Update(MyNextbotEntity *actor, float dt) override;
		
	private:
		PathFollower m_path;
		ChasePath m_chasePath;
		CountdownTimer m_repathTimer;
	};
}
#endif