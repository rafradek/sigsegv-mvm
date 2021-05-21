#include "mod/ai/mvm_defender_bots/trackers/flags.h"
#include "mod/ai/mvm_defender_bots/helpers.h"
#include "stub/tfbot.h"
#include "re/path.h"


namespace Mod::AI::MvM_Defender_Bots
{
	/* for use when we don't have an actual bot in mind but still want to figure
	 * out what the flag carrier's path cost would roughly be */
	class HypotheticalFlagCarrierPathCost : public IPathCost
	{
	public:
		HypotheticalFlagCarrierPathCost(RouteType rtype, int team) :
			m_iRouteType(rtype), m_iTeam(team),
			m_flStepHeight(18.0f), m_flMaxJumpHeight(72.0f), m_flDeathDropHeight(1000.0f) {}
		
		virtual float operator()(CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length) const
		{
			CTFNavArea *tfArea = static_cast<CTFNavArea *>(area);
			
			if (fromArea == nullptr) return 0.0f;
			
			if (!this->IsAreaTraversable(tfArea)) return -1.0f;
			
			float dist;
			if (length != 0.0f) {
				dist = length;
			} else {
				dist = (area->GetCenter() - fromArea->GetCenter()).Length();
			}
			
			float cost = dist;
			
			float delta_z = fromArea->ComputeAdjacentConnectionHeightChange(area);
			if (delta_z >= this->m_flStepHeight) {
				if (delta_z >= this->m_flMaxJumpHeight) return -1.0f;
				
				cost *= 2;
			} else {
				if (delta_z < -this->m_flDeathDropHeight) return -1.0f;
			}
			
			if (this->m_iRouteType == SAFEST_ROUTE) {
				if (tfArea->IsInCombat()) {
					cost *= 4.0f * tfArea->GetCombatIntensity();
				}
				
				if ((this->m_iTeam == TF_TEAM_RED  && tfArea->HasTFAttributes(BLUE_SENTRY)) ||
					(this->m_iTeam == TF_TEAM_BLUE && tfArea->HasTFAttributes(RED_SENTRY))) {
					cost *= 5.0f;
				}
			}
			
			if (area->HasAttributes(NAV_MESH_FUNC_COST)) {
				cost *= ComputeFuncNavCost(tfArea);
			}
			
			return fromArea->GetCostSoFar() + cost;
		}
		
	private:
		/* based on CTFBotLocomotion::IsAreaTraversable */
		bool IsAreaTraversable(CTFNavArea *area) const
		{
			if (area->IsBlocked(this->m_iTeam)) return false;
			
			if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() == this->m_iTeam) {
				return true;
			} else {
				TFNavAttributeType attr = area->GetTFAttributes();
				
				if ((attr & RED_SPAWN_ROOM)  != 0 && this->m_iTeam == TF_TEAM_BLUE) return false;
				if ((attr & BLUE_SPAWN_ROOM) != 0 && this->m_iTeam == TF_TEAM_RED)  return false;
				
				return true;
			}
		}
		
		/* based on CNavArea::ComputeFuncNavCost */
		float ComputeFuncNavCost(CTFNavArea *area) const
		{
			float funcCost = 1.0f;
			
			for (int i = 0; i < area->m_funcNavCostVector->Count(); ++i) {
				if (area->m_funcNavCostVector[i] != nullptr) {
					funcCost *= GetCostMultiplier(area->m_funcNavCostVector[i]);
				}
			}
			
			return funcCost;
		}
		
		/* based on CFuncNavCost::GetCostMultiplier */
		float GetCostMultiplier(CFuncNavCost *cost) const
		{
			if (rtti_cast<CFuncNavAvoid *>(cost) != nullptr) {
				if (IsApplicableTo(cost)) return 25.0f;
			} else if (rtti_cast<CFuncNavPrefer *>(cost) != nullptr) {
				if (IsApplicableTo(cost)) return 0.04f;
			}
			
			return 1.0f;
		}
		
		/* based on CFuncNavCost::IsApplicableTo */
		bool IsApplicableTo(CFuncNavCost *cost) const
		{
			/* we're only checking bomb_carrier stuff because flag carriers ignore
			 * all other tags */
			
			if (cost->HasTag("bomb_carrier")) return true;
			
			// TODO: custom bomb_carrier tags
			
			return false;
		}
		
