#include "mod.h"
#include "mod/pop/kv_conditional.h"
#include "stub/tfbot.h"
#include "stub/populators.h"
#include "stub/strings.h"
#include "stub/gamerules.h"
#include "stub/tfbot_behavior.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/particles.h"
#include "util/iterate.h"
#include "util/clientmsg.h"
#include "mod/pop/pointtemplate.h"
#include "mod/pop/common.h"


// update data with every call to:
// - CTFBot::OnEventChangeAttributes
//   (verify that this does get called even for the initial default set of settings when spawning)

// clear data with every call to:
// - CTFBot::~CTFBot (D0+D2)
// - CTFBot::Event_Killed
// - CTFBot::ChangeTeam(TEAM_SPECTATOR)
// - CTFBot::ForceChangeTeam(TEAM_SPECTATOR)

// we probably want maps of:
// - CHandle<CTFBot> -> their CTFBotSpawner of origin
// - CHandle<CTFBot> -> their current EventChangeAttributes_t * (in their CTFBotSpawner of origin)

#define PLAYER_ANIM_WEARABLE_ITEM_ID 12138

namespace Mod::Pop::PopMgr_Extensions
{
	CTFItemDefinition *GetCustomWeaponItemDef(std::string name);
	bool AddCustomWeaponAttributes(std::string name, CEconItemView *view);
	const char *GetCustomWeaponNameOverride(const char *name);
	int GetEventPopfile();
}

class PlayerBody : public IBody {
	public:
		CBaseEntity * GetEntity() {return vt_GetEntity(this);}
	private:
		static MemberVFuncThunk<PlayerBody *, CBaseEntity*>   vt_GetEntity;
};
MemberVFuncThunk<PlayerBody *, CBaseEntity*                                                          > PlayerBody::vt_GetEntity                  (TypeName<PlayerBody>(), "PlayerBody::GetEntity");


#ifdef ADD_EXTATTR
namespace Mod::Pop::ECAttr_Extensions
{
	IForward *custom_item_get_def_id;
	IForward *custom_item_equip_itemname;
	IForward *custom_item_set_attribute;

	struct HomingRockets
	{
		bool enable                 = false;
		bool ignore_disguised_spies = true;
		bool ignore_stealthed_spies = true;
		bool follow_crosshair       = false;
		float speed                 = 1.0f;
		float turn_power            = 10.0f;
		float min_dot_product       = -0.25f;
		float aim_time              = 9999.0f;
		float acceleration          = 0.0f;
		float acceleration_time     = 9999.0f;
		float acceleration_start    = 0.0f;
		float gravity               = 0.0f;
	};

	enum AimAt
	{
		AIM_DEFAULT,
		AIM_HEAD,
		AIM_BODY,
		AIM_FEET
	};

	struct EventChangeAttributesData
	{
		std::map<std::string,std::map<std::string, std::string>> custom_attrs;
		int skin = -1;
		std::map<std::string, int> bodygroup;
		
		AimAt aim_at = AIM_DEFAULT;
		Vector aim_offset = vec3_origin;
		float aim_lead_target_speed = 0.0f;

		int rocket_jump_type = 0;

		float head_rotate_speed = -1.0f;

		float tracking_interval = -1.0f;

		std::string use_custom_model;

		// 1 - No sap attribute
		// 2 - Can be sapped
		int use_human_model  = 0;
		bool use_buster_model = false;

		bool use_human_animations = false;

		float ring_of_fire = -1.0f;

		bool use_melee_threat_prioritization = false;
		
		bool always_glow = false;
		
		bool no_glow = false;
		
		bool no_bomb_upgrade = false;
		
		bool use_best_weapon = false;

		bool fast_update = false;

		bool no_pushaway = false;

		bool no_crouch_button_release = false;

		float scale_speed = 1.0f;

		float spell_drop_rate_common = 0.0f;
		float spell_drop_rate_rare = 0.0f;
		bool spell_drop = false;

		bool eye_particle_color = false;
		uint8 eye_particle_r = 0;
		uint8 eye_particle_g = 0;
		uint8 eye_particle_b = 0;

		std::string death_sound = "DEF";
		std::string pain_sound = "DEF";

		std::map<CHandle<CTFWeaponBase>, float> projectile_speed_cache;
		
		std::vector<ShootTemplateData> shoot_templ;
		
		std::string fire_sound = "";
		std::map<int, std::string> custom_weapon_models;
		
		std::string rocket_custom_model;
		std::string rocket_custom_particle;

		std::string custom_eye_particle = "";
		
		HomingRockets homing_rockets;

		std::map<int, float> weapon_resists;
		
		std::map<std::string, color32> item_colors;
		std::map<std::string, std::string> item_models;

		std::vector<int> strip_item_slot;

		std::vector<AddCond> dmgappliesconds;

		std::vector<AddCond> addconds;

		std::vector<PeriodicTaskImpl> periodic_tasks;

		bool has_override_step_sound;
		std::string override_step_sound;

		std::vector<std::string> strip_item;

		CRC32_t spray_file;
	};

	/* maps ECAttr instances -> extra data instances */
	std::map<CTFBot::EventChangeAttributes_t *, EventChangeAttributesData> extdata;
	
	/* maps CTFBot instances -> their current ECAttr instance */
	std::unordered_map<CTFBot *, EventChangeAttributesData *> ecattr_map;

	
	std::vector<DelayedAddCond> delayed_addconds;

	std::vector<PeriodicTask> pending_periodic_tasks;
	
#if 0
	/* maps CTFBot instances -> their current ECAttr name */
	std::map<CHandle<CTFBot>, std::string> ecattr_map;
#endif
	
	std::unordered_map<std::string, CTFItemDefinition*> item_defs;

	std::unordered_map<std::string, bool> item_custom_remap;
	
#if 0
	const std::string& GetCurrentTFBotECAttrName(CTFBot *bot)
	{
		CHandle<CTFBot> handle = bot;
		
		auto it = ecattr_map.find(handle);
		if (it == ecattr_map.end()) {
			return "default";
		}
		return *it;
	}
#endif
	
	
	// ecattr_map:
	// clear in OnUnload and SetEnabled(false)
	// update every time CTFBot::OnEventChangeAttributes is called
	// 
	
	EventChangeAttributesData *GetDataForBot(CTFBot *bot)
	{
		if (bot == nullptr)
			return nullptr;

		auto it = ecattr_map.find(bot);
		if (it == ecattr_map.end()) {
			return nullptr;
		}
		return it->second;
	}

	EventChangeAttributesData *GetDataForBot(CBaseEntity *bot)
	{
		return GetDataForBot(ToTFBot(bot));
	}

	enum ClearAction {
		DESTRUCT,
		DIE,
		CHANGE
	};

	void ClearAllData()
	{
		ecattr_map.clear();
		extdata.clear();
		item_defs.clear();
		item_custom_remap.clear();
		delayed_addconds.clear();
		pending_periodic_tasks.clear();
	}

	void ClearDataForBot(CTFBot *bot, ClearAction clear_action)
	{
		if (clear_action != DESTRUCT) {
			
			auto data = GetDataForBot(bot);
			if (data != nullptr && data->use_human_animations) {
				for(int i = 0; i < bot->GetNumWearables(); i++) {
					CEconWearable *wearable = bot->GetWearable(i);
					if (wearable != nullptr && wearable->GetItem() != nullptr && wearable->GetItem()->m_iItemDefinitionIndex == PLAYER_ANIM_WEARABLE_ITEM_ID) {
						int model_index = wearable->m_nModelIndexOverrides[0];
						bot->GetPlayerClass()->SetCustomModel(modelinfo->GetModelName(modelinfo->GetModel(model_index)), true);
						wearable->Remove();
					}
				}
				if (bot->GetRenderMode() == 1) {
					servertools->SetKeyValue(bot, "rendermode", "0");
					bot->SetRenderColorA(255);
				}
			}
		}
		
		ecattr_map.erase(bot);

		for (auto it = delayed_addconds.begin(); it != delayed_addconds.end(); ) {
			if ((*it).bot == bot) {
				it = delayed_addconds.erase(it);
			} else {
				++it;
			}
		}

		for (auto it = pending_periodic_tasks.begin(); it != pending_periodic_tasks.end(); ) {
			if ((*it).bot == bot) {
				it = pending_periodic_tasks.erase(it);
			} else {
				++it;
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFBot_dtor0)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
	//	DevMsg("CTFBot %08x: dtor0, clearing data\n", (uintptr_t)bot);
		ClearDataForBot(bot, DESTRUCT);
		
		DETOUR_MEMBER_CALL(CTFBot_dtor0)();
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_dtor2)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
	//	DevMsg("CTFBot %08x: dtor2, clearing data\n", (uintptr_t)bot);
		ClearDataForBot(bot, DESTRUCT);
		
		DETOUR_MEMBER_CALL(CTFBot_dtor2)();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_StateEnter, int nState)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (nState == TF_STATE_WELCOME || nState == TF_STATE_OBSERVER) {
			CTFBot *bot = ToTFBot(player);
			if (bot != nullptr) {
				ClearDataForBot(bot, DIE);
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayer_StateEnter)(nState);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_StateLeave)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		if (player->StateGet() == TF_STATE_WELCOME || player->StateGet() == TF_STATE_OBSERVER) {
			CTFBot *bot = ToTFBot(player);
			if (bot != nullptr) {
			//	DevMsg("Bot #%d [\"%s\"]: StateLeave %s, clearing data\n", ENTINDEX(bot), bot->GetPlayerName(), GetStateName(bot->StateGet()));
				ClearDataForBot(bot, DIE);
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayer_StateLeave)();
	}


	CTFBotSpawner *current_spawner = nullptr;
	KeyValues *current_spawner_kv = nullptr;

	//Contains ecattr extra data for the currently parsed spawner
	std::unordered_map<int, EventChangeAttributesData> spawner_ecattr;
	
	void CopyEvent(CTFBot::EventChangeAttributes_t& orig, const CTFBot::EventChangeAttributes_t& copy)
	{

		orig.m_iSkill = copy.m_iSkill;
		orig.m_nWeaponRestrict = copy.m_nWeaponRestrict;
		orig.m_nMission = copy.m_nMission;
		orig.pad_10 = copy.pad_10;
		orig.m_nBotAttrs = copy.m_nBotAttrs;
		orig.m_flVisionRange = copy.m_flVisionRange;

		orig.m_ItemNames.RemoveAll();
			
		orig.m_ItemAttrs.RemoveAll();
		orig.m_CharAttrs.RemoveAll();
		orig.m_Tags.RemoveAll();

		for (int i=0; i<copy.m_ItemNames.Count(); ++i)
		{
			orig.m_ItemNames.CopyAndAddToTail(copy.m_ItemNames[i]);
		}

		orig.m_ItemAttrs = copy.m_ItemAttrs;
		orig.m_CharAttrs = copy.m_CharAttrs;

		for (int i=0; i<copy.m_Tags.Count(); ++i)
		{
			orig.m_Tags.CopyAndAddToTail(copy.m_Tags[i]);
		}
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_Parse, KeyValues *kv_orig)
	{
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		
		current_spawner = spawner;
		current_spawner_kv = kv_orig;

		spawner_ecattr.clear();

		auto result = DETOUR_MEMBER_CALL(CTFBotSpawner_Parse)(kv_orig);

		CTFBot::EventChangeAttributes_t *default_ecattr = nullptr;
		for (int i = 0; i < spawner->m_ECAttrs.Count(); i++) {
			extdata[&spawner->m_ECAttrs[i]] = spawner_ecattr[i+1];
			if (default_ecattr == nullptr && strcmp(spawner->m_ECAttrs[i].m_strName.Get(), "DefaultSigOverride") == 0)
			{
				default_ecattr = &spawner->m_ECAttrs[i];
				default_ecattr->m_strName.Set("Default");
				CopyEvent(spawner->m_DefaultAttrs, *default_ecattr);
				extdata[&spawner->m_DefaultAttrs] = spawner_ecattr[i + 1];
			}
		}
		if (default_ecattr == nullptr) {
			extdata[&spawner->m_DefaultAttrs] = spawner_ecattr[0];
		}
		return result;
	}

	ConVar cvar_no_romevision("sig_no_romevision_cosmetics", "0", FCVAR_NONE,
		"Disable romevision cosmetics");
	ConVar cvar_creators_custom_item("sig_creators_custom_item", "0", FCVAR_NONE,
		"Enable fallback to creators custom item");

