#include "mod.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"
#include "stub/gamerules.h"
#include "stub/tfentities.h"
#include "util/iterate.h"

#include "mod/ai/mvm_defender_bots/helpers.h"
#include "mod/ai/mvm_defender_bots/trackers.h"
#include "mod/ai/mvm_defender_bots/actions/defender.h"
#include "mod/ai/mvm_defender_bots/actions/attack_tank.h"
#include "mod/ai/mvm_defender_bots/actions/defend_gate.h"
#include "mod/ai/mvm_defender_bots/actions/collect_money.h"
#include "mod/ai/mvm_defender_bots/actions/mark_giant.h"
#include "mod/ai/mvm_defender_bots/actions/goto_upgrade_station.h"
#include "mod/ai/mvm_defender_bots/actions/purchase_upgrades.h"


#define TRACE_ENABLE 1
#define TRACE_TERSE  0
#include "util/trace.h"


namespace Mod::AI::MvM_Defender_Bots
{
	// TODO: is there ever any need to override/spoof TFGameRules()->GetGameType()?
	
	
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotTacticalMonitor_InitialContainedAction, CTFBot *actor)
	{
		if (TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() == TF_TEAM_RED) {
			return new CTFBotMvMDefender();
		}
		
		return DETOUR_MEMBER_CALL(actor);
	}
	
	
	// do we need to override actor->IsAllowedToPickUpFlag to false?
	// do we need to throw in some CTFBotDefendPoint for gate defense?
	
	
	DETOUR_DECL_MEMBER(float, CTFBot_GetTimeLeftToCapture)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			return FLT_MAX;
		}
		
		return DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_HandleCommand_JoinTeam, const char *team)
	{
		DETOUR_MEMBER_CALL(team);
		
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (TFGameRules()->IsMannVsMachineMode() && player->GetTeamNumber() == TF_TEAM_RED) {
			CTFBot *bot = ToTFBot(player);
			if (bot != nullptr) {
				(void)bot->GetAutoTeam(5);
			}
		}
	}
	
	
	/* ENHANCEMENT: make defender bots always aware of the bomb carrier(s) as
	 * well as tanks */
	void Enhancement_NotifyDefendersAboutBombs()
	{
#if 0
		std::set<CBaseEntity *> bombs;
		
		ForEachFlag([&](CCaptureFlag *flag, bool& done){
			CBaseEntity *owner = flag->GetOwnerEntity();
			if (owner == nullptr) return true;
			
			if (owner->GetTeamNumber() != TF_TEAM_BLUE) return true;
			
			CTFBot *carrier = ToTFBot(owner);
			if (carrier == nullptr) return true;
			
			bombs.insert(carrier);
			
			return true;
		});
		
		ForEachTank([&](CTFTankBoss *tank, bool& done){
			if (tank->GetTeamNumber() == TF_TEAM_BLUE) {
				bombs.insert(tank);
			}
			
			return true;
		});
		
		if (bombs.empty()) return;
		
		ForEachDefenderBot([&](CTFBot *bot, bool& done){
			if (!bot->IsAlive()) return true;
			
			IVision *vision = bot->GetVisionInterface();
			for (auto bomb : bombs) {
				vision->AddKnownEntity(bomb);
			}
			
			return true;
		});
#endif
	}
	
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotTacticalMonitor_OnCommandString, CTFBot *actor, const char *cmd)
	{
		#warning put me back!
	/*	if (V_stricmp(cmd, "gotoupgradestation") == 0) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotGoToUpgradeStation(), "Going to an upgrade station.");
		}
		if (V_stricmp(cmd, "purchaseupgrades") == 0) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotPurchaseUpgrades(), "Purchasing upgrades.");
		}*/
		
		if (V_stricmp(cmd, "collectmoney") == 0 && CTFBotCollectMoney::IsPossible(actor)) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotCollectMoney(), "Collecting money.");
		}
		
		return DETOUR_MEMBER_CALL(actor, cmd);
	}
	
	
	void GiveLoadout(CTFBot *bot)
	{
		switch (bot->GetPlayerClass()->GetClassIndex()) {
		case TF_CLASS_SCOUT:
			bot->AddItem("Mad Milk");
			bot->AddItem("The Fan O'War");
			break;
		case TF_CLASS_SOLDIER:
			bot->AddItem("The Buff Banner");
			break;
		case TF_CLASS_PYRO:
			bot->AddItem("The Flare Gun");
			break;
		case TF_CLASS_DEMOMAN:
			break;
		case TF_CLASS_HEAVYWEAPONS:
			bot->AddItem("The Sandvich");
			break;
		case TF_CLASS_ENGINEER:
			break;
		case TF_CLASS_MEDIC:
			bot->AddItem("The Crusader's Crossbow");
			bot->AddItem("The Kritzkrieg");
			bot->AddItem("The Ubersaw");
			break;
		case TF_CLASS_SNIPER:
			bot->AddItem("Jarate");
			break;
		case TF_CLASS_SPY:
			break;
		}
		
		// TODO: make them switch back to their primary weapon
		// otherwise, they'll stay on the most recently given item slot
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBot_Spawn)
	{
		DETOUR_MEMBER_CALL();
		
		auto bot = reinterpret_cast<CTFBot *>(this);
		if (bot->GetTeamNumber() == TF_TEAM_RED) {
			GiveLoadout(bot);
		}
	}
	
	
