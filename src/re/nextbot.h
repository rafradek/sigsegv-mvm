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
class ChasePath;
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
SIZE_CHECK(INextBotComponent,          0x0018);
SIZE_CHECK(IIntention,                 0x001c);
SIZE_CHECK(IBody,                      0x0018);
SIZE_CHECK(ILocomotion,                0x005c);
SIZE_CHECK(IVision,                    0x00c8);
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


class INextBotReply
{
public:
	enum class FailureReason : int
	{
		REJECTED      = 0, // AimHeadTowards denied the aim
		PREEMPTED     = 1, // a higher priority aim preempted our aim
		UNIMPLEMENTED = 2, // subclass didn't override IBody::AimHeadTowards
	};
	
	virtual void OnSuccess(INextBot *nextbot);
	virtual void OnFail(INextBot *nextbot, FailureReason reason);
};

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
					TypeName<T>(true), (uintptr_t)action, action->GetName(), (uintptr_t)actor, ENTINDEX(actor), actor->IsPlayer() ? ToBasePlayer(actor)->GetPlayerName() : "", (actor->IsAlive() ? "true" : "false"));
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
						TypeName<T>(true), (uintptr_t)action, action->GetName(), (uintptr_t)actor, ENTINDEX(actor), actor->IsPlayer() ? ToBasePlayer(actor)->GetPlayerName() : "", (actor->IsAlive() ? "true" : "false"));
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

