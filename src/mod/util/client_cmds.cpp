#include "mod.h"
#include "stub/econ.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/objects.h"
#include "util/admin.h"
#include "util/clientmsg.h"
#include "util/misc.h"


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
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Client_Cmds")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand, "CTFPlayer::ClientCommand");
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
