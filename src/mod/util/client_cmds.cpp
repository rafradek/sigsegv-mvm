#include "mod.h"
#include "stub/econ.h"
#include "stub/extraentitydata.h"
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
#include "util/override.h"
#include "util/expression_eval.h"
#include "mod/pop/popmgr_extensions.h"
#include <sys/resource.h>
#include <fmt/core.h>
#include "mod/common/commands.h"
#include "mod/common/text_hud.h"

namespace Mod::Attr::Custom_Attributes
{
	float GetFastAttributeFloat(CBaseEntity *entity, float value, int name);
	
	enum FastAttributeClassItem
	{
		ALWAYS_CRIT,
		ADD_COND_ON_ACTIVE,
		MAX_AOE_TARGETS,
		ATTRIB_COUNT_ITEM,
	};
}

namespace Mod::Util::Client_Cmds
{
	// TODO: another version that allows setting a different player's scale...?
	void CC_SetPlayerScale(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ModCommandResponse("[sig_setplayerscale] Usage:  sig_setplayerscale <scale>\n");
			return;
		}
		
		float scale = 1.0f;
		if (!StringToFloatStrict(args[1], scale)) {
			ModCommandResponse("[sig_setplayerscale] Error: couldn't parse \"%s\" as a floating-point number.\n", args[1]);
			return;
		}
		
		player->SetModelScale(scale);
		
