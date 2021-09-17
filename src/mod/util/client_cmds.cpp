#include "mod.h"
#include "stub/econ.h"
#include "stub/projectiles.h"
#include "stub/tfplayer.h"
#include "stub/tfbot.h"
#include "stub/tfweaponbase.h"
#include "stub/objects.h"
#include "stub/misc.h"
#include "stub/gamerules.h"
#include "stub/strings.h"
#include "stub/server.h"
#include "util/admin.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "util/iterate.h"

class IHasAttributes {

};

namespace Mod::Util::Client_Cmds
{
	// TODO: another version that allows setting a different player's scale...?
	void CC_SetPlayerScale(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ClientMsg(player, "[sig_setplayerscale] Usage:  sig_setplayerscale <scale>\n");
			return;
		}
		
		float scale = 1.0f;
		if (!StringToFloatStrict(args[1], scale)) {
			ClientMsg(player, "[sig_setplayerscale] Error: couldn't parse \"%s\" as a floating-point number.\n", args[1]);
			return;
		}
		
		player->SetModelScale(scale);
		
		ClientMsg(player, "[sig_setplayerscale] Set scale of player %s to %.2f.\n", player->GetPlayerName(), scale);
	}
	
	
	// TODO: another version that allows setting a different player's model...?
	void CC_SetPlayerModel(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ClientMsg(player, "[sig_setplayermodel] Usage:  sig_setplayermodel <model_path>\n");
			return;
		}
		
		const char *model_path = args[1];
		
		player->GetPlayerClass()->SetCustomModel(model_path, true);
		player->UpdateModel();
		
		ClientMsg(player, "[sig_setplayermodel] Set model of player %s to \"%s\".\n", player->GetPlayerName(), model_path);
	}
	

	// TODO: another version that allows resetting a different player's model...?
	void CC_ResetPlayerModel(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 1) {
			ClientMsg(player, "[sig_resetplayermodel] Usage:  sig_resetplayermodel\n");
			return;
		}
		
		player->GetPlayerClass()->SetCustomModel(nullptr, true);
		player->UpdateModel();
		
		ClientMsg(player, "[sig_resetplayermodel] Reset model of player %s to the default.\n", player->GetPlayerName());
	}
	
	
	void CC_UnEquip(CTFPlayer *player, const CCommand& args)
	{
		auto l_usage = [=]{
			ClientMsg(player, "[sig_unequip] Usage: any of the following:\n"
				"  sig_unequip <item_name>          | item names that include spaces need quotes\n"
				"  sig_unequip <item_def_index>     | item definition indexes can be found in the item schema\n"
				"  sig_unequip slot <slot_name>     | slot names are in the item schema (look for \"item_slot\")\n"
				"  sig_unequip slot <slot_number>   | slot numbers should only be used if you know what you're doing\n"
				"  sig_unequip region <region_name> | cosmetic equip regions are in the item schema (look for \"equip_regions_list\")\n"
				"  sig_unequip all                  | remove all equipped weapons, cosmetics, taunts, etc\n"
				"  sig_unequip help                 | get a list of valid slot names/numbers and equip regions\n");
		};
		
		if (args.ArgC() == 2) {
			// TODO: help
			if (FStrEq(args[1], "help")) {
				ClientMsg(player, "[sig_unequip] UNIMPLEMENTED: help\n");
				return;
			}
			
			bool all = FStrEq(args[1], "all");
			CTFItemDefinition *item_def = nullptr;
			
			if (!all) {
				/* attempt lookup first by item name, then by item definition index */
				auto item_def = rtti_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(args[1]));
				if (item_def == nullptr) {
					int idx = -1;
					if (StringToIntStrict(args[1], idx)) {
						item_def = FilterOutDefaultItemDef(rtti_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinition(idx)));
					}
				}
				
				if (item_def == nullptr) {
					ClientMsg(player, "[sig_unequip] Error: couldn't find any items in the item schema matching \"%s\"\n", args[1]);
					return;
				}
			}
			
			int n_weapons_removed = 0;
			int n_wearables_removed = 0;
			
			for (int i = player->WeaponCount() - 1; i >= 0; --i) {
				CBaseCombatWeapon *weapon = player->GetWeapon(i);
				if (weapon == nullptr) continue;
				
				CEconItemView *item_view = weapon->GetItem();
				if (item_view == nullptr) continue;
				
				if (all || item_view->GetItemDefIndex() == item_def->m_iItemDefIndex) {
					ClientMsg(player, "[sig_unequip] Unequipped weapon %s from slot %s\n", item_view->GetStaticData()->GetName(), GetLoadoutSlotName(item_view->GetStaticData()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex())));
					
					player->Weapon_Detach(weapon);
					weapon->Remove();
					
					++n_weapons_removed;
				}
			}
			
			for (int i = player->GetNumWearables() - 1; i >= 0; --i) {
				CEconWearable *wearable = player->GetWearable(i);
				if (wearable == nullptr) continue;
				
				CEconItemView *item_view = wearable->GetItem();
				if (item_view == nullptr) continue;
				
				if (all || item_view->GetItemDefIndex() == item_def->m_iItemDefIndex) {
					ClientMsg(player, "[sig_unequip] Unequipped cosmetic %s from slot %s\n", item_view->GetStaticData()->GetName(), GetLoadoutSlotName(item_view->GetStaticData()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex())));
					
					player->RemoveWearable(wearable);
					
					++n_wearables_removed;
				}
			}
			
			ClientMsg(player, "[sig_unequip] Unequipped %d weapons and %d cosmetics.\n", n_weapons_removed, n_wearables_removed);
			return;
		} else if (args.ArgC() == 3) {
			if (FStrEq(args[1], "slot")) {
				int slot = -1;
				if (StringToIntStrict(args[2], slot)) {
					if (!IsValidLoadoutSlotNumber(slot)) {
						ClientMsg(player, "[sig_unequip] Error: %s is not a valid loadout slot number\n", args[2]);
						return;
					}
				} else {
					slot = GetLoadoutSlotByName(args[2]);
					if (!IsValidLoadoutSlotNumber(slot)) {
						ClientMsg(player, "[sig_unequip] Error: %s is not a valid loadout slot name\n", args[2]);
						return;
					}
				}
				
				int n_weapons_removed = 0;
				int n_wearables_removed = 0;
				
				CEconEntity *econ_entity;
				do {
					econ_entity = nullptr;
					
					CEconItemView *item_view = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot(player, slot, &econ_entity);
					if (econ_entity != nullptr) {
						if (econ_entity->IsBaseCombatWeapon()) {
							auto weapon = rtti_cast<CBaseCombatWeapon *>(econ_entity);
							
							ClientMsg(player, "[sig_unequip] Unequipped weapon %s from slot %s\n", item_view->GetStaticData()->GetName(), GetLoadoutSlotName(slot));
							
							player->Weapon_Detach(weapon);
							weapon->Remove();
							
							++n_weapons_removed;
						} else if (econ_entity->IsWearable()) {
							auto wearable = rtti_cast<CEconWearable *>(econ_entity);
							
							ClientMsg(player, "[sig_unequip] Unequipped cosmetic %s from slot %s\n", item_view->GetStaticData()->GetName(), GetLoadoutSlotName(slot));
							
							player->RemoveWearable(wearable);
							
							++n_wearables_removed;
						} else {
							ClientMsg(player, "[sig_unequip] Unequipped unexpected entity with classname \"%s\" from slot %s\n", econ_entity->GetClassname(), GetLoadoutSlotName(slot));
							
							econ_entity->Remove();
						}
					}
				} while (econ_entity != nullptr);
				
				ClientMsg(player, "[sig_unequip] Unequipped %d weapons and %d cosmetics.\n", n_weapons_removed, n_wearables_removed);
				return;
			} else if (FStrEq(args[1], "region")) {
				// TODO: region_name
				ClientMsg(player, "[sig_unequip] UNIMPLEMENTED: region\n");
				return;
			} else {
				l_usage();
				return;
			}
		} else {
			l_usage();
			return;
		}
	}
	
	
	std::string DescribeCondDuration(float duration)
	{
		if (duration == -1.0f) {
			return "unlimited";
		}
		
		return CFmtStdStr("%.3f sec", duration);
	}
	
	std::string DescribeCondProvider(CBaseEntity *provider)
	{
		if (provider == nullptr) {
			return "none";
		}
		
		CTFPlayer *player = ToTFPlayer(provider);
		if (player != nullptr) {
			return CFmtStdStr("%s %s", (player->IsBot() ? "bot" : "player"), player->GetPlayerName());
		}
		
		auto obj = rtti_cast<CBaseObject *>(provider);
		if (obj != nullptr) {
			std::string str;
			switch (obj->GetType()) {
			case OBJ_DISPENSER:
				if (rtti_cast<CObjectCartDispenser *>(obj) != nullptr) {
					str = "payload cart dispenser";
				} else {
					str = "dispenser";
				}
				break;
			case OBJ_TELEPORTER:
				if (obj->GetObjectMode() == 0) {
					str = "tele entrance";
				} else {
					str = "tele exit";
				}
				break;
			case OBJ_SENTRYGUN:
				str = "sentry gun";
				break;
			case OBJ_ATTACHMENT_SAPPER:
				str = "sapper";
				break;
			default:
				str = "unknown building";
				break;
			}
			
			CTFPlayer *builder = obj->GetBuilder();
			if (builder != nullptr) {
				str += CFmtStdStr(" built by %s", builder->GetPlayerName());
			} else {
				str += CFmtStdStr(" #%d", ENTINDEX(obj));
			}
			
			return str;
		}
		
		return CFmtStdStr("%s #%d", provider->GetClassname(), ENTINDEX(provider));
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_AddCond(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2 && args.ArgC() != 3) {
			ClientMsg(player, "[sig_addcond] Usage: any of the following:\n"
				"  sig_addcond <cond_number>            | add condition by number (unlimited duration)\n"
				"  sig_addcond <cond_name>              | add condition by name (unlimited duration)\n"
				"  sig_addcond <cond_number> <duration> | add condition by number (limited duration)\n"
				"  sig_addcond <cond_name> <duration>   | add condition by name (limited duration)\n"
				"  (condition names are \"TF_COND_*\"; look them up in tf.fgd or on the web)\n");
			return;
		}
		
		ETFCond cond = TF_COND_INVALID;
		if (StringToIntStrict(args[1], (int&)cond)) {
			if (!IsValidTFConditionNumber(cond)) {
				ClientMsg(player, "[sig_addcond] Error: %s is not a valid condition number (valid range: 0-%d inclusive)\n", args[1], GetNumberOfTFConds() - 1);
				return;
			}
		} else {
			cond = GetTFConditionFromName(args[1]);
			if (!IsValidTFConditionNumber(cond)) {
				ClientMsg(player, "[sig_addcond] Error: %s is not a valid condition name\n", args[1]);
				return;
			}
		}
		
		float duration = -1.0f;
		if (args.ArgC() == 3) {
			if (!StringToFloatStrict(args[2], duration)) {
				ClientMsg(player, "[sig_addcond] Error: %s is not a valid condition duration\n", args[2]);
				return;
			}
			if (duration < 0.0f) {
				ClientMsg(player, "[sig_addcond] Error: the condition duration cannot be negative\n");
				return;
			}
		}
		
		bool         before_incond   = player->m_Shared->InCond(cond);
		float        before_duration = player->m_Shared->GetConditionDuration(cond);
		CBaseEntity *before_provider = player->m_Shared->GetConditionProvider(cond);
		
		player->m_Shared->AddCond(cond, duration);
		
		bool         after_incond   = player->m_Shared->InCond(cond);
		float        after_duration = player->m_Shared->GetConditionDuration(cond);
		CBaseEntity *after_provider = player->m_Shared->GetConditionProvider(cond);
		
		ClientMsg(player, "[sig_addcond] Adding condition %s (%d) to player %s:\n"
			"\n"
			"            In Cond: %s\n"
			"  BEFORE:  Duration: %s\n"
			"           Provider: %s\n"
			"\n"
			"            In Cond: %s\n"
			"  AFTER:   Duration: %s\n"
			"           Provider: %s\n",
			GetTFConditionName(cond), (int)cond, player->GetPlayerName(),
			(before_incond ? "YES" : "NO"),
			(before_incond ? DescribeCondDuration(before_duration).c_str() : "--"),
			(before_incond ? DescribeCondProvider(before_provider).c_str() : "--"),
			( after_incond ? "YES" : "NO"),
			( after_incond ? DescribeCondDuration( after_duration).c_str() : "--"),
			( after_incond ? DescribeCondProvider( after_provider).c_str() : "--"));
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_RemoveCond(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ClientMsg(player, "[sig_removecond] Usage: any of the following:\n"
				"  sig_removecond <cond_number> | remove condition by number\n"
				"  sig_removecond <cond_name>   | remove condition by name\n"
				"  (condition names are \"TF_COND_*\"; look them up in tf.fgd or on the web)\n");
			return;
		}
		
		ETFCond cond = TF_COND_INVALID;
		if (StringToIntStrict(args[1], (int&)cond)) {
			if (!IsValidTFConditionNumber(cond)) {
				ClientMsg(player, "[sig_removecond] Error: %s is not a valid condition number (valid range: 0-%d inclusive)\n", args[1], GetNumberOfTFConds() - 1);
				return;
			}
		} else {
			cond = GetTFConditionFromName(args[1]);
			if (!IsValidTFConditionNumber(cond)) {
				ClientMsg(player, "[sig_removecond] Error: %s is not a valid condition name\n", args[1]);
				return;
			}
		}
		
		bool         before_incond   = player->m_Shared->InCond(cond);
		float        before_duration = player->m_Shared->GetConditionDuration(cond);
		CBaseEntity *before_provider = player->m_Shared->GetConditionProvider(cond);
		
		player->m_Shared->RemoveCond(cond);
		
		bool         after_incond   = player->m_Shared->InCond(cond);
		float        after_duration = player->m_Shared->GetConditionDuration(cond);
		CBaseEntity *after_provider = player->m_Shared->GetConditionProvider(cond);
		
		ClientMsg(player, "[sig_removecond] Removing condition %s (%d) from player %s:\n"
			"\n"
			"            In Cond: %s\n"
			"  BEFORE:  Duration: %s\n"
			"           Provider: %s\n"
			"\n"
			"            In Cond: %s\n"
			"  AFTER:   Duration: %s\n"
			"           Provider: %s\n",
			GetTFConditionName(cond), (int)cond, player->GetPlayerName(),
			(before_incond ? "YES" : "NO"),
			(before_incond ? DescribeCondDuration(before_duration).c_str() : "--"),
			(before_incond ? DescribeCondProvider(before_provider).c_str() : "--"),
			( after_incond ? "YES" : "NO"),
			( after_incond ? DescribeCondDuration( after_duration).c_str() : "--"),
			( after_incond ? DescribeCondProvider( after_provider).c_str() : "--"));
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_ListConds(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 1) {
			ClientMsg(player, "[sig_listconds] Usage:  sig_listconds\n");
			return;
		}
		
		struct CondInfo
		{
			CondInfo(CTFPlayer *player, ETFCond cond) :
				num(cond),
				str_name(GetTFConditionName(cond)),
				str_duration(DescribeCondDuration(player->m_Shared->GetConditionDuration(cond))),
				str_provider(DescribeCondProvider(player->m_Shared->GetConditionProvider(cond))) {}
			
			ETFCond num;
			std::string str_name;
			std::string str_duration;
			std::string str_provider;
		};
		std::deque<CondInfo> conds;
		
		size_t width_cond     = 0; // CONDITION
		size_t width_duration = 0; // DURATION
		size_t width_provider = 0; // PROVIDER
		
		for (int i = GetNumberOfTFConds() - 1; i >= 0; --i) {
			auto cond = (ETFCond)i;
			
			if (player->m_Shared->InCond(cond)) {
				conds.emplace_front(player, cond);
				
				width_cond     = std::max(width_cond,     conds.front().str_name    .size());
				width_duration = std::max(width_duration, conds.front().str_duration.size());
				width_provider = std::max(width_provider, conds.front().str_provider.size());
			}
		}
		
		if (conds.empty()) {
			ClientMsg(player, "[sig_listconds] Player %s is currently in zero conditions\n", player->GetPlayerName());
			return;
		}
		
		ClientMsg(player, "[sig_listconds] Player %s conditions:\n\n", player->GetPlayerName());
		
		width_cond     = std::max(width_cond + 4, strlen("CONDITION"));
		width_duration = std::max(width_duration, strlen("DURATION"));
		width_provider = std::max(width_provider, strlen("PROVIDER"));
		
		ClientMsg(player, "%-*s  %-*s  %-*s\n",
			(int)width_cond,     "CONDITION",
			(int)width_duration, "DURATION",
			(int)width_provider, "PROVIDER");
		
		for (const auto& cond : conds) {
			ClientMsg(player, "%-*s  %-*s  %-*s\n",
				(int)width_cond,     CFmtStr("%-3d %s", (int)cond.num, cond.str_name.c_str()).Get(),
				(int)width_duration, cond.str_duration.c_str(),
				(int)width_provider, cond.str_provider.c_str());
		}
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_SetHealth(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ClientMsg(player, "[sig_sethealth] Usage: any of the following:\n"
				"  sig_sethealth <hp_value>    | set your health to the given HP value\n"
				"  sig_sethealth <percent>%%max | set your health to the given percentage of your max health\n"
				"  sig_sethealth <percent>%%cur | set your health to the given percentage of your current health\n");
			return;
		}
		
		int hp;
		
		float value;
		size_t pos;
		if (sscanf(args[1], "%f%%max%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt((float)player->GetMaxHealth() * (value / 100.0f));
		} else if (sscanf(args[1], "%f%%cur%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt((float)player->GetHealth() * (value / 100.0f));
		} else if (sscanf(args[1], "%f%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt(value);
		} else {
			ClientMsg(player, "[sig_sethealth] Error: '%s' is not a HP value or max-health/current-health percentage\n", args[1]);
			return;
		}
		
		ClientMsg(player, "[sig_sethealth] Setting health of player %s to %d (previous health: %d).\n",
			player->GetPlayerName(), hp, player->GetHealth());
		
		player->SetHealth(hp);
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_AddHealth(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ClientMsg(player, "[sig_addhealth] Usage: any of the following:\n"
				"  sig_addhealth <hp_value>    | increase your health by the given HP value\n"
				"  sig_addhealth <percent>%%max | increase your health by the given percentage of your max health\n"
				"  sig_addhealth <percent>%%cur | increase your health by the given percentage of your current health\n");
			return;
		}
		
		int hp;
		
		float value;
		size_t pos;
		if (sscanf(args[1], "%f%%max%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt((float)player->GetMaxHealth() * (value / 100.0f));
		} else if (sscanf(args[1], "%f%%cur%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt((float)player->GetHealth() * (value / 100.0f));
		} else if (sscanf(args[1], "%f%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt(value);
		} else {
			ClientMsg(player, "[sig_addhealth] Error: '%s' is not a HP value or max-health/current-health percentage\n", args[1]);
			return;
		}
		
		ClientMsg(player, "[sig_addhealth] Increasing health of player %s by %d (previous health: %d).\n",
			player->GetPlayerName(), hp, player->GetHealth());
		
		player->SetHealth(player->GetHealth() + hp);
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_SubHealth(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ClientMsg(player, "[sig_subhealth] Usage: any of the following:\n"
				"  sig_subhealth <hp_value>    | decrease your health by the given HP value\n"
				"  sig_subhealth <percent>%%max | decrease your health by the given percentage of your max health\n"
				"  sig_subhealth <percent>%%cur | decrease your health by the given percentage of your current health\n");
			return;
		}
		
		int hp;
		
		float value;
		size_t pos;
		if (sscanf(args[1], "%f%%max%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt((float)player->GetMaxHealth() * (value / 100.0f));
		} else if (sscanf(args[1], "%f%%cur%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt((float)player->GetHealth() * (value / 100.0f));
		} else if (sscanf(args[1], "%f%zn", &value, &pos) == 1 && (pos == strlen(args[1]))) {
			hp = RoundFloatToInt(value);
		} else {
			ClientMsg(player, "[sig_subhealth] Error: '%s' is not a HP value or max-health/current-health percentage\n", args[1]);
			return;
		}
		
		ClientMsg(player, "[sig_subhealth] Decreasing health of player %s by %d (previous health: %d).\n",
			player->GetPlayerName(), hp, player->GetHealth());
		
		player->SetHealth(player->GetHealth() - hp);
	}
	
	void CC_Animation(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 3) {
			ClientMsg(player, "[sig_subhealth] Usage: any of the following:\n"
				"  sig_subhealth <hp_value>    | decrease your health by the given HP value\n");
			return;
		}
			//int sequence = playerl->LookupSequence(args[1]);
			
			//int arg1;
			//int arg2;
			//StringToIntStrict(args[1], arg1);
			//StringToIntStrict(args[2], arg2);
			//playerl->GetPlayerClass()->m_bUseClassAnimations = false;
			//playerl->ResetSequence(sequence);
			//playerl->GetPlayerClass() // //playerl->PlaySpecificSequence(args[1]);
			//TE_PlayerAnimEvent( playerl, 21 /*PLAYERANIMEVENT_SPAWN*/, sequence );
			player->PlaySpecificSequence(args[1]);
		
	}

	void CC_Reset_Animation(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 1) {
			ClientMsg(player, "[sig_subhealth] Usage: any of the following:\n"
				"  sig_subhealth <hp_value>    | decrease your health by the given HP value\n");
			return;
		}
		ForEachTFPlayer([&](CTFPlayer *playerl){
			playerl->DoAnimationEvent( 17 /*PLAYERANIMEVENT_SPAWN*/, 0 );
			
		});
		
	}

	void ChangeWeaponAndWearableTeam(CTFPlayer *player, int team)
	{
		
		for (int i = player->WeaponCount() - 1; i >= 0; --i) {
			CBaseCombatWeapon *weapon = player->GetWeapon(i);
			if (weapon == nullptr) continue;
			
			int pre_team = weapon->GetTeamNumber();
			int pre_skin = weapon->m_nSkin;

			weapon->ChangeTeam(team);
			weapon->m_nSkin = (team == TF_TEAM_BLUE ? 1 : 0);
			
			int post_team = weapon->GetTeamNumber();
			int post_skin = weapon->m_nSkin;
			
		}
		
		for (int i = player->GetNumWearables() - 1; i >= 0; --i) {
			CEconWearable *wearable = player->GetWearable(i);
			if (wearable == nullptr) continue;
			
			int pre_team = wearable->GetTeamNumber();
			int pre_skin = wearable->m_nSkin;
			
			wearable->ChangeTeam(team);
			wearable->m_nSkin = (team == TF_TEAM_BLUE ? 1 : 0);
			
			int post_team = wearable->GetTeamNumber();
			int post_skin = wearable->m_nSkin;
		}
	}

	void CC_Team(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 2) {
			ClientMsg(player, "[sig_subhealth] Usage: any of the following:\n"
				"  sig_subhealth <hp_value>    | decrease your health by the given HP value\n");
			return;
		}
		int teamnum;
		StringToIntStrict(args[1], teamnum);
		if (args.ArgC() == 2) {
			player->SetTeamNumber(teamnum);
			ChangeWeaponAndWearableTeam(player, teamnum);
		}
		else
		{
			ForEachTFPlayer([&](CTFPlayer *playerl){
				if (playerl->IsBot()) {
					playerl->SetTeamNumber(teamnum);
					ChangeWeaponAndWearableTeam(playerl, teamnum);
				}
				
			});
		}

		
		//player->StateTransition(TF_STATE_ACTIVE);
	}

	void CC_GiveItem(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 2) {
			ClientMsg(player, "[sig_subhealth] Usage: any of the following:\n"
				"  sig_subhealth <hp_value>    | decrease your health by the given HP value\n");
			return;
		}
		auto item_def = GetItemSchema()->GetItemDefinitionByName(args[1]);
		if (item_def != nullptr) {
			const char *classname = TranslateWeaponEntForClass_improved(item_def->GetItemClass(), player->GetPlayerClass()->GetClassIndex());
			
			CEconEntity *entity = static_cast<CEconEntity *>(ItemGeneration()->SpawnItem(item_def->m_iItemDefIndex, player->WorldSpaceCenter(), vec3_angle, 1, 6, classname));
			ClientMsg(player, "item %d", entity);
			DispatchSpawn(entity);


			if (entity != nullptr) {
				
				GiveItemToPlayer(player, entity, false, true, args[1]);
			}
		}
		//engine->ServerCommand(CFmtStr("ce_mvm_equip_itemname %d \"%s\"\n", ENTINDEX(player), args[1]));
		//engine->ServerExecute();
	}

	void CC_GetMissingBones(CTFPlayer *player, const CCommand& args)
	{
		for (int i = 1; i < 10; i++) {
			const char *robot_model = g_szBotModels[i];
			const char *player_model = CFmtStr("models/player/%s.mdl", g_aRawPlayerClassNamesShort[i]);
			studiohdr_t *studio_player = mdlcache->GetStudioHdr(mdlcache->FindMDL(player_model));
			studiohdr_t *studio_robot = mdlcache->GetStudioHdr(mdlcache->FindMDL(robot_model));
			//anim_robot->SetModel(robot_model);
			//anim_player->SetModel(player_model);

			if (studio_robot == nullptr) {
				ClientMsg(player, "%s is null\n", robot_model);
				break;
			}
			else if (studio_player == nullptr) {
				ClientMsg(player, "%s is null\n", player_model);
				break;
			}

			int numbones_robot = studio_robot->numbones;
			int numbones_player = studio_player->numbones;

			std::set<std::string> bones;

			for (int j = 0; j < numbones_player; j++) {
				auto *bone = studio_player->pBone(j);
				bones.insert(bone->pszName());
			}

			for (int j = 0; j < numbones_robot; j++) {
				auto *bone = studio_robot->pBone(j);
				bones.erase(bone->pszName());
			}

			Msg("Missing bones on class %d\n", i);
			for (auto &string : bones) {
				Msg("%s\n", string);
			}
		}
		//engine->ServerCommand(CFmtStr("ce_mvm_equip_itemname %d \"%s\"\n", ENTINDEX(player), args[1]));
		//engine->ServerExecute();
	}

	CTFPlayer *playertrg = nullptr;
	std::string displaystr = "";
	void CC_Benchmark(CTFPlayer *player, const CCommand& args)
	{
		
		if (args.ArgC() < 3) {
			return;
		}
		int times = 0;
		StringToIntStrict(args[1], times);
		
		bool check = false;

		CFastTimer timer;
		timer.Start(); 
		for(int i = 0; i < times; i++) {
			asm ("");
		}
		timer.End();
		displaystr = "";
		displaystr += CFmtStr("loop time: %.9f", timer.GetDuration().GetSeconds());

		if (strcmp(args[2], "testplayer") == 0) {
			CBaseEntity *target = player;
			if (args.ArgC() == 4)
				target = servertools->FindEntityByClassname(nullptr, "tf_gamerules");
			timer.Start(); 
			for(int i = 0; i < times; i++) {
				check = (rtti_cast<const CBasePlayer *>(target) != nullptr);
			}
			timer.End();
			
			displaystr += CFmtStr("rtti cast time: %.9f", timer.GetDuration().GetSeconds());
		
			timer.Start();
			for(int i = 0; i < times; i++) {
				check = target->IsPlayer();
			}
			timer.End();
			displaystr += CFmtStr("virtual check time timer: %.9f", timer.GetDuration().GetSeconds());
			
			timer.Start();
			for(int i = 0; i < times; i++) {
				check = target->IsPlayer();
			}
			timer.End();
			
			displaystr += CFmtStr("virtual check time: %.9f", timer.GetDuration().GetSeconds());

			
		}
		else if (strcmp(args[2], "totfbot") == 0) {
			
			CBaseEntity *bot = nullptr;

			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				bot = ToTFBot(UTIL_PlayerByIndex(i));
				if (bot != nullptr)
					break;
			}

			timer.Start(); 
			for(int i = 0; i < times; i++) {
				check = bot != nullptr && bot->IsPlayer() && rtti_cast<INextBot *>(bot);
			}
			timer.End();
			
			displaystr += CFmtStr("rtti cast time: %.9f", timer.GetDuration().GetSeconds());
			
			timer.Start(); 
			for(int i = 0; i < times; i++) {
				check = bot != nullptr && bot->IsPlayer() && rtti_cast<CTFBot *>(bot);
			}
			timer.End();
			
			displaystr += CFmtStr("rtti downcast cast time: %.9f", timer.GetDuration().GetSeconds());

			//timer.Start(); 
			timer.Start();
			for(int i = 0; i < times; i++) {
				check = ToTFBot(bot) != nullptr;
			}
			timer.End();
			//timer.End();
			//
			displaystr += CFmtStr("virtual check time: %.9f", timer.GetDuration().GetSeconds());

			IVision *vision = bot != nullptr ? bot->MyNextBotPointer()->GetVisionInterface() : nullptr;
			timer.Start();
			for(int i = 0; i < times; i++) {
				check = vision != nullptr && static_cast<CTFBot *>(vision->GetBot()->GetEntity())->ExtAttr()[CTFBot::ExtendedAttr::TARGET_STICKIES];
			}
			timer.End();
			//timer.End();
			//
			displaystr += CFmtStr("virtual check time: %.9f", timer.GetDuration().GetSeconds());

		}
		else if (strcmp(args[2], "attributes") == 0) {
			playertrg = player;
			CBaseEntity *bot = nullptr;
			CBaseEntity *bot2 = nullptr;
			CBaseEntity *bot3 = nullptr;

			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				if (bot == nullptr)
					bot = ToTFBot(UTIL_PlayerByIndex(i));
				else if (bot2 == nullptr)
					bot2 = ToTFBot(UTIL_PlayerByIndex(i));
				else if (bot3 == nullptr)
					bot3 = ToTFBot(UTIL_PlayerByIndex(i));
			}
			CAttributeManager *mgr = player->GetAttributeManager();

			timer.Start(); 
			string_t poolstr = AllocPooledString_StaticConstantStringPointer("mult_dmg");
			mgr->ApplyAttributeFloatWrapperFunc(1.0f, player, poolstr);
			//CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_dmg", player, nullptr, true);
			timer.End();
			
			displaystr += CFmtStr("attr init: %.9f", timer.GetDuration().GetSeconds());

			CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_dmg", player, nullptr, true);
			CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_dmg", player, nullptr, true);
			
			mgr->ApplyAttributeFloatWrapperFunc(1.0f, player, poolstr);
			mgr->ApplyAttributeFloatWrapperFunc(1.0f, player, poolstr );
			mgr->ApplyAttributeFloatWrapperFunc(1.0f, player, poolstr );


			timer.Start(); 
			for(int i = 0; i < times; i++) {
				poolstr = AllocPooledString_StaticConstantStringPointer("mult_dmg");
				mgr->ApplyAttributeFloatWrapperFunc(1.0f, player, poolstr );
			}
			timer.End();
			
			displaystr += CFmtStr("attr single time wrap: %.9f", timer.GetDuration().GetSeconds());

			timer.Start(); 
			
			for(int i = 0; i < times; i++) {
				CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_dmg", player, nullptr, true);
			}
			timer.End();
			
			displaystr += CFmtStr("attr single time: %.9f", timer.GetDuration().GetSeconds());


			timer.Start(); 
			for(int i = 0; i < times; i++) {
				CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_dmg", player, nullptr, true);
				CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_sniper_charge_per_sec", bot, nullptr, true);
				CAttributeManager::ft_AttribHookValue_int(0, "set_turn_to_ice", bot2, nullptr, true);
				CAttributeManager::ft_AttribHookValue_int(0, "use_large_smoke_explosion", bot3, nullptr, true);
			}
			timer.End();
			
			displaystr += CFmtStr("attr multi time: %.9f", timer.GetDuration().GetSeconds());

			auto weapon = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_PRIMARY);
			auto weapon2 = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_SECONDARY);
			auto weapon3 = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_MELEE);
			auto weapon4 = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_ACTION);
			timer.Start(); 
			for(int i = 0; i < times; i++) {
				float attr = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, attr, mult_dmg);
			}
			timer.End();
			
			displaystr += CFmtStr("wep single time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start(); 
			for(int i = 0; i < times; i++) {
				int attr = 1;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, attr, mult_dmg);
			}
			timer.End();
			
			displaystr += CFmtStr("wep int single time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start(); 
			for(int i = 0; i < times; i++) {
				CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_dmg", weapon, nullptr, true);
				CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_sniper_charge_per_sec", weapon2, nullptr, true);
				CAttributeManager::ft_AttribHookValue_int(0, "set_turn_to_ice", weapon, nullptr, true);
				CAttributeManager::ft_AttribHookValue_int(0, "use_large_smoke_explosion", weapon2, nullptr, true);
			}
			timer.End();
			
			displaystr += CFmtStr("wep multi time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start(); 
			for(int i = 0; i < times; i++) {
				CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_dmg", weapon, nullptr, true);
				CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_sniper_charge_per_sec", weapon2, nullptr, true);
				CAttributeManager::ft_AttribHookValue_int(0, "set_turn_to_ice", weapon3, nullptr, true);
				CAttributeManager::ft_AttribHookValue_int(0, "use_large_smoke_explosion", weapon4, nullptr, true);
			}
			timer.End();
			
			displaystr += CFmtStr("wep multi 4 time: %.9f", timer.GetDuration().GetSeconds());

			playertrg = nullptr;
		}
		else if (strcmp(args[2], "isfakeclient") == 0) {
			
			timer.Start(); 
			for(int i = 0; i < times; i++) {
				check = player->IsFakeClient();
			}
			timer.End();
			
			displaystr += CFmtStr("virtual time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start(); 
			for(int i = 0; i < times; i++) {
				check = playerinfomanager->GetPlayerInfo(player->edict())->IsFakeClient();
			}
			timer.End();
			
			displaystr += CFmtStr("playerinfo time: %.9f", timer.GetDuration().GetSeconds());
		}
		else if (strcmp(args[2], "pooledstring") == 0) {
			
			AllocPooledString_StaticConstantStringPointer("Pooled str");
			AllocPooledString("Pooled str");
			FindPooledString("Pooled str");

			timer.Start();
			for(int i = 0; i < times; i++) {
				AllocPooledString_StaticConstantStringPointer("Pooled str");
			}
			timer.End();
			
			displaystr += CFmtStr("alloc pooled constant time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start();
			for(int i = 0; i < times; i++) {
				AllocPooledString("Pooled str");
			}
			timer.End();
			
			displaystr += CFmtStr("alloc pooled time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start();
			for(int i = 0; i < times; i++) {
				FindPooledString("Pooled str");
			}
			timer.End();
			
			displaystr += CFmtStr("find pooled time: %.9f", timer.GetDuration().GetSeconds());
		}
		else if (strcmp(args[2], "rtticast") == 0) {
			
			float timespent_getrtti = 0.0f;
			float timespent_cast = 0.0f;
			CBaseEntity *testent = player;
			CTFPlayer *resultplayer = nullptr;
			CBaseObject *resultobj = nullptr;
			CBaseEntity *gamerules = servertools->FindEntityByClassname(nullptr, "tf_gamerules");

			auto rtti_base_entity = RTTI::GetRTTI<CBaseEntity>();
			auto rtti_tf_player   = RTTI::GetRTTI<CTFPlayer>();
			auto rtti_base_object = RTTI::GetRTTI<CBaseObject>();
			auto rtti_has_attributes = RTTI::GetRTTI<IHasAttributes>();

			
			timer.Start();
			for(int i = 0; i < times; i++) {
				rtti_cast<CBaseEntity *>(player);
			}
			timer.End();
			
			displaystr += CFmtStr("player to entity time: %.9f", timer.GetDuration().GetSeconds());

			
			timer.Start();
			for(int i = 0; i < times; i++) {
				rtti_cast<CTFPlayer *>(testent);
			}
			timer.End();
			
			displaystr += CFmtStr("entity to player time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start();
			for(int i = 0; i < times; i++) {
				rtti_scast<CTFPlayer *>(testent);
			}
			timer.End();
			
			displaystr += CFmtStr("entity to player static time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start();
			for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
				auto proj = rtti_scast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
			}
			timer.End();

			displaystr += CFmtStr("projectile cast: %.9f", timer.GetDuration().GetSeconds());

			ClientMsg(player, "%s", displaystr.c_str());
			displaystr = "";
			displaystr += CFmtStr("\n\nrtti_cast casting tests: \n\n");
			displaystr += CFmtStr("(tfplayer)entity->player %d | ", rtti_cast<CTFPlayer *>(testent));
			displaystr += CFmtStr("(gamerules)entity->player %d | ", rtti_cast<CTFPlayer *>(gamerules));
			displaystr += CFmtStr("(tfplayer)player->entity %d | ", rtti_cast<CBaseEntity *>(player));
			displaystr += CFmtStr("(tfplayer)player->hasattributes %d | ", rtti_cast<IHasAttributes *>(player));
			displaystr += CFmtStr("(tfplayer)player->hasattributes (static) %d | ", rtti_scast<IHasAttributes *>(player));
			displaystr += CFmtStr("(tfplayer)player->iscorer %d | ", rtti_cast<IScorer *>(player));
			displaystr += CFmtStr("(tfplayer)player->iscorer (static) %d | ", rtti_scast<IScorer *>(player));
			displaystr += CFmtStr("(gamerules)entity->hasattributes %d | ", rtti_cast<IHasAttributes *>(gamerules));
			displaystr += CFmtStr("(gamerules)entity->hasattributes (static) %d | ", rtti_scast<IHasAttributes *, CTFPlayer *>(gamerules));
			displaystr += CFmtStr("(tfplayer)entity->hasattributes %d | ", rtti_cast<IHasAttributes *>(testent));
			displaystr += CFmtStr("(tfplayer)object->entity %d | ", rtti_cast<CBaseEntity *>(reinterpret_cast<CBaseObject *>(testent)));
			displaystr += CFmtStr("(tfplayer)entity->object %d | ", rtti_cast<CBaseObject *>(testent));
			displaystr += CFmtStr("(tfplayer)player->object %d | ", rtti_cast<CBaseObject *>(player));

			ClientMsg(player, "%s", displaystr.c_str());
			displaystr = "";
			displaystr += CFmtStr("\n\nupcast casting tests: \n\n");
			displaystr += CFmtStr("(tfplayer)entity->player %d | ", static_cast<const std::type_info *>(rtti_base_entity)->__do_upcast(rtti_tf_player, (void **)&testent));
			displaystr += CFmtStr("(gamerules)entity->player %d | ", static_cast<const std::type_info *>(rtti_base_entity)->__do_upcast(rtti_tf_player, (void **)&gamerules));
			displaystr += CFmtStr("(tfplayer)player->entity %d | ", static_cast<const std::type_info *>(rtti_tf_player)->__do_upcast(rtti_base_entity, (void **)&testent));
			displaystr += CFmtStr("(tfplayer)player->hasattributes %d | ", static_cast<const std::type_info *>(rtti_tf_player)->__do_upcast(rtti_has_attributes, (void **)&testent));
			displaystr += CFmtStr("(gamerules)entity->hasattributes %d | ", static_cast<const std::type_info *>(rtti_base_entity)->__do_upcast(rtti_has_attributes, (void **)&gamerules));
			displaystr += CFmtStr("(tfplayer)entity->hasattributes %d | ", static_cast<const std::type_info *>(rtti_base_entity)->__do_upcast(rtti_has_attributes, (void **)&testent));
			displaystr += CFmtStr("(tfplayer)object->entity %d | ", static_cast<const std::type_info *>(rtti_base_object)->__do_upcast(rtti_base_entity, (void **)&testent));
			displaystr += CFmtStr("(tfplayer)entity->object %d | ", static_cast<const std::type_info *>(rtti_base_entity)->__do_upcast(rtti_base_object, (void **)&testent));
			displaystr += CFmtStr("(tfplayer)player->object %d | ", static_cast<const std::type_info *>(rtti_tf_player)->__do_upcast(rtti_base_object, (void **)&testent));

		}
		else if (strcmp(args[2], "rtticastscorer") == 0) {
			
			/*CTFBot *bot = nullptr;

			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				bot = ToTFBot(UTIL_PlayerByIndex(i));
				if (bot != nullptr)
					break;
			}*/

			//if (bot != nullptr) {
				displaystr += CFmtStr("rtti_cast %d | ", rtti_cast<IHasAttributes *>(player));
				displaystr += CFmtStr("rtti_cast2 %d | ", rtti_cast<CBaseEntity *>(player));
				displaystr += CFmtStr("player %d | ", player);
			//}


		}
		ClientMsg(player, "%s", displaystr.c_str());
	}

	void CC_Taunt(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 2) {
			ClientMsg(player, "[sig_taunt] Usage: any of the following:\n"
				"  sig_taunt <tauntname>    | start taunt of name\n");
			return;
		}

		static std::unordered_map<std::string, CEconItemView *> taunt_item_view;

		CEconItemView *view = taunt_item_view[args[2]];
						
		if (view == nullptr) {
			auto item_def = GetItemSchema()->GetItemDefinitionByName(args[2]);
			if (item_def != nullptr) {
				view = CEconItemView::Create();
				view->Init(item_def->m_iItemDefIndex, 6, 9999, 0);
				taunt_item_view[args[2]] = view;
			}
		}

		if (view != nullptr) {
			
			std::vector<CBasePlayer *> vec;
			GetSMTargets(player, args[1], vec);
			if (vec.empty()) {
				ClientMsg(player, "[sig_taunt] Error: no matching target found for \"%s\".\n", args[1]);
				return;
			}
			for (CBasePlayer *target : vec) {
				ToTFPlayer(target)->PlayTauntSceneFromItem(view);
			}
		}
	}
	
	void CC_AddAttr(CTFPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 4) {
			ClientMsg(player, "[sig_addattr] Usage: any of the following:\n"
				"  sig_addattr <target> <attribute> <value> [slot]   | add attribute to held item\n");
			return;
		}

		int slot = -1;
		if (args.ArgC() == 5) 
			StringToIntStrict(args[4], (int&)slot);

		char target_names[64];
		std::vector<CBasePlayer *> vec;
		GetSMTargets(player, args[1], vec, target_names, 64);
		if (vec.empty()) {
			ClientMsg(player, "[sig_addattr] Error: no matching target found for \"%s\".\n", args[1]);
			return;
		}

		for (CBasePlayer *target : vec) {
			CAttributeList *list;
			if (slot != -1) {
				CEconEntity *item = GetEconEntityAtLoadoutSlot(ToTFPlayer(target), slot);
				if (item != nullptr) {
					list = &item->GetItem()->GetAttributeList();
				}
			}
			else {
				list = ToTFPlayer(target)->GetAttributeList();
			}

			if (list != nullptr) {
				
				/* attempt lookup first by attr name, then by attr definition index */
				CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(args[2]);
				if (attr_def == nullptr) {
					int idx = -1;
					if (StringToIntStrict(args[2], idx)) {
						attr_def = GetItemSchema()->GetAttributeDefinition(idx);
					}
				}
				
				if (attr_def == nullptr) {
					ClientMsg(player, "[sig_addattr] Error: couldn't find any attributes in the item schema matching \"%s\".\n", args[2]);
					return;
				}
				list->AddStringAttribute(attr_def, args[3]);
			}
			else {
				ClientMsg(player, "[sig_addattr] Error: couldn't find any item in slot\"%d\".\n", slot);
				return;
			}
		}
		ClientMsg(player, "[sig_addattr] Successfully added attribute %s = %s to %s.\n", args[2], args[3], target_names);
	}

	bool allow_create_dropped_weapon = false;
	void CC_DropItem(CTFPlayer *player, const CCommand& args)
	{
		std::vector<CBasePlayer *> vec;
		GetSMTargets(player, args[1], vec);
		if (vec.empty()) {
			ClientMsg(player, "[sig_dropitem] Error: no matching target found for \"%s\".\n", args[1]);
			return;
		}

		for (CBasePlayer *target : vec) {
			CTFWeaponBase *item = ToTFPlayer(target)->GetActiveTFWeapon();

			if (item != nullptr) {
				CEconItemView *item_view = item->GetItem();

				allow_create_dropped_weapon = true;
				CTFDroppedWeapon::Create(ToTFPlayer(target), player->EyePosition(), vec3_angle, item->GetWorldModel(), item_view);
				allow_create_dropped_weapon = false;
			}
			else {
				ClientMsg(player, "[sig_dropitem] Error: couldn't find any item.\n");
			}
		}
	}
	std::map<std::string, int> modelmap;
	std::map<std::string, int> modelmapmedal;
	std::vector<CBasePlayer *> modelplayertargets;

	void CC_GiveEveryItem(CTFPlayer *player, const CCommand& args)
	{
		GetSMTargets(player, args[1], modelplayertargets);
		if (modelplayertargets.empty()) {
			ClientMsg(player, "[sig_giveeveryitem] Error: no matching target found for \"%s\".\n", args[1]);
			return;
		}
		int items = 0;
		
		modelmap.clear();
		modelmapmedal.clear();

		for (int i = 0; i < 60000; i++) {
			auto item_def = GetItemSchema()->GetItemDefinition(i);
			if (item_def != nullptr && strncmp(item_def->GetItemClass(), "tf_wearable", sizeof("tf_wearable")) == 0) {
				if (args.ArgC() == 2 || item_def->GetKeyValues()->FindKey("used_by_classes")->GetInt(args[2], 0) == 1) {
					const char *model = item_def->GetKeyValues()->GetString("model_player", nullptr);
					if (model == nullptr) {
						auto kv_perclass = item_def->GetKeyValues()->FindKey("model_player_per_class");
						if (kv_perclass != nullptr && kv_perclass->GetFirstSubKey() != nullptr) {
							model = kv_perclass->GetFirstSubKey()->GetString();
						}
					}
					if (model != nullptr) {
						//if (modelmap.find(model) == modelmap.end())
							//Msg("item: %s\n", model);
							
						if (strcmp(item_def->GetKeyValues()->GetString("equip_region", ""), "medal") != 0)
							modelmap[model] = i;
						else
							modelmapmedal[model] = i;
					}
				}
			}
			
		}

		ClientMsg(player, "items: %d\n", modelmap.size());
		ClientMsg(player, "items medals: %d\n", modelmapmedal.size());
	}
	
	void CC_DumpInventory(CTFPlayer *player, const CCommand& args)
	{
	}

	void CC_DumpItems(CTFPlayer *player, const CCommand& args)
	{
		std::vector<CBasePlayer *> vec;
		GetSMTargets(player, args[1], vec);
		if (vec.empty()) {
			ClientMsg(player, "[sig_listitemattr] Error: no matching target found for \"%s\".\n", args[1]);
			return;
		}
		std::string displaystr;
		for (CBasePlayer *target : vec) {
			ForEachTFPlayerEconEntity(ToTFPlayer(target), [&](CEconEntity *entity) {
				CEconItemView *view = entity->GetItem();
				if (view != nullptr) {
					auto &attrs = view->GetAttributeList().Attributes();

					ClientMsg(player, "Item %s:\n", view->GetItemDefinition()->GetName());

					class Wrapper : public IEconItemAttributeIterator
					{
					public:
						Wrapper(CTFPlayer *player) : m_player(player) {}

						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, unsigned int                             value) const
						{
							attribute_data_union_t valueu;
							valueu.m_UInt = value;
							ClientMsg(m_player, "%s = %f (%d) (%.17en)\n", pAttrDef->GetName(), valueu.m_Float, valueu.m_UInt, valueu.m_Float);
							return true;
						}
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, float                                    value) const { return true; }
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const uint64&                            value) const { return true; }
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String&                 value) const
						{
							const char *pstr;
							CopyStringAttributeValueToCharPointerOutput(&value, &pstr);
							ClientMsg(m_player, "%s = %s\n", pAttrDef->GetName(), pstr);
							return true;
						}

						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value) const { return true; }
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_ItemSlotCriteria&       value) const { return true; }
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_WorldItemPlacement&     value) const { return true; }

					private:
						CTFPlayer *m_player;
					};

					Wrapper wrapper(player);
					view->IterateAttributes(&wrapper);
					ClientMsg(player, "\n");
				}
			});
		}
		ClientMsg(player, "%s", displaystr.c_str());
	}

	void CC_Sprays(CTFPlayer *player, const CCommand& args)
	{
		for (int i = 0; i < sv->GetClientCount(); i++) {
			CBaseClient *client = static_cast<CBaseClient *> (sv->GetClient(i));
			if (client != nullptr) {
				DevMsg("Spray apply %x %d %d\n", client->m_nCustomFiles[0].crc, client->m_nClientSlot, client->m_nEntityIndex);
			}
		}
	}

	// TODO: use an std::unordered_map so we don't have to do any V_stricmp's at all for lookups
	// (also make this change in Util:Make_Item)
	static const std::map<const char *, void (*)(CTFPlayer *, const CCommand&), VStricmpLess> cmds {
		{ "sig_setplayerscale",   CC_SetPlayerScale   },
		{ "sig_setplayermodel",   CC_SetPlayerModel   },
		{ "sig_resetplayermodel", CC_ResetPlayerModel },
		{ "sig_unequip",          CC_UnEquip          },
		{ "sig_addcond",          CC_AddCond          },
		{ "sig_removecond",       CC_RemoveCond       },
		{ "sig_listconds",        CC_ListConds        },
		{ "sig_sethealth",        CC_SetHealth        },
		{ "sig_addhealth",        CC_AddHealth        },
		{ "sig_subhealth",        CC_SubHealth        },
		{ "sig_animation",        CC_Animation        },
		{ "sig_resetanim",        CC_Reset_Animation  },
		{ "sig_teamset",          CC_Team             },
		{ "sig_giveitemcreator",  CC_GiveItem         },
		{ "sig_benchmark",        CC_Benchmark        },
		{ "sig_taunt",            CC_Taunt            },
		{ "sig_addattr",          CC_AddAttr          },
		{ "sig_dropitem",         CC_DropItem         },
		{ "sig_giveeveryitem",    CC_GiveEveryItem    },
		{ "sig_getmissingbones",  CC_GetMissingBones  },
		{ "sig_listitemattr",     CC_DumpItems        },
		{ "sig_sprays",           CC_Sprays           },
	};

	DETOUR_DECL_MEMBER(bool, CTFPlayer_ClientCommand, const CCommand& args)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player != nullptr) {
			auto it = cmds.find(args[0]);
			if (it != cmds.end()) {
				extern ConVar cvar_adminonly;
				if (!cvar_adminonly.GetBool() || PlayerIsSMAdminOrBot(player)) {
					auto func = (*it).second;
					(*func)(player, args);
				} else {
					ClientMsg(player, "[%s] You are not authorized to use this command because you are not a SourceMod admin. Sorry.\n", (*it).first);
				}
				
				return true;
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_ClientCommand)(args);
	}
	
	DETOUR_DECL_STATIC(CTFDroppedWeapon *, CTFDroppedWeapon_Create, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner, const char *pszModelName, const CEconItemView *pItemView)
	{
		// this is really ugly... we temporarily override m_bPlayingMannVsMachine
		// because the alternative would be to make a patch
		
		bool is_mvm_mode = TFGameRules()->IsMannVsMachineMode();

		if (allow_create_dropped_weapon) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);
		}
		
		auto result = DETOUR_STATIC_CALL(CTFDroppedWeapon_Create)(vecOrigin, vecAngles, pOwner, pszModelName, pItemView);
		
		if (allow_create_dropped_weapon) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(is_mvm_mode);
		}
		
		return result;
	}
	
	class CMod : public IMod, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:Client_Cmds")
		{
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand, "CTFPlayer::ClientCommand");
			MOD_ADD_DETOUR_STATIC(CTFDroppedWeapon_Create, "CTFDroppedWeapon::Create");
		}
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void FrameUpdatePostEntityThink() override
		{
			if (!modelplayertargets.empty()) {
				for (CBasePlayer *target : modelplayertargets) {
					int amount = 0;
					for (auto it = modelmap.begin(); it != modelmap.end();) {
						if (engine->GetEntityCount() > 1920) {
							modelmap.clear();
							break;
						}

						CEconWearable *wearable = static_cast<CEconWearable *>(ItemGeneration()->SpawnItem(it->second, Vector(0,0,0), QAngle(0,0,0), 6, 9999, "tf_wearable"));
						if (wearable != nullptr) {
							
							wearable->m_bValidatedAttachedEntity = true;
							wearable->GiveTo(target);
							target->EquipWearable(wearable);
						}
						it = modelmap.erase(it);
						amount++;
						if (amount >= 100) {
							break;
						}
					}
				}
			}
		}
	};
	CMod s_Mod;
	
	
	/* by way of incredibly annoying persistent requests from Hell-met,
	 * I've acquiesced and made this mod convar non-notifying (sigh) */
	ConVar cvar_enable("sig_util_client_cmds", "0", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Utility: enable client cheat commands",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	/* default: admin-only mode ENABLED */
	ConVar cvar_adminonly("sig_util_client_cmds_adminonly", "1", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Utility: restrict this mod's functionality to SM admins only");
}