#if 0
	DETOUR_DECL_MEMBER(void, CTFTankBoss_TankBossThink)
	{
		static CountdownTimer ctNodes;
		
		if (ctNodes.IsElapsed()) {
			ctNodes.Start(0.5f);
			
			ForEachEntityByRTTI<CPathTrack>([](CPathTrack *node){
				NDebugOverlay::Box(node->GetAbsOrigin(), Vector(-10.0f, -10.0f, -10.0f), Vector(10.0f, 10.0f, 10.0f),
					0xff, 0xff, 0xff, 0x80, 0.5f);
				NDebugOverlay::EntityTextAtPosition(node->GetAbsOrigin(), 0, CFmtStrN<16>("#%d", i), 0.5f, 0xff, 0xff, 0xff, 0xff);
				
				CPathTrack *next = node->GetNext();
				if (next != nullptr) {
					NDebugOverlay::HorzArrow(node->GetAbsOrigin(), next->GetAbsOrigin(), 3.0f, 0xff, 0xff, 0xff, 0xff, true, 0.5f);
				}
			});
		}
		
		auto tank = reinterpret_cast<CTFTankBoss *>(this);
		
		NDebugOverlay::EntityText(ENTINDEX(tank), 0, CFmtStrN<256>("%.1f%%", GetTankProgress(tank) * 100.0f),
			gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
		
	//	NDebugOverlay::EntityText(ENTINDEX(tank), 0, CFmtStrN<256>("m_hCurrentNode:    #%d", ENTINDEX(*m_hCurrentNode)),
	//		gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
	//	NDebugOverlay::EntityText(ENTINDEX(tank), 1, CFmtStrN<256>("m_iCurrentNode:    %d", *m_iCurrentNode),
	//		gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
	//	NDebugOverlay::EntityText(ENTINDEX(tank), 2, CFmtStrN<256>("m_flTotalDistance: %.0f", *m_flTotalDistance),
	//		gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
	//	NDebugOverlay::EntityText(ENTINDEX(tank), 3, CFmtStrN<256>("m_NodeDists[i]:    %.0f", (*m_NodeDists)[*m_iCurrentNode]),
	//		gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
		
		DETOUR_MEMBER_CALL();
	}
#endif
	
	
	void AddQuirks_IsFakeClient(IMod *mod);
	void AddQuirks_IsBot(IMod *mod);
	void AddQuirks_IsBotOfType(IMod *mod);
	void AddQuirks_GetLastKnownArea(IMod *mod);
	void AddQuirks_MannVsMachineMode(IMod *mod);
	void AddQuirks_SteamID(IMod *mod);
	
	
	/* show team ownership of nav areas based on flag position and owner capture
	 * points */
	void DrawOverlay_Ownership(long frame)
	{
		if (frame % 3 != 0) return;
		
		// TODO: default ownership when there's no bomb is wrong
		// TODO: handle areas that have incursion value of -1.0f
		
		float flag_inc = 0.0f;
		
		CCaptureFlag *flag = TheFlagTracker.GetFrontFlag();
		if (flag != nullptr) {
			auto area = static_cast<CTFNavArea *>(TheNavMesh->GetNearestNavArea(flag));
			if (area != nullptr) {
				flag_inc = area->GetIncursionDistance(TF_TEAM_RED);
			}
		}
		
		for (auto area : (CUtlVector<CTFNavArea *>&)TheNavAreas) {
			float inc = area->GetIncursionDistance(TF_TEAM_RED);
			
			if (inc < 0.0f || area->HasTFAttributes(BLUE_SPAWN_ROOM)) {
				/* gray */
			//	area->DrawFilled(0x80, 0x80, 0x80, 0x00, 3 * gpGlobals->interval_per_tick, true, 0.0f);
				area->DrawFilled(0x80, 0x80, 0x80, 0x80, 3 * gpGlobals->interval_per_tick, true, 3.0f);
			} else {
				if (flag_inc > 0.0f && inc > flag_inc) {
					/* blue */
				//	area->DrawFilled(0x20, 0x20, 0xff, 0x00, 3 * gpGlobals->interval_per_tick, true, 0.0f);
					area->DrawFilled(0x20, 0x20, 0xff, 0x80, 3 * gpGlobals->interval_per_tick, true, 3.0f);
				} else {
					/* red */
				//	area->DrawFilled(0xff, 0x20, 0x20, 0x00, 3 * gpGlobals->interval_per_tick, true, 0.0f);
					area->DrawFilled(0xff, 0x20, 0x20, 0x80, 3 * gpGlobals->interval_per_tick, true, 3.0f);
				}
			}
		}
		
#if 0
		for (const auto& pair : TheFlagTracker.GetFlagInfos()) {
			CCaptureFlag *flag   = pair.first;
			const FlagInfo& info = pair.second;
			
			NDebugOverlay::EntityText(ENTINDEX(flag), 0, "FLAG", 3 * gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(flag), 2, "Hatch path dist:", 3 * gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(flag), 3, CFmtStrN<256>("%.0f HU", info.hatch_path_dist), 3 * gpGlobals->interval_per_tick, 0xff, 0xff, 0xff, 0xff);
		}
#endif
		
		// TODO: draw gates
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("AI:MvM_Defender_Bots")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_InitialContainedAction, "CTFBotTacticalMonitor::InitialContainedAction");
			
			AddQuirks_IsFakeClient(this);
			AddQuirks_IsBot(this);
			AddQuirks_IsBotOfType(this);
			AddQuirks_GetLastKnownArea(this);
			AddQuirks_MannVsMachineMode(this);
			AddQuirks_SteamID(this);
			
			/* QUIRK: bots won't stay in SeekAndDestroy because they always
			* believe the round timer is about to expire */
			MOD_ADD_DETOUR_MEMBER(CTFBot_GetTimeLeftToCapture, "CTFBot::GetTimeLeftToCapture");
			
			/* QUIRK: bots don't get money when they join */
			/* QUIRK: bots remember upgrades when first joining */
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_HandleCommand_JoinTeam, "CTFPlayer::HandleCommand_JoinTeam");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_OnCommandString, "CTFBotTacticalMonitor::OnCommandString");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_Spawn, "CTFBot::Spawn");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			++frame;
			
			if (frame % 22 == 0) {
				Enhancement_NotifyDefendersAboutBombs();
			}
			
			UpdateTrackers();
			
