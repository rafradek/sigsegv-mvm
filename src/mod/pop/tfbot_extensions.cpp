#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/particles.h"
#include "stub/populators.h"
#include "stub/projectiles.h"
#include "stub/objects.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/strings.h"
#include "util/scope.h"
#include "mod/pop/kv_conditional.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot_behavior.h"
#include "util/iterate.h"
#include "mod/pop/pointtemplate.h"
#include "mod/pop/common.h"
#include "stub/usermessages_sv.h"
#include "stub/trace.h"
#include "util/clientmsg.h"
#include "mod/pop/popmgr_extensions.h"
#include "stub/team.h"
#include <ctime>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>


static StaticFuncThunk<bool, CTFBot *, CTFPlayer *, int> ft_TeleportNearVictim  ("TeleportNearVictim");

bool TeleportNearVictim(CTFBot *spy, CTFPlayer *victim, int dist) {return ft_TeleportNearVictim(spy,victim,dist);};

namespace Mod::Pop::TFBot_Extensions
{

	std::unordered_map<CTFBot*, CBaseEntity*> targets_sentrybuster;

	/* mobber AI, based on CTFBotAttackFlagDefenders */
	class CTFBotMobber : public IHotplugAction<CTFBot>
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
			DevMsg("Remove mobber\n");
		}
		
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override
		{
			DevMsg("On end mobber\n");
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
					auto attrs = actor->ExtAttr();
					if (!attrs[CTFBot::ExtendedAttr::IGNORE_NPC]) {
						do {
							target_ent = servertools->FindEntityByClassname(target_ent, "tank_boss");

							if (target_ent != nullptr && target_ent->GetTeamNumber() != actor->GetTeamNumber()) {
								this->m_hTarget = target_ent->MyCombatCharacterPointer();
								break;
							}

						} while (target_ent != nullptr);
						
						target_ent = nullptr;
						do {
							target_ent = servertools->FindEntityByClassname(target_ent, "headless_hatman");

							if (target_ent != nullptr && target_ent->GetTeamNumber() != actor->GetTeamNumber()) {
								this->m_hTarget = target_ent->MyCombatCharacterPointer();
								break;
							}

						} while (target_ent != nullptr);
					}
				}
				
				if (this->m_hTarget == nullptr) {
					CBaseEntity *target_ent = nullptr;
					auto attrs = actor->ExtAttr();
					std::vector<CBaseObject *> objs;
					if (!attrs[CTFBot::ExtendedAttr::IGNORE_BUILDINGS]) {
						for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
							auto obj = rtti_scast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
							if (obj != nullptr && obj->GetType() == OBJ_SENTRYGUN && obj->IsTargetable() && obj->GetTeamNumber() != actor->GetTeamNumber()) {
								objs.push_back(obj);
							}
						}
					}
					if (!objs.empty()) {
						this->m_hTarget = objs[RandomInt(0, objs.size()-1)];
					}
				}
				if (this->m_hTarget == nullptr) {
					return ActionResult<CTFBot>::Continue();
				}
			}
			
			actor->GetVisionInterface()->AddKnownEntity(this->m_hTarget);
			
			auto nextbot = actor->MyNextBotPointer();
			
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

	class CTFBotPassive : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotPassive()
		{
			this->m_Attack = CTFBotAttack::New();
		}
		virtual ~CTFBotPassive()
		{
			if (this->m_Attack != nullptr) {
				delete this->m_Attack;
			}
			DevMsg("Remove mobber\n");
		}
		
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override
		{
			DevMsg("On end mobber\n");
		}

		virtual const char *GetName() const override { return "Passive"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
			
			this->m_hTarget = nullptr;
			
			return this->m_Attack->OnStart(actor, action);
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			actor->GetVisionInterface()->Update();
			const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
			if (threat != nullptr) {
				actor->EquipBestWeaponForThreat(threat);
			}
			
			ActionResult<CTFBot> result = this->m_Attack->Update(actor, dt);
			if (result.transition != ActionTransition::DONE) {
				return ActionResult<CTFBot>::Continue();
			}
			
			if (threat == nullptr || threat->IsObsolete() || threat->GetEntity()->MyCombatCharacterPointer() == nullptr) {
				return ActionResult<CTFBot>::Continue();
			}
			
			auto nextbot = actor->MyNextBotPointer();
			
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
				
				CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
				this->m_PathFollower.Compute(nextbot, threat->GetEntity()->MyCombatCharacterPointer(), cost_func, 0.0f, true);
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
		ACTION_EscortFlag,
		ACTION_Idle,
		
		// custom
		ACTION_Mobber,
		ACTION_Passive,
	};

    class TeleporterModule : public EntityModule
    {
    public:
        TeleporterModule(CBaseEntity *entity) : EntityModule(entity) {}

        std::unordered_set<std::string> teleportWhere;
    };

	class SpawnerData
	{
	public:
		std::vector<AddCond> addconds;

		std::vector<std::shared_ptr<PeriodicTask>> periodic_tasks;
		
		bool force_romevision_cosmetics = false;

		//string_t fire_sound = MAKE_STRING("");
		
		ActionType action = ACTION_Default;
		
		bool suppress_timed_fetchflag = false;

		std::vector<PointTemplateInfo> templ;

		bool neutral = false;
//#ifdef ENABLE_BROKEN_STUFF
		bool drop_weapon = false;
//#endif
		bool no_wait_for_formation = false;
		bool no_formation = false;

		bool no_idle_sound = false;

		bool prefer_extra_slots = false;
	};

	std::unordered_map<CTFBotSpawner *, SpawnerData> spawners;
	
	std::map<CHandle<CTFBot>, CTFBotSpawner *> spawner_of_bot;

	std::unordered_map<CTFBot *, SpawnerData *> bots_data;
	
	std::vector<DelayedAddCond> delayed_addconds;
	//std::vector<CHandle<CTFBot>> spawned_bots_first_tick;

	std::vector<PeriodicTaskImpl> pending_periodic_tasks;

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

	void ClearAllData()
	{
		spawners.clear();
		spawner_of_bot.clear();
		bots_data.clear();
		delayed_addconds.clear();
		pending_periodic_tasks.clear();
		targets_sentrybuster.clear();
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
		//auto data = spawners[spawner];
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
		auto data = bots_data[bot];

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

		// Revert player anims on bots to normal
		if (player->StateGet() == TF_STATE_ACTIVE) {
			auto data = GetDataForBot(ToTFBot(player));
		}

		// Force drop all picked bot weapons
		for(int i = 0; i < player->WeaponCount(); i++ ) {
			CBaseCombatWeapon *weapon = player->GetWeapon(i);
			if (weapon == nullptr) continue;

			int droppedWeapon = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, droppedWeapon, is_dropped_weapon);
			
			if (droppedWeapon != 0) {
				weapon->Remove();
			}
		}

		if (player->StateGet() == TF_STATE_WELCOME || player->StateGet() == TF_STATE_OBSERVER) {
			CTFBot *bot = ToTFBot(player);
			if (bot != nullptr) {
			//	DevMsg("Bot #%d [\"%s\"]: StateLeave %s, clearing data\n", ENTINDEX(bot), bot->GetPlayerName(), GetStateName(bot->StateGet()));
				ClearDataForBot(bot);
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayer_StateLeave)();
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
		//} else if (FStrEq(value, "Medic")) {
		//	spawners[spawner].action = ACTION_MedicHeal;
		} else if (FStrEq(value, "Sniper")) {
			spawners[spawner].action = ACTION_SniperLurk;
		} else if (FStrEq(value, "SuicideBomber")) {
			spawners[spawner].action = ACTION_DestroySentries;
		} else if (FStrEq(value, "EscortFlag")) {
			spawners[spawner].action = ACTION_EscortFlag;
		} else if (FStrEq(value, "Idle")) {
			spawners[spawner].action = ACTION_Idle;
		} else if (FStrEq(value, "Passive")) {
			spawners[spawner].action = ACTION_Passive;
		} else {
			Warning("Unknown value \'%s\' for TFBot Action.\n", value);
		}
		DevMsg("Parse action %d %s \n", spawners[spawner].action, value); 
	}
	
	void Parse_EventChangeAttributesSig(CTFBotSpawner *spawner, KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, eventkv) {
			const char *name = eventkv->GetName();
			
			int index = -1;
			bool create_new = false;
			for(int i = 0; i < spawner->m_ECAttrs.Count(); i++) {

				if (FStrEq(spawner->m_ECAttrs[i].m_strName, name)) {
					index = i;
					break;
				}
			}

			if (index == -1) {
				create_new = true;
				index = spawner->m_ECAttrs.AddToTail();
			}

			CTFBot::EventChangeAttributes_t &event = spawner->m_ECAttrs[index];
			
			if (create_new || FStrEq(name, "default")) {
				event = spawner->m_DefaultAttrs;
			}

			FOR_EACH_SUBKEY(eventkv, subkey) {
				ParseDynamicAttributes(event, subkey);
			}
			
			if (FStrEq(name, "default")) {
				spawner->m_DefaultAttrs = event;
			}
		}
	}
	

	GlobalThunk<CHandle<CBaseEntity>> s_lastTeleporter("s_lastTeleporter");
	bool FindTeleporter(const char *name, Vector &vSpawnPosition, bool tryAll)
	{
		std::vector<CBaseEntity *> objectsToUse;
		for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
			CBaseObject *obj = rtti_scast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
			if (obj->GetType() != OBJ_TELEPORTER) continue;
			if (!obj->IsFunctional()) continue;

			auto mod = obj->GetEntityModule<TeleporterModule>("teleportwhere");
			if (mod != nullptr) {
				
				if (mod->teleportWhere.contains(name) || (tryAll && mod->teleportWhere.contains("all"))) {
					objectsToUse.push_back(obj);
				}
			}
		}

		if (!objectsToUse.empty()) {
			auto destination = objectsToUse[RandomInt(0, objectsToUse.size() - 1)];
			vSpawnPosition = destination->WorldSpaceCenter();
			vSpawnPosition.z += destination->CollisionProp()->OBBMaxs().z;
			s_lastTeleporter.GetRef() = destination;
			return true;
		}
		return false;
	}

	CTFBotSpawner *current_spawner = nullptr;
	KeyValues *current_spawner_kv = nullptr;
	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_Parse, KeyValues *kv_orig)
	{
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		
		current_spawner = spawner;
		current_spawner_kv = kv_orig;
		// make a temporary copy of the KV subtree for this spawner
		// the reason for this: `kv_orig` *might* be a ptr to a shared template KV subtree
		// we'll be deleting our custom keys after we parse them so that the Valve code doesn't see them
		// but we don't want to delete them from the shared template KV subtree (breaks multiple uses of the template)
		// so we use this temp copy, delete things from it, pass it to the Valve code, then delete it
		// (we do the same thing in Pop:WaveSpawn_Extensions)
		KeyValues *kv = kv_orig->MakeCopy();
		
	//	DevMsg("CTFBotSpawner::Parse\n");
		
		std::vector<KeyValues *> del_kv;

		// Merge keys from AddTemplate templates first
		KeyValues *addtemplate = nullptr;
		while ((addtemplate = kv->FindKey("AddTemplate")) != nullptr) {
		
			KeyValues *templates = g_pPopulationManager->m_pTemplates;
			if (templates != nullptr) {
				KeyValues *tmpl = templates->FindKey(addtemplate->GetString());
				if (tmpl != nullptr) {
					KeyValues *next = addtemplate->GetNextKey();
					KeyValues *copy = tmpl->GetFirstSubKey()->MakeCopy(true);
					addtemplate->SetNextKey(copy);
					KeyValues *copyNext = copy;
					while (copyNext->GetNextKey() != nullptr) {
						copyNext = copyNext->GetNextKey();
					}
					copyNext->SetNextKey(next);
				}
				else {
					Warning("CTFBotSpawner: Template %s not found\n", addtemplate->GetString());
				}
				kv->RemoveSubKey(addtemplate);
				addtemplate->deleteThis();
			}
			else {
				kv->RemoveSubKey(addtemplate);
				addtemplate->deleteThis();
			}
		}
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			bool del = true;
			if (FStrEq(name, "Template")) {
				if (g_pPopulationManager->m_pTemplates == nullptr) {
					Warning("No Templates in WaveSchedule. Did you forget to include robot_standard.pop ?\n");
				}
				else {
					del = false;
				}
			} else if (FStrEq(name, "AddCond")) {
				Parse_AddCond(spawners[spawner].addconds, subkey);
			} else if (Parse_PeriodicTask(spawners[spawner].periodic_tasks, subkey, name)) {

			} else if (FStrEq(name, "Action")) {
				Parse_Action(spawner, subkey);
			} else if (FStrEq(name, "EventChangeAttributesSig")) {
				Parse_EventChangeAttributesSig(spawner, subkey);
			} else if (FStrEq(name, "ForceRomeVision")) {
				spawners[spawner].force_romevision_cosmetics = subkey->GetBool();
			} else if (FStrEq(name, "SuppressTimedFetchFlag")) {
				spawners[spawner].suppress_timed_fetchflag = subkey->GetBool();
			} else if (FStrEq(name, "SpawnTemplate")) {
				spawners[spawner].templ.push_back(Parse_SpawnTemplate(subkey));
			} else if (FStrEq(name, "Neutral")) {
				spawners[spawner].neutral = subkey->GetBool();
			} else if (FStrEq(name, "NoIdleSound")) {
				spawners[spawner].no_idle_sound = subkey->GetBool();
			} else if (FStrEq(name, "PreferExtraSlots")) {
				spawners[spawner].prefer_extra_slots = subkey->GetBool();
//#ifdef ENABLE_BROKEN_STUFF
			} else if (FStrEq(name, "DropWeapon")) {
				spawners[spawner].drop_weapon = subkey->GetBool();
//#endif
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
			if (spawners[spawner].action == ACTION_Mobber || spawners[spawner].action == ACTION_Passive) {
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
		actor->m_Shared->AddCond(TF_COND_STEALTHED_USER_BUFF, 2.0f);
				
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

	StaticFuncThunk<void *, CTFBot *> ft_OnBotTeleported("OnBotTeleported");
	// clock_t start_time_spawn;
	void OnBotSpawn(CTFBotSpawner *spawner, CUtlVector<CHandle<CBaseEntity>> *ents, SpawnerData &data) {
		
		
		// clock_t endn = clock() ;
		// float timespent = ((endn-start) / (float)CLOCKS_PER_SEC);
		// DevMsg("native spawning took %f\n",timespent);
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
		
		if (ents != nullptr && !ents->IsEmpty()) {
			CTFBot *bot_leader = ToTFBot(ents->Head());
			CTFBot *bot = ToTFBot(ents->Tail());
			if (bot != nullptr) {
				spawner_of_bot[bot] = spawner;
				bots_data[bot] = &data;
				
			//	DevMsg("CTFBotSpawner %08x: found %u AddCond's\n", (uintptr_t)spawner, data.addconds.size());
				ApplyAddCond(bot, data.addconds, delayed_addconds);
				ApplyPendingTask(bot, data.periodic_tasks, pending_periodic_tasks);

				for (auto templ : data.templ) {
					templ.SpawnTemplate(bot);
					//if (Point_Templates().find(templ) != Point_Templates().end())
					//	Point_Templates()[templ].SpawnTemplate(bot);
				}

				if (data.force_romevision_cosmetics) {
					for (int i = 0; i < 2; i++) {
						//CEconItemView *item_view= CEconItemView::Create();
						//item_view->Init(152, 6, 9999, 0);
						
						CEconWearable *wearable = static_cast<CEconWearable *>(ItemGeneration()->SpawnItem(152, Vector(0,0,0), QAngle(0,0,0), 6, 9999, "tf_wearable"));
						if (wearable) {
							
							wearable->m_bValidatedAttachedEntity = true;
							wearable->GiveTo(bot);
							DevMsg("Created wearable %d\n",bot->GetPlayerClass()->GetClassIndex()*2 + i);
							bot->EquipWearable(wearable);
							const char *path = ROMEVISON_MODELS[bot->GetPlayerClass()->GetClassIndex()*2 + i];
							int model_index = CBaseEntity::PrecacheModel(path);
							wearable->SetModelIndex(model_index);
							for (int j = 0; j < MAX_VISION_MODES; ++j) {
								wearable->SetModelIndexOverride(j, model_index);
							}
						}
					}
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
				
				if (!(bot->m_nBotAttrs & CTFBot::AttributeType::ATTR_TELEPORT_TO_HINT)) {
					bool done = false;
					CBaseEntity *destination = nullptr;
					Vector spawnPosition;
					if ((!(bot_leader != nullptr && bot_leader->IsMiniBoss()) && FindTeleporter("small", spawnPosition, false)) || ((bot_leader != nullptr && bot_leader->IsMiniBoss()) && FindTeleporter("giants", spawnPosition, false)) ) {
						bool is_space_to_spawn = IsSpaceToSpawnHere(spawnPosition);
						if (!is_space_to_spawn)
							spawnPosition.z += 50.0f;
						if (is_space_to_spawn || IsSpaceToSpawnHere(spawnPosition)){
							bot->Teleport(&(spawnPosition),&vec3_angle,&(bot->GetAbsVelocity()));
							ft_OnBotTeleported(bot);
						}
					}
					
				}

				if (bot->GetPlayerClass()->GetClassIndex() != TF_CLASS_ENGINEER && (bot->m_nBotAttrs & CTFBot::AttributeType::ATTR_TELEPORT_TO_HINT))
					TeleportToHint(bot, data.action != ACTION_Default);

				if (data.action == ACTION_BotSpyInfiltrate) {
					SpyInitAction(bot);
				}
				
				//DevMsg("Client get pre %s\n", bot->GetPlayerName());
				//DevMsg("Client setting user info changed\n");

				//reinterpret_cast<CBaseServer *>(sv)->UserInfoChanged(client->GetPlayerSlot());
				//DevMsg("Client success\n");
				
				//player_info_t *pi = (player_info_t*) sv->GetUserInfoTable()->GetStringUserData( ENTINDEX(bot), NULL );
				//spawned_bots_first_tick.push_back(bot);

				if (data.neutral) {
					bot->SetTeamNumber(TEAM_SPECTATOR);

					ForEachTFPlayerEconEntity(bot, [&](CEconEntity *entity) {
						entity->ChangeTeam(TEAM_SPECTATOR);
					});
				}
				
				if (data.prefer_extra_slots) {
					// Swap the team vector contents back
					auto team = TFTeamMgr()->GetTeam(TEAM_SPECTATOR);
					auto &vec = team->m_aPlayers.Get();
					int count = vec.Count();
					for (int i = 0; i < count/2; i++) {
						auto swap = vec[count - 1 - i];
						vec[count - 1 - i] = vec[i];
						vec[i] = swap;
					}
				}

				/*for (int i = 0; i < bot->WeaponCount(); i++) {
					CBaseCombatWeapon *weapon = bot->GetWeapon(i);
					if (weapon != nullptr) {
						bot->Weapon_Switch(weapon);

						//DevMsg("Is active %d %d %d\n", weapon == player->GetActiveWeapon(), weapon->GetEffects(), weapon->GetRenderMode());
					}
				}*/
			}
		}
		
		//clock_t end = clock() ;
		//timespent = ((end-endn) / (float)CLOCKS_PER_SEC);
		//DevMsg("detour spawning took %f %d\n",timespent, spawned_bots_first_tick.capacity());
	}

	DETOUR_DECL_STATIC(int, DoTeleporterOverride, CBaseEntity *spawnEnt, Vector& vSpawnPosition, bool bClosestPointOnNav)
	{
		if (FindTeleporter(STRING(spawnEnt->GetEntityName()), vSpawnPosition, true)) return SPAWN_LOCATION_TELEPORTER;
		
		return DETOUR_STATIC_CALL(DoTeleporterOverride)(spawnEnt, vSpawnPosition, bClosestPointOnNav);
	}

	RefCount rc_CTFBotSpawner_Spawn;
	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		current_spawner = spawner;

		auto it = spawners.find(spawner);
		if (it != spawners.end()) {
			SpawnerData &data = (*it).second;
			if (data.prefer_extra_slots) {
				// Swap the team vector contents to use extra slots instead of normal
				auto team = TFTeamMgr()->GetTeam(TEAM_SPECTATOR);
				auto &vec = team->m_aPlayers.Get();
				int count = vec.Count();
				for (int i = 0; i < count/2; i++) {
					auto swap = vec[count - 1 - i];
					vec[count - 1 - i] = vec[i];
					vec[i] = swap;
				}
			}
		}
		auto result = DETOUR_MEMBER_CALL(CTFBotSpawner_Spawn)(where, ents);
		if (result && it != spawners.end()) {
			OnBotSpawn(spawner,ents, (*it).second);
		}
		return result;
	}
	int paused_wave_time = -1;
	DETOUR_DECL_MEMBER(bool, CTFGameRules_ClientConnected, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
	{
		auto gamerules = reinterpret_cast<CTFGameRules *>(this);
		
		Msg("Client connect, pausing wave\n");
		if (g_pPopulationManager != nullptr)
			g_pPopulationManager->PauseSpawning();
		paused_wave_time = gpGlobals->tickcount;
		
		return DETOUR_MEMBER_CALL(CTFGameRules_ClientConnected)(pEntity, pszName, pszAddress, reject, maxrejectlen);
	}

	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotScenarioMonitor_DesiredScenarioAndClassAction, CTFBot *actor)
	{
		auto data = GetDataForBot(actor);
		if (data != nullptr) {
			switch (data->action) {
			
			case ACTION_Default:
				break;
			
			case ACTION_EscortFlag:
			case ACTION_FetchFlag:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to FetchFlag\n", ENTINDEX(actor), actor->GetPlayerName());
				return CTFBotFetchFlag::New();
			
			case ACTION_PushToCapturePoint:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to PushToCapturePoint[-->FetchFlag]\n", ENTINDEX(actor), actor->GetPlayerName());
				return CTFBotPushToCapturePoint::New(CTFBotFetchFlag::New());
			
			case ACTION_Mobber:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to Mobber\n", ENTINDEX(actor), actor->GetPlayerName());
				return new CTFBotMobber();

			case ACTION_Passive:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to Passive\n", ENTINDEX(actor), actor->GetPlayerName());
				return new CTFBotPassive();

			case ACTION_BotSpyInfiltrate:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to SpyInfiltrate\n", ENTINDEX(actor), actor->GetPlayerName());
				return CTFBotSpyInfiltrate::New();
			case ACTION_SniperLurk:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to Sniper\n", ENTINDEX(actor), actor->GetPlayerName());
				actor->SetMission(CTFBot::MISSION_SNIPER);
				break;
			case ACTION_Idle:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to Idle\n", ENTINDEX(actor), actor->GetPlayerName());
				return nullptr;
			//case ACTION_MedicHeal:
			//	DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to MedicHeal\n", ENTINDEX(actor), actor->GetPlayerName());
			//	return CTFBotMedicHeal::New();
			case ACTION_DestroySentries:
				DevMsg("CTFBotSpawner: setting initial action of bot #%d \"%s\" to DestroySentries\n", ENTINDEX(actor), actor->GetPlayerName());
				CBaseEntity *target = actor->SelectRandomReachableEnemy();
					
				if (target == nullptr) {
					CBaseEntity *target = servertools->FindEntityByClassname(nullptr, "obj_sentrygun");
				}
				targets_sentrybuster[actor]=target;
				//if (target != nullptr) {
					//float m_flScale; // +0x2bf4
					
					//float scale = *(float *)((uintptr_t)actor+ 0x2bf4);
					//*(CHandle<CBaseEntity>*)((uintptr_t)actor+ 0x2c00) = target;
				//}
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
		if (actor->HasItem()) return;

		auto data = GetDataForBot(actor);

		if (data != nullptr && data->action != ACTION_Default) return;

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
		
		SCOPED_INCREMENT(rc_CTFBot_GetFlagToFetch);
		
		auto data = GetDataForBot(bot);
		if (data != nullptr && rc_CTFBotScenarioMonitor_Update > 0 && data->suppress_timed_fetchflag) {
			return nullptr;
		}
		/* disable the implicit "Attributes IgnoreFlag" thing given to
		* engineer bots if they have one of our Action overrides
		* enabled (the pop author can explicitly give the engie bot
		* "Attributes IgnoreFlag" if they want, of course)
		* NOTE: this logic will NOT take effect if the bot also has the
		* SuppressTimedFetchFlag custom parameter */
		bool IsEngineer = data != nullptr && data->action != ACTION_Default && bot->IsPlayerClass(TF_CLASS_ENGINEER);
		if (IsEngineer) {
			bot->GetPlayerClass()->SetClassIndex(TF_CLASS_SOLDIER);
		}
		auto result = DETOUR_MEMBER_CALL(CTFBot_GetFlagToFetch)();
		if (IsEngineer) {
			bot->GetPlayerClass()->SetClassIndex(TF_CLASS_ENGINEER);
		}
		
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
			else
			{
				me->m_bDetReachedGoal = true;
				me->m_vecTargetPos = actor->GetAbsOrigin();
				me->m_vecDetonatePos = actor->GetAbsOrigin();
			}
			targets_sentrybuster.erase(actor);
		}
		DevMsg("reached goal %d,detonating %d, %d %f\n",me->m_bDetReachedGoal, me->m_bDetonating ,ENTINDEX(me->m_hTarget), me->m_vecDetonatePos.x);
		return result;
	}

	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMissionSuicideBomber_Update, CTFBot *actor, float dt)
	{
		auto me = reinterpret_cast<CTFBotMissionSuicideBomber *>(this);
		//DevMsg("Bomberupdate %d %d\n", me->m_hTarget != nullptr, me->m_hTarget != nullptr && me->m_hTarget->IsAlive() && !me->m_hTarget->IsBaseObject());
		if (me->m_hTarget != nullptr && me->m_hTarget->IsAlive() && !me->m_hTarget->IsBaseObject()) {
			bool unreachable = PointInRespawnRoom(me->m_hTarget,me->m_hTarget->WorldSpaceCenter(), false);
			if (!unreachable) {
				
				CTFNavArea *area =  static_cast<CTFNavArea *>(TheNavMesh->GetNearestNavArea(me->m_hTarget->WorldSpaceCenter()));
				unreachable = area == nullptr || area->HasTFAttributes((TFNavAttributeType)(BLOCKED | RED_SPAWN_ROOM | BLUE_SPAWN_ROOM | NO_SPAWNING | RESCUE_CLOSET));
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
		//DevMsg("reached goal %d,detonating %d,failures %d, %d %f %f\n",me->m_bDetReachedGoal, me->m_bDetonating ,me->m_nConsecutivePathFailures,ENTINDEX(me->m_hTarget), me->m_vecDetonatePos.x, me->m_vecTargetPos.x);
		auto result = DETOUR_MEMBER_CALL(CTFBotMissionSuicideBomber_Update)(actor, dt);
		
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
				auto zone = rtti_scast<CCaptureZone *>(elem);
				if (zone == nullptr) continue;
				
				return zone;
			}
			
			return nullptr;
		}
		
		return DETOUR_MEMBER_CALL(CTFBot_GetFlagCaptureZone)();
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

		if (bot->EquipRequiredWeapon())
			return;

		CTFWeaponBase *secondary = static_cast< CTFWeaponBase *>( bot->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );
		bool set = false;
		if (secondary != nullptr && threat && IsRangeLessThan(bot, *(threat->GetLastKnownPosition()), 800)) {
			if (secondary->HasAmmo() && (secondary->GetWeaponID() == TF_WEAPON_JAR || secondary->GetWeaponID() == TF_WEAPON_JAR_MILK || secondary->GetWeaponID() == TF_WEAPON_JAR_GAS || secondary->GetWeaponID() == TF_WEAPON_CLEAVER)) {
				bot->Weapon_Switch( secondary );	
				set = true;
			}
		}

		for (int i = 0; i < MAX_WEAPONS; i++) {
			CTFWeaponBase  *actionItem = static_cast< CTFWeaponBase *>( bot->GetWeapon( i ));
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

		// CTFFlameThrower weapon class is guaranteed;
		float radius = static_cast<CTFFlameThrower*>(bot->GetActiveTFWeapon())->GetDeflectionRadius();
		
		//60% - 84% airblast radius depending on skill, +10% for giants
		radius *= 0.48f + 0.12f * difficulty + (bot->IsMiniBoss() ? 0.1f : 0.0f);
		Vector vecCenter = vecEye + vecForward * radius;

		const int maxCollectedEntities = 128;
		CBaseEntity	*pObjects[ maxCollectedEntities ];
		
		CFlaggedEntitiesEnum iter = CFlaggedEntitiesEnum(pObjects, maxCollectedEntities, FL_CLIENT | FL_GRENADE );

		partition->EnumerateElementsInSphere(PARTITION_ENGINE_NON_STATIC_EDICTS, vecCenter, radius, false, &iter);
		int count = iter.GetCount();

		// Random chance to not airblast non rocket entities on difficulties that are not expert
		bool randomskip = difficulty < 3 && bot->TransientlyConsistentRandomValue( 1.0f, 165553463 ) < 0.5f;

		// 30 - 45 degress airblast cone depending on difficulty
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
		//DevMsg("SetBlockLos\n");
	}

	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInSphere, SpatialPartitionListMask_t listMask, const Vector& origin, float radius, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		DevMsg("Radius: %f %f %f %f\n", radius, origin.x, origin.y, origin.z);
		DETOUR_MEMBER_CALL(ISpatialPartition_EnumerateElementsInSphere)(listMask, origin, radius, coarseTest, pIterator);
	}
	

	DETOUR_DECL_MEMBER(float, CTFWeaponBaseGun_GetProjectileSpeed)
	{
		return 1.0f;
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

	bool rc_CTFBotMainAction_Update = false;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		rc_CTFBotMainAction_Update = false;

		if (actor->GetTeamNumber() == TEAM_SPECTATOR && actor->StateGet() == TF_STATE_ACTIVE) {
			rc_CTFBotMainAction_Update = true;
			actor->SetTeamNumber(TF_TEAM_BLUE);
		}
		return DETOUR_MEMBER_CALL(CTFBotMainAction_Update)(actor, dt);
	}
	
	ConVar sig_no_bot_partner_taunt("sig_no_bot_partner_taunt", "1", FCVAR_NONE, "Disable bots answering to partner taunts");
	DETOUR_DECL_MEMBER(CTFPlayer *, CTFPlayer_FindPartnerTauntInitiator)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (rc_CTFBotMainAction_Update) {
			player->SetTeamNumber(TEAM_SPECTATOR);
		}
		rc_CTFBotMainAction_Update = false;

		if (sig_no_bot_partner_taunt.GetBool() && player->IsBot())
			return nullptr;

		return DETOUR_MEMBER_CALL(CTFPlayer_FindPartnerTauntInitiator)();
	}

	RefCount rc_CTFBotEscortFlagCarrier_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotEscortFlagCarrier_Update, CTFBot *actor, float dt)
	{
		auto data = GetDataForBot(actor);
		SCOPED_INCREMENT_IF(rc_CTFBotEscortFlagCarrier_Update, data != nullptr && data->action == ACTION_EscortFlag);
		return DETOUR_MEMBER_CALL(CTFBotEscortFlagCarrier_Update)(actor, dt);
	}

	RefCount rc_CTFBotAttackFlagDefenders_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotAttackFlagDefenders_Update, CTFBot *actor, float dt)
	{
		auto data = GetDataForBot(actor);
		SCOPED_INCREMENT_IF(rc_CTFBotAttackFlagDefenders_Update, data != nullptr && data->action == ACTION_EscortFlag);
		return DETOUR_MEMBER_CALL(CTFBotAttackFlagDefenders_Update)(actor, dt);
	}

	DETOUR_DECL_STATIC(int, GetBotEscortCount, int team)
	{
		if (rc_CTFBotEscortFlagCarrier_Update > 0 || rc_CTFBotAttackFlagDefenders_Update > 0)
			return 0;

		return DETOUR_STATIC_CALL(GetBotEscortCount)(team);
	}

	bool is_this_map_ignoring_cannot_pickup = false;

	RefCount rc_CTFBotFetchFlag_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotFetchFlag_Update, CTFBot *actor, float dt)
	{
		int cannotPickupInteligence = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(actor, cannotPickupInteligence, cannot_pick_up_intelligence);
		SCOPED_INCREMENT_IF(rc_CTFBotFetchFlag_Update, cannotPickupInteligence > 0 && !is_this_map_ignoring_cannot_pickup);
		return DETOUR_MEMBER_CALL(CTFBotFetchFlag_Update)(actor, dt);
	}

	DETOUR_DECL_MEMBER(void, CCaptureFlag_PickUp, CTFPlayer *player, bool invisible)
	{
		if (rc_CTFBotFetchFlag_Update)
			return;
		DETOUR_MEMBER_CALL(CCaptureFlag_PickUp)(player, invisible);
	}

	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
		ClearAllData();
		return DETOUR_MEMBER_CALL(CPopulationManager_Parse)();
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_CanMoveDuringTaunt)
	{
		//CBaseClient *client = reinterpret_cast<CBaseClient *>(this);
		bool result = DETOUR_MEMBER_CALL(CTFPlayer_CanMoveDuringTaunt)();
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		bool movetaunt = player->m_bAllowMoveDuringTaunt;
		float movespeed = player->m_flCurrentTauntMoveSpeed;
		//DevMsg("can move: %d %f\n", movetaunt, movespeed);
		//if (!result && player->IsBot() && player->m_Shared->InCond(TF_COND_TAUNTING))
			//return true;

		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ParseSharedTauntDataFromEconItemView, CEconItemView *item)
	{
		DETOUR_MEMBER_CALL(CTFPlayer_ParseSharedTauntDataFromEconItemView)(item);

		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->IsBot()) {
			float value = 0.f;
			FindAttribute(item->GetStaticData(), GetItemSchema()->GetAttributeDefinitionByName("taunt move speed"), &value);
			if (value != 0.f) {
				player->m_bAllowMoveDuringTaunt = true;
			}
		}
	}

	bool spawner_failed = false;

	DETOUR_DECL_MEMBER(bool, CRandomChoiceSpawner_Parse, KeyValues *kv)
	{
		spawner_failed = false;
		auto result = DETOUR_MEMBER_CALL(CRandomChoiceSpawner_Parse)(kv);
		if (spawner_failed) 
			return false;
		return result;
	}

	DETOUR_DECL_STATIC(IPopulationSpawner *, IPopulationSpawner_ParseSpawner, IPopulator *populator, KeyValues *data)
	{
		auto result = DETOUR_STATIC_CALL(IPopulationSpawner_ParseSpawner)(populator, data);

		spawner_failed |= result == nullptr;

		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFBotDeliverFlag_OnEnd, CTFBot *me, Action< CTFBot > *nextAction)
	{
		CTFBot::AttributeType has_attr = me->m_nBotAttrs;
		DETOUR_MEMBER_CALL(CTFBotDeliverFlag_OnEnd)(me, nextAction);
		static auto tf_mvm_bot_allow_flag_carrier_to_fight = ConVarRef("tf_mvm_bot_allow_flag_carrier_to_fight");
		if (tf_mvm_bot_allow_flag_carrier_to_fight.IsValid() && tf_mvm_bot_allow_flag_carrier_to_fight.GetBool()) {
			me->m_nBotAttrs = has_attr;
		}
	}

	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, Action_CTFBot_InvokeOnEnd, CTFBot *actor, Behavior<CTFBot> *behavior, Action<CTFBot> *nextaction)
	{
		auto action = reinterpret_cast<Action<CTFBot> *>(this);
		//DevMsg("Message1\n");
		DevMsg("InvokeEndAction %s %d %d:", action->GetName(), action, nextaction);
		if (actor != nullptr) {
		//DevMsg("Message2\n");
			DevMsg(" botname: %s", actor->GetPlayerName());
		}
		if (nextaction != nullptr) {
			//DevMsg("Message3\n");
			DevMsg(" nextaction: %s", nextaction->GetName());
		}
		DevMsg("\n");
		DETOUR_MEMBER_CALL(Action_CTFBot_InvokeOnEnd)(actor, behavior, nextaction);
	}

	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, Action_CTFBot_OnKilled, CTFBot *actor, const CTakeDamageInfo& info)
	{
		auto action = reinterpret_cast<Action<CTFBot> *>(this);
		DevMsg("OnKilled %s %s\n", actor->GetPlayerName(), action->GetName());
		
		return DETOUR_MEMBER_CALL(Action_CTFBot_OnKilled)(actor, info);
	}

	DETOUR_DECL_MEMBER(void, CTFBotEscortSquadLeader_OnEnd, CTFBot *actor, Action<CTFBot> *nextaction)
	{
		auto action = reinterpret_cast<CTFBotEscortSquadLeader*>(this);
		if (nextaction != action->m_actionToDoAfterSquadDisbands && action->m_actionToDoAfterSquadDisbands != nullptr) {
			delete action->m_actionToDoAfterSquadDisbands;
			action->m_actionToDoAfterSquadDisbands = nullptr;
		}
		
		DETOUR_MEMBER_CALL(CTFBotEscortSquadLeader_OnEnd)(actor, nextaction);
	}

	CBaseEntity *SelectTargetByName(CTFBot *actor, const char *name)
	{
		CBaseEntity *target = servertools->FindEntityByName(nullptr, name, actor);
		if (target == nullptr && FStrEq(name,"RandomEnemy")) {
			target = actor->SelectRandomReachableEnemy();
		}
		else if (target == nullptr && FStrEq(name, "ClosestPlayer")) {
			float closest_dist = FLT_MAX;
			ForEachTFPlayer([&](CTFPlayer *player){
				if (player->IsAlive() && !player->IsBot()) {
					float dist = player->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
					if (dist < closest_dist) {
						closest_dist = dist;
						target = player;
					}
				}
			});
		}
		else if (target == nullptr) {
			ForEachTFBot([&](CTFBot *bot) {
				if (bot->IsAlive() && FStrEq(bot->GetPlayerName(), name)) {
					target = bot; 
				}
			});
		}
		if (target == nullptr) {
			float closest_dist = FLT_MAX;
			ForEachEntityByClassname(name, [&](CBaseEntity *entity) {
				float dist = entity->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
				if (dist < closest_dist) {
					closest_dist = dist;
					target = entity;
				}
			});
		}
		return target;

	}

	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotTacticalMonitor_OnCommandString, CTFBot *actor, const char *cmd)
	{
		if (actor->IsAlive() && V_strnicmp(cmd, "interrupt_action", strlen("interrupt_action")) == 0) {
			CCommand command = CCommand();
			command.Tokenize(cmd);
			
			auto action = reinterpret_cast<Action<CTFBot> *>(this);

			const char *other_target = "";

			auto interrupt_action = new CTFBotMoveTo();
			for (int i = 1; i < command.ArgC(); i++) {
				if (strcmp(command[i], "-name") == 0) {
					interrupt_action->SetName(command[i+1]);
					i++;
				}
				else if (strcmp(command[i], "-pos") == 0) {
					Vector pos;
					pos.x = strtof(command[i+1], nullptr);
					pos.y = strtof(command[i+2], nullptr);
					pos.z = strtof(command[i+3], nullptr);

					interrupt_action->SetTargetPos(pos);
					i += 3;
				}
				else if (strcmp(command[i], "-lookpos") == 0) {
					Vector pos;
					pos.x = strtof(command[i+1], nullptr);
					pos.y = strtof(command[i+2], nullptr);
					pos.z = strtof(command[i+3], nullptr);

					interrupt_action->SetTargetAimPos(pos);
					i += 3;
				}
				else if (strcmp(command[i], "-posent") == 0) {
					if (strcmp(other_target, command[i+1]) == 0) {
						interrupt_action->SetTargetPosEntity(interrupt_action->GetTargetAimPosEntity());
					}
					else {
						CBaseEntity *target = SelectTargetByName(actor, command[i+1]);
						other_target = command[i+1];
						interrupt_action->SetTargetPosEntity(target);
					}
					i++;
				}
				else if (strcmp(command[i], "-lookposent") == 0) {
					if (strcmp(other_target, command[i+1]) == 0) {
						interrupt_action->SetTargetAimPosEntity(interrupt_action->GetTargetPosEntity());
					}
					else {
						CBaseEntity *target = SelectTargetByName(actor, command[i+1]);
						other_target = command[i+1];
						interrupt_action->SetTargetAimPosEntity(target);
					}
					i++;
				}
				else if (strcmp(command[i], "-duration") == 0) {
					interrupt_action->SetDuration(strtof(command[i+1], nullptr));
					i++;
				}
				else if (strcmp(command[i], "-waituntildone") == 0) {
					interrupt_action->SetWaitUntilDone(true);
				}
				else if (strcmp(command[i], "-killlook") == 0) {
					interrupt_action->SetKillLook(true);
				}
				else if (strcmp(command[i], "-ondoneattributes") == 0) {
					interrupt_action->SetOnDoneAttributes(command[i+1]);
					i++;
				}
			}	

			return EventDesiredResult<CTFBot>::SuspendFor(interrupt_action, "Executing interrupt task");
		}
		
		return DETOUR_MEMBER_CALL(CTFBotTacticalMonitor_OnCommandString)(actor, cmd);
	}

	DETOUR_DECL_MEMBER(bool, CSquadSpawner_Parse, KeyValues *kv_orig)
	{
		auto spawner = reinterpret_cast<CSquadSpawner *>(this);

		std::vector<KeyValues *> del_kv;

        bool no_wait_for_formation = false;
		bool no_formation = false;
		FOR_EACH_SUBKEY(kv_orig, subkey) {
			const char *name = subkey->GetName();
			bool del = true;
			if (FStrEq(name, "NoWaitForFormation")) {
				no_wait_for_formation = subkey->GetBool();
			} else if (FStrEq(name, "NoFormation")) {
				no_formation = subkey->GetBool();
			} else {
				del = false;
			}
			
			if (del) {
				del_kv.push_back(subkey);
			} else {
			}
		}
		
		for (auto subkey : del_kv) {
			kv_orig->RemoveSubKey(subkey);
			subkey->deleteThis();
		}

		auto result = DETOUR_MEMBER_CALL(CSquadSpawner_Parse)(kv_orig);

		for (auto bot_spawner : spawner->m_SubSpawners) {
			auto find = spawners.find(static_cast<CTFBotSpawner *>(bot_spawner));
			if (find != spawners.end()) {
				auto &data = find->second;
				data.no_wait_for_formation = no_wait_for_formation;
				data.no_formation = no_formation;
			}
		}
		return result;
	}

    CTFBot *bot_tactical_monitor = nullptr;
    DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotTacticalMonitor_Update, CTFBot *actor, float dt)
	{
        bot_tactical_monitor = actor;
		auto result = DETOUR_MEMBER_CALL(CTFBotTacticalMonitor_Update)(actor, dt);
        bot_tactical_monitor = nullptr;
		return result;
	}

    DETOUR_DECL_MEMBER(bool, CTFBotSquad_ShouldSquadLeaderWaitForFormation)
	{
        if (bot_tactical_monitor != nullptr) {
            auto data = GetDataForBot(bot_tactical_monitor);
            if (data != nullptr && data->no_wait_for_formation)
                return false;
        }
        return DETOUR_MEMBER_CALL(CTFBotSquad_ShouldSquadLeaderWaitForFormation)();
    }

	DETOUR_DECL_MEMBER(bool, CSquadSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		auto spawner = reinterpret_cast<CSquadSpawner *>(this);
		auto result = DETOUR_MEMBER_CALL(CSquadSpawner_Spawn)(where, ents);
		if (result) {
            for (int i = 0; i < ents->Count(); i++) {
                CTFBot *bot = ToTFBot(ents->Element(i));
				auto data = GetDataForBot(bot);
                if (bot != nullptr && data != nullptr) {
                    if (data->no_formation)
                        bot->LeaveSquad();
                }
            }
		}
		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFBot_StartIdleSound)
	{
		
        if (spawners[current_spawner].no_idle_sound)
			return;

		DETOUR_MEMBER_CALL(CTFBot_StartIdleSound)();
	}


	RefCount rc_CTFPlayer_Regenerate;
	DETOUR_DECL_MEMBER(void, CTFPlayer_Regenerate, bool ammo)
	{
		SCOPED_INCREMENT_IF(rc_CTFPlayer_Regenerate,reinterpret_cast<CTFPlayer *>(this)->IsBot());
		DETOUR_MEMBER_CALL(CTFPlayer_Regenerate)(ammo);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_GiveDefaultItems)
	{
		if (rc_CTFPlayer_Regenerate)
			return;

		DETOUR_MEMBER_CALL(CTFPlayer_GiveDefaultItems)();
	}

	DETOUR_DECL_MEMBER(int, CSpawnLocation_FindSpawnLocation, Vector& vSpawnPosition)
	{
		auto location = reinterpret_cast<CSpawnLocation *>(this);
		FOR_EACH_VEC(location->m_teamSpawnVector, i) {
			if (location->m_teamSpawnVector[i] == nullptr) {
				location->m_teamSpawnVector.Remove(i);
				i--;
			}
		}

		return DETOUR_MEMBER_CALL(CSpawnLocation_FindSpawnLocation)(vSpawnPosition);
	}

	DETOUR_DECL_MEMBER(bool, CBaseCombatCharacter_Weapon_Detach, CBaseCombatWeapon *weapon)
	{
		if (weapon == nullptr)
			return false;

		return DETOUR_MEMBER_CALL(CBaseCombatCharacter_Weapon_Detach)(weapon);
	}

	RefCount rc_CBasePlayer_ChangeTeam;
	DETOUR_DECL_MEMBER(void, CBasePlayer_ChangeTeam, int iTeamNum, bool b1, bool b2, bool b3)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DETOUR_MEMBER_CALL(CBasePlayer_ChangeTeam)(iTeamNum, b1, b2, b3);
		SCOPED_INCREMENT(rc_CBasePlayer_ChangeTeam);
	}

	DETOUR_DECL_MEMBER(void, CQuestItemTracker_FireGameEvent, IGameEvent* event)
	{
		return;
		// Msg("FireGameEvent\n");
		// if (rc_CBasePlayer_ChangeTeam) return;

		// DETOUR_MEMBER_CALL(CQuestItemTracker_FireGameEvent)(event);
	}
	DETOUR_DECL_MEMBER(void, CQuestItemTracker_FireGameEvent_NonVirtual, IGameEvent* event)
	{
		return;
		//Msg("FireGameEvent\n");
		// if (rc_CBasePlayer_ChangeTeam) return;

		// DETOUR_MEMBER_CALL(CQuestItemTracker_FireGameEvent_NonVirtual)(event);
	}
	
	DETOUR_DECL_MEMBER(bool, CBasePlayer_GetSteamID, CSteamID *pID)
	{
		if (this == nullptr) return false;
		
		return DETOUR_MEMBER_CALL(CBasePlayer_GetSteamID)(pID);
	}

	RefCount rc_IVision_IsAbleToSee;
	CBaseEntity *vision_subject = nullptr;
	bool vision_entity_melee = false;
	DETOUR_DECL_MEMBER(bool, IVision_IsAbleToSee, CBaseEntity *subject, int checkFOV, Vector *visibleSpot)
	{
		// {
		// 	TIME_SCOPE2(visible);
		// 	if (subject != nullptr && subject->MyCombatCharacterPointer() != nullptr && subject->MyCombatCharacterPointer()->GetLastKnownArea() != nullptr) {
		// 		subject->MyCombatCharacterPointer()->GetLastKnownArea()->Contains(subject->GetAbsOrigin());
		// 	}
		// }
		SCOPED_INCREMENT(rc_IVision_IsAbleToSee);
		vision_subject = subject;
		auto entity = reinterpret_cast<IVision *>(this)->GetBot()->GetEntity();
		vision_entity_melee = entity->GetActiveWeapon() != nullptr && entity->GetActiveWeapon()->IsMeleeWeapon();
		bool ret = DETOUR_MEMBER_CALL(IVision_IsAbleToSee)(subject,checkFOV, visibleSpot);
		vision_subject = nullptr;
		vision_entity_melee = false;

		return ret;
	}

	DETOUR_DECL_MEMBER(bool, IVision_IsLineOfSightClearToEntity, CBaseEntity *subject, Vector *visibleSpot)
	{
		CFastTimer timer;
		timer.Start();
		bool ret = DETOUR_MEMBER_CALL(IVision_IsLineOfSightClearToEntity)(subject, visibleSpot);
		timer.End();
		if (rc_IVision_IsAbleToSee) {
			Msg("LineOfSight %.9f %d\n", timer.GetDuration().GetSeconds(), gpGlobals->tickcount);
		}

		return ret;
	}

	bool IsOnNav(CBaseEntity *entity, float maxZDistance, float maxXYDistance)
	{
		auto subjectCombat = entity->MyCombatCharacterPointer();
		if (subjectCombat != nullptr) {
			auto area = subjectCombat->GetLastKnownArea();
			if (area != nullptr) {
				
				auto &vec = entity->GetAbsOrigin();
				bool contains = area->IsOverlapping(vec,maxXYDistance);
				float z = area->GetZ(vec.x, vec.y);
				if (!contains || z - vec.z > maxZDistance) {
					return false;
				}
			}
		}
		return true;
	}

	DETOUR_DECL_MEMBER(bool, CNavArea_IsPotentiallyVisible, CNavArea *area)
	{
		
		// CFastTimer timer;
		// timer.Start();
		if (rc_IVision_IsAbleToSee && vision_subject != nullptr && !vision_entity_melee && vision_subject->IsPlayer() && ToTFPlayer(vision_subject)->IsRealPlayer()) {
			return true;
		}
		bool ret = DETOUR_MEMBER_CALL(CNavArea_IsPotentiallyVisible)(area);
		if (!ret && rc_IVision_IsAbleToSee && !vision_entity_melee && vision_subject != nullptr && !IsOnNav(vision_subject, 20, 4)) {
			return true;
		}
		return ret;
	}

	// DETOUR_DECL_MEMBER(const CKnownEntity *, CTFBotMainAction_SelectCloserThreat, CTFBot *me, const CKnownEntity *threat1, const CKnownEntity *threat2)
	// {
	// 	auto closer = DETOUR_MEMBER_CALL(CTFBotMainAction_SelectCloserThreat)(me, threat1, threat2);
	// 	if (me->GetActiveTFWeapon() != nullptr && me->GetActiveTFWeapon()->IsMeleeWeapon()) {
	// 		float meleeRange = 48 * me->GetModelScale();
	// 		float maxZ = me->EyePosition().z - me->GetAbsOrigin().z + meleeRange;
	// 		float maxXY = meleeRange / 2 + 12;
	// 		bool threat1OnNav = IsOnNav(threat1->GetEntity(), maxZ, maxXY);
	// 		bool threat2OnNav = IsOnNav(threat2->GetEntity(), maxZ, maxXY);


	// 		if (threat1OnNav && !threat2OnNav) return threat1;
	// 		if (threat2OnNav && !threat1OnNav) return threat2;
	// 	}
	// 	return closer;
	// }