		RouteType m_iRouteType;
		int m_iTeam;
		float m_flStepHeight;
		float m_flMaxJumpHeight;
		float m_flDeathDropHeight;
	};
	
	
	void ForEachFlag(const std::function<void(CCaptureFlag *, bool&)>& functor)
	{
		for (int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i) {
			auto flag = rtti_scast<CCaptureFlag *>(ICaptureFlagAutoList::AutoList()[i]);
			if (flag == nullptr) continue;
			
			bool done = false;
			functor(flag, done);
			if (done) break;
		}
	}
	
	
	void FlagTracker::Update()
	{
		CCaptureZone *zone = GetCaptureZone();
		if (zone == nullptr) {
			Warning("FlagTracker::Update: GetCaptureZone() returned nullptr\n");
			return;
		}
		
		CNavArea *area = TheNavMesh->GetNavArea(zone->WorldSpaceCenter());
		if (area == nullptr) {
			Warning("FlagTracker::Update: Can't find nav area for capture zone\n");
			return;
		}
		
		HypotheticalFlagCarrierPathCost cost_func(FASTEST_ROUTE, TF_TEAM_BLUE);
		
		this->m_FlagInfos.clear();
		
		ForEachFlag([&](CCaptureFlag *flag, bool& done){
			if (flag->IsDisabled()) return;
			
			if (flag->GetTeamNumber() != TF_TEAM_BLUE) return;
			
			Vector pos;
			CTFBot *carrier = ToTFBot(flag->GetOwnerEntity());
			if (carrier != nullptr) {
				pos = carrier->GetAbsOrigin();
			} else {
				pos = flag->GetAbsOrigin();
			}
			
			auto& info = this->m_FlagInfos[flag];
			info.hatch_path_dist = NavAreaTravelDistance(TheNavMesh->GetNavArea(pos), area, cost_func);
		});
	}
	
	
	CCaptureFlag *FlagTracker::GetFrontFlag() const
	{
		CCaptureFlag *best_flag = nullptr;
		float best_dist = FLT_MAX;
		
		for (const auto& pair : this->m_FlagInfos) {
			CCaptureFlag *flag = pair.first;
			const FlagInfo& info = pair.second;
			
			if (flag == nullptr) continue;
			if (flag->IsHome()) continue;
			if (info.hatch_path_dist < 0.0f) continue;
			
			if (info.hatch_path_dist < best_dist) {
				best_dist = info.hatch_path_dist;
				best_flag = flag;
			}
		}
		
		return best_flag;
	}
}


#if 0
inline CCaptureFlag *GetClosestFlagToHatch()
{
	CCaptureZone *zone = GetCaptureZone();
	if (zone == nullptr) {
		Warning("GetClosestFlagToHatch: GetCaptureZone() returned nullptr\n");
		return nullptr;
	}
	
	CNavArea *area = TheNavMesh->GetNavArea(zone->WorldSpaceCenter());
	if (area == nullptr) {
		Warning("GetClosestFlagToHatch: Can't find nav area for capture zone\n");
		return nullptr;
	}
	
	HypotheticalFlagCarrierPathCost cost_func(FASTEST_ROUTE, TF_TEAM_BLUE);
	
	CCaptureFlag *closest = nullptr;
	float min_dist = FLT_MAX;
	
	ForEachFlag([&](CCaptureFlag *flag, bool& done){
		if (flag->IsDisabled()) return;
		
		if (flag->GetTeamNumber() != TF_TEAM_BLUE) return;
		
		Vector pos;
		
		CTFBot *carrier = ToTFBot(flag->GetOwnerEntity());
		if (carrier != nullptr) {
			pos = carrier->GetAbsOrigin();
		} else {
			pos = flag->GetAbsOrigin();
		}
		
		float dist = NavAreaTravelDistance(TheNavMesh->GetNavArea(pos), area, cost_func);
		if (dist == -1.0f) return;
		
		NDebugOverlay::EntityText(ENTINDEX(flag), 0, CFmtStrN<16>("dist: %.1f", dist), 3.0f, 0xff, 0xff, 0xff, 0xff);
		
		if (dist < min_dist) {
			min_dist = dist;
			closest = flag;
		}
	});
	
	return closest;
}
#endif
