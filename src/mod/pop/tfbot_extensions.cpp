#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/populators.h"
#include "stub/projectiles.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "util/scope.h"
#include "mod/pop/kv_conditional.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot_behavior.h"
#include "util/iterate.h"
#include "mod/pop/pointtemplate.h"
#include "stub/usermessages_sv.h"
#include "stub/misc.h"
#include "stub/trace.h"
#include "util/clientmsg.h"
#include <ctime>

static StaticFuncThunk<bool, CTFBot *, CTFPlayer *, int> ft_TeleportNearVictim  ("TeleportNearVictim");

bool TeleportNearVictim(CTFBot *spy, CTFPlayer *victim, int dist) {return ft_TeleportNearVictim(spy,victim,dist);};
class PlayerBody : public IBody {
	public:
		CBaseEntity * GetEntity() {return vt_GetEntity(this);}
	private:
		static MemberVFuncThunk<PlayerBody *, CBaseEntity*>   vt_GetEntity;
};
MemberVFuncThunk<PlayerBody *, CBaseEntity*                                                          > PlayerBody::vt_GetEntity                  (TypeName<PlayerBody>(), "PlayerBody::GetEntity");

namespace Mod::Pop::TFBot_Extensions
{

	std::unordered_map<CTFBot*, CBaseEntity*> targets_sentrybuster;

	/* mobber AI, based on CTFBotAttackFlagDefenders */
	class CTFBotMobber : public IHotplugAction
	{
	public:
		CTFBotMobber()
		{
			this->m_Attack = CTFBotAttack::New();
		}
		virtual ~CTFBotMobber()
		{
			if (this->m_Attack != nullptr) {
				delete this->m_Attack;
			}
		}
		
		virtual const char *GetName() const override { return "Mobber"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
			
			this->m_hTarget = nullptr;
			
			return this->m_Attack->OnStart(actor, action);
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
			if (threat != nullptr) {
				actor->EquipBestWeaponForThreat(threat);
			}
			
			ActionResult<CTFBot> result = this->m_Attack->Update(actor, dt);
			if (result.transition != ActionTransition::DONE) {
				return ActionResult<CTFBot>::Continue();
			}
			
			/* added teamnum check to fix some TF_COND_REPROGRAMMED quirks */
			if (this->m_hTarget == nullptr || !this->m_hTarget->IsAlive() || this->m_hTarget->GetTeamNumber() == actor->GetTeamNumber()) {
				
				this->m_hTarget = actor->SelectRandomReachableEnemy();
				
				if (this->m_hTarget == nullptr) {
					CBaseEntity *target_ent = nullptr;
					do {
						target_ent = servertools->FindEntityByClassname(target_ent, "tank_boss");

						if (target_ent != nullptr && target_ent->GetTeamNumber() != actor->GetTeamNumber()) {
							this->m_hTarget = rtti_cast<CBaseCombatCharacter *>(target_ent);
							break;
						}

					} while (target_ent != nullptr);
					
					target_ent = nullptr;
					do {
						target_ent = servertools->FindEntityByClassname(target_ent, "headless_hatman");

						if (target_ent != nullptr && target_ent->GetTeamNumber() != actor->GetTeamNumber()) {
							this->m_hTarget = rtti_cast<CBaseCombatCharacter *>(target_ent);
							break;
						}

					} while (target_ent != nullptr);
				}
				
				if (this->m_hTarget == nullptr) {
					return ActionResult<CTFBot>::Continue();
				}
			}
			
			actor->GetVisionInterface()->AddKnownEntity(this->m_hTarget);
			
			auto nextbot = rtti_cast<INextBot *>(actor);
			
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
				
				CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
				this->m_PathFollower.Compute(nextbot, this->m_hTarget, cost_func, 0.0f, true);
			}
			
			this->m_PathFollower.Update(nextbot);
			
			return ActionResult<CTFBot>::Continue();
		}
		
	private:
		CTFBotAttack *m_Attack = nullptr;
		