	void Parse_WeaponResist(EventChangeAttributesData &data, KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			int weapon_id = GetWeaponId(name);
			if (weapon_id == TF_WEAPON_NONE) {
				Warning("Unknown weapon ID \'%s\' in WeaponResist block.\n", name);
				continue;
			}
			
			DevMsg("CTFBotSpawner %08x: resist %s (0x%02x): %4.02f\n", (uintptr_t)&data, name, weapon_id, subkey->GetFloat());
			data.weapon_resists[weapon_id] = subkey->GetFloat();
		}
	}
	
	void Parse_ItemColor(EventChangeAttributesData &data, KeyValues *kv)
	{
		const char *item_name = "";
		int color_r           = 0x00;
		int color_g           = 0x00;
		int color_b           = 0x00;
		
		bool got_name  = false;
		bool got_col_r = false;
		bool got_col_g = false;
		bool got_col_b = false;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "ItemName")) {
				item_name = subkey->GetString();
				got_name = true;
			} else if (FStrEq(name, "Red")) {
				color_r = Clamp(subkey->GetInt(), 0x00, 0xff);
				got_col_r = true;
			} else if (FStrEq(name, "Green")) {
				color_g = Clamp(subkey->GetInt(), 0x00, 0xff);
				got_col_g = true;
			} else if (FStrEq(name, "Blue")) {
				color_b = Clamp(subkey->GetInt(), 0x00, 0xff);
				got_col_b = true;
			} else {
				Warning("Unknown key \'%s\' in ItemColor block.\n", name);
			}
		}
		
		if (!got_name) {
			Warning("No ItemName specified in ItemColor block.\n");
			return;
		}
		
		if (!got_col_r) {
			Warning("No Red color value specified in ItemColor block.\n");
			return;
		}
		if (!got_col_g) {
			Warning("No Green color value specified in ItemColor block.\n");
			return;
		}
		if (!got_col_b) {
			Warning("No Blue color value specified in ItemColor block.\n");
			return;
		}
		
		DevMsg("CTFBotSpawner %08x: add ItemColor(\"%s\", %02X%02X%02X)\n", (uintptr_t)&data, item_name, color_r, color_g, color_b);
		data.item_colors[item_name] = { color_r, color_g, color_b, 0xff };
	}

	void Parse_ItemModel(EventChangeAttributesData &data, KeyValues *kv)
	{
		const char *item_name = "";
		const char *model_name = "";
		
		bool got_name  = false;
		bool got_model = false;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "ItemName")) {
				item_name = subkey->GetString();
				got_name = true;
			} else if (FStrEq(name, "Model")) {
				model_name = subkey->GetString();
				got_model = true;
			} else {
				Warning("Unknown key \'%s\' in ItemModel block.\n", name);
			}
		}
		
		if (!got_name) {
			Warning("No ItemName specified in ItemModel block.\n");
			return;
		}
		
		if (!got_model) {
			Warning("No Model value specified in ItemModel block.\n");
			return;
		}
		
		DevMsg("CTFBotSpawner %08x: add ItemModel(\"%s\", \"%s\")\n", (uintptr_t)&data, item_name, model_name);
		data.item_models[item_name] = model_name;
	}

	void Parse_CustomWeaponModel(EventChangeAttributesData &data, KeyValues *kv)
	{
		int slot = -1;
		const char *path = "";
		
		bool got_slot = false;
		bool got_path = false;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "Slot")) {
				if (subkey->GetDataType() == KeyValues::TYPE_STRING) {
					slot = GetLoadoutSlotByName(subkey->GetString());
				} else {
					slot = subkey->GetInt();
				}
				got_slot = true;
			} else if (FStrEq(name, "Model")) {
				path = subkey->GetString();
				got_path = true;
			} else {
				Warning("Unknown key \'%s\' in CustomWeaponModel block.\n", name);
			}
		}
		
		if (!got_slot) {
			Warning("No weapon slot specified in CustomWeaponModel block.\n");
			return;
		}
		
		if (!got_path) {
			Warning("No Model path specified in CustomWeaponModel block.\n");
			return;
		}
		
		if (slot < LOADOUT_POSITION_PRIMARY || slot > LOADOUT_POSITION_PDA2) {
			Warning("CustomWeaponModel Slot must be in the inclusive range [LOADOUT_POSITION_PRIMARY, LOADOUT_POSITION_PDA2], i.e. [%d, %d].\n",
				(int)LOADOUT_POSITION_PRIMARY, (int)LOADOUT_POSITION_PDA2);
			return;
		}
		
		DevMsg("CTFBotSpawner %08x: add CustomWeaponModel(%d, \"%s\")\n",
			(uintptr_t)&data, slot, path);
		data.custom_weapon_models[slot] = path;
	}

	
	void Parse_HomingRockets(EventChangeAttributesData &data, KeyValues *kv)
	{
		HomingRockets& hr = data.homing_rockets;
		hr.enable = true;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "IgnoreDisguisedSpies")) {
				hr.ignore_disguised_spies = subkey->GetBool();
			} else if (FStrEq(name, "IgnoreStealthedSpies")) {
				hr.ignore_stealthed_spies = subkey->GetBool();
			} else if (FStrEq(name, "FollowCrosshair")) {
				hr.follow_crosshair = subkey->GetBool();
			} else if (FStrEq(name, "RocketSpeed")) {
				hr.speed = subkey->GetFloat();
			} else if (FStrEq(name, "TurnPower")) {
				hr.turn_power = subkey->GetFloat();
			} else if (FStrEq(name, "MinDotProduct")) {
				hr.min_dot_product = Clamp(subkey->GetFloat(), -1.0f, 1.0f);
			} else if (FStrEq(name, "MaxAimError")) {
				hr.min_dot_product = std::cos(DEG2RAD(Clamp(subkey->GetFloat(), 0.0f, 180.0f)));
			} else if (FStrEq(name, "AimTime")) {
				hr.aim_time = subkey->GetFloat();
			} else if (FStrEq(name, "Acceleration")) {
				hr.acceleration = subkey->GetFloat();
			} else if (FStrEq(name, "AccelerationTime")) {
				hr.acceleration_time = subkey->GetFloat();
			} else if (FStrEq(name, "AccelerationStartTime")) {
				hr.acceleration_start = subkey->GetFloat();
			} else if (FStrEq(name, "Gravity")) {
				hr.gravity = subkey->GetFloat();
			} else if (FStrEq(name, "Enable")) {
				/* this used to be a parameter but it was redundant and has been removed;
				 * ignore it without issuing a warning */
			} else {
				Warning("Unknown key \'%s\' in HomingRockets block.\n", name);
			}
		}
		
		DevMsg("CTFBotSpawner %08x: set HomingRockets(%s, %s, %.2f, %.1f, %.2f)\n",
			(uintptr_t)&data,
			(hr.ignore_disguised_spies ? "true" : "false"),
			(hr.ignore_stealthed_spies ? "true" : "false"),
			hr.speed, hr.turn_power, hr.min_dot_product);
	}

	void Parse_DamageAppliesCond(EventChangeAttributesData &data, KeyValues *kv)
	{
		AddCond addcond;
		
		bool got_cond     = false;
		bool got_duration = false;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "Index")) {
				addcond.cond = (ETFCond)subkey->GetInt();
				got_cond = true;
			} else if (FStrEq(name, "Name")) {
				ETFCond cond = GetTFConditionFromName(subkey->GetString());
				if (cond != -1) {
					addcond.cond = cond;
					got_cond = true;
				} else {
					Warning("Unrecognized condition name \"%s\" in DamageAppliesCond block.\n", subkey->GetString());
				}
			} else if (FStrEq(name, "Duration")) {
				addcond.duration = subkey->GetFloat();
				got_duration = true;
			} else if (FStrEq(name, "IfHealthBelow")) {
				addcond.health_below = subkey->GetInt();
			} else if (FStrEq(name, "IfHealthAbove")) {
				addcond.health_above = subkey->GetInt();
			} else {
				Warning("Unknown key \'%s\' in DamageAppliesCond block.\n", name);
			}
		}
		
		if (!got_cond) {
			Warning("Could not find a valid condition index/name in DamageAppliesCond block.\n");
			return;
		}
		
		DevMsg("CTFBotSpawner %08x: add DamageAppliesCond(%d, %f)\n", (uintptr_t)&data, addcond.cond, addcond.duration);
		data.dmgappliesconds.push_back(addcond);
	}

	void Parse_Bodygroup(EventChangeAttributesData &data, KeyValues *kv)
	{
		const char *bodygroup = nullptr;
		int value = -1;

		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();

			if (FStrEq(name, "Name")) {
				bodygroup = subkey->GetString();
			} else if (FStrEq(name, "Value")) {
				value = subkey->GetInt();
			} else {
				Warning("Unknown key \'%s\' in Bodygroup block.\n", name);
			}
		}

		if (value == -1 || bodygroup == nullptr) {
			Warning("Invalid Bodygroup block.\n");
			return;
		}

		data.bodygroup[bodygroup] = value;
	}

	AimAt Parse_AimAt(KeyValues *kv) {
		const char * aim = kv->GetString();
		if (FStrEq(aim, "Default")){
			return AIM_DEFAULT;
		}
		else if (FStrEq(aim, "Head")){
			return AIM_HEAD;
		}
		else if (FStrEq(aim, "Body")){
			return AIM_BODY;
		}
		else if (FStrEq(aim, "Feet")){
			return AIM_FEET;
		}
		else
			return AIM_DEFAULT;
	}

	void Parse_SprayFile(EventChangeAttributesData &data, KeyValues *kv)
	{
		CRC32_t value;
		
		if (LoadUserDataFile(value, kv->GetString())) {
			data.spray_file = value;
		}
		else {
			Warning("CTFBotSpawner: Spray file %s not found\n", kv->GetString());
		}
	}

	CTFBot::EventChangeAttributes_t *default_ecattr = nullptr;
	CTFBotSpawner *last_spawner = nullptr;
	CTFBot::EventChangeAttributes_t *last_ecattr = nullptr;
	int parse_dynamic_index = 0;
	#warning __gcc_regcall detours considered harmful!
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, bool, ParseDynamicAttributes, CTFBot::EventChangeAttributes_t &ecattr, KeyValues *kv)
	{
		
		// Copy from default ecattr to this ecattr

		int data_index = parse_dynamic_index;
		if (parse_dynamic_index == 0 && default_ecattr != nullptr) {
			data_index = last_spawner->m_ECAttrs.Count();

			if (last_ecattr != &ecattr) {
				CTFBot::EventChangeAttributes_t * copy_from = default_ecattr;
				//int copy_from_index = -1;
				for (int i = 0; i < last_spawner->m_ECAttrs.Count(); i++) {
					if (&ecattr != &last_spawner->m_ECAttrs[i] && FStrEq(last_spawner->m_ECAttrs[i].m_strName.Get(), ecattr.m_strName.Get())) {
						
						parse_dynamic_index = i + 1;
						bool result = Detour_ParseDynamicAttributes(current_spawner->m_ECAttrs[i], kv);

						parse_dynamic_index = 0;
						return result;

						//copy_from = &last_spawner->m_ECAttrs[i];
						//copy_from_index = i;
						//break;
					}
				}
				last_ecattr = &ecattr;

				//if (copy)
				CopyEvent(ecattr, *copy_from);

				spawner_ecattr[data_index] = spawner_ecattr[0];
			}
		}
		// Copy from this default ecattr to all ecattr
		if ((&ecattr) == &current_spawner->m_DefaultAttrs) {
			for (int i = 0; i < current_spawner->m_ECAttrs.Count(); i++) {
				parse_dynamic_index = i + 1;
				
				if (!Detour_ParseDynamicAttributes(current_spawner->m_ECAttrs[i], kv)){
					parse_dynamic_index = 0;
					return false;
				}
				parse_dynamic_index = 0;
			}
		}

		auto &data = spawner_ecattr[data_index];

		const char *name = kv->GetName();
		
		bool found = true;
		if (FStrEq(name, "Skin")) {
			data.skin = kv->GetInt();
		} else if (FStrEq(name, "AimAt")) {
			data.aim_at = Parse_AimAt(kv);
		} else if (FStrEq(name, "AimOffset")) {
			Vector offset;
			sscanf(kv->GetString(),"%f %f %f",&offset.x,&offset.y,&offset.z);
			data.aim_offset = offset;
		} else if (FStrEq(name, "AimLeadProjectileSpeed")) {
			data.aim_lead_target_speed = kv->GetFloat();
		} else if (FStrEq(name, "RocketJump")) {
			data.rocket_jump_type = kv->GetInt();
		} else if (FStrEq(name, "AimTrackingInterval")) {
			data.tracking_interval = kv->GetFloat();
		} else if (FStrEq(name, "HeadRotateSpeed")) {
			data.head_rotate_speed = kv->GetFloat();
		} else if (FStrEq(name, "WeaponRestrictions")) {
			const char *val = kv->GetString();
			if (FStrEq(val, "PDAOnly"))
				ecattr.m_nWeaponRestrict = CTFBot::WeaponRestriction::PDA_ONLY;
			else if (FStrEq(val, "BuildingOnly"))
				ecattr.m_nWeaponRestrict = CTFBot::WeaponRestriction::BUILDING_ONLY;
			else
				found = false;
		} else if (FStrEq(name, "UseBusterModel")) {
			data.use_buster_model = kv->GetBool();
		} else if (FStrEq(name, "UseCustomModel")) {
			data.use_custom_model = kv->GetString();
		} else if (FStrEq(name, "UseHumanModel")) {
			data.use_human_model = kv->GetInt();
		} else if (FStrEq(name, "UseHumanAnimations")) {
			data.use_human_animations = kv->GetBool();
		} else if (FStrEq(name, "AlwaysGlow")) {
			data.always_glow = kv->GetBool();
		} else if (FStrEq(name, "NoGlow")) {
			data.always_glow = kv->GetBool();
		} else if (FStrEq(name, "UseBestWeapon")) {
			data.use_best_weapon = kv->GetBool();
		} else if (FStrEq(name, "RingOfFire")) {
			data.ring_of_fire = kv->GetFloat();
		} else if (FStrEq(name, "UseMeleeThreatPrioritization")) {
			data.use_melee_threat_prioritization = kv->GetBool();
		} else if (FStrEq(name, "NoBombUpgrades")) {
			data.no_bomb_upgrade = kv->GetBool();
		} else if (FStrEq(name, "DeathSound")) {
			data.death_sound = kv->GetString();
			if (!enginesound->PrecacheSound(kv->GetString(), true))
				CBaseEntity::PrecacheScriptSound(kv->GetString());
		} else if (FStrEq(name, "PainSound")) {
			data.pain_sound = kv->GetString();
			if (!enginesound->PrecacheSound(kv->GetString(), true))
				CBaseEntity::PrecacheScriptSound(kv->GetString());
		} else if (FStrEq(name, "WeaponResist")) {
			Parse_WeaponResist(data, kv);
		} else if (FStrEq(name, "ItemColor")) {
			Parse_ItemColor(data, kv);
		} else if (FStrEq(name, "ItemModel")) {
			Parse_ItemModel(data, kv);
		} else if (FStrEq(name, "HomingRockets")) {
			Parse_HomingRockets(data, kv);
		} else if (FStrEq(name, "CustomWeaponModel")) {
			Parse_CustomWeaponModel(data, kv);
		} else if (FStrEq(name, "RocketCustomModel")) {
			data.rocket_custom_model = kv->GetString();
		} else if (FStrEq(name, "RocketCustomParticle")) {
			data.rocket_custom_particle = kv->GetString();
		} else if (FStrEq(name, "ShootTemplate")) {
			ShootTemplateData shootdata;
			if (Parse_ShootTemplate(shootdata, kv))
				data.shoot_templ.push_back(shootdata);
		} else if (FStrEq(name, "DamageAppliesCond")) {
			Parse_DamageAppliesCond(data, kv);
		} else if (FStrEq(name, "FireSound")) {
			data.fire_sound = kv->GetString();//AllocPooledString(subkey->GetString());
			if (!enginesound->PrecacheSound(kv->GetString(), true))
				CBaseEntity::PrecacheScriptSound(kv->GetString());
		} else if (FStrEq(name, "Bodygroup")) {
			Parse_Bodygroup(data, kv);
		} else if (FStrEq(name, "FastUpdate")) {
			data.fast_update = kv->GetBool();
		} else if (FStrEq(name, "NoPushaway")) {
			data.no_pushaway = kv->GetBool();
		} else if (FStrEq(name, "BodyPartScaleSpeed")) {
			data.scale_speed = kv->GetFloat();
		} else if (FStrEq(name, "NoCrouchButtonRelease")) {
			data.no_crouch_button_release = kv->GetBool();
		} else if (FStrEq(name, "CustomEyeParticle")) {
			data.custom_eye_particle = kv->GetString();
		} else if (FStrEq(name, "CustomEyeGlowColor")) {
			data.eye_particle_color = true;
			sscanf(kv->GetString(), "%d %d %d", &data.eye_particle_r, &data.eye_particle_g, &data.eye_particle_b);
		} else if (FStrEq(name, "StripItemSlot")) {
			int val = GetLoadoutSlotByName(kv->GetString());
			data.strip_item_slot.push_back(val == -1 ? kv->GetInt() : val);
		} else if (FStrEq(name, "SprayFile")) {
			Parse_SprayFile(data, kv);
		} else if (FStrEq(name, "SpellDropRateCommon")) {
			data.spell_drop_rate_common = Clamp(kv->GetFloat(), 0.0f, 1.0f);
			data.spell_drop = true;
		} else if (FStrEq(name, "SpellDropRateRare")) {
			data.spell_drop_rate_rare = Clamp(kv->GetFloat(), 0.0f, 1.0f);
			data.spell_drop = true;
		} else if (FStrEq(name, "AdditionalStepSound")) {
			data.override_step_sound = kv->GetString();
			data.has_override_step_sound = true;
			if (!enginesound->PrecacheSound(kv->GetString(), true))
				CBaseEntity::PrecacheScriptSound(kv->GetString());
		} else if (FStrEq(name, "StripItem")) {
			data.strip_item.push_back(kv->GetString());
		} else {
			found = false;
		}
		
		// Separate conditions that do not apply on default ecattr
		if (!found && default_ecattr != nullptr) {
			found = true;
			if (FStrEq(name, "AddCond")) {
				Parse_AddCond(data.addconds, kv);
			} else if (Parse_PeriodicTask(data.periodic_tasks, kv, name)) {

			} else {
				found = false;
			}
		}

		if (found)
			return true;

		if (!cvar_creators_custom_item.GetBool())
			return DETOUR_STATIC_CALL(ParseDynamicAttributes)(ecattr, kv);

		//DevMsg("ParseDynamicAttributesTFBot: \"%s\" \"%s\"\n", kv->GetName(), kv->GetString());
		
		CFastTimer timer2;
		timer2.Start();
		if (FStrEq(kv->GetName(), "ItemAttributes")) {
			//DevMsg("Item Attributes\n");
			std::map<std::string, std::string> attribs;
			std::string item_name = "";
			std::vector<KeyValues *> del_kv;

			std::string classname;

			
			FOR_EACH_SUBKEY( kv, subkey )
			{
				if (FStrEq(subkey->GetName(), "ItemName"))
				{
					item_name = subkey->GetString();
					auto it = item_defs.find(item_name);
					CTFItemDefinition *item_def;
					if (it != item_defs.end()) {
						item_def = it->second;
					}
					else {
						item_def = static_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(item_name.c_str()));
						item_defs[item_name] = item_def;
					}

					if (item_def == nullptr) {
						
						auto it_remap = item_custom_remap.find(item_name);

						if (it_remap == item_custom_remap.end() || it_remap->second) {
							int class_index = current_spawner->m_iClass;
							if (class_index == 0) {
								classname = current_spawner_kv->GetString("Class", "");
								
								class_index = GetClassIndexFromString(classname.c_str());
							}
							
							// Creators.tf Custom Weapons handling
							// engine->ServerCommand(CFmtStr("ce_mvm_get_itemdef_id \"%s\" \"%s\"\n", item_name.c_str(), classname.c_str()));
							// engine->ServerExecute();
							
							cell_t result = 0;
							cell_t item_id_result = -1;
							custom_item_get_def_id->PushString(item_name.c_str());
							custom_item_get_def_id->PushCell(class_index);
							custom_item_get_def_id->PushCellByRef(&item_id_result);
							custom_item_get_def_id->Execute(&result);

							DevMsg("Custom item name %s\n",item_name.c_str());
							if (item_id_result != -1) {
								item_def = static_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinition(item_id_result));
								std::string newname = item_def->GetName("");
								item_custom_remap[item_name] = true;
								
								DevMsg("Added custom item %s to reg %s\n",item_name.c_str(), newname.c_str());
								subkey->SetStringValue(newname.c_str());
								item_name = newname;

							}
							else {
								
								item_custom_remap[item_name] = false;
								DevMsg("Not found item %s, shown %d\n",item_name.c_str(),item_id_result);
								//item_custom_remap[item_name] = "";
							}
						}
						
					}
				}
				else
				{
					//DevMsg("attribute Name %s\n",subkey->GetName());
					if (current_spawner != nullptr && GetItemSchema()->GetAttributeDefinitionByName(subkey->GetName()) == nullptr) {
						DevMsg("Found unknown attribute %s\n",subkey->GetName());
						attribs[subkey->GetName()] = subkey->GetString();
						del_kv.push_back(subkey);
					}
				}
			}
			for (auto subkey : del_kv) {
				kv->RemoveSubKey(subkey);
				subkey->deleteThis();
			}
			data.custom_attrs[item_name] = attribs;
			//DevMsg("put event changed attributes data %s %d %d\n",item_name.c_str(), attribs.size(), &ecattr);
		}
		//DevMsg("  Passing through to actual ParseDynamicAttributes\n");
		return DETOUR_STATIC_CALL(ParseDynamicAttributes)(ecattr, kv);
	}

	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
		ClearAllData();
		return DETOUR_MEMBER_CALL(CPopulationManager_Parse)();
	}

	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_ParseEventChangeAttributes, KeyValues *data)
	{
		FOR_EACH_SUBKEY(data, subkey)
		{
			if (FStrEq(subkey->GetName(),"Default")) {
				subkey->SetName("DefaultSigOverride");
			}
		}
		default_ecattr = &reinterpret_cast<CTFBotSpawner *>(this)->m_DefaultAttrs;
		last_ecattr = nullptr;
		last_spawner = reinterpret_cast<CTFBotSpawner *>(this);
		bool result = DETOUR_MEMBER_CALL(CTFBotSpawner_ParseEventChangeAttributes)(data);
		default_ecattr = nullptr;

		return result;
	}

	bool HasRobotBlood(CTFPlayer *player) {
		static ConVarRef sig_mvm_bots_bleed("sig_mvm_bots_bleed");
		//static ConVarRef sig_mvm_bots_are_humans("sig_mvm_bots_are_humans");

		if (*(player->GetPlayerClass()->GetCustomModel()) == 0 )
			return true;

		if (sig_mvm_bots_bleed.GetBool())
			return true;

		auto data = GetDataForBot(player);
		if (data != nullptr && data->use_human_model)
			return true;

		return false;
	}

	void SetEyeColorForDiff(Vector &vec, int difficulty) {
        switch (difficulty) {
            case 0: vec.Init( 0, 240, 255 ); break;
            case 1: vec.Init( 0, 120, 255 ); break;
            case 2: vec.Init( 255, 100, 36); break;
            case 3: vec.Init( 255, 180, 36); break;
            default: vec.Init( 0, 240, 255 );
        }
    }

    THINK_FUNC_DECL(EyeParticle)
    {
		auto player = reinterpret_cast<CTFBot *>(this);
		auto data = GetDataForBot(player);
        
		if (data == nullptr) return;

        StopParticleEffects(player);

		Vector vColor;
		SetEyeColorForDiff(vColor, player->m_nBotSkill);

		if (data->eye_particle_color) {
			vColor.x = data->eye_particle_r;
			vColor.y = data->eye_particle_g;
			vColor.z = data->eye_particle_b;
		}
        Vector vColorL = vColor / 255;

        const char *particle = "bot_eye_glow";
        if (!data->custom_eye_particle.empty())
			particle = data->custom_eye_particle.c_str();

        CReliableBroadcastRecipientFilter filter;
        te_tf_particle_effects_control_point_t cp = { PATTACH_ABSORIGIN, vColor };

        const char *eye1 = player->IsMiniBoss() ? "eye_boss_1" : "eye_1";
        if (player->LookupAttachment(eye1) == 0)
            eye1 = "eyeglow_L";
        
        DispatchParticleEffect(particle, PATTACH_POINT_FOLLOW, player, eye1, vec3_origin, true, vColorL, vColorL, true, false, &cp, &filter);

        const char *eye2 = player->IsMiniBoss() ? "eye_boss_2" : "eye_2";
        if (player->LookupAttachment(eye2) == 0)
            eye2 = "eyeglow_R";

        DispatchParticleEffect(particle, PATTACH_POINT_FOLLOW, player, eye2, vec3_origin, true, vColorL, vColorL, true, false, &cp, &filter);
	}

	THINK_FUNC_DECL(SetBodygroup)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		auto data = GetDataForBot(bot);

		if (data == nullptr) return;

		for (const auto& pair : data->bodygroup) {
			const char *name = pair.first.c_str();
			int value = pair.second;

			int group = bot->FindBodygroupByName(name);
			if (group != -1) {
				bot->SetBodygroup(group, value);
			}
			DevMsg("Group %s %d %d\n", name, value, group);
			int count = bot->GetNumBodyGroups();
			for (int i = 0; i < count; i++) {
				name = bot->GetBodygroupName(i);
				DevMsg("Group sp %s %d\n", name, bot->GetBodygroupCount(i));
			}
		}
	}

	void ApplyCurrentEventChangeAttributes(CTFBot *bot)
	{
		auto data = GetDataForBot(bot);
		if (data != nullptr) {
			for(auto ititem = data->custom_attrs.begin(); ititem != data->custom_attrs.end(); ititem++) {

				ForEachTFPlayerEconEntity(bot, [&](CEconEntity *entity) {
					CEconItemView *item_view = entity->GetItem();
					if (item_view == nullptr) return;
					//DevMsg("Compare item to name %s %s\n",item_view->GetStaticData()->GetName(), ititem->first.c_str());
					if (FStrEq(item_view->GetStaticData()->GetName(), ititem->first.c_str())) {
						//DevMsg("Custom attrib count %d\n ",ititem->second.size());
						for(auto itattr = ititem->second.begin(); itattr != ititem->second.end(); itattr++) {
							DevMsg("Added custom attribute %s\n",itattr->first.c_str());
							
							cell_t result = 0;
							custom_item_set_attribute->PushCell(ENTINDEX(entity));
							custom_item_set_attribute->PushString(itattr->first.c_str());
							custom_item_set_attribute->PushString(itattr->second.c_str());
							custom_item_set_attribute->Execute(&result);
					
						//	engine->ServerCommand(CFmtStr("ce_mvm_set_attribute %d \"%s\" %s\n", ENTINDEX(entity), itattr->first.c_str(), itattr->second.c_str()));
						//	engine->ServerExecute();
						}
					}
				});
			}

			ApplyAddCond(bot, data->addconds, delayed_addconds);
			ApplyPendingTask(bot, data->periodic_tasks, pending_periodic_tasks);

			if (data->skin != -1){
				bot->SetForcedSkin(data->skin);
			}
			else
				bot->ResetForcedSkin();

			
			static ConVarRef sig_mvm_bots_are_humans("sig_mvm_bots_are_humans");
			static ConVarRef sig_mvm_bots_bleed("sig_mvm_bots_bleed");
			if (!data->use_custom_model.empty()) {
				
				bot->GetPlayerClass()->SetCustomModel(data->use_custom_model.c_str(), true);
				bot->UpdateModel();
				bot->SetBloodColor(DONT_BLEED);
				
				// TODO: RomeVision...?
			} else if (data->use_buster_model) {
				
				// here we mimic what CMissionPopulator::UpdateMissionDestroySentries does
				bot->GetPlayerClass()->SetCustomModel("models/bots/demo/bot_sentry_buster.mdl", true);
				bot->UpdateModel();
				bot->SetBloodColor(DONT_BLEED);
				
				// TODO: filter-out addition of Romevision cosmetics to UseBusterModel bots
				// TODO: manually add Romevision cosmetic for SentryBuster to UseBusterModel bots
			} else if (data->use_human_model != 0 || sig_mvm_bots_are_humans.GetBool()) {
				
				bool can_be_sapped = data->use_human_model == 2 || sig_mvm_bots_are_humans.GetInt() == 2;
				// calling SetCustomModel with a nullptr string *seems* to reset the model
				// dunno what the bool parameter should be; I think it doesn't matter for the nullptr case
				bot->GetPlayerClass()->SetCustomModel(nullptr, true);
				bot->UpdateModel();
				bot->SetBloodColor(BLOOD_COLOR_RED);
				
				//Cannot be sapped custom attribute
				if (!can_be_sapped) {
					auto sap_def = GetItemSchema()->GetAttributeDefinitionByName("cannot be sapped");
					if (sap_def != nullptr)
						bot->GetAttributeList()->SetRuntimeAttributeValue(sap_def, 1.0f);
				}
				
				// TODO: filter-out addition of Romevision cosmetics to UseHumanModel bots
			}

			if (HasRobotBlood(bot)) {
				bot->SetBloodColor(BLOOD_COLOR_RED);
			}

			if (data->use_human_animations) {
				CEconWearable *wearable = static_cast<CEconWearable *>(ItemGeneration()->SpawnItem(PLAYER_ANIM_WEARABLE_ITEM_ID, Vector(0,0,0), QAngle(0,0,0), 6, 9999, "tf_wearable"));
				DevMsg("Use human anims %d\n", wearable != nullptr);
				if (wearable != nullptr) {
					
					wearable->m_bValidatedAttachedEntity = true;
					wearable->GiveTo(bot);
					servertools->SetKeyValue(bot, "rendermode", "1");
					bot->SetRenderColorA(0);
					bot->EquipWearable(wearable);
					const char *path = bot->GetPlayerClass()->GetCustomModel();
					int model_index = CBaseEntity::PrecacheModel(path);
					wearable->SetModelIndex(model_index);
					for (int j = 0; j < MAX_VISION_MODES; ++j) {
						wearable->SetModelIndexOverride(j, model_index);
					}
					bot->GetPlayerClass()->SetCustomModel(nullptr, true);
				}
				
			}

			for (const auto& pair : data->item_colors) {
				const char *item_name     = pair.first.c_str();
				const color32& item_color = pair.second;

				CEconEntity *entity = bot->GetEconEntityByName(item_name);
				if (entity != nullptr) {
					DevMsg("CTFBotSpawner %08x: applying color %02X%02X%02X to item \"%s\"\n",
						(uintptr_t)&data, item_color.r, item_color.g, item_color.b, item_name);
					
					entity->SetRenderColorR(item_color.r);
					entity->SetRenderColorG(item_color.g);
					entity->SetRenderColorB(item_color.b);
				}
			}

			for (const auto& pair : data->item_models) {
				const char *item_name     = pair.first.c_str();
				const char *item_model = pair.second.c_str();
				
				CEconEntity *entity = bot->GetEconEntityByName(item_name);
				if (entity != nullptr) {
					int model_index = CBaseEntity::PrecacheModel(item_model);
					entity->SetModelIndex(model_index);
					for (int i = 0; i < MAX_VISION_MODES; ++i) {
						entity->SetModelIndexOverride(i, model_index);
					}
				}
			}

			CTFWearable *pActionSlotEntity = bot->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION );
			if ( pActionSlotEntity  != nullptr) {

				// get the equipped item and see what it is
				CTFPowerupBottle *pPowerupBottle = rtti_cast< CTFPowerupBottle* >( pActionSlotEntity );
				if (pPowerupBottle  != nullptr) {
					int val=0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER(pPowerupBottle, val, powerup_charges);
					pPowerupBottle->m_usNumCharges = val;
				}
			}

			for (const auto& pair : data->custom_weapon_models) {
				int slot         = pair.first;
				const char *path = pair.second.c_str();
				
				CBaseEntity *item;
				if ((item = bot->GetEquippedWearableForLoadoutSlot(slot)) == nullptr &&
					(item = bot->Weapon_GetSlot(slot)) == nullptr) {
					DevMsg("CTFBotSpawner %08x: can't find item slot %d for CustomWeaponModel\n",
						(uintptr_t)&data, slot);
					continue;
				}
				
				DevMsg("CTFBotSpawner %08x: item slot %d is entity #%d classname \"%s\"\n",
					(uintptr_t)&data, slot, ENTINDEX(item), item->GetClassname());
				
				DevMsg("CTFBotSpawner %08x: changing item model to \"%s\"\n",
					(uintptr_t)&data, path);
				
				int model_index = CBaseEntity::PrecacheModel(path);
				for (int i = 0; i < MAX_VISION_MODES; ++i) {
					item->SetModelIndexOverride(i, model_index);
				}
			}

			if (!data->bodygroup.empty()) {
				THINK_FUNC_SET(bot, SetBodygroup, gpGlobals->curtime + 0.1f);
			}

			if (data->custom_eye_particle != "" || data->eye_particle_color) {
				THINK_FUNC_SET(bot, EyeParticle, gpGlobals->curtime + 0.1f);
			}

			//Replenish clip, if clip bonus is being applied
			for (int i = 0; i < bot->WeaponCount(); ++i) {
				CBaseCombatWeapon *weapon = bot->GetWeapon(i);
				if (weapon == nullptr) continue;
				
				int fire_when_full = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, fire_when_full, auto_fires_full_clip);

				if (fire_when_full == 0)
					weapon->m_iClip1 = weapon->GetMaxClip1();
			}

			CBaseClient *client = static_cast<CBaseClient *> (sv->GetClient(ENTINDEX(bot) - 1));
			client->m_nCustomFiles[0].crc = data->spray_file;

			if (!data->strip_item.empty()) {
				for (std::string &itemname : data->strip_item) {
					ForEachTFPlayerEconEntity(bot, [&](CEconEntity *entity){
						if (entity->GetItem() != nullptr && FStrEq(entity->GetItem()->GetItemDefinition()->GetName(), itemname.c_str())) {
							if (entity->MyCombatWeaponPointer() != nullptr) {
								bot->Weapon_Detach(entity->MyCombatWeaponPointer());
							}
							entity->Remove();
						}
					});
				}
				bot->EquipBestWeaponForThreat(nullptr);
			}
		}
	}
	
	RefCount rc_CTFBotSpawner_Spawn;
	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		SCOPED_INCREMENT(rc_CTFBotSpawner_Spawn);
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		auto result = DETOUR_MEMBER_CALL(CTFBotSpawner_Spawn)(where, ents);

		// Delay execution of the first event change attributes
		if (result) {
			if (ents != nullptr && !ents->IsEmpty()) {
				CTFBot *bot = ToTFBot(ents->Tail());
				if (bot != nullptr) {
					ApplyCurrentEventChangeAttributes(bot);
				}
			}
		}
		return result;
	}

	RefCount rc_CTFBot_OnEventChangeAttributes;
	DETOUR_DECL_MEMBER(void, CTFBot_OnEventChangeAttributes, CTFBot::EventChangeAttributes_t *ecattr)
	{
		SCOPED_INCREMENT(rc_CTFBot_OnEventChangeAttributes);

		CTFBot *bot = reinterpret_cast<CTFBot *>(this);

		if (GetDataForBot(bot) != nullptr) {
			ClearDataForBot(bot, CHANGE);
		}

		// Load ctfbot's ecattr data
		auto data = &extdata[ecattr];
		ecattr_map[bot] = data;

		for (int slot : data->strip_item_slot) {
			CBaseEntity *item;
			bool held_item = false;
			if ((item = bot->GetEquippedWearableForLoadoutSlot(slot)) != nullptr || (item = bot->Weapon_GetSlot(slot)) != nullptr) {
				CBaseCombatWeapon *weapon;

				if (bot->GetActiveTFWeapon() == item) {
					held_item = true;
				}

				if ((weapon = item->MyCombatWeaponPointer()) != nullptr) {
					bot->Weapon_Detach(weapon);
				}

				item->Remove();
			}

			//if (held_item)
			bot->EquipBestWeaponForThreat(nullptr);
		}

		DETOUR_MEMBER_CALL(CTFBot_OnEventChangeAttributes)(ecattr);


		if (!rc_CTFBotSpawner_Spawn)
			ApplyCurrentEventChangeAttributes(bot);
		else
			// Reset model to default (prevent sentry buster model lingering on common bots on halloween missions)
			bot->GetPlayerClass()->SetCustomModel("", true);

		//DevMsg("OnEventChange %d %d %d %d\n",bot != nullptr, data != nullptr, ecattr, current_spawner);
		
		
		
	}

	RefCount rc_CTFBot_AddItem;
	DETOUR_DECL_MEMBER(int, CTFItemDefinition_GetLoadoutSlot, int classIndex)
	{
		CTFItemDefinition *item_def=reinterpret_cast<CTFItemDefinition *>(this);
		int slot = DETOUR_MEMBER_CALL(CTFItemDefinition_GetLoadoutSlot)(classIndex);
		if (rc_CTFBot_OnEventChangeAttributes) {
			const char *name = item_def->GetItemClass();
			if (rc_CTFBot_AddItem && (FStrEq(name,"tf_weapon_buff_item") || FStrEq(name,"tf_weapon_parachute") || strncmp(name,"tf_wearable", strlen("tf_wearable")) == 0))
				return slot;
			
			if (slot == -1){
				if(FStrEq(name,"tf_weapon_revolver"))
					slot = 0;
				else
					slot = item_def->GetLoadoutSlot(TF_CLASS_UNDEFINED);
			}
		}
		return slot;
	}
	
	bool is_revolver;
	const char *classname_gl;
	const char *item_name;
	CTFBot *bot_additem;
	int bot_classnum = TF_CLASS_UNDEFINED;
	DETOUR_DECL_MEMBER(void *, CItemGeneration_GenerateRandomItem, void *criteria, const Vector &vec, const QAngle &ang)
	{
		if (rc_CTFBot_AddItem > 0) {

			auto it = item_defs.find(item_name);
			CTFItemDefinition *item_def;
			
			void *ret = nullptr;
			if (it != item_defs.end()){
				item_def = it->second;
			}
			else {
				item_def = static_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(item_name));
				//if (item_def == nullptr)
				//	item_def = Mod::Pop::PopMgr_Extensions::GetCustomWeaponItemDef(item_name);
				item_defs[item_name] = item_def;
			}

			if (item_def != nullptr) {
				//No romevision cosmetics
				if (item_def->m_iItemDefIndex >= 30143 && item_def->m_iItemDefIndex <= 30161 && cvar_no_romevision.GetBool())
					return nullptr;

				const char *classname = TranslateWeaponEntForClass_improved(item_def->GetItemClass(), bot_classnum);
				//CEconItemView *item_view= CEconItemView::Create();
				//item_view->Init(item_def->m_iItemDefIndex, 6, 9999, 0);
				ret = ItemGeneration()->SpawnItem(item_def->m_iItemDefIndex,vec, ang, 6, 9999, classname);
				//CEconItemView::Destroy(item_view);

				if (ret != nullptr)
					Mod::Pop::PopMgr_Extensions::AddCustomWeaponAttributes(item_name, reinterpret_cast<CEconEntity *>(ret)->GetItem());

			}

			if (ret == nullptr){
				if (cvar_creators_custom_item.GetBool() && bot_additem ) {
					DevMsg("equip custom ctf item %s\n", item_name);

					cell_t result = 0;
					custom_item_equip_itemname->PushCell(ENTINDEX(bot_additem));
					custom_item_equip_itemname->PushString(item_name);
					custom_item_equip_itemname->Execute(&result);

					//engine->ServerCommand(CFmtStr("ce_mvm_equip_itemname %d \"%s\"\n", ENTINDEX(bot_additem), item_name));
					//engine->ServerExecute();

					// static ConVarRef ce_mvm_equip_itemname_cvar("sig_mvm_set_credit_team");
					// if (sig_mvm_set_credit_team.IsValid() && sig_mvm_set_credit_team.GetBool()) {
					// 	return sig_mvm_set_credit_team.GetInt();
					// }

					bot_additem = nullptr;
				}

				else
					ret = DETOUR_MEMBER_CALL(CItemGeneration_GenerateRandomItem)(criteria,vec,ang);
			}
			

			return ret;
		}
		return DETOUR_MEMBER_CALL(CItemGeneration_GenerateRandomItem)(criteria,vec,ang);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBot_EquipRequiredWeapon)
	{
		auto result = DETOUR_MEMBER_CALL(CTFBot_EquipRequiredWeapon)();
		if (!result)
		{
			auto bot = reinterpret_cast<CTFBot *>(this);
			if (bot->m_iWeaponRestrictionFlags & CTFBot::PDA_ONLY)
			{
				bot->Weapon_Switch(bot->Weapon_GetSlot(TF_WPN_TYPE_PDA));
				return true;
			}
			if (bot->m_iWeaponRestrictionFlags & CTFBot::BUILDING_ONLY)
			{
				bot->Weapon_Switch(bot->Weapon_GetSlot(TF_WPN_TYPE_BUILDING));
				return true;
			}
		}
		return result;
	}
	
	DETOUR_DECL_MEMBER(CEconItemDefinition *, CEconItemSchema_GetItemDefinitionByName, const char *name)
	{
		name = Mod::Pop::PopMgr_Extensions::GetCustomWeaponNameOverride(name);
		return DETOUR_MEMBER_CALL(CEconItemSchema_GetItemDefinitionByName)(name);
	}
	// DETOUR_DECL_MEMBER(void *, CSchemaFieldHandle_CEconItemDefinition, const char* name) {
	// 	DevMsg("CShemaItemDefHandle 1 %s %d\n",name, rc_CTFBot_OnEventChangeAttributes);
	// 	return DETOUR_MEMBER_CALL(CSchemaFieldHandle_CEconItemDefinition)(name);
	// }
	// DETOUR_DECL_MEMBER(void *, CSchemaFieldHandle_CEconItemDefinition2, const char* name) {
	// 	DevMsg("CShemaItemDefHandle 2 %s %d\n",name, rc_CTFBot_OnEventChangeAttributes);
	// 	return DETOUR_MEMBER_CALL(CSchemaFieldHandle_CEconItemDefinition2)(name);
	// }
	
	DETOUR_DECL_MEMBER(void, CTFBot_AddItem, const char *item)
	{
		SCOPED_INCREMENT(rc_CTFBot_AddItem);
		//clock_t start = clock();
		item_name = item;
		bot_additem = reinterpret_cast<CTFBot *>(this);
		bot_classnum = bot_additem->GetPlayerClass()->GetClassIndex();
		DETOUR_MEMBER_CALL(CTFBot_AddItem)(item);
	}

	bool ShouldRocketJump(CTFBot *bot, EventChangeAttributesData *data, bool release_fire) {
		if (data->rocket_jump_type == 0)
			return false;

		if (bot->m_Shared->InCond( TF_COND_BLASTJUMPING ))
			return false;
		
		CTFWeaponBase *weapon = bot->GetActiveTFWeapon();
		if (weapon == nullptr)
			return false;

		if (data->rocket_jump_type == 2 && weapon->m_iClip1 < weapon->GetMaxClip1())
			return false;

		return true;

		// int almost_ready_clip = weapon->m_iClip1; 
		// if (release_fire && weapon->m_flNextPrimaryAttack < gpGlobals->curtime + 0.2f)
		// 	almost_ready_clip += 1;

		// return almost_ready_clip > 0 && weapon->m_flNextPrimaryAttack < gpGlobals->curtime && bot->GetItem() == nullptr
		// 	&& (data->rocket_jump_type == 1 || (data->rocket_jump_type == 2 && weapon->GetMaxClip1() <= almost_ready_clip)) && !bot->m_Shared->InCond( TF_COND_BLASTJUMPING );
	}

	DETOUR_DECL_MEMBER(Vector, CTFBotMainAction_SelectTargetPoint, const INextBot *nextbot, CBaseCombatCharacter *subject)
	{
		auto bot = ToTFBot(nextbot->GetEntity());

		if (bot != nullptr) {
			auto data = GetDataForBot(bot);

			if (data != nullptr) {

				Vector aim = subject->WorldSpaceCenter();
				
				auto weapon = bot->GetActiveTFWeapon();

				if (data->aim_at != AIM_DEFAULT) {
					
					if ( data->aim_at == AIM_FEET && bot->GetVisionInterface()->IsAbleToSee( subject->GetAbsOrigin(), IVision::FieldOfViewCheckType::DISREGARD_FOV ) )
						aim = subject->GetAbsOrigin();

					else if ( data->aim_at == AIM_HEAD && bot->GetVisionInterface()->IsAbleToSee( subject->EyePosition(), IVision::FieldOfViewCheckType::DISREGARD_FOV) )
						aim = subject->EyePosition();
				}
				else 
					aim = DETOUR_MEMBER_CALL(CTFBotMainAction_SelectTargetPoint)(nextbot,subject);
				
				aim += data->aim_offset;

				if (data->aim_lead_target_speed > 0) {
					float speed = data->aim_lead_target_speed;
					if (speed == 1.0f) {
						auto find = data->projectile_speed_cache.find(weapon);
						if (find == data->projectile_speed_cache.end() || gpGlobals->tickcount % 10 == 0) {
							data->projectile_speed_cache[weapon] = speed = CalculateProjectileSpeed(rtti_cast<CTFWeaponBaseGun *>(weapon));
						}
						else {
							speed = data->projectile_speed_cache[weapon];
						}
					}
					auto distance = nextbot->GetRangeTo(subject->GetAbsOrigin());

					float time_to_travel = distance / speed;

					Vector target_pos = aim + time_to_travel * subject->GetAbsVelocity();

					if (bot->GetVisionInterface()->IsAbleToSee( target_pos, IVision::FieldOfViewCheckType::DISREGARD_FOV))
						aim = target_pos;
				}
				return aim;
			}
		}
		return DETOUR_MEMBER_CALL(CTFBotMainAction_SelectTargetPoint)(nextbot,subject);
	}

	ConVar cvar_head_tracking_interval("sig_head_tracking_interval_multiplier", "1", FCVAR_NONE,	
		"Head tracking interval multiplier");	
	DETOUR_DECL_MEMBER(float, CTFBotBody_GetHeadAimTrackingInterval)
	{
		auto body = reinterpret_cast<PlayerBody *>(this);

		float mult = cvar_head_tracking_interval.GetFloat();

		auto bot = body->GetEntity();
		
		auto data = GetDataForBot(bot);
		if (data != nullptr && data->tracking_interval >= 0.f) {
			return data->tracking_interval;
		}
		else
			return DETOUR_MEMBER_CALL(CTFBotBody_GetHeadAimTrackingInterval)() * mult;

		
	}
	DETOUR_DECL_MEMBER(float, PlayerBody_GetMaxHeadAngularVelocity)
	{
		auto body = reinterpret_cast<PlayerBody *>(this);

		float mult = cvar_head_tracking_interval.GetFloat();

		auto bot = ToTFBot(body->GetEntity());
		
		auto data = GetDataForBot(bot);

		if (data != nullptr && bot != nullptr && ShouldRocketJump(bot, data, false)) {
			mult *= 2.5f;
		}

		if (data != nullptr && data->head_rotate_speed >= 0.f) {
			return data->head_rotate_speed;
		}

		if (data != nullptr && data->tracking_interval >= 0.f && data->tracking_interval < 0.5f)
			return 10000.0f;
		else
			return DETOUR_MEMBER_CALL(PlayerBody_GetMaxHeadAngularVelocity)();
	}
	
	DETOUR_DECL_MEMBER(void, CTFBotMainAction_FireWeaponAtEnemy, CTFBot *actor)
	{
		DETOUR_MEMBER_CALL(CTFBotMainAction_FireWeaponAtEnemy)(actor);
		auto data = GetDataForBot(actor);
		if (data != nullptr && data->rocket_jump_type > 0) {
			auto weapon = actor->GetActiveTFWeapon();
			const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
			if (weapon != nullptr && (data->rocket_jump_type == 1 || weapon->m_iClip1 >= weapon->GetMaxClip1()) && threat != nullptr && threat->GetEntity() != nullptr && actor->IsLineOfFireClear( threat->GetEntity()->EyePosition() )/*&& ShouldRocketJump(actor, weapon, data, true)*//*weapon != nullptr*/) {
				
				if (actor->GetFlags() & FL_ONGROUND ) {
					actor->ReleaseFireButton();
					actor->SetAttribute(CTFBot::ATTR_SUPPRESS_FIRE);
				}
				if (!actor->m_Shared->InCond( TF_COND_BLASTJUMPING )) {
					if (weapon->m_flNextPrimaryAttack < gpGlobals->curtime ) {

						Vector dir = actor->GetAbsOrigin() - threat->GetEntity()->GetAbsOrigin();

						if (actor->GetItem() != nullptr) {
							dir = -actor->GetAbsVelocity();
						}

						dir.z = 0;
						dir.NormalizeInPlace();

						Vector actoreyes;
						AngleVectors(actor->EyeAngles(), &actoreyes, NULL, NULL);
						actoreyes.z = 0;
						actoreyes.NormalizeInPlace();

						Vector aim = actor->GetAbsOrigin() + dir * 28;
						
						actor->GetBodyInterface()->AimHeadTowards(aim, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.10f, NULL, "Aiming at target we need to destroy to progress");

						if (DotProduct(dir,actoreyes) > 0.78) {
							actor->PressJumpButton(0.1f);
							if ((actor->GetFlags() & FL_ONGROUND) != 0) {
								actor->m_nBotAttrs = actor->m_nBotAttrs & ~(CTFBot::ATTR_SUPPRESS_FIRE);
								actor->PressFireButton(0.1f);
							}
						}

						if ((actor->GetFlags() & FL_ONGROUND) == 0) {
							actor->PressCrouchButton(0.1f);
						}
						//if (!actor->GetFlags() & FL_ONGROUND && DotProduct(dir,actoreyes) > 0.78/*DotProduct(dir,actoreyes) > 0.85*/) {
						//	
						//}
					}
				}
				else {
					actor->m_nBotAttrs = actor->m_nBotAttrs & ~(CTFBot::ATTR_SUPPRESS_FIRE);
				}
				
				//if ((data->rocket_jump_type == 1 || (data->rocket_jump_type == 2 && weapon->GetMaxClip1() == weapon->m_iClip1)) && !actor->m_Shared->InCond( TF_COND_BLASTJUMPING )) {

				//}
			}
		}
	}
	/*
	DETOUR_DECL_STATIC(CBaseEntity *, Creat
	eEntityByName, const char *className, int iForceEdictIndex)
	{
		if (rc_CTFBot_AddItem > 0) {
			return DETOUR_STATIC_CALL(CreateEntityByName)(TranslateWeaponEntForClass_improved(className,bot_classnum), iForceEdictIndex);
		}
		return DETOUR_STATIC_CALL(CreateEntityByName)(className, iForceEdictIndex);
	}*/
	DETOUR_DECL_MEMBER(void, CTFBot_EquipBestWeaponForThreat, const CKnownEntity * threat)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		bool mannvsmachine = TFGameRules()->IsMannVsMachineMode();
		
		bool use_best_weapon = false;
		
		auto data = GetDataForBot(bot);
		if (data != nullptr && data->use_best_weapon) {
			use_best_weapon = true;
		}

		if (use_best_weapon)
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);

		DETOUR_MEMBER_CALL(CTFBot_EquipBestWeaponForThreat)(threat);

		if (use_best_weapon)
			TFGameRules()->Set_m_bPlayingMannVsMachine(mannvsmachine);
	}
	
	DETOUR_DECL_MEMBER(const CKnownEntity *, CTFBotMainAction_SelectMoreDangerousThreatInternal, const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2)
	{
		auto action = reinterpret_cast<const CTFBotMainAction *>(this);
		
		// TODO: make the perf impact of this less obnoxious if possible
		CTFBot *actor = ToTFBot(nextbot->GetEntity());
		if (actor != nullptr) {
			auto data = GetDataForBot(actor);
			if (data != nullptr) {
				/* do the exact same thing as the game code does when the bot has WeaponRestrictions MeleeOnly */
				if (data->use_melee_threat_prioritization) {
					return action->SelectCloserThreat(actor, threat1, threat2);
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFBotMainAction_SelectMoreDangerousThreatInternal)(nextbot, them, threat1, threat2);
	}

	DETOUR_DECL_MEMBER(bool,CTFBotDeliverFlag_UpgradeOverTime, CTFBot *bot)
	{
		auto data = GetDataForBot(bot);
		if (data != nullptr && data->no_bomb_upgrade) {
			return false;
		}
		return DETOUR_MEMBER_CALL(CTFBotDeliverFlag_UpgradeOverTime)(bot);
	}

	
	DETOUR_DECL_MEMBER(void, CTFPlayer_PainSound, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (TFGameRules()->IsMannVsMachineMode()) {
			auto data = GetDataForBot(player);
			if (data != nullptr) {
				if (data->pain_sound != "DEF") {
					player->EmitSound(STRING(AllocPooledString(data->pain_sound.c_str())));
					return;
				}
			}

		}
		DETOUR_MEMBER_CALL(CTFPlayer_PainSound)(info);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_DeathSound, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (TFGameRules()->IsMannVsMachineMode()) {
			auto data = GetDataForBot(player);
			if (data != nullptr) {
				if (data->death_sound != "DEF") {
					player->EmitSound(STRING(AllocPooledString(data->death_sound.c_str())));
					return;
				}
			}

		}
		DETOUR_MEMBER_CALL(CTFPlayer_DeathSound)(info);
	}

	void UpdateAlwaysGlow()
	{
		if (gpGlobals->tickcount % 3 == 0) {
			ForEachTFBot([](CTFBot *bot){	
				if (!bot->IsAlive()) return;
				
				auto data = GetDataForBot(bot);
				if (data != nullptr && data->always_glow) {
					if (!bot->IsGlowEffectActive()){
						bot->AddGlowEffect();
					}
				}
				if (data != nullptr && data->no_glow) {
					if (bot->IsGlowEffectActive()){
						bot->RemoveGlowEffect();
					}
				}
			});
		}
	}

	void UpdateRingOfFire()
	{
		static int ring_of_fire_tick_interval = (int)(0.500f / (float)gpGlobals->interval_per_tick);
		if (gpGlobals->tickcount % ring_of_fire_tick_interval == 0) {
			ForEachTFBot([](CTFBot *bot){
				if (!bot->IsAlive()) return;
				
				auto data = GetDataForBot(bot);
				if (data != nullptr && data->ring_of_fire >= 0.0f) {
					ForEachEntityInSphere(bot->GetAbsOrigin(), 135.0f, [&](CBaseEntity *ent){
						CTFPlayer *victim = ToTFPlayer(ent);
						if (victim == nullptr) return;
						
						if (victim->GetTeamNumber() == bot->GetTeamNumber()) return;
						if (victim->m_Shared->IsInvulnerable())              return;
						
						Vector victim_mins = victim->GetPlayerMins();
						Vector victim_maxs = victim->GetPlayerMaxs();
						
						if (bot->GetAbsOrigin().z >= victim->GetAbsOrigin().z + victim_maxs.z) return;
						
						Vector closest;
						victim->CollisionProp()->CalcNearestPoint(bot->GetAbsOrigin(), &closest);
						if (closest.DistToSqr(bot->GetAbsOrigin()) > Square(135.0f)) return;
						
						// trace start should be minigun WSC
						trace_t tr;
						UTIL_TraceLine(bot->WorldSpaceCenter(), victim->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, bot, COLLISION_GROUP_PROJECTILE, &tr);
						
						if (tr.fraction == 1.0f || tr.m_pEnt == victim) {
							Vector bot_origin = bot->GetAbsOrigin();
							victim->TakeDamage(CTakeDamageInfo(bot, bot, bot->GetActiveTFWeapon(), vec3_origin, bot_origin, data->ring_of_fire, DMG_IGNITE, 0, &bot_origin));
							victim->m_Shared->Burn(bot,nullptr,7.5f);
						}
					});
					
					DispatchParticleEffect("heavy_ring_of_fire", bot->GetAbsOrigin(), vec3_angle);
				}
			});
		}
	}

	DETOUR_DECL_MEMBER(int, CTFGameRules_ApplyOnDamageModifyRules, CTakeDamageInfo& info, CBaseEntity *pVictim, bool b1)
	{
		auto data = GetDataForBot(pVictim);
		if (data != nullptr) {
			auto pTFWeapon = static_cast<CTFWeaponBase *>(ToBaseCombatWeapon(info.GetWeapon()));
			if (pTFWeapon != nullptr) {
				int weapon_id = pTFWeapon->GetWeaponID();
				
				auto it = data->weapon_resists.find(pTFWeapon->GetWeaponID());
				if (it != data->weapon_resists.end()) {
					float resist = (*it).second;
					info.ScaleDamage(resist);
					DevMsg("Bot #%d taking damage from weapon_id 0x%02x; resist is %4.2f\n", ENTINDEX(pVictim), weapon_id, resist);
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFGameRules_ApplyOnDamageModifyRules)(info, pVictim, b1);
	}
	DETOUR_DECL_MEMBER(CTFProjectile_Rocket *, CTFWeaponBaseGun_FireRocket, CTFPlayer *player, int i1)
	{
		auto proj = DETOUR_MEMBER_CALL(CTFWeaponBaseGun_FireRocket)(player, i1);
		
		if (proj != nullptr) {
			auto data = GetDataForBot(proj->GetOwnerEntity());
			if (data != nullptr) {
				if (!data->rocket_custom_model.empty()) {
					proj->SetModel(data->rocket_custom_model.c_str());
				}
				
				if (!data->rocket_custom_particle.empty()) {
					if (data->rocket_custom_particle.front() == '~') {
						StopParticleEffects(proj);
						DispatchParticleEffect(data->rocket_custom_particle.c_str() + 1, PATTACH_ABSORIGIN_FOLLOW, proj, INVALID_PARTICLE_ATTACHMENT, false);
					} else {
						DispatchParticleEffect(data->rocket_custom_particle.c_str(), PATTACH_ABSORIGIN_FOLLOW, proj, INVALID_PARTICLE_ATTACHMENT, false);
					}
				}
			}
		}
		
		return proj;
	}
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ApplyOnHitAttributes, CBaseEntity *ent, CTFPlayer *player, const CTakeDamageInfo& info)
	{
		DETOUR_MEMBER_CALL(CTFWeaponBase_ApplyOnHitAttributes)(ent, player, info);
		
		CTFPlayer *victim = ToTFPlayer(ent);
		CTFBot *attacker  = ToTFBot(player);
		if (victim != nullptr && attacker != nullptr) {
			auto data = GetDataForBot(attacker);
			if (data != nullptr) {
				for (const auto& addcond : data->dmgappliesconds) {
					if ((addcond.health_below == 0 || addcond.health_below >= attacker->GetHealth()) &&
						(addcond.health_above == 0 || addcond.health_above < attacker->GetHealth()))
					victim->m_Shared->AddCond(addcond.cond, addcond.duration, attacker);
				}
			}
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFProjectile_Rocket_Spawn)
	{
		DETOUR_MEMBER_CALL(CTFProjectile_Rocket_Spawn)();
		
		auto rocket = reinterpret_cast<CTFProjectile_Rocket *>(this);
		
		auto data = GetDataForBot(rocket->GetOwnerPlayer());
		if (data != nullptr) {
			if (data->homing_rockets.enable) {
				rocket->SetMoveType(MOVETYPE_CUSTOM);
			}
		}
	}
	
	bool HasRobotHumanVoice(CTFPlayer *player) {
		static ConVarRef sig_mvm_human_bots_robot_voice("sig_mvm_human_bots_robot_voice");
		static ConVarRef sig_mvm_bots_are_humans("sig_mvm_bots_are_humans");

		if (sig_mvm_human_bots_robot_voice.GetBool())
			return false;

		if (sig_mvm_bots_are_humans.GetBool())
			return true;

		auto data = GetDataForBot(player);
		if (data != nullptr && data->use_human_model) {
			return true;
		}

		return false;
	}

	DETOUR_DECL_MEMBER(const char *, CTFPlayer_GetSceneSoundToken)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->IsBot() && HasRobotHumanVoice(player)) {
			return "";
		}
		//const char *token=DETOUR_MEMBER_CALL( CTFPlayer_GetSceneSoundToken)();
		//DevMsg("CTFPlayer::GetSceneSoundToken %s\n", token);
		return DETOUR_MEMBER_CALL( CTFPlayer_GetSceneSoundToken)();
	}

//	std::string GetStrForEntity(CBaseEntity *ent)
//	{
//		if (ent == nullptr) {
//			return "nullptr";
//		}
//		
//		CTFPlayer *player = ToTFPlayer(ent);
//		if (player == nullptr) {
//			return CFmtStr("[#%-4d] entity \"%s\" on teamnum %d", ENTINDEX(ent), ent->GetClassname(), ent->GetTeamNumber()).Get();
//		}
//		
//		return CFmtStr("[#%-4d] player \"%s\" on teamnum %d", ENTINDEX(player), player->GetPlayerName(), player->GetTeamNumber()).Get();
//	}
//	
//	void DeflectDebug(CBaseEntity *ent)
//	{
//		if (ent == nullptr) return;
//		
//		auto rocket = rtti_cast<CTFProjectile_Rocket *>(ent);
//		if (rocket == nullptr) return;
//		
//		DevMsg("\n"
//			"[#%-4d] tf_projectile_rocket on teamnum %d\n"
//			"  rocket->GetOwnerEntity():             %s\n"
//			"  rocket->GetOwnerPlayer():             %s\n"
//			"  IScorer::GetScorer():                 %s\n"
//			"  IScorer::GetAssistant():              %s\n"
//			"  CBaseProjectile::m_hOriginalLauncher: %s\n"
//			"  CTFBaseRocket::m_hLauncher:           %s\n"
//			"  CTFBaseRocket::m_iDeflected:          %d\n",
//			ENTINDEX(rocket), rocket->GetTeamNumber(),
//			GetStrForEntity(rocket->GetOwnerEntity()).c_str(),
//			GetStrForEntity(rocket->GetOwnerPlayer()).c_str(),
//			GetStrForEntity(rtti_cast<IScorer *>(rocket)->GetScorer()).c_str(),
//			GetStrForEntity(rtti_cast<IScorer *>(rocket)->GetAssistant()).c_str(),
//			GetStrForEntity(rocket->GetOriginalLauncher()).c_str(),
//			GetStrForEntity(rocket->GetLauncher()).c_str(),
//			(int)rocket->m_iDeflected);
//		
//		// ->GetOwner
//		// ->GetOwnerPlayer
//		
//		// IScorer::GetScorer
//		// IScorer::GetAssistant
//		
//		// CBaseProjectile::m_hOriginalLauncher
//		
//		// CTFBaseRocket::m_hLauncher
//		// CTFBaseRocket::m_iDeflected
//		
//		// CBaseGrenade::m_hThrower
//		
//		// CTFWeaponBaseGrenadeProj::m_hLauncher
//		// CTFWeaponBaseGrenadeProj::m_iDeflected
//		// CTFWeaponBaseGrenadeProj::m_hDeflectOwner
//	}
	
	// =========================================================================
	// VALUE                                   BEFORE DEFLECT --> AFTER DEFLECT
	// =========================================================================
	// IScorer::GetScorer():                   soldier        --> soldier
	// IScorer::GetAssistant():                <nullptr>      --> <nullptr>
	// CBaseEntity::GetOwnerEntity():          soldier        --> pyro
	// CBaseProjectile::GetOriginalLauncher(): rocketlauncher --> rocketlauncher
	// CTFBaseRocket::GetOwnerPlayer():        soldier        --> pyro
	// CTFBaseRocket::m_hLauncher:             rocketlauncher --> flamethrower
	// CTFBaseRocket::m_iDeflected:            0              --> 1
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_PerformCustomPhysics, Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity)
	{
		CTFProjectile_Rocket *rocket = nullptr;
		const HomingRockets *hr      = nullptr;
		
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		const char *classname = ent->GetClassname();

		if ((strcmp(classname, "tf_projectile_rocket") == 0 || strcmp(classname, "tf_projectile_sentryrocket") == 0)) {
			rocket = static_cast<CTFProjectile_Rocket *>(ent);
			auto data = GetDataForBot(rtti_scast<IScorer *>(rocket)->GetScorer());
			if (data != nullptr) {
				hr = &data->homing_rockets;
			}
		} 
		if (hr == nullptr || !hr->enable) {
			DETOUR_MEMBER_CALL(CBaseEntity_PerformCustomPhysics)(pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity);
			return;
		}
		
		constexpr float physics_interval = 0.25f;

		
		float time = (float)(ent->m_flSimulationTime) - (float)(ent->m_flAnimTime);

		if (time < hr->aim_time && gpGlobals->tickcount % (int)(physics_interval / gpGlobals->interval_per_tick) == 0) {
			Vector target_vec = vec3_origin;

			if (hr->follow_crosshair) {
				CBaseEntity *owner = rocket->GetOwnerEntity();
				if (owner != nullptr) {
					Vector forward;
					AngleVectors(owner->EyeAngles(), &forward);

					trace_t result;
					UTIL_TraceLine(owner->EyePosition(), owner->EyePosition() + 4000.0f * forward, MASK_SHOT, owner, COLLISION_GROUP_NONE, &result);

					target_vec = result.endpos;
				}
			}
			else {

				CTFPlayer *target_player = nullptr;
				float target_distsqr     = FLT_MAX;
				
				ForEachTFPlayer([&](CTFPlayer *player){
					if (!player->IsAlive())                                 return;
					if (player->GetTeamNumber() == TEAM_SPECTATOR)          return;
					if (player->GetTeamNumber() == rocket->GetTeamNumber()) return;
					
					if (hr->ignore_disguised_spies) {
						if (player->m_Shared->InCond(TF_COND_DISGUISED) && player->m_Shared->GetDisguiseTeam() == rocket->GetTeamNumber()) {
							return;
						}
					}
					
					if (hr->ignore_stealthed_spies) {
						if (player->m_Shared->IsStealthed() && player->m_Shared->GetPercentInvisible() >= 0.75f &&
							!player->m_Shared->InCond(TF_COND_STEALTHED_BLINK) && !player->m_Shared->InCond(TF_COND_BURNING) && !player->m_Shared->InCond(TF_COND_URINE) && !player->m_Shared->InCond(TF_COND_BLEEDING)) {
							return;
						}
					}
					
					Vector delta = player->WorldSpaceCenter() - rocket->WorldSpaceCenter();
					if (DotProduct(delta.Normalized(), pNewVelocity->Normalized()) < hr->min_dot_product) return;
					
					float distsqr = rocket->WorldSpaceCenter().DistToSqr(player->WorldSpaceCenter());
					if (distsqr < target_distsqr) {
						trace_t tr;
						UTIL_TraceLine(player->WorldSpaceCenter(), rocket->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, player, COLLISION_GROUP_NONE, &tr);
						
						if (!tr.DidHit() || tr.m_pEnt == rocket) {
							target_player  = player;
							target_distsqr = distsqr;
						}
					}
				});
				
				if (target_player != nullptr) {
					target_vec = target_player->WorldSpaceCenter();
				}
			}
			
			if (target_vec != vec3_origin) {
				QAngle angToTarget;
				VectorAngles(target_vec - rocket->WorldSpaceCenter(), angToTarget);
				
				pNewAngVelocity->x = Clamp(Approach(AngleDiff(angToTarget.x, pNewAngles->x) * 4.0f, pNewAngVelocity->x, hr->turn_power), -360.0f, 360.0f);
				pNewAngVelocity->y = Clamp(Approach(AngleDiff(angToTarget.y, pNewAngles->y) * 4.0f, pNewAngVelocity->y, hr->turn_power), -360.0f, 360.0f);
				pNewAngVelocity->z = Clamp(Approach(AngleDiff(angToTarget.z, pNewAngles->z) * 4.0f, pNewAngVelocity->z, hr->turn_power), -360.0f, 360.0f);
			}
		}
		
		if (time < hr->aim_time)
			*pNewAngles += (*pNewAngVelocity * gpGlobals->frametime);
		
		Vector vecOrientation;
		AngleVectors(*pNewAngles, &vecOrientation);
		*pNewVelocity = vecOrientation * (1100.0f * hr->speed + hr->acceleration * Clamp(time - hr->acceleration_start, 0.0f, hr->acceleration_time)) + Vector(0,0,-hr->gravity * gpGlobals->interval_per_tick);
		
		*pNewPosition += (*pNewVelocity * gpGlobals->frametime);
		
	//	if (gpGlobals->tickcount % 2 == 0) {
	//		NDebugOverlay::EntityText(ENTINDEX(rocket), -2, CFmtStr("  AngVel: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewAngVelocity)), 0.030f);
	//		NDebugOverlay::EntityText(ENTINDEX(rocket), -1, CFmtStr("  Angles: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewAngles)),      0.030f);
	//		NDebugOverlay::EntityText(ENTINDEX(rocket),  1, CFmtStr("Velocity: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewVelocity)),    0.030f);
	//		NDebugOverlay::EntityText(ENTINDEX(rocket),  2, CFmtStr("Position: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewPosition)),    0.030f);
	//	}
		
	//	DevMsg("[%d] PerformCustomPhysics: #%d %s\n", gpGlobals->tickcount, ENTINDEX(ent), ent->GetClassname());
	}

	
	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFWeaponBaseGun_FireProjectile, CTFPlayer *player)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			
			auto data = GetDataForBot(player);
			if (data != nullptr) {
				bool stopproj = false;
				auto weapon = reinterpret_cast<CTFWeaponBaseGun*>(this);
				for(auto it = data->shoot_templ.begin(); it != data->shoot_templ.end(); it++) {
					ShootTemplateData &temp_data = *it;
					
					if (temp_data.weapon != "" && !FStrEq(weapon->GetItem()->GetStaticData()->GetName(), temp_data.weapon.c_str()))
						continue;

					if (temp_data.parent_to_projectile) {
						CBaseAnimating *proj = DETOUR_MEMBER_CALL(CTFWeaponBaseGun_FireProjectile)(player);
						if (proj != nullptr) {
							Vector vec = temp_data.offset;
							QAngle ang = temp_data.angles;
							auto inst = temp_data.templ->SpawnTemplate(proj, vec, ang, true, nullptr);
						}
						return proj;
					}
					
					stopproj = temp_data.Shoot(player, weapon) | stopproj;
				}
				if (stopproj) {
					if (weapon->ShouldPlayFireAnim()) {
						player->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
					}
					
					weapon->RemoveProjectileAmmo(player);
					weapon->m_flLastFireTime = gpGlobals->curtime;
					weapon->DoFireEffects();
					weapon->UpdatePunchAngles(player);
					
					if (player->m_Shared->IsStealthed() && weapon->ShouldRemoveInvisibilityOnPrimaryAttack()) {
						player->RemoveInvisibility();
					}
					return nullptr;
				}
			}
		}
		return DETOUR_MEMBER_CALL(CTFWeaponBaseGun_FireProjectile)(player);
	}
	
	DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_WeaponSound, int index, float soundtime) 
	{
		if ((index == 1 || index == 5) && TFGameRules()->IsMannVsMachineMode()) {
			auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
			CTFBot *owner = ToTFBot(weapon->GetOwner());
			if (owner != nullptr) {
				auto data = GetDataForBot(owner);
				if (data != nullptr && !data->fire_sound.empty()) {
					owner->EmitSound(data->fire_sound.c_str(), soundtime);
					return;
				}
			}
		}
		DETOUR_MEMBER_CALL(CBaseCombatWeapon_WeaponSound)(index, soundtime);
	}

	struct NextBotData
    {
        int vtable;
        INextBotComponent *m_ComponentList;              // +0x04
        PathFollower *m_CurrentPath;                     // +0x08
        int m_iManagerIndex;                             // +0x0c
        bool m_bScheduledForNextTick;                    // +0x10
        int m_iLastUpdateTick;                           // +0x14
    };

	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{	
		auto data = GetDataForBot(actor);
		if (data != nullptr && data->fast_update) {
			reinterpret_cast<NextBotData *>(actor->MyNextBotPointer())->m_bScheduledForNextTick = true;
		}
		return DETOUR_MEMBER_CALL(CTFBotMainAction_Update)(actor, dt);
	}

	DETOUR_DECL_MEMBER(void, CTFBot_AvoidPlayers, void *cmd)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		auto data = GetDataForBot(bot);
		if (data != nullptr && data->no_pushaway) {
			return;
		}
		DETOUR_MEMBER_CALL(CTFBot_AvoidPlayers)(cmd);
	}

	DETOUR_DECL_MEMBER(float, CTFPlayer_GetHandScaleSpeed)
	{
		auto data = GetDataForBot(reinterpret_cast<CTFPlayer *>(this));
		if (data != nullptr) {
			return DETOUR_MEMBER_CALL(CTFPlayer_GetHandScaleSpeed)() * data->scale_speed;
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_GetHandScaleSpeed)();
	}
	
	DETOUR_DECL_MEMBER(float, CTFPlayer_GetHeadScaleSpeed)
	{
		auto data = GetDataForBot(reinterpret_cast<CTFPlayer *>(this));
		if (data != nullptr) {
			return DETOUR_MEMBER_CALL(CTFPlayer_GetHeadScaleSpeed)() * data->scale_speed;
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_GetHeadScaleSpeed)();
	}
	
	DETOUR_DECL_MEMBER(float, CTFPlayer_GetTorsoScaleSpeed)
	{
		auto data = GetDataForBot(reinterpret_cast<CTFPlayer *>(this));
		if (data != nullptr) {
			return DETOUR_MEMBER_CALL(CTFPlayer_GetTorsoScaleSpeed)() * data->scale_speed;
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_GetTorsoScaleSpeed)();
	}

	RefCount rc_CTFBotLocomotion_Update;
	DETOUR_DECL_MEMBER(void, CTFBotLocomotion_Update)
	{
		auto data = GetDataForBot(reinterpret_cast<ILocomotion *>(this)->GetBot()->GetEntity());
		SCOPED_INCREMENT_IF(rc_CTFBotLocomotion_Update, data != nullptr && data->no_crouch_button_release);
		DETOUR_MEMBER_CALL(CTFBotLocomotion_Update)();
	}
	
	DETOUR_DECL_MEMBER(void, NextBotPlayer_CTFPlayer_PressCrouchButton, float time)
	{
		if (rc_CTFBotLocomotion_Update)
			return;
		DETOUR_MEMBER_CALL(NextBotPlayer_CTFPlayer_PressCrouchButton)(time);
	}

	DETOUR_DECL_MEMBER(void, NextBotPlayer_CTFPlayer_ReleaseCrouchButton)
	{
		if (rc_CTFBotLocomotion_Update)
			return;
		DETOUR_MEMBER_CALL(NextBotPlayer_CTFPlayer_ReleaseCrouchButton)();
	}

	RefCount rc_CTFGameRules_PlayerKilled;
	CBasePlayer *killed = nullptr;
	DETOUR_DECL_MEMBER(void, CTFGameRules_PlayerKilled, CBasePlayer *pVictim, const CTakeDamageInfo& info)
	{
	//	DevMsg("CTFGameRules::PlayerKilled\n");
		killed = pVictim;
		SCOPED_INCREMENT(rc_CTFGameRules_PlayerKilled);
		DETOUR_MEMBER_CALL(CTFGameRules_PlayerKilled)(pVictim, info);
		if (TFGameRules()->IsMannVsMachineMode()) {
			auto *data = GetDataForBot(killed);
			if (data != nullptr && (data->spell_drop)) {
				float rnd  = RandomFloat(0.0f, 1.0f);
				float rate_rare = data->spell_drop_rate_rare;
				float rate_common = rate_rare + data->spell_drop_rate_common;
				
				if (rnd < rate_rare) {
					TFGameRules()->DropSpellPickup(pVictim->GetAbsOrigin(), 0);
				}
				else if (rnd < rate_common) {
					TFGameRules()->DropSpellPickup(pVictim->GetAbsOrigin(), 1);
				}
			}
			if (data != nullptr && data->custom_eye_particle != "") {
				StopParticleEffects(killed);
			}
		}
	}
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_ShouldDropSpellPickup)
	{
	//	DevMsg("CTFGameRules::ShouldDropSpellPickup\n");
		
		if (TFGameRules()->IsMannVsMachineMode() && rc_CTFGameRules_PlayerKilled > 0) {
			auto *data = GetDataForBot(killed);
			if (data != nullptr && (data->spell_drop)) {
				return false;
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFGameRules_ShouldDropSpellPickup)();
	}
	
	DETOUR_DECL_MEMBER(void, CBasePlayer_PlayStepSound, Vector& vecOrigin, surfacedata_t *psurface, float fvol, bool force)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (TFGameRules()->IsMannVsMachineMode()) {
			auto *data = GetDataForBot(player);
			if (data != nullptr && data->has_override_step_sound) {
				player->EmitSound(data->override_step_sound.c_str());
			}
		}
		DETOUR_MEMBER_CALL(CBasePlayer_PlayStepSound)(vecOrigin, psurface, fvol, force);
	}

	RefCount rc_CTFPlayer_OnTakeDamage_Alive;
	bool was_bleed = false;
	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage_Alive, const CTakeDamageInfo &info)
	{
		
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		SCOPED_INCREMENT_IF(rc_CTFPlayer_OnTakeDamage_Alive, HasRobotBlood(player));
		auto result = DETOUR_MEMBER_CALL(CTFPlayer_OnTakeDamage_Alive)(info);
		if (was_bleed) {
			Vector vDamagePos = info.GetDamagePosition();

			if (vDamagePos == vec3_origin) {
				vDamagePos = player->WorldSpaceCenter();
			}

			Vector vecDir = vec3_origin;
			if (info.GetInflictor()) {
				vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector(0.0f, 0.0f, 10.0f) - player->WorldSpaceCenter();
				VectorNormalize(vecDir);
			}

			CPVSFilter filter(vDamagePos);
			TE_TFBlood( filter, 0.0, vDamagePos, -vecDir, player->entindex() );
			
		}
		was_bleed = false;

		return result;
	}

	RefCount rc_CTFPlayer_Event_Killed;
	DETOUR_DECL_MEMBER(void, CTFPlayer_Event_Killed, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		SCOPED_INCREMENT_IF(rc_CTFPlayer_Event_Killed, HasRobotBlood(player));
		DETOUR_MEMBER_CALL(CTFPlayer_Event_Killed)(info);
	}
	
	DETOUR_DECL_STATIC(void, DispatchParticleEffect, char const *name, Vector vec, QAngle ang, CBaseEntity *entity)
	{
		if (rc_CTFPlayer_OnTakeDamage_Alive && (strcmp(name, "bot_impact_light") == 0 || strcmp(name, "bot_impact_heavy") == 0)) {
			was_bleed = true;
			return;
		}
		DETOUR_STATIC_CALL(DispatchParticleEffect)(name, vec, ang, entity);
	}

	DETOUR_DECL_STATIC(void, TE_TFParticleEffect, IRecipientFilter& recipement, float value, char const* name, Vector vector, QAngle angles, CBaseEntity* entity, ParticleAttachment_t attach)
	{
		if (rc_CTFPlayer_Event_Killed && strcmp(name, "bot_death") == 0) {
			return;
		}
		DETOUR_STATIC_CALL(TE_TFParticleEffect)(recipement, value, name, vector, angles, entity, attach);
	}

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Pop:ECAttr_Extensions")
		{
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_dtor0, "CTFBot::~CTFBot [D0]");
			MOD_ADD_DETOUR_MEMBER(CTFBot_dtor2, "CTFBot::~CTFBot [D2]");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_StateEnter, "CTFPlayer::StateEnter");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_StateLeave, "CTFPlayer::StateLeave");

			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Parse, "CTFBotSpawner::Parse");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Spawn, "CTFBotSpawner::Spawn");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_OnEventChangeAttributes,        "CTFBot::OnEventChangeAttributes");
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_ParseEventChangeAttributes,        "CTFBotSpawner::ParseEventChangeAttributes");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse, "CPopulationManager::Parse");

			MOD_ADD_DETOUR_MEMBER(CTFBot_AddItem,     "CTFBot::AddItem");
			MOD_ADD_DETOUR_MEMBER(CTFItemDefinition_GetLoadoutSlot,     "CTFItemDefinition::GetLoadoutSlot");
			//MOD_ADD_DETOUR_STATIC(CreateEntityByName, "CreateEntityByName");
			MOD_ADD_DETOUR_MEMBER(CItemGeneration_GenerateRandomItem,        "CItemGeneration::GenerateRandomItem");
			MOD_ADD_DETOUR_STATIC(ParseDynamicAttributes,         "ParseDynamicAttributes");
			MOD_ADD_DETOUR_MEMBER(CEconItemSchema_GetItemDefinitionByName,        "CEconItemSchema::GetItemDefinitionByName");
			MOD_ADD_DETOUR_MEMBER(CTFBot_EquipRequiredWeapon,        "CTFBot::EquipRequiredWeapon");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_FireWeaponAtEnemy, "CTFBotMainAction::FireWeaponAtEnemy");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_SelectTargetPoint, "CTFBotMainAction::SelectTargetPoint");

			MOD_ADD_DETOUR_MEMBER(CTFBotBody_GetHeadAimTrackingInterval, "CTFBotBody::GetHeadAimTrackingInterval");
			MOD_ADD_DETOUR_MEMBER(PlayerBody_GetMaxHeadAngularVelocity, "PlayerBody::GetMaxHeadAngularVelocity");
			MOD_ADD_DETOUR_MEMBER(CTFBot_EquipBestWeaponForThreat, "CTFBot::EquipBestWeaponForThreat");
			MOD_ADD_DETOUR_MEMBER(CTFBotDeliverFlag_UpgradeOverTime,        "CTFBotDeliverFlag::UpgradeOverTime");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_SelectMoreDangerousThreatInternal, "CTFBotMainAction::SelectMoreDangerousThreatInternal");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DeathSound,        "CTFPlayer::DeathSound");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_PainSound,        "CTFPlayer::PainSound");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_WeaponSound,        "CBaseCombatWeapon::WeaponSound");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireProjectile,        "CTFWeaponBaseGun::FireProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_Spawn,       "CTFProjectile_Rocket::Spawn");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PerformCustomPhysics, "CBaseEntity::PerformCustomPhysics");

			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageModifyRules, "CTFGameRules::ApplyOnDamageModifyRules");
			
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireRocket, "CTFWeaponBaseGun::FireRocket");
			
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ApplyOnHitAttributes, "CTFWeaponBase::ApplyOnHitAttributes");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update, "CTFBotMainAction::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBot_AvoidPlayers, "CTFBot::AvoidPlayers");

			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetHandScaleSpeed, "CTFPlayer::GetHandScaleSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetHeadScaleSpeed, "CTFPlayer::GetHeadScaleSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetTorsoScaleSpeed, "CTFPlayer::GetTorsoScaleSpeed");

			MOD_ADD_DETOUR_MEMBER(CTFBotLocomotion_Update, "CTFBotLocomotion::Update");
			MOD_ADD_DETOUR_MEMBER(NextBotPlayer_CTFPlayer_PressCrouchButton, "NextBotPlayer<CTFPlayer>::PressCrouchButton");
			MOD_ADD_DETOUR_MEMBER(NextBotPlayer_CTFPlayer_ReleaseCrouchButton, "NextBotPlayer<CTFPlayer>::ReleaseCrouchButton");
			
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_PlayerKilled,                     "CTFGameRules::PlayerKilled");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFGameRules_ShouldDropSpellPickup,            "CTFGameRules::ShouldDropSpellPickup", HIGH);
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CBasePlayer_PlayStepSound,                     "CBasePlayer::PlayStepSound",     HIGH);

			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage_Alive, "CTFPlayer::OnTakeDamage_Alive");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Event_Killed,                        "CTFPlayer::Event_Killed");
			MOD_ADD_DETOUR_STATIC(DispatchParticleEffect, "DispatchParticleEffect [overload 3]");
			MOD_ADD_DETOUR_STATIC(TE_TFParticleEffect, "TE_TFParticleEffect");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetSceneSoundToken, "CTFPlayer::GetSceneSoundToken");
		}

		virtual bool OnLoad() override
		{
			custom_item_equip_itemname = forwards->CreateForward("SIG_OnGiveCustomItem", ET_Hook, 2, NULL, Param_Cell, Param_String);
			custom_item_get_def_id = forwards->CreateForward("SIG_GetCustomItemID", ET_Hook, 3, NULL, Param_String, Param_Cell, Param_CellByRef);
			custom_item_set_attribute = forwards->CreateForward("SIG_SetCustomAttribute", ET_Hook, 3, NULL, Param_Cell, Param_String, Param_String);
			return true; 
		}
		
		virtual void OnUnload() override
		{
			
			forwards->ReleaseForward(custom_item_equip_itemname);
			forwards->ReleaseForward(custom_item_get_def_id);
			forwards->ReleaseForward(custom_item_set_attribute);
			ClearAllData();
		}
		
		virtual void OnDisable() override
		{
			ClearAllData();
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void FrameUpdatePostEntityThink() override
		{
			UpdateDelayedAddConds(delayed_addconds);
			UpdatePeriodicTasks(pending_periodic_tasks);
			UpdateRingOfFire();
			UpdateAlwaysGlow();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_ecattr_extensions", "0", FCVAR_NOTIFY,
		"Mod: enable extended KV in EventChangeAttributes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	class CKVCond_ECAttr : public IKVCond
	{
	public:
		virtual bool operator()() override
		{
			return s_Mod.IsEnabled();
		}
	};
	CKVCond_ECAttr cond;
}
#endif