#define INER INextBotEventResponder
#define AR   ActionResult<Actor>
#define EDR  EventDesiredResult<Actor>
template<class Actor> extern MemberFuncThunk<const Action<Actor> *, INER *>                                                                          ft_Action_INER_FirstContainedResponder;
template<class Actor> extern MemberFuncThunk<const Action<Actor> *, INER *, INER *>                                                                  ft_Action_INER_NextContainedResponder;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLeaveGround;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLandOnGround;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *, CGameTrace *>                                   ft_Action_INER_OnContact;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             const Path *>                                                  ft_Action_INER_OnMoveToSuccess;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             const Path *, INER::MoveToFailureType>                         ft_Action_INER_OnMoveToFailure;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void>                                                                            ft_Action_INER_OnStuck;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void>                                                                            ft_Action_INER_OnUnStuck;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void>                                                                            ft_Action_INER_OnPostureChanged;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             int>                                                           ft_Action_INER_OnAnimationActivityComplete;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             int>                                                           ft_Action_INER_OnAnimationActivityInterrupted;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             animevent_t *>                                                 ft_Action_INER_OnAnimationEvent;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void>                                                                            ft_Action_INER_OnIgnite;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             const CTakeDamageInfo&>                                        ft_Action_INER_OnInjured;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             const CTakeDamageInfo&>                                        ft_Action_INER_OnKilled;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseCombatCharacter *, const CTakeDamageInfo&>                ft_Action_INER_OnOtherKilled;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnSight;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLostSight;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *, const Vector&, KeyValues *>                     ft_Action_INER_OnSound;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseCombatCharacter *, const char *, AI_Response *>           ft_Action_INER_OnSpokeConcept;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseCombatCharacter *, CBaseCombatWeapon *>                   ft_Action_INER_OnWeaponFired;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CNavArea *, CNavArea *>                                        ft_Action_INER_OnNavAreaChanged;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void>                                                                            ft_Action_INER_OnModelChanged;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *, CBaseCombatCharacter *>                         ft_Action_INER_OnPickUp;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnDrop;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseCombatCharacter *, int>                                   ft_Action_INER_OnActorEmoted;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnCommandAttack;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             const Vector&, float>                                          ft_Action_INER_OnCommandApproach_vec;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnCommandApproach_ent;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *, float>                                          ft_Action_INER_OnCommandRetreat;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             float>                                                         ft_Action_INER_OnCommandPause;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void>                                                                            ft_Action_INER_OnCommandResume;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             const char *>                                                  ft_Action_INER_OnCommandString;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnShoved;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnBlinded;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             int>                                                           ft_Action_INER_OnTerritoryContested;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             int>                                                           ft_Action_INER_OnTerritoryCaptured;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             int>                                                           ft_Action_INER_OnTerritoryLost;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void>                                                                            ft_Action_INER_OnWin;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void>                                                                            ft_Action_INER_OnLose;
template<class Actor> extern MemberFuncThunk<const Action<Actor> *, bool,             const char *>                                                  ft_Action_IsNamed;
template<class Actor> extern MemberFuncThunk<const Action<Actor> *, char *>                                                                          ft_Action_GetFullName;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, AR,               Actor *, Action<Actor> *>                                    ft_Action_OnStart;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, AR,               Actor *, float>                                               ft_Action_Update;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             Actor *, Action<Actor> *>                                    ft_Action_OnEnd;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, AR,               Actor *, Action<Actor> *>                                    ft_Action_OnSuspend;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, AR,               Actor *, Action<Actor> *>                                    ft_Action_OnResume;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, Action<Actor> *, Actor *>                                                      ft_Action_InitialContainedAction;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnLeaveGround;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnLandOnGround;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *, CGameTrace *>                         ft_Action_OnContact;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, const Path *>                                        ft_Action_OnMoveToSuccess;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, const Path *, INER::MoveToFailureType>               ft_Action_OnMoveToFailure;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *>                                                      ft_Action_OnStuck;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *>                                                      ft_Action_OnUnStuck;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *>                                                      ft_Action_OnPostureChanged;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, int>                                                 ft_Action_OnAnimationActivityComplete;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, int>                                                 ft_Action_OnAnimationActivityInterrupted;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, animevent_t *>                                       ft_Action_OnAnimationEvent;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *>                                                      ft_Action_OnIgnite;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, const CTakeDamageInfo&>                              ft_Action_OnInjured;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, const CTakeDamageInfo&>                              ft_Action_OnKilled;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseCombatCharacter *, const CTakeDamageInfo&>      ft_Action_OnOtherKilled;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnSight;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnLostSight;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *, const Vector&, KeyValues *>           ft_Action_OnSound;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseCombatCharacter *, const char *, AI_Response *> ft_Action_OnSpokeConcept;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseCombatCharacter *, CBaseCombatWeapon *>         ft_Action_OnWeaponFired;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CNavArea *, CNavArea *>                              ft_Action_OnNavAreaChanged;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *>                                                      ft_Action_OnModelChanged;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *, CBaseCombatCharacter *>               ft_Action_OnPickUp;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnDrop;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseCombatCharacter *, int>                         ft_Action_OnActorEmoted;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnCommandAttack;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, const Vector&, float>                                ft_Action_OnCommandApproach_vec;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnCommandApproach_ent;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *, float>                                ft_Action_OnCommandRetreat;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, float>                                               ft_Action_OnCommandPause;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *>                                                      ft_Action_OnCommandResume;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, const char *>                                        ft_Action_OnCommandString;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnShoved;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, CBaseEntity *>                                       ft_Action_OnBlinded;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, int>                                                 ft_Action_OnTerritoryContested;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, int>                                                 ft_Action_OnTerritoryCaptured;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *, int>                                                 ft_Action_OnTerritoryLost;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *>                                                      ft_Action_OnWin;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, EDR,              Actor *>                                                      ft_Action_OnLose;
template<class Actor> extern MemberFuncThunk<const Action<Actor> *, bool,             const INextBot *>                                              ft_Action_IsAbleToBlockMovementOf;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, Action<Actor> *, Actor *, Behavior<Actor> *, AR>                              ft_Action_ApplyResult;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, void,             Actor *, Behavior<Actor> *, Action<Actor> *>                ft_Action_InvokeOnEnd;
template<class Actor> extern MemberFuncThunk<      Action<Actor> *, AR,               Actor *, Behavior<Actor> *, Action<Actor> *>                ft_Action_InvokeOnResume;
template<class Actor> extern MemberFuncThunk<const Action<Actor> *, char *,           char[256], const Action<Actor> *>                             ft_Action_BuildDecoratedName;
template<class Actor> extern MemberFuncThunk<const Action<Actor> *, char *>                                                                          ft_Action_DebugString;
template<class Actor> extern MemberFuncThunk<const Action<Actor> *, void>                                                                            ft_Action_PrintStateToConsole;