//#ifdef ENABLE_BROKEN_STUFF
	bool drop_weapon_bot = false;
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
	
	DETOUR_DECL_STATIC(CTFDroppedWeapon *, CTFDroppedWeapon_Create, CTFPlayer *pOwner, const Vector& vecOrigin, const QAngle& vecAngles, const char *pszModelName, const CEconItemView *pItemView)
	{
		// this is really ugly... we temporarily override m_bPlayingMannVsMachine
		// because the alternative would be to make a patch
		
		auto data = GetDataForBot(pOwner);

		bool is_mvm_mode = TFGameRules()->IsMannVsMachineMode();

		auto dropped_weapon_def = GetItemSchema()->GetAttributeDefinitionByName("is dropped weapon");
		float dropped_weapon_val = 0.0f;
		FindAttribute(&pItemView->GetAttributeList(), dropped_weapon_def, &dropped_weapon_val);

		TFGameRules()->Set_m_bPlayingMannVsMachine(is_mvm_mode && !(data != nullptr && data->drop_weapon) && !(dropped_weapon_val != 0.0f));
		
		auto result = DETOUR_STATIC_CALL(CTFDroppedWeapon_Create)(pOwner, vecOrigin, vecAngles, pszModelName, pItemView);
		
		if (result != nullptr) {
			CAttributeList &list = result->m_Item->GetAttributeList();
			auto cannot_upgrade_def = GetItemSchema()->GetAttributeDefinitionByName("cannot be upgraded");
			if (cannot_upgrade_def != nullptr && list.GetAttributeByName("cannot be upgraded") == nullptr) {
				list.SetRuntimeAttributeValue(cannot_upgrade_def, 1.0f);
			}
			if (dropped_weapon_def != nullptr) {
				list.SetRuntimeAttributeValue(dropped_weapon_def, 1.0f);
			}
		}

		TFGameRules()->Set_m_bPlayingMannVsMachine(is_mvm_mode);
		
		return result;
	}

