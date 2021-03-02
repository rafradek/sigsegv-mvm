#include "mod.h"
#include "mod/ai/mvm_defender_bots/collectmoney.h"
#include "mod/ai/mvm_defender_bots/helpers.h"
#include "stub/gamerules.h"
#include "util/scope.h"


namespace Mod::AI::MvM_Defender_Bots
{
	struct CreditInfo
	{
		std::map<CHandle<CTFBot>, float> t_lastseen;
		
		bool WasEverSeen() const
		{
			return !t_lastseen.empty();
		}
		
		float GetTimeSinceLastSeen() const
		{
			float t_least = FLT_MAX;
			
			for (const auto& pair : t_lastseen) {
				t_least = Min(t_least, gpGlobals->curtime - pair.second);
			}
			
			return t_least;
		}
	};
	
	
	class CreditTracker
	{
	public:
		static void Update()
		{
			constexpr int UPDATE_INTERVAL = 7;
			
			static long frame = 0;
			++frame;
			
			if (frame % UPDATE_INTERVAL != 0) return;
			
			/* remove entries for obsolete currency packs */
			for (auto it = s_CreditInfos.begin(); it != s_CreditInfos.end(); ) {
				if ((*it).first == nullptr) {
					it = s_CreditInfos.erase(it);
				} else {
					++it;
				}
			}
			
			/* add/update entries for currently visible currency packs */
			ForEachCurrencyPack([](CCurrencyPack *pack, bool& done1){
				ForEachDefenderBot([=](CTFBot *bot, bool& done2){
					if (!bot->IsAlive()) return true;
					
					if (bot->GetVisionInterface()->IsAbleToSee(pack, IVision::FieldOfViewCheckType::USE_FOV, nullptr)) {
						s_CreditInfos[pack].t_lastseen[bot] = gpGlobals->curtime;
					}
					
					return true;
				});
				
				return true;
			});
			
			for (const auto& pair1 : s_CreditInfos) {
				CCurrencyPack *pack    = pair1.first;
				const CreditInfo& info = pair1.second;
				
				int line = 0;
				for (const auto& pair2 : info.t_lastseen) {
					CTFBot *bot      = pair2.first;
					float t_lastseen = pair2.second;
					
					NDebugOverlay::EntityText(ENTINDEX(pack), line, CFmtStrN<256>("%-20s %.1f", bot->GetPlayerName(), gpGlobals->curtime - t_lastseen),
						UPDATE_INTERVAL * gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
					
					bool recent = (gpGlobals->curtime - t_lastseen < 0.2f);
					NDebugOverlay::HorzArrow(bot->EyePosition(), pack->GetAbsOrigin(), 1.0f,
						(recent ? 0xff : 0x80),
						(recent ? 0xff : 0x80),
						(recent ? 0xff : 0x80),
						(recent ? 0xff : 0x80),
						true, UPDATE_INTERVAL * gpGlobals->interval_per_tick);
					
					++line;
				}
			}
		}
		
	private:
		static std::map<CHandle<CCurrencyPack>, CreditInfo> s_CreditInfos;
		
	public:
		static decltype(s_CreditInfos)& GetCreditInfos() { return s_CreditInfos; }
	};
	std::map<CHandle<CCurrencyPack>, CreditInfo> CreditTracker::s_CreditInfos;
	
	
	void UpdateVisibleCredits()
	{
		CreditTracker::Update();
	}
	
	
	float GetTimeUntilRemoved(CTFPowerup *powerup)
	{
		return (powerup->GetNextThink("PowerupRemoveThink") - gpGlobals->curtime);
	}
	
	
	bool CompareCurrencyPacks(CCurrencyPack *lhs, CCurrencyPack *rhs)
	{
		float t_remove_lhs = GetTimeUntilRemoved(lhs);
		float t_remove_rhs = GetTimeUntilRemoved(rhs);
		
		bool is_red_lhs = lhs->IsDistributed();
		bool is_red_rhs = rhs->IsDistributed();
		
		int value_lhs = lhs->GetAmount();
		int value_rhs = rhs->GetAmount();
		
		//float dist_lhs = 
	}
	
	
	// TODO: prioritize currency packs based on these criteria:
	// - age (0~30 seconds)
	// - type (green vs red)
	// - credit value
	// - distance
	
	
	ActionResult<CTFBot> CTFBotCollectMoney::OnStart(CTFBot *actor, Action<CTFBot> *action)
	{
		this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
		
		this->SelectCurrencyPack();
		
		return ActionResult<CTFBot>::Continue();
	}
	
	ActionResult<CTFBot> CTFBotCollectMoney::Update(CTFBot *actor, float dt)
	{
		if (this->m_hCurrencyPack == nullptr) {
			return ActionResult<CTFBot>::Done("Currency pack is gone.");
		}
		
		auto nextbot = actor->MyNextBotPointer();
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(0.4f, 0.6f));
			
			CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
			this->m_PathFollower.Compute(nextbot, this->m_hCurrencyPack->GetAbsOrigin(), cost_func, 0.0f, true);
			
			// TODO: handle Path::Compute failure
		}
		
		this->m_PathFollower.Update(nextbot);
		
		return ActionResult<CTFBot>::Continue();
	}
	
	void CTFBotCollectMoney::OnEnd(CTFBot *actor, Action<CTFBot> *action)
	{
		
	}
	
	
//	EventDesiredResult<CTFBot> CTFBotCollectMoney::OnMoveToSuccess(CTFBot *actor, const Path *path)
//	{
//		DevMsg("CTFBotCollectMoney::OnMoveToSuccess(#%d)\n", ENTINDEX(actor));
//		return EventDesiredResult<CTFBot>::Continue();
//	}
//	
//	EventDesiredResult<CTFBot> CTFBotCollectMoney::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
//	{
//		DevMsg("CTFBotCollectMoney::OnMoveToFailure(#%d, %d)\n", ENTINDEX(actor), (int)fail);
//		return EventDesiredResult<CTFBot>::Continue();
//	}
//	
//	EventDesiredResult<CTFBot> CTFBotCollectMoney::OnStuck(CTFBot *actor)
//	{
//		return EventDesiredResult<CTFBot>::Done("Stuck! Giving up.");
//	}
	
	
	bool CTFBotCollectMoney::IsPossible(CTFBot *actor)
	{
		if (!TFGameRules()->IsMannVsMachineMode()) return false;
		if (!actor->IsPlayerClass(TF_CLASS_SCOUT)) return false;
		
		if (!CreditTracker::GetCreditInfos().empty()) {
			return true;
		} else {
			return false;
		}
	}
	
	
	void CTFBotCollectMoney::SelectCurrencyPack()
	{
		for (const auto& pair : CreditTracker::GetCreditInfos()) {
			this->m_hCurrencyPack = pair.first;
		}
	}
	
	
	namespace CollectMoney
	{
#if 0
		RefCount rc_IVision_UpdateKnownEntities;
		DETOUR_DECL_MEMBER(void, IVision_UpdateKnownEntities)
		{
			SCOPED_INCREMENT(rc_IVision_UpdateKnownEntities);
			DETOUR_MEMBER_CALL(IVision_UpdateKnownEntities)();
		}
		
		DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
		{
			if (rc_IVision_UpdateKnownEntities == 0) {
				if (strcmp(ent->GetClassname(), "item_currencypack") == 0) {
					return true;
				}
			}
			
			return DETOUR_MEMBER_CALL(CTFBotVision_IsIgnored)(ent);
		}
		
		
		DETOUR_DECL_MEMBER(void, CTFBotVision_CollectPotentiallyVisibleEntities, CUtlVector<CBaseEntity *> *ents)
		{
			DETOUR_MEMBER_CALL(CTFBotVision_CollectPotentiallyVisibleEntities)(ents);
			
			IVision *vision = reinterpret_cast<IVision *>(this);
			
			if (vision->GetBot()->GetEntity()->GetTeamNumber() == TF_TEAM_RED) {
				for (int i = 0; i < ICurrencyPackAutoList::AutoList().Count(); ++i) {
					auto pack = rtti_cast<CCurrencyPack *>(ICurrencyPackAutoList::AutoList()[i]);
					if (pack == nullptr) continue;
					
					ents->AddToTail(pack);
				}
			}
		}
#endif
		
		
//		DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotTacticalMonitor_Update, CTFBot *actor, float dt)
//		{
//			if (CTFBotCollectMoney::IsPossible(actor)) {
//				return ActionResult<CTFBot>::SuspendFor(new CTFBotCollectMoney(), "Collecting money.");
//			}
//			
//			return DETOUR_MEMBER_CALL(CTFBotTacticalMonitor_Update)(actor, dt);
//		}
		
		
		#define AddDetour_Member(detour, addr) \
			mod->AddDetour(new CDetour(addr, GET_MEMBER_CALLBACK(detour), GET_MEMBER_INNERPTR(detour)))
		#define AddDetour_Static(detour, addr) \
			mod->AddDetour(new CDetour(addr, GET_STATIC_CALLBACK(detour), GET_STATIC_INNERPTR(detour)))
		
		void AddDetours(IMod *mod)
		{
//			AddDetour_Member(IVision_UpdateKnownEntities,                    "IVision::UpdateKnownEntities");
//			AddDetour_Member(CTFBotVision_IsIgnored,                         "CTFBotVision::IsIgnored");
//			AddDetour_Member(CTFBotVision_CollectPotentiallyVisibleEntities, "CTFBotVision::CollectPotentiallyVisibleEntities");
			
//			AddDetour_Member(CTFBotTacticalMonitor_Update, "CTFBotTacticalMonitor::Update");
		}
	}
	
	// TODO: need TacticalMonitor::Update hook
	// - should call CTFBotCollectMoney::IsPossible
	// - if is possible, should SuspendFor it
}
