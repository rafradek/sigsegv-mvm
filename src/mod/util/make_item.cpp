#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/econ.h"
#include "stub/misc.h"
#include "util/misc.h"
#include "util/clientmsg.h"
#include "util/admin.h"
#include "mod/common/commands.h"


// TODO 20170825:
// - client message acknowledging a successful give
// - fix integer parsing issue ("1" --> some very big number)
// - help command
// - give for players other than self
// - maybe: allow referring to attributes by attribute_class
//   - have an associative array of <attr_class string> --> <list of CEconItemAttributeDefinition ptrs>
//   - pre-load the array once initially by iterating over all CEconItemAttributeDefinition's
//   - see how reasonable or bad the "multiple attr defs have the same attr class" situation is
//     - keep in mind that we can't necessarily just pick one attr def arbitrarily,
//       since the attr value will either stack or not stack with existing attr values depending on
//       whether the attr def itself matches, not the attr class
// - maybe: make a way to apply unimplemented attributes to items
//   - if we do this by adding our own attr defs to the schema, then we may need to hack the networking code for
//     CAttributeList so that the client doesn't see our custom-made ones
// - maybe: user restriction functionality (so everyone doesn't use this mod willy-nilly)


#warning WARNING: as of 20180708, `sig_makeitem_add_attr "fire rate penalty" 2.5` reliably crashes via _int_free/malloc_printerr!!!
// "double free or corruption (out)"
// CRASH STACK:
// raise
// abort
// __libc_message
// malloc_printerr
// _int_free
// std::basic_string<char, std::char_traits<char>, std::allocator<char> >::_Rep::_M_dispose(std::allocator<char> const&)
// std::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace_safe(unsigned int, unsigned int, char const*, unsigned int)
// std::basic_string<char, std::char_traits<char>, std::allocator<char> >::assign(char const*, unsigned int)
// CSchemaAttributeType_Default::ConvertEconAttributeValueToString(CEconItemAttributeDefinition const*, attribute_data_union_t const&, std::basic_string<char, std::char_traits<char>, std::allocator<char> >*) const
// Mod_Util_Make_Item::CC_AddAttr(CTFPlayer*, CCommand const&)


namespace Mod::Util::Make_Item
{
	const CSteamID GetCommandClientSteamID(const char *func, CTFPlayer *player)
	{
		CSteamID steamid;
		if (player != nullptr) {
			auto steamidptr = engine->GetClientSteamID(player->edict());
			if (steamidptr != nullptr) {
				steamid = *steamidptr;
			}
		}
		return steamid;
	}
	
	
	class CEconItemViewWrapper
	{
	public:
		CEconItemViewWrapper()
		{
			this->m_pEconItemView = CEconItemView::Create();
		}
		~CEconItemViewWrapper()
		{
			// the attr value needs to be deallocated AT SOME POINT to avoid leaks...
			// but how do we both allow for items to persist past mod unload, yet not leak memory?
		//	for (auto& attr : this->m_pEconItemView->GetAttributeList().m_Attributes) {
		//		attr.GetStaticData()->GetType()->UnloadEconAttributeValue(attr.GetValuePtr());
		//	}
			
			CEconItemView::Destroy(this->m_pEconItemView);
		}
		
		CEconItemViewWrapper(const CEconItemViewWrapper&)                        = delete;
		const CEconItemViewWrapper& operator=(const CEconItemViewWrapper&) const = delete;
		
		operator CEconItemView *() { return this->m_pEconItemView; }
		CEconItemView *operator*() { return this->m_pEconItemView; }
		
	private:
		CEconItemView *m_pEconItemView;
	};
	
	static std::map<CSteamID, CEconItemViewWrapper> state;
	
	
	void CC_Help(CCommandPlayer *player, const CCommand& args)
	{
		// TODO
		// show usage info for all commands
		// as well as examples, probably
	}
	
	
	void CC_Clear(CCommandPlayer *player, const CCommand& args)
	{
		auto steamid = GetCommandClientSteamID("CC_Clear", player);
		
		auto it = state.find(steamid);
		if (it == state.end()) {
			ModCommandResponse("[sig_makeitem_clear] No item is in progress; nothing to clear.\n");
			return;
		}
		
		CEconItemView *item_view = (*it).second;
		if (item_view != nullptr) {
			ModCommandResponse("[sig_makeitem_clear] Clearing out information for unfinished item \"%s\".\n", item_view->GetStaticData()->GetName());
		}
		
		state.erase(it);
	}
	