//#endif
	

    VHOOK_DECL(bool, CObjectTeleporter_KeyValue, const char *szKeyName, const char *szValue)
	{
        if (FStrEq(szKeyName, "TeleportWhere") || FStrEq(szKeyName, "$TeleportWhere")) {
			auto me = reinterpret_cast<CObjectTeleporter *>(this);
			auto mod = me->GetOrCreateEntityModule<TeleporterModule>("teleportwhere");
			mod->teleportWhere.insert(szValue);
		}
        return VHOOK_CALL(CObjectTeleporter_KeyValue)(szKeyName, szValue);
    }
	
	
	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
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
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsPlayerClass,      "CTFPlayer::IsPlayerClass");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_GetFlagCaptureZone, "CTFBot::GetFlagCaptureZone");
			
			//MOD_ADD_DETOUR_STATIC(FireEvent,           "FireEvent");
			
//#ifdef ENABLE_BROKEN_STUFF
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldDropAmmoPack, "CTFPlayer::ShouldDropAmmoPack");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DropAmmoPack,       "CTFPlayer::DropAmmoPack");
			MOD_ADD_DETOUR_STATIC(CTFDroppedWeapon_Create,      "CTFDroppedWeapon::Create");
			MOD_ADD_DETOUR_STATIC(CTFAmmoPack_Create,           "CTFAmmoPack::Create");
