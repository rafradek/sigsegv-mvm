#include "mod.h"
#include "stub/entities.h"
#include "stub/projectiles.h"
#include "stub/tfbot.h"
#include "stub/gamerules.h"
#include "stub/populators.h"
#include "stub/misc.h"
#include "util/iterate.h"
#include "util/scope.h"
#include "mod/pop/kv_conditional.h"
#include "stub/team.h"
#include "mod/pop/pointtemplate.h"
#include "stub/strings.h"
#include "stub/usermessages_sv.h"
WARN_IGNORE__REORDER()
#include <../server/vote_controller.h>
WARN_RESTORE()
#define UNW_LOCAL_ONLY
#include <cxxabi.h>
#include <libunwind.h>

void backtrace() {
  unw_cursor_t cursor;
  unw_context_t context;

  // Initialize cursor to current frame for local unwinding.
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  // Unwind frames one by one, going up the frame stack.
  while (unw_step(&cursor) > 0) {
    unw_word_t offset, pc;
    unw_get_reg(&cursor, UNW_REG_IP, &pc);
    if (pc == 0) {
      break;
    }
    Msg("0x%lx:", pc);

    char sym[256];
    if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
      char* nameptr = sym;
      int status;
      char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
      if (status == 0) {
        nameptr = demangled;
      }
      Msg(" (%s+0x%lx)\n", nameptr, offset);
      std::free(demangled);
    } else {
      Msg(" -- error: unable to obtain symbol name for this frame\n");
    }
  }
}

enum SpawnResult
{
	SPAWN_FAIL     = 0,
	SPAWN_NORMAL   = 1,
	SPAWN_TELEPORT = 2,
};

namespace Mod::Pop::PopMgr_Extensions
{

	void PrintToChatAll(const char *str)
	{
		int msg_type = usermessages->LookupUserMessage("SayText2");
		if (msg_type == -1) return;
		
		CReliableBroadcastRecipientFilter filter;
		
		bf_write *msg = engine->UserMessageBegin(&filter, msg_type);
		if (msg == nullptr) return;
		
		msg->WriteByte(0x00);
		msg->WriteByte(0x00);
		msg->WriteString(str);
		
		engine->MessageEnd();
	}

	void PrintToChat(const char *str, CTFPlayer *player)
	{
		int msg_type = usermessages->LookupUserMessage("SayText2");
		if (msg_type == -1) return;
		
		CRecipientFilter filter;
		filter.AddRecipient(player);
		filter.MakeReliable();

		bf_write *msg = engine->UserMessageBegin(&filter, msg_type);
		if (msg == nullptr) return;
		
		msg->WriteByte(0x00);
		msg->WriteByte(0x00);
		msg->WriteString(str);
		
		engine->MessageEnd();
	}

	void ResendUpgradeFile(bool force);

