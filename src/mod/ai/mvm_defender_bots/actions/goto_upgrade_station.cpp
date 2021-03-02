#include "mod/ai/mvm_defender_bots/actions/goto_upgrade_station.h"
#include "mod/ai/mvm_defender_bots/actions/purchase_upgrades.h"
#include "stub/gamerules.h"
#include "util/iterate.h"

#include <random>


namespace Mod::AI::MvM_Defender_Bots
{
	bool WouldPlayerAtLocationTouchTrigger(const Vector& where, CBasePlayer *player, CBaseEntity *trigger)
	{
		Ray_t ray;
		ray.Init(where, where, VEC_HULL_MIN_SCALED(player), VEC_HULL_MAX_SCALED(player));
		
		trace_t tr;
		enginetrace->ClipRayToEntity(ray, CONTENTS_SOLID, reinterpret_cast<IHandleEntity *>(trigger), &tr);
		
		return ((tr.contents & CONTENTS_SOLID) != 0);
	}
	
	
	struct NavSubPoint
	{
		NavSubPoint(CNavArea *area, const Vector& subpoint) :
			area(area), subpoint(subpoint) {}
		
		CNavArea *area;
		Vector subpoint;
	};
	
	struct UpgradeStationInfo
	{
		UpgradeStationInfo(CUpgrades *station, CTFBot *actor, CNavArea *start_area, CTFBotPathCost& cost_func) :
			station(station)
		{
			Extent extent;
			extent.Init(station);
			
			CUtlVector<CTFNavArea *> overlapping;
			TheNavMesh->CollectAreasOverlappingExtent(extent, &overlapping);
			
			FOR_EACH_VEC(overlapping, i) {
				auto area = overlapping[i];
				
				Extent ext_nav;
				area->GetExtent(&ext_nav);
				
				constexpr float bloat = 0.0f;
				Vector bloat_mins(-bloat, -bloat, -bloat);
				Vector bloat_maxs( bloat,  bloat,  bloat);
				
				Extent ext_vol = ext_nav;
				ext_vol.lo += (VEC_HULL_MIN_SCALED(actor) + bloat_mins);
				ext_vol.hi += (VEC_HULL_MAX_SCALED(actor) + bloat_maxs);
				
				Vector center = (ext_vol.hi + ext_vol.lo) * 0.5f;
				
				Vector mins = -(ext_vol.hi - center);
				Vector maxs =  (ext_vol.hi - center);
				
				Ray_t ray;
				ray.Init(center, center, mins, maxs);
				
				trace_t tr;
				enginetrace->ClipRayToEntity(ray, CONTENTS_SOLID, reinterpret_cast<IHandleEntity *>(station), &tr);
				
				if ((tr.contents & CONTENTS_SOLID) == 0) continue;
				
				constexpr float unit_size = 25.0f;
				
				int units_x = (int)(ext_nav.SizeX() / unit_size);
				int units_y = (int)(ext_nav.SizeY() / unit_size);
				
				std::vector<Vector> spots_edges;
				std::vector<Vector> spots_middle;
				
				for (int i = 0; i <= units_x; ++i) {
					for (int j = 0; j <= units_y; ++j) {
						float x = i * (ext_nav.SizeX() / units_x);
						float y = j * (ext_nav.SizeY() / units_y);
						
						// TODO: use area->GetZ for this part
						float z = ext_nav.lo.z;
						
						if (i == 0 || j == 0 || i == units_x || j == units_y) {
							spots_edges.emplace_back(ext_nav.lo.x + x, ext_nav.lo.y + y, z);
						} else {
							spots_middle.emplace_back(ext_nav.lo.x + x, ext_nav.lo.y + y, z);
						}
					}
				}
				
				std::random_shuffle(spots_edges.begin(),  spots_edges.end());
				std::random_shuffle(spots_middle.begin(), spots_middle.end());
				
#if 0
				int idx = 0;
				for (const auto& spot : spots_edges) {
					bool would_touch = WouldPlayerAtLocationTouchTrigger(spot, actor, station);
					NDebugOverlay::Box(spot, Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f),
						(would_touch ? 0x00 : 0xff),
						(would_touch ? 0xff : 0x00),
						0x00, 0xff, 60.0f);
					++idx;
				}
				
				idx = 0;
				for (const auto& spot : spots_middle) {
					bool would_touch = WouldPlayerAtLocationTouchTrigger(spot, actor, station);
					NDebugOverlay::Box(spot, Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f),
						(would_touch ? 0x00 : 0xff),
						(would_touch ? 0xff : 0x00),
						0x00, 0xff, 60.0f);
					++idx;
				}
#endif
				
				bool found_spot = false;
				
				for (const auto& spot : spots_edges) {
					if (found_spot) break;
					
					if (WouldPlayerAtLocationTouchTrigger(spot, actor, station)) {
					//	NDebugOverlay::Box(spot, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), 0x00, 0xff, 0x00, 0xff, 3600.0f);
						this->spots.emplace_back(area, spot);
						found_spot = true;
					}
				}
				
				for (const auto& spot : spots_middle) {
					if (found_spot) break;
					
					if (WouldPlayerAtLocationTouchTrigger(spot, actor, station)) {
					//	NDebugOverlay::Box(spot, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), 0xff, 0x00, 0x00, 0xff, 3600.0f);
						this->spots.emplace_back(area, spot);
						found_spot = true;
					}
				}
				
				if (found_spot) {
					NDebugOverlay::Box(ext_nav.lo, vec3_origin, (ext_nav.hi - ext_nav.lo), 0xff, 0xff, 0xff, 0x80, 60.0f);
				}
			}
			
//			DevMsg("UpgradeStationInfo(#%d): %u areas\n", ENTINDEX(station), this->areas.size());
			
