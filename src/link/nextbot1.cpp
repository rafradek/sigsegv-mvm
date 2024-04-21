#if SE_TF2
#include "link/link.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"
#include "stub/nextbot_cc.h"


#define INER INextBotEventResponder

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
static MemberVFuncThunk<PathFollower *, void, INextBot *> vt_PathFollower_Update                 ("12PathFollower", "PathFollower::Update");
static MemberVFuncThunk<PathFollower *, void>             vt_PathFollower_Invalidate             ("12PathFollower", "PathFollower::Invalidate");
static MemberVFuncThunk<PathFollower *, void, float>      vt_PathFollower_SetMinLookAheadDistance("12PathFollower", "PathFollower::SetMinLookAheadDistance");

/* ChasePath */
static MemberVFuncThunk<ChasePath *, void, INextBot *, CBaseEntity *, const IPathCost &, Vector *>      vt_ChasePath_Update("9ChasePath", "ChasePath::Update");

/* CTFBotPathCost */
static MemberFuncThunk<const CTFBotPathCost *, float, CNavArea *, CNavArea *, const CNavLadder *, const CFuncElevator *, float> ft_CTFBotPathCost_op_func("CTFBotPathCost::operator()");


/* CZombiePathCost */
static MemberFuncThunk<const CZombiePathCost *, float, CNavArea *, CNavArea *, const CNavLadder *, const CFuncElevator *, float> ft_CZombiePathCost_op_func("CZombiePathCost::operator()");

/* Behavior<CTFBot> */
static MemberFuncThunk<const Behavior<CTFBot> *, INER *> ft_Behavior_FirstContainedResponder("Behavior<CTFBot>::FirstContainedResponder");

/* Action<CTFBot> */