	ConVar cvar_custom_upgrades_file("sig_mvm_custom_upgrades_file", "", FCVAR_NONE,
		"Set upgrades file to specified one",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			ResendUpgradeFile(true);
		});	
	ConVar cvar_bots_are_humans("sig_mvm_bots_are_humans", "0", FCVAR_NONE,
		"Bots should use human models",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
		});	
	ConVar cvar_vanilla_mode("sig_vanilla_mode", "0", FCVAR_NONE,	
		"Disable most mods",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
		});	
		
	std::string last_custom_upgrades = "";
	bool received_message_tick = false;
	void ResendUpgradeFile(bool force) {
		
		std::string convalue = cvar_custom_upgrades_file.GetString();
			
			if (convalue == "")
				convalue = "scripts/items/mvm_upgrades.txt";

			if (convalue == "scripts/items/mvm_upgrades.txt") {
				if(force) {
					/*ForEachTFPlayer([&](CTFPlayer *player){
						if (!player->IsFakeClient()) {
							DevMsg("Refunded money\n");
							player->m_Shared->m_bInUpgradeZone = true;
							auto kv = new KeyValues("MVM_Respec");
							serverGameClients->ClientCommandKeyValues(player->GetNetworkable()->GetEdict(), kv);
							player->m_Shared->m_bInUpgradeZone = false;
						}
					});*/
					TFGameRules()->SetCustomUpgradesFile(convalue.c_str());
				}
			}
			else {
				/*ForEachTFPlayer([&](CTFPlayer *player){
					if (!player->IsFakeClient()) {
						DevMsg("Refunded money\n");
						player->m_Shared->m_bInUpgradeZone = true;
						auto kv = new KeyValues("MVM_Respec");
						serverGameClients->ClientCommandKeyValues(player->GetNetworkable()->GetEdict(), kv);
						player->m_Shared->m_bInUpgradeZone = false;
					}
				});*/
				TFGameRules()->SetCustomUpgradesFile("scripts/items/mvm_upgrades.txt");
				convalue = "scripts/items/"+convalue;
				TFGameRules()->SetCustomUpgradesFile(convalue.c_str());
				convalue = "download/"+convalue;
				TFGameRules()->SetCustomUpgradesFile(convalue.c_str());
				if (force && last_custom_upgrades != convalue) {
					received_message_tick = true;
					PrintToChatAll("\x07""ffb200This mission uses custom upgrades. If you only see regular upgrades, enable downloads in options (Download all files or Don't download sound files)");
				}
				last_custom_upgrades = convalue;
			}
	}

	const char g_sSounds[10][24] = {"", "Scout.No03",   "Sniper.No04", "Soldier.No01",
		"Demoman.No03", "Medic.No03",  "heavy.No02",
		"Pyro.No01",    "Spy.No02",    "Engineer.No03"};

	template<typename T>
	class IPopOverride
	{
	public:
		virtual ~IPopOverride() {}
		
		bool IsOverridden() const { return this->m_bOverridden; }
		
		void Reset()
		{
			if (this->m_bOverridden) {
				this->Restore();
				this->m_bOverridden = false;
			}
		}
		void Set(const T& val)
		{
			if (!this->m_bOverridden) {
				this->Backup();
				this->m_bOverridden = true;
			}
			this->SetValue(val);
		}
		T Get() { return this->GetValue(); }
		
	protected:
		virtual T GetValue() = 0;
		virtual void SetValue(const T& val) = 0;
		
	private:
		void Backup()
		{
			this->m_Backup = this->GetValue();
		}
		void Restore()
		{
			this->SetValue(this->m_Backup);
		}
		
		bool m_bOverridden = false;
		T m_Backup;
	};
	
	
	// TODO: consider rewriting this in terms of IConVarOverride from util/misc.h
	template<typename T>
	class CPopOverride_ConVar : public IPopOverride<T>
	{
	public:
		CPopOverride_ConVar(const char *name) :
			m_pszConVarName(name) {}
		
		virtual T GetValue() override { return ConVar_GetValue<T>(MyConVar()); }
		
		virtual void SetValue(const T& val) override
		{
			/* set the ConVar value in a manner that circumvents:
			 * - FCVAR_NOTIFY notifications
			 * - minimum value limits
			 * - maximum value limits
			 */
			
			int old_flags   = MyConVar().Ref_Flags();
			bool old_hasmin = MyConVar().Ref_HasMin();
			bool old_hasmax = MyConVar().Ref_HasMax();
			
			MyConVar().Ref_Flags() &= ~FCVAR_NOTIFY;
			MyConVar().Ref_HasMin() = false;
			MyConVar().Ref_HasMax() = false;
			
			ConVar_SetValue<T>(MyConVar(), val);
			
			MyConVar().Ref_Flags()  = old_flags;
			MyConVar().Ref_HasMin() = old_hasmin;
			MyConVar().Ref_HasMax() = old_hasmax;
		}
		
	private:
		ConVarRef& MyConVar()
		{
			if (this->m_pConVar == nullptr) {
				this->m_pConVar = std::unique_ptr<ConVarRef>(new ConVarRef(this->m_pszConVarName));
			}
			return *(this->m_pConVar);
		}
		
		const char *m_pszConVarName;
		std::unique_ptr<ConVarRef> m_pConVar;
	};
	
	

	// TODO: fix problems related to client-side convar tf_medieval_thirdperson:
	// - players start out in first person, until they taunt or respawn or whatever
	// - players do not get forced back into first person upon popfile switch if the new pop doesn't have this enabled
	// see: firstperson/thirdperson client-side concommands are FCVAR_SERVER_CAN_EXECUTE in TF2; might be a solution
	//      static ConCommand thirdperson( "thirdperson", ::CAM_ToThirdPerson, "Switch to thirdperson camera.", FCVAR_CHEAT | FCVAR_SERVER_CAN_EXECUTE );
	//      static ConCommand firstperson( "firstperson", ::CAM_ToFirstPerson, "Switch to firstperson camera.", FCVAR_SERVER_CAN_EXECUTE );
	// also: just generally look at SetAppropriateCamera in the client DLL
	struct CPopOverride_MedievalMode : public IPopOverride<bool>
	{
		virtual bool GetValue() override                { return TFGameRules()->IsInMedievalMode(); }
		virtual void SetValue(const bool& val) override { TFGameRules()->Set_m_bPlayingMedieval(val); }
	};
	
	
	class CPopOverride_CustomUpgradesFile : public IPopOverride<std::string>
	{
	public:
		virtual std::string GetValue() override
		{
			std::string val = TFGameRules()->GetCustomUpgradesFile();
			
			if (val == "") {
				val = "scripts/items/mvm_upgrades.txt";
			}
			
			return val;
		}
		
		virtual void SetValue(const std::string& val) override
		{
			std::string real_val = val;
			
			if (real_val == "") {
				real_val = "scripts/items/mvm_upgrades.txt";
			}
			
			if (this->GetValue() != real_val) {
				ConColorMsg(Color(0x00, 0xff, 0xff, 0xff), "CPopOverride_CustomUpgradesFile: SetCustomUpgradesFile(\"%s\")\n", real_val.c_str());
				TFGameRules()->SetCustomUpgradesFile(real_val.c_str());
			}
		}
	};

	class ItemListEntry
	{
	public:
		virtual ~ItemListEntry() = default;
		virtual bool Matches(const char *classname, const CEconItemView *item_view) const = 0;
	};
	
	class ItemListEntry_Classname : public ItemListEntry
	{
	public:
		ItemListEntry_Classname(const char *classname) : m_strClassname(classname) {}
		
		virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
		{
			if (classname == nullptr) return false;
			return FStrEq(this->m_strClassname.c_str(), classname);
		}
		
	private:
		std::string m_strClassname;
	};
	
	class ItemListEntry_Name : public ItemListEntry
	{
	public:
		ItemListEntry_Name(const char *name) : m_strName(name) {}
		
		virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
		{
			if (item_view == nullptr) return false;
			return FStrEq(this->m_strName.c_str(), item_view->GetStaticData()->GetName(""));
		}
		
	private:
		std::string m_strName;
	};
	
	class ItemListEntry_DefIndex : public ItemListEntry
	{
	public:
		ItemListEntry_DefIndex(int def_index) : m_iDefIndex(def_index) {}
		
		virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
		{
			if (item_view == nullptr) return false;
			return (this->m_iDefIndex == item_view->GetItemDefIndex());
		}
		
	private:
		int m_iDefIndex;
	};
	
	
	class ExtraTankPath
	{
	public:
		ExtraTankPath(const char *name) : m_strName(name) {}
		
		~ExtraTankPath()
		{
			for (CPathTrack *node : this->m_PathNodes) {
				if (node != nullptr) {
					node->Remove();
				}
			}
		}
		
		bool AddNode(const Vector& origin)
		{
			assert(!this->m_bSpawned);
			
			auto node = rtti_cast<CPathTrack *>(CreateEntityByName("path_track"));
			if (node == nullptr) return false;
			
			CFmtStr name("%s_%zu", this->m_strName.c_str(), this->m_PathNodes.size() + 1);
			node->SetName(AllocPooledString(name));
			
			node->SetAbsOrigin(origin);
			node->SetAbsAngles(vec3_angle);
			
			node->m_eOrientationType = 1;
			
			this->m_PathNodes.emplace_back(node);
			
			DevMsg("ExtraTankPath: AddNode [ % 7.1f % 7.1f % 7.1f ] \"%s\"\n", VectorExpand(origin), name.Get());
			return true;
		}
		
		void SpawnNodes()
		{
			assert(!this->m_bSpawned);
			
			/* connect up the nodes' target links */
			for (auto it = this->m_PathNodes.begin(); (it + 1) != this->m_PathNodes.end(); ++it) {
				CPathTrack *curr = *it;
				CPathTrack *next = *(it + 1);
				
				DevMsg("ExtraTankPath: Link \"%s\" -> \"%s\"\n", STRING(curr->GetEntityName()), STRING(next->GetEntityName()));
				curr->m_target = next->GetEntityName();
			}
			
			/* now call Spawn() on each node */
			for (CPathTrack *node : this->m_PathNodes) {
				DevMsg("ExtraTankPath: Spawn \"%s\"\n", STRING(node->GetEntityName()));
				node->Spawn();
			}
			
			/* now call Activate() on each node */
			for (CPathTrack *node : this->m_PathNodes) {
				DevMsg("ExtraTankPath: Activate \"%s\"\n", STRING(node->GetEntityName()));
				node->Activate();
			}
			
			this->m_bSpawned = true;
		}
		
	private:
		std::string m_strName;
		std::vector<CHandle<CPathTrack>> m_PathNodes;
		bool m_bSpawned = false;
	};


	struct PopState
	{
		PopState() :
			m_SpellsEnabled           ("tf_spells_enabled"),
			m_GrapplingHook           ("tf_grapplinghook_enable"),
			m_RespecEnabled           ("tf_mvm_respec_enabled"),
			m_RespecLimit             ("tf_mvm_respec_limit"),
			m_BonusRatioHalf          ("tf_mvm_currency_bonus_ratio_min"),
			m_BonusRatioFull          ("tf_mvm_currency_bonus_ratio_max"),
			m_FixedBuybacks           ("tf_mvm_buybacks_method"),
			m_BuybacksPerWave         ("tf_mvm_buybacks_per_wave"),
			m_DeathPenalty            ("tf_mvm_death_penalty"),
			m_SentryBusterFriendlyFire("tf_bot_suicide_bomb_friendly_fire"),
			m_BotPushaway             ("tf_avoidteammates_pushaway"),
			m_HumansMustJoinTeam      ("mp_humans_must_join_team"),
			m_ForceHoliday            ("tf_forced_holiday"),
			m_AllowJoinTeamBlue       ("sig_mvm_jointeam_blue_allow"),
			m_AllowJoinTeamBlueMax    ("sig_mvm_jointeam_blue_allow_max"),
			m_BluHumanFlagPickup      ("sig_mvm_bluhuman_flag_pickup"),
			m_BluHumanFlagCapture     ("sig_mvm_bluhuman_flag_capture"),
			m_SetCreditTeam           ("sig_mvm_set_credit_team"),
			m_EnableDominations       ("sig_mvm_dominations"),
			m_RobotLimit              ("sig_mvm_robot_limit_override"),
			m_CustomUpgradesFile      ("sig_mvm_custom_upgrades_file"),
			m_BotsHumans              ("sig_mvm_bots_are_humans"),
			m_VanillaMode             ("sig_vanilla_mode"),
			m_BodyPartScaleSpeed      ("sig_mvm_body_scale_speed"),
			m_SandmanStuns      	  ("sig_mvm_stunball_stun"),
			m_StandableHeads      	  ("sig_robot_standable_heads"),
			m_MedigunShieldDamage     ("sig_mvm_medigunshield_damage"),
			m_NoRomevisionCosmetics   ("sig_no_romevision_cosmetics")
			
		{
			this->Reset();
		}
		
		void Reset()
		{
			this->m_bGiantsDropRareSpells   = false;
			this->m_flSpellDropRateCommon   = 1.00f;
			this->m_flSpellDropRateGiant    = 1.00f;
			this->m_bNoReanimators          = false;
			this->m_bNoMvMDeathTune         = false;
			this->m_bSniperHideLasers       = false;
			this->m_bSniperAllowHeadshots   = false;
			this->m_bDisableUpgradeStations = false;
			this->m_bRedPlayersRobots = false;
			this->m_bBluPlayersRobots = false;
			this->m_flRemoveGrapplingHooks  = -1.0f;
			this->m_bReverseWinConditions   = false;
			this->m_bDeclaredClassAttrib    = false;
			this->m_bFixSetCustomModelInput = false;
			this->m_bSendBotsToSpectatorImmediately = false;
			
			this->m_MedievalMode            .Reset();
			this->m_SpellsEnabled           .Reset();
			this->m_GrapplingHook           .Reset();
			this->m_RespecEnabled           .Reset();
			this->m_RespecLimit             .Reset();
			this->m_BonusRatioHalf          .Reset();
			this->m_BonusRatioFull          .Reset();
			this->m_FixedBuybacks           .Reset();
			this->m_BuybacksPerWave         .Reset();
			this->m_DeathPenalty            .Reset();
			this->m_SentryBusterFriendlyFire.Reset();
			this->m_BotPushaway             .Reset();
			this->m_HumansMustJoinTeam      .Reset();
			this->m_BotsHumans      .Reset();
			this->m_ForceHoliday      .Reset();
			
			this->m_AllowJoinTeamBlue   .Reset();
			this->m_AllowJoinTeamBlueMax.Reset();
			this->m_BluHumanFlagPickup  .Reset();
			this->m_BluHumanFlagCapture .Reset();
			this->m_SetCreditTeam       .Reset();
			this->m_EnableDominations   .Reset();
			this->m_RobotLimit          .Reset();
			this->m_VanillaMode          .Reset();
			this->m_BodyPartScaleSpeed   .Reset();
			this->m_SandmanStuns   .Reset();
			this->m_StandableHeads   .Reset();
			this->m_MedigunShieldDamage   .Reset();
			this->m_NoRomevisionCosmetics   .Reset();
			
			this->m_CustomUpgradesFile.Reset();
			
			this->m_DisableSounds  .clear();
			this->m_OverrideSounds  .clear();
			this->m_ItemWhitelist  .clear();
			this->m_ItemBlacklist  .clear();
			this->m_ItemAttributes  .clear();
			this->m_PlayerAttributes.clear();
			this->m_PlayerAddCond.clear();

			for (int i=0; i < 10; i++)
				this->m_DisallowedClasses[i] = -1;

			for (int i=0; i < 10; i++)
				this->m_PlayerAttributesClass[i].clear();
		//	this->m_DisallowedItems.clear();
			
			this->m_FlagResetTimes.clear();
			
			for (CTFTeamSpawn *spawn : this->m_ExtraSpawnPoints) {
				if (spawn != nullptr) {
					spawn->Remove();
				}
			}
			this->m_ExtraSpawnPoints.clear();
			
			this->m_ExtraTankPaths.clear();
			Clear_Point_Templates();
		}
		
		bool  m_bGiantsDropRareSpells;
		float m_flSpellDropRateCommon;
		float m_flSpellDropRateGiant;
		bool  m_bNoReanimators;
		bool  m_bNoMvMDeathTune;
		bool  m_bSniperHideLasers;
		bool  m_bSniperAllowHeadshots;
		bool  m_bDisableUpgradeStations;
		bool  m_bRedPlayersRobots;
		bool  m_bBluPlayersRobots;
		float m_flRemoveGrapplingHooks;
		bool  m_bReverseWinConditions;
		bool m_bDeclaredClassAttrib;
		bool m_bFixSetCustomModelInput;
		bool m_bSendBotsToSpectatorImmediately;

		CPopOverride_MedievalMode        m_MedievalMode;
		CPopOverride_ConVar<bool>        m_SpellsEnabled;
		CPopOverride_ConVar<bool>        m_GrapplingHook;
		CPopOverride_ConVar<bool>        m_RespecEnabled;
		CPopOverride_ConVar<int>         m_RespecLimit;
		CPopOverride_ConVar<float>       m_BonusRatioHalf;
		CPopOverride_ConVar<float>       m_BonusRatioFull;
		CPopOverride_ConVar<bool>        m_FixedBuybacks;
		CPopOverride_ConVar<int>         m_BuybacksPerWave;
		CPopOverride_ConVar<int>         m_DeathPenalty;
		CPopOverride_ConVar<bool>        m_SentryBusterFriendlyFire;
		CPopOverride_ConVar<bool>        m_BotPushaway;
		CPopOverride_ConVar<std::string> m_HumansMustJoinTeam;
		
		CPopOverride_ConVar<bool> m_AllowJoinTeamBlue;
		CPopOverride_ConVar<int>  m_AllowJoinTeamBlueMax;
		CPopOverride_ConVar<bool> m_BluHumanFlagPickup;
		CPopOverride_ConVar<bool> m_BluHumanFlagCapture;
		CPopOverride_ConVar<int>  m_SetCreditTeam;
		CPopOverride_ConVar<bool> m_EnableDominations;
		CPopOverride_ConVar<int>  m_RobotLimit;
		CPopOverride_ConVar<bool> m_BotsHumans;
		CPopOverride_ConVar<int> m_ForceHoliday;
		CPopOverride_ConVar<bool> m_VanillaMode;
		CPopOverride_ConVar<float> m_BodyPartScaleSpeed;
		CPopOverride_ConVar<bool> m_SandmanStuns;
		CPopOverride_ConVar<bool> m_MedigunShieldDamage;
		CPopOverride_ConVar<bool> m_StandableHeads;
		CPopOverride_ConVar<bool> m_NoRomevisionCosmetics;
		
		//CPopOverride_CustomUpgradesFile m_CustomUpgradesFile;
		CPopOverride_ConVar<std::string> m_CustomUpgradesFile;
		std::set<std::string>                       m_DisableSounds;
		std::map<std::string,std::string>           m_OverrideSounds;
		std::vector<std::unique_ptr<ItemListEntry>> m_ItemWhitelist;
		std::vector<std::unique_ptr<ItemListEntry>> m_ItemBlacklist;
		std::map<std::unique_ptr<ItemListEntry>,std::map<std::string,std::string>> m_ItemAttributes;
	//	std::set<int>                               m_DisallowedItems;
		
		std::map<std::string, int> m_FlagResetTimes;
		
		std::vector<CHandle<CTFTeamSpawn>> m_ExtraSpawnPoints;
		
		std::vector<ExtraTankPath> m_ExtraTankPaths;

		std::unordered_set<CTFPlayer*> m_PlayerUpgradeSend;

		int m_DisallowedClasses[10];

		std::map<std::string,float> m_PlayerAttributes;
		std::map<std::string,float> m_PlayerAttributesClass[10] = {};
		std::vector<ETFCond> m_PlayerAddCond;
	};
	PopState state;
	
	
	

	/* HACK: allow MvM:JoinTeam_Blue_Allow to force-off its admin-only functionality if the pop file explicitly set
	 * 'AllowJoinTeamBlue 1' (rather than someone manually setting 'sig_mvm_jointeam_blue_allow 1' via rcon) */
	bool PopFileIsOverridingJoinTeamBlueConVarOn()
	{
		return (state.m_AllowJoinTeamBlue.IsOverridden() && state.m_AllowJoinTeamBlue.Get());
	}
	
	
	bool bot_killed_check = false;
	RefCount rc_CTFGameRules_PlayerKilled;
	CBasePlayer *killed = nullptr;
	DETOUR_DECL_MEMBER(void, CTFGameRules_PlayerKilled, CBasePlayer *pVictim, const CTakeDamageInfo& info)
	{
	//	DevMsg("CTFGameRules::PlayerKilled\n");
		killed = pVictim;
		SCOPED_INCREMENT(rc_CTFGameRules_PlayerKilled);
		DETOUR_MEMBER_CALL(CTFGameRules_PlayerKilled)(pVictim, info);

		if(state.m_bSendBotsToSpectatorImmediately && pVictim != nullptr && pVictim->IsBot()) {
			bot_killed_check = true;
		}
	}
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_ShouldDropSpellPickup)
	{
	//	DevMsg("CTFGameRules::ShouldDropSpellPickup\n");
		
		if (TFGameRules()->IsMannVsMachineMode() && rc_CTFGameRules_PlayerKilled > 0) {
			CTFBot *bot = ToTFBot(killed);
			if (bot == nullptr) return false;
			if (!state.m_SpellsEnabled.Get()) return false;
			
			float rnd  = RandomFloat(0.0f, 1.0f);
			float rate = (bot->IsMiniBoss() ? state.m_flSpellDropRateGiant : state.m_flSpellDropRateCommon);
			
			if (rnd > rate) {
	//			DevMsg("  %.3f > %.3f, returning false\n", rnd, rate);
				return false;
			}
			
	//		DevMsg("  %.3f <= %.3f, returning true\n", rnd, rate);
			return true;
		}
		
		return DETOUR_MEMBER_CALL(CTFGameRules_ShouldDropSpellPickup)();
	}
	
	DETOUR_DECL_MEMBER(void, CTFGameRules_DropSpellPickup, const Vector& where, int tier)
	{
	//	DevMsg("CTFGameRules::DropSpellPickup\n");
		
		if (TFGameRules()->IsMannVsMachineMode() && rc_CTFGameRules_PlayerKilled > 0) {
			CTFBot *bot = ToTFBot(killed);
			if (bot != nullptr) {
	//			DevMsg("  is a bot\n");
				if (state.m_bGiantsDropRareSpells && bot->IsMiniBoss()) {
	//				DevMsg("  dropping rare spell for miniboss\n");
					tier = 1;
				}
			}
		}
		
		DETOUR_MEMBER_CALL(CTFGameRules_DropSpellPickup)(where, tier);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_IsUsingSpells)
	{
	//	DevMsg("CTFGameRules::IsUsingSpells\n");
		
		if (TFGameRules()->IsMannVsMachineMode() && rc_CTFGameRules_PlayerKilled > 0) {
	//		DevMsg("  returning true\n");
			return true;
		}
		
		return DETOUR_MEMBER_CALL(CTFGameRules_IsUsingSpells)();
	}
	
	DETOUR_DECL_STATIC(CTFReviveMarker *, CTFReviveMarker_Create, CTFPlayer *player)
	{
		if (state.m_bNoReanimators || TFGameRules()->IsInMedievalMode()) {
			return nullptr;
		}
		
		return DETOUR_STATIC_CALL(CTFReviveMarker_Create)(player);
	}
	
	DETOUR_DECL_MEMBER(void, CTFSniperRifle_CreateSniperDot)
	{
		auto rifle = reinterpret_cast<CTFSniperRifle *>(this);
		
		if (state.m_bSniperHideLasers && TFGameRules()->IsMannVsMachineMode()) {
			CTFPlayer *owner = rifle->GetTFPlayerOwner();
			if (owner != nullptr && owner->GetTeamNumber() == TF_TEAM_BLUE) {
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CTFSniperRifle_CreateSniperDot)();
	}
	
	RefCount rc_CTFSniperRifle_CanFireCriticalShot;
	DETOUR_DECL_MEMBER(bool, CTFSniperRifle_CanFireCriticalShot, bool bIsHeadshot, CBaseEntity *ent1)
	{
		SCOPED_INCREMENT(rc_CTFSniperRifle_CanFireCriticalShot);
		return DETOUR_MEMBER_CALL(CTFSniperRifle_CanFireCriticalShot)(bIsHeadshot, ent1);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_CanFireCriticalShot, bool bIsHeadshot, CBaseEntity *ent1)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		
		if (state.m_bSniperAllowHeadshots && rc_CTFSniperRifle_CanFireCriticalShot > 0 && TFGameRules()->IsMannVsMachineMode()) {
			CTFPlayer *owner = weapon->GetTFPlayerOwner();
			if (owner != nullptr && owner->GetTeamNumber() == TF_TEAM_BLUE) {
				return true;
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFWeaponBase_CanFireCriticalShot)(bIsHeadshot, ent1);
	}
	
	RefCount rc_CTFProjectile_Arrow_StrikeTarget;
	DETOUR_DECL_MEMBER(bool, CTFProjectile_Arrow_StrikeTarget, mstudiobbox_t *bbox, CBaseEntity *ent)
	{
		SCOPED_INCREMENT(rc_CTFProjectile_Arrow_StrikeTarget);
		return DETOUR_MEMBER_CALL(CTFProjectile_Arrow_StrikeTarget)(bbox, ent);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_IsPVEModeControlled, CBaseEntity *ent)
	{
		if (state.m_bSniperAllowHeadshots && rc_CTFProjectile_Arrow_StrikeTarget > 0 && TFGameRules()->IsMannVsMachineMode()) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL(CTFGameRules_IsPVEModeControlled)(ent);
	}
	
	DETOUR_DECL_MEMBER(void, CUpgrades_UpgradeTouch, CBaseEntity *pOther)
	{
		if (state.m_bDisableUpgradeStations && TFGameRules()->IsMannVsMachineMode()) {
			CTFPlayer *player = ToTFPlayer(pOther);
			if (player != nullptr) {
				gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, "The Upgrade Station is disabled for this mission!");
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CUpgrades_UpgradeTouch)(pOther);
	}
	
	DETOUR_DECL_MEMBER(void, CTeamplayRoundBasedRules_BroadcastSound, int iTeam, const char *sound, int iAdditionalSoundFlags)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
		//	DevMsg("CTeamplayRoundBasedRules::BroadcastSound(%d, \"%s\", 0x%08x)\n", iTeam, sound, iAdditionalSoundFlags);
			
			if (sound != nullptr && state.m_DisableSounds.count(std::string(sound)) != 0) {
				DevMsg("Blocked sound \"%s\" via CTeamplayRoundBasedRules::BroadcastSound\n", sound);
				return;
			}
			else if (sound != nullptr && state.m_OverrideSounds.count(std::string(sound)) != 0) {
				
				DETOUR_MEMBER_CALL(CTeamplayRoundBasedRules_BroadcastSound)(iTeam, state.m_OverrideSounds[sound].c_str(), iAdditionalSoundFlags);
				DevMsg("Blocked sound \"%s\" via CBaseEntity::EmitSound\n", sound);
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CTeamplayRoundBasedRules_BroadcastSound)(iTeam, sound, iAdditionalSoundFlags);
	}
	bool callfrom=false;
	DETOUR_DECL_STATIC(void, CBaseEntity_EmitSound_static_emitsound, IRecipientFilter& filter, int iEntIndex, const EmitSound_t& params)
	{
		if (!callfrom && TFGameRules()->IsMannVsMachineMode()) {
			const char *sound = params.m_pSoundName;
			if (iEntIndex > 0 && iEntIndex < 34 && strncmp(sound,"mvm/player/footsteps/robostep",26) == 0){
				filter = CReliableBroadcastRecipientFilter();
				edict_t *edict = INDEXENT( iEntIndex );
				if ( edict && !edict->IsFree() )
				{
					CTFPlayer *player = rtti_cast<CTFPlayer *>(GetContainingEntity(edict));
					if (player != nullptr && !player->IsBot() &&((state.m_bRedPlayersRobots && player->GetTeamNumber() == TF_TEAM_RED) || 
							(state.m_bBluPlayersRobots && player->GetTeamNumber() == TF_TEAM_BLUE))) {
						//CBaseEntity_EmitSound_static_emitsound
						callfrom = true;
						//DevMsg("CBaseEntity::ModEmitSound(#%d, \"%s\")\n", iEntIndex, sound);
						player->EmitSound("MVM.BotStep");
						return;
					}
				}
				
			}
			if (state.m_bNoMvMDeathTune && sound != nullptr && strcmp(sound, "MVM.PlayerDied") == 0) {
				return;
			}
			
			if (sound != nullptr && state.m_DisableSounds.count(std::string(sound)) != 0) {
				DevMsg("Blocked sound \"%s\" via CBaseEntity::EmitSound\n", sound);
				return;
			}
			else if (sound != nullptr && state.m_OverrideSounds.count(std::string(sound)) != 0) {
				EmitSound_t es;
				es.m_nChannel = params.m_nChannel;
				es.m_pSoundName = state.m_OverrideSounds[sound].c_str();
				es.m_flVolume = params.m_flVolume;
				es.m_SoundLevel = params.m_SoundLevel;
				es.m_nFlags = params.m_nFlags;
				es.m_nPitch = params.m_nPitch;
				//CBaseEntity::EmitSound(filter,iEntIndex,es);
				
				DETOUR_STATIC_CALL(CBaseEntity_EmitSound_static_emitsound)(filter, iEntIndex, es);
				DevMsg("Blocked sound \"%s\" via CBaseEntity::EmitSound\n", sound);
				return;
			}
		}
		DETOUR_STATIC_CALL(CBaseEntity_EmitSound_static_emitsound)(filter, iEntIndex, params);
		callfrom = false;
	}
	
	DETOUR_DECL_STATIC(void, CBaseEntity_EmitSound_static_emitsound_handle, IRecipientFilter& filter, int iEntIndex, const EmitSound_t& params, HSOUNDSCRIPTHANDLE& handle)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			const char *sound = params.m_pSoundName;
			
			//DevMsg("CBaseEntity::EmitSound(#%d, \"%s\", 0x%04x)\n", iEntIndex, sound, (uint16_t)handle);
			
			if (sound != nullptr && state.m_DisableSounds.count(std::string(sound)) != 0) {
				DevMsg("Blocked sound \"%s\" via CBaseEntity::EmitSound\n", sound);
				return;
			}
			else if (sound != nullptr && state.m_OverrideSounds.count(std::string(sound)) != 0) {
				EmitSound_t es;
				es.m_nChannel = params.m_nChannel;
				es.m_pSoundName = state.m_OverrideSounds[sound].c_str();
				es.m_flVolume = params.m_flVolume;
				es.m_SoundLevel = params.m_SoundLevel;
				es.m_nFlags = params.m_nFlags;
				es.m_nPitch = params.m_nPitch;
				DETOUR_STATIC_CALL(CBaseEntity_EmitSound_static_emitsound_handle)(filter, iEntIndex, es, handle);
				DevMsg("Blocked sound \"%s\" via CBaseEntity::EmitSound\n", sound);
				return;
			}
		}
		
		DETOUR_STATIC_CALL(CBaseEntity_EmitSound_static_emitsound_handle)(filter, iEntIndex, params, handle);
	}
	