//#endif
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_OnStart,       "CTFBotMissionSuicideBomber::OnStart");
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_Update,        "CTFBotMissionSuicideBomber::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBot_EquipBestWeaponForThreat, "CTFBot::EquipBestWeaponForThreat");

			
			//MOD_ADD_DETOUR_MEMBER(CSchemaFieldHandle_CEconItemDefinition, "CSchemaFieldHandle<CEconItemDefinition>::CSchemaFieldHandle");
			//MOD_ADD_DETOUR_MEMBER(CSchemaFieldHandle_CEconItemDefinition2, "CSchemaFieldHandle<CEconItemDefinition>::CSchemaFieldHandle2");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_HandleCommand_JoinClass,        "CTFPlayer::HandleCommand_JoinClass");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsMiniBoss,        "CTFPlayer::IsMiniBoss");
			//MOD_ADD_DETOUR_MEMBER(CBaseCombatCharacter_SetBloodColor,        "CBaseCombatCharacter::SetBloodColor");
			//MOD_ADD_DETOUR_MEMBER(CBaseAnimating_SetModelScale,        "CBaseAnimating::SetModelScale");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_GetProjectileSpeed,        "CTFWeaponBaseGun::GetProjectileSpeed");
			
			/* Hold fire until full reload on all weapons fix */
			MOD_ADD_DETOUR_MEMBER(CTFBot_IsBarrageAndReloadWeapon,        "CTFBot::IsBarrageAndReloadWeapon");
			//MOD_ADD_DETOUR_MEMBER(IVision_IsLineOfSightClear, "IVision::IsLineOfSightClear");

			/* Improved airblast*/
			MOD_ADD_DETOUR_MEMBER(CTFBot_ShouldFireCompressionBlast, "CTFBot::ShouldFireCompressionBlast");
			//MOD_ADD_DETOUR_MEMBER(CBaseProjectile_Spawn, "CBaseProjectile::Spawn");

			/* Fix to stop bumping ai on spies carrying bomb*/
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_AvoidBumpingEnemies, "CTFBotTacticalMonitor::AvoidBumpingEnemies");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotEscortFlagCarrier_Update, "CTFBotEscortFlagCarrier::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotAttackFlagDefenders_Update, "CTFBotAttackFlagDefenders::Update");
			MOD_ADD_DETOUR_STATIC(GetBotEscortCount, "GetBotEscortCount");

			/* Fix to prevent cannot pick up intelligence bots to spawn with the flag */
			MOD_ADD_DETOUR_MEMBER(CTFBotFetchFlag_Update, "CTFBotFetchFlag::Update");
			MOD_ADD_DETOUR_MEMBER(CCaptureFlag_PickUp, "CCaptureFlag::PickUp");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update, "CTFBotMainAction::Update");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_FindPartnerTauntInitiator, "CTFPlayer::FindPartnerTauntInitiator");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ClientConnected, "CTFGameRules::ClientConnected");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse, "CPopulationManager::Parse");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanMoveDuringTaunt, "CTFPlayer::CanMoveDuringTaunt");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ParseSharedTauntDataFromEconItemView, "CTFPlayer::ParseSharedTauntDataFromEconItemView");
			
			// Random choise spawner crash on parse fail fix
			MOD_ADD_DETOUR_MEMBER(CRandomChoiceSpawner_Parse, "CRandomChoiceSpawner::Parse");
			MOD_ADD_DETOUR_STATIC_PRIORITY(IPopulationSpawner_ParseSpawner, "IPopulationSpawner::ParseSpawner", HIGHEST);

			// Suppress fire removal when flag is dropped fix
			MOD_ADD_DETOUR_MEMBER(CTFBotDeliverFlag_OnEnd, "CTFBotDeliverFlag::OnEnd");
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_OnCommandString, "CTFBotTacticalMonitor::OnCommandString");
			//MOD_ADD_DETOUR_MEMBER(Action_CTFBot_OnKilled, "Action<CTFBot>::OnKilled");

			// Fix disband action memory leak
			MOD_ADD_DETOUR_MEMBER(CTFBotEscortSquadLeader_OnEnd, "CTFBotEscortSquadLeader::OnEnd");
			
			MOD_ADD_DETOUR_MEMBER(CSquadSpawner_Parse, "CSquadSpawner::Parse");
			MOD_ADD_DETOUR_MEMBER(CSquadSpawner_Spawn, "CSquadSpawner::Spawn");
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_Update,     "CTFBotTacticalMonitor::Update");
            MOD_ADD_DETOUR_MEMBER(CTFBotSquad_ShouldSquadLeaderWaitForFormation, "CTFBotSquad::ShouldSquadLeaderWaitForFormation");

			//MOD_ADD_DETOUR_MEMBER(Action_CTFBot_InvokeOnEnd, "Action<CTFBot>::InvokeOnEnd");
			
			//MOD_ADD_DETOUR_MEMBER(GetWeaponId, "Global::GetWeaponID");
			//MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInSphere, "ISpatialPartition::EnumerateElementsInSphere");

			MOD_ADD_DETOUR_MEMBER(CTFBot_StartIdleSound,        "CTFBot::StartIdleSound");

			// Prevent resupply from giving default items to robots
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Regenerate,        "CTFPlayer::Regenerate");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GiveDefaultItems,  "CTFPlayer::GiveDefaultItems");

			// Fix crash if spawn location is being removed during the round
			MOD_ADD_DETOUR_MEMBER(CSpawnLocation_FindSpawnLocation,  "CSpawnLocation::FindSpawnLocation");

			// Fix crash if trying to remove null weapon (happens when CTFBot::AddItem tries to remove cosmetics in weapon slot)
			MOD_ADD_DETOUR_MEMBER(CBaseCombatCharacter_Weapon_Detach,  "CBaseCombatCharacter::Weapon_Detach");

			// Fix crash related to bot change teams?
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_ChangeTeam,               "CBasePlayer::ChangeTeam [int, bool, bool, bool]");
			MOD_ADD_DETOUR_MEMBER(CQuestItemTracker_FireGameEvent,  "CQuestItemTracker::FireGameEvent");
			MOD_ADD_DETOUR_MEMBER(CQuestItemTracker_FireGameEvent_NonVirtual,  "CQuestItemTracker::FireGameEvent_NonVirtual");
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_GetSteamID,  "CBasePlayer::GetSteamID");


			// Custom TeleportWhere teleports 
			MOD_ADD_DETOUR_STATIC(DoTeleporterOverride, "DoTeleporterOverride");
			MOD_ADD_VHOOK(CObjectTeleporter_KeyValue, TypeName<CObjectTeleporter>(), "CBaseEntity::KeyValue");

			// Fix god spots
			MOD_ADD_DETOUR_MEMBER(IVision_IsAbleToSee, "IVision::IsAbleToSee2");
			//MOD_ADD_DETOUR_MEMBER(IVision_IsLineOfSightClearToEntity, "IVision::IsLineOfSightClearToEntity");
			MOD_ADD_DETOUR_MEMBER(CNavArea_IsPotentiallyVisible, "CNavArea::IsPotentiallyVisible");
			//MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_SelectCloserThreat, "CTFBotMainAction::SelectCloserThreat");
			
			

			//MOD_ADD_DETOUR_MEMBER(CTFBot_AddItem,        "CTFBot::AddItem");
			//MOD_ADD_DETOUR_MEMBER(CItemGeneration_GenerateRandomItem,        "CItemGeneration::GenerateRandomItem");
			// TEST! REMOVE ME!