		CHandle<CBaseCombatCharacter> m_hTarget;
		
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
	};
	
	
	struct AddCond
	{
		ETFCond cond   = (ETFCond)-1;
		float duration = -1.0f;
		float delay    =  0.0f;
		int health_below = 0;
		int health_above = 0;
	};

	enum PeriodicTaskType {
		TASK_TAUNT,
		TASK_GIVE_SPELL,
		TASK_VOICE_COMMAND,
		TASK_FIRE_WEAPON,
		TASK_CHANGE_ATTRIBUTES,
		TASK_SPAWN_TEMPLATE,
		TASK_FIRE_INPUT
	};

	class PeriodicTaskImpl {
	public:
		float when = 10;
		float cooldown = 10;
		int repeats = -1;
		PeriodicTaskType type;
		int spell_type = 0;
		int spell_count = 1;
		int max_spells = 0;
		float duration = 0.1f;
		bool if_target = false;
		int health_below = 0;
		int health_above = 0;
		std::string attrib_name;
		std::string input_name;
		std::string param;
	};

	struct HomingRockets
	{
		bool enable                 = false;
		bool ignore_disguised_spies = true;
		bool ignore_stealthed_spies = true;
		float speed                 = 1.0f;
		float turn_power            = 10.0f;
		float min_dot_product       = -0.25f;
	};
	
	enum AimAt
	{
		AIM_DEFAULT,
		AIM_HEAD,
		AIM_BODY,
		AIM_FEET
	};

	enum ActionType
	{
		ACTION_Default,
		
		// built-in
		ACTION_FetchFlag,
		ACTION_PushToCapturePoint,
		ACTION_BotSpyInfiltrate,
		ACTION_MedicHeal,
		ACTION_SniperLurk,
		ACTION_DestroySentries,
		
		// custom
		ACTION_Mobber,
	};
	struct EventChangeAttributesData
	{
		std::map<std::string,std::map<std::string, float>> custom_attrs;
	};

	struct ShootTemplateData
	{
		PointTemplate *templ = nullptr;
		float speed = 1000.0f;
		float spread = 0.0f;
		bool override_shoot = false;
		Vector offset = Vector(0,0,0);
		QAngle angles = QAngle(0,0,0);
		bool parent_to_projectile = false;
		std::string weapon = "";
	};

	struct SpawnerData
	{
		std::vector<AddCond> addconds;
		std::vector<AddCond> dmgappliesconds;

		std::vector<PeriodicTaskImpl> periodic_tasks;

		std::map<int, float> weapon_resists;
		
		std::map<std::string, color32> item_colors;
		std::map<std::string, std::string> item_models;
		
		bool use_human_model  = false;
		bool use_buster_model = false;
		bool force_romevision_cosmetics = false;

		std::string use_custom_model;

		std::string death_sound = "DEF";
		std::string pain_sound = "DEF";
		string_t fire_sound = MAKE_STRING("");

		std::map<int, std::string> custom_weapon_models;
		
		std::string rocket_custom_model;
		std::string rocket_custom_particle;
		
		float ring_of_fire = -1.0f;

		float scale_speed = 1.0f;
		
		HomingRockets homing_rockets;
		
		ActionType action = ACTION_Default;
		
		bool use_melee_threat_prioritization = false;
		
		bool suppress_timed_fetchflag = false;

		bool no_bomb_upgrade = false;

		bool always_glow = false;
		
		bool use_best_weapon = false;

		float tracking_interval = -1.0f;

		std::vector<PointTemplateInfo> templ;
		std::vector<ShootTemplateData> shoot_templ;

		std::map<void *, EventChangeAttributesData> event_change_atttributes_data;

		std::vector<int> strip_item_slot;
		AimAt aim_at = AIM_DEFAULT;
		Vector aim_offset = vec3_origin;
		float aim_lead_target_speed = 0.0f;

		int rocket_jump_type = 0;

#ifdef ENABLE_BROKEN_STUFF
		bool drop_weapon = false;
#endif
	};
	
	std::unordered_map<std::string,CTFItemDefinition*> item_defs;
	std::unordered_map<std::string,std::string> item_custom_remap;

	std::map<CTFBotSpawner *, SpawnerData> spawners;
	
	std::map<CHandle<CTFBot>, CTFBotSpawner *> spawner_of_bot;

	std::unordered_map<CTFBot *, SpawnerData *> bots_data;
	
	struct DelayedAddCond
	{
		CHandle<CTFBot> bot;
		float when;
		ETFCond cond;
		float duration;
		int health_below = 0;
		int health_above = 0;
		
	};
	std::vector<DelayedAddCond> delayed_addconds;


	
	struct PeriodicTask
	{
		CHandle<CTFBot> bot;
		PeriodicTaskType type;
		float when = 10;
		float cooldown = 10;
		int repeats = 0;
		float duration = 0.1;
		bool if_target = false;
		std::string attrib_name;
		std::string input_name;
		std::string param;

		int spell_type=0;
		int spell_count=1;
		int max_spells=0;

		int health_below = 0;
		int health_above = 0;
	};
	std::vector<PeriodicTask> pending_periodic_tasks;

	int SPELL_TYPE_COUNT=12;
	int SPELL_TYPE_COUNT_ALL=15;
	int INPUT_TYPE_COUNT=7;
	const char *INPUT_TYPE[] = {
		"Primary",
		"Secondary",
		"Special",
		"Reload",
		"Jump",
		"Crouch",
		"Action"
	};

	const char *SPELL_TYPE[] = {
		"Fireball",
		"Ball O' Bats",
		"Healing Aura",
		"Pumpkin MIRV",
		"Superjump",
		"Invisibility",
		"Teleport",
		"Tesla Bolt",
		"Minify",
		"Meteor Shower",
		"Summon Monoculus",
		"Summon Skeletons",
		"Common",
		"Rare",
		"All"
	};
	const char *ROMEVISON_MODELS[] = {
		"",
		"",
		"models/workshop/player/items/scout/tw_scoutbot_armor/tw_scoutbot_armor.mdl",
		"models/workshop/player/items/scout/tw_scoutbot_hat/tw_scoutbot_hat.mdl",
		"models/workshop/player/items/sniper/tw_sniperbot_armor/tw_sniperbot_armor.mdl",
		"models/workshop/player/items/sniper/tw_sniperbot_helmet/tw_sniperbot_helmet.mdl",
		"models/workshop/player/items/soldier/tw_soldierbot_armor/tw_soldierbot_armor.mdl",
		"models/workshop/player/items/soldier/tw_soldierbot_helmet/tw_soldierbot_helmet.mdl",
		"models/workshop/player/items/demo/tw_demobot_armor/tw_demobot_armor.mdl",
		"models/workshop/player/items/demo/tw_demobot_helmet/tw_demobot_helmet.mdl",
		"models/workshop/player/items/medic/tw_medibot_chariot/tw_medibot_chariot.mdl",
		"models/workshop/player/items/medic/tw_medibot_hat/tw_medibot_hat.mdl",
		"models/workshop/player/items/heavy/tw_heavybot_armor/tw_heavybot_armor.mdl",
		"models/workshop/player/items/heavy/tw_heavybot_helmet/tw_heavybot_helmet.mdl",
		"models/workshop/player/items/pyro/tw_pyrobot_armor/tw_pyrobot_armor.mdl",
		"models/workshop/player/items/pyro/tw_pyrobot_helmet/tw_pyrobot_helmet.mdl",
		"models/workshop/player/items/spy/tw_spybot_armor/tw_spybot_armor.mdl",
		"models/workshop/player/items/spy/tw_spybot_hood/tw_spybot_hood.mdl",
		"models/workshop/player/items/engineer/tw_engineerbot_armor/tw_engineerbot_armor.mdl",
		"models/workshop/player/items/engineer/tw_engineerbot_helmet/tw_engineerbot_helmet.mdl",
		"models/workshop/player/items/demo/tw_sentrybuster/tw_sentrybuster.mdl"
	};
	int GetResponseFor(const char *text) {
		if (FStrEq(text,"Medic"))
			return 0;
		else if (FStrEq(text,"Help"))
			return 20;
		else if (FStrEq(text,"Go"))
			return 2;
		else if (FStrEq(text,"Move up"))
			return 3;
		else if (FStrEq(text,"Left"))
			return 4;
		else if (FStrEq(text,"Right"))
			return 5;
		else if (FStrEq(text,"Yes"))
			return 6;
		else if (FStrEq(text,"No"))
			return 7;
		//else if (FStrEq(text,"Taunt"))
		//	return MP_CONCEPT_PLAYER_TAUNT;
		else if (FStrEq(text,"Incoming"))
			return 10;
		else if (FStrEq(text,"Spy"))
			return 11;
		else if (FStrEq(text,"Thanks"))
			return 1;
		else if (FStrEq(text,"Jeers"))
			return 23;
		else if (FStrEq(text,"Battle cry"))
			return 21;
		else if (FStrEq(text,"Cheers"))
			return 22;
		else if (FStrEq(text,"Charge ready"))
			return 17;
		else if (FStrEq(text,"Activate charge"))
			return 16;
		else if (FStrEq(text,"Sentry here"))
			return 15;
		else if (FStrEq(text,"Dispenser here"))
			return 14;
		else if (FStrEq(text,"Teleporter here"))
			return 13;
		else if (FStrEq(text,"Good job"))
			return 27;
		else if (FStrEq(text,"Sentry ahead"))
			return 12;
		else if (FStrEq(text,"Positive"))
			return 24;
		else if (FStrEq(text,"Negative"))
			return 25;
		else if (FStrEq(text,"Nice shot"))
			return 26;
		return -1;
	}


	void UpdateDelayedAddConds()
	{
		for (auto it = delayed_addconds.begin(); it != delayed_addconds.end(); ) {
			const auto& info = *it;
			
			if (info.bot == nullptr || !info.bot->IsAlive()) {
				it = delayed_addconds.erase(it);
				continue;
			}
			
			if (gpGlobals->curtime >= info.when && (info.health_below == 0 || info.bot->GetHealth() <= info.health_below)) {
				info.bot->m_Shared->AddCond(info.cond, info.duration);
				
				it = delayed_addconds.erase(it);
				continue;
			}
			
			++it;
		}
	}
	
	void UpdatePeriodicTasks()
	{
		for (auto it = pending_periodic_tasks.begin(); it != pending_periodic_tasks.end(); ) {
			auto& pending_task = *it;

			if (pending_task.bot == nullptr || !pending_task.bot->IsAlive()) {
				it = pending_periodic_tasks.erase(it);
				continue;
			}
			CTFBot *bot = pending_task.bot;
			if (gpGlobals->curtime >= pending_task.when && (pending_task.health_below == 0 || bot->GetHealth() <= pending_task.health_below)) {
				const CKnownEntity *threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(false);
				if ((pending_task.health_above > 0 && bot->GetHealth() <= pending_task.health_above) || (
						pending_task.if_target && (threat == nullptr || threat->GetEntity() == nullptr || !threat->IsVisibleRecently()))) {
					if (pending_task.health_below > 0)
						pending_task.when = gpGlobals->curtime;

					pending_task.when+=pending_task.cooldown;
					continue;
				}
				if (pending_task.type==TASK_TAUNT) {
					const char * commandn = "taunt";
					CCommand command = CCommand();
					command.Tokenize(commandn);
					ToTFPlayer(bot)->ClientCommand(command);
				}
				else if (pending_task.type==TASK_GIVE_SPELL) {
					CTFPlayer *ply = ToTFPlayer(bot);
					for (int i = 0; i < MAX_WEAPONS; ++i) {
						CBaseCombatWeapon *weapon = ply->GetWeapon(i);
						if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) continue;
						
						CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
						if (pending_task.spell_type < SPELL_TYPE_COUNT){
							spellbook->m_iSelectedSpellIndex=pending_task.spell_type;
						}
						else{
							if (pending_task.spell_type == 12) //common spell
								spellbook->m_iSelectedSpellIndex=RandomInt(0,6);
							else if (pending_task.spell_type == 13) //rare spell
								spellbook->m_iSelectedSpellIndex=RandomInt(7,11);
							else if (pending_task.spell_type == 14) //all spells
								spellbook->m_iSelectedSpellIndex=RandomInt(0,11);
						}
						spellbook->m_iSpellCharges+=pending_task.spell_count;
						if (spellbook->m_iSpellCharges > pending_task.max_spells)
							spellbook->m_iSpellCharges = pending_task.max_spells;
						
							
						DevMsg("Weapon %d %s\n",i , weapon -> GetClassname());
						break;
					}
					DevMsg("Spell task executed %d\n", pending_task.spell_type);
				}
				else if (pending_task.type==TASK_VOICE_COMMAND) {
					TFGameRules()->VoiceCommand(reinterpret_cast<CBaseMultiplayerPlayer*>(bot), pending_task.spell_type / 10, pending_task.spell_type % 10);
					//ToTFPlayer(bot)->SpeakConceptIfAllowed(pending_task.spell_type);
					/*const char * commandn3= "voicemenu 0 0";
					CCommand command3 = CCommand();
					command3.Tokenize(commandn3);
					DevMsg("Command test 1 %d\n", command3.ArgC());
					for (int i = 0; i < command3.ArgC(); i++){
						DevMsg("%d. Argument %s\n",i, command3[i]);
					}

					DevMsg("\n");
					ToTFPlayer(bot)->ClientCommand(command);*/
					DevMsg("Voice command executed %d\n", pending_task.spell_type);
				}
				else if (pending_task.type==TASK_FIRE_WEAPON) {

					switch (pending_task.spell_type) {
					case 0:
						bot->ReleaseFireButton();
						break;
					case 1:
						bot->ReleaseAltFireButton();
						break;
					case 2:
						bot->ReleaseSpecialFireButton();
						break;
					case 3:
						bot->ReleaseReloadButton();
						break;
					case 4:
						bot->ReleaseJumpButton();
						break;
					case 5:
						bot->ReleaseCrouchButton();
						break;
					case 6:
						bot->UseActionSlotItemReleased();
						break;
					}
					if (pending_task.duration >= 0){

						CTFBot::AttributeType attrs = bot->m_nBotAttrs;

						bot->m_nBotAttrs = CTFBot::ATTR_NONE;

						switch (pending_task.spell_type) {
						case 0:
							bot->PressFireButton(pending_task.duration);
							break;
						case 1:
							bot->PressAltFireButton(pending_task.duration);
							break;
						case 2:
							bot->PressSpecialFireButton(pending_task.duration);
							break;
						case 3:
							bot->PressReloadButton(pending_task.duration);
							break;
						case 4:
							bot->PressJumpButton(pending_task.duration);
							break;
						case 5:
							bot->PressCrouchButton(pending_task.duration);
							break;
						}

						bot->m_nBotAttrs = attrs;
					}
					if (pending_task.spell_type == 6) {
						bot->UseActionSlotItemPressed();
					}
					
					DevMsg("FIRE_WEAPON_ %d\n", pending_task.spell_type);
				}
				else if (pending_task.type==TASK_CHANGE_ATTRIBUTES) {
					
					const CTFBot::EventChangeAttributes_t *attrib = bot->GetEventChangeAttributes(pending_task.attrib_name.c_str());
					if (attrib != nullptr){
						DevMsg("Attribute exists %s\n", pending_task.attrib_name.c_str());
						bot->OnEventChangeAttributes(attrib);
					}
					DevMsg("Attribute changed %s\n", pending_task.attrib_name.c_str());
				}
				else if (pending_task.type==TASK_FIRE_INPUT) {
					variant_t variant1;
					string_t m_iParameter = AllocPooledString(pending_task.param.c_str());
						variant1.SetString(m_iParameter);
					CEventQueue &que = g_EventQueue;
					que.AddEvent(STRING(AllocPooledString(pending_task.attrib_name.c_str())),STRING(AllocPooledString(pending_task.input_name.c_str())),variant1,0,bot,bot,-1);
					std::string targetname = STRING(bot->GetEntityName());
					int findamp = targetname.find('&');
					if (findamp != -1){
						que.AddEvent(STRING(AllocPooledString((pending_task.attrib_name+targetname.substr(findamp)).c_str())),STRING(AllocPooledString(pending_task.input_name.c_str())),variant1,0,bot,bot,-1);
					}

					//trigger->AcceptInput("trigger", this->parent, this->parent ,variant1,-1);
				}
				//info.Execute(pending_task.bot);
				DevMsg("Periodic task executed %f, %f\n", pending_task.when,gpGlobals->curtime);
				if (--pending_task.repeats == 0) {
					it =  pending_periodic_tasks.erase(it);
					continue;
				}
				else{
					pending_task.when+=pending_task.cooldown;

				}
			}
			
			++it;
		}
	}

	void ClearAllData()
	{
		spawners.clear();
		spawner_of_bot.clear();
		bots_data.clear();
		delayed_addconds.clear();
		pending_periodic_tasks.clear();
		targets_sentrybuster.clear();
		item_defs.clear();
		item_custom_remap.clear();
	}
	
	
	void RemoveSpawner(CTFBotSpawner *spawner)
	{
		for (auto it = spawner_of_bot.begin(); it != spawner_of_bot.end(); ) {
			if ((*it).second == spawner) {
				it = spawner_of_bot.erase(it);
			} else {
				++it;
			}
		}
		auto data = spawners[spawner];
		spawners.erase(spawner);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBotSpawner_dtor0)
	{
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		
	//	DevMsg("CTFBotSpawner %08x: dtor0, clearing data\n", (uintptr_t)spawner);
		RemoveSpawner(spawner);
		
		DETOUR_MEMBER_CALL(CTFBotSpawner_dtor0)();
	}
	
	DETOUR_DECL_MEMBER(void, CTFBotSpawner_dtor2)
	{
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		
	//	DevMsg("CTFBotSpawner %08x: dtor2, clearing data\n", (uintptr_t)spawner);
		RemoveSpawner(spawner);
		
		DETOUR_MEMBER_CALL(CTFBotSpawner_dtor2)();
	}
	
	
	const char *GetStateName(int nState)
	{
		switch (nState) {
		case TF_STATE_ACTIVE:   return "ACTIVE";
		case TF_STATE_WELCOME:  return "WELCOME";
		case TF_STATE_OBSERVER: return "OBSERVER";
		case TF_STATE_DYING:    return "DYING";
		default:                return "<INVALID>";
		}
	}
	
	
	void ClearDataForBot(CTFBot *bot)
	{
		spawner_of_bot.erase(bot);
		bots_data.erase(bot);
		
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
	
	
	SpawnerData *GetDataForBot(CTFBot *bot)
	{
		if (bot == nullptr) return nullptr;
		
		return bots_data[bot];
		/*auto it1 = spawner_of_bot.find(bot);
		if (it1 == spawner_of_bot.end()) return nullptr;
		CTFBotSpawner *spawner = (*it1).second;

		auto it2 = spawners.find(spawner);
		if (it2 == spawners.end()) return nullptr;
		SpawnerData& data = (*it2).second;
		
		return &data;*/
	}
	SpawnerData *GetDataForBot(CBaseEntity *ent)
	{
		CTFBot *bot = ToTFBot(ent);
		return GetDataForBot(bot);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBot_dtor0)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
	//	DevMsg("CTFBot %08x: dtor0, clearing data\n", (uintptr_t)bot);
		ClearDataForBot(bot);
		
		DETOUR_MEMBER_CALL(CTFBot_dtor0)();
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_dtor2)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
	//	DevMsg("CTFBot %08x: dtor2, clearing data\n", (uintptr_t)bot);
		ClearDataForBot(bot);
		
		DETOUR_MEMBER_CALL(CTFBot_dtor2)();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_StateEnter, int nState)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (nState == TF_STATE_WELCOME || nState == TF_STATE_OBSERVER) {
			CTFBot *bot = ToTFBot(player);
			if (bot != nullptr) {
			//	DevMsg("Bot #%d [\"%s\"]: StateEnter %s, clearing data\n", ENTINDEX(bot), bot->GetPlayerName(), GetStateName(nState));
				ClearDataForBot(bot);
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
				ClearDataForBot(bot);
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayer_StateLeave)();
	}
	
	
	void Parse_AddCond(CTFBotSpawner *spawner, KeyValues *kv)
	{
		AddCond addcond;
		
		bool got_cond     = false;
		bool got_duration = false;
		bool got_delay    = false;
		
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
					Warning("Unrecognized condition name \"%s\" in AddCond block.\n", subkey->GetString());
				}
			} else if (FStrEq(name, "Duration")) {
				addcond.duration = subkey->GetFloat();
				got_duration = true;
			} else if (FStrEq(name, "Delay")) {
				addcond.delay = subkey->GetFloat();
				got_delay = true;
			} else if (FStrEq(name, "IfHealthBelow")) {
				addcond.health_below = subkey->GetInt();
			} else if (FStrEq(name, "IfHealthAbove")) {
				addcond.health_above = subkey->GetInt();
			}
			 else {
				Warning("Unknown key \'%s\' in AddCond block.\n", name);
			}
		}
		
		if (!got_cond) {
			Warning("Could not find a valid condition index/name in AddCond block.\n");
			return;
		}
		
		DevMsg("CTFBotSpawner %08x: add AddCond(%d, %f)\n", (uintptr_t)spawner, addcond.cond, addcond.duration);
		spawners[spawner].addconds.push_back(addcond);
	}
	
	void Parse_PeriodicTask(CTFBotSpawner *spawner, KeyValues *kv, PeriodicTaskType type)
	{
		PeriodicTaskImpl task;
		task.type = type;
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "Cooldown")) {
				task.cooldown = Max(0.015f,subkey->GetFloat());
			} else if (FStrEq(name, "Repeats")) {
				task.repeats = subkey->GetInt();
			} else if (FStrEq(name, "Delay")) {
				task.when = subkey->GetFloat();
			}
			else if (FStrEq(name, "Type")) {
				task.spell_type=subkey->GetInt();

				if (type == TASK_GIVE_SPELL){
					const char *typen =subkey->GetString();
					for (int i = 0; i < SPELL_TYPE_COUNT_ALL; i++) {
						if(FStrEq(typen,SPELL_TYPE[i])){
							task.spell_type = i;
						}	
					}
				}
				else if (type == TASK_VOICE_COMMAND){
					const char *typen =subkey->GetString();
					int resp = GetResponseFor(typen);
					if (resp >= 0)
						task.spell_type = resp;
				}
				else if (type == TASK_FIRE_WEAPON){
					const char *typen =subkey->GetString();
					for (int i = 0; i < INPUT_TYPE_COUNT; i++) {
						if(FStrEq(typen,INPUT_TYPE[i])){
							task.spell_type = i;
						}	
					}
				}
			}
			else if (FStrEq(name, "Limit")) {
				task.max_spells=subkey->GetInt();
			}
			else if (FStrEq(name, "Charges")) {
				task.spell_count=subkey->GetInt();
			}
			else if (FStrEq(name, "Duration")) {
				task.duration=subkey->GetFloat();
			}
			else if (FStrEq(name, "IfSeeTarget")) {
				task.if_target=subkey->GetBool();
			}
			else if (FStrEq(name, "Name") || FStrEq(name, "Target")) {
				task.attrib_name=subkey->GetString();
			}
			else if (FStrEq(name, "Action")) {
				task.input_name=subkey->GetString();
			}
			else if (FStrEq(name, "Param")) {
				task.param=subkey->GetString();
			}
			else if (FStrEq(name, "IfHealthBelow")) {
				task.health_below=subkey->GetInt();
			}
			else if (FStrEq(name, "IfHealthAbove")) {
				task.health_above=subkey->GetInt();
			}
			//else if (FStrEq(name, "SpawnTemplate")) {
			//	spawners[spawner].periodic_templ.push_back(Parse_SpawnTemplate(subkey));
			//	task.spell_type = spawners[spawner].periodic_templ.size()-1;
			//}
		}
		if (task.max_spells == 0)
			task.max_spells = task.spell_count;
		DevMsg("CTFBotSpawner %08x: add periodic(%f, %f)\n", (uintptr_t)spawner, task.cooldown, task.when);
		spawners[spawner].periodic_tasks.push_back(task);
	}

	void Parse_DamageAppliesCond(CTFBotSpawner *spawner, KeyValues *kv)
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
		
		DevMsg("CTFBotSpawner %08x: add DamageAppliesCond(%d, %f)\n", (uintptr_t)spawner, addcond.cond, addcond.duration);
		spawners[spawner].dmgappliesconds.push_back(addcond);
	}
	
	void Parse_Action(CTFBotSpawner *spawner, KeyValues *kv)
	{
		const char *value = kv->GetString();
		
		if (FStrEq(value, "Default")) {
			spawners[spawner].action = ACTION_Default;
		} else if (FStrEq(value, "FetchFlag")) {
			spawners[spawner].action = ACTION_FetchFlag;
		} else if (FStrEq(value, "PushToCapturePoint")) {
			spawners[spawner].action = ACTION_PushToCapturePoint;
		} else if (FStrEq(value, "Mobber")) {
			spawners[spawner].action = ACTION_Mobber;
		} else if (FStrEq(value, "Spy")) {
			spawners[spawner].action = ACTION_BotSpyInfiltrate;
		} else if (FStrEq(value, "Medic")) {
			spawners[spawner].action = ACTION_MedicHeal;
		} else if (FStrEq(value, "Sniper")) {
			spawners[spawner].action = ACTION_SniperLurk;
		} else if (FStrEq(value, "SuicideBomber")) {
			spawners[spawner].action = ACTION_DestroySentries;
		} else {
			Warning("Unknown value \'%s\' for TFBot Action.\n", value);
		}
	}
	
	void Parse_WeaponResist(CTFBotSpawner *spawner, KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			int weapon_id = GetWeaponId(name);
			if (weapon_id == TF_WEAPON_NONE) {
				Warning("Unknown weapon ID \'%s\' in WeaponResist block.\n", name);
				continue;
			}
			
			DevMsg("CTFBotSpawner %08x: resist %s (0x%02x): %4.02f\n", (uintptr_t)spawner, name, weapon_id, subkey->GetFloat());
			spawners[spawner].weapon_resists[weapon_id] = subkey->GetFloat();
		}
	}
	
	void Parse_ItemColor(CTFBotSpawner *spawner, KeyValues *kv)
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
		
		DevMsg("CTFBotSpawner %08x: add ItemColor(\"%s\", %02X%02X%02X)\n", (uintptr_t)spawner, item_name, color_r, color_g, color_b);
		spawners[spawner].item_colors[item_name] = { color_r, color_g, color_b, 0xff };
	}
	
	void Parse_HomingRockets(CTFBotSpawner *spawner, KeyValues *kv)
	{
		HomingRockets& hr = spawners[spawner].homing_rockets;
		hr.enable = true;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "IgnoreDisguisedSpies")) {
				hr.ignore_disguised_spies = subkey->GetBool();
			} else if (FStrEq(name, "IgnoreStealthedSpies")) {
				hr.ignore_stealthed_spies = subkey->GetBool();
			} else if (FStrEq(name, "RocketSpeed")) {
				hr.speed = subkey->GetFloat();
			} else if (FStrEq(name, "TurnPower")) {
				hr.turn_power = subkey->GetFloat();
			} else if (FStrEq(name, "MinDotProduct")) {
				hr.min_dot_product = Clamp(subkey->GetFloat(), -1.0f, 1.0f);
			} else if (FStrEq(name, "MaxAimError")) {
				hr.min_dot_product = std::cos(DEG2RAD(Clamp(subkey->GetFloat(), 0.0f, 180.0f)));
			} else if (FStrEq(name, "Enable")) {
				/* this used to be a parameter but it was redundant and has been removed;
				 * ignore it without issuing a warning */
			} else {
				Warning("Unknown key \'%s\' in HomingRockets block.\n", name);
			}
		}
		
		DevMsg("CTFBotSpawner %08x: set HomingRockets(%s, %s, %.2f, %.1f, %.2f)\n",
			(uintptr_t)spawner,
			(hr.ignore_disguised_spies ? "true" : "false"),
			(hr.ignore_stealthed_spies ? "true" : "false"),
			hr.speed, hr.turn_power, hr.min_dot_product);
	}
	
	void Parse_ItemModel(CTFBotSpawner *spawner, KeyValues *kv)
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
		
		DevMsg("CTFBotSpawner %08x: add ItemModel(\"%s\", \"%s\")\n", (uintptr_t)spawner, item_name, model_name);
		spawners[spawner].item_models[item_name] = model_name;
	}

	void Parse_CustomWeaponModel(CTFBotSpawner *spawner, KeyValues *kv)
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
			(uintptr_t)spawner, slot, path);
		spawners[spawner].custom_weapon_models[slot] = path;
	}
	void Parse_ShootTemplate(CTFBotSpawner *spawner, KeyValues *kv)
	{
		ShootTemplateData data;
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();

			if (FStrEq(name, "Speed")) {
				data.speed = subkey->GetFloat();
			}
			else if (FStrEq(name, "Spread")){
				data.spread = subkey->GetFloat();
			}
			else if (FStrEq(name, "Offset")){
				sscanf(subkey->GetString(),"%f %f %f",&data.offset.x,&data.offset.y,&data.offset.z);
			}
			else if (FStrEq(name, "Angles")) {
				sscanf(subkey->GetString(),"%f %f %f",&data.angles.x,&data.angles.y,&data.angles.z);
			}
			else if (FStrEq(name, "OverrideShoot")) {
				data.override_shoot = subkey->GetBool();
			}
			else if (FStrEq(name, "AttachToProjectile")) {
				data.parent_to_projectile = subkey->GetBool();
			}
			else if (FStrEq(name, "Name")) {
				auto it = Point_Templates().find(subkey->GetString()) ;
				if(it != Point_Templates().end())
					data.templ = &(it->second);
			}
			else if (FStrEq(name, "ItemName")) {
				data.weapon = subkey->GetString();
			}
		}
		if (data.templ != nullptr)
			spawners[spawner].shoot_templ.push_back(data);
	}
	AimAt Parse_AimAt(CTFBotSpawner *spawner, KeyValues *kv) {
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

	CTFBotSpawner *current_spawner = nullptr;
	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_Parse, KeyValues *kv_orig)
	{
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		
		current_spawner = spawner;
		// make a temporary copy of the KV subtree for this spawner
		// the reason for this: `kv_orig` *might* be a ptr to a shared template KV subtree
		// we'll be deleting our custom keys after we parse them so that the Valve code doesn't see them
		// but we don't want to delete them from the shared template KV subtree (breaks multiple uses of the template)
		// so we use this temp copy, delete things from it, pass it to the Valve code, then delete it
		// (we do the same thing in Pop:WaveSpawn_Extensions)
		KeyValues *kv = kv_orig->MakeCopy();
		
	//	DevMsg("CTFBotSpawner::Parse\n");
		
		std::vector<KeyValues *> del_kv;
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			bool del = true;
			if (FStrEq(name, "AddCond")) {
				Parse_AddCond(spawner, subkey);
			} else if (FStrEq(name, "DamageAppliesCond")) {
				Parse_DamageAppliesCond(spawner, subkey);
			} else if (FStrEq(name, "Taunt")) {
				Parse_PeriodicTask(spawner, subkey, TASK_TAUNT);
			} else if (FStrEq(name, "Spell")) {
				Parse_PeriodicTask(spawner, subkey, TASK_GIVE_SPELL);
			} else if (FStrEq(name, "VoiceCommand")) {
				Parse_PeriodicTask(spawner, subkey, TASK_VOICE_COMMAND);
			} else if (FStrEq(name, "FireWeapon")) {
				Parse_PeriodicTask(spawner, subkey, TASK_FIRE_WEAPON);
			} else if (FStrEq(name, "ChangeAttributes")) {
				Parse_PeriodicTask(spawner, subkey, TASK_CHANGE_ATTRIBUTES);
			} else if (FStrEq(name, "FireInput")) {
				Parse_PeriodicTask(spawner, subkey, TASK_FIRE_INPUT);
			} else if (FStrEq(name, "Action")) {
				Parse_Action(spawner, subkey);
			} else if (FStrEq(name, "WeaponResist")) {
				Parse_WeaponResist(spawner, subkey);
			} else if (FStrEq(name, "ItemColor")) {
				Parse_ItemColor(spawner, subkey);
			} else if (FStrEq(name, "ItemModel")) {
				Parse_ItemModel(spawner, subkey);
			} else if (FStrEq(name, "HomingRockets")) {
				Parse_HomingRockets(spawner, subkey);
			} else if (FStrEq(name, "CustomWeaponModel")) {
				Parse_CustomWeaponModel(spawner, subkey);
			} else if (FStrEq(name, "ShootTemplate")) {
				Parse_ShootTemplate(spawner, subkey);
			} else if (FStrEq(name, "AimAt")) {
				spawners[spawner].aim_at = Parse_AimAt(spawner,subkey);
			} else if (FStrEq(name, "AimOffset")) {
				Vector offset;
				sscanf(subkey->GetString(),"%f %f %f",&offset.x,&offset.y,&offset.z);
				spawners[spawner].aim_offset = offset;
			} else if (FStrEq(name, "AimLeadProjectileSpeed")) {
				spawners[spawner].aim_lead_target_speed = subkey->GetFloat();
			} else if (FStrEq(name, "RocketJump")) {
				spawners[spawner].rocket_jump_type = subkey->GetInt();
			} else if (FStrEq(name, "ForceRomeVision")) {
				spawners[spawner].force_romevision_cosmetics = subkey->GetBool();
			} else if (FStrEq(name, "UseHumanModel")) {
				spawners[spawner].use_human_model = subkey->GetBool();
			} else if (FStrEq(name, "StripItemSlot")) {
				int val = GetLoadoutSlotByName(subkey->GetString());
				spawners[spawner].strip_item_slot.push_back(val == -1 ? subkey->GetInt() : val);
			} else if (FStrEq(name, "UseBusterModel")) {
				spawners[spawner].use_buster_model = subkey->GetBool();
			} else if (FStrEq(name, "UseCustomModel")) {
				spawners[spawner].use_custom_model = subkey->GetString();
			} else if (FStrEq(name, "DeathSound")) {
				spawners[spawner].death_sound = subkey->GetString();
				if (!enginesound->PrecacheSound(subkey->GetString(), true))
					CBaseEntity::PrecacheScriptSound(subkey->GetString());
			} else if (FStrEq(name, "PainSound")) {
				spawners[spawner].pain_sound = subkey->GetString();
				if (!enginesound->PrecacheSound(subkey->GetString(), true))
					CBaseEntity::PrecacheScriptSound(subkey->GetString());
			} else if (FStrEq(name, "FireSound")) {
				spawners[spawner].fire_sound = AllocPooledString(subkey->GetString());
				if (!enginesound->PrecacheSound(subkey->GetString(), true))
					CBaseEntity::PrecacheScriptSound(subkey->GetString());
			} else if (FStrEq(name, "RocketCustomModel")) {
				spawners[spawner].rocket_custom_model = subkey->GetString();
			} else if (FStrEq(name, "RocketCustomParticle")) {
				spawners[spawner].rocket_custom_particle = subkey->GetString();
			} else if (FStrEq(name, "RingOfFire")) {
				spawners[spawner].ring_of_fire = subkey->GetFloat();
			} else if (FStrEq(name, "UseMeleeThreatPrioritization")) {
				spawners[spawner].use_melee_threat_prioritization = subkey->GetBool();
			} else if (FStrEq(name, "SuppressTimedFetchFlag")) {
				spawners[spawner].suppress_timed_fetchflag = subkey->GetBool();
			} else if (FStrEq(name, "NoBombUpgrades")) {
				spawners[spawner].no_bomb_upgrade = subkey->GetBool();
			} else if (FStrEq(name, "SpawnTemplate")) {
				spawners[spawner].templ.push_back(Parse_SpawnTemplate(subkey));
			} else if (FStrEq(name, "AlwaysGlow")) {
				spawners[spawner].always_glow = subkey->GetBool();
			} else if (FStrEq(name, "UseBestWeapon")) {
				spawners[spawner].use_best_weapon = subkey->GetBool();
			} else if (FStrEq(name, "BodyPartScaleSpeed")) {
				spawners[spawner].scale_speed = subkey->GetFloat();
			} else if (FStrEq(name, "AimTrackingInterval")) {
				spawners[spawner].tracking_interval = subkey->GetFloat();
#ifdef ENABLE_BROKEN_STUFF
			} else if (FStrEq(name, "DropWeapon")) {
				spawners[spawner].drop_weapon = subkey->GetBool();}
#endif
			} else {
				del = false;
			}
			
			if (del) {
	//			DevMsg("Key \"%s\": processed, will delete\n", name);
				del_kv.push_back(subkey);
			} else {
	//			DevMsg("Key \"%s\": passthru\n", name);
			}
		}
		
		for (auto subkey : del_kv) {
	//		DevMsg("Deleting key \"%s\"\n", subkey->GetName());
			kv->RemoveSubKey(subkey);
			subkey->deleteThis();
		}
		
		bool result = DETOUR_MEMBER_CALL(CTFBotSpawner_Parse)(kv);
		
		// delete the temporary copy of the KV subtree
		kv->deleteThis();
		
		/* post-processing: modify all of the spawner's EventChangeAttributes_t structs as necessary */
		auto l_postproc_ecattr = [](CTFBotSpawner *spawner, CTFBot::EventChangeAttributes_t& ecattr){
			/* Action Mobber: add implicit Attributes IgnoreFlag */
			if (spawners[spawner].action == ACTION_Mobber) {
				/* operator|= on enums: >:[ */
				ecattr.m_nBotAttrs = static_cast<CTFBot::AttributeType>(ecattr.m_nBotAttrs | CTFBot::ATTR_IGNORE_FLAG);
			}
		};
		
		l_postproc_ecattr(spawner, spawner->m_DefaultAttrs);
		for (auto& ecattr : spawner->m_ECAttrs) {
			l_postproc_ecattr(spawner, ecattr);
		}
		
		return result;
	}
	
	// DETOUR_DECL_STATIC(void, FireEvent, EventInfo *info, const char *name)
	// {
	// 	DevMsg("Fired eventbef");
	// 	DevMsg("Fired event");
	// 	DETOUR_STATIC_CALL(FireEvent)(info, name);
	// }