			float min_dist = FLT_MAX;
			for (const auto& spot : this->spots) {
				float nav_dist = NavAreaTravelDistance(start_area, spot.area, cost_func);
				if (nav_dist == -1.0f) continue;
				
				min_dist = Min(min_dist, nav_dist);
			}
			this->dist = min_dist;
			
//			DevMsg("UpgradeStationInfo(#%d): min distance: %f\n", ENTINDEX(station), this->dist);
		}
		
		CUpgrades *station;
		std::vector<NavSubPoint> spots;
		float dist;
	};
	
	
	ActionResult<CTFBot> CTFBotGoToUpgradeStation::OnStart(CTFBot *actor, Action<CTFBot> *action)
	{
		this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
		
		if (!this->FindClosestUpgradeStation()) {
			return ActionResult<CTFBot>::Done("Couldn't find a suitable upgrade station.");
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	ActionResult<CTFBot> CTFBotGoToUpgradeStation::Update(CTFBot *actor, float dt)
	{
		if (this->m_hUpgradeStation == nullptr) {
			return ActionResult<CTFBot>::Done("Upgrade station is invalid.");
		}
		
		if (actor->m_Shared->m_bInUpgradeZone) {
			return ActionResult<CTFBot>::ChangeTo(new CTFBotPurchaseUpgrades(), "Reached upgrade station; buying upgrades.");
		}
		
		auto nextbot = actor->MyNextBotPointer();
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			
			CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
			this->m_PathFollower.Compute(nextbot, this->m_vecUpgradeStation, cost_func, 0.0f, true);
			
			// TODO: handle Path::Compute failure
		}
		
		this->m_PathFollower.Update(nextbot);
		
		return ActionResult<CTFBot>::Continue();
	}
	
	void CTFBotGoToUpgradeStation::OnEnd(CTFBot *actor, Action<CTFBot> *action)
	{
		
	}
	
	
	EventDesiredResult<CTFBot> CTFBotGoToUpgradeStation::OnMoveToSuccess(CTFBot *actor, const Path *path)
	{
		DevMsg("CTFBotGoToUpgradeStation::OnMoveToSuccess(#%d)\n", ENTINDEX(actor));
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	EventDesiredResult<CTFBot> CTFBotGoToUpgradeStation::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
	{
		DevMsg("CTFBotGoToUpgradeStation::OnMoveToFailure(#%d, %d)\n", ENTINDEX(actor), (int)fail);
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	EventDesiredResult<CTFBot> CTFBotGoToUpgradeStation::OnStuck(CTFBot *actor)
	{
		return EventDesiredResult<CTFBot>::Done("Stuck! Giving up.");
	}
	
	
	bool CTFBotGoToUpgradeStation::FindClosestUpgradeStation()
	{
		CTFBotPathCost cost_func(this->GetActor(), FASTEST_ROUTE);
		CNavArea *start_area = this->GetActor()->GetLastKnownArea();
		
		std::vector<UpgradeStationInfo> stations;
		ForEachEntityByRTTI<CUpgrades>([&](CUpgrades *station){
			if (station->m_bDisabled) return;
			
//			DevMsg("Found func_upgradestation: #%d @ [ %.0f %.0f %.0f ]\n",
//				ENTINDEX(station),
//				station->WorldSpaceCenter().x,
//				station->WorldSpaceCenter().y,
//				station->WorldSpaceCenter().z);
			
			stations.emplace_back(station, this->GetActor(), start_area, cost_func);
		});
		
		if (stations.empty()) {
			DevMsg("FindClosestUpgradeStation: 0 stations\n");
			return false;
		}
		
//		for (const auto& station : stations) {
//			float dist_abs = (station.station->WorldSpaceCenter() - this->GetActor()->GetAbsOrigin()).Length();
//			float dist_nav = station.dist;
//			
//			DevMsg("Station #%d: abs:%.0f nav:%.0f\n",
//				ENTINDEX(station.station), dist_abs, dist_nav);
//		}
		
		auto it = std::min_element(stations.begin(), stations.end(), [](const UpgradeStationInfo& lhs, const UpgradeStationInfo& rhs){
			return (lhs.dist < rhs.dist);
		});
		if (it == stations.end()) {
			DevMsg("FindClosestUpgradeStation: min_element failed\n");
			return false;
		}
		const UpgradeStationInfo& closest = *it;
		
		if (closest.spots.empty()) {
			DevMsg("FindClosestUpgradeStation: closest station has 0 spots\n");
			return false;
		}
		this->m_hUpgradeStation = closest.station;
		
		const NavSubPoint& spot = closest.spots[RandomInt(0, closest.spots.size() - 1)];
		this->m_vecUpgradeStation = spot.subpoint;
	//	random->GetClosestPointOnArea(closest.station->WorldSpaceCenter(), &this->m_vecUpgradeStation);
		
//		NDebugOverlay::Box(this->m_vecUpgradeStation, Vector(-10.0f, -10.0f, -10.0f), Vector(10.0f, 10.0f, 10.0f), 0xff, 0xff, 0xff, 0xff, 3600.0f);
		
//		DevMsg("Closest func_upgradestation: #%d @ [ %.0f %.0f %.0f ]\n",
//			ENTINDEX(station()),
//			station()->WorldSpaceCenter().x,
//			station()->WorldSpaceCenter().y,
//			station()->WorldSpaceCenter().z);
		
		return true;
	}
}