#if 0
			/* remove me */
			if (frame % 7 == 0) {
				CCaptureFlag *flag = GetClosestFlagToHatch();
				if (flag != nullptr) {
					CNavArea *area = TheNavMesh->GetNavArea(flag->GetAbsOrigin());
					if (area != nullptr) {
						float flag_dist = reinterpret_cast<CTFNavArea *>(area)->GetIncursionDistance(TF_TEAM_BLUE);
						
						constexpr float D_LOW  =   0.0f;
						constexpr float D_HIGH = 300.0f;
						
						for (auto area : (CUtlVector<CTFNavArea *>&)TheNavAreas) {
							float dist = area->GetIncursionDistance(TF_TEAM_BLUE);
							
							if (dist < flag_dist - D_LOW)  continue;
							if (dist > flag_dist + D_HIGH) continue;
							
							area->DrawFilled(0xff, 0xff, 0xff, 0x00, gpGlobals->interval_per_tick * 7, true, 0.0f);
							area->DrawFilled(0xff, 0xff, 0xff, 0x40, gpGlobals->interval_per_tick * 7, true, 0.0f);
						}
					}
				}
			}
#endif
			
#if 0
			if (frame % 22 == 0) {
				float inc_min = FLT_MAX;
				float inc_max = FLT_MIN;
				
				for (auto area : (CUtlVector<CTFNavArea *>&)TheNavAreas) {
					float inc = area->GetIncursionDistance(TF_TEAM_BLUE);
					if (inc < 0.0f) continue;
					
					if (inc < inc_min) inc_min = inc;
					if (inc > inc_max) inc_max = inc;
				}
				
				for (auto area : (CUtlVector<CTFNavArea *>&)TheNavAreas) {
					float val = area->GetIncursionDistance(TF_TEAM_BLUE);
					float rat = RemapValClamped(val, 0.0f, inc_max, 0.0f, 1.0f);
					
					int r;
					int g;
					int b;
					
					if (rat < 0.25f) {
						r = 0xff;
						g = RemapValClamped(rat, 0.00f, 0.25f, 0.0f, 255.0f);
						b = 0x00;
					} else if (rat < 0.50f) {
						r = RemapValClamped(rat, 0.25f, 0.50f, 255.0f, 0.0f);
						g = 0xff;
						b = 0x00;
					} else if (rat < 0.75f) {
						r = 0x00;
						g = 0xff;
						b = RemapValClamped(rat, 0.50f, 0.75f, 0.0f, 255.0f);
					} else {
						r = 0x00;
						g = RemapValClamped(rat, 0.75f, 1.00f, 255.0f, 0.0f);
						b = 0xff;
					}
					
					area->DrawFilled(r, g, b, 0x00, 0.33f, true, 0.0f);
					area->DrawFilled(r, g, b, 0x80, 0.33f, true, 0.0f);
					
					NDebugOverlay::EntityTextAtPosition(area->GetCenter(), 0,
						CFmtStrN<64>("%.1f%%", rat * 100.0f),
						0.33f, 0xff, 0xff, 0xff, 0xff);
				//	NDebugOverlay::EntityTextAtPosition(area->GetCenter(), 1,
				//		CFmtStrN<64>("%.0f HU", val),
				//		0.33f, 0xff, 0xff, 0xff, 0xff);
				}
			}
