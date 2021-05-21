#include "mod.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "util/iterate.h"


namespace Mod::Debug::SentryBuster_Mannhattan
{
	std::vector<CFmtStr> messages;
	
	
	RefCount rc_CTFBotTacticalMonitor_OnNavAreaChanged;
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotTacticalMonitor_OnNavAreaChanged, CTFBot *actor, CNavArea *area1, CNavArea *area2)
	{
		messages.clear();
		
		SCOPED_INCREMENT(rc_CTFBotTacticalMonitor_OnNavAreaChanged);
		auto result = DETOUR_MEMBER_CALL(CTFBotTacticalMonitor_OnNavAreaChanged)(actor, area1, area2);
		
		if (result.transition == ActionTransition::SUSPEND_FOR) {
			DevMsg("CTFBotTacticalMonitor::OnNavAreaChanged\n");
			
			for (const char *msg : messages) {
				DevMsg("%s", msg);
			}
		}
		
		return result;
	}
	
	DETOUR_DECL_MEMBER(bool, CBaseTrigger_PassesTriggerFilters, CBaseEntity *pOther)
	{
		auto trigger = reinterpret_cast<CBaseTrigger *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CBaseTrigger_PassesTriggerFilters)(pOther);
		
		if (rc_CTFBotTacticalMonitor_OnNavAreaChanged > 0) {
			
			
			messages.emplace_back(CFmtStr("- [prereq: %s] [passed filter: %s]\n", STRING(trigger->GetEntityName()), (result ? "true" : "false")));
		}
		
		return result;
	}
	
	
	// CFilterMultiple    filter_multi
	// CFilterTFBotHasTag filter_tfbot_has_tag
	
	/*
	_ZN11CBaseFilter16PassesFilterImplEP11CBaseEntityS1_
	_ZN15CFilterMultiple16PassesFilterImplEP11CBaseEntityS1_
	_ZN18CFilterTFBotHasTag16PassesFilterImplEP11CBaseEntityS1_
	*/
	
	
	void DrawPrerequisiteOverlays()
	{
		static CountdownTimer ctDraw;
		if (!ctDraw.IsElapsed()) return;
		ctDraw.Start(1.000f);
		
		ForEachEntityByRTTI<CFuncNavPrerequisite>([](CFuncNavPrerequisite *prereq){
			constexpr float dur = 1.000f;
			
			int task                  = prereq->m_task;
			const char *task_ent_name = STRING((string_t)prereq->m_taskEntityName);
			float task_value          = prereq->m_taskValue;
			bool enabled              = !prereq->m_isDisabled;
			
			CFmtStr task_str;
			switch (task) {
			case CFuncNavPrerequisite::DESTROY_ENTITY: task_str.sprintf("DESTROY_ENTITY"); break;
			case CFuncNavPrerequisite::MOVE_TO:        task_str.sprintf("MOVE_TO");        break;
			case CFuncNavPrerequisite::WAIT:           task_str.sprintf("WAIT");           break;
			
			default:
				task_str.sprintf("(?) %d", task);
				break;
			}
			
			int r = (enabled ? 0x00 : 0xff);
			int g = (enabled ? 0xff : 0x00);
			int b = 0x00;
			int a = 0x20;
			NDebugOverlay::EntityBounds(prereq, r, g, b, a, dur);
			
			const Vector& wsc = prereq->WorldSpaceCenter();
			
			int l = -2;
			NDebugOverlay::EntityTextAtPosition(wsc, l++, CFmtStr("Name:       %s", STRING(prereq->GetEntityName())), dur);
			NDebugOverlay::EntityTextAtPosition(wsc, l++, CFmtStr("Task:       %s", task_str.Get()),                  dur);
			NDebugOverlay::EntityTextAtPosition(wsc, l++, CFmtStr("Task ent:   %s", task_ent_name),                   dur);
			NDebugOverlay::EntityTextAtPosition(wsc, l++, CFmtStr("Task value: %f", task_value),                      dur);
			
			++l;
			NDebugOverlay::EntityTextAtPosition(wsc, l++, CFmtStr("m_bDisabled:  %s", (prereq->m_bDisabled  ? "true" : "false")), dur);
			NDebugOverlay::EntityTextAtPosition(wsc, l++, CFmtStr("m_isDisabled: %s", (prereq->m_isDisabled ? "true" : "false")), dur);
			
			CBaseEntity *task_ent = servertools->FindEntityByName(nullptr, task_ent_name);
			if (task_ent != nullptr && !task_ent->ClassMatches("func_nav_prerequisite")) {
				NDebugOverlay::EntityBounds(task_ent, 0xff, 0xff, 0xff, 0x20, dur);
				
				NDebugOverlay::EntityTextAtPosition(task_ent->WorldSpaceCenter(), 0, task_ent_name, dur);
			}
		});
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:SentryBuster_Mannhattan")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_OnNavAreaChanged, "CTFBotTacticalMonitor::OnNavAreaChanged");
			MOD_ADD_DETOUR_MEMBER(CBaseTrigger_PassesTriggerFilters,      "CBaseTrigger::PassesTriggerFilters");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			DrawPrerequisiteOverlays();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_sentrybuster_mannhattan", "0", FCVAR_NOTIFY,
		"Debug: investigate the specifics of the func_nav_prerequisite gate issues related to busters",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