#define INER INextBotEventResponder
#define AR   ActionResult<CBotNPCArcher>
#define EDR  EventDesiredResult<CBotNPCArcher>
template<>  MemberFuncThunk<const Action<CBotNPCArcher> *, INER *>                                                                          ft_Action_INER_FirstContainedResponder<CBotNPCArcher>(       "Action<CZombie>::FirstContainedResponder"        " [INER]");
template<>  MemberFuncThunk<const Action<CBotNPCArcher> *, INER *, INER *>                                                                  ft_Action_INER_NextContainedResponder<CBotNPCArcher>(        "Action<CZombie>::NextContainedResponder"         " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLeaveGround<CBotNPCArcher>(                 "Action<CZombie>::OnLeaveGround"                  " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLandOnGround<CBotNPCArcher>(                "Action<CZombie>::OnLandOnGround"                 " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *, CGameTrace *>                                   ft_Action_INER_OnContact<CBotNPCArcher>(                     "Action<CZombie>::OnContact"                      " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             const Path *>                                                  ft_Action_INER_OnMoveToSuccess<CBotNPCArcher>(               "Action<CZombie>::OnMoveToSuccess"                " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             const Path *, INER::MoveToFailureType>                         ft_Action_INER_OnMoveToFailure<CBotNPCArcher>(               "Action<CZombie>::OnMoveToFailure"                " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void>                                                                            ft_Action_INER_OnStuck<CBotNPCArcher>(                       "Action<CZombie>::OnStuck"                        " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void>                                                                            ft_Action_INER_OnUnStuck<CBotNPCArcher>(                     "Action<CZombie>::OnUnStuck"                      " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void>                                                                            ft_Action_INER_OnPostureChanged<CBotNPCArcher>(              "Action<CZombie>::OnPostureChanged"               " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             int>                                                           ft_Action_INER_OnAnimationActivityComplete<CBotNPCArcher>(   "Action<CZombie>::OnAnimationActivityComplete"    " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             int>                                                           ft_Action_INER_OnAnimationActivityInterrupted<CBotNPCArcher>("Action<CZombie>::OnAnimationActivityInterrupted" " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             animevent_t *>                                                 ft_Action_INER_OnAnimationEvent<CBotNPCArcher>(              "Action<CZombie>::OnAnimationEvent"               " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void>                                                                            ft_Action_INER_OnIgnite<CBotNPCArcher>(                      "Action<CZombie>::OnIgnite"                       " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             const CTakeDamageInfo&>                                        ft_Action_INER_OnInjured<CBotNPCArcher>(                     "Action<CZombie>::OnInjured"                      " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             const CTakeDamageInfo&>                                        ft_Action_INER_OnKilled<CBotNPCArcher>(                      "Action<CZombie>::OnKilled"                       " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseCombatCharacter *, const CTakeDamageInfo&>                ft_Action_INER_OnOtherKilled<CBotNPCArcher>(                 "Action<CZombie>::OnOtherKilled"                  " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnSight<CBotNPCArcher>(                       "Action<CZombie>::OnSight"                        " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLostSight<CBotNPCArcher>(                   "Action<CZombie>::OnLostSight"                    " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *, const Vector&, KeyValues *>                     ft_Action_INER_OnSound<CBotNPCArcher>(                       "Action<CZombie>::OnSound"                        " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseCombatCharacter *, const char *, AI_Response *>           ft_Action_INER_OnSpokeConcept<CBotNPCArcher>(                "Action<CZombie>::OnSpokeConcept"                 " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseCombatCharacter *, CBaseCombatWeapon *>                   ft_Action_INER_OnWeaponFired<CBotNPCArcher>(                 "Action<CZombie>::OnWeaponFired"                  " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CNavArea *, CNavArea *>                                        ft_Action_INER_OnNavAreaChanged<CBotNPCArcher>(              "Action<CZombie>::OnNavAreaChanged"               " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void>                                                                            ft_Action_INER_OnModelChanged<CBotNPCArcher>(                "Action<CZombie>::OnModelChanged"                 " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *, CBaseCombatCharacter *>                         ft_Action_INER_OnPickUp<CBotNPCArcher>(                      "Action<CZombie>::OnPickUp"                       " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnDrop<CBotNPCArcher>(                        "Action<CZombie>::OnDrop"                         " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseCombatCharacter *, int>                                   ft_Action_INER_OnActorEmoted<CBotNPCArcher>(                 "Action<CZombie>::OnActorEmoted"                  " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnCommandAttack<CBotNPCArcher>(               "Action<CZombie>::OnCommandAttack"                " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             const Vector&, float>                                          ft_Action_INER_OnCommandApproach_vec<CBotNPCArcher>(         "Action<CZombie>::OnCommandApproach(vec)"         " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnCommandApproach_ent<CBotNPCArcher>(         "Action<CZombie>::OnCommandApproach(ent)"         " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *, float>                                          ft_Action_INER_OnCommandRetreat<CBotNPCArcher>(              "Action<CZombie>::OnCommandRetreat"               " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             float>                                                         ft_Action_INER_OnCommandPause<CBotNPCArcher>(                "Action<CZombie>::OnCommandPause"                 " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void>                                                                            ft_Action_INER_OnCommandResume<CBotNPCArcher>(               "Action<CZombie>::OnCommandResume"                " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             const char *>                                                  ft_Action_INER_OnCommandString<CBotNPCArcher>(               "Action<CZombie>::OnCommandString"                " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnShoved<CBotNPCArcher>(                      "Action<CZombie>::OnShoved"                       " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnBlinded<CBotNPCArcher>(                     "Action<CZombie>::OnBlinded"                      " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             int>                                                           ft_Action_INER_OnTerritoryContested<CBotNPCArcher>(          "Action<CZombie>::OnTerritoryContested"           " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             int>                                                           ft_Action_INER_OnTerritoryCaptured<CBotNPCArcher>(           "Action<CZombie>::OnTerritoryCaptured"            " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             int>                                                           ft_Action_INER_OnTerritoryLost<CBotNPCArcher>(               "Action<CZombie>::OnTerritoryLost"                " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void>                                                                            ft_Action_INER_OnWin<CBotNPCArcher>(                         "Action<CZombie>::OnWin"                          " [INER]");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void>                                                                            ft_Action_INER_OnLose<CBotNPCArcher>(                        "Action<CZombie>::OnLose"                         " [INER]");
template<>  MemberFuncThunk<const Action<CBotNPCArcher> *, bool,             const char *>                                                  ft_Action_IsNamed<CBotNPCArcher>(                            "Action<CZombie>::IsNamed");
template<>  MemberFuncThunk<const Action<CBotNPCArcher> *, char *>                                                                          ft_Action_GetFullName<CBotNPCArcher>(                        "Action<CZombie>::GetFullName");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, AR,               CBotNPCArcher *, Action<CBotNPCArcher> *>                                    ft_Action_OnStart<CBotNPCArcher>(                            "Action<CZombie>::OnStart");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, AR,               CBotNPCArcher *, float>                                               ft_Action_Update<CBotNPCArcher>(                             "Action<CZombie>::Update");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBotNPCArcher *, Action<CBotNPCArcher> *>                                    ft_Action_OnEnd<CBotNPCArcher>(                              "Action<CZombie>::OnEnd");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, AR,               CBotNPCArcher *, Action<CBotNPCArcher> *>                                    ft_Action_OnSuspend<CBotNPCArcher>(                          "Action<CZombie>::OnSuspend");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, AR,               CBotNPCArcher *, Action<CBotNPCArcher> *>                                    ft_Action_OnResume<CBotNPCArcher>(                           "Action<CZombie>::OnResume");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, Action<CBotNPCArcher> *, CBotNPCArcher *>                                                      ft_Action_InitialContainedAction<CBotNPCArcher>(             "Action<CZombie>::InitialContainedAction");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnLeaveGround<CBotNPCArcher>(                      "Action<CZombie>::OnLeaveGround");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnLandOnGround<CBotNPCArcher>(                     "Action<CZombie>::OnLandOnGround");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *, CGameTrace *>                         ft_Action_OnContact<CBotNPCArcher>(                          "Action<CZombie>::OnContact");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, const Path *>                                        ft_Action_OnMoveToSuccess<CBotNPCArcher>(                    "Action<CZombie>::OnMoveToSuccess");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, const Path *, INER::MoveToFailureType>               ft_Action_OnMoveToFailure<CBotNPCArcher>(                    "Action<CZombie>::OnMoveToFailure");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *>                                                      ft_Action_OnStuck<CBotNPCArcher>(                            "Action<CZombie>::OnStuck");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *>                                                      ft_Action_OnUnStuck<CBotNPCArcher>(                          "Action<CZombie>::OnUnStuck");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *>                                                      ft_Action_OnPostureChanged<CBotNPCArcher>(                   "Action<CZombie>::OnPostureChanged");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, int>                                                 ft_Action_OnAnimationActivityComplete<CBotNPCArcher>(        "Action<CZombie>::OnAnimationActivityComplete");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, int>                                                 ft_Action_OnAnimationActivityInterrupted<CBotNPCArcher>(     "Action<CZombie>::OnAnimationActivityInterrupted");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, animevent_t *>                                       ft_Action_OnAnimationEvent<CBotNPCArcher>(                   "Action<CZombie>::OnAnimationEvent");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *>                                                      ft_Action_OnIgnite<CBotNPCArcher>(                           "Action<CZombie>::OnIgnite");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, const CTakeDamageInfo&>                              ft_Action_OnInjured<CBotNPCArcher>(                          "Action<CZombie>::OnInjured");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, const CTakeDamageInfo&>                              ft_Action_OnKilled<CBotNPCArcher>(                           "Action<CZombie>::OnKilled");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseCombatCharacter *, const CTakeDamageInfo&>      ft_Action_OnOtherKilled<CBotNPCArcher>(                      "Action<CZombie>::OnOtherKilled");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnSight<CBotNPCArcher>(                            "Action<CZombie>::OnSight");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnLostSight<CBotNPCArcher>(                        "Action<CZombie>::OnLostSight");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *, const Vector&, KeyValues *>           ft_Action_OnSound<CBotNPCArcher>(                            "Action<CZombie>::OnSound");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseCombatCharacter *, const char *, AI_Response *> ft_Action_OnSpokeConcept<CBotNPCArcher>(                     "Action<CZombie>::OnSpokeConcept");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseCombatCharacter *, CBaseCombatWeapon *>         ft_Action_OnWeaponFired<CBotNPCArcher>(                      "Action<CZombie>::OnWeaponFired");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CNavArea *, CNavArea *>                              ft_Action_OnNavAreaChanged<CBotNPCArcher>(                   "Action<CZombie>::OnNavAreaChanged");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *>                                                      ft_Action_OnModelChanged<CBotNPCArcher>(                     "Action<CZombie>::OnModelChanged");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *, CBaseCombatCharacter *>               ft_Action_OnPickUp<CBotNPCArcher>(                           "Action<CZombie>::OnPickUp");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnDrop<CBotNPCArcher>(                             "Action<CZombie>::OnDrop");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseCombatCharacter *, int>                         ft_Action_OnActorEmoted<CBotNPCArcher>(                      "Action<CZombie>::OnActorEmoted");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnCommandAttack<CBotNPCArcher>(                    "Action<CZombie>::OnCommandAttack");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, const Vector&, float>                                ft_Action_OnCommandApproach_vec<CBotNPCArcher>(              "Action<CZombie>::OnCommandApproach(vec)");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnCommandApproach_ent<CBotNPCArcher>(              "Action<CZombie>::OnCommandApproach(ent)");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *, float>                                ft_Action_OnCommandRetreat<CBotNPCArcher>(                   "Action<CZombie>::OnCommandRetreat");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, float>                                               ft_Action_OnCommandPause<CBotNPCArcher>(                     "Action<CZombie>::OnCommandPause");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *>                                                      ft_Action_OnCommandResume<CBotNPCArcher>(                    "Action<CZombie>::OnCommandResume");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, const char *>                                        ft_Action_OnCommandString<CBotNPCArcher>(                    "Action<CZombie>::OnCommandString");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnShoved<CBotNPCArcher>(                           "Action<CZombie>::OnShoved");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, CBaseEntity *>                                       ft_Action_OnBlinded<CBotNPCArcher>(                          "Action<CZombie>::OnBlinded");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, int>                                                 ft_Action_OnTerritoryContested<CBotNPCArcher>(               "Action<CZombie>::OnTerritoryContested");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, int>                                                 ft_Action_OnTerritoryCaptured<CBotNPCArcher>(                "Action<CZombie>::OnTerritoryCaptured");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *, int>                                                 ft_Action_OnTerritoryLost<CBotNPCArcher>(                    "Action<CZombie>::OnTerritoryLost");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *>                                                      ft_Action_OnWin<CBotNPCArcher>(                              "Action<CZombie>::OnWin");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, EDR,              CBotNPCArcher *>                                                      ft_Action_OnLose<CBotNPCArcher>(                             "Action<CZombie>::OnLose");
template<>  MemberFuncThunk<const Action<CBotNPCArcher> *, bool,             const INextBot *>                                              ft_Action_IsAbleToBlockMovementOf<CBotNPCArcher>(            "Action<CZombie>::IsAbleToBlockMovementOf");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, Action<CBotNPCArcher> *, CBotNPCArcher *, Behavior<CBotNPCArcher> *, AR>                              ft_Action_ApplyResult<CBotNPCArcher>(                        "Action<CZombie>::ApplyResult");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, void,             CBotNPCArcher *, Behavior<CBotNPCArcher> *, Action<CBotNPCArcher> *>                ft_Action_InvokeOnEnd<CBotNPCArcher>(                        "Action<CZombie>::InvokeOnEnd");
template<>  MemberFuncThunk<      Action<CBotNPCArcher> *, AR,               CBotNPCArcher *, Behavior<CBotNPCArcher> *, Action<CBotNPCArcher> *>                ft_Action_InvokeOnResume<CBotNPCArcher>(                     "Action<CZombie>::InvokeOnResume");
template<>  MemberFuncThunk<const Action<CBotNPCArcher> *, char *,           char[256], const Action<CBotNPCArcher> *>                             ft_Action_BuildDecoratedName<CBotNPCArcher>(                 "Action<CZombie>::BuildDecoratedName [clone]");
//template<>  MemberFuncThunk<const Action<CBotNPCArcher> *, char *>                                                                          ft_Action_DebugString<CBotNPCArcher>(                        "Action<CZombie>::DebugString [clone]");
template<>  MemberFuncThunk<const Action<CBotNPCArcher> *, void>                                                                            ft_Action_PrintStateToConsole<CBotNPCArcher>(                "Action<CZombie>::PrintStateToConsole");

#define AR   ActionResult<CTFBot>
#define EDR  EventDesiredResult<CTFBot>
template<>  MemberFuncThunk<const Action<CTFBot> *, INER *>                                                                          ft_Action_INER_FirstContainedResponder<CTFBot>(       "Action<CTFBot>::FirstContainedResponder"        " [INER]");
template<>  MemberFuncThunk<const Action<CTFBot> *, INER *, INER *>                                                                  ft_Action_INER_NextContainedResponder<CTFBot>(        "Action<CTFBot>::NextContainedResponder"         " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLeaveGround<CTFBot>(                 "Action<CTFBot>::OnLeaveGround"                  " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLandOnGround<CTFBot>(                "Action<CTFBot>::OnLandOnGround"                 " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *, CGameTrace *>                                   ft_Action_INER_OnContact<CTFBot>(                     "Action<CTFBot>::OnContact"                      " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             const Path *>                                                  ft_Action_INER_OnMoveToSuccess<CTFBot>(               "Action<CTFBot>::OnMoveToSuccess"                " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             const Path *, INER::MoveToFailureType>                         ft_Action_INER_OnMoveToFailure<CTFBot>(               "Action<CTFBot>::OnMoveToFailure"                " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnStuck<CTFBot>(                       "Action<CTFBot>::OnStuck"                        " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnUnStuck<CTFBot>(                     "Action<CTFBot>::OnUnStuck"                      " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnPostureChanged<CTFBot>(              "Action<CTFBot>::OnPostureChanged"               " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnAnimationActivityComplete<CTFBot>(   "Action<CTFBot>::OnAnimationActivityComplete"    " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnAnimationActivityInterrupted<CTFBot>("Action<CTFBot>::OnAnimationActivityInterrupted" " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             animevent_t *>                                                 ft_Action_INER_OnAnimationEvent<CTFBot>(              "Action<CTFBot>::OnAnimationEvent"               " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnIgnite<CTFBot>(                      "Action<CTFBot>::OnIgnite"                       " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             const CTakeDamageInfo&>                                        ft_Action_INER_OnInjured<CTFBot>(                     "Action<CTFBot>::OnInjured"                      " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             const CTakeDamageInfo&>                                        ft_Action_INER_OnKilled<CTFBot>(                      "Action<CTFBot>::OnKilled"                       " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseCombatCharacter *, const CTakeDamageInfo&>                ft_Action_INER_OnOtherKilled<CTFBot>(                 "Action<CTFBot>::OnOtherKilled"                  " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnSight<CTFBot>(                       "Action<CTFBot>::OnSight"                        " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnLostSight<CTFBot>(                   "Action<CTFBot>::OnLostSight"                    " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *, const Vector&, KeyValues *>                     ft_Action_INER_OnSound<CTFBot>(                       "Action<CTFBot>::OnSound"                        " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseCombatCharacter *, const char *, AI_Response *>           ft_Action_INER_OnSpokeConcept<CTFBot>(                "Action<CTFBot>::OnSpokeConcept"                 " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseCombatCharacter *, CBaseCombatWeapon *>                   ft_Action_INER_OnWeaponFired<CTFBot>(                 "Action<CTFBot>::OnWeaponFired"                  " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CNavArea *, CNavArea *>                                        ft_Action_INER_OnNavAreaChanged<CTFBot>(              "Action<CTFBot>::OnNavAreaChanged"               " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnModelChanged<CTFBot>(                "Action<CTFBot>::OnModelChanged"                 " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *, CBaseCombatCharacter *>                         ft_Action_INER_OnPickUp<CTFBot>(                      "Action<CTFBot>::OnPickUp"                       " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnDrop<CTFBot>(                        "Action<CTFBot>::OnDrop"                         " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseCombatCharacter *, int>                                   ft_Action_INER_OnActorEmoted<CTFBot>(                 "Action<CTFBot>::OnActorEmoted"                  " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnCommandAttack<CTFBot>(               "Action<CTFBot>::OnCommandAttack"                " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             const Vector&, float>                                          ft_Action_INER_OnCommandApproach_vec<CTFBot>(         "Action<CTFBot>::OnCommandApproach(vec)"         " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnCommandApproach_ent<CTFBot>(         "Action<CTFBot>::OnCommandApproach(ent)"         " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *, float>                                          ft_Action_INER_OnCommandRetreat<CTFBot>(              "Action<CTFBot>::OnCommandRetreat"               " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             float>                                                         ft_Action_INER_OnCommandPause<CTFBot>(                "Action<CTFBot>::OnCommandPause"                 " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnCommandResume<CTFBot>(               "Action<CTFBot>::OnCommandResume"                " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             const char *>                                                  ft_Action_INER_OnCommandString<CTFBot>(               "Action<CTFBot>::OnCommandString"                " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnShoved<CTFBot>(                      "Action<CTFBot>::OnShoved"                       " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CBaseEntity *>                                                 ft_Action_INER_OnBlinded<CTFBot>(                     "Action<CTFBot>::OnBlinded"                      " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnTerritoryContested<CTFBot>(          "Action<CTFBot>::OnTerritoryContested"           " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnTerritoryCaptured<CTFBot>(           "Action<CTFBot>::OnTerritoryCaptured"            " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             int>                                                           ft_Action_INER_OnTerritoryLost<CTFBot>(               "Action<CTFBot>::OnTerritoryLost"                " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnWin<CTFBot>(                         "Action<CTFBot>::OnWin"                          " [INER]");
template<>  MemberFuncThunk<      Action<CTFBot> *, void>                                                                            ft_Action_INER_OnLose<CTFBot>(                        "Action<CTFBot>::OnLose"                         " [INER]");
template<>  MemberFuncThunk<const Action<CTFBot> *, bool,             const char *>                                                  ft_Action_IsNamed<CTFBot>(                            "Action<CTFBot>::IsNamed");
template<>  MemberFuncThunk<const Action<CTFBot> *, char *>                                                                          ft_Action_GetFullName<CTFBot>(                        "Action<CTFBot>::GetFullName");
template<>  MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, Action<CTFBot> *>                                    ft_Action_OnStart<CTFBot>(                            "Action<CTFBot>::OnStart");
template<>  MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, float>                                               ft_Action_Update<CTFBot>(                             "Action<CTFBot>::Update");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CTFBot *, Action<CTFBot> *>                                    ft_Action_OnEnd<CTFBot>(                              "Action<CTFBot>::OnEnd");
template<>  MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, Action<CTFBot> *>                                    ft_Action_OnSuspend<CTFBot>(                          "Action<CTFBot>::OnSuspend");
template<>  MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, Action<CTFBot> *>                                    ft_Action_OnResume<CTFBot>(                           "Action<CTFBot>::OnResume");
template<>  MemberFuncThunk<      Action<CTFBot> *, Action<CTFBot> *, CTFBot *>                                                      ft_Action_InitialContainedAction<CTFBot>(             "Action<CTFBot>::InitialContainedAction");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnLeaveGround<CTFBot>(                      "Action<CTFBot>::OnLeaveGround");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnLandOnGround<CTFBot>(                     "Action<CTFBot>::OnLandOnGround");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *, CGameTrace *>                         ft_Action_OnContact<CTFBot>(                          "Action<CTFBot>::OnContact");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const Path *>                                        ft_Action_OnMoveToSuccess<CTFBot>(                    "Action<CTFBot>::OnMoveToSuccess");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const Path *, INER::MoveToFailureType>               ft_Action_OnMoveToFailure<CTFBot>(                    "Action<CTFBot>::OnMoveToFailure");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnStuck<CTFBot>(                            "Action<CTFBot>::OnStuck");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnUnStuck<CTFBot>(                          "Action<CTFBot>::OnUnStuck");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnPostureChanged<CTFBot>(                   "Action<CTFBot>::OnPostureChanged");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnAnimationActivityComplete<CTFBot>(        "Action<CTFBot>::OnAnimationActivityComplete");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnAnimationActivityInterrupted<CTFBot>(     "Action<CTFBot>::OnAnimationActivityInterrupted");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, animevent_t *>                                       ft_Action_OnAnimationEvent<CTFBot>(                   "Action<CTFBot>::OnAnimationEvent");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnIgnite<CTFBot>(                           "Action<CTFBot>::OnIgnite");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const CTakeDamageInfo&>                              ft_Action_OnInjured<CTFBot>(                          "Action<CTFBot>::OnInjured");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const CTakeDamageInfo&>                              ft_Action_OnKilled<CTFBot>(                           "Action<CTFBot>::OnKilled");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseCombatCharacter *, const CTakeDamageInfo&>      ft_Action_OnOtherKilled<CTFBot>(                      "Action<CTFBot>::OnOtherKilled");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnSight<CTFBot>(                            "Action<CTFBot>::OnSight");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnLostSight<CTFBot>(                        "Action<CTFBot>::OnLostSight");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *, const Vector&, KeyValues *>           ft_Action_OnSound<CTFBot>(                            "Action<CTFBot>::OnSound");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseCombatCharacter *, const char *, AI_Response *> ft_Action_OnSpokeConcept<CTFBot>(                     "Action<CTFBot>::OnSpokeConcept");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseCombatCharacter *, CBaseCombatWeapon *>         ft_Action_OnWeaponFired<CTFBot>(                      "Action<CTFBot>::OnWeaponFired");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CNavArea *, CNavArea *>                              ft_Action_OnNavAreaChanged<CTFBot>(                   "Action<CTFBot>::OnNavAreaChanged");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnModelChanged<CTFBot>(                     "Action<CTFBot>::OnModelChanged");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *, CBaseCombatCharacter *>               ft_Action_OnPickUp<CTFBot>(                           "Action<CTFBot>::OnPickUp");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnDrop<CTFBot>(                             "Action<CTFBot>::OnDrop");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseCombatCharacter *, int>                         ft_Action_OnActorEmoted<CTFBot>(                      "Action<CTFBot>::OnActorEmoted");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnCommandAttack<CTFBot>(                    "Action<CTFBot>::OnCommandAttack");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const Vector&, float>                                ft_Action_OnCommandApproach_vec<CTFBot>(              "Action<CTFBot>::OnCommandApproach(vec)");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnCommandApproach_ent<CTFBot>(              "Action<CTFBot>::OnCommandApproach(ent)");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *, float>                                ft_Action_OnCommandRetreat<CTFBot>(                   "Action<CTFBot>::OnCommandRetreat");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, float>                                               ft_Action_OnCommandPause<CTFBot>(                     "Action<CTFBot>::OnCommandPause");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnCommandResume<CTFBot>(                    "Action<CTFBot>::OnCommandResume");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, const char *>                                        ft_Action_OnCommandString<CTFBot>(                    "Action<CTFBot>::OnCommandString");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnShoved<CTFBot>(                           "Action<CTFBot>::OnShoved");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, CBaseEntity *>                                       ft_Action_OnBlinded<CTFBot>(                          "Action<CTFBot>::OnBlinded");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnTerritoryContested<CTFBot>(               "Action<CTFBot>::OnTerritoryContested");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnTerritoryCaptured<CTFBot>(                "Action<CTFBot>::OnTerritoryCaptured");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *, int>                                                 ft_Action_OnTerritoryLost<CTFBot>(                    "Action<CTFBot>::OnTerritoryLost");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnWin<CTFBot>(                              "Action<CTFBot>::OnWin");
template<>  MemberFuncThunk<      Action<CTFBot> *, EDR,              CTFBot *>                                                      ft_Action_OnLose<CTFBot>(                             "Action<CTFBot>::OnLose");
template<>  MemberFuncThunk<const Action<CTFBot> *, bool,             const INextBot *>                                              ft_Action_IsAbleToBlockMovementOf<CTFBot>(            "Action<CTFBot>::IsAbleToBlockMovementOf");
template<>  MemberFuncThunk<      Action<CTFBot> *, Action<CTFBot> *, CTFBot *, Behavior<CTFBot> *, AR>                              ft_Action_ApplyResult<CTFBot>(                        "Action<CTFBot>::ApplyResult");
template<>  MemberFuncThunk<      Action<CTFBot> *, void,             CTFBot *, Behavior<CTFBot> *, Action<CTFBot> *>                ft_Action_InvokeOnEnd<CTFBot>(                        "Action<CTFBot>::InvokeOnEnd");
template<>  MemberFuncThunk<      Action<CTFBot> *, AR,               CTFBot *, Behavior<CTFBot> *, Action<CTFBot> *>                ft_Action_InvokeOnResume<CTFBot>(                     "Action<CTFBot>::InvokeOnResume");
template<>  MemberFuncThunk<const Action<CTFBot> *, char *,           char[256], const Action<CTFBot> *>                             ft_Action_BuildDecoratedName<CTFBot>(                 "Action<CTFBot>::BuildDecoratedName [clone]");
//template<>  MemberFuncThunk<const Action<CTFBot> *, char *>                                                                          ft_Action_DebugString<CTFBot>(                        "Action<CTFBot>::DebugString [clone]");
template<>  MemberFuncThunk<const Action<CTFBot> *, void>                                                                            ft_Action_PrintStateToConsole<CTFBot>(                "Action<CTFBot>::PrintStateToConsole");

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
void PathFollower::Update(INextBot *nextbot)           { vt_PathFollower_Update                 (this, nextbot); }
void PathFollower::Invalidate()                        { vt_PathFollower_Invalidate             (this); }
void PathFollower::SetMinLookAheadDistance(float dist) { vt_PathFollower_SetMinLookAheadDistance(this, dist); }

/* ChasePath */
void ChasePath::Update(INextBot *bot, CBaseEntity *subject, const IPathCost &cost, Vector *pPredictedSubjectPos = nullptr) { vt_ChasePath_Update (this, bot, subject, cost, pPredictedSubjectPos); }

/* CTFBotPathCost */
float CTFBotPathCost::operator()(CNavArea *area1, CNavArea *area2, const CNavLadder *ladder, const CFuncElevator *elevator, float f1) const { return ft_CTFBotPathCost_op_func(this, area1, area2, ladder, elevator, f1); }

/* CZombiePathCost */
float CZombiePathCost::operator()(CNavArea *area1, CNavArea *area2, const CNavLadder *ladder, const CFuncElevator *elevator, float f1) const { return ft_CZombiePathCost_op_func(this, area1, area2, ladder, elevator, f1); }

/* Behavior<CTFBot> */
template<> INextBotEventResponder *Behavior<CTFBot>::FirstContainedResponder() const { return ft_Behavior_FirstContainedResponder(this); }

/* NextBotManager */
void NextBotManager::CollectAllBots(CUtlVector<INextBot *> *nextbots) {        ft_NextBotManager_CollectAllBots(this, nextbots); }
NextBotManager& TheNextBots()                                         { return ft_TheNextBots                  (); }

#endif