void TeleportToHint(CTFBot *actor,bool force) {
		if (!force && actor->IsPlayerClass(TF_CLASS_ENGINEER))
			return;

		CHandle<CTFBotHintEngineerNest> h_nest;
		DevMsg("Teleport to hint \n");
		CTFBotMvMEngineerHintFinder::FindHint(true, false, &h_nest);
		
		if (h_nest == nullptr)
			return;
		//if (h_nest != nullptr) {
		//	TFGameRules()->PushAllPlayersAway(h_nest->GetAbsOrigin(),
		//		400.0f, 500.0f, TF_TEAM_RED, nullptr);
	
		DevMsg("Teleport to hint found\n");
		Vector tele_pos = h_nest->GetAbsOrigin();
		QAngle tele_ang = h_nest->GetAbsAngles();
		
		actor->Teleport(&tele_pos, &tele_ang, &vec3_origin);
		DevMsg("Teleporting\n");
		CPVSFilter filter(tele_pos);
		
		TE_TFParticleEffect(filter, 0.0f, "teleported_blue", tele_pos, vec3_angle);
		TE_TFParticleEffect(filter, 0.0f, "player_sparkles_blue", tele_pos, vec3_angle);
		
		if (true) {
			TE_TFParticleEffect(filter, 0.0f, "teleported_mvm_bot", tele_pos, vec3_angle);
			actor->EmitSound("Engineer.MvM_BattleCry07");
			h_nest->EmitSound("MvM.Robot_Engineer_Spawn");
			
			/*if (g_pPopulationManager != nullptr) {
				CWave *wave = g_pPopulationManager->GetCurrentWave();
				if (wave != nullptr) {
					if (wave->m_iEngiesTeleportedIn == 0) {
						TFGameRules()->BroadcastSound(255,
							"Announcer.MvM_First_Engineer_Teleport_Spawned");
					} else {
						TFGameRules()->BroadcastSound(255,
							"Announcer.MvM_Another_Engineer_Teleport_Spawned");
					}
					++wave->m_iEngiesTeleportedIn;
				}
			}*/
		}
		DevMsg("Effects\n");
	}

	void SpyInitAction(CTFBot *actor) {
		ToTFPlayer(actor)->m_Shared->AddCond(TF_COND_STEALTHED_USER_BUFF, 2.0f);
				
		CUtlVector<CTFPlayer *> enemies;
		CollectPlayers<CTFPlayer>(&enemies, GetEnemyTeam(actor), true, false);

		//CUtlVector<CTFPlayer *> enemies2 = enemies;

		if (enemies.Count() > 1) {
			enemies.Shuffle();
		}

		bool success = false;
		int range = 0;
		DevMsg("Pos pre tp %f\n", actor->GetAbsOrigin().x);
		while(!success && range < 3) {
			range++;
			FOR_EACH_VEC(enemies, i) {

				CTFPlayer *enemy = enemies[i];
				
				if(TeleportNearVictim(actor, enemy, range)){
					success = true;
					break;
				}
			}
		}
		DevMsg("Pos post tp %d %f\n", success, actor->GetAbsOrigin().x);
	}
	
	RefCount rc_CTFBotSpawner_Spawn;
	clock_t start_time_spawn;
	DETOUR_DECL_MEMBER(int, CTFBotSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		
		SCOPED_INCREMENT(rc_CTFBotSpawner_Spawn);
		current_spawner = spawner;
		clock_t start = clock() ;
		start_time_spawn = start;
		auto result = DETOUR_MEMBER_CALL(CTFBotSpawner_Spawn)(where, ents);
		clock_t endn = clock() ;
		float timespent = ((endn-start) / (float)CLOCKS_PER_SEC);
		DevMsg("native spawning took %f\n",timespent);
	//	DevMsg("\nCTFBotSpawner %08x: SPAWNED\n", (uintptr_t)spawner);
	//	DevMsg("  [classicon \"%s\"] [miniboss %d]\n", STRING(spawner->GetClassIcon(0)), spawner->IsMiniBoss(0));
	//	DevMsg("- result: %d\n", result);
	//	if (ents != nullptr) {
	//		DevMsg("- ents:  ");
	//		FOR_EACH_VEC((*ents), i) {
	//			DevMsg(" #%d", ENTINDEX((*ents)[i]));
	//		}
	//		DevMsg("\n");
	//	}
		
		if (result && ents != nullptr && !ents->IsEmpty()) {
			auto it = spawners.find(spawner);
			if (it != spawners.end()) {
				SpawnerData& data = (*it).second;
				CTFBot *bot_leader = ToTFBot(ents->Head());
				CTFBot *bot = ToTFBot(ents->Tail());
				if (bot != nullptr) {
					spawner_of_bot[bot] = spawner;
					bots_data[bot] = &data;
					
				//	DevMsg("CTFBotSpawner %08x: found %u AddCond's\n", (uintptr_t)spawner, data.addconds.size());
					for (auto addcond : data.addconds) {
						if (addcond.delay == 0.0f && addcond.health_below == 0) {
							DevMsg("CTFBotSpawner %08x: applying AddCond(%d, %f)\n", (uintptr_t)spawner, addcond.cond, addcond.duration);
							bot->m_Shared->AddCond(addcond.cond, addcond.duration);
						} else {
							delayed_addconds.push_back({
								bot,
								gpGlobals->curtime + addcond.delay,
								addcond.cond,
								addcond.duration,
								addcond.health_below,
								addcond.health_above
							});
						}
					}
					
					for (auto task_spawner : data.periodic_tasks) {
						PeriodicTask ptask;
						ptask.bot=bot;
						ptask.type = task_spawner.type;
						ptask.when = task_spawner.when + gpGlobals->curtime;
						ptask.repeats = task_spawner.repeats;
						ptask.cooldown = task_spawner.cooldown;
						ptask.spell_type = task_spawner.spell_type;
						ptask.spell_count = task_spawner.spell_count;
						ptask.max_spells = task_spawner.max_spells;
						ptask.duration = task_spawner.duration;
						ptask.if_target = task_spawner.if_target;
						ptask.attrib_name = task_spawner.attrib_name;
						ptask.health_below = task_spawner.health_below;
						ptask.health_above = task_spawner.health_above;
						ptask.input_name = task_spawner.input_name;
						ptask.param = task_spawner.param;
						pending_periodic_tasks.push_back(ptask);
					}

					for (auto templ : data.templ) {
						templ.SpawnTemplate(bot);
						//if (Point_Templates().find(templ) != Point_Templates().end())
						//	Point_Templates()[templ].SpawnTemplate(bot);
					}
					if (!data.use_custom_model.empty()) {

					}
					static ConVarRef sig_mvm_bots_are_humans("sig_mvm_bots_are_humans");
					if (!data.use_custom_model.empty()) {
						DevMsg("CTFBotSpawner %08x: applying UseCustomModel(\"%s\") on bot #%d\n", (uintptr_t)spawner, data.use_custom_model.c_str(), ENTINDEX(bot));
						
						bot->GetPlayerClass()->SetCustomModel(data.use_custom_model.c_str(), true);
						bot->UpdateModel();
						bot->SetBloodColor(DONT_BLEED);
						
						// TODO: RomeVision...?
					} else if (data.use_buster_model) {
						DevMsg("CTFBotSpawner %08x: applying UseBusterModel on bot #%d\n", (uintptr_t)spawner, ENTINDEX(bot));
						
						// here we mimic what CMissionPopulator::UpdateMissionDestroySentries does
						bot->GetPlayerClass()->SetCustomModel("models/bots/demo/bot_sentry_buster.mdl", true);
						bot->UpdateModel();
						bot->SetBloodColor(DONT_BLEED);
						
						// TODO: filter-out addition of Romevision cosmetics to UseBusterModel bots
						// TODO: manually add Romevision cosmetic for SentryBuster to UseBusterModel bots
					} else if (data.use_human_model || sig_mvm_bots_are_humans.GetBool()) {
						DevMsg("CTFBotSpawner %08x: applying UseHumanModel on bot #%d\n", (uintptr_t)spawner, ENTINDEX(bot));
						
						// calling SetCustomModel with a nullptr string *seems* to reset the model
						// dunno what the bool parameter should be; I think it doesn't matter for the nullptr case
						bot->GetPlayerClass()->SetCustomModel(nullptr, true);
						bot->UpdateModel();
						bot->SetBloodColor(BLOOD_COLOR_RED);
						
						// TODO: filter-out addition of Romevision cosmetics to UseHumanModel bots
					}
					
					// should really be in OnEventChangeAttributes, where ItemAttributes are applied
					for (const auto& pair : data.item_colors) {
						const char *item_name     = pair.first.c_str();
						const color32& item_color = pair.second;
						
						CEconItemDefinition *item_def = GetItemSchema()->GetItemDefinitionByName(item_name);
						if (item_def == nullptr) continue;
						
						for (int i = 0; i < bot->GetNumWearables(); ++i) {
							CEconWearable *wearable = bot->GetWearable(i);
							if (wearable == nullptr) continue;
							
							CEconItemView *item_view = wearable->GetItem();
							if (item_view == nullptr) continue;
							
							if (item_view->GetItemDefIndex() == item_def->m_iItemDefIndex) {
								DevMsg("CTFBotSpawner %08x: applying color %02X%02X%02X to item \"%s\"\n",
									(uintptr_t)spawner, item_color.r, item_color.g, item_color.b, item_name);
								
								wearable->SetRenderColorR(item_color.r);
								wearable->SetRenderColorG(item_color.g);
								wearable->SetRenderColorB(item_color.b);
							}
						}
						
						for (int i = 0; i < bot->WeaponCount(); ++i) {
							CBaseCombatWeapon *weapon = bot->GetWeapon(i);
							if (weapon == nullptr) continue;
							
							CEconItemView *item_view = weapon->GetItem();
							if (item_view == nullptr) continue;
							
							if (item_view->GetItemDefIndex() == item_def->m_iItemDefIndex) {
								DevMsg("CTFBotSpawner %08x: applying color %02X%02X%02X to item \"%s\"\n",
									(uintptr_t)spawner, item_color.r, item_color.g, item_color.b, item_name);
								
								weapon->SetRenderColorR(item_color.r);
								weapon->SetRenderColorG(item_color.g);
								weapon->SetRenderColorB(item_color.b);
							}
						}
						
					#if 0
						for (int slot = GetNumberOfLoadoutSlots() - 1; slot >= 0; --slot) {
							CEconEntity *econ_entity = nullptr;
							
							CEconItemView *item_view = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot(bot, slot, &econ_entity);
							if (item_view == nullptr || econ_entity == nullptr) continue;
							
							if (item_view->GetItemDefIndex() == item_def->m_iItemDefIndex) {
								DevMsg("CTFBotSpawner %08x: applying color %02X%02X%02X to item \"%s\"\n",
									(uintptr_t)spawner, item_color.r, item_color.g, item_color.b, item_name);
								
								econ_entity->SetRenderColorR(item_color.r);
								econ_entity->SetRenderColorG(item_color.g);
								econ_entity->SetRenderColorB(item_color.b);
							}
						}
					#endif
					}

					for (const auto& pair : data.item_models) {
						const char *item_name     = pair.first.c_str();
						const char *item_model = pair.second.c_str();
						
						CEconItemDefinition *item_def = GetItemSchema()->GetItemDefinitionByName(item_name);
						if (item_def == nullptr) continue;
						
						for (int i = 0; i < bot->GetNumWearables(); ++i) {
							CEconWearable *wearable = bot->GetWearable(i);
							if (wearable == nullptr) continue;
							
							CEconItemView *item_view = wearable->GetItem();
							if (item_view == nullptr) continue;
							
							if (item_view->GetItemDefIndex() == item_def->m_iItemDefIndex) {
								
								CBaseEntity::PrecacheModel(item_model);
								wearable->SetModelIndex(modelinfo->GetModelIndex(item_model));
								for (int i = 0; i < MAX_VISION_MODES; ++i) {
									wearable->SetModelIndexOverride(i, modelinfo->GetModelIndex(item_model));
								}
							}
						}
						
						for (int i = 0; i < bot->WeaponCount(); ++i) {
							CBaseCombatWeapon *weapon = bot->GetWeapon(i);
							if (weapon == nullptr) continue;
							
							CEconItemView *item_view = weapon->GetItem();
							if (item_view == nullptr) continue;
							
							if (item_view->GetItemDefIndex() == item_def->m_iItemDefIndex) {
								
								CBaseEntity::PrecacheModel(item_model);
								weapon->SetModelIndex(modelinfo->GetModelIndex(item_model));
								for (int i = 0; i < MAX_VISION_MODES; ++i) {
									weapon->SetModelIndexOverride(i, modelinfo->GetModelIndex(item_model));
								}
							}
						}
					}
					//Replenish clip
					for (int i = 0; i < bot->WeaponCount(); ++i) {
						CBaseCombatWeapon *weapon = bot->GetWeapon(i);
						if (weapon == nullptr) continue;
						weapon->m_iClip1 = weapon->GetMaxClip1();
					}

					CTFWearable *pActionSlotEntity = bot->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION );
					if ( pActionSlotEntity  != nullptr) {

						// get the equipped item and see what it is
						CTFPowerupBottle *pPowerupBottle = rtti_cast< CTFPowerupBottle* >( pActionSlotEntity );
						if (pPowerupBottle  != nullptr) {
							int val=0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(pPowerupBottle, val, powerup_charges);
							DevMsg("Poweup Bottles %d\n",val);
							pPowerupBottle->m_usNumCharges = val;
						}
					}

					for (const auto& pair : data.custom_weapon_models) {
						int slot         = pair.first;
						const char *path = pair.second.c_str();
						
						CBaseEntity *item;
						if ((item = bot->GetEquippedWearableForLoadoutSlot(slot)) == nullptr &&
							(item = bot->Weapon_GetSlot(slot)) == nullptr) {
							DevMsg("CTFBotSpawner %08x: can't find item slot %d for CustomWeaponModel\n",
								(uintptr_t)spawner, slot);
							continue;
						}
						
						DevMsg("CTFBotSpawner %08x: item slot %d is entity #%d classname \"%s\"\n",
							(uintptr_t)spawner, slot, ENTINDEX(item), item->GetClassname());
						
						DevMsg("CTFBotSpawner %08x: changing item model to \"%s\"\n",
							(uintptr_t)spawner, path);
						
						CBaseEntity::PrecacheModel(path);
						for (int i = 0; i < MAX_VISION_MODES; ++i) {
							item->SetModelIndexOverride(i, modelinfo->GetModelIndex(path));
						}
					}

					for (int slot : data.strip_item_slot) {
						CBaseEntity *item;
						bool held_item = false;
						if ((item = bot->GetEquippedWearableForLoadoutSlot(slot)) != nullptr || (item = bot->Weapon_GetSlot(slot)) != nullptr) {
							CBaseCombatWeapon *weapon;

							if (bot->GetActiveTFWeapon() == item) {
								held_item = true;
							}

							if ((weapon = rtti_cast<CBaseCombatWeapon *>(item)) != nullptr) {
								bot->Weapon_Detach(weapon);
							}

							item->Remove();
						}

						//if (held_item)
						bot->EquipBestWeaponForThreat(nullptr);

						DevMsg ("Strip slot %d %d %d %d\n", slot, item != nullptr, bot->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY ) != nullptr, held_item);
					}

					if (data.force_romevision_cosmetics) {
						for (int i = 0; i < 2; i++) {
							CEconItemView *item_view= CEconItemView::Create();
							item_view->Init(152, 6, 9999, 0);
							CEconWearable *wearable = rtti_cast<CEconWearable *>(ItemGeneration()->GenerateItemFromScriptData(item_view,Vector(0,0,0), QAngle(0,0,0),"tf_wearable"));
							if (wearable) {
								
								wearable->m_bValidatedAttachedEntity = true;
								wearable->GiveTo(bot);
								DevMsg("Created wearable %d\n",bot->GetPlayerClass()->GetClassIndex()*2 + i);
								bot->EquipWearable(wearable);
								const char *path = ROMEVISON_MODELS[bot->GetPlayerClass()->GetClassIndex()*2 + i];
								CBaseEntity::PrecacheModel(path);
								wearable->SetModelIndex(modelinfo->GetModelIndex(path));
								for (int j = 0; j < MAX_VISION_MODES; ++j) {
									wearable->SetModelIndexOverride(j, modelinfo->GetModelIndex(path));
								}
							}
						}
					}

					DevMsg("Dests %d\n",Teleport_Destination().size());
					if (!(bot->m_nBotAttrs & CTFBot::AttributeType::ATTR_TELEPORT_TO_HINT) && !Teleport_Destination().empty()) {
						bool done = false;
						CBaseEntity *destination = nullptr;

						if (Teleport_Destination().find("small") != Teleport_Destination().end() && !bot_leader->IsMiniBoss()) {
							destination = Teleport_Destination().find("small")->second;
						}
						else if (Teleport_Destination().find("giants") != Teleport_Destination().end() && bot_leader->IsMiniBoss()) {
							destination = Teleport_Destination().find("giants")->second;
						}
						else if (Teleport_Destination().find("all") != Teleport_Destination().end()) {
							destination = Teleport_Destination().find("all")->second;
						}
						else {
							ForEachEntityByClassname("info_player_teamspawn", [&](CBaseEntity *ent){
								if (done)
									return;

								auto vec = ent->WorldSpaceCenter();
								
								auto area = TheNavMesh->GetNearestNavArea(vec);

								if (area != nullptr) {
									vec = area->GetCenter();
								}

								float dist = vec.DistToSqr(bot->GetAbsOrigin());
								DevMsg("Dist %f %s\n",dist, ent->GetEntityName());
								if (dist < 1000) {
									auto dest = Teleport_Destination().find(STRING(ent->GetEntityName()));
									if(dest != Teleport_Destination().end() && dest->second != nullptr){
										destination = dest->second;
										done = true;
									}
								}
							});
						}
						if (destination != nullptr)
						{
							auto vec = destination->WorldSpaceCenter();
							vec.z += destination->CollisionProp()->OBBMaxs().z;
							bool is_space_to_spawn = IsSpaceToSpawnHere(vec);
							if (!is_space_to_spawn)
								vec.z += 50.0f;
							if (is_space_to_spawn || IsSpaceToSpawnHere(vec)){
								bot->Teleport(&(vec),&(destination->GetAbsAngles()),&(bot->GetAbsVelocity()));
								bot->EmitSound("MVM.Robot_Teleporter_Deliver");
								bot->m_Shared->AddCond(TF_COND_INVULNERABLE_CARD_EFFECT,1.5f);
							}
						}
						
					}

					if (bot->GetPlayerClass()->GetClassIndex() != TF_CLASS_ENGINEER && (bot->m_nBotAttrs & CTFBot::AttributeType::ATTR_TELEPORT_TO_HINT))
						TeleportToHint(bot, data.action != ACTION_Default);

					if (data.action == ACTION_BotSpyInfiltrate) {
						SpyInitAction(bot);
					}
				}
			}
		}
		
		clock_t end = clock() ;
		timespent = ((end-endn) / (float)CLOCKS_PER_SEC);
		DevMsg("detour spawning took %f",timespent);

		return result;
	}
	
	// DETOUR_DECL_MEMBER(bool, CTFPlayer_IsMiniBoss)
	// {
	// 	if (rc_CTFBotSpawner_Spawn > 0) {
	// 		clock_t endn = clock();
	// 		float timespent = ((endn-start_time_spawn) / (float)CLOCKS_PER_SEC);
	// 		DevMsg("IsMiniBoss %f\n",timespent);
	// 	}
	// 	return DETOUR_MEMBER_CALL(CTFPlayer_IsMiniBoss)();
	// }

	
	// DETOUR_DECL_MEMBER(void, CTFPlayer_HandleCommand_JoinClass, const char *pClassName, bool b1)
	// {
	// 	if (rc_CTFBotSpawner_Spawn > 0) {
	// 		clock_t endn = clock();
	// 		float timespent = ((endn-start_time_spawn) / (float)CLOCKS_PER_SEC);
	// 		DevMsg("JoinClass %f\n",timespent);
	// 	}
	// 	DETOUR_MEMBER_CALL(CTFPlayer_HandleCommand_JoinClass)(pClassName, b1);
	// }
	// DETOUR_DECL_MEMBER(void, CBaseCombatCharacter_SetBloodColor, int color)
	// {
	// 	if (rc_CTFBotSpawner_Spawn > 0) {
	// 		clock_t endn = clock();
	// 		float timespent = ((endn-start_time_spawn) / (float)CLOCKS_PER_SEC);
	// 		DevMsg("SetBloodColor %d %f\n",color, timespent);
	// 	}
	// 	DETOUR_MEMBER_CALL(CBaseCombatCharacter_SetBloodColor)(color);
	// }
	// DETOUR_DECL_MEMBER(void, CBaseAnimating_SetModelScale, float scale, float changeduration)
	// {
	// 	if (rc_CTFBotSpawner_Spawn > 0) {
	// 		clock_t endn = clock();
	// 		float timespent = ((endn-start_time_spawn) / (float)CLOCKS_PER_SEC);
	// 		DevMsg("SetModelScale %f %f\n",scale, timespent);
	// 	}
	// 	DETOUR_MEMBER_CALL(CBaseAnimating_SetModelScale)(scale, changeduration);
	// }
	
	// DETOUR_DECL_MEMBER(void, CTFBot_StartIdleSound)
	// {
	// 	if (rc_CTFBotSpawner_Spawn > 0) {
	// 		clock_t endn = clock();
	// 		float timespent = ((endn-start_time_spawn) / (float)CLOCKS_PER_SEC);
	// 		DevMsg("StartIdleSound %f\n",timespent);
	// 	}
	// 	DETOUR_MEMBER_CALL(CTFBot_StartIdleSound)();
	// }
	DETOUR_DECL_MEMBER(void, CTFBot_AddEventChangeAttributes, CTFBot::EventChangeAttributes_t * ecattr)
	{
	 	if (ecattr == nullptr || ecattr->m_strName == nullptr ) {
			PrintToChatAll("Bot spawned with invalid event change attributes");
			return;
		}

	 	DETOUR_MEMBER_CALL(CTFBot_AddEventChangeAttributes)(ecattr);
	}
	DETOUR_DECL_MEMBER(CTFBot::EventChangeAttributes_t *, CTFBot_GetEventChangeAttributes, const char *name)
	{
		if (name == nullptr) {
			PrintToChatAll("Invalid changebotattributes name");
			return nullptr;
		}
		return DETOUR_MEMBER_CALL(CTFBot_GetEventChangeAttributes)(name);
	}
	// DETOUR_DECL_MEMBER(void *, CItemGeneration_GenerateRandomItem, void *criteria, const Vector &vec, const QAngle &ang)
	// {
	// 	if (rc_CTFBotSpawner_Spawn > 0) {
	// 		clock_t endn = clock();
	// 		float timespent = ((endn-start_time_spawn) / (float)CLOCKS_PER_SEC);
	// 		DevMsg("GenerateRandomItem %f\n",timespent);
	// 	}
	// 	void* ret = DETOUR_MEMBER_CALL(CItemGeneration_GenerateRandomItem)(criteria,vec,ang);
	// 	if (rc_CTFBotSpawner_Spawn > 0) {
	// 		clock_t endn = clock();
	// 		float timespent = ((endn-start_time_spawn) / (float)CLOCKS_PER_SEC);
	// 		DevMsg("GenerateRandomItem end %f\n",timespent);
	// 	}
	// 	return ret;
	// }

	ConVar cvar_no_romevision("sig_no_romevision_cosmetics", "0", FCVAR_NONE,
		"Disable romevision cosmetics");
	ConVar cvar_creators_custom_item("sig_creators_custom_item", "0", FCVAR_NONE,
		"Enable fallback to creators custom item");

	const char *TranslateWeaponEntForClass_improved(const char *name, int classnum)
	{
		DevMsg("classname %s\n",name);
		if (strncasecmp(name, "tf_weapon_", 10) == 0)
		{
			DevMsg("is translated %s\n",name+10);
			if (strcasecmp(name+10, "shotgun") == 0) {
				DevMsg("is shotgun\n");
				switch (classnum) {
				case TF_CLASS_SOLDIER:
					return "tf_weapon_shotgun_soldier";
				case TF_CLASS_PYRO:
					return "tf_weapon_shotgun_pyro";
				case TF_CLASS_HEAVYWEAPONS:
					return "tf_weapon_shotgun_hwg";
				case TF_CLASS_ENGINEER:
					return "tf_weapon_shotgun_primary";
				default:
					return "tf_weapon_shotgun_pyro";
				}
			}
		
			if (strcasecmp(name+10, "throwable") == 0) {
				switch (classnum) {
				case TF_CLASS_MEDIC:
					return "tf_weapon_throwable_primary";
				default:
					return "tf_weapon_throwable_secondary";
				}
			}
			
			if (strcasecmp(name+10, "parachute") == 0) {
				switch (classnum) {
				case TF_CLASS_SOLDIER:
					return "tf_weapon_parachute_secondary";
				case TF_CLASS_DEMOMAN:
					return "tf_weapon_parachute_primary";
				default:
					return "tf_weapon_parachute";
				}
			}
			
			if (strcasecmp(name+10, "revolver") == 0) {
				switch (classnum) {
				case TF_CLASS_ENGINEER:
					return "tf_weapon_revolver_secondary";
				case TF_CLASS_SPY:
					return "tf_weapon_revolver";
				default:
					return "tf_weapon_revolver";
				}
			}
			if (strcasecmp(name+10, "pistol") == 0) {
				switch (classnum) {
				case TF_CLASS_SCOUT:
					return "tf_weapon_pistol_scout";
				case TF_CLASS_ENGINEER:
					return "tf_weapon_pistol";
				default:
					return "tf_weapon_pistol";
				}
			}
			
			if (strcasecmp(name+10, "shovel") == 0 || strcasecmp(name+10, "bottle") == 0) {
				switch (classnum) {
				case TF_CLASS_SOLDIER:
					return "tf_weapon_shovel";
				case TF_CLASS_DEMOMAN:
					return "tf_weapon_bottle";
				}
			}
		}
		else if (strcasecmp(name, "saxxy") == 0) {
			switch (classnum) {
			case TF_CLASS_SCOUT:
				return "tf_weapon_bat";
			case TF_CLASS_SOLDIER:
				return "tf_weapon_shovel";
			case TF_CLASS_PYRO:
				return "tf_weapon_fireaxe";
			case TF_CLASS_DEMOMAN:
				return "tf_weapon_bottle";
			case TF_CLASS_HEAVYWEAPONS:
				return "tf_weapon_fireaxe";
			case TF_CLASS_ENGINEER:
				return "tf_weapon_wrench";
			case TF_CLASS_MEDIC:
				return "tf_weapon_bonesaw";
			case TF_CLASS_SNIPER:
				return "tf_weapon_club";
			case TF_CLASS_SPY:
				return "tf_weapon_knife";
			}
		}
		
		/* if not handled: return original entity name, not an empty string */
		return name;
	}

	RefCount rc_CTFBot_AddItem;
	RefCount rc_CTFBot_OnEventChangeAttributes;
	DETOUR_DECL_MEMBER(int, CTFItemDefinition_GetLoadoutSlot, int classIndex)
	{
		CTFItemDefinition *item_def=reinterpret_cast<CTFItemDefinition *>(this);
		int slot = DETOUR_MEMBER_CALL(CTFItemDefinition_GetLoadoutSlot)(classIndex);
		if (rc_CTFBot_OnEventChangeAttributes) {
			DevMsg("Get loadoutslot %d\n", rc_CTFBot_AddItem);
			const char *name = item_def->GetItemClass();
			if (rc_CTFBot_AddItem && (FStrEq(name,"tf_weapon_buff_item") || FStrEq(name,"tf_weapon_parachute") || FStrEq(name,"tf_wearable_demoshield") || FStrEq(name,"tf_wearable")))
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
				item_def = rtti_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(item_name));
				item_defs[item_name] = item_def;
			}

			if (item_def != nullptr) {
				//No romevision cosmetics
				if (item_def->m_iItemDefIndex >= 30143 && item_def->m_iItemDefIndex <= 30161 && cvar_no_romevision.GetBool())
					return nullptr;

				const char *classname = item_def->GetItemClass();
				CEconItemView *item_view= CEconItemView::Create();
				item_view->Init(item_def->m_iItemDefIndex, 6, 9999, 0);
				ret = ItemGeneration()->GenerateItemFromScriptData(item_view,vec, ang,classname);
			}

			if (ret == nullptr){
				if (cvar_creators_custom_item.GetBool() && bot_additem ) {
					DevMsg("equip custom ctf item %s\n", item_name);
					engine->ServerCommand(CFmtStr("ce_mvm_equip_itemname %d \"%s\"\n", ENTINDEX(bot_additem), item_name));
					engine->ServerExecute();

					// static ConVarRef ce_mvm_equip_itemname_cvar("sig_mvm_set_credit_team");
					// if (sig_mvm_set_credit_team.IsValid() && sig_mvm_set_credit_team.GetBool()) {
					// 	return sig_mvm_set_credit_team.GetInt();
					// }

					bot_additem = nullptr;
				}

				else
					ret = DETOUR_MEMBER_CALL(CItemGeneration_GenerateRandomItem)(criteria,vec,ang);
			}
			if (ret != nullptr) {

			}

			return ret;
		}
		return DETOUR_MEMBER_CALL(CItemGeneration_GenerateRandomItem)(criteria,vec,ang);
	}
	DETOUR_DECL_MEMBER(void, CTFBot_OnEventChangeAttributes, void *ecattr)
	{
		SCOPED_INCREMENT(rc_CTFBot_OnEventChangeAttributes);
		DETOUR_MEMBER_CALL(CTFBot_OnEventChangeAttributes)(ecattr);

		CTFBot *bot = reinterpret_cast<CTFBot *>(this);
		auto data = GetDataForBot(bot);
		if (data == nullptr)
			data = &(spawners[current_spawner]);
		//DevMsg("OnEventChange %d %d\n",bot != nullptr, data != nullptr);
		
		if (data != nullptr) {
			//DevMsg("size: %d\n",data->event_change_atttributes_data.size());
			auto it = data->event_change_atttributes_data.find(ecattr);
			if (it != data->event_change_atttributes_data.end()) {
				EventChangeAttributesData &event_data = it->second;
				
				//DevMsg("has ecattr, size %d\n",event_data.custom_attrs.size());
				for(auto ititem = event_data.custom_attrs.begin(); ititem != event_data.custom_attrs.end(); ititem++) {
					for (int i = 0; i < bot->GetNumWearables(); ++i) {
						CEconWearable *wearable = bot->GetWearable(i);
						if (wearable == nullptr) continue;
						
						CEconItemView *item_view = wearable->GetItem();
						if (item_view == nullptr) continue;
						//DevMsg("Compare item to name %s %s\n",item_view->GetStaticData()->GetName(), ititem->first.c_str());
						if (FStrEq(item_view->GetStaticData()->GetName(), ititem->first.c_str())) {
							//DevMsg("Custom attrib count %d\n ",ititem->second.size());
							for(auto itattr = ititem->second.begin(); itattr != ititem->second.end(); itattr++) {
								DevMsg("Added custom attribute %s\n",itattr->first.c_str());
								engine->ServerCommand(CFmtStr("ce_mvm_set_attribute %d \"%s\" %f\n", ENTINDEX(wearable), itattr->first.c_str(), itattr->second));
								engine->ServerExecute();
							}
						}
					}
					
					for (int i = 0; i < bot->WeaponCount(); ++i) {
						CBaseCombatWeapon *weapon = bot->GetWeapon(i);
						if (weapon == nullptr) continue;
						
						CEconItemView *item_view = weapon->GetItem();
						if (item_view == nullptr) continue;
						
						//DevMsg("Compare item to name %s %s\n",item_view->GetStaticData()->GetName(), ititem->first.c_str());
						if (FStrEq(item_view->GetStaticData()->GetName(), ititem->first.c_str())) {
							
							for(auto itattr = ititem->second.begin(); itattr != ititem->second.end(); itattr++) {
								DevMsg("Added custom attribute %s\n",itattr->first.c_str());
								engine->ServerCommand(CFmtStr("ce_mvm_set_attribute %d \"%s\" %f\n", ENTINDEX(weapon), itattr->first.c_str(), itattr->second));
								engine->ServerExecute();
								
							}
						}
					}
				}
			}
		}
		
	}

	#warning __gcc_regcall detours considered harmful!
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, bool, ParseDynamicAttributes, void *ecattr, KeyValues *kv)
	{
		if (!cvar_creators_custom_item.GetBool())
			return DETOUR_STATIC_CALL(ParseDynamicAttributes)(ecattr, kv);

		//DevMsg("ParseDynamicAttributesTFBot: \"%s\" \"%s\"\n", kv->GetName(), kv->GetString());
		
		if (V_stricmp(kv->GetName(), "ItemAttributes") == 0) {
			//DevMsg("Item Attributes\n");
			std::map<std::string, float> attribs;
			std::string item_name = "";
			std::vector<KeyValues *> del_kv;

			FOR_EACH_SUBKEY( kv, subkey )
			{
				if (V_stricmp(subkey->GetName(), "ItemName") == 0)
				{
					item_name = subkey->GetString();
					//DevMsg("Item Name %s\n",item_name.c_str());
					auto it = item_defs.find(item_name);
					CTFItemDefinition *item_def;
					if (it != item_defs.end()) {
						item_def = it->second;
					}
					else {
						item_def = rtti_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(item_name.c_str()));
						item_defs[item_name] = item_def;
					}

					if (item_def == nullptr) {
						auto it_remap = item_custom_remap.find(item_name);
						if (it_remap != item_custom_remap.end()) {
							subkey->SetStringValue(it_remap->second.c_str());
							DevMsg("Found item %s as %s\n",item_name.c_str(), it_remap->second.c_str());
							item_name = it_remap->second.c_str(); 
						}
						else {
							engine->ServerCommand(CFmtStr("ce_mvm_get_itemdef_id \"%s\"\n", item_name.c_str()));
							engine->ServerExecute();
							engine->ServerExecute();

							static ConVarRef ce_mvm_check_itemname_cvar("ce_mvm_check_itemname_cvar");
							DevMsg("Custom item name %s\n",item_name.c_str());
							if (ce_mvm_check_itemname_cvar.IsValid() && ce_mvm_check_itemname_cvar.GetInt() != -1) {
								item_def = rtti_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinition(ce_mvm_check_itemname_cvar.GetInt()));
								std::string newname = item_def->GetName("");
								item_custom_remap[item_name] = newname;
								
								DevMsg("Added custom item %s to reg %s\n",item_name.c_str(), newname.c_str());
								subkey->SetStringValue(newname.c_str());
								item_name = newname;

							}
							else if (ce_mvm_check_itemname_cvar.IsValid()){
								DevMsg("Not found item %s, shown %d\n",item_name.c_str(),ce_mvm_check_itemname_cvar.GetInt());
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
						attribs[subkey->GetName()] = subkey->GetFloat();
						del_kv.push_back(subkey);
					}
				}
			}
			for (auto subkey : del_kv) {
				kv->RemoveSubKey(subkey);
				subkey->deleteThis();
			}
			spawners[current_spawner].event_change_atttributes_data[ecattr].custom_attrs[item_name] = attribs;
			DevMsg("put event changed attributes data %s %d\n",item_name.c_str(), attribs.size());
		}
		
		//DevMsg("  Passing through to actual ParseDynamicAttributes\n");
		return DETOUR_STATIC_CALL(ParseDynamicAttributes)(ecattr, kv);
	}

	DETOUR_DECL_MEMBER(CEconItemDefinition *, CEconItemSchema_GetItemDefinitionByName, const char *name)
	{
		DevMsg("GetItemDefinitionByName %s %d\n",name, rc_CTFBot_OnEventChangeAttributes);
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
	
	DETOUR_DECL_STATIC(CBaseEntity *, CreateEntityByName, const char *className, int iForceEdictIndex)
	{
		if (rc_CTFBot_AddItem > 0) {
			return DETOUR_STATIC_CALL(CreateEntityByName)(TranslateWeaponEntForClass_improved(className,bot_classnum), iForceEdictIndex);
		}
		return DETOUR_STATIC_CALL(CreateEntityByName)(className, iForceEdictIndex);
	}

	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotScenarioMonitor_DesiredScenarioAndClassAction, CTFBot *actor)
	{
		auto data = GetDataForBot(actor);
		if (data != nullptr) {
			switch (data->action) {
			
			case ACTION_Default:
				break;
			
			case ACTION_FetchFlag:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to FetchFlag\n", ENTINDEX(actor), actor->GetPlayerName());
				return CTFBotFetchFlag::New();
			
			case ACTION_PushToCapturePoint:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to PushToCapturePoint[-->FetchFlag]\n", ENTINDEX(actor), actor->GetPlayerName());
				return CTFBotPushToCapturePoint::New(CTFBotFetchFlag::New());
			
			case ACTION_Mobber:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to Mobber\n", ENTINDEX(actor), actor->GetPlayerName());
				return new CTFBotMobber();

			case ACTION_BotSpyInfiltrate:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to SpyInfiltrate\n", ENTINDEX(actor), actor->GetPlayerName());
				return CTFBotSpyInfiltrate::New();
			case ACTION_SniperLurk:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to Sniper\n", ENTINDEX(actor), actor->GetPlayerName());
				actor->SetMission(CTFBot::MISSION_SNIPER);
				break;
			case ACTION_DestroySentries:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to DestroySentries\n", ENTINDEX(actor), actor->GetPlayerName());
				CBaseEntity *target = actor->SelectRandomReachableEnemy();
					
				if (target == nullptr) {
					CBaseEntity *target = servertools->FindEntityByClassname(nullptr, "obj_sentrygun");
				}
				if (target != nullptr) {
					//float m_flScale; // +0x2bf4
					targets_sentrybuster[actor]=target;
					//float scale = *(float *)((uintptr_t)actor+ 0x2bf4);
					//*(CHandle<CBaseEntity>*)((uintptr_t)actor+ 0x2c00) = target;
				}
				actor->SetMission(CTFBot::MISSION_DESTROY_SENTRIES);
				break;
			}
		}
		//if (actor->m_nBotAttrs & CTFBot::AttributeType::ATTR_TELEPORT_TO_HINT)
		//	TeleportToHint(actor,data != nullptr && data->action != ACTION_Default);
		Action<CTFBot> *action = DETOUR_MEMBER_CALL(CTFBotScenarioMonitor_DesiredScenarioAndClassAction)(actor);

		/*if (data != nullptr && action != nullptr && data->action == ACTION_DestroySentries) {
			DevMsg("CTFBotSpawner: got bomber \n");
			auto bomber = reinterpret_cast<CTFBotMissionSuicideBomber *>(action);
			if (bomber != nullptr) {
				DevMsg("CTFBotSpawner: success getting class \n");
				CBaseEntity *target = actor->SelectRandomReachableEnemy();
				if (target != nullptr) {
					DevMsg("CTFBotSpawner: getting reachable enemy #%d - we are \n", ENTINDEX(target));
					*(CHandle<CBaseEntity>*)((uintptr_t)actor+ 0x2c00) = target;
					bomber->m_vecDetonatePos = target->GetAbsOrigin();
					bomber->m_hTarget = target;
					
				}
			}
		}*/
		return action;
	}
	
	

	DETOUR_DECL_MEMBER(void, CTFBotTacticalMonitor_AvoidBumpingEnemies, CTFBot *actor)
	{
		if (!actor->HasItem())
			DETOUR_MEMBER_CALL(CTFBotTacticalMonitor_AvoidBumpingEnemies)(actor);
	}

	RefCount rc_CTFBotScenarioMonitor_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotScenarioMonitor_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_CTFBotScenarioMonitor_Update);
		return DETOUR_MEMBER_CALL(CTFBotScenarioMonitor_Update)(actor, dt);
	}
	
	RefCount rc_CTFBot_GetFlagToFetch;
	DETOUR_DECL_MEMBER(CCaptureFlag *, CTFBot_GetFlagToFetch)
	{
		// if (rc_CTFBotSpawner_Spawn > 0) {
		// 	clock_t endn = clock();
		// 	float timespent = ((endn-start_time_spawn) / (float)CLOCKS_PER_SEC);
		// 	DevMsg("GetFlagToFetch %f\n",timespent);
		// }
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		/* for SuppressTimedFetchFlag, we carefully ensure that we only spoof
		 * the result of CTFBot::GetFlagToFetch when called from
		 * CTFBotScenarioMonitor::Update (the part of the AI where the timer
		 * checks and the actual SuspendFor(CTFBotFetchFlag) occur);
		 * the rest of the AI's calls to GetFlagToFetch will be untouched */
		if (rc_CTFBotScenarioMonitor_Update > 0) {
			auto data = GetDataForBot(bot);
			if (data != nullptr && data->suppress_timed_fetchflag) {
				return nullptr;
			}
		}
		
		SCOPED_INCREMENT(rc_CTFBot_GetFlagToFetch);
		auto result = DETOUR_MEMBER_CALL(CTFBot_GetFlagToFetch)();
		
	//	DevMsg("CTFBot::GetFlagToFetch([#%d \"%s\"]) = [#%d \"%s\"]\n",
	//		ENTINDEX(bot), bot->GetPlayerName(),
	//		(result != nullptr ? ENTINDEX(result) : 0),
	//		(result != nullptr ? STRING(result->GetEntityName()) : "nullptr"));
	//	DevMsg("    --> bot attributes: %08x\n", bot->m_nBotAttrs);
		
		return result;
	}
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsPlayerClass, int iClass)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
	//	if (rc_CTFBot_GetFlagToFetch > 0 && iClass == TF_CLASS_ENGINEER) {
	//		DevMsg("CTFPlayer::IsPlayerClass([#%d \"%s\"], TF_CLASS_ENGINEER): called from CTFBot::GetFlagToFetch, returning false.\n", ENTINDEX(player), player->GetPlayerName());
	//		return false;
	//	}
		
		auto result = DETOUR_MEMBER_CALL(CTFPlayer_IsPlayerClass)(iClass);
		
		if (rc_CTFBot_GetFlagToFetch > 0 && result && iClass == TF_CLASS_ENGINEER) {
			auto data = GetDataForBot(player);
			if (data != nullptr) {
				/* disable the implicit "Attributes IgnoreFlag" thing given to
				 * engineer bots if they have one of our Action overrides
				 * enabled (the pop author can explicitly give the engie bot
				 * "Attributes IgnoreFlag" if they want, of course)
				 * NOTE: this logic will NOT take effect if the bot also has the
				 * SuppressTimedFetchFlag custom parameter */
				if (data->action != ACTION_Default) {
					return false;
				}
			}
		}
		
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMissionSuicideBomber_OnStart, CTFBot *actor, Action<CTFBot> *action)
	{
		auto me = reinterpret_cast<CTFBotMissionSuicideBomber *>(this);
		DevMsg("executed suicide bomber%d %d\n",me->m_bDetReachedGoal, me->m_bDetonating );
		
		auto result = DETOUR_MEMBER_CALL(CTFBotMissionSuicideBomber_OnStart)(actor, action);
		if (me->m_hTarget == nullptr && targets_sentrybuster.find(actor) != targets_sentrybuster.end()){
			CBaseEntity *target = targets_sentrybuster[actor];
			me->m_hTarget = target;
			if (target != nullptr && target->GetAbsOrigin().IsValid() && ENTINDEX(target) > 0){
				me->m_hTarget = target;
				me->m_vecTargetPos = target->GetAbsOrigin();
				me->m_vecDetonatePos = target->GetAbsOrigin();
			}
			targets_sentrybuster.erase(actor);
		}
		DevMsg("reached goal %d,detonating %d, %d %f\n",me->m_bDetReachedGoal, me->m_bDetonating ,ENTINDEX(me->m_hTarget), me->m_vecDetonatePos.x);
		return result;
	}

	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMissionSuicideBomber_Update, CTFBot *actor, float dt)
	{
		auto me = reinterpret_cast<CTFBotMissionSuicideBomber *>(this);
		if (me->m_hTarget != nullptr && me->m_hTarget->IsAlive() && !me->m_hTarget->IsBaseObject()) {
			bool unreachable = PointInRespawnRoom(me->m_hTarget,me->m_hTarget->WorldSpaceCenter(), false);
			if (!unreachable) {
				CTFNavArea *area =  static_cast<CTFNavArea *>(TheNavMesh->GetNearestNavArea(actor->WorldSpaceCenter()));
				unreachable = area != nullptr && area->HasTFAttributes((TFNavAttributeType)(BLOCKED | RED_SPAWN_ROOM | BLUE_SPAWN_ROOM | NO_SPAWNING | RESCUE_CLOSET));
			}

			if (unreachable) {
				CTFPlayer *newtarget = actor->SelectRandomReachableEnemy();
				if (newtarget != nullptr && newtarget->GetAbsOrigin().IsValid() && ENTINDEX(newtarget) > 0) {
					me->m_hTarget = newtarget;
					me->m_vecTargetPos = newtarget->GetAbsOrigin();
					me->m_vecDetonatePos = newtarget->GetAbsOrigin();
				}
				else {
					me->m_hTarget = nullptr;
				}
			}
		}
		
		
		if (me->m_hTarget != nullptr && me->m_hTarget->IsAlive() && !me->m_hTarget->IsBaseObject() && RandomInt(0,2) == 0)
			me->m_nConsecutivePathFailures = 0;
		//DevMsg("\n[Update]\n");
		auto result = DETOUR_MEMBER_CALL(CTFBotMissionSuicideBomber_Update)(actor, dt);
		//DevMsg("reached goal %d,detonating %d,failures %d, %d %f\n",me->m_bDetReachedGoal, me->m_bDetonating ,me->m_nConsecutivePathFailures,ENTINDEX(me->m_hTarget), me->m_vecDetonatePos.x);
		return result;
	}

	DETOUR_DECL_MEMBER(CCaptureZone *, CTFBot_GetFlagCaptureZone)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		/* make Action PushToCapturePoint work on red MvM bots such that they
		 * will push to the bomb hatch */
		if (TFGameRules()->IsMannVsMachineMode() && bot->GetTeamNumber() != TF_TEAM_BLUE) {
			/* same as normal code, except we don't do the teamnum check */
			for (auto elem : ICaptureZoneAutoList::AutoList()) {
				auto zone = rtti_cast<CCaptureZone *>(elem);
				if (zone == nullptr) continue;
				
				return zone;
			}
			
			return nullptr;
		}
		
		return DETOUR_MEMBER_CALL(CTFBot_GetFlagCaptureZone)();
	}
	
	
	DETOUR_DECL_MEMBER(int, CTFGameRules_ApplyOnDamageModifyRules, CTakeDamageInfo& info, CBaseEntity *pVictim, bool b1)
	{
		auto data = GetDataForBot(pVictim);
		if (data != nullptr) {
			auto pTFWeapon = rtti_cast<CTFWeaponBase *>(info.GetWeapon());
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
	DETOUR_DECL_MEMBER(bool,CTFBotDeliverFlag_UpgradeOverTime, CTFBot *bot)
	{
		auto data = GetDataForBot(bot);
		if (data != nullptr && data->no_bomb_upgrade) {
			return false;
		}
		return DETOUR_MEMBER_CALL(CTFBotDeliverFlag_UpgradeOverTime)(bot);
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

	bool IsRangeLessThan( CBaseEntity *bot, const Vector &pos, float range)
	{
		Vector to = pos - bot->GetAbsOrigin();
		return to.IsLengthLessThan(range);
	}

	bool IsRangeGreaterThan( CBaseEntity *bot, const Vector &pos, float range)
	{
		Vector to = pos - bot->GetAbsOrigin();
		return to.IsLengthGreaterThan(range);
	}

	DETOUR_DECL_MEMBER(void, CTFBot_EquipBestWeaponForThreat, const CKnownEntity * threat)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		bool mannvsmachine = TFGameRules()->IsMannVsMachineMode();
		
		bool use_best_weapon = false;

		if (bot->EquipRequiredWeapon())
			return;
		
		auto data = GetDataForBot(bot);
		if (data != nullptr && data->use_best_weapon) {
			use_best_weapon = true;
		}

		if (use_best_weapon)
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);


		CTFWeaponBase *secondary = rtti_cast< CTFWeaponBase *>( bot->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );
		bool set = false;
		if (secondary != nullptr && threat && IsRangeLessThan(bot, *(threat->GetLastKnownPosition()), 800)) {
			if (secondary->HasAmmo() && (secondary->GetWeaponID() == TF_WEAPON_JAR || secondary->GetWeaponID() == TF_WEAPON_JAR_MILK || secondary->GetWeaponID() == TF_WEAPON_JAR_GAS || secondary->GetWeaponID() == TF_WEAPON_CLEAVER)) {
				bot->Weapon_Switch( secondary );	
				set = true;
			}
		}

		for (int i = 0; i < MAX_WEAPONS; i++) {
			CTFWeaponBase  *actionItem = rtti_cast< CTFWeaponBase *>( bot->GetWeapon( i ));
			if (actionItem != nullptr) {
				
				if (actionItem->GetWeaponID() == TF_WEAPON_GRAPPLINGHOOK && threat && (!bot->GetGrapplingHookTarget() || RandomFloat(0.0f,1.0f) > 0.05f || bot->GetGrapplingHookTarget()->IsPlayer()) &&
				 IsRangeGreaterThan(bot, *(threat->GetLastKnownPosition()), 200)) {
					bot->Weapon_Switch( actionItem );
					set = true;
				}
				else if (actionItem->GetWeaponID() == TF_WEAPON_SPELLBOOK && rtti_cast< CTFSpellBook* >( actionItem )->m_iSpellCharges > 0)
				{
					bot->Weapon_Switch( actionItem );	
					set = true;
				}
			}
		}

		if (!set)
			DETOUR_MEMBER_CALL(CTFBot_EquipBestWeaponForThreat)(threat);

		if (use_best_weapon)
			TFGameRules()->Set_m_bPlayingMannVsMachine(mannvsmachine);
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
		if ((ent->ClassMatches("tf_projectile_rocket") || ent->ClassMatches("tf_projectile_sentryrocket")) && (rocket = rtti_cast<CTFProjectile_Rocket *>(ent)) != nullptr) {
			auto data = GetDataForBot(rtti_cast<IScorer *>(rocket)->GetScorer());
			if (data != nullptr) {
				hr = &data->homing_rockets;
			}
		}
		
		if (hr == nullptr || !hr->enable) {
			DETOUR_MEMBER_CALL(CBaseEntity_PerformCustomPhysics)(pNewPosition, pNewVelocity, pNewAngles, pNewAngVelocity);
			return;
		}
		
		constexpr float physics_interval = 0.25f;
		if (gpGlobals->tickcount % (int)(physics_interval / gpGlobals->interval_per_tick) == 0) {
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
				QAngle angToTarget;
				VectorAngles(target_player->WorldSpaceCenter() - rocket->WorldSpaceCenter(), angToTarget);
				
				pNewAngVelocity->x = Clamp(Approach(AngleDiff(angToTarget.x, pNewAngles->x) * 4.0f, pNewAngVelocity->x, hr->turn_power), -360.0f, 360.0f);
				pNewAngVelocity->y = Clamp(Approach(AngleDiff(angToTarget.y, pNewAngles->y) * 4.0f, pNewAngVelocity->y, hr->turn_power), -360.0f, 360.0f);
				pNewAngVelocity->z = Clamp(Approach(AngleDiff(angToTarget.z, pNewAngles->z) * 4.0f, pNewAngVelocity->z, hr->turn_power), -360.0f, 360.0f);
			}
		}
		
		*pNewAngles += (*pNewAngVelocity * gpGlobals->frametime);
		
		Vector vecOrientation;
		AngleVectors(*pNewAngles, &vecOrientation);
		*pNewVelocity = vecOrientation * (1100.0f * hr->speed);
		
		*pNewPosition += (*pNewVelocity * gpGlobals->frametime);
		
	//	if (gpGlobals->tickcount % 2 == 0) {
	//		NDebugOverlay::EntityText(ENTINDEX(rocket), -2, CFmtStr("  AngVel: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewAngVelocity)), 0.030f);
	//		NDebugOverlay::EntityText(ENTINDEX(rocket), -1, CFmtStr("  Angles: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewAngles)),      0.030f);
	//		NDebugOverlay::EntityText(ENTINDEX(rocket),  1, CFmtStr("Velocity: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewVelocity)),    0.030f);
	//		NDebugOverlay::EntityText(ENTINDEX(rocket),  2, CFmtStr("Position: % 6.1f % 6.1f % 6.1f", VectorExpand(*pNewPosition)),    0.030f);
	//	}
		
	//	DevMsg("[%d] PerformCustomPhysics: #%d %s\n", gpGlobals->tickcount, ENTINDEX(ent), ent->GetClassname());
	}
	
	
