#ifndef _INCLUDE_SIGSEGV_RE_NEXTBOT_H_
#define _INCLUDE_SIGSEGV_RE_NEXTBOT_H_


#if defined _MSC_VER
#pragma message("NextBot classes have not been checked against the VC++ build!")
#endif


#include "mem/scan.h"
#include "stub/nav.h"
#include "util/misc.h"
#include "util/rtti.h"
#include "util/base_off.h"


class CTFBot;
class INextBot;
class Path;
class PathFollower;
struct animevent_t;
class AI_Response;
class CBaseCombatWeapon;
class INextBotEntityFilter;
class INextBotReply;
class CNavLadder;
class NextBotCombatCharacter;
template<typename T> class Action;

// TODO: uhhh, this redefinition seems... bad...
#define CUtlVectorAutoPurge CUtlVector

#include "../mvm-reversed/server/NextBot/NextBotKnownEntity.h"
#include "../mvm-reversed/server/NextBot/NextBotEventResponderInterface.h"
#include "../mvm-reversed/server/NextBot/NextBotContextualQueryInterface.h"
#include "../mvm-reversed/server/NextBot/NextBotComponentInterface.h"
#include "../mvm-reversed/server/NextBot/NextBotIntentionInterface.h"
#include "../mvm-reversed/server/NextBot/NextBotBodyInterface.h"
#include "../mvm-reversed/server/NextBot/NextBotLocomotionInterface.h"
#include "../mvm-reversed/server/NextBot/NextBotVisionInterface.h"
#include "../mvm-reversed/server/NextBot/NextBotInterface.h"
#include "../mvm-reversed/server/NextBot/NextBotBehavior.h"
// TODO: NextBot/Player/PlayerBody.h
// TODO: NextBot/Player/PlayerLocomotion.h
#include "../mvm-reversed/server/NextBot/NextBotManager.h"


SIZE_CHECK(CKnownEntity,               0x0030); // 0x002d
SIZE_CHECK(INextBotEventResponder,     0x0004);
SIZE_CHECK(IContextualQuery,           0x0004);
SIZE_CHECK(INextBotComponent,          0x0014);
SIZE_CHECK(IIntention,                 0x0018);
SIZE_CHECK(IBody,                      0x0014);
SIZE_CHECK(ILocomotion,                0x0058);
SIZE_CHECK(IVision,                    0x00c4);
SIZE_CHECK(INextBot,                   0x0060);
SIZE_CHECK(Behavior<CTFBot>,           0x0050);
SIZE_CHECK(Action<CTFBot>,             0x0034); // 0x0032
SIZE_CHECK(ActionResult<CTFBot>,       0x000c);
SIZE_CHECK(EventDesiredResult<CTFBot>, 0x0010);
SIZE_CHECK(NextBotManager,             0x0050);


#warning REMOVE THIS CRAP PLEASE
/* NextBotKnownEntity.cpp */
//inline CKnownEntity::~CKnownEntity() {}

/* NextBotContextualQueryInterface.cpp */
//inline IContextualQuery::~IContextualQuery() {}

/* NextBotEventResponder.cpp */
//inline INextBotEventResponder::~INextBotEventResponder() {}

/* NextBotBehavior.cpp */
template<typename T> Action<T>::Action() :
	m_Behavior         (nullptr),
	m_ActionParent     (nullptr),
	m_ActionChild      (nullptr),
	m_ActionWeSuspended(nullptr),
	m_ActionSuspendedUs(nullptr),
	m_Actor            (nullptr),
	m_bStarted         (false),
	m_bSuspended       (false)
{
	memset(&this->m_Result, 0x00, sizeof(this->m_Result));
}
template<typename T> Behavior<T> *Action<T>::GetBehavior() const { return this->m_Behavior; }
template<typename T> T           *Action<T>::GetActor()    const { return this->m_Actor; }


/* IHotplugAction: wrapper for Action<T> which can be rapidly stopped at mod unload time */
class IHotplugActionBase
{
public:
	using UnloadAllFunc_t = void (*)();
	
	static void UnloadAll()
	{
		for (auto it = s_Register.begin(); it != s_Register.end(); it = s_Register.erase(it)) {
			UnloadAllFunc_t func = *it;
			(*func)();
		}
	}
	
protected:
	static void Register(UnloadAllFunc_t func) { (void)s_Register.insert(func); }

private:
	static inline std::set<UnloadAllFunc_t> s_Register;
};


template<typename T>
class IHotplugAction : public Action<T>, public IHotplugActionBase, public AutoList<IHotplugAction<T>>
{
public:
	IHotplugAction()
	{
		IHotplugActionBase::Register(&UnloadAll);
	}
	
