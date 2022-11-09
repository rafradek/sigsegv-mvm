#include "link/link.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"


#define INER INextBotEventResponder
#define AR   ActionResult<CTFBot>
#define EDR  EventDesiredResult<CTFBot>

/* INextBot */
static MemberFuncThunk<const INextBot *, bool, CBaseEntity *, float> ft_INextBot_IsRangeLessThan(                 "INextBot::IsRangeLessThan");
static MemberFuncThunk<const INextBot *, bool, CBaseEntity *, float> ft_INextBot_IsRangeGreaterThan(              "INextBot::IsRangeGreaterThan");

static MemberVFuncThunk<const INextBot *, CBaseCombatCharacter *> vt_INextBot_GetEntity(TypeName<NextBotPlayer<CTFPlayer>>(), "NextBotPlayer<CTFPlayer>::GetEntity");

/* INextBotComponent */
static MemberFuncThunk<const INextBotComponent *, INextBot *> vt_INextBotComponent_GetBot( "INextBotComponent::GetBot");

/* CKnownEntity */
static MemberFuncThunk<      CKnownEntity *, void>                      ft_CKnownEntity_Destroy(                     "CKnownEntity::Destroy");
static MemberFuncThunk<      CKnownEntity *, void>                      ft_CKnownEntity_UpdatePosition(              "CKnownEntity::UpdatePosition");
static MemberFuncThunk<const CKnownEntity *, CBaseEntity *>             ft_CKnownEntity_GetEntity(                   "CKnownEntity::GetEntity");
static MemberFuncThunk<const CKnownEntity *, const Vector *>            ft_CKnownEntity_GetLastKnownPosition(        "CKnownEntity::GetLastKnownPosition");
static MemberFuncThunk<const CKnownEntity *, bool>                      ft_CKnownEntity_HasLastKnownPositionBeenSeen("CKnownEntity::HasLastKnownPositionBeenSeen");
static MemberFuncThunk<      CKnownEntity *, void>                      ft_CKnownEntity_MarkLastKnownPositionAsSeen( "CKnownEntity::MarkLastKnownPositionAsSeen");
static MemberFuncThunk<const CKnownEntity *, CNavArea *>                ft_CKnownEntity_GetLastKnownArea(            "CKnownEntity::GetLastKnownArea");
static MemberFuncThunk<const CKnownEntity *, float>                     ft_CKnownEntity_GetTimeSinceLastKnown(       "CKnownEntity::GetTimeSinceLastKnown");
static MemberFuncThunk<const CKnownEntity *, float>                     ft_CKnownEntity_GetTimeSinceBecameKnown(     "CKnownEntity::GetTimeSinceBecameKnown");
static MemberFuncThunk<      CKnownEntity *, void, bool>                ft_CKnownEntity_UpdateVisibilityStatus(      "CKnownEntity::UpdateVisibilityStatus");
static MemberFuncThunk<const CKnownEntity *, bool>                      ft_CKnownEntity_IsVisibleInFOVNow(           "CKnownEntity::IsVisibleInFOVNow");
static MemberFuncThunk<const CKnownEntity *, bool>                      ft_CKnownEntity_IsVisibleRecently(           "CKnownEntity::IsVisibleRecently");
static MemberFuncThunk<const CKnownEntity *, float>                     ft_CKnownEntity_GetTimeSinceBecameVisible(   "CKnownEntity::GetTimeSinceBecameVisible");
static MemberFuncThunk<const CKnownEntity *, float>                     ft_CKnownEntity_GetTimeWhenBecameVisible(    "CKnownEntity::GetTimeWhenBecameVisible");
static MemberFuncThunk<const CKnownEntity *, float>                     ft_CKnownEntity_GetTimeSinceLastSeen(        "CKnownEntity::GetTimeSinceLastSeen");
static MemberFuncThunk<const CKnownEntity *, bool>                      ft_CKnownEntity_WasEverVisible(              "CKnownEntity::WasEverVisible");
static MemberFuncThunk<const CKnownEntity *, bool>                      ft_CKnownEntity_IsObsolete(                  "CKnownEntity::IsObsolete");
static MemberFuncThunk<const CKnownEntity *, bool, const CKnownEntity&> ft_CKnownEntity_op_equal(                    "CKnownEntity::operator==");
static MemberFuncThunk<const CKnownEntity *, bool, CBaseEntity *>       ft_CKnownEntity_Is(                          "CKnownEntity::Is");

