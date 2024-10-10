#include "mod.h"
#include "stub/misc.h"
#include "soundchars.h"
#include "stub/tf_objective_resource.h"
#include "stub/populators.h"
#include "stub/gamerules.h"
#include "stub/extraentitydata.h"
#include "stub/server.h"
#include "util/misc.h"
#include "util/clientmsg.h"
#include "util/iterate.h"
#include "mod/etc/sendprop_override.h"
#include "mod/common/commands.h"
#include "mod/pop/popmgr_extensions.h"

#include <regex>
//#include <filesystem>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <fmt/core.h>
#include <utime.h>
#include <sys/time.h>


/*

Manual KV file format:

"Downloads"
{
	"Whatever1"
	{
		"Map" ".*"
		// Required key (must have at least one)
		// Uses case-insensitive regex matching
		// Otherwise, add one Map key per map that this block should apply to
		
		"File" "scripts/items/mvm_upgrades_sigsegv_v8.txt"
		// Required key (must have at least one)
		// Can have arbitrarily many
		// Wildcard matching enabled (only for last component of filename, not for directories)
		
		"Precache" "no"
		// Required key (can only appear once)
		// Can specify "No" for no precache
		// Can specify "Generic" for PrecacheGeneric
		// Can specify "Model" for PrecacheModel
		// Can specify "Decal" for PrecacheDecal
		// Can specify "Sound" for PrecacheSound
	}
	
	"Whatever2"
	{
		// ...
	}
}

*/

namespace Mod::Util::Download_Manager
{
	extern ConVar cvar_kvpath;
	extern ConVar cvar_downloadpath;

	extern ConVar cvar_mission_owner;

	void CreateNotifyDirectory();
	void ResetVoteMapList();

	static void ReloadConfigIfModEnabled();
	void GenerateLateDownloadablesCurrentMission();