	static void UnloadAll()
	{
		// NOTE: [20190204] we've been getting some cases where the autolist-is-empty assertion fails due to actors being nullptr lately
		
		Msg("IHotplugAction<%s>::UnloadAll: found %zu currently existent hotplug actions\n", TypeName<T>(true), AutoList<IHotplugAction<T>>::List().size());
		
		std::set<T *> actors;
		
		for (auto action : AutoList<IHotplugAction<T>>::List()) {
			T *actor = action->GetActor();
			
			if (actor == nullptr && action->m_Behavior != nullptr && action->m_Behavior->m_Actor != nullptr) {
				actor = action->m_Behavior->m_Actor;
				Msg("IHotplugAction<%s>::UnloadAll: WARNING: action [0x%08X \"%s\"] has null actor; but its behavior has non-null actor [0x%08X #%d \"%s\" alive:%s]!\n",
					TypeName<T>(true), (uintptr_t)action, action->GetName(), (uintptr_t)actor, ENTINDEX(actor), actor->GetPlayerName(), (actor->IsAlive() ? "true" : "false"));
			}
			
			if (actor == nullptr) {
				static auto l_BoolStr = [](bool val){ return (val ? "true" : "false"); };
				
				static auto l_Action_GetName = [](const Action<T> *action){
					if (action == nullptr) return "<nullptr>";
					return action->GetName();
				};
				
				static auto l_EDR_Str_Transition = [](const EventDesiredResult<T>& edr){
					static char buf[256]; /* not tremendously safe */
					switch (edr.transition) {
					case ActionTransition::CONTINUE:    return "0:CONTINUE";
					case ActionTransition::CHANGE_TO:   return "1:CHANGE_TO";
					case ActionTransition::SUSPEND_FOR: return "2:SUSPEND_FOR";
					case ActionTransition::DONE:        return "3:DONE";
					case ActionTransition::SUSTAIN:     return "4:SUSTAIN";
					default: V_sprintf_safe(buf, "<0x%08X>", (uint32_t)edr.transition); return const_cast<const char *>(buf);
					}
				};
				static auto l_EDR_Str_Action = [](const EventDesiredResult<T>& edr){
					return l_Action_GetName(edr.action);
				};
				static auto l_EDR_Str_Reason = [](const EventDesiredResult<T>& edr){
					if (edr.reason == nullptr) return "<nullptr>";
					return edr.reason;
				};
				static auto l_EDR_Str_Severity = [](const EventDesiredResult<T>& edr){
					static char buf[256]; /* not tremendously safe */
					switch (edr.severity) {
					case ResultSeverity::ZERO:     return "0:ZERO";
					case ResultSeverity::LOW:      return "1:LOW";
					case ResultSeverity::MEDIUM:   return "2:MEDIUM";
					case ResultSeverity::CRITICAL: return "3:CRITICAL";
					default: V_sprintf_safe(buf, "<0x%08X>", (uint32_t)edr.severity); return const_cast<const char *>(buf);
					}
				};
				
				Msg("IHotplugAction<%s>::UnloadAll: WARNING: action [0x%08X \"%s\"] has null actor!\n", TypeName<T>(true), (uintptr_t)action, action->GetName());
				Msg("  GetFullName(): \"%s\"\n", action->GetFullName());
				Msg("  DebugString(): \"%s\"\n", action->DebugString());
				Msg("  m_bStarted:          %s\n", l_BoolStr(action->m_bStarted));
				Msg("  m_bSuspended:        %s\n", l_BoolStr(action->m_bSuspended));
				Msg("  m_Result:            [transition:%s action:\"%s\" reason:\"%s\" severity:%s]\n", l_EDR_Str_Transition(action->m_Result), l_EDR_Str_Action(action->m_Result), l_EDR_Str_Reason(action->m_Result), l_EDR_Str_Severity(action->m_Result));
				Msg("  m_ActionParent:      [0x%08X \"%s\"]\n", (uintptr_t)action->m_ActionParent,      l_Action_GetName(action->m_ActionParent));
				Msg("  m_ActionChild:       [0x%08X \"%s\"]\n", (uintptr_t)action->m_ActionChild,       l_Action_GetName(action->m_ActionChild));
				Msg("  m_ActionWeSuspended: [0x%08X \"%s\"]\n", (uintptr_t)action->m_ActionWeSuspended, l_Action_GetName(action->m_ActionWeSuspended));
				Msg("  m_ActionSuspendedUs: [0x%08X \"%s\"]\n", (uintptr_t)action->m_ActionSuspendedUs, l_Action_GetName(action->m_ActionSuspendedUs));
				Msg("  m_Behavior:          0x%08X\n", (uintptr_t)action->m_Behavior);
				if (action->m_Behavior != nullptr) {
					Msg("  m_Behavior->m_strName:                  \"%s\"\n", action->m_Behavior->m_strName.Get());
					Msg("  m_Behavior->m_MainAction:               0x%08X \"%s\"\n", (uintptr_t)action->m_Behavior->m_MainAction, l_Action_GetName(action->m_Behavior->m_MainAction));
					Msg("  m_Behavior->m_DestroyedActions.Count(): %d\n", action->m_Behavior->m_DestroyedActions.Count());
					for (int i = 0; i < action->m_Behavior->m_DestroyedActions.Count(); ++i) {
						Action<T> *daction = action->m_Behavior->m_DestroyedActions[i];
						if (daction == nullptr) {
							Msg("   [%2d] nullptr\n", i);
						} else {
							Msg("   [%2d] 0x%08X \"%s\" started:%s suspended:%s behavior:0x%08X\n", i, (uintptr_t)daction, daction->GetName(), l_BoolStr(daction->m_bStarted), l_BoolStr(daction->m_bSuspended), (uintptr_t)daction->m_Behavior);
							Msg("         \"%s\"\n", daction->DebugString());
						}
					}
				}
				
				if (action->m_Behavior != nullptr && action->m_Behavior->m_Actor != nullptr) {
					actor = action->m_Behavior->m_Actor;
					Msg("IHotplugAction<%s>::UnloadAll: WARNING: action [0x%08X \"%s\"] has null actor; but its behavior has non-null actor [0x%08X #%d \"%s\" alive:%s]!\n",
						TypeName<T>(true), (uintptr_t)action, action->GetName(), (uintptr_t)actor, ENTINDEX(actor), actor->GetPlayerName(), (actor->IsAlive() ? "true" : "false"));
				}
			}
			
			if (actor != nullptr) {
				actors.insert(actor);
			}
		}
		
		Msg("IHotplugAction<%s>::UnloadAll: calling Reset() on %zu NextBot intention interfaces\n", TypeName<T>(true), actors.size());
		
		for (auto actor : actors) {
			actor->GetIntentionInterface()->Reset();
		}
		
		assert(AutoList<IHotplugAction<T>>::List().empty());
	}
};