/* INextBotEventResponder */
static MemberFuncThunk<const INER *, INER *>                                                    ft_INextBotEventResponder_FirstContainedResponder(       "INextBotEventResponder::FirstContainedResponder");
static MemberFuncThunk<const INER *, INER *, INER *>                                            ft_INextBotEventResponder_NextContainedResponder(        "INextBotEventResponder::NextContainedResponder");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnLeaveGround(                 "INextBotEventResponder::OnLeaveGround");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnLandOnGround(                "INextBotEventResponder::OnLandOnGround");
static MemberFuncThunk<      INER *, void, CBaseEntity *, CGameTrace *>                         ft_INextBotEventResponder_OnContact(                     "INextBotEventResponder::OnContact");
static MemberFuncThunk<      INER *, void, const Path *>                                        ft_INextBotEventResponder_OnMoveToSuccess(               "INextBotEventResponder::OnMoveToSuccess");
static MemberFuncThunk<      INER *, void, const Path *, INER::MoveToFailureType>               ft_INextBotEventResponder_OnMoveToFailure(               "INextBotEventResponder::OnMoveToFailure");
static MemberFuncThunk<      INER *, void>                                                      ft_INextBotEventResponder_OnStuck(                       "INextBotEventResponder::OnStuck");
static MemberFuncThunk<      INER *, void>                                                      ft_INextBotEventResponder_OnUnStuck(                     "INextBotEventResponder::OnUnStuck");
static MemberFuncThunk<      INER *, void>                                                      ft_INextBotEventResponder_OnPostureChanged(              "INextBotEventResponder::OnPostureChanged");
static MemberFuncThunk<      INER *, void, int>                                                 ft_INextBotEventResponder_OnAnimationActivityComplete(   "INextBotEventResponder::OnAnimationActivityComplete");
static MemberFuncThunk<      INER *, void, int>                                                 ft_INextBotEventResponder_OnAnimationActivityInterrupted("INextBotEventResponder::OnAnimationActivityInterrupted");
static MemberFuncThunk<      INER *, void, animevent_t *>                                       ft_INextBotEventResponder_OnAnimationEvent(              "INextBotEventResponder::OnAnimationEvent");
static MemberFuncThunk<      INER *, void>                                                      ft_INextBotEventResponder_OnIgnite(                      "INextBotEventResponder::OnIgnite");
static MemberFuncThunk<      INER *, void, const CTakeDamageInfo&>                              ft_INextBotEventResponder_OnInjured(                     "INextBotEventResponder::OnInjured");
static MemberFuncThunk<      INER *, void, const CTakeDamageInfo&>                              ft_INextBotEventResponder_OnKilled(                      "INextBotEventResponder::OnKilled");
static MemberFuncThunk<      INER *, void, CBaseCombatCharacter *, const CTakeDamageInfo&>      ft_INextBotEventResponder_OnOtherKilled(                 "INextBotEventResponder::OnOtherKilled");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnSight(                       "INextBotEventResponder::OnSight");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnLostSight(                   "INextBotEventResponder::OnLostSight");
static MemberFuncThunk<      INER *, void, CBaseEntity *, const Vector&, KeyValues *>           ft_INextBotEventResponder_OnSound(                       "INextBotEventResponder::OnSound");
static MemberFuncThunk<      INER *, void, CBaseCombatCharacter *, const char *, AI_Response *> ft_INextBotEventResponder_OnSpokeConcept(                "INextBotEventResponder::OnSpokeConcept");
static MemberFuncThunk<      INER *, void, CBaseCombatCharacter *, CBaseCombatWeapon *>         ft_INextBotEventResponder_OnWeaponFired(                 "INextBotEventResponder::OnWeaponFired");
static MemberFuncThunk<      INER *, void, CNavArea *, CNavArea *>                              ft_INextBotEventResponder_OnNavAreaChanged(              "INextBotEventResponder::OnNavAreaChanged");
static MemberFuncThunk<      INER *, void>                                                      ft_INextBotEventResponder_OnModelChanged(                "INextBotEventResponder::OnModelChanged");
static MemberFuncThunk<      INER *, void, CBaseEntity *, CBaseCombatCharacter *>               ft_INextBotEventResponder_OnPickUp(                      "INextBotEventResponder::OnPickUp");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnDrop(                        "INextBotEventResponder::OnDrop");
static MemberFuncThunk<      INER *, void, CBaseCombatCharacter *, int>                         ft_INextBotEventResponder_OnActorEmoted(                 "INextBotEventResponder::OnActorEmoted");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnCommandAttack(               "INextBotEventResponder::OnCommandAttack");
static MemberFuncThunk<      INER *, void, const Vector&, float>                                ft_INextBotEventResponder_OnCommandApproach_vec(         "INextBotEventResponder::OnCommandApproach(vec)");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnCommandApproach_ent(         "INextBotEventResponder::OnCommandApproach(ent)");
static MemberFuncThunk<      INER *, void, CBaseEntity *, float>                                ft_INextBotEventResponder_OnCommandRetreat(              "INextBotEventResponder::OnCommandRetreat");
static MemberFuncThunk<      INER *, void, float>                                               ft_INextBotEventResponder_OnCommandPause(                "INextBotEventResponder::OnCommandPause");
static MemberFuncThunk<      INER *, void>                                                      ft_INextBotEventResponder_OnCommandResume(               "INextBotEventResponder::OnCommandResume");
static MemberFuncThunk<      INER *, void, const char *>                                        ft_INextBotEventResponder_OnCommandString(               "INextBotEventResponder::OnCommandString");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnShoved(                      "INextBotEventResponder::OnShoved");
static MemberFuncThunk<      INER *, void, CBaseEntity *>                                       ft_INextBotEventResponder_OnBlinded(                     "INextBotEventResponder::OnBlinded");
static MemberFuncThunk<      INER *, void, int>                                                 ft_INextBotEventResponder_OnTerritoryContested(          "INextBotEventResponder::OnTerritoryContested");
static MemberFuncThunk<      INER *, void, int>                                                 ft_INextBotEventResponder_OnTerritoryCaptured(           "INextBotEventResponder::OnTerritoryCaptured");
static MemberFuncThunk<      INER *, void, int>                                                 ft_INextBotEventResponder_OnTerritoryLost(               "INextBotEventResponder::OnTerritoryLost");
static MemberFuncThunk<      INER *, void>                                                      ft_INextBotEventResponder_OnWin(                         "INextBotEventResponder::OnWin");
static MemberFuncThunk<      INER *, void>                                                      ft_INextBotEventResponder_OnLose(                        "INextBotEventResponder::OnLose");

/* IContextualQuery */
static MemberFuncThunk<const IContextualQuery *, QueryResponse,        const INextBot *, CBaseEntity *>                                                            ft_IContextualQuery_ShouldPickUp(             "IContextualQuery::ShouldPickUp");
static MemberFuncThunk<const IContextualQuery *, QueryResponse,        const INextBot *>                                                                           ft_IContextualQuery_ShouldHurry(              "IContextualQuery::ShouldHurry");
static MemberFuncThunk<const IContextualQuery *, QueryResponse,        const INextBot *>                                                                           ft_IContextualQuery_ShouldRetreat(            "IContextualQuery::ShouldRetreat");
static MemberFuncThunk<const IContextualQuery *, QueryResponse,        const INextBot *, const CKnownEntity *>                                                     ft_IContextualQuery_ShouldAttack(             "IContextualQuery::ShouldAttack");
static MemberFuncThunk<const IContextualQuery *, QueryResponse,        const INextBot *, CBaseEntity *>                                                            ft_IContextualQuery_IsHindrance(              "IContextualQuery::IsHindrance");
static MemberFuncThunk<const IContextualQuery *, Vector,               const INextBot *, const CBaseCombatCharacter *>                                             ft_IContextualQuery_SelectTargetPoint(        "IContextualQuery::SelectTargetPoint");
static MemberFuncThunk<const IContextualQuery *, QueryResponse,        const INextBot *, const Vector&>                                                            ft_IContextualQuery_IsPositionAllowed(        "IContextualQuery::IsPositionAllowed");
static MemberFuncThunk<const IContextualQuery *, const CKnownEntity *, const INextBot *, const CBaseCombatCharacter *, const CKnownEntity *, const CKnownEntity *> ft_IContextualQuery_SelectMoreDangerousThreat("IContextualQuery::SelectMoreDangerousThreat");

/* Path */
static MemberFuncThunk<const Path *, bool>                                                                   ft_Path_IsValid                       ("Path::IsValid");
static MemberFuncThunk<      Path *, bool, INextBot *, const Vector&>                                        ft_Path_ComputePathDetails            ("Path::ComputePathDetails");
// Inlined in the code
// static MemberFuncThunk<      Path *, bool, INextBot *, const Vector&, CTFBotPathCost&, float, bool>          ft_Path_Compute_CTFBotPathCost_goal   ("Path::Compute<CTFBotPathCost> [goal]");
// static MemberFuncThunk<      Path *, bool, INextBot *, CBaseCombatCharacter *, CTFBotPathCost&, float, bool> ft_Path_Compute_CTFBotPathCost_subject("Path::Compute<CTFBotPathCost> [subject]");
static MemberFuncThunk<      Path *, bool, INextBot *, const Vector&>                                        ft_Path_BuildTrivialPath              ("Path::BuildTrivialPath");
static MemberFuncThunk<      Path *, void, INextBot *>                                                       ft_Path_Optimize                      ("Path::Optimize");
static MemberFuncThunk<      Path *, void>                                                                   ft_Path_PostProcess                   ("Path::PostProcess");

