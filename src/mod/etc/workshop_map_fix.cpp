#include "mod.h"
#include "stub/gamerules.h"
#include "mod/common/commands.h"
#include "util/misc.h"
#include <filesystem>
#include <sys/stat.h>
#include <sys/inotify.h>
#include "util/vi.h"

struct ItemInstalled_t
{
	enum { k_iCallback = 5 };
	AppId_t m_unAppID;
	uint64_t m_nPublishedFileId;
};


class SVC_ServerInfo
{
public:
    uintptr_t vtable;
    uintptr_t pad;
	bool				m_bReliable;	// true if message should be send reliable
	INetChannel			*m_NetChannel;	// netchannel this message is from/for
    IServerMessageHandler *m_pMessageHandler;
    int			m_nProtocol;	// protocol version
	int			m_nServerCount;	// number of changelevels since server start
	bool		m_bIsDedicated;  // dedicated server ?	
	bool		m_bIsHLTV;		// HLTV server ?
	bool		m_bIsReplay;	// Replay server ?
	char		m_cOS;			// L = linux, W = Win32
	CRC32_t		m_nMapCRC;		// server map CRC (only used by older demos)
	MD5Value_t	m_nMapMD5;		// server map MD5
	int			m_nMaxClients;	// max number of clients on server
	int			m_nMaxClasses;	// max number of server classes
	int			m_nPlayerSlot;	// our client slot number
	float		m_fTickInterval;// server tick interval
	const char	*m_szGameDir;	// game directory eg "tf2"
	const char	*m_szMapName;	// name of current map 
};

namespace Mod::Etc::Workshop_Map_Fix
{
	std::unordered_set<uint64_t> scanMapList;
	std::unordered_map<std::string, std::string> mapNameToWorkshopName;

	const char *game_path;

	std::filesystem::path ugcPath;

	bool searchPathAdded = false;
	