#if defined __GNUC__ && !defined __clang__

/* ActionStub: provides a base for interoperating with real Action<T> objects in the game */
template<typename T>
class ActionStub : public Action<T>
{
public:
	virtual const char *GetName() const override { return nullptr; }
	
protected:
	ActionStub() = default;
	
	/* needed for some cases when desiring to construct an in-game version of the action,
	 * and there isn't a non-inlined constructor available to simply call,
	 * and you want to put the in-game vtable ptrs into place so things will work correctly */
	template<typename U, typename V = U>
	void OverwriteVTPtr()
	{
		ptrdiff_t offset = base_off<U, V>();
		
		uint32_t vt_ptr;
		if (offset == 0x0000) {
			/* main vtable @ +0x0000 */
			vt_ptr = (uint32_t)RTTI::GetVTable<V>();
		} else {
			/* an additional vtable located past +0x0000, e.g. IContextualQuery @ +0x0004 */
			vt_ptr = FindAdditionalVTable<U>(offset);
		}
		
		*(uint32_t *)((uintptr_t)this + offset) = vt_ptr;
	}
	
	/* overwrite multiple vtable ptrs in one concise call! */
	template<typename U, typename V, typename... MORE>
	void OverwriteVTPtrs()
	{
		OverwriteVTPtr<U, V>();
		
		/* recurse: call self again, with V now set to the first member of `MORE...` */
		if constexpr (sizeof...(MORE) > 0) {
			OverwriteVTPtrs<U, MORE...>();
		}
	}
	
private:
	struct VTPreamble
	{
		ptrdiff_t base_neg_off;
		const rtti_t *derived_tinfo;
	};
	static_assert(sizeof(VTPreamble) == 0x8);
	
	template<typename U>
	static uint32_t FindAdditionalVTable(ptrdiff_t offset)
	{
		VTPreamble preamble{ -offset, RTTI::GetRTTI<U>() };
		
	//	DevMsg("### %s\n", __PRETTY_FUNCTION__);
	//	DevMsg("    server_srv base: %08X\n", (uint32_t)LibMgr::GetInfo(Library::SERVER).BaseAddr());
	//	DevMsg("    offset = %d\n", offset);
	//	DevMsg("    RTTI::GetRTTI<U>   = %08X\n", (uint32_t)RTTI::GetRTTI<U>());
	//	DevMsg("    RTTI::GetVTable<U> = %08X\n", (uint32_t)RTTI::GetVTable<U>());
	//	DevMsg("    preamble: %08X %08X\n", *reinterpret_cast<uint32_t *>(&preamble.base_neg_off), *reinterpret_cast<uint32_t *>(&preamble.derived_tinfo));
		
		using VTScanner = CTypeScanner<ScanDir::FORWARD, ScanResults::ALL, VTPreamble, 4>;
		CScan<VTScanner> scan(CAddrOffBounds(RTTI::GetVTable<U>(), 0x2000), preamble);
		
	//	DevMsg("    scan.GetBounds().first  = %08X\n", (uint32_t)scan.GetScanner().GetBounds().first);
	//	DevMsg("    scan.GetBounds().second = %08X\n", (uint32_t)scan.GetScanner().GetBounds().second);
	//	for (const void *match : scan.Matches()) {
	//		DevMsg("    match: %08X\n", (uint32_t)match);
	//	}
	//	HexDumpToSpewFunc(&DevMsg, RTTI::GetVTable<U>(), 0x2000, true);
		
		assert(scan.ExactlyOneMatch());
		
		return ((uint32_t)scan.FirstMatch() + sizeof(VTPreamble));
	}
};

#endif


#endif