#endif
			
			DrawOverlay_Ownership(frame);
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_mvm_defender_bots", "0", FCVAR_NOTIFY,
		"Mod: make red TFBots in MvM do things other than stand around uselessly",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


// TODO: make all of this stuff work:
// - situationally use jars
// - situationally use sandviches in a way that isn't stupid
// - kritzkrieg
// - fan-o-war
// - if ShouldHurry and !ShouldAttack
//   - and bot has a "go faster" item of some sort (GRU, powerjack, etc)
//   - then use it at that time
// - for all opportunistically used items:
//   - add a new callback, "ShouldUseSituationalItem", so we can control when they're used better
// - DR spy?
// - spy stab giants repeatedly in an intelligent manner
// - phlogistinator


// TODO: make bots try to go to dispensers when they need health or ammo


// TODO: money and respecs when bots join
// need to call CTFPlayer::GetAutoTeam(5) and do nothing with the result
// (from CTFPlayer::HandleCommand_JoinTeam, if MvM mode is active and is a TFBot and is on team 2)


// should maybe warn with a stack trace whenever we push and the count was already at least 1,
// to avoid some surprise nested conditions...


// BUG: medic bots getting stuck in FetchFlag somehow

// TODO: bomb carrier should always be known to defender bots
// TODO: bomb carrier should be highest priority?
// TODO: same stuff for the tank

// TODO: check vision update frequency

// TODO: refill bots' ammo on wave start or end


// TODO: bots need to automatically always be readied-up
// TODO: give medic bots automatic Attributes ProjectileShield so they'll actually use it
// TODO: probably should make red engie bots ignore sentry hints entirely

// TODO: make sure red bots aren't being erroneously affected by:
// - func_nav_avoid
// - tags (including from CTriggerBotTag)

// wishlist:
// - bots buy upgrades (lol...)
//   - make them go to an upgrade station on first spawn and between waves
//   - give them fixed rough upgrade paths
// - auto-equip certain meta-ish items (to the extent that bots can actually use them)
// - scouts track down money?
//   - create new behavior for this that makes them path toward currency packs
//   - whenever a currency pack spawns, have TacticalMonitor suspend for this
//     - or maybe only do this when they run within a certain radius of a currency pack
//     - or maybe only when they see one (enable them as known entities etc)
//   - blinking packs are highest priority, red packs are lowest priority
//   - also prioritize larger packs
// - medics attempt to heal reanimators
// - medics use kritzkrieg intelligently
// - buy back into game if it's urgent
// - spy prioritization for giants
// - spy logic for using sapper on enemies
// - teach bots how to use reverse path on two-way teles