//	// TEST! REMOVE ME!
//	// (for making human-model MvM bots use non-robot footstep sfx)
//	DETOUR_DECL_MEMBER(const char *, CTFPlayer_GetOverrideStepSound, const char *pszBaseStepSoundName)
//	{
//		DevMsg("CTFPlayer::OverrideStepSound(\"%s\")\n", pszBaseStepSoundName);
//		return pszBaseStepSoundName;
//	}
//	
//	// TEST! REMOVE ME!
//	// (for making human-model MvM bots use non-robot vo lines)
//	DETOUR_DECL_MEMBER(const char *, CTFPlayer_GetSceneSoundToken)
//	{
//		DevMsg("CTFPlayer::GetSceneSoundToken\n");
//		return "";
//	}
	
	

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

	ConVar improved_airblast    ("sig_bot_improved_airblast", "0", FCVAR_NOTIFY, "Bots can reflect grenades stickies and arrows, makes them aware of new JI airblast");

	CTFBot *bot_shouldfirecompressionblast = nullptr;
	RefCount rc_CTFBot_ShouldFireCompressionBlast;
	int bot_shouldfirecompressionblast_difficulty = 0;
	int totalok;
	int totaltries;
	DETOUR_DECL_MEMBER(bool, CTFBot_ShouldFireCompressionBlast)
	{
		SCOPED_INCREMENT(rc_CTFBot_ShouldFireCompressionBlast);

		if (!TFGameRules()->IsMannVsMachineMode() || !improved_airblast.GetBool())
			return DETOUR_MEMBER_CALL(CTFBot_ShouldFireCompressionBlast)();

		auto bot = reinterpret_cast<CTFBot *>(this);
		
		int difficulty = bot->m_nBotSkill;
		totaltries += 1;
		if ( difficulty == 0 )
		{
			return false;
		}

		if ( difficulty == 1 )
		{
			if ( bot->TransientlyConsistentRandomValue(1.0f, 0 ) < 0.65f )
			{
				return false;
			}
		}

		if ( difficulty == 2 )
		{
			if ( bot->TransientlyConsistentRandomValue(1.0f, 0 ) < 0.25f )
			{
				return false;
			}
		}
		totalok+=1;

		DevMsg("total ok: %d, total: %d",totalok, totaltries);
		Vector vecEye = bot->EyePosition();
		Vector vecForward, vecRight, vecUp;

		AngleVectors( bot->EyeAngles(), &vecForward, &vecRight, &vecUp );

		float radius = rtti_cast<CTFFlameThrower*>(bot->GetActiveTFWeapon())->GetDeflectionRadius();
		
		radius *= 0.55f + 0.1f * difficulty + (bot->IsMiniBoss() ? 0.1f : 0.0f);
		Vector vecCenter = vecEye + vecForward * radius;

		const int maxCollectedEntities = 128;
		CBaseEntity	*pObjects[ maxCollectedEntities ];
		
		CFlaggedEntitiesEnum iter = CFlaggedEntitiesEnum(pObjects, maxCollectedEntities, FL_CLIENT | FL_GRENADE );

		partition->EnumerateElementsInSphere(PARTITION_ENGINE_NON_STATIC_EDICTS, vecCenter, radius, false, &iter);
		//partition->EnumerateElementsInBox( PARTITION_ENGINE_NON_STATIC_EDICTS, vecCenter - vecSize, vecCenter + vecSize, false, &iter );
		int count = iter.GetCount();

		//DevMsg("Airblast size %f\n", radius);
		bool randomskip = difficulty < 3 && bot->TransientlyConsistentRandomValue( 1.0f, 165553463 ) < 0.5f;

		float minimal_dot = 0.705f * (1.3f -  0.07f * difficulty - (bot->IsMiniBoss() ? 0.09f : 0.0f));
		for ( int i = 0; i < count; i++ )
		{
			CBaseEntity *pObject = pObjects[i];
			if ( pObject == bot )
				continue;

			if ( pObject->GetTeamNumber() == bot->GetTeamNumber() )
				continue;

			// should air blast player logic is already done before this loop
			if ( pObject->IsPlayer() )
				continue;

			// is this something I want to deflect?
			if ( !pObject->IsDeflectable() )
				continue;
			

			float dot = DotProduct(vecForward.Normalized(), (pObject->WorldSpaceCenter() - vecEye).Normalized());
			//Rockets are more likely to be increased
			bool is_rocket = FStrEq(pObject->GetClassname(),"tf_projectile_rocket" ) || FStrEq(pObject->GetClassname(),"tf_projectile_energy_ball" );

			if ((!is_rocket && dot < minimal_dot) || (is_rocket && dot < minimal_dot * 0.75))
				continue;

			if ( randomskip && !( is_rocket))
			{
				continue;
			}

			// can I see it?
			bool blockslos = pObject->BlocksLOS();
			pObject->SetBlocksLOS(false);
			if ( !bot->GetVisionInterface()->IsLineOfSightClear( pObject->WorldSpaceCenter() + Vector(0,0,16) ) )
				continue;
			pObject->SetBlocksLOS(blockslos);

			// bounce it!
			return true;
		}

		return false;

		/*auto bot = reinterpret_cast<CTFBot *>(this);
		bot_shouldfirecompressionblast = bot;
		auto data = GetDataForBot(bot);
		if (data != nullptr) {
			bot_shouldfirecompressionblast_difficulty = data->difficulty;
			//DevMsg("skill %d\n", bot_shouldfirecompressionblast_difficulty);
		}
		bool ret = DETOUR_MEMBER_CALL(CTFBot_ShouldFireCompressionBlast)();
		bot_shouldfirecompressionblast = nullptr;

		return ret;*/
	}

	DETOUR_DECL_MEMBER(bool, IVision_IsLineOfSightClear, const Vector &pos)
	{
		if (rc_CTFBot_ShouldFireCompressionBlast) {
			Vector corrected = pos + Vector(0,0,16);
			if (RandomInt(0,1) == 0) {
				DevMsg("Unsee\n");
				return false;
			}
			bool ret = DETOUR_MEMBER_CALL(IVision_IsLineOfSightClear)(corrected);
			DevMsg("Can Reflect: %d \n", ret);
			return ret;
		}
		return DETOUR_MEMBER_CALL(IVision_IsLineOfSightClear)(pos);
	}
	
	DETOUR_DECL_MEMBER(void, CBaseProjectile_Spawn)
	{
		DETOUR_MEMBER_CALL(CBaseProjectile_Spawn)();
		auto projectile = reinterpret_cast<CBaseEntity *>(this);
		DevMsg("SetBlockLos\n");
	}

	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInSphere, SpatialPartitionListMask_t listMask, const Vector& origin, float radius, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		DevMsg("Radius: %f %f %f %f\n", radius, origin.x, origin.y, origin.z);
		DETOUR_MEMBER_CALL(ISpatialPartition_EnumerateElementsInSphere)(listMask, origin, radius, coarseTest, pIterator);
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

	DETOUR_DECL_MEMBER(Vector, CTFBotMainAction_SelectTargetPoint, const INextBot *nextbot, CBaseCombatCharacter *subject)
	{
		auto bot = ToTFBot(nextbot->GetEntity());

		if (bot != nullptr) {
			auto data = GetDataForBot(bot);

			if (data != nullptr) {

				Vector aim = subject->WorldSpaceCenter();
				
				auto weapon = bot->GetActiveTFWeapon();
				
				if (weapon != nullptr && weapon->m_iClip1 > 0 && weapon->m_flNextPrimaryAttack < gpGlobals->curtime && (data->rocket_jump_type == 1 || (data->rocket_jump_type == 2 && weapon->GetMaxClip1() == weapon->m_iClip1)) && !bot->m_Shared->InCond( TF_COND_BLASTJUMPING )) {
					
					bot->ReleaseFireButton();

					Vector dir = bot->GetAbsOrigin() - subject->GetAbsOrigin();

					if (bot->GetItem() != nullptr) {
						dir = -bot->GetAbsVelocity();
					}

					dir.z = 0;
					dir.NormalizeInPlace();

					Vector boteyes;
					AngleVectors(bot->EyeAngles(), &boteyes, NULL, NULL);
					boteyes.z = 0;
					boteyes.NormalizeInPlace();

					aim = bot->GetAbsOrigin() + dir * 28;
					if (DotProduct(dir,boteyes) > 0.88)
						bot->PressJumpButton(0.1f);

					if ((bot->GetFlags() & FL_ONGROUND) == 0) {
						bot->PressCrouchButton(0.1f);
					}
					if (DotProduct(dir,boteyes) > 0.94)
						bot->PressFireButton(0.1f);
					
					return aim;	
				}

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
					auto distance = nextbot->GetRangeTo(subject->GetAbsOrigin());

					float time_to_travel = distance / data->aim_lead_target_speed;

					Vector target_pos = aim + time_to_travel * subject->GetAbsVelocity();

					if (bot->GetVisionInterface()->IsAbleToSee( target_pos, IVision::FieldOfViewCheckType::DISREGARD_FOV))
						aim = target_pos;
				}
				return aim;
			}
		}
		return DETOUR_MEMBER_CALL(CTFBotMainAction_SelectTargetPoint)(nextbot,subject);
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

	DETOUR_DECL_MEMBER(float, CTFWeaponBaseGun_GetProjectileSpeed)
	{
		return 1.0f;
	}
	CBaseEntity *entityOnFireCollide;
	DETOUR_DECL_MEMBER(float, CTFFlameManager_GetFlameDamageScale, void * point, CTFPlayer * player)
	{
		float ret = DETOUR_MEMBER_CALL(CTFFlameManager_GetFlameDamageScale)(point, player);
		bool istank = entityOnFireCollide != nullptr && strncmp(entityOnFireCollide->GetClassname(),"ta",2) == 0; //classname is tank_boss, but there is only one targetable entity starting with ta
		if (istank) {
			ret+=0.04f;
			if (ret > 1.0f)
				ret = 1.0f;
			if (ret < 0.77f) {
				ret = 0.77f;
			}
		}
		return ret;
	}
	
	DETOUR_DECL_MEMBER(void, CTFFlameManager_OnCollide, CBaseEntity* entity, int value)
	{
		entityOnFireCollide = entity;
		DETOUR_MEMBER_CALL(CTFFlameManager_OnCollide)(entity, value);
		entityOnFireCollide = nullptr;
	}

	void AttackEnemyProjectiles( CTFPlayer *player, CTFWeaponBase *weapon, int shoot_projectiles)
	{

		const int nSweepDist = 300;	// How far out
		const int nHitDist = ( player->IsMiniBoss() ) ? 56 : 38;	// How far from the center line (radial)

		// Pos
		const Vector &vecGunPos = ( player->IsMiniBoss() ) ? player->Weapon_ShootPosition() : player->EyePosition();
		Vector vecForward;
		AngleVectors( weapon->GetAbsAngles(), &vecForward );
		Vector vecGunAimEnd = vecGunPos + vecForward * (float)nSweepDist;

		// Iterate through each grenade/rocket in the sphere
		const int maxCollectedEntities = 128;
		CBaseEntity	*pObjects[ maxCollectedEntities ];
		
		CFlaggedEntitiesEnum iter = CFlaggedEntitiesEnum(pObjects, maxCollectedEntities, FL_GRENADE );

		partition->EnumerateElementsInSphere(PARTITION_ENGINE_NON_STATIC_EDICTS, vecGunPos, nSweepDist, false, &iter);

		int count = iter.GetCount();

		for ( int i = 0; i < count; i++ )
		{
			if ( player->GetTeamNumber() == pObjects[i]->GetTeamNumber() )
				continue;

			// Hit?
			const Vector &vecGrenadePos = pObjects[i]->GetAbsOrigin();
			float flDistToLine = CalcDistanceToLineSegment( vecGrenadePos, vecGunPos, vecGunAimEnd );
			if ( flDistToLine <= nHitDist )
			{
				if ( player->FVisible( pObjects[i], MASK_SOLID ) == false )
					continue;

				if ( ( pObjects[i]->GetFlags() & FL_ONGROUND ) )
					continue;
					
				if ( !pObjects[i]->IsDeflectable() )
					continue;

				CBaseProjectile *pProjectile = rtti_cast< CBaseProjectile* >( pObjects[i] );
				if ( pProjectile && pProjectile->IsDestroyable() )
				{
					pProjectile->Destroy( false, true );

					weapon->EmitSound( "Halloween.HeadlessBossAxeHitWorld" );
				}
			}
		}
	}

	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFWeaponBaseGun_FireProjectile, CTFPlayer *player)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {

			int shoot_projectiles = 0;
			
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, shoot_projectiles, attack_projectiles);
			if (shoot_projectiles > 0) {
				auto weapon = reinterpret_cast<CTFWeaponBase*>(this);
				if (weapon->GetWeaponID() != TF_WEAPON_MINIGUN)
					AttackEnemyProjectiles(player, reinterpret_cast<CTFWeaponBase*>(this), shoot_projectiles);
			}
			auto data = GetDataForBot(player);
			if (data != nullptr) {
				bool stopproj = false;
				auto weapon = reinterpret_cast<CTFWeaponBase*>(this);
				for(auto it = data->shoot_templ.begin(); it != data->shoot_templ.end(); it++) {
					ShootTemplateData &temp_data = *it;
					
					if (temp_data.weapon != "" && FStrEq(weapon->GetItem()->GetStaticData()->GetName(), temp_data.weapon.c_str()))
						continue;

					if (temp_data.parent_to_projectile) {
						CBaseAnimating *proj = DETOUR_MEMBER_CALL(CTFWeaponBaseGun_FireProjectile)(player);
						if (proj != nullptr) {
							Vector vec = temp_data.offset;
							QAngle ang = temp_data.angles;
							PointTemplateInstance *inst = temp_data.templ->SpawnTemplate(proj, vec, ang, true, nullptr);
						}
						return proj;
					}

					Vector vecSrc;
					QAngle angForward;
					Vector vecOffset( 23.5f, 12.0f, -3.0f );
					vecOffset += temp_data.offset;
					if ( player->GetFlags() & FL_DUCKING )
					{
						vecOffset.z = 8.0f;
					}
					weapon->GetProjectileFireSetup( player, vecOffset, &vecSrc, &angForward, false ,2000);
					
					angForward += temp_data.angles;

					stopproj = stopproj || temp_data.override_shoot;

					PointTemplateInstance *inst = temp_data.templ->SpawnTemplate(player, vecSrc, angForward, false, nullptr);
					for (auto entity : inst->entities) {
						Vector vForward,vRight,vUp;
						QAngle angSpawnDir( angForward );
						AngleVectors( angSpawnDir, &vForward, &vRight, &vUp );
						Vector vecShootDir = vForward;
						vecShootDir += vRight * RandomFloat(-1, 1) * temp_data.spread;
						vecShootDir += vForward * RandomFloat(-1, 1) * temp_data.spread;
						vecShootDir += vUp * RandomFloat(-1, 1) * temp_data.spread;
						VectorNormalize( vecShootDir );
						vecShootDir *= temp_data.speed;

						// Apply it to the entity
						IPhysicsObject *pPhysicsObject = entity->VPhysicsGetObject();
						if ( pPhysicsObject )
						{
							pPhysicsObject->AddVelocity(&vecShootDir, NULL);
						}
						else
						{
							entity->SetAbsVelocity( vecShootDir );
						}
					}
				}
				if (stopproj) {
					player->DoAnimationEvent(0,0);
					return nullptr;
				}
			}
		}
		return DETOUR_MEMBER_CALL(CTFWeaponBaseGun_FireProjectile)(player);
	}

	DETOUR_DECL_MEMBER(void, CTFBotMainAction_FireWeaponAtEnemy, CTFBot *actor)
	{
		
		DETOUR_MEMBER_CALL(CTFBotMainAction_FireWeaponAtEnemy)(actor);
		auto data = GetDataForBot(actor);
		if (data != nullptr) {
			auto weapon = actor->GetActiveTFWeapon();
			if (weapon != nullptr) {
				if ((data->rocket_jump_type == 1 || (data->rocket_jump_type == 2 && weapon->GetMaxClip1() == weapon->m_iClip1)) && !actor->m_Shared->InCond( TF_COND_BLASTJUMPING )) {
					actor->ReleaseFireButton();
				}
			}
		}
	}

	DETOUR_DECL_MEMBER(bool, CTFBot_IsBarrageAndReloadWeapon, CTFWeaponBase *gun)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			int fire_when_full = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(gun, fire_when_full, auto_fires_full_clip);
			return fire_when_full == 0;
		}
		return DETOUR_MEMBER_CALL(CTFBot_IsBarrageAndReloadWeapon)(gun);
	}

	DETOUR_DECL_MEMBER(const char*, CTFWeaponBase_GetShootSound, int index)
	{
		if ((index == 1 || index == 5) && TFGameRules()->IsMannVsMachineMode()) {
			auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
			CTFBot *owner = ToTFBot(weapon->GetOwner());
			if (owner != nullptr) {
				auto data = GetDataForBot(owner);
				if (strlen(STRING(data->fire_sound)) > 0) {
					return STRING(data->fire_sound);
				}
			}
		}
		return DETOUR_MEMBER_CALL(CTFWeaponBase_GetShootSound)(index);
	}
	