//	RefCount rc_CTFPlayer_GiveDefaultItems;
//	DETOUR_DECL_MEMBER(void, CTFPlayer_GiveDefaultItems)
//	{
//		SCOPED_INCREMENT(rc_CTFPlayer_GiveDefaultItems);
//		DETOUR_MEMBER_CALL(CTFPlayer_GiveDefaultItems)();
//	}
	
	DETOUR_DECL_MEMBER(CBaseEntity *, CTFPlayer_GiveNamedItem, const char *classname, int i1, const CEconItemView *item_view, bool b1)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		/* this only applies to red team, for what essentially amounts to "legacy" reasons */
		if (TFGameRules()->IsMannVsMachineMode() && player->GetTeamNumber() == TF_TEAM_RED) {
			/* only enforce the whitelist/blacklist if they are non-empty */
			
			if (!state.m_ItemWhitelist.empty()) {
				bool found = false;
				for (const auto& entry : state.m_ItemWhitelist) {
					if (entry->Matches(classname, item_view)) {
						found = true;
						break;
					}
				}
				if (!found) {
					DevMsg("[%s] GiveNamedItem(\"%s\"): denied by whitelist\n", player->GetPlayerName(), classname);
					return nullptr;
				}
			}
			
			if (!state.m_ItemBlacklist.empty()) {
				bool found = false;
				for (const auto& entry : state.m_ItemBlacklist) {
					if (entry->Matches(classname, item_view)) {
						found = true;
						break;
					}
				}
				if (found) {
					DevMsg("[%s] GiveNamedItem(\"%s\"): denied by blacklist\n", player->GetPlayerName(), classname);
					return nullptr;
				}
			}

			
		}
		
	//	DevMsg("[%s] GiveNamedItem(\"%s\"): provisionally allowed\n", player->GetPlayerName(), classname);
		CBaseEntity *entity = DETOUR_MEMBER_CALL(CTFPlayer_GiveNamedItem)(classname, i1, item_view, b1);
		
		return entity;
	}
	DETOUR_DECL_MEMBER(void, CTFPlayer_ReapplyItemUpgrades, CEconItemView *item_view)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (TFGameRules()->IsMannVsMachineMode() && player->GetTeamNumber() == TF_TEAM_RED) {
			if (!state.m_ItemAttributes.empty()) {
				DevMsg("ReapplyItemUpgrades\n");
				bool found = false;
				const char *classname = item_view->GetItemDefinition()->GetItemClass();
				std::map<std::string, std::string> *attribs;
				for (auto& entry : state.m_ItemAttributes) {
					if (entry.first->Matches(classname, item_view)) {
						attribs = &(entry.second);
						found = true;
						break;
					}
				}
				if (found && attribs != nullptr) {
					CEconItemView *view = item_view;
					for (auto& entry : *attribs) {
						CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(entry.first.c_str());
						
						if (attr_def == nullptr) {
							DevMsg("[popmgr_extensions] Error: couldn't find any attributes in the item schema matching \"%s\".\n", entry.first.c_str());
							continue;
						}
						
						const char *value_str = STRING(AllocPooledString(entry.second.c_str()));
						
						attribute_data_union_t value;
						// TODO: maybe make a wrapper class for this or something, so that if we return early, we still do the proper
						// UnloadEconAttributeValue call to avoid leaking memory
						attr_def->GetType()->InitializeNewEconAttributeValue(&value);
						if (!attr_def->GetType()->BConvertStringToEconAttributeValue(attr_def, value_str, &value, true)) {
							DevMsg("[popmgr_extensions] Error: couldn't parse attribute value \"%s\".\n", entry.second.c_str());
							attr_def->GetType()->UnloadEconAttributeValue(&value);
							continue;
						}
						CEconItemAttribute *attr = CEconItemAttribute::Create(attr_def->GetIndex());
						*attr->GetValuePtr() = value;
						view->GetAttributeList().AddAttribute(attr); // <-- this calls the copy ctor of the CEconItemAttribute; so DO NOT deallocate the attr afterwards!
					//	CEconItemAttribute::Destroy(&attr);
						
						char buf[1024];
						attr_def->ConvertValueToString(value, buf, sizeof(buf));
						DevMsg("[%s] GiveNamedItem(\"%s\"): added weapon attribute\n", player->GetPlayerName(), classname);
					}
				}
			}
		}
		DETOUR_MEMBER_CALL(CTFPlayer_ReapplyItemUpgrades)(item_view);
	}