	bool server_activated = false;
	ConVar cvar_resource_file("sig_util_download_manager_resource_file", "tf_mvm_missioncycle.res", FCVAR_NONE, "Download Manager: Mission cycle file to write to. If set to empty string, vote list is not automatically updated",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			if (server_activated)
				ResetVoteMapList();
		});

	ConVar cvar_mapcycle_file("sig_util_download_manager_maplist_file", "cfg/mapcycle.txt", FCVAR_NONE, "Download Manager: Map cycle file to write to. If set to empty string, vote list is not automatically updated",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			if (server_activated)
				ResetVoteMapList();
		});

	ConVar cvar_download_refresh("sig_util_download_manager_download_refresh", "1", FCVAR_NOTIFY,
		"Download Manager: Set if the downloads should be refreshed periodically", 
		
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			CreateNotifyDirectory();
		});

	ConVar cvar_mappath("sig_util_download_manager_map_path", "", FCVAR_NOTIFY,
		"Download Manager: if specified, only include maps in the directory to the vote menu",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			if (server_activated)
				ResetVoteMapList();
		});

	ConVar cvar_late_download("sig_util_download_manager_late_download", "1", FCVAR_NOTIFY,
		"Download Manager: enable late downloads of small files",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			GenerateLateDownloadablesCurrentMission();
		});

	ConVar cvar_late_download_size_limit("sig_util_download_manager_late_download_max_file_size", "180", FCVAR_NOTIFY,
		"Download Manager: Max file size for late download in kb",true, 0, true, 64 * 1024,
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			GenerateLateDownloadablesCurrentMission();
		});

	ConVar cvar_late_download_other_maps("sig_util_download_manager_late_download_for_other_maps", "0", FCVAR_NOTIFY,
		"Download Manager: Late download files for other maps",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			GenerateLateDownloadablesCurrentMission();
		});

	std::unordered_set<std::string> banned_maps;
	ConVar cvar_banned_maps("sig_util_download_manager_banned_maps", "", FCVAR_NOTIFY,
		"Download Manager: banned map list separated by comma",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			banned_maps.clear();
			std::string str(cvar_banned_maps.GetString());
            boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>(","));

            for (auto &token : tokens) {
                banned_maps.insert(token);
            }
			if (server_activated)
				ResetVoteMapList();
		});

	const char *game_path;
	int base_path_len;
	
	struct DownloadBlock
	{
		DownloadBlock(                     ) = default;
		DownloadBlock(      DownloadBlock& ) = delete;
		DownloadBlock(const DownloadBlock& ) = delete;
		DownloadBlock(      DownloadBlock&&) = default;
		DownloadBlock(const DownloadBlock&&) = delete;
		
		std::string name;
		
		std::vector<std::string> keys_map;
		std::vector<std::string> keys_precache;
		std::vector<std::string> keys_file;
		
		void (*l_precache)(const char *) = [](const char *path){ /* do nothing */ };
	};
	
	bool case_sensitive_toggle = false;
	CTFPlayer *GetMissionOwner()
	{
		CTFPlayer *retplayer = nullptr;
		ForEachTFPlayer([&](CTFPlayer *player) {
			if (player->IsBot()) return true;

			IGamePlayer *smplayer = playerhelpers->GetGamePlayer(player->edict());
			AdminId id = smplayer->GetAdminId();
			
			if (id == INVALID_ADMIN_ID) return true;
			uint count = adminsys->GetAdminGroupCount(id);
			for (uint i = 0; i < count; i++) {
				const char *group;
				adminsys->GetAdminGroup(id, i, &group);
				if (strcmp(group, "Mission Maker") == 0)  {
					retplayer = player;
					return false;
				}
			}
			return true;
		});
		return retplayer;
		//return cvar_mission_owner.GetInt();
	}

	void ReadFileString(FILE *file, char *buf, size_t length)
	{
		char ch;
		size_t i = 0;
		do
		{
			ch = fgetc(file);

			if (EOF == ch || i == length - 1)
			{
				buf[i] = '\0';
				return;
			}

			buf[i++] = ch;

		} while ('\0' != ch);
	}

	template<typename T>
	inline void ReadFile(FILE *file, T &buf)
	{
		fread(&buf, sizeof(buf), 1, file);
	}

	template<typename T>
	inline T ReadFile(FILE *file)
	{
		T buf;
		fread(&buf, sizeof(buf), 1, file);
		return buf;
	}

	KeyValues *KeyValuesFromFile(const char *filepath, const char *resourceName) 
	{
		KeyValues *kv = nullptr;
		FILE *file = fopen(filepath, "r");
		if (file) {
			fseek(file, 0, SEEK_END);
			size_t bufsize = ftell(file);
			fseek(file, 0, SEEK_SET);
			char *buffer = new char[bufsize + 1];
			size_t bytesRead = fread(buffer, 1, bufsize, file);
			buffer[bytesRead] = 0;
			fclose(file);
			kv = new KeyValues("file");
			kv->UsesConditionals(false);
			kv->LoadFromBuffer(resourceName, buffer, filesystem);
			delete[] buffer;
		}
		
		return kv;
	}

	void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
		if(from.empty())
			return;
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
	}

	CHandle<CTFPlayer> mission_owner;
	std::unordered_multimap<int, std::string> missing_files;
	std::unordered_map<std::string, int> missing_files_dirs;
	
	std::vector<std::string> late_dl_files;
	int late_dl_files_current_mission_count = 0;
	std::vector<std::string> late_dl_from_all_maps_files;

	std::unordered_map<std::string, std::vector<std::string>> late_dl_files_per_mission;
	std::unordered_map<uint64_t, std::unordered_set<std::string>> late_dl_download_history;
	
	struct LateDownloadInfo
	{
		bool active = false;
		bool lateDlChecked = false;
		bool lateDlEnabled = true;
		bool lateDlUploadInfoSend = false;
		bool lateDlCurMissionOnlyInformed = false;
		bool lateDlCurMissionOnly = false;
		std::unordered_set<std::string> filesQueried;
		std::deque<std::string> filesToDownload;
		std::vector<std::string> filesDownloading;
		std::unordered_multiset<std::string> iconsDownloading;
	};
	LateDownloadInfo download_infos[MAX_PLAYERS + 1];

	void StopLateFilesToDownloadForPlayer(int player) {
		
		if (player > (int)ARRAYSIZE(download_infos)) return;
		
		auto *netchan = static_cast<CNetChan *>(engine->GetPlayerNetInfo(player));
		if (netchan == nullptr) return;

		auto &info = download_infos[player];
		if (!info.lateDlEnabled || !info.active) return;

		for (auto &file : info.filesToDownload) {
			info.filesQueried.erase(file);
			if (file.starts_with("materials/hud/leaderboard_class_")) {
				auto find = info.iconsDownloading.find(file.substr(strlen("materials/hud/leaderboard_class_"), file.size() - 4 - strlen("materials/hud/leaderboard_class_")));
				if (find != info.iconsDownloading.end()) {
					info.iconsDownloading.erase(find);
				}
			}
		}
		info.filesToDownload.clear();
	}

	void AddLateFilesToDownloadForPlayer(int player, const std::vector<std::string> &files, int curMissionFilesCount = 0) {
		
		if (player > (int)ARRAYSIZE(download_infos)) return;

		auto *netchan = static_cast<CNetChan *>(engine->GetPlayerNetInfo(player));
		if (netchan == nullptr) return;

		auto &info = download_infos[player];
		if (!info.lateDlEnabled) return;

		int i = 0;
		for (auto &file : files) {
			if (i++ >= curMissionFilesCount && info.lateDlCurMissionOnly) return;

			if (late_dl_download_history[((CBaseClient *) sv->GetClient(player - 1))->m_SteamID.ConvertToUint64()].contains(file)) continue;

			auto [it, inserted] = info.filesQueried.insert(file);
			if (inserted) {
				info.active = true;
				info.filesToDownload.push_back(file);
				
				if (file.starts_with("materials/hud/leaderboard_class_")) {
					info.iconsDownloading.insert(file.substr(strlen("materials/hud/leaderboard_class_"), file.size() - 4 - strlen("materials/hud/leaderboard_class_")));
				}
			}
		}
	}

	void WatchMissingFile(std::string &path);

	struct CustomFileEntry
	{
		std::string name;
		size_t size = 0;
	};

	class CustomFileScanner
	{
	public:
		void AddFileIfCustom(std::string value, bool onlyIfExist = false)
		{
			bool v;
			ReplaceAll(value, "\\","/");

			if (files_add_present.contains(value)) return;

			std::string valueLowercase = value;
			boost::algorithm::to_lower(valueLowercase);

			std::string pathfull = game_path;
			pathfull += "/";
			pathfull += cvar_downloadpath.GetString();
			pathfull += "/";
			
			std::string pathfullLowercase = pathfull;

			pathfull += value;
			pathfullLowercase += valueLowercase;

			
			bool customNormalCase = access(pathfull.c_str(), F_OK) == 0;
			bool customLowerCase = access(pathfullLowercase.c_str(), F_OK) == 0;
			bool custom = customNormalCase || customLowerCase;

			bool add = custom; // || (!filesystem->FileExists(value.c_str()) && !onlyIfExist); // Dont add non existing files since no cache

			//Msg("File %s %d %d\n", pathfull.c_str(), add, custom);

			// File is missing, inform the mission maker
			if (!custom) {
				if (!filesystem->FileExists(value.c_str(), "vpks")) {
					if (printmissing && mission_owner != nullptr && !StringEndsWith(value.c_str(),".phy")) {
						missingfilemention = true;
						ClientMsg(mission_owner, "File missing: %s\n", value.c_str());
					}
					WatchMissingFile(value);
				}
			}

			files_add_present.insert(value);

			if (custom && (loadIconsForLateDl || loadOtherForLateDl)) {
				if (filesystem->FileExists(value.c_str(), "vpks")) return;
			}
				
			if (custom) {
				if (StringEndsWith(value.c_str(),".vmt", false)){
					KeyValues *kvroot = KeyValuesFromFile(customNormalCase ? pathfull.c_str() : pathfullLowercase.c_str(), value.c_str());
					KeyValues *kv = kvroot;
					if (kvroot != nullptr) {
						kv = kv->GetFirstSubKey();
						if (kv != nullptr) {
							do {
								if (kv->GetDataType() == KeyValues::TYPE_STRING){
									const char *name = kv->GetName();

									const char *texturename = kv->GetString();

									if (strchr(texturename, '/') != nullptr || strchr(texturename, '\\') != nullptr || StringEndsWith(texturename, ".vtf", false)) {
										char texturepath[256];
										snprintf(texturepath, 256, "materials/%s%s",texturename, !StringEndsWith(texturename, ".vtf", false) ? ".vtf" : "");
										AddFileIfCustom(texturepath);
									}
								}
							}
							while ((kv = kv->GetNextKey()) != nullptr);
						}

						kvroot->deleteThis();
					}
				}
				if (StringEndsWith(value.c_str(),".mdl", false)){
					char texname[128];
					char texdir[128];
					

					FILE *mdlFile = fopen(customNormalCase ? pathfull.c_str() : pathfullLowercase.c_str(),"r");
					//Msg("Model %s\n",value.c_str());
					if (mdlFile != nullptr) {
							
						std::vector<std::string> texdirs;

						fseek(mdlFile,212,SEEK_SET);
						int texdircount = 0;
						ReadFile(mdlFile, texdircount);
						if (texdircount > 8) {
							texdircount = 8;
							//PrintToServer("material directory overload in model %s", value);
						}

						fseek(mdlFile,216,SEEK_SET);

						int offsetdir1 = 0;
						ReadFile(mdlFile, offsetdir1);
						fseek(mdlFile,offsetdir1, SEEK_SET);

						int offsetdir2 = 0;
						ReadFile(mdlFile, offsetdir2);
						fseek(mdlFile,offsetdir2, SEEK_SET);
						
						//Msg("dircount %d\n", texdircount);
						for (int i = 0; i < texdircount; i++) {
							ReadFileString(mdlFile, texdir, 128);
							texdirs.push_back(texdir);
							//Msg("dir %s\n", texdir);
						}
						
						fseek(mdlFile,208,SEEK_SET);
						int offset1 = 0;
						ReadFile(mdlFile, offset1);

						fseek(mdlFile,220,SEEK_SET);
						int texcount = 0;
						ReadFile(mdlFile, texcount);
						//Msg("texcount %d\n", texcount);
						if (texcount > 64) {
							texcount = 64;
							//PrintToServer("material overload in model %s", value);
						}

						fseek(mdlFile,offset1, SEEK_SET);
						int offset2=0;
						ReadFile(mdlFile, offset2);
						if (offset2 > 0) {
							fseek(mdlFile,offset2-4, SEEK_CUR);
							char path[256];
							char pathvmt[256];
							for (int i = 0; i < texcount; i++){
								ReadFileString(mdlFile, texname, 128);
								if (strlen(texname) == 0)
									continue;
								bool found_texture = false;
								for (int j = 0; j < texdircount; j++) {
									std::string &texdirstr = texdirs[j];

									//Format(path,256,"materials/%s%s.vtf",texdir,texname);
									snprintf(pathvmt,256,"materials/%s%s.vmt",texdirstr.c_str() + ((texdirstr[0] == '/' || texdirstr[0] == '\\') ? 1 : 0),texname);
									//AddFileIfCustom(path);
									found_texture = true;
									//Msg("Added model texture materials/%s%s my %s\n",texdir,texname,texname);
									if (filesystem->FileExists(pathvmt)) {
										break;
									}
									
								}
								if (found_texture)
									AddFileIfCustom(pathvmt);
							}
						}
						fclose(mdlFile);
					}
				}
			}

			if (custom) {
				struct stat stats;
				if (readSize) {
					stat(customNormalCase ? pathfull.c_str() : pathfullLowercase.c_str(), &stats);
				}
				files_add.push_back({customNormalCase ? value : valueLowercase, stats.st_size});
			}
		}

		void KeyValueBrowse(KeyValues *kv)
		{
			do {
				auto sub = kv->GetFirstSubKey();
				if (sub != nullptr) {
					KeyValueBrowse(sub);
				}
				else {
					if (kv->GetDataType() == KeyValues::TYPE_STRING) {
						const char *name = kv->GetName();
						//Msg("%s %s\n", name, kv->GetString());
						if ((loadIconsForLateDl || !cvar_late_download.GetBool()) && FStrEq(name, "ClassIcon")) {
							AddFileIfCustom(fmt::format("{}{}{}", "materials/hud/leaderboard_class_",kv->GetString(),".vmt"));
						}
						else if (loadIconsForLateDl && !loadOtherForLateDl) {

						}
						else if(FStrEq(name, "CustomUpgradesFile")) {
							AddFileIfCustom(fmt::format("{}{}","scripts/items/",kv->GetString()));
						}
						else if(StringEndsWith(name, "Decal", false)) {
							AddFileIfCustom(fmt::format("{}{}",kv->GetString(),".vmt"));
						}
						else if(StringEndsWith(name,"Generic", false)){
							AddFileIfCustom(kv->GetString());
						}
						else if(StringEndsWith(name,"Model", false) || FStrEq(name,"HandModelOverride")){
							const char *value = kv->GetString();
							char fmt[200];
							V_strncpy(fmt, value, sizeof(fmt));

							int strlenval = strlen(fmt);

							char *dotpos = strchr(fmt, '.');
							if (dotpos != nullptr) {
								AddFileIfCustom(fmt);
								if (StringEndsWith(fmt, ".mdl", false)) {
									strncpy(fmt + (strlenval-4),".vvd",5);
									AddFileIfCustom(fmt);
									strncpy(fmt + (strlenval-4),".phy",5);
									AddFileIfCustom(fmt, true);
									//strcopy(value[strlenval-4],8,".sw.vtx");
									//AddFileIfCustom(value);
									strncpy(fmt + (strlenval-4),".dx80.vtx",10);
									AddFileIfCustom(fmt);
									strncpy(fmt + (strlenval-4),".dx90.vtx",10);
									AddFileIfCustom(fmt);
								}
							}
						}
						else if(FStrEq(name,"SoundFile") || FStrEq(name, "OverrideSounds") || StringEndsWith(name,"Sound", false) || StringEndsWith(name,"message", false)){
							const char *value = kv->GetString();
							const char *startpos = strchr(value, '|');
							if (startpos != nullptr) {
								value = startpos + 1;
							}
							value = PSkipSoundChars(value);
							if (*value == '#' || *value == '(') {
								value += 1;
							}
							
							if (StringEndsWith(value,".mp3") || StringEndsWith(value,".wav") ) {
								AddFileIfCustom(fmt::format("{}{}","sound/", value));
							}
						}
					}
				}
			}
			while ((kv = kv->GetNextKey()) != nullptr);
		}

		std::unordered_set<std::string> files_add_present;
		std::vector<CustomFileEntry> files_add;
		bool printmissing = false;
		bool missingfilemention = false;
		bool readSize = false;
		bool loadIconsForLateDl = false;
		bool loadOtherForLateDl = false;
	};
	

	std::unordered_set<std::string> popfiles_to_forced_update;
	void ScanTemplateFileChange(const char *filename)
	{
		TIME_SCOPE2(ScanTemplate);
		char filepath[512];
		DIR *dir;
		dirent *ent;
		const char *map = STRING(gpGlobals->mapname);
		char poppath[256];
		snprintf(poppath, sizeof(poppath), "%s/%s/scripts/population", game_path, cvar_downloadpath.GetString());

		if ((dir = opendir(poppath)) != nullptr) {
			while ((ent = readdir(dir)) != nullptr) {
				if (StringStartsWith(ent->d_name, map)) {
					bool has = false;
					snprintf(filepath, sizeof(filepath), "%s/%s", poppath, ent->d_name);
					
					FILE *file = fopen(filepath, "r");
					if (file != nullptr) {
						char line[128];
						while(fgets(line, sizeof(line), file) != nullptr) {
							char *startcomment = strstr(line, "//");

							char *start = strstr(line, "#base");
							if (start != nullptr && (startcomment == nullptr || start < startcomment) && strstr(start+5, filename) != nullptr) {
								popfiles_to_forced_update.insert(ent->d_name);
								has = true;
								break;
							}
							char *brake = strchr(line, '{');
							if (brake != nullptr && (startcomment == nullptr || brake < startcomment)) {
								break;
							}
						}
						fclose(file);
					}
				}
			}
			closedir(dir);
		}
	}

	bool ShouldLateDownload(const std::string &filename)
	{
		return cvar_late_download.GetBool() && filename.find("leaderboard_class_") != std::string::npos;
	}
	
	CustomFileScanner late_dl_all_missions_scanner;
	DIR *late_dl_all_missions_dir = nullptr;
	dirent *late_dl_all_missions_ent = nullptr;

	bool late_dl_all_missions_generate_progress = false;
	void GenerateLateDownloadablesAllMissionsEnd()
	{
		closedir(late_dl_all_missions_dir);
		late_dl_all_missions_dir = nullptr;
		late_dl_all_missions_generate_progress = false;
		// Icons get extra weight so they are downloaded later
		for (auto &entry : late_dl_all_missions_scanner.files_add) {
			if (entry.name.starts_with("materials/hud/leaderboard_class_")) {
				entry.size += 95000;
			}
		}
		std::sort(late_dl_all_missions_scanner.files_add.begin(), late_dl_all_missions_scanner.files_add.end(), [](auto &entry1, auto &entry2){
			return entry1.size < entry2.size;
		});
		static ConVarRef net_maxfilesize("net_maxfilesize");
		size_t sizeLimit = (size_t) Min(net_maxfilesize.GetInt() * 1024 * 1024, cvar_late_download_size_limit.GetInt() * 1024);
		for (auto &entry : late_dl_all_missions_scanner.files_add) {
			if (entry.size > sizeLimit) continue;
			late_dl_from_all_maps_files.push_back(entry.name);
		}

		if (!late_dl_from_all_maps_files.empty()) {
			GenerateLateDownloadablesCurrentMission();
		}
	}

	bool GenerateLateDownloadablesAllMissionsUpdate()
	{
		char respath[512];

		const char *map = STRING(gpGlobals->mapname);

		int amount = 8;
		while ((late_dl_all_missions_ent = readdir(late_dl_all_missions_dir)) != nullptr) {
			if (!StringStartsWith(late_dl_all_missions_ent->d_name, map) && StringStartsWith(late_dl_all_missions_ent->d_name, "mvm_") && StringEndsWith(late_dl_all_missions_ent->d_name, ".pop")) {
				snprintf(respath, sizeof(respath), "%s%s", "scripts/population/",late_dl_all_missions_ent->d_name);
				KeyValues *kv = new KeyValues("kv");
				kv->UsesConditionals(false);
				if (kv->LoadFromFile(filesystem, respath)) {
					late_dl_all_missions_scanner.KeyValueBrowse(kv);
				}
				kv->deleteThis();
				if (--amount <= 0) break;
			}
		}
		if (late_dl_all_missions_ent == nullptr) {
			GenerateLateDownloadablesAllMissionsEnd();
		}
		return late_dl_all_missions_ent != nullptr;
	}

	void GenerateLateDownloadablesAllMissions()
	{
		if (!cvar_late_download.GetBool() || !cvar_late_download_other_maps.GetBool()) return;

		late_dl_from_all_maps_files.clear();

		late_dl_all_missions_scanner = CustomFileScanner();
		late_dl_all_missions_scanner.readSize = true;
		late_dl_all_missions_scanner.loadIconsForLateDl = true;
		late_dl_all_missions_scanner.loadOtherForLateDl = true;

		char poppath[256];
		snprintf(poppath, sizeof(poppath), "%s/%s/scripts/population", game_path, cvar_downloadpath.GetString());
		
		if (late_dl_all_missions_dir != nullptr) {
			closedir(late_dl_all_missions_dir);
		}
		late_dl_all_missions_dir = opendir(poppath);
		if (late_dl_all_missions_dir != nullptr) {
			late_dl_all_missions_generate_progress = true;
		}
	}

	void GenerateLateDownloadablesCurrentMission()
	{
		late_dl_files.clear();
		if (!cvar_late_download.GetBool()) return;

		const char *currentMission = g_pPopulationManager != nullptr ? g_pPopulationManager->GetPopulationFilename() : nullptr;
		if (currentMission != nullptr) {
			auto iconsMission = late_dl_files_per_mission.find(currentMission);
			if (iconsMission != late_dl_files_per_mission.end()) {
				for (auto &icon : iconsMission->second) {
					late_dl_files.push_back(icon);
				}
			}
		}
		late_dl_files_current_mission_count = late_dl_files.size();

		for (auto &entry : late_dl_files_per_mission) {
			if (currentMission == nullptr || entry.first != currentMission) {
				for (auto &icon : entry.second) {
					late_dl_files.push_back(icon);
				}
			}
		}

		if (cvar_late_download_other_maps.GetBool()) {
			late_dl_files.insert(late_dl_files.end(), late_dl_from_all_maps_files.begin(), late_dl_from_all_maps_files.end());
		}
		if (!late_dl_files.empty()) {
			for (int i = 1; i <= gpGlobals->maxClients; i++) {
				// Re-prioritize downloads
				StopLateFilesToDownloadForPlayer(i);
				AddLateFilesToDownloadForPlayer(i, late_dl_files, late_dl_files_current_mission_count);
			}
		}
	}

	time_t update_timestamp = 0;
	void GenerateDownloadables(time_t timestamp = 0)
	{
		case_sensitive_toggle = true;
		update_timestamp = time(nullptr);
		CFastTimer timer;
		timer.Start();
		INetworkStringTable *downloadables = networkstringtable->FindTable("downloadables");
		if (downloadables == nullptr) {
			Warning("LoadDownloadsFile: String table \"downloadables\" apparently doesn't exist!\n");
			return;
		}

		CustomFileScanner scanner;

		char poppath[256];
		snprintf(poppath, sizeof(poppath), "%s/%s/scripts/population", game_path, cvar_downloadpath.GetString());
		char filepath[512];
		char respath[512];
		DIR *dir;
		dirent *ent;
		const char *map = STRING(gpGlobals->mapname);
		
		if (timestamp == 0) {
			missing_files.clear();
			late_dl_files_per_mission.clear();
		}

		auto admin = GetMissionOwner();
		mission_owner = admin;
		const char *currentMission = (admin != nullptr && g_pPopulationManager != nullptr) ? g_pPopulationManager->GetPopulationFilename() : nullptr;

		if ((dir = opendir(poppath)) != nullptr) {
			while ((ent = readdir(dir)) != nullptr) {
				if (StringStartsWith(ent->d_name, map) && StringEndsWith(ent->d_name, ".pop")) {
					CFastTimer timer;
					timer.Start();
					snprintf(filepath, sizeof(filepath), "%s/%s", poppath, ent->d_name);
					
					struct stat stats;
					stat(filepath, &stats);
					if (stats.st_mtime < timestamp && !(!popfiles_to_forced_update.empty() && popfiles_to_forced_update.count(ent->d_name))) continue;

					snprintf(respath, sizeof(respath), "%s%s", "scripts/population/",ent->d_name);
					KeyValues *kv = new KeyValues("kv");
					kv->UsesConditionals(false);
					if (kv->LoadFromFile(filesystem, respath)) {
						scanner.printmissing = admin != nullptr && strcmp(respath, currentMission) == 0;
						scanner.KeyValueBrowse(kv);

						if (cvar_late_download.GetBool()) {
							CustomFileScanner iconScanner;
							iconScanner.printmissing = admin != nullptr && strcmp(respath, currentMission) == 0;
							iconScanner.loadIconsForLateDl = true;
							iconScanner.KeyValueBrowse(kv);
							
							auto &iconVec = late_dl_files_per_mission[respath];
							iconVec.clear();
							for (auto &entry : iconScanner.files_add) {
								iconVec.push_back(entry.name);
							}
						}
					}
					kv->deleteThis();
					timer.End();
					//Msg("FIle: %s time: %.9f\n", respath, timer.GetDuration().GetSeconds());
				}
			}
			closedir(dir);
		}
		
		auto upgradeFile = TFGameRules()->GetCustomUpgradesFile();
		if (upgradeFile != nullptr && upgradeFile[0] != '\0')
			scanner.AddFileIfCustom(StringStartsWith(upgradeFile, "download/") ? upgradeFile + strlen("download/") : upgradeFile);

		if (scanner.missingfilemention && admin != nullptr) {
			PrintToChatSM(admin, 1, "%t\n", "Missing files");
		}
		popfiles_to_forced_update.clear();

		bool saved_lock = engine->LockNetworkStringTables(false);

		for (auto &entry : scanner.files_add) {
			downloadables->AddString(true, entry.name.c_str());
		}
		engine->LockNetworkStringTables(saved_lock);
		timer.End();
		Msg("GenerateDownloadables time %.9f\n", timer.GetDuration().GetSeconds());
		case_sensitive_toggle = false;
	}

	CON_COMMAND_F(sig_util_download_manager_scan_all_missing, "Utility: scan all missing files in all population files", FCVAR_NOTIFY)
	{
		char poppath[512];
		snprintf(poppath, sizeof(poppath), "%s/%s/scripts/population", game_path, cvar_downloadpath.GetString());
		DIR *dir;
		dirent *ent;
		char filepath[512];
		char respath[512];
		char origMapFile[256];
		snprintf(origMapFile, sizeof(origMapFile), "maps/%s.bsp", STRING(gpGlobals->mapname));
		filesystem->RemoveSearchPath(origMapFile, "GAME");

		CustomFileScanner scanner;

		FileFindHandle_t mapHandle;
		// Find maps in all game directories
		for (const char *mapName = filesystem->FindFirstEx("maps/mvm_*.bsp", "GAME", &mapHandle);
						mapName != nullptr; mapName = filesystem->FindNext(mapHandle)) {
			//Remove extension from the map name
			std::string mapNameNoExt(mapName, strlen(mapName) - 4);
			char mapFile[256];
			snprintf(mapFile, sizeof(mapFile), "maps/%s", mapName);
			bool addedToPath = false;
			if ((dir = opendir(poppath)) != nullptr) {
				
				while ((ent = readdir(dir)) != nullptr) {
					if (StringStartsWith(ent->d_name, mapNameNoExt.c_str()) && StringEndsWith(ent->d_name, ".pop")) {
						if (!addedToPath) {
							filesystem->AddSearchPath(mapFile, "GAME");
							addedToPath = true;
						}
						snprintf(filepath, sizeof(filepath), "%s/%s", poppath, ent->d_name);

						snprintf(respath, sizeof(respath), "%s%s", "scripts/population/",ent->d_name);
						KeyValues *kv = new KeyValues("kv");
						kv->UsesConditionals(false);
						if (kv->LoadFromFile(filesystem, respath)) {
							scanner.printmissing = false;
							scanner.KeyValueBrowse(kv);
						}
						kv->deleteThis();
						//Msg("FIle: %s time: %.9f\n", respath, timer.GetDuration().GetSeconds());
					}
				}
				closedir(dir);
			}
			if (addedToPath) {
				filesystem->RemoveSearchPath(mapFile, "GAME");
			}

		}
		filesystem->FindClose(mapHandle);
		filesystem->AddSearchPath(origMapFile, "GAME");
		Msg("Missing files:\n");
		size_t pathfullLength = strlen(game_path) + 1 + strlen(cvar_downloadpath.GetString()) + 1;
		for (auto &entry : missing_files) {
			if (!entry.second.ends_with(".phy") && entry.second.length() > pathfullLength) {
				Msg("%s\n", entry.second.c_str() + pathfullLength);
			}
		}
	}

	KeyValues *kvBannedMissions = nullptr;
	bool hasRestrictedPopfiles = false;
	std::vector<std::string> popfilesRestricted;
	void AddMapToVoteList(const char *mapName, KeyValues *kv, int &files, std::string &maplistStr)
	{
		std::string mapNameStr(mapName, strlen(mapName) - 4);
		if (banned_maps.count(mapNameStr)) return;

		if (hasRestrictedPopfiles && !Mod::Pop::PopMgr_Extensions::CheckRestricts(mapNameStr.c_str(), popfilesRestricted, kvBannedMissions)) return;

		files++;

		maplistStr.append(mapNameStr);
		maplistStr.push_back('\n');

		auto kvmap = kv->CreateNewKey();
		kvmap->SetString("map", mapNameStr.c_str());
		kvmap->SetString("popfile", mapNameStr.c_str());
	}

	void ResetVoteMapList() 
	{
		if (strlen(cvar_resource_file.GetString()) == 0 || strlen(cvar_mapcycle_file.GetString()) == 0) {
			return;
		}
		case_sensitive_toggle = true;
		CFastTimer timer1;
		timer1.Start();
		KeyValues *kv = new KeyValues(cvar_resource_file.GetString());

		kvBannedMissions = Mod::Pop::PopMgr_Extensions::LoadBannedMissionsFile();
		popfilesRestricted.clear();
		hasRestrictedPopfiles = Mod::Pop::PopMgr_Extensions::GetPopfiles(popfilesRestricted);

		//FileToKeyValues(kv,pathres);

		kv->SetInt("categories",1);
		auto kvcat = kv->CreateNewKey();

		FileFindHandle_t mapHandle;
		std::string maplistStr;
		int files = 0;
		// Find maps in all game directories
		if (*cvar_mappath.GetString() == '\0') {
			for (const char *mapName = filesystem->FindFirstEx("maps/mvm_*.bsp", "GAME", &mapHandle);
							mapName != nullptr; mapName = filesystem->FindNext(mapHandle)) {

				AddMapToVoteList(mapName, kvcat, files, maplistStr);
			}
		}
		// Find maps in the specified directory
		else {
			std::string path = fmt::format("{}/{}", game_path, cvar_mappath.GetString());
			DIR *dir;
			dirent *ent;

			if ((dir = opendir(path.c_str())) != nullptr) {
				while ((ent = readdir(dir)) != nullptr) {
					if (StringStartsWith(ent->d_name, "mvm_") && StringEndsWith(ent->d_name, ".bsp")) {
						AddMapToVoteList(ent->d_name, kvcat, files, maplistStr);
					}
				}
				closedir(dir);
			}
		}
		filesystem->FindClose(mapHandle);
		timer1.End();
		CFastTimer timer3;
		timer3.Start();

		if (kvBannedMissions != nullptr) {
			kvBannedMissions->deleteThis();
			kvBannedMissions = nullptr;
		}
		
		kvcat->SetInt("count", files);
		kv->SaveToFile(filesystem, cvar_resource_file.GetString());
		kv->deleteThis();

		static ConVarRef tf_mvm_missioncyclefile("tf_mvm_missioncyclefile");
		std::string prevMissionCycleFile = tf_mvm_missioncyclefile.GetString();
		tf_mvm_missioncyclefile.SetValue("empty");
		tf_mvm_missioncyclefile.SetValue(prevMissionCycleFile.c_str());

		std::string resourceFileWrite = fmt::format("{}/{}wr", game_path, cvar_mapcycle_file.GetString());
		FILE *mapcycle = fopen(resourceFileWrite.c_str(), "w");
		if (mapcycle != nullptr) {
			fputs(maplistStr.c_str(), mapcycle);
			fflush(mapcycle);
			fclose(mapcycle);
		}
		rename(resourceFileWrite.c_str(), fmt::format("{}/{}", game_path, cvar_mapcycle_file.GetString()).c_str());
		timer3.End();
		CFastTimer timer2;
		timer2.Start();

		std::string poplistStr;
		CUtlVector<CUtlString> vec;	
		CPopulationManager::FindDefaultPopulationFileShortNames(vec);
		FOR_EACH_VEC(vec, i)
		{
			poplistStr += vec[i];
			poplistStr += '\n';
		}

		/*char fmt[256];
		snprintf(fmt, sizeof(fmt), "scripts/population/%s*.pop", STRING(gpGlobals->mapname));
		FileFindHandle_t missionHandle;
		std::string poplistStr;
		int mapLength = strlen(STRING(gpGlobals->mapname));
		for (const char *missionName = filesystem->FindFirstEx(fmt, "GAME", &missionHandle);
						missionName != nullptr; missionName = filesystem->FindNext(missionHandle)) {
			int missionNameLen = strlen(missionName);
			poplistStr.append(missionName, missionNameLen - 4);

			// Display missions without name as normal
			if (missionNameLen - 4 == mapLength) {
				poplistStr.append("_normal");
			}
			poplistStr.push_back('\n');
		}
		filesystem->FindClose(missionHandle);*/

		bool saved_lock = engine->LockNetworkStringTables(false);
		INetworkStringTable *strtablemaplist = networkstringtable->FindTable("ServerMapCycleMvM");
		INetworkStringTable *strtablepoplist = networkstringtable->FindTable("ServerPopFiles");

		if (strtablemaplist != nullptr && strtablepoplist != nullptr) {
			if (!maplistStr.empty() && strtablemaplist->GetNumStrings() > 0)
				strtablemaplist->SetStringUserData(0, maplistStr.size() + 1, maplistStr.c_str());

			if (strtablepoplist->GetNumStrings() == 0 && !poplistStr.empty()) {
				strtablepoplist->AddString(true, "ServerPopFiles", poplistStr.size() + 1, poplistStr.c_str());
			}
			if (strtablepoplist->GetNumStrings() > 0) {
				strtablepoplist->SetStringUserData(0, poplistStr.size() + 1, poplistStr.c_str());
			}
		}
		engine->LockNetworkStringTables(saved_lock);
		timer2.End();
		DevMsg("Time vote %.9f %.9f %.9f\n", timer1.GetDuration().GetSeconds(), timer2.GetDuration().GetSeconds(), timer3.GetDuration().GetSeconds());
		case_sensitive_toggle = false;
	}

	void LoadDownloadsFile()
	{
	//	if (!g_pSM->IsMapRunning()) return;
		
		INetworkStringTable *downloadables = networkstringtable->FindTable("downloadables");
		if (downloadables == nullptr) {
			Warning("LoadDownloadsFile: String table \"downloadables\" apparently doesn't exist!\n");
			return;
		}
		
		auto kv = new KeyValues("Downloads");
		kv->UsesEscapeSequences(true);
		
		if (kv->LoadFromFile(filesystem, cvar_kvpath.GetString())) {
			std::vector<DownloadBlock> blocks;
			
			FOR_EACH_SUBKEY(kv, kv_block) {
				DownloadBlock block;
				bool errors = false;
				
				block.name = kv_block->GetName();
				
				FOR_EACH_SUBKEY(kv_block, kv_key) {
					const char *name  = kv_key->GetName();
					const char *value = kv_key->GetString();
					
					if (FStrEq(name, "Map")) {
						block.keys_map.emplace_back(value);
					} else if (FStrEq(name, "File")) {
						block.keys_file.emplace_back(value);
					} else if (FStrEq(name, "Precache")) {
						block.keys_precache.emplace_back(value);
					} else {
						Warning("LoadDownloadsFile: Block \"%s\": Invalid key type \"%s\"\n", block.name.c_str(), name);
						errors = true;
					}
				}
				
				if (block.keys_map.empty()) {
					Warning("LoadDownloadsFile: Block \"%s\": Must have at least one Map key\n", block.name.c_str());
					errors = true;
				}
				
				if (block.keys_file.empty()) {
					Warning("LoadDownloadsFile: Block \"%s\": Must have at least one File key\n", block.name.c_str());
					errors = true;
				}
				
				if (block.keys_precache.size() != 1) {
					Warning("LoadDownloadsFile: Block \"%s\": Must have exactly one Precache key\n", block.name.c_str());
					errors = true;
				}
				
				if (FStrEq(block.keys_precache[0].c_str(), "Generic")) {
					block.l_precache = [](const char *path){ engine->PrecacheGeneric(path, true); };
				} else if (FStrEq(block.keys_precache[0].c_str(), "Model")) {
					block.l_precache = [](const char *path){ CBaseEntity::PrecacheModel(path, true); };
				} else if (FStrEq(block.keys_precache[0].c_str(), "Decal")) {
					block.l_precache = [](const char *path){ engine->PrecacheDecal(path, true); };
				} else if (FStrEq(block.keys_precache[0].c_str(), "Sound")) {
					block.l_precache = [](const char *path){ enginesound->PrecacheSound(path, true); };
				} else if (!FStrEq(block.keys_precache[0].c_str(), "No")) {
					Warning("LoadDownloadsFile: Block \"%s\": Invalid Precache value \"%s\"\n", block.name.c_str(), block.keys_precache[0].c_str());
				}
				
				if (!errors) {
					blocks.push_back(std::move(block));
				} else {
					Warning("LoadDownloadsFile: Not applying block \"%s\" due to errors\n", block.name.c_str());
				}
			}
			
		//	std::vector<std::string> map_names;
		//	ForEachMapName([&](const char *map_name){
		//		map_names.emplace_back(map_name);
		//	});
			
#ifndef _MSC_VER
#warning NEED try/catch for std::regex ctor and funcs!
#endif
			
			for (const auto& block : blocks) {
			//	DevMsg("LoadDownloadsFile: Block \"%s\"\n", block.name.c_str());
				
				/* check each Map regex pattern against the current map and see if any is applicable */
				bool match = false;
				for (const auto& map_re : block.keys_map) {
					std::regex re(map_re, std::regex::ECMAScript | std::regex::icase);
					
					if (std::regex_match(STRING(gpGlobals->mapname), re, std::regex_constants::match_any)) {
					//	DevMsg("LoadDownloadsFile:   Map \"%s\" vs \"%s\": MATCH\n", map_re.c_str(), STRING(gpGlobals->mapname));
						match = true;
						break;
					} else {
					//	DevMsg("LoadDownloadsFile:   Map \"%s\" vs \"%s\": nope\n", map_re.c_str(), STRING(gpGlobals->mapname));
					}
				}
				if (!match) continue;
				
				/* for each File wildcard pattern, find all matching files and add+precache them */
				for (const auto& file_wild : block.keys_file) {
				//	DevMsg("LoadDownloadsFile:   File \"%s\":\n", file_wild.c_str());
					
					// TODO: maybe use an explicit PathID, rather than nullptr...?
					FileFindHandle_t handle;
					for (const char *file = filesystem->FindFirstEx(file_wild.c_str(), nullptr, &handle);
						file != nullptr; file = filesystem->FindNext(handle)) {
						char path[0x400];
						V_ExtractFilePath(file_wild.c_str(), path, sizeof(path));
						V_AppendSlash(path, sizeof(path));
						V_strcat_safe(path, file);
						
						if (filesystem->FindIsDirectory(handle)) {
						//	DevMsg("LoadDownloadsFile:     Skip Directory \"%s\"\n", path);
							continue;
						}
						
						const char *ext = V_GetFileExtension(path);
						if (ext != nullptr && FStrEq(ext, "bz2")) {
						//	DevMsg("LoadDownloadsFile:     Skip Bzip2 \"%s\"\n", path);
							continue;
						}
						
					//	DevMsg("LoadDownloadsFile:     Match \"%s\"\n", path);
						
					//	DevMsg("LoadDownloadsFile:       Precache\n");
						block.l_precache(path);
						
					//	DevMsg("LoadDownloadsFile:       StringTable\n");
						bool saved_lock = engine->LockNetworkStringTables(false);
						downloadables->AddString(true, path);
						engine->LockNetworkStringTables(saved_lock);
					}
					filesystem->FindClose(handle);
				}
			}
		} else {
			Warning("LoadDownloadsFile: Could not load KeyValues from \"%s\".\n", cvar_kvpath.GetString());
		}
		
		kv->deleteThis();
	}
	
	DETOUR_DECL_MEMBER(void, CServerGameDLL_ServerActivate, edict_t *pEdictList, int edictList, int clientMax)
	{
		DETOUR_MEMBER_CALL(pEdictList, edictList, clientMax);
		
		server_activated = true;
		late_dl_files.clear();
		late_dl_files_current_mission_count = 0;
		LoadDownloadsFile();
		GenerateDownloadables();
		ResetVoteMapList();
		GenerateLateDownloadablesAllMissions();
		GenerateLateDownloadablesCurrentMission();
	}

	DETOUR_DECL_STATIC(bool, findFileInDirCaseInsensitive, const char *file, char* output, size_t bufSize)
	{
		bool hasupper = false;
		int i = 0;
		for(const char *c = file; *c != '\0'; c++) {
			hasupper |= i++ > base_path_len && *c >= 'A' && *c <= 'Z';
		}
		if (!hasupper) {
			return false;
		}
		strncpy(output, file, bufSize);
		V_strlower(output + base_path_len);
		struct stat stats;
		return stat(output, &stats) == 0;
		//return DETOUR_STATIC_CALL(file, output, bufSize);
	}

	int paused_wave_time = -1;

	std::string latedl_curmission_only_path;

	StaticFuncThunk<int, IClient *, const char *, bool> ft_SendCvarValueQueryToClient("SendCvarValueQueryToClient");
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_ClientConnected, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
	{
		auto gamerules = reinterpret_cast<CTFGameRules *>(this);
		auto client = ((CBaseClient *) sv->GetClient(ENTINDEX(pEntity) - 1));
		if (!client->IsFakeClient()) {
			auto &info = download_infos[ENTINDEX(pEntity)];
			info = LateDownloadInfo();
			
			std::vector<std::string> icons;

			char path[512];
			snprintf(path, 512, "%s%llu", latedl_curmission_only_path.c_str(), client->m_SteamID.ConvertToUint64());
			info.lateDlCurMissionOnly = access(path, F_OK) == 0;
			info.lateDlCurMissionOnlyInformed = info.lateDlCurMissionOnly;

			AddLateFilesToDownloadForPlayer(ENTINDEX(pEntity), late_dl_files, late_dl_files_current_mission_count);
		}

		return DETOUR_MEMBER_CALL(pEntity, pszName, pszAddress, reject, maxrejectlen);
	}

	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
        auto ret = DETOUR_MEMBER_CALL();
		GenerateLateDownloadablesCurrentMission();
		return ret;
    }

	DETOUR_DECL_MEMBER(void, CServerPlugin_OnQueryCvarValueFinished, QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue)
	{
		DETOUR_MEMBER_CALL(iCookie, pPlayerEntity, eStatus, pCvarName, pCvarValue);
		if (pCvarName != nullptr && !FStrEq(pCvarName, "sv_allowupload")) return;

		auto &info = download_infos[pPlayerEntity->m_EdictIndex];
		if (eStatus == eQueryCvarValueStatus_ValueIntact && pCvarValue != nullptr && atoi(pCvarValue) != 0) {

		}
		else{
			info.iconsDownloading.clear();
			info.filesToDownload.clear();
			info.filesDownloading.clear();
			info.lateDlEnabled = false;
			info.active = false;
		}
		
    }

	DETOUR_DECL_MEMBER(void, CTFGameRules_OnPlayerSpawned, CTFPlayer *player)
	{
		DETOUR_MEMBER_CALL(player);
		if (player->IsRealPlayer()) {
			auto &info = download_infos[player->entindex()];
			
			if (!info.lateDlEnabled && !info.lateDlUploadInfoSend) {
				auto text = FormatTextForPlayerSM(player, 1, "%t\n", "Late DL sv_allowupload notify");
				PrintToChat(text, player);
				ClientMsg(player, "%s", text);
				info.lateDlUploadInfoSend = true;
			}
		}
	}

	void CallbackMvMCounts(CBaseEntity *entity, int clientIndex, DVariant &value, int callbackIndex, SendProp *prop, uintptr_t data) {
		auto &info = download_infos[clientIndex];
		if (!info.active) return;
		if (info.iconsDownloading.empty()) return;
		auto iconName = data < 12 ? TFObjectiveResource()->m_iszMannVsMachineWaveClassNames[data % 12] : TFObjectiveResource()->m_iszMannVsMachineWaveClassNames2[data % 12];

		if (info.iconsDownloading.contains(STRING(iconName))) {
			value.m_Int = 0;
		}
	}

	int inotify_fd = -1;
	int inotify_wd = -1;
	
	#define EVENT_SIZE  ( sizeof (struct inotify_event) )
	#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

	void StopNotifyDirectory()
	{
		if (inotify_fd >= 0) {
			inotify_rm_watch(inotify_fd, inotify_wd);
			close(inotify_fd);
			inotify_fd = -1;
		}
	}
	void CreateNotifyDirectory()
	{
		StopNotifyDirectory();
		if (!cvar_download_refresh.GetBool()) return;

		inotify_fd = inotify_init1(IN_NONBLOCK);
		
		char poppath[256];
		snprintf(poppath, sizeof(poppath), "%s/%s/scripts/population", game_path, cvar_downloadpath.GetString());

		inotify_wd = inotify_add_watch(inotify_fd, poppath, IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO);
		
	}
	
	void WatchMissingFile(std::string &path)
	{
		if (inotify_fd >= 0) {
			std::string fullPath = game_path;
			fullPath += "/";
			fullPath += cvar_downloadpath.GetString();
			fullPath += "/";
			fullPath += path;

			std::string pathDir = fullPath.substr(0,fullPath.rfind('/'));

			auto find = missing_files_dirs.find(pathDir);
			missing_files.emplace(find != missing_files_dirs.end() ? find->second : inotify_add_watch(inotify_fd, pathDir.c_str(), IN_CREATE | IN_MOVED_TO), path);
		}
	}

	GlobalThunk<SendTable> DT_TFObjectiveResource_g_SendTable("DT_TFObjectiveResource::g_SendTable");

	class CMod : public IMod, IModCallbackListener, IFrameUpdatePostEntityThinkListener
	{
	public:
		//SH_DECL_HOOK2(IBaseFileSystem, Size, SH_NOATTRIB, 0, unsigned int, const char *, const char *);

		int sizeHook = 0;

		CMod() : IMod("Util:Download_Manager")
		{
			//
			MOD_ADD_DETOUR_MEMBER(CServerGameDLL_ServerActivate, "CServerGameDLL::ServerActivate");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ClientConnected, "CTFGameRules::ClientConnected");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse, "CPopulationManager::Parse");
			MOD_ADD_DETOUR_MEMBER(CServerPlugin_OnQueryCvarValueFinished, "CServerPlugin::OnQueryCvarValueFinished");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_OnPlayerSpawned, "CTFGameRules::OnPlayerSpawned");

			// Faster implementation of findFileInDirCaseInsensitive, instead of looking for any matching file with a different case, only look for files with lowercase letters instead
			//MOD_ADD_DETOUR_STATIC(findFileInDirCaseInsensitive, "findFileInDirCaseInsensitive");
		}
		virtual bool OnLoad() override
		{
			game_path = g_SMAPI->GetBaseDir();
			base_path_len = filesystem->GetSearchPath( "BASE_PATH", true, nullptr, 0);

			auto precalc = DT_TFObjectiveResource_g_SendTable.GetRef().m_pPrecalc;
			auto &table = DT_TFObjectiveResource_g_SendTable.GetRef();
			SendProp *mvmCountProp = nullptr;
			SendProp *mvmCountProp2 = nullptr;
			for (int i = 0; i < table.GetNumProps(); i++) {
				if (strcmp(table.GetProp(i)->GetName(), "m_nMannVsMachineWaveClassCounts") == 0) {
					mvmCountProp = table.GetProp(i)->GetDataTable()->GetProp(0);
					mvm_count_table = table.GetProp(i)->GetDataTable();
				}
				if (strcmp(table.GetProp(i)->GetName(), "m_nMannVsMachineWaveClassCounts2") == 0) {
					mvmCountProp2 = table.GetProp(i)->GetDataTable()->GetProp(0);
					mvm_count_table2 = table.GetProp(i)->GetDataTable();
				}
			}
			if (mvmCountProp != nullptr && mvmCountProp2 != nullptr) {
				for (int i = 0; i < precalc->m_Props.Count(); i++) {
					if (precalc->m_Props[i] == mvmCountProp) {
						mvm_count_prop_index = i;
					}
					if (precalc->m_Props[i] == mvmCountProp2) {
						mvm_count_prop_index2 = i;
					}
				}
			}

			char path_sm[PLATFORM_MAX_PATH];
        	g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/latedl_curmission_only/");
			latedl_curmission_only_path = path_sm;
			mkdir(path_sm, 0766);

			return true;
		}

		virtual void OnEnable() override
		{
			// For faster game file lookup, create a separate path id with vpks and only vpks
			filesystem->AddSearchPath("tf/tf2_misc.vpk", "vpks");
			filesystem->AddSearchPath("tf/tf2_sound_misc.vpk", "vpks");
			filesystem->AddSearchPath("tf/tf2_sound_vo_english.vpk", "vpks");
			filesystem->AddSearchPath("tf/tf2_textures.vpk", "vpks");
			filesystem->AddSearchPath("hl2/hl2_textures.vpk", "vpks");
			filesystem->AddSearchPath("hl2/hl2_sound_vo_english.vpk", "vpks");
			filesystem->AddSearchPath("hl2/hl2_sound_misc.vpk", "vpks");
			filesystem->AddSearchPath("hl2/hl2_misc.vpk", "vpks");
			filesystem->AddSearchPath("platform/platform_misc.vpk", "vpks");

			//sizeHook = SH_ADD_VPHOOK(IBaseFileSystem, Size, filesystem, SH_MEMBER(this, &CMod::SizeHook), false);
			LoadDownloadsFile();
			GenerateDownloadables();
			CreateNotifyDirectory();
		}
		virtual void OnDisable() override
		{
			//SH_REMOVE_HOOK_ID(sizeHook);
			filesystem->RemoveSearchPaths("vpks");
			StopNotifyDirectory();

			auto resource = TFObjectiveResource();
			if (resource != nullptr && resourceOverrideAdded) {
				resourceOverrideAdded = false;
				auto mod = TFObjectiveResource()->GetEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");
				if (mod != nullptr) {
					for (int i = 0; i < 24; i++) {
						if (propOverrideIds[i] != 0) {
							mod->RemoveOverride(propOverrideIds[i]);
						}
					}
				}
			}
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		DIR *ContinueReadingDir(DIR *dir, time_t &updateTime)
		{
			TIME_SCOPE2(CheckNewFilesContinueReading);	
			dirent *ent;
			int i = 0;
			const char *map = STRING(gpGlobals->mapname);
			char poppath[256];
			char filepath[512];
			snprintf(poppath, sizeof(poppath), "%s/%s/scripts/population", game_path, cvar_downloadpath.GetString());
			while ((ent = readdir(dir)) != nullptr) {
				
				if (StringEndsWith(ent->d_name, ".pop") && (StringStartsWith(ent->d_name, map) || !StringStartsWith(ent->d_name, "mvm_"))) {
					snprintf(filepath, sizeof(filepath), "%s/%s", poppath,ent->d_name);
					struct stat stats;
					stat(filepath, &stats);
					if (stats.st_mtime > updateTime && stats.st_mtime > updateTime) {
						updateTime = stats.st_mtime;
						if (!StringStartsWith(ent->d_name, "mvm_")) {
							ScanTemplateFileChange(ent->d_name);
						}
					}
				}
				if (i++ > 32) return dir;
			}
			closedir(dir);
			return nullptr;
		}
		
		bool LateDownloadUpdate() {
			if (!cvar_late_download.GetBool()) return false;

			static uint transferId = RandomInt(0, UINT_MAX);
			bool hasIconDownloads = false;
			int maxclients = Min(gpGlobals->maxClients, (int)ARRAYSIZE(download_infos) - 1);
			for (int i = 1; i <= maxclients; i++) {
				auto &info = download_infos[i];
				
				if (!info.active) continue;

				if (!info.lateDlEnabled) continue;

				auto client = (CBaseClient *) sv->GetClient(i - 1);
				CNetChan *netchan = client != nullptr ? static_cast<CNetChan *>(client->GetNetChannel()) : nullptr;

				if (netchan == nullptr) {
					info = LateDownloadInfo();
					continue;
				}

				if (client->m_nSignonState <= 2) continue;

				if (!info.lateDlChecked) {
					auto val = ft_SendCvarValueQueryToClient(sv->GetClient(i - 1), "sv_allowupload", true);
					if (val != InvalidQueryCvarCookie) {
						info.lateDlChecked = true;
					}
				}
				if (info.filesDownloading.empty() && !info.filesToDownload.empty() && UTIL_PlayerByIndex(i) != nullptr) {
					ClientMsg(UTIL_PlayerByIndex(i), "Started late download of %d files\n", info.filesToDownload.size());
				}

				RemoveIf(info.filesDownloading, [&](auto &filename){
					bool waiting = netchan->IsFileInWaitingList(filename.c_str());
					if (!waiting) {
						int curFileIndex = info.filesQueried.size() - (info.filesDownloading.size() + info.filesToDownload.size()) + 1;
						if (curFileIndex > 50 && !info.lateDlCurMissionOnlyInformed && !info.lateDlCurMissionOnly && UTIL_PlayerByIndex(i) != nullptr) {
							info.lateDlCurMissionOnlyInformed = true;
							ClientMsg(UTIL_PlayerByIndex(i), "Too many download messages? Type sig_latedl_current_mission_download_only in console\n");
						}
						if (curFileIndex % 100 == 0 && UTIL_PlayerByIndex(i) != nullptr) {
							ClientMsg(UTIL_PlayerByIndex(i), "(%d/%d) File downloaded: %s\n", curFileIndex, info.filesQueried.size(), filename.c_str());
						}
						late_dl_download_history[((CBaseClient *) sv->GetClient(i - 1))->m_SteamID.ConvertToUint64()].insert(filename);
						
						if (filename.starts_with("materials/hud/leaderboard_class_")) {
							auto find = info.iconsDownloading.find(filename.substr(strlen("materials/hud/leaderboard_class_"), filename.size() - 4 - strlen("materials/hud/leaderboard_class_")));
							if (find != info.iconsDownloading.end()) {
								info.iconsDownloading.erase(find);
							}
						}
					}
					return !waiting;
				});
				if (!info.filesToDownload.empty() && info.filesDownloading.size() < 2) {
					auto &filename = info.filesToDownload.front();
					if (netchan->SendFile(filename.c_str(), transferId++)) {
						info.filesDownloading.push_back(filename);
					}
					else {
						if (UTIL_PlayerByIndex(i) != nullptr) {
							ClientMsg(UTIL_PlayerByIndex(i), "Fail downloading file %s\n", filename.c_str());
						}
						if (filename.starts_with("materials/hud/leaderboard_class_")) {
							auto find = info.iconsDownloading.find(filename.substr(strlen("materials/hud/leaderboard_class_"), filename.size() - 4 - strlen("materials/hud/leaderboard_class_")));
							if (find != info.iconsDownloading.end()) {
								info.iconsDownloading.erase(find);
							}
						}
					}
					info.filesToDownload.pop_front();
				}
				if (info.filesDownloading.empty() && info.filesToDownload.empty()) {
					if (UTIL_PlayerByIndex(i) != nullptr) {
						ClientMsg(UTIL_PlayerByIndex(i), "%d files downloaded\n", info.filesQueried.size());
					}
					info.iconsDownloading.clear();
					info.active = false;
					//netchan->SetFileTransmissionMode(UTIL_PlayerByIndex(i) != nullptr);
				}
				else {
					//netchan->SetFileTransmissionMode(UTIL_PlayerByIndex(i) != nullptr && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING);
				}
				if (!info.iconsDownloading.empty()) {
					hasIconDownloads = true;
				}
			}
			return hasIconDownloads;
		}

		bool resourceOverrideAdded = false;
		virtual void LevelInitPreEntity() override
		{
			resourceOverrideAdded = false;
			LateDownloadUpdate();
		}

		virtual void LevelInitPostEntity() override
		{
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			for (int i = 0; i < ARRAYSIZE(download_infos); i++) {
				download_infos[i] = LateDownloadInfo();
			}
		}

		virtual void FrameUpdatePostEntityThink() override
		{
			bool hasIconDownloads = LateDownloadUpdate();

			static bool resetVoteMap = false;
			static DIR *dir = nullptr;
			static bool lastUpdated = false;

			if (resetVoteMap) {
				resetVoteMap = false;
				ResetVoteMapList();
			}
			int ticknum = gpGlobals->tickcount % 256;

			if (ticknum == 42) {
				//TIME_SCOPE2(CheckNewFiles);	
				if (!cvar_download_refresh.GetBool()) return;
				
				const char *map = STRING(gpGlobals->mapname);
				bool updated = false;
				if (inotify_fd >= 0) {
					char buffer[BUF_LEN];
					ssize_t length, i = 0;
					length = read( inotify_fd, buffer, BUF_LEN );  
					while (length != -1 && i < length) {
						struct inotify_event *event = (struct inotify_event *) &buffer[i];
						if (event != nullptr && event->len) {
							// Look for popfile updates
							if (event->wd == inotify_wd) {
								const char *name = event->name;
								if (StringEndsWith(name, ".pop") && (StringStartsWith(name, map) || !StringStartsWith(name, "mvm_"))) {
									updated = true;
									if (!StringStartsWith(name, "mvm_")) {
										ScanTemplateFileChange(name);
									}
								}
							}
							// Look for missing files
							else {
								auto range = missing_files.equal_range(event->wd);
								for (auto it = range.first; it != range.second;) {
									const auto &str = it->second;
									if (StringEndsWith(str.c_str(), event->name, false)) {
										// File name with the case as the one on the disk
										auto realFileName = str;
										realFileName.replace(str.size() - strlen(event->name), strlen(event->name), event->name);

										// Always late download if the file was previously missing
										late_dl_files.push_back(realFileName);
										for (int i = 1; i <= gpGlobals->maxClients; i++) {
											AddLateFilesToDownloadForPlayer(i, {realFileName}, 1);
										}
										if (!ShouldLateDownload(realFileName)) {
											INetworkStringTable *downloadables = networkstringtable->FindTable("downloadables");
											bool saved_lock = engine->LockNetworkStringTables(false);
											downloadables->AddString(true, realFileName.c_str());
											engine->LockNetworkStringTables(saved_lock);
										}
										
										if (mission_owner != nullptr) {
											PrintToChat(CFmtStr("File no longer missing: %s\n", realFileName.c_str()), mission_owner);
										}
										it = missing_files.erase(it);
									}
									else {
										it++;
									}
								}
								if (missing_files.count(event->wd) == 0) {
									inotify_rm_watch(inotify_fd, event->wd);
								}
							}
						}
						i += EVENT_SIZE + event->len;
					}
				}
				if (lastUpdated && !updated) {
					GenerateDownloadables(update_timestamp);
					resetVoteMap = true;
				}
				lastUpdated = updated;
			}
			auto resource = TFObjectiveResource();
			if (resource != nullptr && !resourceOverrideAdded && hasIconDownloads) {
				resourceOverrideAdded = true;
				auto mod = TFObjectiveResource()->GetOrCreateEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");
				for (int i = 0; i < 24; i++) {
					propOverrideIds[i] = mod->AddOverride(CallbackMvMCounts, (i < 12 ? mvm_count_prop_index : mvm_count_prop_index2) + i % 12, (i < 12 ? mvm_count_table : mvm_count_table2)->GetProp(i % 12), i);
				}
			}
			else if (resource != nullptr && resourceOverrideAdded && !hasIconDownloads) { 
				resourceOverrideAdded = false;
				auto mod = TFObjectiveResource()->GetOrCreateEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");
				for (int i = 0; i < 24; i++) {
					if (propOverrideIds[i] != 0) {
						mod->RemoveOverride(propOverrideIds[i]);
					}
					propOverrideIds[i] = 0;
				}
			}

			if (late_dl_all_missions_generate_progress) {
				late_dl_all_missions_generate_progress = GenerateLateDownloadablesAllMissionsUpdate();
			}
		}
		unsigned int mvm_count_prop_index = 0;
		unsigned int mvm_count_prop_index2 = 0;
		SendTable *mvm_count_table = nullptr;
		SendTable *mvm_count_table2 = nullptr;

		int propOverrideIds[24] {};
	};
	CMod s_Mod;
	
	
	// TODO: 'download/' prefix fixup for custom MvM upgrade files!
	// TODO: AUTOMATIC download entry generation by scanning directories!
	
	
	static void ReloadConfigIfModEnabled()
	{
		if (s_Mod.IsEnabled()) {
			LoadDownloadsFile();
			GenerateDownloadables();
			ResetVoteMapList();
			GenerateLateDownloadablesAllMissions();
			GenerateLateDownloadablesCurrentMission();
		}
	}

	// is FCVAR_NOTIFY even a valid thing for commands...?
	CON_COMMAND_F(sig_util_download_manager_missing_files, "Utility: list missing files", FCVAR_NOTIFY)
	{
		for (auto &value : missing_files) {
			Msg("Missing file: %s\n", value.second.c_str());
		}
	}

	// is FCVAR_NOTIFY even a valid thing for commands...?
	CON_COMMAND_F(sig_util_download_manager_dump_latedl, "Utility: list late dl files", FCVAR_NOTIFY)
	{
		for (auto &value : late_dl_files) {
			Msg("%s\n", value.c_str());
		}
	}

	CON_COMMAND_F(sig_util_download_manager_missing_files_all, "Utility: list missing files", FCVAR_NOTIFY)
	{
		FileFindHandle_t mapHandle;
		std::string maplistStr;
		int files = 0;
		std::unordered_set<std::string> missing_files_all;
		for (const char *mapName = filesystem->FindFirstEx("maps/mvm_*.bsp", "GAME", &mapHandle);
							mapName != nullptr; mapName = filesystem->FindNext(mapHandle)) {
			string_t pre = gpGlobals->mapname;
			std::string mapNameNoExt(mapName, strlen(mapName) - 4);
			gpGlobals->mapname = AllocPooledString(mapNameNoExt.c_str());
			GenerateDownloadables();
			for (auto &value : missing_files) {
				missing_files_all.insert(value.second);
			}
		}
		filesystem->FindClose(mapHandle);
		
		
		for (auto &value : missing_files_all) {
			Msg("Missing file: %s\n", value.c_str());
		}
	}
			
	
	// is FCVAR_NOTIFY even a valid thing for commands...?
	CON_COMMAND_F(sig_util_download_manager_reload, "Utility: reload the configuration file", FCVAR_NOTIFY)
	{
		ReloadConfigIfModEnabled();
	}

	void AddPathToTail(const char *path, const char *pathID)
	{
		std::string fullPath = fmt::format("{}/{}", game_path, path);
		filesystem->RemoveSearchPath(fullPath.c_str(), pathID);
		filesystem->AddSearchPath(fullPath.c_str(), pathID);
	}

	ConVar cvar_custom_search_path("sig_util_download_manager_custom_search_path_tail", "", FCVAR_NOTIFY,
		"Utility: optional additional search path",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			
			if (pOldValue != nullptr && pOldValue[0] != '\0') {
				std::string oldFullPath = fmt::format("{}/{}", game_path, pOldValue);
				filesystem->RemoveSearchPath(oldFullPath.c_str(), "game");
				filesystem->RemoveSearchPath(oldFullPath.c_str(), "mod");
				filesystem->RemoveSearchPath(oldFullPath.c_str(), "custom");
			}

			if (strlen(cvar_custom_search_path.GetString()) > 0) {
				AddPathToTail(cvar_custom_search_path.GetString(), "game");
				AddPathToTail(cvar_custom_search_path.GetString(), "mod");
				AddPathToTail(cvar_custom_search_path.GetString(), "custom");
			}
			
		});

	// is FCVAR_NOTIFY even a valid thing for commands...?
	CON_COMMAND_F(sig_util_download_manager_add_search_path_tail, "Utility: add path to search path", FCVAR_NOTIFY)
	{
		if (args.ArgC() < 2) return;
		if (args.ArgC() == 2) {
			AddPathToTail(args[1], "game");
			AddPathToTail(args[1], "mod");
			AddPathToTail(args[1], "custom");
		}
		else {
			AddPathToTail(args[1], args[2]);
		}
	}

	// is FCVAR_NOTIFY even a valid thing for commands...?
	CON_COMMAND_F(sig_util_download_manager_reload_add, "Utility: reload the configuration file", FCVAR_NOTIFY)
	{
		if (s_Mod.IsEnabled()) {
			LoadDownloadsFile();
			GenerateDownloadables(update_timestamp);
			ResetVoteMapList();
		}
	}
	ModCommandClient sig_latedl_download("sig_latedl_download", [](CCommandPlayer *player, const CCommand& args){

		StopLateFilesToDownloadForPlayer(player->entindex());
		AddLateFilesToDownloadForPlayer(player->entindex(), late_dl_files, late_dl_files_current_mission_count);
	}, &s_Mod);
	
	ModCommandClient sig_latedl_current_mission_download_only("sig_latedl_current_mission_download_only", [](CCommandPlayer *player, const CCommand& args){
		auto &info = download_infos[player->entindex()];
		info.lateDlCurMissionOnlyInformed = true;
		auto &disallowed = info.lateDlCurMissionOnly;
		disallowed = !disallowed;

		char path[512];
		snprintf(path, 512, "%s%llu", latedl_curmission_only_path.c_str(), player->GetSteamID().ConvertToUint64());
		if (disallowed) {
			auto file = fopen(path, "w");
			if (file) {
				fclose(file);
			}
		}
		else {
			remove(path);
		}
		StopLateFilesToDownloadForPlayer(player->entindex());
		AddLateFilesToDownloadForPlayer(player->entindex(), late_dl_files, late_dl_files_current_mission_count);

		ClientMsg(player, disallowed ? "Only downloading icons from the current mission\n" : "Downloading icons from all missions\n");
	}, &s_Mod);

	ConVar cvar_mission_owner("sig_util_download_manager_mission_owner", "0", FCVAR_NONE, "Mission owner id");

	ConVar cvar_downloadpath("sig_util_download_manager_path", "download", FCVAR_NOTIFY,
		"Utility: specifiy the path to the custom files",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			if (server_activated)
				ReloadConfigIfModEnabled();
			CreateNotifyDirectory();
		});

	ConVar cvar_kvpath("sig_util_download_manager_kvpath", "cfg/downloads.kv", FCVAR_NOTIFY,
		"Utility: specify the path to the configuration file",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			ReloadConfigIfModEnabled();
		});
	
	
	ConVar cvar_enable("sig_util_download_manager", "0", FCVAR_NOTIFY,
		"Utility: add custom downloads from popfiles to the downloadables string table in a smart way, generates map list for mvm map votes. Recommended to set sig_perf_filesystem_optimize 1",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