#ifdef ENABLE_BROKEN_STUFF
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ShouldDropAmmoPack)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		auto data = GetDataForBot(player);
		if (data != nullptr) {
			if (data->drop_weapon) {
			//	DevMsg("ShouldDropAmmoPack[%s]: yep\n", player->GetPlayerName());
				return true;
		//	} else {
		//		DevMsg("ShouldDropAmmoPack[%s]: nope\n", player->GetPlayerName());
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_ShouldDropAmmoPack)();
	}
	
	RefCount rc_CTFPlayer_DropAmmoPack;
	DETOUR_DECL_MEMBER(void, CTFPlayer_DropAmmoPack, const CTakeDamageInfo& info, bool b1, bool b2)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_DropAmmoPack);
		DETOUR_MEMBER_CALL(CTFPlayer_DropAmmoPack)(info, b1, b2);
	}
	
	DETOUR_DECL_STATIC(CTFAmmoPack *, CTFAmmoPack_Create, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner, const char *pszModelName)
	{
		// basically re-implementing the logic we bypassed in CTFPlayer::ShouldDropAmmoPack
		// but in such a way that we only affect the actual ammo packs, not dropped weapons
		// (and actually we use GetTeamNumber rather than IsBot)
		if (rc_CTFPlayer_DropAmmoPack > 0 && TFGameRules()->IsMannVsMachineMode() && (pOwner == nullptr || pOwner->GetTeamNumber() == TF_TEAM_BLUE)) {
			return nullptr;
		}
		
		return DETOUR_STATIC_CALL(CTFAmmoPack_Create)(vecOrigin, vecAngles, pOwner, pszModelName);
	}
	
	DETOUR_DECL_STATIC(CTFDroppedWeapon *, CTFDroppedWeapon_Create, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner, const char *pszModelName, const CEconItemView *pItemView)
	{
		// this is really ugly... we temporarily override m_bPlayingMannVsMachine
		// because the alternative would be to make a patch
		
		bool is_mvm_mode = TFGameRules()->IsMannVsMachineMode();
		TFGameRules()->Set_m_bPlayingMannVsMachine(false);
		
		auto result = DETOUR_STATIC_CALL(CTFDroppedWeapon_Create)(vecOrigin, vecAngles, pOwner, pszModelName, pItemView);
		
		TFGameRules()->Set_m_bPlayingMannVsMachine(is_mvm_mode);
		
		return result;
	}