/* Action<CTFBot> */
template<class Actor> INextBotEventResponder *Action<Actor>::FirstContainedResponder() const                                                                    { return ft_Action_INER_FirstContainedResponder       <Actor>(this);                           }
template<class Actor> INextBotEventResponder *Action<Actor>::NextContainedResponder(INextBotEventResponder *prev) const                                         { return ft_Action_INER_NextContainedResponder        <Actor>(this, prev);                     }
template<class Actor> void Action<Actor>::OnLeaveGround(CBaseEntity *ent)                                                                                       {        ft_Action_INER_OnLeaveGround                 <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnLandOnGround(CBaseEntity *ent)                                                                                      {        ft_Action_INER_OnLandOnGround                <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnContact(CBaseEntity *ent, CGameTrace *trace)                                                                        {        ft_Action_INER_OnContact                     <Actor>(this, ent, trace);               }
template<class Actor> void Action<Actor>::OnMoveToSuccess(const Path *path)                                                                                     {        ft_Action_INER_OnMoveToSuccess               <Actor>(this, path);                     }
template<class Actor> void Action<Actor>::OnMoveToFailure(const Path *path, MoveToFailureType fail)                                                             {        ft_Action_INER_OnMoveToFailure               <Actor>(this, path, fail);               }
template<class Actor> void Action<Actor>::OnStuck()                                                                                                             {        ft_Action_INER_OnStuck                       <Actor>(this);                           }
template<class Actor> void Action<Actor>::OnUnStuck()                                                                                                           {        ft_Action_INER_OnUnStuck                     <Actor>(this);                           }
template<class Actor> void Action<Actor>::OnPostureChanged()                                                                                                    {        ft_Action_INER_OnPostureChanged              <Actor>(this);                           }
template<class Actor> void Action<Actor>::OnAnimationActivityComplete(int i1)                                                                                   {        ft_Action_INER_OnAnimationActivityComplete   <Actor>(this, i1);                       }
template<class Actor> void Action<Actor>::OnAnimationActivityInterrupted(int i1)                                                                                {        ft_Action_INER_OnAnimationActivityInterrupted<Actor>(this, i1);                       }
template<class Actor> void Action<Actor>::OnAnimationEvent(animevent_t *a1)                                                                                     {        ft_Action_INER_OnAnimationEvent              <Actor>(this, a1);                       }
template<class Actor> void Action<Actor>::OnIgnite()                                                                                                            {        ft_Action_INER_OnIgnite                      <Actor>(this);                           }
template<class Actor> void Action<Actor>::OnInjured(const CTakeDamageInfo& info)                                                                                {        ft_Action_INER_OnInjured                     <Actor>(this, info);                     }
template<class Actor> void Action<Actor>::OnKilled(const CTakeDamageInfo& info)                                                                                 {        ft_Action_INER_OnKilled                      <Actor>(this, info);                     }
template<class Actor> void Action<Actor>::OnOtherKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info)                                                 {        ft_Action_INER_OnOtherKilled                 <Actor>(this, who, info);                }
template<class Actor> void Action<Actor>::OnSight(CBaseEntity *ent)                                                                                             {        ft_Action_INER_OnSight                       <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnLostSight(CBaseEntity *ent)                                                                                         {        ft_Action_INER_OnLostSight                   <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnSound(CBaseEntity *ent, const Vector& v1, KeyValues *kv)                                                            {        ft_Action_INER_OnSound                       <Actor>(this, ent, v1, kv);              }
template<class Actor> void Action<Actor>::OnSpokeConcept(CBaseCombatCharacter *who, const char *s1, AI_Response *response)                                      {        ft_Action_INER_OnSpokeConcept                <Actor>(this, who, s1, response);        }
template<class Actor> void Action<Actor>::OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)                                                   {        ft_Action_INER_OnWeaponFired                 <Actor>(this, who, weapon);              }
template<class Actor> void Action<Actor>::OnNavAreaChanged(CNavArea *area1, CNavArea *area2)                                                                    {        ft_Action_INER_OnNavAreaChanged              <Actor>(this, area1, area2);             }
template<class Actor> void Action<Actor>::OnModelChanged()                                                                                                      {        ft_Action_INER_OnModelChanged                <Actor>(this);                           }
template<class Actor> void Action<Actor>::OnPickUp(CBaseEntity *ent, CBaseCombatCharacter *who)                                                                 {        ft_Action_INER_OnPickUp                      <Actor>(this, ent, who);                 }
template<class Actor> void Action<Actor>::OnDrop(CBaseEntity *ent)                                                                                              {        ft_Action_INER_OnDrop                        <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnActorEmoted(CBaseCombatCharacter *who, int emote_concept)                                                           {        ft_Action_INER_OnActorEmoted                 <Actor>(this, who, emote_concept);       }
template<class Actor> void Action<Actor>::OnCommandAttack(CBaseEntity *ent)                                                                                     {        ft_Action_INER_OnCommandAttack               <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnCommandApproach(const Vector& v1, float f1)                                                                         {        ft_Action_INER_OnCommandApproach_vec         <Actor>(this, v1, f1);                   }
template<class Actor> void Action<Actor>::OnCommandApproach(CBaseEntity *ent)                                                                                   {        ft_Action_INER_OnCommandApproach_ent         <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnCommandRetreat(CBaseEntity *ent, float f1)                                                                          {        ft_Action_INER_OnCommandRetreat              <Actor>(this, ent, f1);                  }
template<class Actor> void Action<Actor>::OnCommandPause(float f1)                                                                                              {        ft_Action_INER_OnCommandPause                <Actor>(this, f1);                       }
template<class Actor> void Action<Actor>::OnCommandResume()                                                                                                     {        ft_Action_INER_OnCommandResume               <Actor>(this);                           }
template<class Actor> void Action<Actor>::OnCommandString(const char *cmd)                                                                                      {        ft_Action_INER_OnCommandString               <Actor>(this, cmd);                      }
template<class Actor> void Action<Actor>::OnShoved(CBaseEntity *ent)                                                                                            {        ft_Action_INER_OnShoved                      <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnBlinded(CBaseEntity *ent)                                                                                           {        ft_Action_INER_OnBlinded                     <Actor>(this, ent);                      }
template<class Actor> void Action<Actor>::OnTerritoryContested(int i1)                                                                                          {        ft_Action_INER_OnTerritoryContested          <Actor>(this, i1);                       }
template<class Actor> void Action<Actor>::OnTerritoryCaptured(int i1)                                                                                           {        ft_Action_INER_OnTerritoryCaptured           <Actor>(this, i1);                       }
template<class Actor> void Action<Actor>::OnTerritoryLost(int i1)                                                                                               {        ft_Action_INER_OnTerritoryLost               <Actor>(this, i1);                       }
template<class Actor> void Action<Actor>::OnWin()                                                                                                               {        ft_Action_INER_OnWin                         <Actor>(this);                           }
template<class Actor> void Action<Actor>::OnLose()                                                                                                              {        ft_Action_INER_OnLose                        <Actor>(this);                           }
template<class Actor> bool Action<Actor>::IsNamed(const char *name) const                                                                                       { return ft_Action_IsNamed                            <Actor>(this, name);                     }
template<class Actor> char *Action<Actor>::GetFullName() const                                                                                                  { return ft_Action_GetFullName                        <Actor>(this);                           }
template<class Actor> ActionResult<Actor> Action<Actor>::OnStart(Actor *actor, Action<Actor> *action)                                                        { return ft_Action_OnStart                            <Actor>(this, actor, action);            }
template<class Actor> ActionResult<Actor> Action<Actor>::Update(Actor *actor, float dt)                                                                       { return ft_Action_Update                             <Actor>(this, actor, dt);                }
template<class Actor> void Action<Actor>::OnEnd(Actor *actor, Action<Actor> *action)                                                                          {        ft_Action_OnEnd                              <Actor>(this, actor, action);            }
template<class Actor> ActionResult<Actor> Action<Actor>::OnSuspend(Actor *actor, Action<Actor> *action)                                                      { return ft_Action_OnSuspend                          <Actor>(this, actor, action);            }
template<class Actor> ActionResult<Actor> Action<Actor>::OnResume(Actor *actor, Action<Actor> *action)                                                       { return ft_Action_OnResume                           <Actor>(this, actor, action);            }
template<class Actor> Action<Actor> *Action<Actor>::InitialContainedAction(Actor *actor)                                                                      { return ft_Action_InitialContainedAction             <Actor>(this, actor);                    }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnLeaveGround(Actor *actor, CBaseEntity *ent)                                                  { return ft_Action_OnLeaveGround                      <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnLandOnGround(Actor *actor, CBaseEntity *ent)                                                 { return ft_Action_OnLandOnGround                     <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnContact(Actor *actor, CBaseEntity *ent, CGameTrace *trace)                                   { return ft_Action_OnContact                          <Actor>(this, actor, ent, trace);        }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnMoveToSuccess(Actor *actor, const Path *path)                                                { return ft_Action_OnMoveToSuccess                    <Actor>(this, actor, path);              }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnMoveToFailure(Actor *actor, const Path *path, MoveToFailureType fail)                        { return ft_Action_OnMoveToFailure                    <Actor>(this, actor, path, fail);        }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnStuck(Actor *actor)                                                                          { return ft_Action_OnStuck                            <Actor>(this, actor);                    }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnUnStuck(Actor *actor)                                                                        { return ft_Action_OnUnStuck                          <Actor>(this, actor);                    }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnPostureChanged(Actor *actor)                                                                 { return ft_Action_OnPostureChanged                   <Actor>(this, actor);                    }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnAnimationActivityComplete(Actor *actor, int i1)                                              { return ft_Action_OnAnimationActivityComplete        <Actor>(this, actor, i1);                }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnAnimationActivityInterrupted(Actor *actor, int i1)                                           { return ft_Action_OnAnimationActivityInterrupted     <Actor>(this, actor, i1);                }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnAnimationEvent(Actor *actor, animevent_t *a1)                                                { return ft_Action_OnAnimationEvent                   <Actor>(this, actor, a1);                }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnIgnite(Actor *actor)                                                                         { return ft_Action_OnIgnite                           <Actor>(this, actor);                    }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnInjured(Actor *actor, const CTakeDamageInfo& info)                                           { return ft_Action_OnInjured                          <Actor>(this, actor, info);              }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnKilled(Actor *actor, const CTakeDamageInfo& info)                                            { return ft_Action_OnKilled                           <Actor>(this, actor, info);              }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnOtherKilled(Actor *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info)            { return ft_Action_OnOtherKilled                      <Actor>(this, actor, who, info);         }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnSight(Actor *actor, CBaseEntity *ent)                                                        { return ft_Action_OnSight                            <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnLostSight(Actor *actor, CBaseEntity *ent)                                                    { return ft_Action_OnLostSight                        <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnSound(Actor *actor, CBaseEntity *ent, const Vector& v1, KeyValues *kv)                       { return ft_Action_OnSound                            <Actor>(this, actor, ent, v1, kv);       }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnSpokeConcept(Actor *actor, CBaseCombatCharacter *who, const char *s1, AI_Response *response) { return ft_Action_OnSpokeConcept                     <Actor>(this, actor, who, s1, response); }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnWeaponFired(Actor *actor, CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)              { return ft_Action_OnWeaponFired                      <Actor>(this, actor, who, weapon);       }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnNavAreaChanged(Actor *actor, CNavArea *area1, CNavArea *area2)                               { return ft_Action_OnNavAreaChanged                   <Actor>(this, actor, area1, area2);      }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnModelChanged(Actor *actor)                                                                   { return ft_Action_OnModelChanged                     <Actor>(this, actor);                    }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnPickUp(Actor *actor, CBaseEntity *ent, CBaseCombatCharacter *who)                            { return ft_Action_OnPickUp                           <Actor>(this, actor, ent, who);          }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnDrop(Actor *actor, CBaseEntity *ent)                                                         { return ft_Action_OnDrop                             <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnActorEmoted(Actor *actor, CBaseCombatCharacter *who, int i1)                                 { return ft_Action_OnActorEmoted                      <Actor>(this, actor, who, i1);           }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnCommandAttack(Actor *actor, CBaseEntity *ent)                                                { return ft_Action_OnCommandAttack                    <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnCommandApproach(Actor *actor, const Vector& v1, float f1)                                    { return ft_Action_OnCommandApproach_vec              <Actor>(this, actor, v1, f1);            }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnCommandApproach(Actor *actor, CBaseEntity *ent)                                              { return ft_Action_OnCommandApproach_ent              <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnCommandRetreat(Actor *actor, CBaseEntity *ent, float f1)                                     { return ft_Action_OnCommandRetreat                   <Actor>(this, actor, ent, f1);           }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnCommandPause(Actor *actor, float f1)                                                         { return ft_Action_OnCommandPause                     <Actor>(this, actor, f1);                }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnCommandResume(Actor *actor)                                                                  { return ft_Action_OnCommandResume                    <Actor>(this, actor);                    }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnCommandString(Actor *actor, const char *cmd)                                                 { return ft_Action_OnCommandString                    <Actor>(this, actor, cmd);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnShoved(Actor *actor, CBaseEntity *ent)                                                       { return ft_Action_OnShoved                           <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnBlinded(Actor *actor, CBaseEntity *ent)                                                      { return ft_Action_OnBlinded                          <Actor>(this, actor, ent);               }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnTerritoryContested(Actor *actor, int i1)                                                     { return ft_Action_OnTerritoryContested               <Actor>(this, actor, i1);                }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnTerritoryCaptured(Actor *actor, int i1)                                                      { return ft_Action_OnTerritoryCaptured                <Actor>(this, actor, i1);                }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnTerritoryLost(Actor *actor, int i1)                                                          { return ft_Action_OnTerritoryLost                    <Actor>(this, actor, i1);                }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnWin(Actor *actor)                                                                            { return ft_Action_OnWin                              <Actor>(this, actor);                    }
template<class Actor> EventDesiredResult<Actor> Action<Actor>::OnLose(Actor *actor)                                                                           { return ft_Action_OnLose                             <Actor>(this, actor);                    }
template<class Actor> bool Action<Actor>::IsAbleToBlockMovementOf(const INextBot *nextbot) const                                                                { return ft_Action_IsAbleToBlockMovementOf            <Actor>(this, nextbot);                  }
template<class Actor> Action<Actor> *Action<Actor>::ApplyResult(Actor *actor, Behavior<Actor> *behavior, ActionResult<Actor> result)                        { return ft_Action_ApplyResult                        <Actor>(this, actor, behavior, result);  }
template<class Actor> void Action<Actor>::InvokeOnEnd(Actor *actor, Behavior<Actor> *behavior, Action<Actor> *action)                                        {        ft_Action_InvokeOnEnd                        <Actor>(this, actor, behavior, action);  }
template<class Actor> ActionResult<Actor> Action<Actor>::InvokeOnResume(Actor *actor, Behavior<Actor> *behavior, Action<Actor> *action)                     { return ft_Action_InvokeOnResume                     <Actor>(this, actor, behavior, action);  }
template<class Actor> char *Action<Actor>::BuildDecoratedName(char buf[256], const Action<Actor> *action) const                                                { return ft_Action_BuildDecoratedName                 <Actor>(this, buf, action);              }
template<class Actor> char *Action<Actor>::DebugString() const                                                                                                  { return ft_Action_DebugString                        <Actor>(this);                           }
template<class Actor> void Action<Actor>::PrintStateToConsole() const                                                                                           {        ft_Action_PrintStateToConsole                <Actor>(this);                           }

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


template<typename Functor>
inline void ForAllPotentiallyVisibleAreas(Functor func, CNavArea *curArea)
{
	CNavArea::s_nCurrVisTestCounter++;
	int i;
	auto &potentiallyVisible = curArea->m_potentiallyVisibleAreas.Get();
	for (i=0; i < potentiallyVisible.Count(); ++i)
	{
		CNavArea *area = potentiallyVisible[i].area;
		if (!area)
			continue;

		area->m_nVisTestCounter = CNavArea::s_nCurrVisTestCounter;

		if (potentiallyVisible[i].attributes == 0)
			continue;

		func(area);
	}

	// for each inherited area
	if (!curArea->m_inheritVisibilityFrom->area)
		return;

	CUtlVectorConservative<AreaBindInfo> &inherited = curArea->m_inheritVisibilityFrom->area->m_potentiallyVisibleAreas;

	for (i=0; i<inherited.Count(); ++i)
	{
		auto &inheritedVisible = inherited[i];
		auto area = inheritedVisible.area;
		if (!area)
			continue;

		if (area->m_nVisTestCounter == CNavArea::s_nCurrVisTestCounter)
			continue;

		if (inheritedVisible.attributes == 0)
			continue;

		func(area);
	}
}


template<typename Functor>
inline void ForAllPotentiallyVisibleAreasReverse(Functor func, CNavArea *curArea)
{
	CNavArea::s_nCurrVisTestCounter++;
	int i;
	auto &potentiallyVisible = curArea->m_potentiallyVisibleAreas.Get();
	for ( i=potentiallyVisible.Count()-1; i >= 0 ; --i )
	{
		CNavArea *area = potentiallyVisible[i].area;
		if (!area)
			continue;
		
		area->m_nVisTestCounter = CNavArea::s_nCurrVisTestCounter;

		if (potentiallyVisible[i].attributes == 0)
			continue;

		func(area);
	}

	// for each inherited area
	if (!curArea->m_inheritVisibilityFrom->area)
		return;

	CUtlVectorConservative<AreaBindInfo> &inherited = curArea->m_inheritVisibilityFrom->area->m_potentiallyVisibleAreas;

	for (i=inherited.Count()-1; i >= 0; --i)
	{
		auto &inheritedVisible = inherited[i];
		auto area = inheritedVisible.area;
		if (!area)
			continue;

		if (area->m_nVisTestCounter == CNavArea::s_nCurrVisTestCounter)
			continue;

		if (inheritedVisible.attributes == 0)
			continue;

		func(area);
	}
}
#endif