#if 0
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ItemIsAllowed, CEconItemView *item_view)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (TFGameRules()->IsMannVsMachineMode() && item_view != nullptr) {
			int item_defidx = item_view->GetItemDefIndex();
			
			if (state.m_DisallowedItems.count(item_defidx) != 0) {
				DevMsg("[%s] CTFPlayer::ItemIsAllowed(%d): denied\n", player->GetPlayerName(), item_defidx);
				return false;
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_ItemIsAllowed)(item_view);
	}
#endif
	
	DETOUR_DECL_MEMBER(int, CCaptureFlag_GetMaxReturnTime)
	{
		auto flag = reinterpret_cast<CCaptureFlag *>(this);
		
		if (TFGameRules()->IsMannVsMachineMode()) {
			auto it = state.m_FlagResetTimes.find(STRING(flag->GetEntityName()));
			if (it != state.m_FlagResetTimes.end()) {
				return (*it).second;
			}
		}
		
		return DETOUR_MEMBER_CALL(CCaptureFlag_GetMaxReturnTime)();
	}
	
	
	RefCount rc_CTFGameRules_ctor;
	DETOUR_DECL_MEMBER(void, CTFGameRules_ctor)
	{
		SCOPED_INCREMENT(rc_CTFGameRules_ctor);
		DETOUR_MEMBER_CALL(CTFGameRules_ctor)();
	}
	
#if 0
	DETOUR_DECL_MEMBER(void, CTFGameRules_SetWinningTeam, int team, int iWinReason, bool bForceMapReset, bool bSwitchTeams, bool bDontAddScore, bool bFinal)
	{
		if (TFGameRules()->IsMannVsMachineMode() && state.m_bReverseWinConditions && team == TF_TEAM_BLUE) {
			// don't forward the call to SetWinningTeam
			// instead, imitate what is done when a wave is completed
			
			ForEachTFPlayer([](CTFPlayer *player){
				if (!player->IsAlive())                     return;
				if (player->GetTeamNumber() != TF_TEAM_RED) return;
				
				player->CommitSuicide(true, false);
			});
			
			// ...
			
			return;
		}
		
		DETOUR_MEMBER_CALL(CTFGameRules_SetWinningTeam)(team, iWinReason, bForceMapReset, bSwitchTeams, bDontAddScore, bFinal);
	}