	ConVar sig_mvm_workshop_map_fix_install_location("sig_etc_workshop_map_fix_install_location", "maps", FCVAR_NOTIFY,
		"Install location for workshop maps links relative to tf directory. Creates missing directories", 
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			filesystem->CreateDirHierarchy(sig_mvm_workshop_map_fix_install_location.GetString(), "DEFAULT_WRITE_PATH");
		});
	
	ConVar sig_mvm_workshop_map_fix_remove_old_links("sig_etc_workshop_map_fix_fix_remove_old_links", "1", FCVAR_NOTIFY,
		"Remove workshop links from install location than are not currently added");

	void RemoveOldLinks() {
		std::error_code er;
		if (sig_mvm_workshop_map_fix_remove_old_links.GetBool()) {
			for (auto &entry : std::filesystem::directory_iterator(std::format("{}/{}", game_path, sig_mvm_workshop_map_fix_install_location.GetString()), er)) {
				if (entry.is_symlink() && !scanMapList.contains(atoll(std::filesystem::read_symlink(entry.path(), er).parent_path().stem().c_str())) && std::filesystem::equivalent(std::filesystem::read_symlink(entry.path(), er).parent_path().parent_path(), ugcPath, er)) {
					std::filesystem::remove(entry.path(), er);
				}
			}
		}
	}
	void ScanWorkshopMaps() {
		std::error_code er;
		for (auto id : scanMapList) {
			std::filesystem::path mapPath = std::format("{}/{}",ugcPath.c_str(),id);
			if (std::filesystem::is_directory(mapPath, er)) {
				for (auto &entry2 : std::filesystem::directory_iterator(mapPath, er)) {
					if (entry2.is_regular_file()) {
						
						mapNameToWorkshopName[entry2.path().filename().stem()] = std::format("workshop/{}.ugc{}",entry2.path().filename().stem().c_str(), id);
						std::filesystem::path mapLinkFrom = std::format("{}/{}/{}", game_path, sig_mvm_workshop_map_fix_install_location.GetString(), entry2.path().filename().c_str());
						bool alreadyExists = std::filesystem::equivalent(std::filesystem::read_symlink(mapLinkFrom, er), entry2.path(), er);
						if (!alreadyExists) {
							std::filesystem::remove(mapLinkFrom, er);
							std::filesystem::create_symlink(entry2,mapLinkFrom, er);
						}
					}
				}
			}
		}
	}
	
	bool scheduleScan = false;
    DETOUR_DECL_MEMBER(void, CTFWorkshopMap_OnUGCItemInstalled, ItemInstalled_t *result)
	{
        DETOUR_MEMBER_CALL(result); 
		scanMapList.insert(result->m_nPublishedFileId);
		scheduleScan = true;
    }

    DETOUR_DECL_MEMBER(void, CTFMapsWorkshop_Steam_OnUGCItemInstalled, ItemInstalled_t *result)
	{
        DETOUR_MEMBER_CALL(result); 
		scanMapList.insert(result->m_nPublishedFileId);
		scheduleScan = true;
    }

    DETOUR_DECL_MEMBER(bool, CTFMapsWorkshop_AddMap, uint64_t id)
	{
        auto result = DETOUR_MEMBER_CALL(id); 
		scanMapList.insert(id);
		scheduleScan = true;
		return result;
    }

    DETOUR_DECL_MEMBER(IVEngineServer::eFindMapResult, CVEngineServer_FindMap, char *pMapName, int nMapNameMax)
	{
		static int lastScanTime = 0;
		if (!scanMapList.empty() && lastScanTime != gpGlobals->tickcount) {
			ScanWorkshopMaps();
			lastScanTime = gpGlobals->tickcount;
		}
		return DETOUR_MEMBER_CALL(pMapName, nMapNameMax); 
    }

    DETOUR_DECL_MEMBER(void, CBaseServer_FillServerInfo, SVC_ServerInfo &serverinfo)
	{
        DETOUR_MEMBER_CALL(serverinfo);
		// Force clients to download workshop map
		if (mapNameToWorkshopName.contains(serverinfo.m_szMapName)) {
        	serverinfo.m_szMapName = mapNameToWorkshopName[serverinfo.m_szMapName].c_str();
		}
    }

    DETOUR_DECL_MEMBER(void, CDownloadListGenerator_OnResourcePrecachedFullPath, const char *path)
	{
		// Remove workshop map from download list
		if (mapNameToWorkshopName.contains(STRING(gpGlobals->mapname)) && std::string(path).ends_with(std::format("maps/{}.bsp",STRING(gpGlobals->mapname)))) {
			return;
		}
        DETOUR_MEMBER_CALL(path);
    }

    DETOUR_DECL_MEMBER(void, CTFMapsWorkshop_PrepareLevelResources, char *mapName, int mapNameLen, char *mapFile, int mapFileLen)
	{
		char newMapName[256] = "";
		char newMapFile[MAX_PATH] = "";
		ScanWorkshopMaps();
		// trick the server into updating the workshop map with non workshop map name
		if (mapNameToWorkshopName.contains(mapName)) {
			auto &str = mapNameToWorkshopName[mapName];
			strncpy(newMapName, str.c_str(), sizeof(newMapName));
			strncpy(newMapFile, mapFile, sizeof(newMapFile));
			mapName = newMapName;
			mapNameLen = sizeof(newMapName);
			mapFile = newMapFile;
			mapFileLen = sizeof(newMapFile);
		}
		DETOUR_MEMBER_CALL(mapName, mapNameLen, mapFile, mapFileLen);
    }

	class CMod : public IMod, IModCallbackListener, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Etc:Workshop_Map_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWorkshopMap_OnUGCItemInstalled, "CTFWorkshopMap::OnUGCItemInstalled");
			MOD_ADD_DETOUR_MEMBER(CTFMapsWorkshop_Steam_OnUGCItemInstalled, "CTFMapsWorkshop::Steam_OnUGCItemInstalled");
			MOD_ADD_DETOUR_MEMBER(CTFMapsWorkshop_AddMap, "CTFMapsWorkshop::AddMap");
			MOD_ADD_DETOUR_MEMBER(CBaseServer_FillServerInfo, "CBaseServer::FillServerInfo");
			MOD_ADD_DETOUR_MEMBER(CDownloadListGenerator_OnResourcePrecachedFullPath, "CDownloadListGenerator::OnResourcePrecachedFullPath");
			MOD_ADD_DETOUR_MEMBER(CVEngineServer_FindMap, "CVEngineServer::FindMap");
			MOD_ADD_DETOUR_MEMBER(CTFMapsWorkshop_PrepareLevelResources, "CTFMapsWorkshop::PrepareLevelResources");
		}
		
		virtual void OnEnable() override
		{
			ScanWorkshopMaps();
		}

		virtual bool OnLoad() override
		{
			game_path = g_SMAPI->GetBaseDir();
			int param = CommandLine()->FindParm("-ugcpath");
			char path[MAX_PATH];
			if (param == 0) {
				filesystem->RelativePathToFullPath("steamapps/workshop/content/440", "BASE_PATH", path, sizeof( path ) );
			}
			else {
				filesystem->RelativePathToFullPath(CommandLine()->GetParm(param + 1), "DEFAULT_WRITE_PATH", path, sizeof( path ) );
				strncat(path, "/workshop/content/440", sizeof(path));
			}
			ugcPath = path;
			return true;
		}

		virtual void LevelShutdownPreEntity() {
			ScanWorkshopMaps();
		}

		CountdownTimerInline removeOldLinksTimer;
		virtual void LevelInitPreEntity() {
			removeOldLinksTimer.Start(1.0f);
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void FrameUpdatePostEntityThink() override
		{
			if (scheduleScan) {
				scheduleScan = false;
				ScanWorkshopMaps();
			}
			if (removeOldLinksTimer.HasStarted() && removeOldLinksTimer.IsElapsed()) {
				RemoveOldLinks();
			}
		}
	};
	CMod s_Mod;

	ConVar cvar_enable("sig_etc_workshop_map_fix", "0", FCVAR_NOTIFY,
		"Mod: automatically install workshop maps into maps directory, allowing to use workshop maps without prefix. Need to run tf_workshop_map_sync command for each workshop map. Clients can still benefit from workshop download",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