	void Start(CTFPlayer *player, const char *name, CSteamID steamid)
	{
		auto it = state.find(steamid);
		if (it != state.end()) {
			CEconItemView *item_view = (*it).second;
			if (item_view != nullptr) {
				ModCommandResponse("[sig_makeitem_start] Warning: discarding information for unfinished item \"%s\".\n", item_view->GetStaticData()->GetName());
			}
			
			state.erase(it);
		}
		
		/* attempt lookup first by item name, then by item definition index */
		auto item_def = rtti_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(name));
		if (item_def == nullptr) {
			int idx = -1;
			if (StringToIntStrict(name, idx)) {
				item_def = FilterOutDefaultItemDef(rtti_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinition(idx)));
			}
		}
		
		if (item_def == nullptr) {
			ModCommandResponse("[sig_makeitem_start] Error: couldn't find any items in the item schema matching \"%s\".\n", name);
			return;
		}
		
		/* insert new element into the map */
		CEconItemView *item_view = state[steamid];
		item_view->Init(item_def->m_iItemDefIndex, item_def->m_iItemQuality, 9999, 0);
		
		ModCommandResponse("\n[sig_makeitem_start] Started making item \"%s\".\n\n", item_def->GetName());
	}
	
	void CC_Start(CCommandPlayer *player, const CCommand& args)
	{
		auto steamid = GetCommandClientSteamID("CC_Start", player);
		
		if (args.ArgC() != 2) {
			ModCommandResponse("[sig_makeitem_start] Usage: any of the following:\n"
				"  sig_makeitem_start <item_name>      | item names that include spaces need quotes\n"
				"  sig_makeitem_start <item_def_index> | item definition indexes can be found in the item schema\n");
			return;
		}
		Start(player, args[1], steamid);
	} 
	
	void AddAttr(CTFPlayer *player, const char *name, const char *value)
	{
		auto steamid = GetCommandClientSteamID("CC_AddAttr", player);
		
		auto it = state.find(steamid);
		if (it == state.end()) {
			ModCommandResponse("[sig_makeitem_add_attr] Error: you need to do sig_makeitem_start before you can do sig_makeitem_add_attr.\n");
			return;
		}
		
		CEconItemView *item_view = (*it).second;
		
		/* attempt lookup first by attr name, then by attr definition index */
		CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(name);
		if (attr_def == nullptr) {
			int idx = -1;
			if (StringToIntStrict(name, idx)) {
				attr_def = GetItemSchema()->GetAttributeDefinition(idx);
			}
		}
		
		if (attr_def == nullptr) {
			ModCommandResponse("[sig_makeitem_add_attr] Error: couldn't find any attributes in the item schema matching \"%s\".\n", name);
			return;
		}
		item_view->GetAttributeList().AddStringAttribute(attr_def, value);

		ModCommandResponse("[sig_makeitem_add_attr] Added attribute \"%s\" with value \"%s\".\n", attr_def->GetName(), value);
	}
	
	void CC_AddAttr(CCommandPlayer *player, const CCommand& args)
	{

		if (args.ArgC() != 3) {
			ModCommandResponse("[sig_makeitem_add_attr] Usage: any of the following:\n"
				"  sig_makeitem_add_attr <attr_name> <value>      | attribute names that include spaces need quotes\n"
				"  sig_makeitem_add_attr <attr_def_index> <value> | attribute definition indexes can be found in the item schema\n");
			return;
		}
		AddAttr(player, args[1], args[2]);
	}

	void Give_Common(CTFPlayer *player, CTFPlayer *recipient, const char *cmd_name, CSteamID steamid, bool no_remove, const char* custom_class)
	{
		// possible ways to use this command:
		// - 0 args: give to me
		// - 1 args: give to steam ID, or user ID, or player name exact match, or player name unique-partial-match
		
		auto it = state.find(steamid);
		if (it == state.end()) {
			ModCommandResponse("[%s] Error: you need to do sig_makeitem_start before you can do %s.\n", cmd_name, cmd_name);
			return;
		}
		
		CEconItemView *item_view = (*it).second;
		
		int slot = item_view->GetStaticData()->GetLoadoutSlot(recipient->GetPlayerClass()->GetClassIndex());
		if (slot == -1) {
			slot = item_view->GetStaticData()->GetLoadoutSlot(TF_CLASS_UNDEFINED);
			if (slot == -1) {
				ModCommandResponse("[%s] WARNING: failed to determine this item's loadout slot for the current player class; weird things may occur.\n", cmd_name);
			} else {
				ModCommandResponse("[%s] WARNING: using best-guess loadout slot #%d (\"%s\"). Not guaranteed to work perfectly for this class.\n", cmd_name, slot, GetLoadoutSlotName(slot));
			}
		}
		
		if (!no_remove) {
			if (IsLoadoutSlot_Cosmetic(static_cast<loadout_positions_t>(slot))) {
				/* equip-region-conflict-based old item removal */
				
				unsigned int mask1 = item_view->GetStaticData()->GetEquipRegionMask();
				
				for (int i = recipient->GetNumWearables() - 1; i >= 0; --i) {
					CEconWearable *wearable = recipient->GetWearable(i);
					if (wearable == nullptr) continue;
					
					unsigned int mask2 = wearable->GetAttributeContainer()->GetItem()->GetStaticData()->GetEquipRegionMask();
					
					if ((mask1 & mask2) != 0) {
						ModCommandResponse("[%s] Removing conflicting wearable \"%s\". (Equip group info: old %08x, new %08x, overlap %08x)\n",
							cmd_name, wearable->GetAttributeContainer()->GetItem()->GetStaticData()->GetName(), mask2, mask1, (mask1 & mask2));
						recipient->RemoveWearable(wearable);
					}
				}
			} else {
				/* slot-based old item removal */
				
				CEconEntity *old_econ_entity = nullptr;
				(void)CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot(recipient, slot, &old_econ_entity);
				
				if (old_econ_entity != nullptr) {
					if (old_econ_entity->IsBaseCombatWeapon()) {
						auto old_weapon = ToBaseCombatWeapon(old_econ_entity);
						
						ModCommandResponse("[%s] Removing old weapon \"%s\" from slot #%d (\"%s\").\n", cmd_name, old_weapon->GetAttributeContainer()->GetItem()->GetStaticData()->GetName(), slot, GetLoadoutSlotName(slot));
						recipient->Weapon_Detach(old_weapon);
						old_weapon->Remove();
					} else if (old_econ_entity->IsWearable()) {
						auto old_wearable = rtti_cast<CEconWearable *>(old_econ_entity);
						
						ModCommandResponse("[%s] Removing old wearable \"%s\" from slot #%d (\"%s\").\n", cmd_name, old_wearable->GetAttributeContainer()->GetItem()->GetStaticData()->GetName(), slot, GetLoadoutSlotName(slot));
						recipient->RemoveWearable(old_wearable);
					} else {
						ModCommandResponse("[%s] Removing old entity \"%s\" from slot #%d (\"%s\").\n", cmd_name, old_econ_entity->GetClassname(), slot, GetLoadoutSlotName(slot));
						old_econ_entity->Remove();
					}
				} else {
				//	Msg("No old entity in slot %d\n", slot);
				}
			}
		}
		
		CBaseEntity *ent = recipient->GiveNamedItem(item_view->GetStaticData()->GetItemClass(""), 0, item_view, false);
		if (ent != nullptr) {
			auto econ_ent = rtti_cast<CEconEntity *>(ent);
			if (econ_ent != nullptr) {
				/* make the model visible for other players */
				econ_ent->Validate();
				econ_ent->GiveTo(recipient);
			} else {
				ModCommandResponse("[%s] Failure: GiveNamedItem returned a non-CEconEntity!\n", cmd_name);
				return;
			}
		} else {
			ModCommandResponse("[%s] Failure: GiveNamedItem returned nullptr!\n", cmd_name);
			return;
		}
		
		if (!item_view->GetAttributeList().Attributes().IsEmpty()) {
			size_t attr_name_len_max = 0;
			for (CEconItemAttribute& attr : item_view->GetAttributeList().Attributes()) {
				attr_name_len_max = Max(attr_name_len_max, strlen(attr.GetStaticData()->GetName()));
			}
			char buf[256];
			//const char *buf = nullptr;
			int attr_num = 1;
			
			ModCommandResponse("[%s] Created item \"%s\" with the following %d attributes:\n", cmd_name, item_view->GetStaticData()->GetName(), item_view->GetAttributeList().Attributes().Count());
			for (CEconItemAttribute& attr : item_view->GetAttributeList().Attributes()) {
				CEconItemAttributeDefinition *attr_def = attr.GetStaticData();
				
				attr_def->ConvertValueToString(*(attr.GetValuePtr()), buf, sizeof(buf));
				//CopyStringAttributeValueToCharPointerOutput(attr.GetValuePtr()->m_String, &buf);
				int pad = ((int)attr_name_len_max - (int)strlen(attr_def->GetName()));
				
				ModCommandResponse("[%s]   [%2d] \"%s\"%*s \"%s\"\n", cmd_name, attr_num, attr_def->GetName(), pad, "", buf);
				++attr_num;
			}
			ModCommandResponse("[%s] And gave it to player \"%s\".\n\n", cmd_name, recipient->GetPlayerName());
		} else {
			ModCommandResponse("[%s] Created item \"%s\" with 0 attributes and gave it to player \"%s\".\n\n", cmd_name, item_view->GetStaticData()->GetName(), recipient->GetPlayerName());
		}
		
		
	}
	
	void CC_Give_Common(CCommandPlayer *player, const CCommand& args, const char *cmd_name, CSteamID steamid, bool no_remove)
	{
		if (args.ArgC() == 1)
			Give_Common(player, player, cmd_name, steamid, no_remove, nullptr);
		else if (args.ArgC() == 2) {
			std::vector<CBasePlayer *> vec;
			GetSMTargets(player, args[1], vec);
			for(CBasePlayer *target : vec) {
				Give_Common(player, ToTFPlayer(target), cmd_name, steamid, no_remove, nullptr);
			}
		}
		state.erase(steamid);
	}

	void CC_Give(CCommandPlayer *player, const CCommand& args)
	{
		auto steamid = GetCommandClientSteamID("CC_Give", player);
		
		ModCommandResponse("\n");
		
		if (args.ArgC() != 1 && args.ArgC() != 2) {
			ModCommandResponse("[sig_makeitem_give] Usage: any of the following:\n"
				"  sig_makeitem_give               | give to yourself\n"
				"  sig_makeitem_give <player_name> | give to the player with this name (names with spaces need quotes)\n"
				"  sig_makeitem_give <steam_id>    | give to the player with this Steam ID\n"
				"  sig_makeitem_give <user_id>     | give to the player with this server user ID\n");
			return;
		}
		
		CC_Give_Common(player, args, "sig_makeitem_give", steamid, false);
	}
	
	void CC_Give_NoRemove(CCommandPlayer *player, const CCommand& args)
	{
		auto steamid = GetCommandClientSteamID("CC_Give_NoRemove", player);
		
		ModCommandResponse("\n");
		
		if (args.ArgC() != 1 && args.ArgC() != 2) {
			ModCommandResponse("[sig_makeitem_give_noremove] Usage: same as sig_makeitem_give.\n"
				"  (This version of the command won't remove pre-existing items that have equip region or item slot conflicts.)\n");
			return;
		}
		
		CC_Give_Common(player, args, "sig_makeitem_give_noremove", steamid, true);
	}

	void Give_OneLine(CCommandPlayer *player, const CCommand& args, const char *cmd_name, CSteamID steamid, bool noremove, const char *custom_class)
	{
		state.erase(steamid);

		ModCommandResponse("\n");
		
		std::vector<CBasePlayer *> vec;

		if (args.ArgC() > 2) { 
			GetSMTargets(player, args[1], vec);
			if (vec.empty())
				return;
			
			Start(player, args[2], steamid);
		}
		else if(player != nullptr && args.ArgC() == 2) {
			vec.push_back(player);
			Start(player, args[1], steamid);
		}
		else {
			ModCommandResponse("[%s] Usage:\n"
				"  %s [target] <item> [<attribute name> <attribute value>]...\n", cmd_name, cmd_name);
			return;
		}



		if (args.ArgC() > 3) {
			for (int i = 3; i < args.ArgC()-1; i += 2) {
				AddAttr(player, args[i], args[i+1]);
			}
		}
		
		for(CBasePlayer *target : vec) {
			Give_Common(player, ToTFPlayer(target), "sig_makeitem", steamid, noremove, custom_class);
		}
		state.erase(steamid);
	}

	void CC_Give_OneLine(CCommandPlayer *player, const CCommand& args)
	{
		auto steamid = GetCommandClientSteamID("CC_Give_OneLine", player);
		Give_OneLine(player, args, "sig_makeitem", steamid, false, nullptr);
	}
	
	void CC_Give_OneLine_NoRemove(CCommandPlayer *player, const CCommand& args)
	{
		auto steamid = GetCommandClientSteamID("CC_Give_OneLine_NoRemove", player);
		Give_OneLine(player, args, "sig_makeitem_noremove", steamid, true, nullptr);
	}

	void CC_Give_Custom_Class(CCommandPlayer *player, const CCommand& args)
	{
		auto steamid = GetCommandClientSteamID("CC_Give_OneLine", player);
		Give_OneLine(player, args, "sig_makeitem", steamid, false, nullptr);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Make_Item")
		{
		}
	};
	CMod s_Mod;
	
	/* default: admin-only mode ENABLED */
	ConVar cvar_adminonly("sig_util_make_item_adminonly", "1", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Utility: restrict this mod's functionality to SM admins only");
	
	class MakeItemCommand : public ModCommand
	{
	public:
		MakeItemCommand(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

		virtual bool CanPlayerCall(CTFPlayer *player) { return player == nullptr || !cvar_adminonly.GetBool() || PlayerIsSMAdminOrBot(player); }
	};

	MakeItemCommand sig_makeitem_help("sig_makeitem_help", CC_Help, &s_Mod);
	MakeItemCommand sig_makeitem_clear("sig_makeitem_clear", CC_Clear, &s_Mod);
	MakeItemCommand sig_makeitem_start("sig_makeitem_start", CC_Start, &s_Mod);
	MakeItemCommand sig_makeitem_add_attr("sig_makeitem_add_attr", CC_AddAttr, &s_Mod);
	MakeItemCommand sig_makeitem_give("sig_makeitem_give", CC_Give, &s_Mod);
	MakeItemCommand sig_makeitem_give_noremove("sig_makeitem_give_noremove", CC_Give_NoRemove, &s_Mod);
	MakeItemCommand sig_makeitem("sig_makeitem", CC_Give_OneLine, &s_Mod);
	MakeItemCommand sig_makeitem_noremove("sig_makeitem_noremove", CC_Give_OneLine_NoRemove, &s_Mod);
	
	ConVar cvar_enable("sig_util_make_item", "0", FCVAR_NOTIFY,
		"Utility: enable sig_makeitem_* client commands",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
#if 0 // REMOVE THIS CODE (OR MOVE IT ELSEWHERE) ===============================
	
	// offsets based on server_srv.so version 20170803a
	CON_COMMAND_F(sig_equipregion_dump, "", FCVAR_CHEAT)
	{
		auto& m_EquipRegions = *reinterpret_cast<CUtlVector<CEconItemSchema::EquipRegion> *>((uintptr_t)GetItemSchema() + 0x378);
		
		Msg("%-8s  %-8s  | %s\n", "BITMASK", "MASK", "REGION");
		for (const auto& region : m_EquipRegions)
		{
			Msg("%08x  %08x  | %s\n", (1U << region.m_nBitMask), region.m_nMask, region.m_strName.Get());
		}
		Msg("\n");
		
		Msg("%-8s  %-8s  |        %s\n", "BITMASK", "MASK", "ITEM");
		auto schema = GetItemSchema();
		for (int i = 1; i < 40000; ++i) {
			auto item_def = schema->GetItemDefinition(i);
			if (item_def == nullptr)                                  continue;
			if (strcmp(item_def->GetName("default"), "default") == 0) continue;
			
			auto& m_nEquipRegionBitMask = *reinterpret_cast<unsigned int *>((uintptr_t)item_def + 0x12c);
			auto& m_nEquipRegionMask    = *reinterpret_cast<unsigned int *>((uintptr_t)item_def + 0x130);
			
			Msg("%08x  %08x  | #%-5d %s\n", m_nEquipRegionBitMask, m_nEquipRegionMask, i, item_def->GetName());
		}
		Msg("\n");
	}
	
#endif // ======================================================================
}