//			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetOverrideStepSound, "CTFPlayer::GetOverrideStepSound");
//			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetSceneSoundToken,   "CTFPlayer::GetSceneSoundToken");
		}

		virtual void PreLoad() {
			//this->AddDetour(new CDetour("Action<CTFBot>::InvokeOnEnda", LibMgr::GetInfo(Library::SERVER).BaseAddr() + 0x00D669C0, GET_MEMBER_CALLBACK(Action_CTFBot_InvokeOnEnd), GET_MEMBER_INNERPTR(Action_CTFBot_InvokeOnEnd)));
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
			// Some maps, such as mvm_berserker, rely on vanilla cannot pick up intelligence behavior, which to not respect their attribute when the bomb first spawn in
			const char *str = engine->GetMapEntitiesString();
			const char *find = FindCaseSensitive(str, "cannot pick up intelligence");
			if (find != nullptr) {
				Msg("Found cannot pick up intelligence\n");
			}
			is_this_map_ignoring_cannot_pickup = find != nullptr;
			// std::string str(cvar_cannot_pickup_flag_ignore_maps.GetString());
            // boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>(","));

			// const char *map = STRING(gpGlobals->mapname);
			// is_this_map_ignoring_cannot_pickup = false;
            // for (auto &token : tokens) {
            //     if (StringStartsWith(map, token.c_str())) {
			// 		is_this_map_ignoring_cannot_pickup = true;
			// 		break;
			// 	}
            // }
			ClearAllData();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			ClearAllData();
		}
		
		virtual void FrameUpdatePostEntityThink() override
		{
			
			UpdateDelayedAddConds(delayed_addconds);
			UpdatePeriodicTasks(pending_periodic_tasks, false);
			if (paused_wave_time != -1 && g_pPopulationManager != nullptr && (gpGlobals->tickcount - paused_wave_time > 5 || gpGlobals->tickcount < paused_wave_time)) {
				paused_wave_time = -1;
				g_pPopulationManager->UnpauseSpawning();
				Msg("Client connected, unpausing\n");
			}
				
			
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