/* references to MvM gamemode / bots that may need some overriding:

FL_FAKECLIENT
=============
TODO

0x548 CBasePlayer::IsFakeClient
===============================
_ZN12CTFGameRules17IsSpawnPointValidEP11CBaseEntityP11CBasePlayerb21PlayerTeamSpawnMode_t

0x6ec CBasePlayer::IsBot
========================
_ZN11CBaseObject18FindSnapToBuildPosEPS_
_ZN11CBaseObject18FindSnapToBuildPosEPS_
_ZN12CTFGameRules16ChangePlayerNameEP9CTFPlayerPKc
_ZN13CObjectSapper5SpawnEv (OF_ALLOW_REPEAT_PLACEMENT)
_ZN13CTFWeaponBase20ApplyOnHitAttributesEP11CBaseEntityP9CTFPlayerRK15CTakeDamageInfo
_ZN14CWeaponMedigun18FindAndHealTargetsEv
_ZN18CPopulationManager8ResetMapEv
_ZN9CTFPlayer20GetMannVsMachineTeamEi
_ZN9CTFPlayer22HandleCommand_JoinTeamEPKc
_ZN9CTFPlayer22HandleCommand_JoinTeamEPKc

0x6f0 CBasePlayer::IsBotOfType
==============================
_ZN14CTriggerBotTag5TouchEP11CBaseEntity
_ZN18CTraceFilterObject15ShouldHitEntityEP13IHandleEntityi
_ZN18CTraceFilterObject15ShouldHitEntityEP13IHandleEntityi
_ZN18CTraceFilterObject15ShouldHitEntityEP13IHandleEntityi
_ZN25CTFBotAttackFlagDefenders6UpdateEP6CTFBotf
_ZNK12CFuncNavCost14IsApplicableToEP20CBaseCombatCharacter
_ZNK14CSpawnLocation15SelectSpawnAreaEv

0x6f4 CBasePlayer::GetBotType
=============================
no references

CTFGameRules::IsPVEModeActive
=============================
none left

CTFGameRules::m_bPlayingMannVsMachine
=====================================
_ZN12CTFBotAttack6UpdateEP6CTFBotf (MvM uses ChasePath instead of Path)
_ZN25CTFBotAttackFlagDefenders6UpdateEP6CTFBotf
_ZN17CTFBotDeliverFlag5OnEndEP6CTFBotP6ActionIS0_E
_ZN17CTFBotDeliverFlag9OnContactEP6CTFBotP11CBaseEntityP10CGameTrace
_ZN17CTFBotDeliverFlag15UpgradeOverTimeEP6CTFBot
_ZN17CTFBotDeliverFlag6UpdateEP6CTFBotf
_ZN15CTFBotFetchFlag6UpdateEP6CTFBotf
_ZN15CTFBotFetchFlag6UpdateEP6CTFBotf
_ZN15CTFBotFetchFlag6UpdateEP6CTFBotf
_ZN24CTeamplayRoundBasedRules17CheckRestartRoundEv
_ZN24CTeamplayRoundBasedRules20State_Think_TEAM_WINEv
_ZN24CTeamplayRoundBasedRules17CheckReadyRestartEv
_ZN24CTeamplayRoundBasedRules20State_Enter_PREROUNDEv
_ZN24CTeamplayRoundBasedRules34GetPotentialPlayersLobbyPlayerInfoER10CUtlVectorI17LobbyPlayerInfo_t10CUtlMemoryIS1_iEEb
_ZN24CTeamplayRoundBasedRules24AreLobbyPlayersConnectedEv
_ZN24CTeamplayRoundBasedRules26AreLobbyPlayersOnTeamReadyEi
_ZN18CTraceFilterObject15ShouldHitEntityEP13IHandleEntityi
_ZN18CTraceFilterObject15ShouldHitEntityEP13IHandleEntityi
_ZN12CTFGameRules17CheckRespawnWavesEv
_ZNK15CTFPlayerShared29CanRecieveMedigunChargeEffectE20medigun_charge_types
*/
