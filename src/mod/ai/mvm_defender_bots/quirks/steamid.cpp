#include "mod.h"
#include "stub/tfbot.h"
//#include "util/backtrace.h"


/* from Open Steamworks */
inline const char *CSteamID::Render() const // renders this steam ID to string using the new rendering style
{
	const int k_cBufLen = 37;
	const int k_cBufs = 4;
	char* pchBuf;

	static char rgchBuf[k_cBufs][k_cBufLen];
	static int nBuf = 0;

	pchBuf = rgchBuf[nBuf++];
	nBuf %= k_cBufs;
	
	union SteamID_t
	{
		struct SteamIDComponent_t
		{
#ifdef VALVE_BIG_ENDIAN
			EUniverse			m_EUniverse : 8;	// universe this account belongs to
			unsigned int		m_EAccountType : 4;			// type of account - can't show as EAccountType, due to signed / unsigned difference
			unsigned int		m_unAccountInstance : 20;	// dynamic instance ID
			uint32				m_unAccountID : 32;			// unique account identifier
#else
			uint32				m_unAccountID : 32;			// unique account identifier
			unsigned int		m_unAccountInstance : 20;	// dynamic instance ID
			unsigned int		m_EAccountType : 4;			// type of account - can't show as EAccountType, due to signed / unsigned difference
			EUniverse			m_EUniverse : 8;	// universe this account belongs to
#endif
		} m_comp;

		uint64 m_unAll64Bits;
	} m_steamid;
	m_steamid.m_unAll64Bits = this->ConvertToUint64();

	switch (m_steamid.m_comp.m_EAccountType)
	{
	case k_EAccountTypeAnonGameServer:
		sprintf(pchBuf, "[A:%u:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID, m_steamid.m_comp.m_unAccountInstance);
		break;
	case k_EAccountTypeGameServer:
		sprintf(pchBuf, "[G:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		break;
	case k_EAccountTypeMultiseat:
		sprintf(pchBuf, "[M:%u:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID, m_steamid.m_comp.m_unAccountInstance);
		break;
	case k_EAccountTypePending:
		sprintf(pchBuf, "[P:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		break;
	case k_EAccountTypeContentServer:
		sprintf(pchBuf, "[C:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		break;
	case k_EAccountTypeClan:
		sprintf(pchBuf, "[g:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		break;
	case k_EAccountTypeChat:
		switch (m_steamid.m_comp.m_unAccountInstance & ~k_EChatAccountInstanceMask)
		{
		case k_EChatInstanceFlagClan:
			sprintf(pchBuf, "[c:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
			break;
		case k_EChatInstanceFlagLobby:
			sprintf(pchBuf, "[L:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
			break;
		default:
			sprintf(pchBuf, "[T:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
			break;
		}
		break;
	case k_EAccountTypeInvalid:
		sprintf(pchBuf, "[I:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		break;
	case k_EAccountTypeIndividual:
		sprintf(pchBuf, "[U:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		break;
	default:
		sprintf(pchBuf, "[i:%u:%u]", m_steamid.m_comp.m_EUniverse, m_steamid.m_comp.m_unAccountID);
		break;
	}

	return pchBuf;
}


namespace Mod::AI::MvM_Defender_Bots
{
	std::map<CHandle<CTFBot>, uint64_t> map_bot2id;
	std::map<uint64_t, CHandle<CTFBot>> map_id2bot;
	
	
	void GenerateSteamIDForBot(CHandle<CTFBot> bot)
	{
		static uint32_t serial = 1;
		
		CSteamID id(serial, 0, k_EUniversePublic, k_EAccountTypeInvalid);
		uint64_t id64 = id.ConvertToUint64();
		
		map_bot2id[bot]  = id64;
		map_id2bot[id64] = bot;
		
		++serial;
		
		DevMsg("GenerateSteamIDForBot: #%d \"%s\" => %s\n",
			ENTINDEX(bot), bot->GetPlayerName(), id.Render());
	}
	
	
	CSteamID BotToSteamID(CTFBot *bot)
	{
		auto it = map_bot2id.find(bot);
		if (it == map_bot2id.end()) {
			GenerateSteamIDForBot(bot);
			it = map_bot2id.find(bot);
			assert(it != map_bot2id.end());
		}
		
		return (*it).second;
	}
	
	CTFBot *SteamIDToBot(CSteamID id)
	{
		auto it = map_id2bot.find(id.ConvertToUint64());
		if (it == map_id2bot.end()) {
			return nullptr;
		}
		
		return (*it).second;
	}
	
	
	RefCount rc_Quirk_GetSteamID;
	DETOUR_DECL_MEMBER(bool, CBasePlayer_GetSteamID, CSteamID *pID)
	{
		if (rc_Quirk_GetSteamID > 0) {
			auto player = reinterpret_cast<CBasePlayer *>(this);
			
			CTFBot *bot = ToTFBot(player);
			if (bot != nullptr) {
			//	DevMsg("QUIRK: CBasePlayer::GetSteamID\n");
			//	BACKTRACE();
				
				CSteamID id = BotToSteamID(bot);
				DevMsg("GetSteamID(#%d): %s\n", ENTINDEX(player), id.Render());
				
				*pID = id;
				return true;
			}
		}
		
		return DETOUR_MEMBER_CALL(pID);
	}
	
	
	// TODO: since we are doing fake steam IDs based on ent index,
	// we'll need to reset stats for the steam ID each time the bot joins/changes class/whatever
	
	
	#define AddDetour_Member(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_MEMBER_CALLBACK(detour), GET_MEMBER_INNERPTR(detour)))
	#define AddDetour_Static(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_STATIC_CALLBACK(detour), GET_STATIC_INNERPTR(detour)))
	
	#define AddQuirk_GetSteamID(addr) \
		mod->AddDetour(new CFuncCount(rc_Quirk_GetSteamID, addr))
	
	void AddQuirks_SteamID(IMod *mod)
	{
		AddDetour_Member(CBasePlayer_GetSteamID, "CBasePlayer::GetSteamID");
		
		AddQuirk_GetSteamID("CMannVsMachineStats::GetUpgradeSpending");
		AddQuirk_GetSteamID("CMannVsMachineStats::GetBottleSpending");
		AddQuirk_GetSteamID("CMannVsMachineStats::GetBuyBackSpending");
		AddQuirk_GetSteamID("CMannVsMachineStats::ResetUpgradeSpending");
		AddQuirk_GetSteamID("CPopulationManager::FindOrAddPlayerUpgradeHistory [CTFPlayer *]");
		AddQuirk_GetSteamID("CPopulationManager::FindCheckpointSnapshot [CTFPlayer *]");
		AddQuirk_GetSteamID("CPopulationManager::RemovePlayerAndItemUpgradesFromHistory");
		AddQuirk_GetSteamID("CPopulationManager::GetNumRespecsAvailableForPlayer");
		AddQuirk_GetSteamID("CPopulationManager::SetNumRespecsForPlayer");
		AddQuirk_GetSteamID("CPopulationManager::AddRespecToPlayer");
		AddQuirk_GetSteamID("CPopulationManager::RemoveRespecFromPlayer");
		
		/* crap: we also need to override GetPlayerBySteamID for:
		CMannVsMachineStats::GetBottleSpending
		CMannVsMachineStats::GetBuyBackSpending
		CMannVsMachineStats::GetUpgradeSpending
		(or not: GetPlayerBySteamID just calls CBasePlayer::GetSteamID, and
		we'll already be detouring it from the relevant functions anyway!)
		*/
	}
}