/* PathFollower */
static MemberFuncThunk<PathFollower *, void, INextBot *> ft_PathFollower_Update                 ("PathFollower::Update");
static MemberFuncThunk<PathFollower *, void>             ft_PathFollower_Invalidate             ("PathFollower::Invalidate");
static MemberFuncThunk<PathFollower *, void, float>      ft_PathFollower_SetMinLookAheadDistance("PathFollower::SetMinLookAheadDistance");

/* ChasePath */

/* CTFBotPathCost */
static MemberFuncThunk<const CTFBotPathCost *, float, CNavArea *, CNavArea *, const CNavLadder *, const CFuncElevator *, float> ft_CTFBotPathCost_op_func("CTFBotPathCost::operator()");

/* Behavior<CTFBot> */
static MemberFuncThunk<const Behavior<CTFBot> *, INER *> ft_Behavior_FirstContainedResponder("Behavior<CTFBot>::FirstContainedResponder");

/* Action<CTFBot> */
static MemberFuncThunk<const Action<CTFBot> *, INER *>                                                                          ft_Action_INER_FirstContainedResponder(       "Action<CTFBot>::FirstContainedResponder"        " [INER]");
static MemberFuncThunk<const Action<CTFBot> *, INER *, INER *>                                                                  ft_Action_INER_NextContainedResponder(        "Action<CTFBot>::NextContainedResponder"         " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLeaveGround(                 "Action<CTFBot>::OnLeaveGround"                  " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLandOnGround(                "Action<CTFBot>::OnLandOnGround"                 " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *, CGameTrace *>                                   ft_Action_INER_OnContact(                     "Action<CTFBot>::OnContact"                      " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             const Path *>                                                  ft_Action_INER_OnMoveToSuccess(               "Action<CTFBot>::OnMoveToSuccess"                " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             const Path *, INER::MoveToFailureType>                         ft_Action_INER_OnMoveToFailure(               "Action<CTFBot>::OnMoveToFailure"                " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnStuck(                       "Action<CTFBot>::OnStuck"                        " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnUnStuck(                     "Action<CTFBot>::OnUnStuck"                      " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnPostureChanged(              "Action<CTFBot>::OnPostureChanged"               " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnAnimationActivityComplete(   "Action<CTFBot>::OnAnimationActivityComplete"    " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnAnimationActivityInterrupted("Action<CTFBot>::OnAnimationActivityInterrupted" " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             animevent_t *>                                                 ft_Action_INER_OnAnimationEvent(              "Action<CTFBot>::OnAnimationEvent"               " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnIgnite(                      "Action<CTFBot>::OnIgnite"                       " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             const CTakeDamageInfo&>                                        ft_Action_INER_OnInjured(                     "Action<CTFBot>::OnInjured"                      " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             const CTakeDamageInfo&>                                        ft_Action_INER_OnKilled(                      "Action<CTFBot>::OnKilled"                       " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseCombatCharacter *, const CTakeDamageInfo&>                ft_Action_INER_OnOtherKilled(                 "Action<CTFBot>::OnOtherKilled"                  " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnSight(                       "Action<CTFBot>::OnSight"                        " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLostSight(                   "Action<CTFBot>::OnLostSight"                    " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *, const Vector&, KeyValues *>                     ft_Action_INER_OnSound(                       "Action<CTFBot>::OnSound"                        " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseCombatCharacter *, const char *, AI_Response *>           ft_Action_INER_OnSpokeConcept(                "Action<CTFBot>::OnSpokeConcept"                 " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseCombatCharacter *, CBaseCombatWeapon *>                   ft_Action_INER_OnWeaponFired(                 "Action<CTFBot>::OnWeaponFired"                  " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CNavArea *, CNavArea *>                                        ft_Action_INER_OnNavAreaChanged(              "Action<CTFBot>::OnNavAreaChanged"               " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnModelChanged(                "Action<CTFBot>::OnModelChanged"                 " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *, CBaseCombatCharacter *>                         ft_Action_INER_OnPickUp(                      "Action<CTFBot>::OnPickUp"                       " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnDrop(                        "Action<CTFBot>::OnDrop"                         " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseCombatCharacter *, int>                                   ft_Action_INER_OnActorEmoted(                 "Action<CTFBot>::OnActorEmoted"                  " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnCommandAttack(               "Action<CTFBot>::OnCommandAttack"                " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             const Vector&, float>                                          ft_Action_INER_OnCommandApproach_vec(         "Action<CTFBot>::OnCommandApproach(vec)"         " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnCommandApproach_ent(         "Action<CTFBot>::OnCommandApproach(ent)"         " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *, float>                                          ft_Action_INER_OnCommandRetreat(              "Action<CTFBot>::OnCommandRetreat"               " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             float>                                                         ft_Action_INER_OnCommandPause(                "Action<CTFBot>::OnCommandPause"                 " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnCommandResume(               "Action<CTFBot>::OnCommandResume"                " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             const char *>                                                  ft_Action_INER_OnCommandString(               "Action<CTFBot>::OnCommandString"                " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnShoved(                      "Action<CTFBot>::OnShoved"                       " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnBlinded(                     "Action<CTFBot>::OnBlinded"                      " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnTerritoryContested(          "Action<CTFBot>::OnTerritoryContested"           " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnTerritoryCaptured(           "Action<CTFBot>::OnTerritoryCaptured"            " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnTerritoryLost(               "Action<CTFBot>::OnTerritoryLost"                " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnWin(                         "Action<CTFBot>::OnWin"                          " [INER]");
static MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnLose(                        "Action<CTFBot>::OnLose"                         " [INER]");
static MemberFuncThunk<const Action<CTFBot> *, bool,             const char *>                                                  ft_Action_IsNamed(                            "Action<CTFBot>::IsNamed");
static MemberFuncThunk<const Action<CTFBot> *, char *>                                                                          ft_Action_GetFullName(                        "Action<CTFBot>::GetFullName");
static MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, Action<CTFBot> *>                                    ft_Action_OnStart(                            "Action<CTFBot>::OnStart");
static MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, float>                                               ft_Action_Update(                             "Action<CTFBot>::Update");
static MemberFuncThunk<      Action<CTFBot> *, void,             CTFBot *, Action<CTFBot> *>                                    ft_Action_OnEnd(                              "Action<CTFBot>::OnEnd");
static MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, Action<CTFBot> *>                                    ft_Action_OnSuspend(                          "Action<CTFBot>::OnSuspend");
static MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, Action<CTFBot> *>                                    ft_Action_OnResume(                           "Action<CTFBot>::OnResume");
static MemberFuncThunk<      Action<CTFBot> *, Action<CTFBot> *, CTFBot *>                                                      ft_Action_InitialContainedAction(             "Action<CTFBot>::InitialContainedAction");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnLeaveGround(                      "Action<CTFBot>::OnLeaveGround");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnLandOnGround(                     "Action<CTFBot>::OnLandOnGround");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *, CGameTrace *>                         ft_Action_OnContact(                          "Action<CTFBot>::OnContact");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const Path *>                                        ft_Action_OnMoveToSuccess(                    "Action<CTFBot>::OnMoveToSuccess");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const Path *, INER::MoveToFailureType>               ft_Action_OnMoveToFailure(                    "Action<CTFBot>::OnMoveToFailure");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnStuck(                            "Action<CTFBot>::OnStuck");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnUnStuck(                          "Action<CTFBot>::OnUnStuck");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnPostureChanged(                   "Action<CTFBot>::OnPostureChanged");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnAnimationActivityComplete(        "Action<CTFBot>::OnAnimationActivityComplete");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnAnimationActivityInterrupted(     "Action<CTFBot>::OnAnimationActivityInterrupted");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, animevent_t *>                                       ft_Action_OnAnimationEvent(                   "Action<CTFBot>::OnAnimationEvent");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnIgnite(                           "Action<CTFBot>::OnIgnite");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const CTakeDamageInfo&>                              ft_Action_OnInjured(                          "Action<CTFBot>::OnInjured");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const CTakeDamageInfo&>                              ft_Action_OnKilled(                           "Action<CTFBot>::OnKilled");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseCombatCharacter *, const CTakeDamageInfo&>      ft_Action_OnOtherKilled(                      "Action<CTFBot>::OnOtherKilled");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnSight(                            "Action<CTFBot>::OnSight");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnLostSight(                        "Action<CTFBot>::OnLostSight");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *, const Vector&, KeyValues *>           ft_Action_OnSound(                            "Action<CTFBot>::OnSound");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseCombatCharacter *, const char *, AI_Response *> ft_Action_OnSpokeConcept(                     "Action<CTFBot>::OnSpokeConcept");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseCombatCharacter *, CBaseCombatWeapon *>         ft_Action_OnWeaponFired(                      "Action<CTFBot>::OnWeaponFired");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CNavArea *, CNavArea *>                              ft_Action_OnNavAreaChanged(                   "Action<CTFBot>::OnNavAreaChanged");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnModelChanged(                     "Action<CTFBot>::OnModelChanged");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *, CBaseCombatCharacter *>               ft_Action_OnPickUp(                           "Action<CTFBot>::OnPickUp");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnDrop(                             "Action<CTFBot>::OnDrop");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseCombatCharacter *, int>                         ft_Action_OnActorEmoted(                      "Action<CTFBot>::OnActorEmoted");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnCommandAttack(                    "Action<CTFBot>::OnCommandAttack");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const Vector&, float>                                ft_Action_OnCommandApproach_vec(              "Action<CTFBot>::OnCommandApproach(vec)");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnCommandApproach_ent(              "Action<CTFBot>::OnCommandApproach(ent)");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *, float>                                ft_Action_OnCommandRetreat(                   "Action<CTFBot>::OnCommandRetreat");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, float>                                               ft_Action_OnCommandPause(                     "Action<CTFBot>::OnCommandPause");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnCommandResume(                    "Action<CTFBot>::OnCommandResume");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const char *>                                        ft_Action_OnCommandString(                    "Action<CTFBot>::OnCommandString");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnShoved(                           "Action<CTFBot>::OnShoved");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnBlinded(                          "Action<CTFBot>::OnBlinded");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnTerritoryContested(               "Action<CTFBot>::OnTerritoryContested");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnTerritoryCaptured(                "Action<CTFBot>::OnTerritoryCaptured");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnTerritoryLost(                    "Action<CTFBot>::OnTerritoryLost");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnWin(                              "Action<CTFBot>::OnWin");
static MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnLose(                             "Action<CTFBot>::OnLose");
static MemberFuncThunk<const Action<CTFBot> *, bool,             const INextBot *>                                              ft_Action_IsAbleToBlockMovementOf(            "Action<CTFBot>::IsAbleToBlockMovementOf");
static MemberFuncThunk<      Action<CTFBot> *, Action<CTFBot> *, CTFBot *, Behavior<CTFBot> *, AR>                              ft_Action_ApplyResult(                        "Action<CTFBot>::ApplyResult");
static MemberFuncThunk<      Action<CTFBot> *, void,             CTFBot *, Behavior<CTFBot> *, Action<CTFBot> *>                ft_Action_InvokeOnEnd(                        "Action<CTFBot>::InvokeOnEnd [clone]");
static MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, Behavior<CTFBot> *, Action<CTFBot> *>                ft_Action_InvokeOnResume(                     "Action<CTFBot>::InvokeOnResume");
static MemberFuncThunk<const Action<CTFBot> *, char *,           char[256], const Action<CTFBot> *>                             ft_Action_BuildDecoratedName(                 "Action<CTFBot>::BuildDecoratedName");
static MemberFuncThunk<const Action<CTFBot> *, char *>                                                                          ft_Action_DebugString(                        "Action<CTFBot>::DebugString");
static MemberFuncThunk<const Action<CTFBot> *, void>                                                                            ft_Action_PrintStateToConsole(                "Action<CTFBot>::PrintStateToConsole");

/* NextBotManager */
static MemberFuncThunk<NextBotManager *, void, CUtlVector<INextBot *> *> ft_NextBotManager_CollectAllBots("NextBotManager::CollectAllBots");
static StaticFuncThunk<NextBotManager&> ft_TheNextBots("TheNextBots");

bool INextBot::IsRangeLessThan(CBaseEntity *ent, float radius) const    { return ft_INextBot_IsRangeLessThan(this, ent, radius);          }
bool INextBot::IsRangeGreaterThan(CBaseEntity *ent, float radius) const { return ft_INextBot_IsRangeGreaterThan(this, ent, radius);       }
CBaseCombatCharacter *INextBot::GetEntity() const                       { return vt_INextBot_GetEntity(this);          }

INextBot *INextBotComponent::GetBot() const { return vt_INextBotComponent_GetBot(this);       }

/* CKnownEntity */
void CKnownEntity::Destroy()                                  {        ft_CKnownEntity_Destroy                     (this);          }
void CKnownEntity::UpdatePosition()                           {        ft_CKnownEntity_UpdatePosition              (this);          }
CBaseEntity *CKnownEntity::GetEntity() const                  { return ft_CKnownEntity_GetEntity                   (this);          }
const Vector *CKnownEntity::GetLastKnownPosition() const      { return ft_CKnownEntity_GetLastKnownPosition        (this);          }
bool CKnownEntity::HasLastKnownPositionBeenSeen() const       { return ft_CKnownEntity_HasLastKnownPositionBeenSeen(this);          }
void CKnownEntity::MarkLastKnownPositionAsSeen()              {        ft_CKnownEntity_MarkLastKnownPositionAsSeen (this);          }
CNavArea *CKnownEntity::GetLastKnownArea() const              { return ft_CKnownEntity_GetLastKnownArea            (this);          }
float CKnownEntity::GetTimeSinceLastKnown() const             { return ft_CKnownEntity_GetTimeSinceLastKnown       (this);          }
float CKnownEntity::GetTimeSinceBecameKnown() const           { return ft_CKnownEntity_GetTimeSinceBecameKnown     (this);          }
void CKnownEntity::UpdateVisibilityStatus(bool visible)       {        ft_CKnownEntity_UpdateVisibilityStatus      (this, visible); }
bool CKnownEntity::IsVisibleInFOVNow() const                  { return ft_CKnownEntity_IsVisibleInFOVNow           (this);          }
bool CKnownEntity::IsVisibleRecently() const                  { return ft_CKnownEntity_IsVisibleRecently           (this);          }
float CKnownEntity::GetTimeSinceBecameVisible() const         { return ft_CKnownEntity_GetTimeSinceBecameVisible   (this);          }
float CKnownEntity::GetTimeWhenBecameVisible() const          { return ft_CKnownEntity_GetTimeWhenBecameVisible    (this);          }
float CKnownEntity::GetTimeSinceLastSeen() const              { return ft_CKnownEntity_GetTimeSinceLastSeen        (this);          }
bool CKnownEntity::WasEverVisible() const                     { return ft_CKnownEntity_WasEverVisible              (this);          }
bool CKnownEntity::IsObsolete() const                         { return ft_CKnownEntity_IsObsolete                  (this);          }
bool CKnownEntity::operator==(const CKnownEntity& that) const { return ft_CKnownEntity_op_equal                    (this, that);    }
bool CKnownEntity::Is(CBaseEntity *ent) const                 { return ft_CKnownEntity_Is                          (this, ent);     }

/* INextBotEventResponder */
INextBotEventResponder *INextBotEventResponder::FirstContainedResponder() const                               { return ft_INextBotEventResponder_FirstContainedResponder       (this);                    }
INextBotEventResponder *INextBotEventResponder::NextContainedResponder(INextBotEventResponder *prev) const    { return ft_INextBotEventResponder_NextContainedResponder        (this, prev);              }
void INextBotEventResponder::OnLeaveGround(CBaseEntity *ent)                                                  {        ft_INextBotEventResponder_OnLeaveGround                 (this, ent);               }
void INextBotEventResponder::OnLandOnGround(CBaseEntity *ent)                                                 {        ft_INextBotEventResponder_OnLandOnGround                (this, ent);               }
void INextBotEventResponder::OnContact(CBaseEntity *ent, CGameTrace *trace)                                   {        ft_INextBotEventResponder_OnContact                     (this, ent, trace);        }
void INextBotEventResponder::OnMoveToSuccess(const Path *path)                                                {        ft_INextBotEventResponder_OnMoveToSuccess               (this, path);              }
void INextBotEventResponder::OnMoveToFailure(const Path *path, MoveToFailureType fail)                        {        ft_INextBotEventResponder_OnMoveToFailure               (this, path, fail);        }
void INextBotEventResponder::OnStuck()                                                                        {        ft_INextBotEventResponder_OnStuck                       (this);                    }
void INextBotEventResponder::OnUnStuck()                                                                      {        ft_INextBotEventResponder_OnUnStuck                     (this);                    }
void INextBotEventResponder::OnPostureChanged()                                                               {        ft_INextBotEventResponder_OnPostureChanged              (this);                    }
void INextBotEventResponder::OnAnimationActivityComplete(int i1)                                              {        ft_INextBotEventResponder_OnAnimationActivityComplete   (this, i1);                }
void INextBotEventResponder::OnAnimationActivityInterrupted(int i1)                                           {        ft_INextBotEventResponder_OnAnimationActivityInterrupted(this, i1);                }
void INextBotEventResponder::OnAnimationEvent(animevent_t *a1)                                                {        ft_INextBotEventResponder_OnAnimationEvent              (this, a1);                }
void INextBotEventResponder::OnIgnite()                                                                       {        ft_INextBotEventResponder_OnIgnite                      (this);                    }
void INextBotEventResponder::OnInjured(const CTakeDamageInfo& info)                                           {        ft_INextBotEventResponder_OnInjured                     (this, info);              }
void INextBotEventResponder::OnKilled(const CTakeDamageInfo& info)                                            {        ft_INextBotEventResponder_OnKilled                      (this, info);              }
void INextBotEventResponder::OnOtherKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info)            {        ft_INextBotEventResponder_OnOtherKilled                 (this, who, info);         }
void INextBotEventResponder::OnSight(CBaseEntity *ent)                                                        {        ft_INextBotEventResponder_OnSight                       (this, ent);               }
void INextBotEventResponder::OnLostSight(CBaseEntity *ent)                                                    {        ft_INextBotEventResponder_OnLostSight                   (this, ent);               }
void INextBotEventResponder::OnSound(CBaseEntity *ent, const Vector& v1, KeyValues *kv)                       {        ft_INextBotEventResponder_OnSound                       (this, ent, v1, kv);       }
void INextBotEventResponder::OnSpokeConcept(CBaseCombatCharacter *who, const char *s1, AI_Response *response) {        ft_INextBotEventResponder_OnSpokeConcept                (this, who, s1, response); }
void INextBotEventResponder::OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)              {        ft_INextBotEventResponder_OnWeaponFired                 (this, who, weapon);       }
void INextBotEventResponder::OnNavAreaChanged(CNavArea *area1, CNavArea *area2)                               {        ft_INextBotEventResponder_OnNavAreaChanged              (this, area1, area2);      }
void INextBotEventResponder::OnModelChanged()                                                                 {        ft_INextBotEventResponder_OnModelChanged                (this);                    }
void INextBotEventResponder::OnPickUp(CBaseEntity *ent, CBaseCombatCharacter *who)                            {        ft_INextBotEventResponder_OnPickUp                      (this, ent, who);          }
void INextBotEventResponder::OnDrop(CBaseEntity *ent)                                                         {        ft_INextBotEventResponder_OnDrop                        (this, ent);               }
void INextBotEventResponder::OnActorEmoted(CBaseCombatCharacter *who, int emote_concept)                      {        ft_INextBotEventResponder_OnActorEmoted                 (this, who, emote_concept);}
void INextBotEventResponder::OnCommandAttack(CBaseEntity *ent)                                                {        ft_INextBotEventResponder_OnCommandAttack               (this, ent);               }
void INextBotEventResponder::OnCommandApproach(const Vector& v1, float f1)                                    {        ft_INextBotEventResponder_OnCommandApproach_vec         (this, v1, f1);            }
void INextBotEventResponder::OnCommandApproach(CBaseEntity *ent)                                              {        ft_INextBotEventResponder_OnCommandApproach_ent         (this, ent);               }
void INextBotEventResponder::OnCommandRetreat(CBaseEntity *ent, float f1)                                     {        ft_INextBotEventResponder_OnCommandRetreat              (this, ent, f1);           }
void INextBotEventResponder::OnCommandPause(float f1)                                                         {        ft_INextBotEventResponder_OnCommandPause                (this, f1);                }
void INextBotEventResponder::OnCommandResume()                                                                {        ft_INextBotEventResponder_OnCommandResume               (this);                    }
void INextBotEventResponder::OnCommandString(const char *cmd)                                                 {        ft_INextBotEventResponder_OnCommandString               (this, cmd);               }
void INextBotEventResponder::OnShoved(CBaseEntity *ent)                                                       {        ft_INextBotEventResponder_OnShoved                      (this, ent);               }
void INextBotEventResponder::OnBlinded(CBaseEntity *ent)                                                      {        ft_INextBotEventResponder_OnBlinded                     (this, ent);               }
void INextBotEventResponder::OnTerritoryContested(int i1)                                                     {        ft_INextBotEventResponder_OnTerritoryContested          (this, i1);                }
void INextBotEventResponder::OnTerritoryCaptured(int i1)                                                      {        ft_INextBotEventResponder_OnTerritoryCaptured           (this, i1);                }
void INextBotEventResponder::OnTerritoryLost(int i1)                                                          {        ft_INextBotEventResponder_OnTerritoryLost               (this, i1);                }
void INextBotEventResponder::OnWin()                                                                          {        ft_INextBotEventResponder_OnWin                         (this);                    }
void INextBotEventResponder::OnLose()                                                                         {        ft_INextBotEventResponder_OnLose                        (this);                    }

/* IContextualQuery */
QueryResponse IContextualQuery::ShouldPickUp(const INextBot *nextbot, CBaseEntity *it) const                                                                                               { return ft_IContextualQuery_ShouldPickUp             (this, nextbot, it);                     }
QueryResponse IContextualQuery::ShouldHurry(const INextBot *nextbot) const                                                                                                                 { return ft_IContextualQuery_ShouldHurry              (this, nextbot);                         }
QueryResponse IContextualQuery::ShouldRetreat(const INextBot *nextbot) const                                                                                                               { return ft_IContextualQuery_ShouldRetreat            (this, nextbot);                         }
QueryResponse IContextualQuery::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const                                                                                    { return ft_IContextualQuery_ShouldAttack             (this, nextbot, threat);                 }
QueryResponse IContextualQuery::IsHindrance(const INextBot *nextbot, CBaseEntity *it) const                                                                                                { return ft_IContextualQuery_IsHindrance              (this, nextbot, it);                     }
Vector IContextualQuery::SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const                                                                                { return ft_IContextualQuery_SelectTargetPoint        (this, nextbot, them);                   }
QueryResponse IContextualQuery::IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const                                                                                         { return ft_IContextualQuery_IsPositionAllowed        (this, nextbot, v1);                     }
const CKnownEntity *IContextualQuery::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const { return ft_IContextualQuery_SelectMoreDangerousThreat(this, nextbot, them, threat1, threat2); }

/* Path */
bool Path::IsValid() const                                                                                                                           { return ft_Path_IsValid                       (this); }
bool Path::ComputePathDetails(INextBot *nextbot, const Vector& vec)                                                                                  { return ft_Path_ComputePathDetails            (this, nextbot, vec); }
#if TOOLCHAIN_FIXES
template<> bool Path::Compute<CTFBotPathCost>(INextBot *nextbot, const Vector& vec, CTFBotPathCost& cost_func, float maxPathLength, bool b1)         { return ft_Path_Compute_CTFBotPathCost_goal   (this, nextbot, vec, cost_func, maxPathLength, b1); }
template<> bool Path::Compute<CTFBotPathCost>(INextBot *nextbot, CBaseCombatCharacter *who, CTFBotPathCost& cost_func, float maxPathLength, bool b1) { return ft_Path_Compute_CTFBotPathCost_subject(this, nextbot, who, cost_func, maxPathLength, b1); }
#endif
bool Path::BuildTrivialPath(INextBot *nextbot, const Vector& dest)                                                                                   { return ft_Path_BuildTrivialPath              (this, nextbot, dest); }
void Path::Optimize(INextBot *nextbot)                                                                                                               {        ft_Path_Optimize                      (this, nextbot); }
void Path::PostProcess()                                                                                                                             {        ft_Path_PostProcess                   (this); }

/* PathFollower */
void PathFollower::Update(INextBot *nextbot)           { ft_PathFollower_Update                 (this, nextbot); }
void PathFollower::Invalidate()                        { ft_PathFollower_Invalidate             (this); }
void PathFollower::SetMinLookAheadDistance(float dist) { ft_PathFollower_SetMinLookAheadDistance(this, dist); }

/* ChasePath */

/* CTFBotPathCost */
float CTFBotPathCost::operator()(CNavArea *area1, CNavArea *area2, const CNavLadder *ladder, const CFuncElevator *elevator, float f1) const { return ft_CTFBotPathCost_op_func(this, area1, area2, ladder, elevator, f1); }

/* Behavior<CTFBot> */
template<> INextBotEventResponder *Behavior<CTFBot>::FirstContainedResponder() const { return ft_Behavior_FirstContainedResponder(this); }

/* Action<CTFBot> */
template<> INextBotEventResponder *Action<CTFBot>::FirstContainedResponder() const                                                                    { return ft_Action_INER_FirstContainedResponder       (this);                           }
template<> INextBotEventResponder *Action<CTFBot>::NextContainedResponder(INextBotEventResponder *prev) const                                         { return ft_Action_INER_NextContainedResponder        (this, prev);                     }
template<> void Action<CTFBot>::OnLeaveGround(CBaseEntity *ent)                                                                                       {        ft_Action_INER_OnLeaveGround                 (this, ent);                      }
template<> void Action<CTFBot>::OnLandOnGround(CBaseEntity *ent)                                                                                      {        ft_Action_INER_OnLandOnGround                (this, ent);                      }
template<> void Action<CTFBot>::OnContact(CBaseEntity *ent, CGameTrace *trace)                                                                        {        ft_Action_INER_OnContact                     (this, ent, trace);               }
template<> void Action<CTFBot>::OnMoveToSuccess(const Path *path)                                                                                     {        ft_Action_INER_OnMoveToSuccess               (this, path);                     }
template<> void Action<CTFBot>::OnMoveToFailure(const Path *path, MoveToFailureType fail)                                                             {        ft_Action_INER_OnMoveToFailure               (this, path, fail);               }
template<> void Action<CTFBot>::OnStuck()                                                                                                             {        ft_Action_INER_OnStuck                       (this);                           }
template<> void Action<CTFBot>::OnUnStuck()                                                                                                           {        ft_Action_INER_OnUnStuck                     (this);                           }
template<> void Action<CTFBot>::OnPostureChanged()                                                                                                    {        ft_Action_INER_OnPostureChanged              (this);                           }
template<> void Action<CTFBot>::OnAnimationActivityComplete(int i1)                                                                                   {        ft_Action_INER_OnAnimationActivityComplete   (this, i1);                       }
template<> void Action<CTFBot>::OnAnimationActivityInterrupted(int i1)                                                                                {        ft_Action_INER_OnAnimationActivityInterrupted(this, i1);                       }
template<> void Action<CTFBot>::OnAnimationEvent(animevent_t *a1)                                                                                     {        ft_Action_INER_OnAnimationEvent              (this, a1);                       }
template<> void Action<CTFBot>::OnIgnite()                                                                                                            {        ft_Action_INER_OnIgnite                      (this);                           }
template<> void Action<CTFBot>::OnInjured(const CTakeDamageInfo& info)                                                                                {        ft_Action_INER_OnInjured                     (this, info);                     }
template<> void Action<CTFBot>::OnKilled(const CTakeDamageInfo& info)                                                                                 {        ft_Action_INER_OnKilled                      (this, info);                     }
template<> void Action<CTFBot>::OnOtherKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info)                                                 {        ft_Action_INER_OnOtherKilled                 (this, who, info);                }
template<> void Action<CTFBot>::OnSight(CBaseEntity *ent)                                                                                             {        ft_Action_INER_OnSight                       (this, ent);                      }
template<> void Action<CTFBot>::OnLostSight(CBaseEntity *ent)                                                                                         {        ft_Action_INER_OnLostSight                   (this, ent);                      }
template<> void Action<CTFBot>::OnSound(CBaseEntity *ent, const Vector& v1, KeyValues *kv)                                                            {        ft_Action_INER_OnSound                       (this, ent, v1, kv);              }
template<> void Action<CTFBot>::OnSpokeConcept(CBaseCombatCharacter *who, const char *s1, AI_Response *response)                                      {        ft_Action_INER_OnSpokeConcept                (this, who, s1, response);        }
template<> void Action<CTFBot>::OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)                                                   {        ft_Action_INER_OnWeaponFired                 (this, who, weapon);              }
template<> void Action<CTFBot>::OnNavAreaChanged(CNavArea *area1, CNavArea *area2)                                                                    {        ft_Action_INER_OnNavAreaChanged              (this, area1, area2);             }
template<> void Action<CTFBot>::OnModelChanged()                                                                                                      {        ft_Action_INER_OnModelChanged                (this);                           }
template<> void Action<CTFBot>::OnPickUp(CBaseEntity *ent, CBaseCombatCharacter *who)                                                                 {        ft_Action_INER_OnPickUp                      (this, ent, who);                 }
template<> void Action<CTFBot>::OnDrop(CBaseEntity *ent)                                                                                              {        ft_Action_INER_OnDrop                        (this, ent);                      }
template<> void Action<CTFBot>::OnActorEmoted(CBaseCombatCharacter *who, int emote_concept)                                                           {        ft_Action_INER_OnActorEmoted                 (this, who, emote_concept);       }
template<> void Action<CTFBot>::OnCommandAttack(CBaseEntity *ent)                                                                                     {        ft_Action_INER_OnCommandAttack               (this, ent);                      }
template<> void Action<CTFBot>::OnCommandApproach(const Vector& v1, float f1)                                                                         {        ft_Action_INER_OnCommandApproach_vec         (this, v1, f1);                   }
template<> void Action<CTFBot>::OnCommandApproach(CBaseEntity *ent)                                                                                   {        ft_Action_INER_OnCommandApproach_ent         (this, ent);                      }
template<> void Action<CTFBot>::OnCommandRetreat(CBaseEntity *ent, float f1)                                                                          {        ft_Action_INER_OnCommandRetreat              (this, ent, f1);                  }
template<> void Action<CTFBot>::OnCommandPause(float f1)                                                                                              {        ft_Action_INER_OnCommandPause                (this, f1);                       }
template<> void Action<CTFBot>::OnCommandResume()                                                                                                     {        ft_Action_INER_OnCommandResume               (this);                           }
template<> void Action<CTFBot>::OnCommandString(const char *cmd)                                                                                      {        ft_Action_INER_OnCommandString               (this, cmd);                      }
template<> void Action<CTFBot>::OnShoved(CBaseEntity *ent)                                                                                            {        ft_Action_INER_OnShoved                      (this, ent);                      }
template<> void Action<CTFBot>::OnBlinded(CBaseEntity *ent)                                                                                           {        ft_Action_INER_OnBlinded                     (this, ent);                      }
template<> void Action<CTFBot>::OnTerritoryContested(int i1)                                                                                          {        ft_Action_INER_OnTerritoryContested          (this, i1);                       }
template<> void Action<CTFBot>::OnTerritoryCaptured(int i1)                                                                                           {        ft_Action_INER_OnTerritoryCaptured           (this, i1);                       }
template<> void Action<CTFBot>::OnTerritoryLost(int i1)                                                                                               {        ft_Action_INER_OnTerritoryLost               (this, i1);                       }
template<> void Action<CTFBot>::OnWin()                                                                                                               {        ft_Action_INER_OnWin                         (this);                           }
template<> void Action<CTFBot>::OnLose()                                                                                                              {        ft_Action_INER_OnLose                        (this);                           }
template<> bool Action<CTFBot>::IsNamed(const char *name) const                                                                                       { return ft_Action_IsNamed                            (this, name);                     }
template<> char *Action<CTFBot>::GetFullName() const                                                                                                  { return ft_Action_GetFullName                        (this);                           }
template<> ActionResult<CTFBot> Action<CTFBot>::OnStart(CTFBot *actor, Action<CTFBot> *action)                                                        { return ft_Action_OnStart                            (this, actor, action);            }
template<> ActionResult<CTFBot> Action<CTFBot>::Update(CTFBot *actor, float dt)                                                                       { return ft_Action_Update                             (this, actor, dt);                }
template<> void Action<CTFBot>::OnEnd(CTFBot *actor, Action<CTFBot> *action)                                                                          {        ft_Action_OnEnd                              (this, actor, action);            }
template<> ActionResult<CTFBot> Action<CTFBot>::OnSuspend(CTFBot *actor, Action<CTFBot> *action)                                                      { return ft_Action_OnSuspend                          (this, actor, action);            }
template<> ActionResult<CTFBot> Action<CTFBot>::OnResume(CTFBot *actor, Action<CTFBot> *action)                                                       { return ft_Action_OnResume                           (this, actor, action);            }
template<> Action<CTFBot> *Action<CTFBot>::InitialContainedAction(CTFBot *actor)                                                                      { return ft_Action_InitialContainedAction             (this, actor);                    }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnLeaveGround(CTFBot *actor, CBaseEntity *ent)                                                  { return ft_Action_OnLeaveGround                      (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnLandOnGround(CTFBot *actor, CBaseEntity *ent)                                                 { return ft_Action_OnLandOnGround                     (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)                                   { return ft_Action_OnContact                          (this, actor, ent, trace);        }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnMoveToSuccess(CTFBot *actor, const Path *path)                                                { return ft_Action_OnMoveToSuccess                    (this, actor, path);              }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)                        { return ft_Action_OnMoveToFailure                    (this, actor, path, fail);        }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnStuck(CTFBot *actor)                                                                          { return ft_Action_OnStuck                            (this, actor);                    }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnUnStuck(CTFBot *actor)                                                                        { return ft_Action_OnUnStuck                          (this, actor);                    }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnPostureChanged(CTFBot *actor)                                                                 { return ft_Action_OnPostureChanged                   (this, actor);                    }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnAnimationActivityComplete(CTFBot *actor, int i1)                                              { return ft_Action_OnAnimationActivityComplete        (this, actor, i1);                }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnAnimationActivityInterrupted(CTFBot *actor, int i1)                                           { return ft_Action_OnAnimationActivityInterrupted     (this, actor, i1);                }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnAnimationEvent(CTFBot *actor, animevent_t *a1)                                                { return ft_Action_OnAnimationEvent                   (this, actor, a1);                }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnIgnite(CTFBot *actor)                                                                         { return ft_Action_OnIgnite                           (this, actor);                    }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)                                           { return ft_Action_OnInjured                          (this, actor, info);              }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnKilled(CTFBot *actor, const CTakeDamageInfo& info)                                            { return ft_Action_OnKilled                           (this, actor, info);              }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnOtherKilled(CTFBot *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info)            { return ft_Action_OnOtherKilled                      (this, actor, who, info);         }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnSight(CTFBot *actor, CBaseEntity *ent)                                                        { return ft_Action_OnSight                            (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnLostSight(CTFBot *actor, CBaseEntity *ent)                                                    { return ft_Action_OnLostSight                        (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnSound(CTFBot *actor, CBaseEntity *ent, const Vector& v1, KeyValues *kv)                       { return ft_Action_OnSound                            (this, actor, ent, v1, kv);       }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnSpokeConcept(CTFBot *actor, CBaseCombatCharacter *who, const char *s1, AI_Response *response) { return ft_Action_OnSpokeConcept                     (this, actor, who, s1, response); }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnWeaponFired(CTFBot *actor, CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)              { return ft_Action_OnWeaponFired                      (this, actor, who, weapon);       }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2)                               { return ft_Action_OnNavAreaChanged                   (this, actor, area1, area2);      }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnModelChanged(CTFBot *actor)                                                                   { return ft_Action_OnModelChanged                     (this, actor);                    }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnPickUp(CTFBot *actor, CBaseEntity *ent, CBaseCombatCharacter *who)                            { return ft_Action_OnPickUp                           (this, actor, ent, who);          }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnDrop(CTFBot *actor, CBaseEntity *ent)                                                         { return ft_Action_OnDrop                             (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnActorEmoted(CTFBot *actor, CBaseCombatCharacter *who, int i1)                                 { return ft_Action_OnActorEmoted                      (this, actor, who, i1);           }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnCommandAttack(CTFBot *actor, CBaseEntity *ent)                                                { return ft_Action_OnCommandAttack                    (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnCommandApproach(CTFBot *actor, const Vector& v1, float f1)                                    { return ft_Action_OnCommandApproach_vec              (this, actor, v1, f1);            }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnCommandApproach(CTFBot *actor, CBaseEntity *ent)                                              { return ft_Action_OnCommandApproach_ent              (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnCommandRetreat(CTFBot *actor, CBaseEntity *ent, float f1)                                     { return ft_Action_OnCommandRetreat                   (this, actor, ent, f1);           }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnCommandPause(CTFBot *actor, float f1)                                                         { return ft_Action_OnCommandPause                     (this, actor, f1);                }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnCommandResume(CTFBot *actor)                                                                  { return ft_Action_OnCommandResume                    (this, actor);                    }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnCommandString(CTFBot *actor, const char *cmd)                                                 { return ft_Action_OnCommandString                    (this, actor, cmd);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnShoved(CTFBot *actor, CBaseEntity *ent)                                                       { return ft_Action_OnShoved                           (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnBlinded(CTFBot *actor, CBaseEntity *ent)                                                      { return ft_Action_OnBlinded                          (this, actor, ent);               }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnTerritoryContested(CTFBot *actor, int i1)                                                     { return ft_Action_OnTerritoryContested               (this, actor, i1);                }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnTerritoryCaptured(CTFBot *actor, int i1)                                                      { return ft_Action_OnTerritoryCaptured                (this, actor, i1);                }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnTerritoryLost(CTFBot *actor, int i1)                                                          { return ft_Action_OnTerritoryLost                    (this, actor, i1);                }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnWin(CTFBot *actor)                                                                            { return ft_Action_OnWin                              (this, actor);                    }
template<> EventDesiredResult<CTFBot> Action<CTFBot>::OnLose(CTFBot *actor)                                                                           { return ft_Action_OnLose                             (this, actor);                    }
template<> bool Action<CTFBot>::IsAbleToBlockMovementOf(const INextBot *nextbot) const                                                                { return ft_Action_IsAbleToBlockMovementOf            (this, nextbot);                  }
template<> Action<CTFBot> *Action<CTFBot>::ApplyResult(CTFBot *actor, Behavior<CTFBot> *behavior, ActionResult<CTFBot> result)                        { return ft_Action_ApplyResult                        (this, actor, behavior, result);  }
template<> void Action<CTFBot>::InvokeOnEnd(CTFBot *actor, Behavior<CTFBot> *behavior, Action<CTFBot> *action)                                        {        ft_Action_InvokeOnEnd                        (this, actor, behavior, action);  }
template<> ActionResult<CTFBot> Action<CTFBot>::InvokeOnResume(CTFBot *actor, Behavior<CTFBot> *behavior, Action<CTFBot> *action)                     { return ft_Action_InvokeOnResume                     (this, actor, behavior, action);  }
template<> char *Action<CTFBot>::BuildDecoratedName(char buf[256], const Action<CTFBot> *action) const                                                { return ft_Action_BuildDecoratedName                 (this, buf, action);              }
template<> char *Action<CTFBot>::DebugString() const                                                                                                  { return ft_Action_DebugString                        (this);                           }
template<> void Action<CTFBot>::PrintStateToConsole() const                                                                                           {        ft_Action_PrintStateToConsole                (this);                           }

/* NextBotManager */
void NextBotManager::CollectAllBots(CUtlVector<INextBot *> *nextbots) {        ft_NextBotManager_CollectAllBots(this, nextbots); }
NextBotManager& TheNextBots()                                         { return ft_TheNextBots                  (); }