#endif
	
	
	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Pop:TFBot_Extensions")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_dtor0, "CTFBotSpawner::~CTFBotSpawner [D0]");
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_dtor2, "CTFBotSpawner::~CTFBotSpawner [D2]");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_dtor0, "CTFBot::~CTFBot [D0]");
			MOD_ADD_DETOUR_MEMBER(CTFBot_dtor2, "CTFBot::~CTFBot [D2]");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_StateEnter, "CTFPlayer::StateEnter");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_StateLeave, "CTFPlayer::StateLeave");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Parse, "CTFBotSpawner::Parse");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Spawn, "CTFBotSpawner::Spawn");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotScenarioMonitor_DesiredScenarioAndClassAction, "CTFBotScenarioMonitor::DesiredScenarioAndClassAction");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotScenarioMonitor_Update, "CTFBotScenarioMonitor::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBot_GetFlagToFetch,        "CTFBot::GetFlagToFetch");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsPlayerClass,      "CTFPlayer::IsPlayerClass");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_GetFlagCaptureZone, "CTFBot::GetFlagCaptureZone");
			
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageModifyRules, "CTFGameRules::ApplyOnDamageModifyRules");
			
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireRocket, "CTFWeaponBaseGun::FireRocket");
			
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ApplyOnHitAttributes, "CTFWeaponBase::ApplyOnHitAttributes");
			
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_Spawn,       "CTFProjectile_Rocket::Spawn");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PerformCustomPhysics, "CBaseEntity::PerformCustomPhysics");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_SelectMoreDangerousThreatInternal, "CTFBotMainAction::SelectMoreDangerousThreatInternal");
			
			//MOD_ADD_DETOUR_STATIC(FireEvent,           "FireEvent");
			