		ModCommandResponse("[sig_setplayerscale] Set scale of player %s to %.2f.\n", player->GetPlayerName(), scale);
	}
	
	
	// TODO: another version that allows setting a different player's model...?
	void CC_SetPlayerModel(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ModCommandResponse("[sig_setplayermodel] Usage:  sig_setplayermodel <model_path>\n");
			return;
		}
		
		const char *model_path = args[1];
		
		player->GetPlayerClass()->SetCustomModel(model_path, true);
		player->UpdateModel();
		
		ModCommandResponse("[sig_setplayermodel] Set model of player %s to \"%s\".\n", player->GetPlayerName(), model_path);
	}
	

	// TODO: another version that allows resetting a different player's model...?
	void CC_ResetPlayerModel(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 1) {
			ModCommandResponse("[sig_resetplayermodel] Usage:  sig_resetplayermodel\n");
			return;
		}
		
		player->GetPlayerClass()->SetCustomModel(nullptr, true);
		player->UpdateModel();
		
		ModCommandResponse("[sig_resetplayermodel] Reset model of player %s to the default.\n", player->GetPlayerName());
	}
	
	
	void CC_UnEquip(CCommandPlayer *player, const CCommand& args)
	{
		auto l_usage = [=]{
			ModCommandResponse("[sig_unequip] Usage: any of the following:\n"
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
				ModCommandResponse("[sig_unequip] UNIMPLEMENTED: help\n");
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
					ModCommandResponse("[sig_unequip] Error: couldn't find any items in the item schema matching \"%s\"\n", args[1]);
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
					ModCommandResponse("[sig_unequip] Unequipped weapon %s from slot %s\n", item_view->GetStaticData()->GetName(), GetLoadoutSlotName(item_view->GetStaticData()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex())));
					
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
					ModCommandResponse("[sig_unequip] Unequipped cosmetic %s from slot %s\n", item_view->GetStaticData()->GetName(), GetLoadoutSlotName(item_view->GetStaticData()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex())));
					
					player->RemoveWearable(wearable);
					
					++n_wearables_removed;
				}
			}
			
			ModCommandResponse("[sig_unequip] Unequipped %d weapons and %d cosmetics.\n", n_weapons_removed, n_wearables_removed);
			return;
		} else if (args.ArgC() == 3) {
			if (FStrEq(args[1], "slot")) {
				int slot = -1;
				if (StringToIntStrict(args[2], slot)) {
					if (!IsValidLoadoutSlotNumber(slot)) {
						ModCommandResponse("[sig_unequip] Error: %s is not a valid loadout slot number\n", args[2]);
						return;
					}
				} else {
					slot = GetLoadoutSlotByName(args[2]);
					if (!IsValidLoadoutSlotNumber(slot)) {
						ModCommandResponse("[sig_unequip] Error: %s is not a valid loadout slot name\n", args[2]);
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
							
							ModCommandResponse("[sig_unequip] Unequipped weapon %s from slot %s\n", item_view->GetStaticData()->GetName(), GetLoadoutSlotName(slot));
							
							player->Weapon_Detach(weapon);
							weapon->Remove();
							
							++n_weapons_removed;
						} else if (econ_entity->IsWearable()) {
							auto wearable = rtti_cast<CEconWearable *>(econ_entity);
							
							ModCommandResponse("[sig_unequip] Unequipped cosmetic %s from slot %s\n", item_view->GetStaticData()->GetName(), GetLoadoutSlotName(slot));
							
							player->RemoveWearable(wearable);
							
							++n_wearables_removed;
						} else {
							ModCommandResponse("[sig_unequip] Unequipped unexpected entity with classname \"%s\" from slot %s\n", econ_entity->GetClassname(), GetLoadoutSlotName(slot));
							
							econ_entity->Remove();
						}
					}
				} while (econ_entity != nullptr);
				
				ModCommandResponse("[sig_unequip] Unequipped %d weapons and %d cosmetics.\n", n_weapons_removed, n_wearables_removed);
				return;
			} else if (FStrEq(args[1], "region")) {
				// TODO: region_name
				ModCommandResponse("[sig_unequip] UNIMPLEMENTED: region\n");
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
	void CC_AddCond(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2 && args.ArgC() != 3 && args.ArgC() != 4) {
			ModCommandResponse("[sig_addcond] Usage: any of the following:\n"
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
				ModCommandResponse("[sig_addcond] Error: %s is not a valid condition number (valid range: 0-%d inclusive)\n", args[1], GetNumberOfTFConds() - 1);
				return;
			}
		} else {
			cond = GetTFConditionFromName(args[1]);
			if (!IsValidTFConditionNumber(cond)) {
				ModCommandResponse("[sig_addcond] Error: %s is not a valid condition name\n", args[1]);
				return;
			}
		}
		
		std::vector<CBasePlayer *> vec;

		if (args.ArgC() > 2) {
			GetSMTargets(player, args[2], vec);
		}
		else {
			vec.push_back(player);
		}
		
		float duration = -1.0f;
		if (args.ArgC() > 3) {
			if (!StringToFloatStrict(args[3], duration)) {
				ModCommandResponse("[sig_addcond] Error: %s is not a valid condition duration\n", args[2]);
				return;
			}
			if (duration < 0.0f) {
				ModCommandResponse("[sig_addcond] Error: the condition duration cannot be negative\n");
				return;
			}
		}
		
		bool         before_incond   = player->m_Shared->InCond(cond);
		float        before_duration = player->m_Shared->GetConditionDuration(cond);
		CBaseEntity *before_provider = player->m_Shared->GetConditionProvider(cond);
		
		if (vec.empty()) {
			ModCommandResponse("[sig_addcond] Error: no matching target found for \"%s\".\n", args[2]);
			return;
		}
		for (auto target : vec) {
			ToTFPlayer(target)->m_Shared->AddCond(cond, duration);
		}
		
		bool         after_incond   = player->m_Shared->InCond(cond);
		float        after_duration = player->m_Shared->GetConditionDuration(cond);
		CBaseEntity *after_provider = player->m_Shared->GetConditionProvider(cond);
		
		ModCommandResponse("[sig_addcond] Adding condition %s (%d) to player %s:\n"
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
	void CC_RemoveCond(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ModCommandResponse("[sig_removecond] Usage: any of the following:\n"
				"  sig_removecond <cond_number> | remove condition by number\n"
				"  sig_removecond <cond_name>   | remove condition by name\n"
				"  (condition names are \"TF_COND_*\"; look them up in tf.fgd or on the web)\n");
			return;
		}
		
		ETFCond cond = TF_COND_INVALID;
		if (StringToIntStrict(args[1], (int&)cond)) {
			if (!IsValidTFConditionNumber(cond)) {
				ModCommandResponse("[sig_removecond] Error: %s is not a valid condition number (valid range: 0-%d inclusive)\n", args[1], GetNumberOfTFConds() - 1);
				return;
			}
		} else {
			cond = GetTFConditionFromName(args[1]);
			if (!IsValidTFConditionNumber(cond)) {
				ModCommandResponse("[sig_removecond] Error: %s is not a valid condition name\n", args[1]);
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
		
		ModCommandResponse("[sig_removecond] Removing condition %s (%d) from player %s:\n"
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
	void CC_ListConds(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 1) {
			ModCommandResponse("[sig_listconds] Usage:  sig_listconds\n");
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
			ModCommandResponse("[sig_listconds] Player %s is currently in zero conditions\n", player->GetPlayerName());
			return;
		}
		
		ModCommandResponse("[sig_listconds] Player %s conditions:\n\n", player->GetPlayerName());
		
		width_cond     = std::max(width_cond + 4, strlen("CONDITION"));
		width_duration = std::max(width_duration, strlen("DURATION"));
		width_provider = std::max(width_provider, strlen("PROVIDER"));
		
		ModCommandResponse("%-*s  %-*s  %-*s\n",
			(int)width_cond,     "CONDITION",
			(int)width_duration, "DURATION",
			(int)width_provider, "PROVIDER");
		
		for (const auto& cond : conds) {
			ModCommandResponse("%-*s  %-*s  %-*s\n",
				(int)width_cond,     CFmtStr("%-3d %s", (int)cond.num, cond.str_name.c_str()).Get(),
				(int)width_duration, cond.str_duration.c_str(),
				(int)width_provider, cond.str_provider.c_str());
		}
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_SetHealth(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ModCommandResponse("[sig_sethealth] Usage: any of the following:\n"
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
			ModCommandResponse("[sig_sethealth] Error: '%s' is not a HP value or max-health/current-health percentage\n", args[1]);
			return;
		}
		
		ModCommandResponse("[sig_sethealth] Setting health of player %s to %d (previous health: %d).\n",
			player->GetPlayerName(), hp, player->GetHealth());
		
		player->SetHealth(hp);
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_AddHealth(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ModCommandResponse("[sig_addhealth] Usage: any of the following:\n"
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
			ModCommandResponse("[sig_addhealth] Error: '%s' is not a HP value or max-health/current-health percentage\n", args[1]);
			return;
		}
		
		ModCommandResponse("[sig_addhealth] Increasing health of player %s by %d (previous health: %d).\n",
			player->GetPlayerName(), hp, player->GetHealth());
		
		player->SetHealth(player->GetHealth() + hp);
	}
	
	
	// TODO: another version that allows affecting other players?
	void CC_SubHealth(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 2) {
			ModCommandResponse("[sig_subhealth] Usage: any of the following:\n"
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
			ModCommandResponse("[sig_subhealth] Error: '%s' is not a HP value or max-health/current-health percentage\n", args[1]);
			return;
		}
		
		ModCommandResponse("[sig_subhealth] Decreasing health of player %s by %d (previous health: %d).\n",
			player->GetPlayerName(), hp, player->GetHealth());
		
		player->SetHealth(player->GetHealth() - hp);
	}
	
	void CC_Animation(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() == 3) {

			int type = atoi(args[2]);
			ForEachTFPlayer([&](CTFPlayer *playerl){
				playerl->DoAnimationEvent( (PlayerAnimEvent_t) type /*17 PLAYERANIMEVENT_SPAWN*/, playerl->LookupSequence(args[1]) );
				
			});
		}
		
		if (args.ArgC() == 2) {
			//int sequence = playerl->LookupSequence(args[1]);
			
			//int arg1;
			//int arg2;
			//StringToIntStrict(args[1], arg1);
			//StringToIntStrict(args[2], arg2);
			//playerl->GetPlayerClass()->m_bUseClassAnimations = false;
			//playerl->ResetSequence(sequence);
			//playerl->GetPlayerClass() // //playerl->PlaySpecificSequence(args[1]);
			//TE_PlayerAnimEvent( playerl, 21 /*PLAYERANIMEVENT_SPAWN*/, sequence );
			ForEachTFPlayer([&](CTFPlayer *playerl){
			playerl->PlaySpecificSequence(args[1]);
			});
		}
		
	}

	void CC_Reset_Animation(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() != 1) {
			ModCommandResponse("[sig_subhealth] Usage: any of the following:\n"
				"  sig_subhealth <hp_value>    | decrease your health by the given HP value\n");
			return;
		}
		int seq = atoi(args[1]);
		int type = atoi(args[2]);
		ForEachTFPlayer([&](CTFPlayer *playerl){
			playerl->DoAnimationEvent( (PlayerAnimEvent_t) type /*17 PLAYERANIMEVENT_SPAWN*/, seq );
			
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

	void CC_Team(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 2) {
			ModCommandResponse("[sig_subhealth] Usage: any of the following:\n"
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

	void CC_GiveItem(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 2) {
			ModCommandResponse("[sig_subhealth] Usage: any of the following:\n"
				"  sig_subhealth <hp_value>    | decrease your health by the given HP value\n");
			return;
		}
		GiveItemByName(player, args[1], false, true);
		//engine->ServerCommand(CFmtStr("ce_mvm_equip_itemname %d \"%s\"\n", ENTINDEX(player), args[1]));
		//engine->ServerExecute();
	}

	void CC_GetMissingBones(CCommandPlayer *player, const CCommand& args)
	{
		for (int i = 1; i < 10; i++) {
			const char *robot_model = g_szBotModels[i];
			const char *player_model = CFmtStr("models/player/%s.mdl", g_aRawPlayerClassNamesShort[i]);
			studiohdr_t *studio_player = mdlcache->GetStudioHdr(mdlcache->FindMDL(player_model));
			studiohdr_t *studio_robot = mdlcache->GetStudioHdr(mdlcache->FindMDL(robot_model));
			//anim_robot->SetModel(robot_model);
			//anim_player->SetModel(player_model);

			if (studio_robot == nullptr) {
				ModCommandResponse("%s is null\n", robot_model);
				break;
			}
			else if (studio_player == nullptr) {
				ModCommandResponse("%s is null\n", player_model);
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

			ModCommandResponse("Missing bones on class %d\n", i);
			for (auto &string : bones) {
				ModCommandResponse("%s\n", string);
			}
		}
		//engine->ServerCommand(CFmtStr("ce_mvm_equip_itemname %d \"%s\"\n", ENTINDEX(player), args[1]));
		//engine->ServerExecute();
	}

	bool rtti_cast_sp(CBaseEntity *player) {
		return rtti_cast<CTFPlayer *>(player) != nullptr;
	}

	void get_extra_data(CBaseEntity *entity) {
		entity->GetEntityModule<HomingRockets>("homing")->speed=4;
	}
	CTFPlayer *playertrg = nullptr;
	std::string displaystr = "";

	PooledString pstr("pooled");
	CThreadLocalInt threadlocal;
	CThreadRWLock threadlock;
	void CC_Benchmark(CCommandPlayer *player, const CCommand& args)
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
			rtti_cast<CTFPlayer *>(target);
			timer.Start(); 
			for(int i = 0; i < times; i++) {
				check = rtti_cast<CTFPlayer *>(target);
			}
			timer.End();
			
			displaystr += CFmtStr("rtti cast time: %.9f", timer.GetDuration().GetSeconds());

			target->IsPlayer();
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
				check = rtti_cast<INextBot *>(bot) != nullptr;
			}
			timer.End();
			
			displaystr += CFmtStr("rtti cast time: %.9f", timer.GetDuration().GetSeconds());
			
			timer.Start(); 
			for(int i = 0; i < times; i++) {
				check = rtti_cast<CTFBot *>(bot) != nullptr;
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
			auto weapon = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_PRIMARY);
			auto weapon2 = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_SECONDARY);
			auto weapon3 = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_MELEE);
			auto weapon4 = GetEconEntityAtLoadoutSlot(player, LOADOUT_POSITION_ACTION);
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

			timer.Start(); 
			for(int i = 0; i < times; i++) {
				CAttributeManager::ft_AttribHookValue_float(1.0f, "mult_dmg", weapon, nullptr, true);
			}
			timer.End();
			
			displaystr += CFmtStr("wep single time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start(); 
			for(int i = 0; i < times; i++) {
				CAttributeManager::ft_AttribHookValue_int(1, "mult_dmg", weapon, nullptr, true);
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

			/*timer.Start(); 
			for(int i = 0; i < times; i++) {
				Mod::Attr::Custom_Attributes::GetFastAttributeFloat(weapon, 0.0f, Mod::Attr::Custom_Attributes::ADD_COND_ON_ACTIVE);
			}
			timer.End();
			
			displaystr += CFmtStr("\nfast wep single time: %.9f", timer.GetDuration().GetSeconds());

			timer.Start(); 
			for(int i = 0; i < times; i++) {
				Mod::Attr::Custom_Attributes::GetFastAttributeFloat(weapon, 0.0f, Mod::Attr::Custom_Attributes::ADD_COND_ON_ACTIVE);
				Mod::Attr::Custom_Attributes::GetFastAttributeFloat(weapon2, 0.0f, Mod::Attr::Custom_Attributes::ADD_COND_ON_ACTIVE);
				Mod::Attr::Custom_Attributes::GetFastAttributeFloat(weapon3, 0.0f, Mod::Attr::Custom_Attributes::ADD_COND_ON_ACTIVE);
				Mod::Attr::Custom_Attributes::GetFastAttributeFloat(weapon4, 0.0f, Mod::Attr::Custom_Attributes::ADD_COND_ON_ACTIVE);
			}
			timer.End();
			
			displaystr += CFmtStr(" fast wep multi time: %.9f", timer.GetDuration().GetSeconds());*/

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
			auto vtable_tf_player = RTTI::GetVTable<CTFPlayer>();

			DevMsg("rtti tf player %d %d, dereferenced player %d, dereferenced player -4 %d, dereferenced player +4 %d\n", vtable_tf_player, rtti_tf_player, *(int *)player, *((int *)player-1),*((int *)player+1));
			DevMsg("dereferenced player vtable %d, dereferenced player vtable -4 %d, dereferenced player vtable +4 %d\n", **((int**)player), (int) *(*((rtti_t***)player)-1),*(*((int**)player)+1));
			
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

			ModCommandResponse("%s", displaystr.c_str());
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

			ModCommandResponse("%s", displaystr.c_str());
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
		else if (strcmp(args[2], "prop") == 0) {
			timer.Start();
			for(int i = 0; i < times; i++) {
				int skin = player->m_nBotSkill;
			}
			timer.End();
			
			displaystr += CFmtStr("prop get: %.9f", timer.GetDuration().GetSeconds());
		}
		else if (strcmp(args[2], "entindex") == 0) {
			timer.Start();
			int entindex = 0;
			for(int i = 0; i < times; i++) {
				entindex = ENTINDEX_NATIVE(player);
			}
			timer.End();
			
			displaystr += CFmtStr("entindex: %.9f", timer.GetDuration().GetSeconds());
			timer.Start();
			for(int i = 0; i < times; i++) {
				entindex = ENTINDEX(player);
			}
			timer.End();
			
			displaystr += CFmtStr("entindex %d native: %.9f", entindex, timer.GetDuration().GetSeconds());
		}
		else if (strcmp(args[2], "extradata") == 0) {
			CBaseEntity *gamerules = servertools->FindEntityByClassname(nullptr, "tf_gamerules");

			GetExtraProjectileData(reinterpret_cast<CBaseProjectile *>(gamerules))->homing = new HomingRockets();
			gamerules->GetOrCreateEntityModule<HomingRockets>("homing");
			timer.Start();
			for(int i = 0; i < times; i++) {
				GetExtraProjectileData(reinterpret_cast<CBaseProjectile *>(gamerules))->homing->speed=4;
			}
			timer.End();
			displaystr += CFmtStr("extra data time: %.9f", timer.GetDuration().GetSeconds());
			timer.Start();
			for(int i = 0; i < times; i++) {
				gamerules->GetEntityModule<HomingRockets>("homing")->speed=4;
			}
			timer.End();
			displaystr += CFmtStr("entity module time: %.9f", timer.GetDuration().GetSeconds());
			

		}
		else if (strcmp(args[2], "findentity") == 0) {
			
			timer.Start();
			for(int i = 0; i < times; i++) {
			for (CBaseEntity *targetEnt = nullptr; (targetEnt = servertools->FindEntityByName(targetEnt, "somename", player, player, player)) != nullptr ;) {
                
            }
			}
			timer.End();
			displaystr += CFmtStr("find entity by name time: %.9f\n", timer.GetDuration().GetSeconds());
			timer.Start();
			for(int i = 0; i < times; i++) {
			for (CBaseEntity *targetEnt = nullptr; (targetEnt = servertools->FindEntityByClassname(targetEnt, "somename")) != nullptr ;) {
                
            }
			}
			timer.End();
			displaystr += CFmtStr("find entity by classname time: %.9f\n", timer.GetDuration().GetSeconds());
			timer.Start();
			for(int i = 0; i < times; i++) {
			for (CBaseEntity *targetEnt = nullptr; (targetEnt = servertools->FindEntityGeneric(targetEnt, "somename")) != nullptr ;) {
                
            }
			}
			timer.End();
			displaystr += CFmtStr("find entity generic time: %.9f\n", timer.GetDuration().GetSeconds());
			timer.Start();
			for(int i = 0; i < times; i++) {
			for (CBaseEntity *targetEnt = nullptr; (targetEnt = servertools->FindEntityByName(targetEnt, "somename*", player, player, player)) != nullptr ;) {
                
            }
			}
			timer.End();
			displaystr += CFmtStr("find entity by name wildcard time: %.9f\n", timer.GetDuration().GetSeconds());
		}
		else if (strcmp(args[2], "locks") == 0) {
			
			timer.Start();
			for(int i = 0; i < times; i++) {
				threadlock.LockForWrite();
				threadlock.UnlockWrite();
			}
			timer.End();
			displaystr += CFmtStr("locks time: %.9f\n", timer.GetDuration().GetSeconds());
		}
		else if (strcmp(args[2], "threadlocal") == 0) {
			
			timer.Start();
			for(int i = 0; i < times; i++) {
				int a = threadlocal.Get();
			}
			timer.End();
			displaystr += CFmtStr("threadlocal time: %.9f\n", timer.GetDuration().GetSeconds());
		}
		else if (strcmp(args[2], "threadlocalmine") == 0) {
			static pthread_key_t index;
			static int done = pthread_key_create( &index, NULL );
			pthread_setspecific(index, (const void *)8878);
			timer.Start();
			for(int i = 0; i < times; i++) {
				int a = (int) pthread_getspecific(index);
			}
			timer.End();
			displaystr += CFmtStr("threadlocalmine time: %.9f\n", timer.GetDuration().GetSeconds());
		}
		ModCommandResponse("%s", displaystr.c_str());
	}

	void CC_Taunt(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 3) {
			ModCommandResponse("[sig_taunt] Usage: any of the following:\n"
				"  sig_taunt <target> <tauntname> [attribute] [value] ...   | start taunt of name\n");
			return;
		}

		auto item_def = GetItemSchema()->GetItemDefinitionByName(args[2]);
		if (item_def == nullptr) {
			ModCommandResponse("[sig_taunt] Error: no matching item \"%s\".\n", args[2]);
			return;
		}

		CEconItemView *view = CEconItemView::Create();
		view->Init(item_def->m_iItemDefIndex, 6, 9999, 0);
#ifndef NO_MVM
		Mod::Pop::PopMgr_Extensions::AddCustomWeaponAttributes(args[2], view);
#endif
		for (int i = 3; i < args.ArgC() - 1; i+=2) {
			CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(args[i]);
			if (attr_def == nullptr) {
				int idx = -1;
				if (StringToIntStrict(args[i], idx)) {
					attr_def = GetItemSchema()->GetAttributeDefinition(idx);
				}
			}

			if (attr_def != nullptr) {
				view->GetAttributeList().AddStringAttribute(attr_def, args[i + 1]);
				ModCommandResponse("[sig_taunt] Added attribute \"%s\" = \"%s\". \n", attr_def->GetName(), args[i + 1]);
			}
		}

		std::vector<CBasePlayer *> vec;
		GetSMTargets(player, args[1], vec);
		if (vec.empty()) {
			ModCommandResponse("[sig_taunt] Error: no matching target found for \"%s\".\n", args[1]);
			return;
		}
		for (CBasePlayer *target : vec) {
			ToTFPlayer(target)->PlayTauntSceneFromItem(view);
		}
		CEconItemView::Destroy(view);
	}
	
	void CC_AddAttr(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 4) {
			ModCommandResponse("[sig_addattr] Usage: any of the following:\n"
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
			ModCommandResponse("[sig_addattr] Error: no matching target found for \"%s\".\n", args[1]);
			return;
		}

		for (CBasePlayer *target : vec) {
			CAttributeList *list = nullptr;
			if (slot != -1) {
				CEconEntity *item = GetEconEntityAtLoadoutSlot(ToTFPlayer(target), slot);
				if (item != nullptr && item->GetItem() != nullptr) {
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
					ModCommandResponse("[sig_addattr] Error: couldn't find any attributes in the item schema matching \"%s\".\n", args[2]);
					return;
				}
				list->AddStringAttribute(attr_def, args[3]);
			}
			else {
				ModCommandResponse("[sig_addattr] Error: couldn't find any item in slot\"%d\".\n", slot);
				return;
			}
		}
		ModCommandResponse("[sig_addattr] Successfully added attribute %s = %s to %s.\n", args[2], args[3], target_names);
	}

	bool allow_create_dropped_weapon = false;
	void CC_DropItem(CCommandPlayer *player, const CCommand& args)
	{
		std::vector<CBasePlayer *> vec;
		GetSMTargets(player, args[1], vec);
		if (vec.empty()) {
			ModCommandResponse("[sig_dropitem] Error: no matching target found for \"%s\".\n", args[1]);
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
				ModCommandResponse("[sig_dropitem] Error: couldn't find any item.\n");
			}
		}
	}
	std::map<std::string, int> modelmap;
	std::map<std::string, int> modelmapmedal;
	std::vector<CBasePlayer *> modelplayertargets;

	void CC_GiveEveryItem(CCommandPlayer *player, const CCommand& args)
	{
		GetSMTargets(player, args[1], modelplayertargets);
		if (modelplayertargets.empty()) {
			ModCommandResponse("[sig_giveeveryitem] Error: no matching target found for \"%s\".\n", args[1]);
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

		ModCommandResponse("items: %d\n", modelmap.size());
		ModCommandResponse("items medals: %d\n", modelmapmedal.size());
	}
	
	void CC_DumpInventory(CCommandPlayer *player, const CCommand& args)
	{
	}

	void CC_DumpItems(CCommandPlayer *player, const CCommand& args)
	{
		std::vector<CBasePlayer *> vec;
		GetSMTargets(player, args[1], vec);
		if (vec.empty()) {
			ModCommandResponse("[sig_listitemattr] Error: no matching target found for \"%s\".\n", args[1]);
			return;
		}
		std::string displaystr;
		for (CBasePlayer *target : vec) {
			ForEachTFPlayerEconEntity(ToTFPlayer(target), [&](CEconEntity *entity) {
				CEconItemView *view = entity->GetItem();
				if (view != nullptr) {
					auto &attrs = view->GetAttributeList().Attributes();

					ModCommandResponse("Item %s:\n", view->GetItemDefinition()->GetName());

					class Wrapper : public IEconItemAttributeIterator
					{
					public:

						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, unsigned int                             value) const
						{
							attribute_data_union_t valueu;
							valueu.m_UInt = value;
							ModCommandResponse("%s = %f (%d) (%.17en)\n", pAttrDef->GetName(), valueu.m_Float, valueu.m_UInt, valueu.m_Float);
							return true;
						}
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, float                                    value) const { return true; }
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const uint64&                            value) const { return true; }
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String&                 value) const
						{
							const char *pstr;
							CopyStringAttributeValueToCharPointerOutput(&value, &pstr);
							ModCommandResponse("%s = %s\n", pAttrDef->GetName(), pstr);
							return true;
						}

						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value) const { return true; }
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_ItemSlotCriteria&       value) const { return true; }
						virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_WorldItemPlacement&     value) const { return true; }
					};

					Wrapper wrapper;
					view->IterateAttributes(&wrapper);
					ModCommandResponse("\n");
				}
			});
		}
		ModCommandResponse("%s", displaystr.c_str());
	}

	void CC_Sprays(CCommandPlayer *player, const CCommand& args)
	{
		for (int i = 0; i < sv->GetClientCount(); i++) {
			CBaseClient *client = static_cast<CBaseClient *> (sv->GetClient(i));
			if (client != nullptr) {
				ModCommandResponse("Spray apply %x %d %d\n", client->m_nCustomFiles[0].crc, client->m_nClientSlot, client->m_nEntityIndex);
			}
		}
	}

	void CC_Vehicle(CCommandPlayer *player, const CCommand& args)
	{
		const char *type = "car";
		const char *model = "models/buggy.mdl";
		const char *script = "scripts/vehicles/jeep_test.txt";
		if (args.ArgC() >= 2) {
			type = args[1];
		}
		
		CPropVehicleDriveable *vehicle = reinterpret_cast<CPropVehicle *>(CreateEntityByName("prop_vehicle_driveable"));
		if (vehicle != nullptr) {
			DevMsg("Vehicle spawned\n");
			vehicle->SetAbsOrigin(player->GetAbsOrigin() + Vector(0,0,100));
			vehicle->KeyValue("actionScale", "1");
			vehicle->KeyValue("spawnflags", "1");
			if (FStrEq(type, "car")) {
				vehicle->m_nVehicleType = 1;
				script = "scripts/vehicles/jeep_test.txt";
				model = "models/buggy.mdl";
			}
			else if (FStrEq(type, "car_raycast")) {
				vehicle->m_nVehicleType = 2;
				script = "scripts/vehicles/jeep_test.txt";
				model = "models/buggy.mdl";
			}
			else if (FStrEq(type, "jetski")) {
				vehicle->m_nVehicleType = 4;
				script = "scripts/vehicles/jetski.txt";
				model = "models/airboat.mdl";
			}
			else if (FStrEq(type, "airboat")) {
				vehicle->m_nVehicleType = 8;
				script = "scripts/vehicles/airboat.txt";
				model = "models/airboat.mdl";
			}

			if (args.ArgC() >= 3) {
				model = args[2];
			}
			if (args.ArgC() >= 4) {
				script = args[3];
			}

			vehicle->KeyValue("model", model);
			vehicle->KeyValue("vehiclescript", script);
			vehicle->Spawn();
			vehicle->Activate();
			vehicle->m_flMinimumSpeedToEnterExit = 100;
		}
	}
	
	void CC_PlayScene(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() == 2) {
			ForEachTFPlayer([&](CTFPlayer *playerl){
				InstancedScriptedScene(playerl, args[1], nullptr, 0, false, nullptr, true, nullptr );
				//playerl->PlayScene(args[1]);
			});
		}
		
	}
	
	void CC_ClientCvar(CCommandPlayer *player, const CCommand& args)
	{
		ModCommandResponse("Command %d\n", args.ArgC());
		if (args.ArgC() == 2) {
			ModCommandResponse("%s\n", engine->GetClientConVarValue(ENTINDEX(player), args[1]));
		}
	}

	void CC_DropMarker(CCommandPlayer *player, const CCommand& args)
	{
		if (args.ArgC() < 2) return;

		std::vector<CBasePlayer *> vec;
		GetSMTargets(player, args[1], vec);
		if (vec.empty()) {
			return;
		}
		std::string displaystr;
		for (CBasePlayer *target : vec) {
			CTFReviveMarker::Create(ToTFPlayer(target));
		}
	}

    namespace {
        // idk how to specify the keyequal argument with unordered_set so
        std::set<
            CConVarOverride_Flags,
            decltype([](const CConVarOverride_Flags& l, 
                        const CConVarOverride_Flags& r){
                return (std::strcmp(
                            l.m_pszConVarName,
                            r.m_pszConVarName)
                       ) < 0;
            })> cvar_overrides{};
        std::vector<std::unique_ptr<char[]>> cvar_strs{};
    }

    void CC_CvarNoCheat(CTFPlayer* player, const CCommand& args)
    {
        const auto usage{[&player]{
            ModCommandResponse(
                    "[sig_nocheat] Removes/toggles cheat flag on a cvar\n"
                    "[sig_nocheat] Usage: sig_nocheat <cvar> [cvars...]\n");
        }};
        if(args.ArgC() < 2){ usage(); return; }
        for(int i{1}; i < args.ArgC(); ++i){
            auto cvar_str{std::make_unique<char[]>(std::strlen(args[i]) + 1)};
            std::strcpy(cvar_str.get(), args[i]);
            CConVarOverride_Flags cvar{cvar_str.get(), FCVAR_NONE, FCVAR_CHEAT};
            if(!cvar.IsValid()){
                ModCommandResponse(
                        "[sig_nocheat] Unknown cvar %s\n", args[i]);
                continue;
            }
            auto[it, inserted]{cvar_overrides.insert(std::move(cvar))};
            const bool removed{it->Toggle()};
            if(removed) 
                ModCommandResponse("[sig_nocheat] Toggled %s (on)\n", args[i]);
            else
                ModCommandResponse("[sig_nocheat] Toggled %s (off)\n", args[i]);
            if(inserted)
                cvar_strs.push_back(std::move(cvar_str));
        }
    }

    void CC_CvarNoCheatList(CTFPlayer* player, const CCommand& args)
    {
        auto enabled_count{std::count_if(
            cvar_overrides.begin(),
            cvar_overrides.end(),
            [](const auto& c){
                return c.IsEnabled();
            })
        };
        ModCommandResponse("[sig_nocheat_list] Overridden cvars: %d (%d on)\n",
                cvar_overrides.size(), enabled_count);
        for(const auto& c : cvar_overrides){
            ModCommandResponse("%s (%s)\n", c.m_pszConVarName, 
                    c.IsEnabled() ? "on" : "off");
        }
    }

    void CC_Expression(CTFPlayer* player, const CCommand& args)
    {
		if (args.ArgC() < 2) {
			ModCommandResponse("[sig_expresion] Usage: sig_expression <expression>\n");
			return;
		}
		variant_t value;
		variant_t value2;
		Evaluation expr(value);
		expr.Evaluate(args[1], player, player, player, value2);
		ModCommandResponse("Result: %s\n", value.String());
    }

	void CC_Expression_Func(CTFPlayer* player, const CCommand& args)
    {
		std::vector<std::string> functions;
		std::string buf;
		Evaluation::GetFunctionInfoStrings(functions);
		for (auto &function : functions) {
			if (buf.size() + function.size() > 1000) {
				ModCommandResponse("%s", buf);
				buf.clear();
			}
			buf += function;
		}
		ModCommandResponse("%s", buf);
	}

	void CC_Expression_Vars(CTFPlayer* player, const CCommand& args)
    {
		std::vector<std::string> vars;
		std::string buf;
		Evaluation::GetVariableStrings(vars);
		for (auto &var : vars) {
			if (buf.size() + var.size() > 1000) {
				ModCommandResponse("%s", buf);
				buf.clear();
			}
			buf += var;
		}
		ModCommandResponse("%s", buf);
	}

	int cpu_show_player[MAX_PLAYERS + 1] {};
	float cpu_usage = 0;
	void CC_Cpu_Usage(CTFPlayer* player, const CCommand& args)
    {
		if (args.ArgC() < 2) {
			ModCommandResponse("[sig_expresion] Usage: sig_cpu_usage [never|default|always]\n");
			return;
		}
		if (FStrEq(args[1], "never")) {
			cpu_show_player[ENTINDEX(player)] = 2;
		}
		else if (FStrEq(args[1], "default")) {
			cpu_show_player[ENTINDEX(player)] = 0;
		}
		else if (FStrEq(args[1], "always")) {
			cpu_show_player[ENTINDEX(player)] = 1;
		}
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
			MOD_ADD_DETOUR_STATIC(CTFDroppedWeapon_Create, "CTFDroppedWeapon::Create");
		}
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void FrameUpdatePostEntityThink() override
		{
			static int lastCheck = 0;
			static struct rusage s_lastUsage;
			struct rusage currentUsage;
			if (gpGlobals->curtime < lastCheck || (int)gpGlobals->curtime > lastCheck) {
				lastCheck = gpGlobals->curtime;
				
				if ( getrusage( RUSAGE_SELF, &currentUsage ) == 0 )
				{
					double flTimeDiff = (currentUsage.ru_utime.tv_sec + (currentUsage.ru_utime.tv_usec / 1000000.0)) - (s_lastUsage.ru_utime.tv_sec + (s_lastUsage.ru_utime.tv_usec / 1000000.0)) + 
						(currentUsage.ru_stime.tv_sec + (currentUsage.ru_stime.tv_usec / 1000000.0)) - (s_lastUsage.ru_stime.tv_sec + (s_lastUsage.ru_stime.tv_usec / 1000000.0));
					cpu_usage = flTimeDiff;

					s_lastUsage = currentUsage;

					extern ConVar cvar_cpushowlevel;
					bool highCpu = cvar_cpushowlevel.GetInt() < (int) (cpu_usage * 100);
					bool highEdict = engine->GetEntityCount() /* gEntList->m_iNumEdicts*/ > 1900;
					bool highEnt = (gEntList->m_iNumEnts - gEntList->m_iNumEdicts) > 4500;

					hudtextparms_t textparam;
					textparam.channel = 5;
					textparam.x = 1.0f;
					textparam.y = 0.0f;
					textparam.effect = 0;
					textparam.r1 = 255;
					textparam.r2 = 255;
					textparam.b1 = highCpu ? 255 : 255;
					textparam.b2 = highCpu ? 255 : 255;
					textparam.g1 = highCpu ? 255 : 255;
					textparam.g2 = highCpu ? 255 : 255;
					textparam.a1 = 0;
					textparam.a2 = 0; 
					textparam.fadeinTime = 0.f;
					textparam.fadeoutTime = 0.f;
					textparam.holdTime = 1.1f;
					textparam.fxTime = 1.0f;
					
					for (int i = 1; i < ARRAYSIZE(cpu_show_player); i++) {
						auto player = UTIL_PlayerByIndex(i);
						if (player != nullptr && !player->IsBot() && cpu_show_player[i] != 2 && PlayerIsSMAdmin(player) && (highCpu || highEdict || highEnt || cpu_show_player[i] == 1)) {
							std::string str;
							if (highCpu || cpu_show_player[i] == 1) {
								str += fmt::format("CPU Usage {:.1f}%\n", (float)(cpu_usage * 100));
							}
							if (highEdict || cpu_show_player[i] == 1) {
								str += fmt::format("Networked Entities: {}/2048\n", engine->GetEntityCount()/*gEntList->m_iNumEdicts*/);
							}
							if (highEnt || cpu_show_player[i] == 1) {
								str += fmt::format("Logic Entities: {}/6143\n", gEntList->m_iNumEnts - gEntList->m_iNumEdicts);
							}
							DisplayHudMessageAutoChannel(player, textparam, str.c_str(), 2);
						}
						if (player == nullptr || player->IsBot()) {
							cpu_show_player[i] = 0;
						}
					}
					
				}
			}

			if (!modelplayertargets.empty() && !modelmap.empty()) {
				for (CBasePlayer *target : modelplayertargets) {
					int amount = 0;
					if (engine->GetEntityCount() > 1920) {
						modelmap.clear();
						break;
					}

					CEconWearable *wearable = static_cast<CEconWearable *>(ItemGeneration()->SpawnItem(modelmap.begin()->second, Vector(0,0,0), QAngle(0,0,0), 6, 9999, "tf_wearable"));
					if (wearable != nullptr) {
						
						wearable->m_bValidatedAttachedEntity = true;
						wearable->GiveTo(target);
						target->EquipWearable(wearable);
					}
				}
				if (!modelmap.empty())
					modelmap.erase(modelmap.begin());
			}


		}
	};
	CMod s_Mod;
	
	/* default: admin-only mode ENABLED */
	ConVar cvar_adminonly("sig_util_client_cmds_adminonly", "1", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Utility: restrict this mod's functionality to SM admins only");

	class ClientCmdsCommand : public ModCommand
	{
	public:
		ClientCmdsCommand(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

		virtual bool CanPlayerCall(CCommandPlayer *player) { return player == nullptr || !cvar_adminonly.GetBool() || PlayerIsSMAdminOrBot(player); }
	};

	class ClientCmdsCommandClient : public ModCommand
	{
	public:
		ClientCmdsCommandClient(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

		virtual bool CanPlayerCall(CCommandPlayer *player) { return player != nullptr && (!cvar_adminonly.GetBool() || PlayerIsSMAdminOrBot(player)); }
	};

	ClientCmdsCommandClient sig_setplayerscale("sig_setplayerscale", CC_SetPlayerScale, &s_Mod);
	ClientCmdsCommandClient sig_setplayermodel("sig_setplayermodel", CC_SetPlayerModel, &s_Mod);
	ClientCmdsCommandClient sig_resetplayermodel("sig_resetplayermodel", CC_ResetPlayerModel, &s_Mod);
	ClientCmdsCommandClient sig_unequip("sig_unequip", CC_UnEquip, &s_Mod);
	ClientCmdsCommandClient sig_addcond("sig_addcond", CC_AddCond, &s_Mod);
	ClientCmdsCommandClient sig_removecond("sig_removecond", CC_RemoveCond, &s_Mod);
	ClientCmdsCommandClient sig_listconds("sig_listconds", CC_ListConds, &s_Mod);
	ClientCmdsCommandClient sig_sethealth("sig_sethealth", CC_SetHealth, &s_Mod);
	ClientCmdsCommandClient sig_animation("sig_animation", CC_Animation, &s_Mod);
	ClientCmdsCommandClient sig_resetanim("sig_resetanim", CC_Reset_Animation, &s_Mod);
	ClientCmdsCommandClient sig_teamset("sig_teamset", CC_Team, &s_Mod);
	ClientCmdsCommandClient sig_giveitemcreator("sig_giveitemcreator", CC_GiveItem, &s_Mod);
	ClientCmdsCommand sig_benchmark("sig_benchmark", CC_Benchmark, &s_Mod);
	ClientCmdsCommand sig_taunt("sig_taunt", CC_Taunt, &s_Mod);
	ClientCmdsCommand sig_addattr("sig_addattr", CC_AddAttr, &s_Mod);
	ClientCmdsCommand sig_dropitem("sig_dropitem", CC_DropItem, &s_Mod);
	ClientCmdsCommand sig_giveeveryitem("sig_giveeveryitem", CC_GiveEveryItem, &s_Mod);
	ClientCmdsCommand sig_getmissingbones("sig_getmissingbones", CC_GetMissingBones, &s_Mod);
	ClientCmdsCommand sig_listitemattr("sig_listitemattr", CC_DumpItems, &s_Mod);
	ClientCmdsCommand sig_sprays("sig_sprays", CC_Sprays, &s_Mod);
	ClientCmdsCommandClient sig_vehicle("sig_vehicle", CC_Vehicle, &s_Mod);
	ClientCmdsCommand sig_playscene("sig_playscene", CC_PlayScene, &s_Mod);
	ClientCmdsCommandClient sig_getclientcvar("sig_getclientcvar", CC_ClientCvar, &s_Mod);
	ClientCmdsCommand sig_dropmarker("sig_dropmarker", CC_DropMarker, &s_Mod);
	ClientCmdsCommand sig_nocheat("sig_nocheat", CC_CvarNoCheat, &s_Mod);
	ClientCmdsCommand sig_nocheat_list("sig_nocheat_list", CC_CvarNoCheatList, &s_Mod);
	ClientCmdsCommand sig_expression("sig_expression", CC_Expression, &s_Mod);
	ClientCmdsCommand sig_expression_functions("sig_expression_functions", CC_Expression_Func, &s_Mod);
	ClientCmdsCommand sig_expression_vars("sig_expression_vars", CC_Expression_Vars, &s_Mod);
	ModCommandDebug sig_cpu_usage("sig_cpu_usage", CC_Cpu_Usage, &s_Mod);
	
	/* by way of incredibly annoying persistent requests from Hell-met,
	 * I've acquiesced and made this mod convar non-notifying (sigh) */
	ConVar cvar_enable("sig_util_client_cmds", "0", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Utility: enable client cheat commands",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

	ConVar cvar_cpushowlevel("sig_util_client_cmds_show_cpu_min_level", "99", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Minimum cpu usage percentage to start displaying to admins by default");
}
