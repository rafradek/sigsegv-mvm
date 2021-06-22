/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_debug_sentry_placement("tf_bot_debug_sentry_placement", "0", FCVAR_CHEAT);
ConVar tf_bot_max_teleport_exit_travel_to_point("tf_bot_max_teleport_exit_travel_to_point", "2500", FCVAR_CHEAT,
	"In an offensive engineer bot's tele exit is farther from the point than this, destroy it");
ConVar tf_bot_min_teleport_travel("tf_bot_min_teleport_travel", "3000", FCVAR_CHEAT,
	"Minimum travel distance between teleporter entrance and exit before engineer bot will build one");


CTFBotEngineerMoveToBuild::CTFBotEngineerMoveToBuild()
{
}

CTFBotEngineerMoveToBuild::~CTFBotEngineerMoveToBuild()
{
}


const char *CTFBotEngineerMoveToBuild::GetName() const
{
	return "EngineerMoveToBuild";
}


ActionResult<CTFBot> CTFBotEngineerMoveToBuild::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	this->SelectBuildLocation(actor);
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerMoveToBuild::Update(CTFBot *actor, float dt)
{
	
	
	// TODO
	
	// CTFGameRules
	// 220 GetGameType
	// 2d0 IsInKothMode
	
	// CBaseObject
	// 568 DestroyObject
}


EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->SelectBuildLocation(actor);
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnStuck(CTFBot *actor)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotEngineerMoveToBuild::OnTerritoryLost(CTFBot *actor, int i1)
{
	// ct 483c .Start(0.2f)
	
	return EventDesiredResult<CTFBot>::Continue();
}


void CTFBotEngineerMoveToBuild::CollectBuildAreas(CTFBot *actor)
{
	// TODO
}

void CTFBotEngineerMoveToBuild::SelectBuildLocation(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	
	// 0034 = nullptr
	
	// 0038 = vec3_origin
	
	// TODO
}


bool CompareRangeToPoint(CTFNavArea *const *area1, CTFNavArea *const *area2)
{
	// TODO: static Vector(?) s_pointCentroid
	
	// TODO
}