#ifdef ENABLE_BROKEN_STUFF
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldDropAmmoPack, "CTFPlayer::ShouldDropAmmoPack");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DropAmmoPack,       "CTFPlayer::DropAmmoPack");
			MOD_ADD_DETOUR_STATIC(CTFDroppedWeapon_Create,      "CTFDroppedWeapon::Create");
			MOD_ADD_DETOUR_STATIC(CTFAmmoPack_Create,           "CTFAmmoPack::Create");
#endif
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_OnStart,       "CTFBotMissionSuicideBomber::OnStart");
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_Update,        "CTFBotMissionSuicideBomber::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotDeliverFlag_UpgradeOverTime,        "CTFBotDeliverFlag::UpgradeOverTime");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DeathSound,        "CTFPlayer::DeathSound");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_PainSound,        "CTFPlayer::PainSound");
			MOD_ADD_DETOUR_MEMBER(CTFBot_EquipBestWeaponForThreat, "CTFBot::EquipBestWeaponForThreat");

			MOD_ADD_DETOUR_MEMBER(CTFBot_AddItem,     "CTFBot::AddItem");
			MOD_ADD_DETOUR_MEMBER(CTFItemDefinition_GetLoadoutSlot,     "CTFItemDefinition::GetLoadoutSlot");
			MOD_ADD_DETOUR_STATIC(CreateEntityByName, "CreateEntityByName");
			MOD_ADD_DETOUR_MEMBER(CItemGeneration_GenerateRandomItem,        "CItemGeneration::GenerateRandomItem");
			MOD_ADD_DETOUR_STATIC(ParseDynamicAttributes,         "ParseDynamicAttributes");
			//MOD_ADD_DETOUR_MEMBER(CEconItemSchema_GetItemDefinitionByName,        "CEconItemSchema::GetItemDefinitionByName");
			//MOD_ADD_DETOUR_MEMBER(CSchemaFieldHandle_CEconItemDefinition, "CSchemaFieldHandle<CEconItemDefinition>::CSchemaFieldHandle");
			//MOD_ADD_DETOUR_MEMBER(CSchemaFieldHandle_CEconItemDefinition2, "CSchemaFieldHandle<CEconItemDefinition>::CSchemaFieldHandle2");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_HandleCommand_JoinClass,        "CTFPlayer::HandleCommand_JoinClass");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsMiniBoss,        "CTFPlayer::IsMiniBoss");
			//MOD_ADD_DETOUR_MEMBER(CBaseCombatCharacter_SetBloodColor,        "CBaseCombatCharacter::SetBloodColor");
			//MOD_ADD_DETOUR_MEMBER(CBaseAnimating_SetModelScale,        "CBaseAnimating::SetModelScale");
			MOD_ADD_DETOUR_MEMBER(CTFBot_OnEventChangeAttributes,        "CTFBot::OnEventChangeAttributes");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_GetProjectileSpeed,        "CTFWeaponBaseGun::GetProjectileSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireProjectile,        "CTFWeaponBaseGun::FireProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFFlameManager_GetFlameDamageScale,        "CTFFlameManager::GetFlameDamageScale");
			MOD_ADD_DETOUR_MEMBER(CTFFlameManager_OnCollide,        "CTFFlameManager::OnCollide");
			MOD_ADD_DETOUR_MEMBER(CTFBot_IsBarrageAndReloadWeapon,        "CTFBot::IsBarrageAndReloadWeapon");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_GetShootSound,        "CTFWeaponBase::GetShootSound");
			//MOD_ADD_DETOUR_MEMBER(IVision_IsLineOfSightClear, "IVision::IsLineOfSightClear");
			MOD_ADD_DETOUR_MEMBER(CTFBot_ShouldFireCompressionBlast, "CTFBot::ShouldFireCompressionBlast");
			MOD_ADD_DETOUR_MEMBER(CBaseProjectile_Spawn, "CBaseProjectile::Spawn");
			MOD_ADD_DETOUR_MEMBER(CTFBotBody_GetHeadAimTrackingInterval, "CTFBotBody::GetHeadAimTrackingInterval");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_SelectTargetPoint, "CTFBotMainAction::SelectTargetPoint");
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_AvoidBumpingEnemies, "CTFBotTacticalMonitor::AvoidBumpingEnemies");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_FireWeaponAtEnemy, "CTFBotMainAction::FireWeaponAtEnemy");
			
			//MOD_ADD_DETOUR_MEMBER(GetWeaponId, "Global::GetWeaponID");
			//MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInSphere, "ISpatialPartition::EnumerateElementsInSphere");

			MOD_ADD_DETOUR_MEMBER(CTFBot_GetEventChangeAttributes,        "CTFBot::GetEventChangeAttributes");
			MOD_ADD_DETOUR_MEMBER(CTFBot_AddEventChangeAttributes,        "CTFBot::AddEventChangeAttributes");
			//MOD_ADD_DETOUR_MEMBER(CTFBot_StartIdleSound,        "CTFBot::StartIdleSound");
			//MOD_ADD_DETOUR_MEMBER(CTFBot_AddItem,        "CTFBot::AddItem");
			//MOD_ADD_DETOUR_MEMBER(CItemGeneration_GenerateRandomItem,        "CItemGeneration::GenerateRandomItem");
			// TEST! REMOVE ME!
