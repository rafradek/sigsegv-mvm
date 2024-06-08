#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
//#include "util/backtrace.h"


namespace Mod::AI::MvM_Defender_Bots
{
	RefCount rc_Quirk_IsBot;
	DETOUR_DECL_MEMBER(bool, NextBotPlayer_CTFPlayer_IsBot)
	{
		auto player = reinterpret_cast<NextBotPlayer<CTFPlayer> *>(this);
		
		if (rc_Quirk_IsBot > 0) {
			if (TFGameRules()->IsMannVsMachineMode() && player->GetTeamNumber() == TF_TEAM_RED) {
			//	DevMsg("QUIRK: NextBotPlayer<CTFPlayer>::IsBot\n");
			//	BACKTRACE();
				
				return false;
			}
		}
		
		return true;
	//	return DETOUR_MEMBER_CALL();
	}
	
	
	#define AddDetour_Member(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_MEMBER_CALLBACK(detour), GET_MEMBER_INNERPTR(detour)))
	#define AddDetour_Static(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_STATIC_CALLBACK(detour), GET_STATIC_INNERPTR(detour)))
	
	#define AddQuirk_IsBot(addr) \
		mod->AddDetour(new CFuncCount(rc_Quirk_IsBot, addr))
	
	void AddQuirks_IsBot(IMod *mod)
	{
		AddDetour_Member(NextBotPlayer_CTFPlayer_IsBot, "NextBotPlayer<CTFPlayer>::IsBot");
		
		/* QUIRK: credit effects (health on scout etc) don't work */
		AddQuirk_IsBot("CCurrencyPack::MyTouch");
		
		/* QUIRK: red bots can instakill giants with backstabs */
		AddQuirk_IsBot("CTFKnife::GetMeleeDamage");
		
		/* QUIRK: red bots don't drop ammo boxes */
		AddQuirk_IsBot("CTFPlayer::ShouldDropAmmoPack");
		
		/* QUIRK: the game code tries to do robot stuff on red bots' deaths
		 * and doesn't drop revive markers */
		AddQuirk_IsBot("CTFPlayer::Event_Killed");
		
		/* QUIRK: red bots can't use robo sapper ability */
		AddQuirk_IsBot("CTFWeaponBuilder::StartBuilding");
		
		/* QUIRK: bots won't taunt other bots after killing them */
		AddQuirk_IsBot("CTFBotMainAction::OnOtherKilled");
		
		/* QUIRK: tf_populator_damage_multiplier not used for bot-vs-bot */
		AddQuirk_IsBot("CTFGameRules::ApplyOnDamageAliveModifyRules");
		
		/* QUIRK: red bots can place sappers repeatedly */
		AddQuirk_IsBot("CObjectSapper::Spawn");
		
		/* QUIRK: bots can have multiple active sappers */
		/* QUIRK: bots can build regardless of object count or cost */
		AddQuirk_IsBot("CTFPlayer::CanBuild");
		
		/* QUIRK: bots can't do any upgrade-related stuff */
		AddQuirk_IsBot("CTFPlayer::ManageRegularWeapons");
		AddQuirk_IsBot("CTFPlayer::ReapplyPlayerUpgrades");
		AddQuirk_IsBot("CTFPlayer::ReapplyItemUpgrades");
		AddQuirk_IsBot("CTFPlayer::RememberUpgrade");
		AddQuirk_IsBot("CTFPlayer::ForgetFirstUpgradeForItem");
		#warning ManageRegularWeapons may break canteen non-auto-equip
		
		/* QUIRK: bots are skipped in deciding whether to allow readying up */
		AddQuirk_IsBot("CTFGameRules::PlayerReadyStatus_HaveMinPlayersToEnable");
		
		/* QUIRK: bots are skipped in unreadying players when someone leaves */
		AddQuirk_IsBot("CTFGameRules::ClientDisconnected");
		
		/* QUIRK: bots are skipped when getting lobby members */
		// TODO: see if there is a replacement for this function after the lobby code reorganization
//		AddQuirk_IsBot("CTeamplayRoundBasedRules::GetAllPlayersLobbyInfo");
	}
}