#endif
	
	DETOUR_DECL_MEMBER(void, CTeamplayRoundBasedRules_State_Enter, gamerules_roundstate_t newState)
	{
		/* the CTeamplayRoundBasedRules ctor calls State_Enter BEFORE the CTFGameRules ctor has had a chance to run yet
		 * and initialize stuff like m_bPlayingMannVsMachine etc; so don't do any of this stuff in that case! */
		if (rc_CTFGameRules_ctor <= 0) {
			gamerules_roundstate_t oldState = TFGameRules()->State_Get();
			
		//	ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[State] MvM:%d Reverse:%d oldState:%d newState:%d\n",
		//		TFGameRules()->IsMannVsMachineMode(), state.m_bReverseWinConditions, oldState, newState);
			
			if (TFGameRules()->IsMannVsMachineMode() && state.m_bReverseWinConditions && oldState == GR_STATE_TEAM_WIN && newState == GR_STATE_PREROUND) {
				ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "GR_STATE_TEAM_WIN --> GR_STATE_PREROUND\n");
				
				CWave *wave = g_pPopulationManager->GetCurrentWave();
				
				// TODO(?): find all TFBots not on TEAM_SPECTATOR and switch them to TEAM_SPECTATOR
				
				/* call this twice to ensure all wavespawns get to state DONE */
				ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "PRE  CWave::ForceFinish\n");
				wave->ForceFinish();
				ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "MID  CWave::ForceFinish\n");
				wave->ForceFinish();
				ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "POST CWave::ForceFinish\n");
				
				// is this necessary or not? unsure...
				// TODO: if enabled, need to block CTFPlayer::CommitSuicide call from ActiveWaveUpdate
			//	ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "PRE  CWave::ActiveWaveUpdate\n");
			//	wave->ActiveWaveUpdate();
			//	ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "POST CWave::ActiveWaveUpdate\n");
				
				ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "PRE  CWave::WaveCompleteUpdate\n");
				wave->WaveCompleteUpdate();
				ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "POST CWave::WaveCompleteUpdate\n");
				
				// TODO(?): for all human players on TF_TEAM_BLUE: if !IsAlive(), then call ForceRespawn()
				
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CTeamplayRoundBasedRules_State_Enter)(newState);
	}
	CBaseEntity *templateTargetEntity = nullptr;
	bool SpawnOurTemplate(CEnvEntityMaker* maker, Vector vector, QAngle angles)
	{
		const char *src = STRING((string_t)maker->m_iszTemplate);
		DevMsg("Spawning template %s\n", src);
		if (Point_Templates().find(src) != Point_Templates().end()){
			DevMsg("Spawning template placeholder\n");
			PointTemplateInstance *inst = Point_Templates().find(src)->second.SpawnTemplate(templateTargetEntity,vector,angles,false);
			for (auto entity : inst->entities) {
				if ( entity->GetMoveType() == MOVETYPE_NONE )
					continue;

				// Calculate a velocity for this entity
				Vector vForward,vRight,vUp;
				QAngle angSpawnDir( maker->m_angPostSpawnDirection );
				if ( maker->m_bPostSpawnUseAngles )
				{
					if ( entity->GetMoveParent()  )
					{
						angSpawnDir += entity->GetMoveParent()->GetAbsAngles();
					}
					else
					{
						angSpawnDir += entity->GetAbsAngles();
					}
				}
				AngleVectors( angSpawnDir, &vForward, &vRight, &vUp );
				Vector vecShootDir = vForward;
				vecShootDir += vRight * RandomFloat(-1, 1) * maker->m_flPostSpawnDirectionVariance;
				vecShootDir += vForward * RandomFloat(-1, 1) * maker->m_flPostSpawnDirectionVariance;
				vecShootDir += vUp * RandomFloat(-1, 1) * maker->m_flPostSpawnDirectionVariance;
				VectorNormalize( vecShootDir );
				vecShootDir *= maker->m_flPostSpawnSpeed;

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
			
			return true;
		}
		else
			return false;
	}
	DETOUR_DECL_MEMBER(void, CEnvEntityMaker_InputForceSpawn, inputdata_t &inputdata)
	{
		auto me = reinterpret_cast<CEnvEntityMaker *>(this);
		if (!SpawnOurTemplate(me,me->GetAbsOrigin(),me->GetAbsAngles())){
			DETOUR_MEMBER_CALL(CEnvEntityMaker_InputForceSpawn)(inputdata);
		}
	}
	DETOUR_DECL_MEMBER(void, CEnvEntityMaker_InputForceSpawnAtEntityOrigin, inputdata_t &inputdata)
	{
		auto me = reinterpret_cast<CEnvEntityMaker *>(this);
		templateTargetEntity = servertools->FindEntityByName( NULL, STRING(inputdata.value.StringID()), me, inputdata.pActivator, inputdata.pCaller );
		DETOUR_MEMBER_CALL(CEnvEntityMaker_InputForceSpawnAtEntityOrigin)(inputdata);
		templateTargetEntity = nullptr;
	}
	DETOUR_DECL_MEMBER(void, CEnvEntityMaker_SpawnEntity, Vector vector, QAngle angles)
	{
		auto me = reinterpret_cast<CEnvEntityMaker *>(this);
		if (!SpawnOurTemplate(me,vector,angles)){
			DETOUR_MEMBER_CALL(CEnvEntityMaker_SpawnEntity)(vector,angles);
		}
	}

	void CheckPlayerClassLimit(CTFPlayer *player)
	{
		int plclass = player->GetPlayerClass()->GetClassIndex();
		const char* classname = g_aRawPlayerClassNames[plclass];
		if (player->GetTeamNumber() != TF_TEAM_RED || player->IsBot() || state.m_DisallowedClasses[plclass] == -1)
			return;

		int taken_slots[10];
		for (int i=0; i < 10; i++)
			taken_slots[i] = 0;

		ForEachTFPlayer([&](CTFPlayer *playerin){
			if(player != playerin && playerin->GetTeamNumber() == TF_TEAM_RED && !playerin->IsBot()){
				int classnum = playerin->GetPlayerClass()->GetClassIndex();
				taken_slots[classnum]+=1;
			}
		});

		if (state.m_DisallowedClasses[plclass] <= taken_slots[plclass]){
			for (int i=1; i < 10; i++){
				if(state.m_DisallowedClasses[i] == -1 || taken_slots[i] < state.m_DisallowedClasses[i]){
					const char *sound = g_sSounds[plclass];
					DevMsg("sound, %s\n",sound);

					CRecipientFilter filter;
					filter.AddRecipient(player);
					CBaseEntity::EmitSound(filter, ENTINDEX(player), sound);

					gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, CFmtStr("%s %s %s", "Exceeded the",classname,"class limit in this mission"));
					player->HandleCommand_JoinClass(g_aRawPlayerClassNames[i]);
					
					int msg_type = usermessages->LookupUserMessage("VGUIMenu");
					if (msg_type == -1) return;
					
					bf_write *msg = engine->UserMessageBegin(&filter, msg_type);
					if (msg == nullptr) return;
					
					msg->WriteString("class_red");
					msg->WriteByte(0x01);
					msg->WriteByte(0x00);

					engine->MessageEnd();
					
					break;
				}
			}
		}
	}
	DETOUR_DECL_MEMBER(void, CTFPlayer_HandleCommand_JoinClass, const char *pClassName, bool b1)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DETOUR_MEMBER_CALL(CTFPlayer_HandleCommand_JoinClass)(pClassName, b1);
		CheckPlayerClassLimit(player);
	}
	DETOUR_DECL_MEMBER(void, CTFPlayer_ChangeTeam, int iTeamNum, bool b1, bool b2, bool b3)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DETOUR_MEMBER_CALL(CTFPlayer_ChangeTeam)(iTeamNum, b1, b2, b3);
		CheckPlayerClassLimit(player);
	}
	DETOUR_DECL_MEMBER(void, CTFGameRules_OnPlayerSpawned, CTFPlayer *player)
	{
		if (!player->IsBot())
			if(state.m_PlayerUpgradeSend.find(player) == state.m_PlayerUpgradeSend.end()){
				state.m_PlayerUpgradeSend.insert(player);
				ResendUpgradeFile(false);
				if (!received_message_tick)
				PrintToChat("\x07""ffb200This mission uses custom upgrades. If you only see regular upgrades, enable downloads in options (Download all files or Don't download sound files)",player);
			}

		DETOUR_MEMBER_CALL(CTFGameRules_OnPlayerSpawned)(player);

		if (!player->IsBot() && ((state.m_bRedPlayersRobots && player->GetTeamNumber() == TF_TEAM_RED) || 
				(state.m_bBluPlayersRobots && player->GetTeamNumber() == TF_TEAM_BLUE))) {

			int model_class=player->GetPlayerClass()->GetClassIndex();
			const char *model      = g_szBotModels[model_class];
			const char *model_boss = g_szBotBossModels[model_class];
			
			//if (g_pFullFileSystem->FileExists(model)) {
			player->GetPlayerClass()->SetCustomModel(model, true);
			player->UpdateModel();
			player->SetBloodColor(DONT_BLEED);
			//}
		}
	}
	DETOUR_DECL_MEMBER(void, CBaseIssue_ctor,const char *type)
	{
		DETOUR_MEMBER_CALL(CBaseIssue_ctor)(type);
	}
	DETOUR_DECL_MEMBER(void, CTFPlayer_InputSetCustomModel, inputdata_t data)
	{
		DETOUR_MEMBER_CALL(CTFPlayer_InputSetCustomModel)(data);
		if (state.m_bFixSetCustomModelInput) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			player->GetPlayerClass()->m_bUseClassAnimations=true;
		}
	}
		DETOUR_DECL_MEMBER(const char *, CTFPlayer_GetOverrideStepSound, const char *pszBaseStepSoundName)
		{
			auto player = reinterpret_cast<CTFPlayer *>(this);
			if (!player->IsBot() &&((state.m_bRedPlayersRobots && player->GetTeamNumber() == TF_TEAM_RED) || 
					(state.m_bBluPlayersRobots && player->GetTeamNumber() == TF_TEAM_BLUE))) {
				return "MVM.BotStep";
			}
			else if((!player->IsBot() && player->GetTeamNumber() == TF_TEAM_BLUE) || cvar_bots_are_humans.GetBool()) {
				return pszBaseStepSoundName;
			}
			return DETOUR_MEMBER_CALL( CTFPlayer_GetOverrideStepSound)(pszBaseStepSoundName);
		}
		
		DETOUR_DECL_MEMBER(const char *, CTFPlayer_GetSceneSoundToken)
		{
			auto player = reinterpret_cast<CTFPlayer *>(this);
			if (!player->IsBot() &&((state.m_bRedPlayersRobots && player->GetTeamNumber() == TF_TEAM_RED) || 
					(state.m_bBluPlayersRobots && player->GetTeamNumber() == TF_TEAM_BLUE))) {
				return "MVM_";
			}
			else if((!player->IsBot() && player->GetTeamNumber() == TF_TEAM_BLUE) || cvar_bots_are_humans.GetBool()) {
				return "";
			}
			//const char *token=DETOUR_MEMBER_CALL( CTFPlayer_GetSceneSoundToken)();
			//DevMsg("CTFPlayer::GetSceneSoundToken %s\n", token);
			return DETOUR_MEMBER_CALL( CTFPlayer_GetSceneSoundToken)();
		}
	
	// DETOUR_DECL_STATIC(void, UserMessageBegin,IRecipientFilter &filter,const char *name)
	// {
	// 	DevMsg("UserMessage %s\n",name);
	// 	DETOUR_STATIC_CALL(UserMessageBegin)(filter,name);
	// }
	// DETOUR_DECL_STATIC(void, MessageWriteString,const char *name)
	// {
	// 	DevMsg("MessageWriteString %s\n",name);
	// 	DETOUR_STATIC_CALL(MessageWriteString)(name);
	// }
	