//			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetOverrideStepSound, "CTFPlayer::GetOverrideStepSound");
//			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetSceneSoundToken,   "CTFPlayer::GetSceneSoundToken");
		}
		
		virtual void OnUnload() override
		{
			ClearAllData();
		}
		
		virtual void OnDisable() override
		{
			ClearAllData();
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void LevelInitPreEntity() override
		{
			ClearAllData();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			ClearAllData();
		}
		
		virtual void FrameUpdatePostEntityThink() override
		{
			UpdateDelayedAddConds();
			UpdatePeriodicTasks();
			UpdateRingOfFire();
			UpdateAlwaysGlow();
			//UpdatePyroAirblast();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_tfbot_extensions", "0", FCVAR_NOTIFY,
		"Mod: enable extended KV in CTFBotSpawner::Parse",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	class CKVCond_TFBot : public IKVCond
	{
	public:
		virtual bool operator()() override
		{
			return s_Mod.IsEnabled();
		}
	};
	CKVCond_TFBot cond;

	
}

/*
Current UseHumanModel mod:
- Fix voices
- Fix step sound

- Fix bullet impact sounds?
- Fix impact particles

- Fix idle sound
- Fix death sound
- Sentry buster model/blood
*/

// TODO: look for random one-off cases of MVM_ or M_MVM_ strings
// (e.g. engineer voice lines and stuff)

// voices:
// server detour of CTFPlayer::GetSceneSoundToken


/*bool TeleportNearVictim(CTFBot *spy, CTFPlayer *victim, int dist)
{
	VPROF_BUDGET("CTFBotSpyLeaveSpawnRoom::TeleportNearVictim", "NextBot");
	
	if (victim == nullptr || victim->GetLastKnownArea() == nullptr) {
		return false;
	}
	
	float dist_limit = Min((500.0f * dist) + 1500.0f, 6000.0f);
	
	CUtlVector<CTFNavArea *> good_areas;
	
	CUtlVector<CNavArea *> near_areas;
	
	float StepHeight = spy->GetLocomotionInterface()->GetStepHeight();
	CollectSurroundingAreas(&near_areas, victim->GetLastKnownArea(), dist_limit,
		StepHeight, StepHeight);
	
	FOR_EACH_VEC(near_areas, i) {
		CTFNavArea *area = static_cast<CTFNavArea *>(near_areas[i]);
		
		if (area->IsValidForWanderingPopulation() &&
			!area->IsPotentiallyVisibleToTeam(victim->GetTeamNumber())) {
			good_areas.AddToTail(area);
		}
	}
	
	int limit = Max(good_areas.Count(), 10);
	for (int i = 0; i < limit; ++i) {
		CTFNavArea *area = good_areas.Random();
		
		Vector pos = {
			.x = area->GetCenter().x,
			.y = area->GetCenter().y,
			.z = area->GetCenter().z + StepHeight,
		};
		
		if (IsSpaceToSpawnHere(pos)) {
			spy->Teleport(pos, vec3_angle, vec3_origin);
			return true;
		}
	}
	
	return false;
}
*/