//	RefCount rc_CTFGameRules_FireGameEvent_teamplay_round_start;
//	DETOUR_DECL_MEMBER(void, CTFGameRules_FireGameEvent, IGameEvent *event)
//	{
//		SCOPED_INCREMENT_IF(rc_CTFGameRules_FireGameEvent_teamplay_round_start, FStrEq(event->GetName(), "teamplay_round_start"));
//		
//		DETOUR_MEMBER_CALL(CTFGameRules_FireGameEvent)(event);
//	}
//	
//	DETOUR_DECL_MEMBER(void, CTFPlayer_ChangeTeam, int iTeamNum, bool b1, bool b2, bool b3)
//	{
//		auto player = reinterpret_cast<CTFPlayer *>(this);
//		
//		if (rc_CTFGameRules_FireGameEvent_teamplay_round_start > 0 && TFGameRules()->IsMannVsMachineMode() && state.m_bReverseWinConditions && iTeamNum == TEAM_SPECTATOR) {
//			ConColorMsg(Color(0x00, 0xff, 0xff, 0xff), "BLOCKING ChangeTeam(TEAM_SPECTATOR) for player #%d \"%s\" on team %d\n",
//				ENTINDEX(player), player->GetPlayerName(), player->GetTeamNumber());
//			return;
//		}
//		
//		DETOUR_MEMBER_CALL(CTFPlayer_ChangeTeam)(iTeamNum, b1, b2, b3);
//	}
	
	
//	DETOUR_DECL_MEMBER(void, CPopulationManager_PostInitialize)
//	{
//		DETOUR_MEMBER_CALL(CPopulationManager_PostInitialize)();
//	}
	
	
	void Parse_ItemWhitelist(KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			if (FStrEq(subkey->GetName(), "Classname")) {
				DevMsg("ItemWhitelist: Add Classname entry: \"%s\"\n", subkey->GetString());
				state.m_ItemWhitelist.push_back(std::make_unique<ItemListEntry_Classname>(subkey->GetString()));
			} else if (FStrEq(subkey->GetName(), "Name")) {
				DevMsg("ItemWhitelist: Add Name entry: \"%s\"\n", subkey->GetString());
				state.m_ItemWhitelist.push_back(std::make_unique<ItemListEntry_Name>(subkey->GetString()));
			} else if (FStrEq(subkey->GetName(), "DefIndex")) {
				DevMsg("ItemWhitelist: Add DefIndex entry: %d\n", subkey->GetInt());
				state.m_ItemWhitelist.push_back(std::make_unique<ItemListEntry_DefIndex>(subkey->GetInt()));
			} else {
				DevMsg("ItemWhitelist: Found DEPRECATED entry with key \"%s\"; treating as Classname entry: \"%s\"\n", subkey->GetName(), subkey->GetString());
				state.m_ItemWhitelist.push_back(std::make_unique<ItemListEntry_Classname>(subkey->GetString()));
			}
		}
	}
	
	void Parse_ItemBlacklist(KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			if (FStrEq(subkey->GetName(), "Classname")) {
				DevMsg("ItemBlacklist: Add Classname entry: \"%s\"\n", subkey->GetString());
				state.m_ItemBlacklist.push_back(std::make_unique<ItemListEntry_Classname>(subkey->GetString()));
			} else if (FStrEq(subkey->GetName(), "Name")) {
				DevMsg("ItemBlacklist: Add Name entry: \"%s\"\n", subkey->GetString());
				state.m_ItemBlacklist.push_back(std::make_unique<ItemListEntry_Name>(subkey->GetString()));
			} else if (FStrEq(subkey->GetName(), "DefIndex")) {
				DevMsg("ItemBlacklist: Add DefIndex entry: %d\n", subkey->GetInt());
				state.m_ItemBlacklist.push_back(std::make_unique<ItemListEntry_DefIndex>(subkey->GetInt()));
			} else {
				DevMsg("ItemBlacklist: Found DEPRECATED entry with key \"%s\"; treating as Classname entry: \"%s\"\n", subkey->GetName(), subkey->GetString());
				state.m_ItemBlacklist.push_back(std::make_unique<ItemListEntry_Classname>(subkey->GetString()));
			}
		}
	}
	void Parse_ItemAttributes(KeyValues *kv)
	{
		std::map<std::string,std::string> attrib;
		bool hasname = false;
		std::map<std::string,std::string> *mappos;
		FOR_EACH_SUBKEY(kv, subkey) {
			//std::unique_ptr<ItemListEntry> key=std::make_unique<ItemListEntry_Classname>("");
			if (FStrEq(subkey->GetName(), "Classname")) {
				DevMsg("ItemAttrib: Add Classname entry: \"%s\"\n", subkey->GetString());
				hasname = true;
				mappos = &(state.m_ItemAttributes.emplace(std::make_unique<ItemListEntry_Classname>(subkey->GetString()),std::map<std::string,std::string>()).first->second);
			} else if (FStrEq(subkey->GetName(), "ItemName")) {
				hasname = true;
				DevMsg("ItemAttrib: Add Name entry: \"%s\"\n", subkey->GetString());
				mappos = &(state.m_ItemAttributes.emplace(std::make_unique<ItemListEntry_Name>(subkey->GetString()),std::map<std::string,std::string>()).first->second);
			} else if (FStrEq(subkey->GetName(), "DefIndex")) {
				hasname = true;
				DevMsg("ItemAttrib: Add DefIndex entry: %d\n", subkey->GetInt());
				mappos = &(state.m_ItemAttributes.emplace(std::make_unique<ItemListEntry_DefIndex>(subkey->GetInt()),std::map<std::string,std::string>()).first->second);
			} else {
				attrib[subkey->GetName()]=subkey->GetString();
			}
		}
		if (hasname) {
			*mappos = attrib;
		}
	}
	void Parse_ClassBlacklist(KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			for(int i=1; i < 10; i++){
				if(FStrEq(g_aRawPlayerClassNames[i],subkey->GetName())){
					state.m_DisallowedClasses[i]=subkey->GetInt();
					break;
				}
			}
		}
		for(int i=1; i < 10; i++){
			DevMsg("%d ",state.m_DisallowedClasses[i]);
		}
		DevMsg("\n");
		ForEachTFPlayer([&](CTFPlayer *player){
			if(player->GetTeamNumber() == TF_TEAM_RED && !player->IsBot()){
				CheckPlayerClassLimit(player);
			}
		});
	}

	void Parse_FlagResetTime(KeyValues *kv)
	{
		const char *name = nullptr; // required
		int reset_time;             // required
		
		bool got_time = false;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			if (FStrEq(subkey->GetName(), "Name")) {
				name = subkey->GetString();
			} else if (FStrEq(subkey->GetName(), "ResetTime")) {
				reset_time = subkey->GetInt();
				got_time = true;
			} else {
				Warning("Unknown key \'%s\' in FlagResetTime block.\n", subkey->GetName());
			}
		}
		
		bool fail = false;
		if (name == nullptr) {
			Warning("Missing Name key in FlagResetTime block.\n");
			fail = true;
		}
		if (!got_time) {
			Warning("Missing ResetTime key in FlagResetTime block.\n");
			fail = true;
		}
		if (fail) return;
		
		DevMsg("FlagResetTime: \"%s\" => %d\n", name, reset_time);
		state.m_FlagResetTimes.emplace(name, reset_time);
	}
	
	void Parse_ExtraSpawnPoint(KeyValues *kv)
	{
		const char *name = nullptr; // required
		int teamnum = TEAM_INVALID; // required
		Vector origin;              // required
		
		bool got_x = false;
		bool got_y = false;
		bool got_z = false;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			if (FStrEq(subkey->GetName(), "Name")) {
				name = subkey->GetString();
			} else if (FStrEq(subkey->GetName(), "TeamNum")) {
				teamnum = Clamp(subkey->GetInt(), (int)TF_TEAM_RED, (int)TF_TEAM_BLUE);
			} else if (FStrEq(subkey->GetName(), "X")) {
				origin.x = subkey->GetFloat(); got_x = true;
			} else if (FStrEq(subkey->GetName(), "Y")) {
				origin.y = subkey->GetFloat(); got_y = true;
			} else if (FStrEq(subkey->GetName(), "Z")) {
				origin.z = subkey->GetFloat(); got_z = true;
			} else {
				Warning("Unknown key \'%s\' in ExtraSpawnPoint block.\n", subkey->GetName());
			}
		}
		
		bool fail = false;
		if (name == nullptr) {
			Warning("Missing Name key in ExtraSpawnPoint block.\n");
			fail = true;
		}
		if (teamnum == TEAM_INVALID) {
			Warning("Missing TeamNum key in ExtraSpawnPoint block.\n");
			fail = true;
		}
		if (!got_x) {
			Warning("Missing X key in ExtraSpawnPoint block.\n");
			fail = true;
		}
		if (!got_y) {
			Warning("Missing Y key in ExtraSpawnPoint block.\n");
			fail = true;
		}
		if (!got_z) {
			Warning("Missing Z key in ExtraSpawnPoint block.\n");
			fail = true;
		}
		if (fail) return;
		
		// Is it actually OK that we're spawning this stuff during parsing...?
		
		auto spawnpoint = rtti_cast<CTFTeamSpawn *>(CreateEntityByName("info_player_teamspawn"));
		if (spawnpoint == nullptr) {
			Warning("Parse_ExtraSpawnPoint: CreateEntityByName(\"info_player_teamspawn\") failed\n");
			return;
		}
		
		spawnpoint->SetName(AllocPooledString(name));
		
		spawnpoint->SetAbsOrigin(origin);
		spawnpoint->SetAbsAngles(vec3_angle);
		
		spawnpoint->ChangeTeam(teamnum);
		
		spawnpoint->Spawn();
		spawnpoint->Activate();
		
		state.m_ExtraSpawnPoints.emplace_back(spawnpoint);
		
		DevMsg("Parse_ExtraSpawnPoint: #%d, %08x, name \"%s\", teamnum %d, origin [ % 7.1f % 7.1f % 7.1f ], angles [ % 7.1f % 7.1f % 7.1f ]\n",
			ENTINDEX(spawnpoint), (uintptr_t)spawnpoint, STRING(spawnpoint->GetEntityName()), spawnpoint->GetTeamNumber(),
			VectorExpand(spawnpoint->GetAbsOrigin()), VectorExpand(spawnpoint->GetAbsAngles()));
	}
	
	void Parse_ExtraTankPath(KeyValues *kv)
	{
		const char *name = nullptr; // required
		std::vector<Vector> points; // required
		
		FOR_EACH_SUBKEY(kv, subkey) {
			if (FStrEq(subkey->GetName(), "Name")) {
				name = subkey->GetString();
			} else if (FStrEq(subkey->GetName(), "Node")) {
				Vector point; char c;
				if (sscanf(subkey->GetString(), " %f %f %f %c", &point.x, &point.y, &point.z, &c) == 3) {
					points.push_back(point);
				} else {
					Warning("Invalid value \'%s\' for Node key in ExtraTankPath block.\n", subkey->GetString());
				}
			} else {
				Warning("Unknown key \'%s\' in ExtraTankPath block.\n", subkey->GetName());
			}
		}
		
		bool fail = false;
		if (name == nullptr) {
			Warning("Missing Name key in ExtraTankPath block.\n");
			fail = true;
		}
		if (points.empty()) {
			Warning("No Node keys specified in ExtraTankPath block.\n");
			fail = true;
		}
		if (fail) return;
		
		state.m_ExtraTankPaths.emplace_back(name);
		auto& path = state.m_ExtraTankPaths.back();
		
		for (const auto& point : points) {
			if (!path.AddNode(point)) {
				Warning("Parse_ExtraTankPath: ExtraTankPath::AddNode reports that entity creation failed!\n");
				state.m_ExtraTankPaths.pop_back();
				return;
			}
		}
		
		// Is it actually OK that we're spawning this stuff during parsing...?
		path.SpawnNodes();
		
		DevMsg("Parse_ExtraTankPath: name \"%s\", %zu nodes\n", name, points.size());
	}

	void InsertFixupPattern(std::string &str, std::set<std::string> &entity_names) {
		int oldpos = 0;
		while(oldpos < str.size()){
			int commapos = str.find(',',oldpos);
			int spacepos = str.find(' ',oldpos);
			int colonpos = str.find(':',oldpos);
			int pos;
			if (colonpos != -1 && (spacepos == -1 || colonpos < spacepos) && (commapos == -1 || colonpos < commapos))
				pos = colonpos;
			else if (commapos != -1 && (spacepos == -1 || commapos < spacepos))
				pos = commapos;
			else
				pos = spacepos;
			if (pos == -1)
				pos = str.size();
			std::string strsub = str.substr(oldpos,pos-oldpos);
			if (entity_names.find(strsub) != entity_names.end()){
				str.insert(pos,"&");
				//templ.fixup_names.insert(str);
			}
			DevMsg("pos %d %d %s", oldpos, pos, strsub.c_str());
			oldpos = pos+1;
		}
	}
	void Parse_PointTemplate(KeyValues *kv)
	{
		std::string tname = kv->GetName();
		std::set<std::string> entity_names;
		DevMsg("Template %s\n", tname.c_str());
		PointTemplate &templ = Point_Templates().emplace(tname,PointTemplate()).first->second;
		templ.name = tname;
		bool hasParentSpecialName = false;
		std::multimap<std::string,std::string> onspawn;
		std::multimap<std::string,std::string> onkilled;
		FOR_EACH_SUBKEY(kv, subkey) {

			const char *cname = subkey->GetName();
			if (FStrEq(cname, "OnSpawnOutput") || FStrEq(cname, "OnParentKilledOutput")){
				
				//auto input = templ.onspawn_inputs.emplace(templ.onspawn_inputs.end());
				std::string target;
				std::string action;
				float delay;
				std::string param;
				FOR_EACH_SUBKEY(subkey, subkey2) {
					const char *name = subkey2->GetName();
					if (FStrEq(name, "Target")) {
						target = subkey2->GetString();
					}
					else if (FStrEq(name, "Action")) {
						action = subkey2->GetString();
					}
					else if (FStrEq(name, "Delay")) {
						delay = subkey2->GetFloat();
					}
					else if (FStrEq(name, "Param")) {
						param = subkey2->GetString();
					}
				}
				
				std::string trig = CFmtStr("%s,%s,%s,%f,1",target.c_str(),action.c_str(),param.c_str(),delay).Get();
				
				if (FStrEq(cname, "OnSpawnOutput"))
					onspawn.insert({"ontrigger",trig});
				else{
					templ.has_on_kill_trigger = true;
					templ.on_kill_triggers.push_back(trig);
				}
				
				//DevMsg("Added onspawn %s %s \n", input->target.c_str(), input->input.c_str());
			}
			else if (FStrEq(cname, "NoFixup")){
				templ.no_fixup = subkey->GetBool();
			}
			else if (FStrEq(cname, "KeepAlive")){
				templ.keep_alive = subkey->GetBool();
			}
			else {
				std::multimap<std::string,std::string> keyvalues;

				keyvalues.insert({"classname", cname});

				FOR_EACH_SUBKEY(subkey, subkey2) {
					const char *name = subkey2->GetName();
					if (FStrEq(name,"connections")){
						FOR_EACH_SUBKEY(subkey2, subkey3) {
							keyvalues.insert({subkey3->GetName(),subkey3->GetString()});
						}
					}
					else if (!FStrEq(name,"editor"))
						keyvalues.insert({name,subkey2->GetString()});
				}

				if (keyvalues.find("targetname") != keyvalues.end())
					entity_names.insert(keyvalues.find("targetname")->second);
				templ.entities.push_back(keyvalues);
			
				
				DevMsg("add Entity %s to template %s\n", cname, tname.c_str());
			}
		}
		if (!onspawn.empty()){
			onspawn.insert({"classname", "logic_relay"});
			onspawn.insert({"targetname", "trigger_spawn_relay"});
			onspawn.insert({"spawnflags", "2"});
			templ.entities.push_back(onspawn);
			entity_names.insert(onspawn.find("targetname")->second);
		}
		if (!templ.no_fixup) {
			for (auto it = templ.on_kill_triggers.begin(); it != templ.on_kill_triggers.end(); ++it){
				InsertFixupPattern(*it, entity_names);
			}
			for (auto it = templ.entities.begin(); it != templ.entities.end(); ++it){
				for (auto it2 = it->begin(); it2 != it->end(); ++it2){
					std::string str=it2->second;
					
					if (!hasParentSpecialName && str.find("!parent") != -1)
						hasParentSpecialName = true;
					
					InsertFixupPattern(str, entity_names);
					it2->second = str;
				}
			}
		}

		templ.has_parent_name = hasParentSpecialName;
		

		CBaseEntity *maker = CreateEntityByName("env_entity_maker");
		maker->KeyValue("EntityTemplate",tname.c_str());
		maker->KeyValue("targetname",tname.c_str());
		DispatchSpawn(maker);
		maker->Activate();

		
	}

	void Parse_PointTemplates(KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			Parse_PointTemplate(subkey);
		}
		
		DevMsg("Parse_PointTemplates\n");
	}
	
	void Parse_PlayerAttributes(KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			int classname = 0;
			for(int i=1; i < 10; i++){
				if(FStrEq(g_aRawPlayerClassNames[i],subkey->GetName())){
					classname=i;
					break;
				}
			}
			if (classname == 0) {
				if (GetItemSchema()->GetAttributeDefinitionByName(subkey->GetName()) != nullptr) {
					state.m_PlayerAttributes[subkey->GetName()] = subkey->GetFloat();
					DevMsg("Parsed attribute %s %f\n", subkey->GetName(),subkey->GetFloat());
				}
			}
			else 
			{
				state.m_bDeclaredClassAttrib = true;
				FOR_EACH_SUBKEY(subkey, subkey2) {
					if (GetItemSchema()->GetAttributeDefinitionByName(subkey2->GetName()) != nullptr) {
						state.m_PlayerAttributesClass[classname][subkey2->GetName()] = subkey2->GetFloat();
						DevMsg("Parsed attribute %s %f\n", subkey2->GetName(),subkey2->GetFloat());
					}
				}
			}
		}
		
		DevMsg("Parsed attributes\n");
	}
	void Parse_PlayerAddCond(KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			if (FStrEq(name, "Index")) {
				state.m_PlayerAddCond.push_back((ETFCond)subkey->GetInt());
			} else if (FStrEq(name, "Name")) {
				ETFCond cond = GetTFConditionFromName(subkey->GetString());
				if (cond != -1) {
					state.m_PlayerAddCond.push_back(cond);
				} else {
					Warning("Unrecognized condition name \"%s\" in AddCond block.\n", subkey->GetString());
				}
			}
		}
		
		DevMsg("Parsed addcond\n");
	}
	void Parse_OverrideSounds(KeyValues *kv)
	{
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			state.m_OverrideSounds[subkey->GetName()]=subkey->GetString();
		}
		
		DevMsg("Parsed addcond\n");
	}
	/*DETOUR_DECL_MEMBER(void, CTFPlayer_ReapplyPlayerUpgrades)
	{
		DETOUR_MEMBER_CALL(CTFPlayer_ReapplyPlayerUpgrades)();
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DevMsg("Adding attributes\n");
		for(auto it = state.m_PlayerAttributes.begin(); it != state.m_PlayerAttributes.end(); ++it){
			player->AddCustomAttribute("hidden maxhealth non buffed",it->second, 60.0f);
			player->AddCustomAttribute("move speed bonus",2.0f, 60.0f);
			player->AddCustomAttribute("fire rate bonus",0.5f, 60.0f);
			player->AddCustomAttribute(it->first.c_str(),it->second, 7200);
			DevMsg("Added attribute %s %f\n",it->first.c_str(), it->second);
		}
	}*/

	RefCount rc_CPopulationManager_Parse;
	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
	//	DevMsg("CPopulationManager::Parse\n");
		ForEachTFPlayer([&](CTFPlayer *player){
					if (!player->IsAlive()) return;
					if (state.m_bRedPlayersRobots || state.m_bBluPlayersRobots){
						player->GetPlayerClass()->SetCustomModel("", true);
						player->UpdateModel();
					}
				});
		state.Reset();
		
		SCOPED_INCREMENT(rc_CPopulationManager_Parse);
		return DETOUR_MEMBER_CALL(CPopulationManager_Parse)();
	}
	
	RefCount rc_KeyValues_LoadFromFile;
	DETOUR_DECL_MEMBER(bool, KeyValues_LoadFromFile, IBaseFileSystem *filesystem, const char *resourceName, const char *pathID, bool refreshCache)
	{
	//	DevMsg("KeyValues::LoadFromFile\n");
		
		++rc_KeyValues_LoadFromFile;
		auto result = DETOUR_MEMBER_CALL(KeyValues_LoadFromFile)(filesystem, resourceName, pathID, refreshCache);
		--rc_KeyValues_LoadFromFile;
		
		if (result && rc_CPopulationManager_Parse > 0 && rc_KeyValues_LoadFromFile == 0) {
			auto kv = reinterpret_cast<KeyValues *>(this);
			
			std::vector<KeyValues *> del_kv;
			FOR_EACH_SUBKEY(kv, subkey) {
				const char *name = subkey->GetName();
				
				bool del = true;
				if (FStrEq(name, "BotsDropSpells")) {
					state.m_SpellsEnabled.Set(subkey->GetBool());
				} else if (FStrEq(name, "GiantsDropRareSpells")) {
					state.m_bGiantsDropRareSpells = subkey->GetBool();
				} else if (FStrEq(name, "SpellDropRateCommon")) {
					state.m_flSpellDropRateCommon = Clamp(subkey->GetFloat(), 0.0f, 1.0f);
				} else if (FStrEq(name, "SpellDropRateGiant")) {
					state.m_flSpellDropRateGiant = Clamp(subkey->GetFloat(), 0.0f, 1.0f);
				} else if (FStrEq(name, "NoReanimators")) {
					state.m_bNoReanimators = subkey->GetBool();
				} else if (FStrEq(name, "NoMvMDeathTune")) {
					state.m_bNoMvMDeathTune = subkey->GetBool();
				} else if (FStrEq(name, "SniperHideLasers")) {
					state.m_bSniperHideLasers = subkey->GetBool();
				} else if (FStrEq(name, "SniperAllowHeadshots")) {
					state.m_bSniperAllowHeadshots = subkey->GetBool();
				} else if (FStrEq(name, "DisableUpgradeStations")) {
					state.m_bDisableUpgradeStations = subkey->GetBool();
				} else if (FStrEq(name, "RemoveGrapplingHooks")) {
					state.m_flRemoveGrapplingHooks = subkey->GetFloat();
				} else if (FStrEq(name, "ReverseWinConditions")) {
					state.m_bReverseWinConditions = subkey->GetBool();
				} else if (FStrEq(name, "MedievalMode")) {
					state.m_MedievalMode.Set(subkey->GetBool());
				} else if (FStrEq(name, "GrapplingHook")) {
					state.m_GrapplingHook.Set(subkey->GetBool());
				} else if (FStrEq(name, "RespecEnabled")) {
					state.m_RespecEnabled.Set(subkey->GetBool());
				} else if (FStrEq(name, "RespecLimit")) {
					state.m_RespecLimit.Set(subkey->GetInt());
				} else if (FStrEq(name, "BonusRatioHalf")) {
					state.m_BonusRatioHalf.Set(subkey->GetFloat());
				} else if (FStrEq(name, "BonusRatioFull")) {
					state.m_BonusRatioFull.Set(subkey->GetFloat());
				} else if (FStrEq(name, "FixedBuybacks")) {
					state.m_FixedBuybacks.Set(subkey->GetBool());
				} else if (FStrEq(name, "BuybacksPerWave")) {
					state.m_BuybacksPerWave.Set(subkey->GetInt());
				} else if (FStrEq(name, "DeathPenalty")) {
					state.m_DeathPenalty.Set(subkey->GetInt());
				} else if (FStrEq(name, "SentryBusterFriendlyFire")) {
					state.m_SentryBusterFriendlyFire.Set(subkey->GetBool());
				} else if (FStrEq(name, "BotPushaway")) {
					state.m_BotPushaway.Set(subkey->GetBool());
				} else if (FStrEq(name, "HumansMustJoinTeam")) {
					state.m_HumansMustJoinTeam.Set(subkey->GetString());
					if (FStrEq(subkey->GetString(),"blue")){
						ForEachTFPlayer([&](CTFPlayer *player){
							if(player->GetTeamNumber() == TF_TEAM_RED && !player->IsBot()){
								player->ForceChangeTeam(3,true);
							}
						});
					}
				} else if (FStrEq(name, "AllowJoinTeamBlue")) {
					state.m_AllowJoinTeamBlue.Set(subkey->GetBool());
				} else if (FStrEq(name, "AllowJoinTeamBlueMax")) {
					state.m_AllowJoinTeamBlueMax.Set(subkey->GetInt());
				} else if (FStrEq(name, "BluHumanFlagPickup")) {
					state.m_BluHumanFlagPickup.Set(subkey->GetBool());
				} else if (FStrEq(name, "BluHumanFlagCapture")) {
					state.m_BluHumanFlagCapture.Set(subkey->GetBool());
				} else if (FStrEq(name, "BluPlayersAreRobots")) {
					state.m_bBluPlayersRobots = subkey->GetBool();
				} else if (FStrEq(name, "RedPlayersAreRobots")) {
					state.m_bRedPlayersRobots = subkey->GetBool();
				} else if (FStrEq(name, "FixSetCustomModelInput")) {
					state.m_bFixSetCustomModelInput = subkey->GetBool();
				} else if (FStrEq(name, "BotsAreHumans")) {
					state.m_BotsHumans.Set(subkey->GetBool());
				} else if (FStrEq(name, "SetCreditTeam")) {
					state.m_SetCreditTeam.Set(subkey->GetInt());
				} else if (FStrEq(name, "EnableDominations")) {
					state.m_EnableDominations.Set(subkey->GetBool());
				} else if (FStrEq(name, "VanillaMode")) {
					state.m_VanillaMode.Set(subkey->GetBool());
				} else if (FStrEq(name, "BodyPartScaleSpeed")) {
					state.m_BodyPartScaleSpeed.Set(subkey->GetFloat());
				} else if (FStrEq(name, "SandmanStun")) {
					state.m_SandmanStuns.Set(subkey->GetBool());
				} else if (FStrEq(name, "MedigunShieldDamage")) {
					state.m_MedigunShieldDamage.Set(subkey->GetBool());
				} else if (FStrEq(name, "StandableHeads")) {
					state.m_StandableHeads.Set(subkey->GetBool());
				} else if (FStrEq(name, "NoRomevisionCosmetics")) {
					state.m_NoRomevisionCosmetics.Set(subkey->GetBool());
				} else if (FStrEq(name, "ForceHoliday")) {
					DevMsg("Forcing holiday\n");
					CBaseEntity *ent = CreateEntityByName("tf_logic_holiday");
					ent->KeyValue("holiday_type", "2");
					DispatchSpawn(ent);
					ent->Activate();
					state.m_ForceHoliday.Set(FStrEq(subkey->GetString(),"Halloween") ? 2 : subkey->GetInt());
				} else if (FStrEq(name, "RobotLimit")) {
					state.m_RobotLimit.Set(subkey->GetInt());
				} else if (FStrEq(name, "CustomUpgradesFile")) {
					//static const std::string prefix("download/scripts/items/");
					state.m_CustomUpgradesFile.Set(subkey->GetString());
				} else if (FStrEq(name, "DisableSound")) {
					state.m_DisableSounds.emplace(subkey->GetString());
				} else if (FStrEq(name, "SendBotsToSpectatorImmediately")) {
					state.m_bSendBotsToSpectatorImmediately = subkey->GetBool();
				} else if (FStrEq(name, "ItemWhitelist")) {
					Parse_ItemWhitelist(subkey);
				} else if (FStrEq(name, "ItemBlacklist")) {
					Parse_ItemBlacklist(subkey);
				} else if (FStrEq(name, "ItemAttributes")) {
					Parse_ItemAttributes(subkey);
				} else if (FStrEq(name, "ClassLimit")) {
					Parse_ClassBlacklist(subkey);
			//	} else if (FStrEq(name, "DisallowedItems")) {
			//		Parse_DisallowedItems(subkey);
				} else if (FStrEq(name, "FlagResetTime")) {
					Parse_FlagResetTime(subkey);
				} else if (FStrEq(name, "ExtraSpawnPoint")) {
					Parse_ExtraSpawnPoint(subkey);
				} else if (FStrEq(name, "ExtraTankPath")) {
					Parse_ExtraTankPath(subkey);
				} else if (FStrEq(name, "PointTemplates")) {
					Parse_PointTemplates(subkey);
				} else if (FStrEq(name, "PlayerAttributes")) {
					Parse_PlayerAttributes(subkey);
				} else if (FStrEq(name, "PlayerAddCond")) {
					Parse_PlayerAddCond(subkey);
				} else if (FStrEq(name, "OverrideSounds")) {
					Parse_OverrideSounds(subkey);
				} else if (FStrEq(name, "SpawnTemplate")) {
					Parse_SpawnTemplate(subkey).SpawnTemplate(nullptr);
				} else if (FStrEq(name, "PrecacheScriptSound"))  { CBaseEntity::PrecacheScriptSound (subkey->GetString());
				} else if (FStrEq(name, "PrecacheSound"))        { enginesound->PrecacheSound       (subkey->GetString(), false);
				} else if (FStrEq(name, "PrecacheModel"))        { engine     ->PrecacheModel       (subkey->GetString(), false);
				} else if (FStrEq(name, "PrecacheSentenceFile")) { engine     ->PrecacheSentenceFile(subkey->GetString(), false);
				} else if (FStrEq(name, "PrecacheDecal"))        { engine     ->PrecacheDecal       (subkey->GetString(), false);
				} else if (FStrEq(name, "PrecacheGeneric"))      { engine     ->PrecacheGeneric     (subkey->GetString(), false);
				} else if (FStrEq(name, "PreloadSound"))         { enginesound->PrecacheSound       (subkey->GetString(), true);
				} else if (FStrEq(name, "PreloadModel"))         { engine     ->PrecacheModel       (subkey->GetString(), true);
				} else if (FStrEq(name, "PreloadSentenceFile"))  { engine     ->PrecacheSentenceFile(subkey->GetString(), true);
				} else if (FStrEq(name, "PreloadDecal"))         { engine     ->PrecacheDecal       (subkey->GetString(), true);
				} else if (FStrEq(name, "PreloadGeneric"))       { engine     ->PrecacheGeneric     (subkey->GetString(), true);
				} else {
					del = false;
				}
				
				if (del) {
				//	DevMsg("Key \"%s\": processed, will delete\n", name);
					del_kv.push_back(subkey);
				} else {
				//	DevMsg("Key \"%s\": passthru\n", name);
				}
			}
			
			for (auto subkey : del_kv) {
			//	DevMsg("Deleting key \"%s\"\n", subkey->GetName());
				kv->RemoveSubKey(subkey);
				subkey->deleteThis();
			}
		}
		
		return result;
	}
	
	void Handler(int sig) {
		fprintf(stderr,"sigsegv error %d",sig);
		backtrace();
		exit(0);
	}
	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Pop:PopMgr_Extensions")
		{
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_PlayerKilled,                     "CTFGameRules::PlayerKilled");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ShouldDropSpellPickup,            "CTFGameRules::ShouldDropSpellPickup");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_DropSpellPickup,                  "CTFGameRules::DropSpellPickup");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_IsUsingSpells,                    "CTFGameRules::IsUsingSpells");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ReapplyItemUpgrades,               "CTFPlayer::ReapplyItemUpgrades");
			MOD_ADD_DETOUR_STATIC(CTFReviveMarker_Create,                        "CTFReviveMarker::Create");
			MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_CreateSniperDot,                "CTFSniperRifle::CreateSniperDot");
			MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_CanFireCriticalShot,            "CTFSniperRifle::CanFireCriticalShot");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_CanFireCriticalShot,             "CTFWeaponBase::CanFireCriticalShot");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_StrikeTarget,              "CTFProjectile_Arrow::StrikeTarget");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_IsPVEModeControlled,              "CTFGameRules::IsPVEModeControlled");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_UpgradeTouch,                        "CUpgrades::UpgradeTouch");
			MOD_ADD_DETOUR_MEMBER(CTeamplayRoundBasedRules_BroadcastSound,       "CTeamplayRoundBasedRules::BroadcastSound");
			MOD_ADD_DETOUR_STATIC(CBaseEntity_EmitSound_static_emitsound,        "CBaseEntity::EmitSound [static: emitsound]");
			MOD_ADD_DETOUR_STATIC(CBaseEntity_EmitSound_static_emitsound_handle, "CBaseEntity::EmitSound [static: emitsound + handle]");
		//	MOD_ADD_DETOUR_MEMBER(CTFPlayer_GiveDefaultItems,                    "CTFPlayer::GiveDefaultItems");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GiveNamedItem,                       "CTFPlayer::GiveNamedItem");
		//	MOD_ADD_DETOUR_MEMBER(CTFPlayer_ItemIsAllowed,                       "CTFPlayer::ItemIsAllowed");
			MOD_ADD_DETOUR_MEMBER(CCaptureFlag_GetMaxReturnTime,                 "CCaptureFlag::GetMaxReturnTime");
			MOD_ADD_DETOUR_MEMBER(CEnvEntityMaker_SpawnEntity,                   "CEnvEntityMaker::SpawnEntity");
			MOD_ADD_DETOUR_MEMBER(CEnvEntityMaker_InputForceSpawn,               "CEnvEntityMaker::InputForceSpawn");
			MOD_ADD_DETOUR_MEMBER(CEnvEntityMaker_InputForceSpawnAtEntityOrigin,               "CEnvEntityMaker::InputForceSpawnAtEntityOrigin");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_HandleCommand_JoinClass,             "CTFPlayer::HandleCommand_JoinClass");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_OnPlayerSpawned,                  "CTFGameRules::OnPlayerSpawned");
			//MOD_ADD_DETOUR_MEMBER(CVoteController_Spawn,                  "CVoteController::Spawn");
			MOD_ADD_DETOUR_MEMBER(CBaseIssue_ctor,                  "CBaseIssue::CBaseIssue");
			//MOD_ADD_DETOUR_MEMBER(DoTeleporterOverride,                          "DoTeleporterOverride");
			
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ctor,                    "CTFGameRules::CTFGameRules [C1]");
		//	MOD_ADD_DETOUR_MEMBER(CTFGameRules_SetWinningTeam,          "CTFGameRules::SetWinningTeam");
			MOD_ADD_DETOUR_MEMBER(CTeamplayRoundBasedRules_State_Enter, "CTeamplayRoundBasedRules::State_Enter");
		//	MOD_ADD_DETOUR_MEMBER(CTFGameRules_FireGameEvent,           "CTFGameRules::FireGameEvent");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ChangeTeam,                 "CTFPlayer::ChangeTeam");
			//MOD_ADD_DETOUR_STATIC(UserMessageBegin,                 "UserMessageBegin");
			//MOD_ADD_DETOUR_STATIC(MessageWriteString,                 "MessageWriteString");
			
		//	MOD_ADD_DETOUR_MEMBER(CPopulationManager_PostInitialize, "CPopulationManager::PostInitialize");
			
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse, "CPopulationManager::Parse");
			MOD_ADD_DETOUR_MEMBER(KeyValues_LoadFromFile,   "KeyValues::LoadFromFile");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetOverrideStepSound, "CTFPlayer::GetOverrideStepSound");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetSceneSoundToken, "CTFPlayer::GetSceneSoundToken");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_InputSetCustomModel, "CTFPlayer::InputSetCustomModel");
			signal(SIGSEGV, Handler);
		}
		
		virtual void OnUnload() override
		{
			state.Reset();
		}
		
		virtual void OnDisable() override
		{
			state.Reset();
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void LevelInitPreEntity() override
		{
			state.Reset();
			state.m_PlayerUpgradeSend.clear();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			state.Reset();
			state.m_PlayerUpgradeSend.clear();
		}
		
		virtual void FrameUpdatePostEntityThink() override
		{
			if (!TFGameRules()->IsMannVsMachineMode()) return;
			
			if (state.m_flRemoveGrapplingHooks >= 0.0f) {
				ForEachEntityByRTTI<CTFProjectile_GrapplingHook>([](CTFProjectile_GrapplingHook *proj){
					float dt = gpGlobals->curtime - proj->m_flTimeInit;
					if (dt > state.m_flRemoveGrapplingHooks) {
						DevMsg("Removing tf_projectile_grapplinghook #%d (alive for %.3f seconds)\n", ENTINDEX(proj), dt);
						proj->Remove();
					}
				});
			}
			if (state.m_PlayerAttributes.size() > 0 || state.m_PlayerAddCond.size() > 0 || state.m_bDeclaredClassAttrib)
				ForEachTFPlayer([&](CTFPlayer *player){
					if (player->GetTeamNumber() != TF_TEAM_RED) return;
					if (!player->IsAlive()) return;
					int classname = player->GetPlayerClass()->GetClassIndex();
					for(auto it = state.m_PlayerAttributes.begin(); it != state.m_PlayerAttributes.end(); ++it){
						player->AddCustomAttribute(it->first.c_str(),it->second, 7200.0f);
					}
					
					for(auto it = state.m_PlayerAttributesClass[classname].begin(); it != state.m_PlayerAttributesClass[classname].end(); ++it){
						player->AddCustomAttribute(it->first.c_str(),it->second, 7200.0f);
					}
					for(auto cond : state.m_PlayerAddCond){
						if (!player->m_Shared->InCond(cond)){
							player->m_Shared->AddCond(cond,-1.0f);
						}
					}
				});

			if (bot_killed_check) {
				bool need_check_again = false;
				ForEachTFPlayer([&](CTFPlayer *player){
					if (!player->IsAlive() && player->IsBot()){
						if(gpGlobals->curtime - player->GetDeathTime() > 0.1f)
							player->ForceChangeTeam(1,false);
						else
							need_check_again = true;
					}
				});
				bot_killed_check = need_check_again;
			}
			received_message_tick = false;
			Update_Point_Templates();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_popmgr_extensions", "0", FCVAR_NOTIFY,
		"Mod: enable extended KV in CPopulationManager::Parse",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	class CKVCond_PopMgr : public IKVCond
	{
	public:
		virtual bool operator()() override
		{
			return s_Mod.IsEnabled();
		}
	};
	CKVCond_PopMgr cond;
	
	
//	// REMOVE ME
//	#warning REMOVE ME
//	CON_COMMAND(sig_test_stopsound, "")
//	{
//		if (args.ArgC() < 3) {
//			Warning("Expected 2 parameters: sound, flags.\n");
//			return;
//		}
//		
//		int team = -1;
//		const char *snd = args[1];
//		int flags = std::stol(args[2], nullptr, 0);
//		
//		IGameEvent *event = gameeventmanager->CreateEvent("teamplay_broadcast_audio");
//		if (event != nullptr) {
//			event->SetInt   ("team",             team);
//			event->SetString("sound",            snd);
//			event->SetInt   ("additional_flags", flags);
//			gameeventmanager->FireEvent(event);
//			
//			Msg("Fired event: teamplay_broadcast_audio\n"
//				"- team:             %d\n"
//				"- sound:            \"%s\"\n"
//				"- additional_flags: 0x%08x\n",
//				team, snd, flags);
//		}
//		
//		// nope: kills all sounds
//	}
}
