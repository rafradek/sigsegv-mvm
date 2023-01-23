#include "mod.h"
#include "stub/misc.h"
#include "soundchars.h"
#include "stub/tf_objective_resource.h"
#include "stub/populators.h"
#include "stub/gamerules.h"
#include "util/misc.h"
#include "util/clientmsg.h"
#include "util/iterate.h"

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

	bool server_activated = false;
	ConVar cvar_resource_file("sig_util_download_manager_resource_file", "tf_mvm_missioncycle.res", FCVAR_NONE, "Download Manager: Mission cycle file to write to",
		[](IConVar *pConVar, const char *pOldValue, float fOldValue) {
			if (server_activated)
				ResetVoteMapList();
		});

	ConVar cvar_mapcycle_file("sig_util_download_manager_maplist_file", "cfg/mapcycle.txt", FCVAR_NONE, "Download Manager: Map cycle file to write to",
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

	struct FileCasePresent
	{
		bool hasLowercase = false;
		bool hasNormal = false;
	};

	std::unordered_map<std::string, FileCasePresent> files_add;
	bool printmissing = false;
	bool missingfilemention = false;
	CHandle<CTFPlayer> mission_owner;
	std::unordered_multimap<int, std::string> missing_files;
	std::unordered_map<std::string, int> missing_files_dirs;

	void WatchMissingFile(std::string &path);

	void AddFileIfCustom(std::string value, bool onlyIfExist = false)
	{
		bool v;
		ReplaceAll(value, "\\","/");

		if (files_add.count(value)) return;

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
				WatchMissingFile(pathfull);
			}
		}

		if (add) {
			files_add[value] = {customNormalCase, customLowerCase};
		}
		else
			files_add[value] = {false, false};
			
		if (custom) {
			if (StringEndsWith(value.c_str(),".vmt", false)){

				KeyValues *kvroot = KeyValuesFromFile(customNormalCase ? pathfull.c_str() : pathfullLowercase.c_str(), value.c_str());
				KeyValues *kv = kvroot;
				if (kvroot != nullptr) {
					kv = kv->GetFirstSubKey();
					if (kv != nullptr) {
						//PrintToServer("loaded %s",name);
						do {
							if (kv->GetDataType() == KeyValues::TYPE_STRING){
								const char *name = kv->GetName();

								const char *texturename = kv->GetString();

								if (strchr(texturename, '/') != nullptr || strchr(texturename, '\\') != nullptr || StringEndsWith(texturename, ".vtf", false)) {
									char texturepath[256];
									snprintf(texturepath, 256, "materials/%s%s",texturename, !StringEndsWith(texturename, ".vtf", false) ? ".vtf" : "");
									//PrintToServer("found texturename %s",texturepath);
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
					if (FStrEq(name, "ClassIcon")) {
						const char *value = kv->GetString();
						char fmt[200];
						snprintf(fmt,200,"%s%s%s","materials/hud/leaderboard_class_",value,".vmt");
						AddFileIfCustom(fmt);
					}
					else if(FStrEq(name, "CustomUpgradesFile")) {
						const char *value = kv->GetString();
						char fmt[200];
						snprintf(fmt,200,"%s%s","scripts/items/",value);
						AddFileIfCustom(fmt);
					}
					else if(StringEndsWith(name, "Decal", false)) {
						const char *value = kv->GetString();
						char fmt[200];
						snprintf(fmt,200,"%s%s",value,".vmt");

						AddFileIfCustom(fmt);
					}
					else if(StringEndsWith(name,"Generic", false)){
						const char *value = kv->GetString();

						AddFileIfCustom(value);
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
							char fmt[200];
							snprintf(fmt,200,"%s%s","sound/", value);
							AddFileIfCustom(fmt);
						}
					}
				}
			}
		}
		while ((kv = kv->GetNextKey()) != nullptr);
	}

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

		char poppath[256];
		snprintf(poppath, sizeof(poppath), "%s/%s/scripts/population", game_path, cvar_downloadpath.GetString());
		char filepath[512];
		char respath[512];
		DIR *dir;
		dirent *ent;
		const char *map = STRING(gpGlobals->mapname);
		
		files_add.clear();
		if (timestamp == 0)
			missing_files.clear();

		auto admin = GetMissionOwner();
		mission_owner = admin;
		const char *currentMission = (admin != nullptr && TFObjectiveResource() != nullptr) ? STRING(TFObjectiveResource()->m_iszMvMPopfileName.Get()) : nullptr;

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
						printmissing = admin != nullptr && strcmp(respath, currentMission) == 0;
						KeyValueBrowse(kv);
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
			AddFileIfCustom(StringStartsWith(upgradeFile, "download/") ? upgradeFile + strlen("download/") : upgradeFile);

		if (missingfilemention && admin != nullptr) {
			PrintToChat("Some files are missing on the server, check console for details\n", admin);
			missingfilemention = false;
		}
		popfiles_to_forced_update.clear();

		bool saved_lock = engine->LockNetworkStringTables(false);

		for (auto &entry : files_add) {
			if (entry.second.hasLowercase) {
				downloadables->AddString(true, boost::algorithm::to_lower_copy(entry.first).c_str());
			}
			if (entry.second.hasNormal) {
				//Msg("%s\n", entry.first.c_str());
				downloadables->AddString(true, entry.first.c_str());
			}
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
							printmissing = false;
							KeyValueBrowse(kv);
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

	void AddMapToVoteList(const char *mapName, KeyValues *kv, int &files, std::string &maplistStr)
	{
		std::string mapNameStr(mapName, strlen(mapName) - 4);
		if (banned_maps.count(mapNameStr)) return;

		files++;

		maplistStr.append(mapNameStr);
		maplistStr.push_back('\n');

		auto kvmap = kv->CreateNewKey();
		kvmap->SetString("map", mapNameStr.c_str());
		kvmap->SetString("popfile", mapNameStr.c_str());
	}

	void ResetVoteMapList() 
	{
		case_sensitive_toggle = true;
		CFastTimer timer1;
		timer1.Start();
		KeyValues *kv = new KeyValues(cvar_resource_file.GetString());
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

		
		kvcat->SetInt("count", files);
		kv->SaveToFile(filesystem, cvar_resource_file.GetString());
		kv->deleteThis();

		engine->ServerCommand("tf_mvm_missioncyclefile empty\n");
		engine->ServerCommand(CFmtStr("tf_mvm_missioncyclefile %s\n",cvar_resource_file.GetString()));

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

			if (!poplistStr.empty() && strtablepoplist->GetNumStrings() > 0)
				strtablepoplist->SetStringUserData(0, poplistStr.size() + 1, poplistStr.c_str());
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
		DETOUR_MEMBER_CALL(CServerGameDLL_ServerActivate)(pEdictList, edictList, clientMax);
		
		server_activated = true;
		LoadDownloadsFile();
		GenerateDownloadables();
		ResetVoteMapList();

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
		//return DETOUR_STATIC_CALL(findFileInDirCaseInsensitive)(file, output, bufSize);
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
			std::string pathDir = path.substr(0,path.rfind('/'));

			auto find = missing_files_dirs.find(pathDir);
			missing_files.emplace(find != missing_files_dirs.end() ? find->second : inotify_add_watch(inotify_fd, pathDir.c_str(), IN_CREATE | IN_MOVED_TO), path);
		}
	}

	class CMod : public IMod, IModCallbackListener, IFrameUpdatePostEntityThinkListener
	{
	public:


		CMod() : IMod("Util:Download_Manager")
		{
			//
			MOD_ADD_DETOUR_MEMBER(CServerGameDLL_ServerActivate, "CServerGameDLL::ServerActivate");

			// Faster implementation of findFileInDirCaseInsensitive, instead of looking for any matching file with a different case, only look for files with lowercase letters instead
			//MOD_ADD_DETOUR_STATIC(findFileInDirCaseInsensitive, "findFileInDirCaseInsensitive");
		}
		virtual bool OnLoad() override
		{
			game_path = g_SMAPI->GetBaseDir();
			base_path_len = filesystem->GetSearchPath( "BASE_PATH", true, nullptr, 0);
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

			LoadDownloadsFile();
			GenerateDownloadables();
			CreateNotifyDirectory();
		}
		virtual void OnDisable() override
		{
			filesystem->RemoveSearchPaths("vpks");
			StopNotifyDirectory();
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

		virtual void FrameUpdatePostEntityThink() override
		{
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
					int length, i = 0;
					length = read( inotify_fd, buffer, BUF_LEN );  

					while (i < length) {
						struct inotify_event *event = (struct inotify_event *) &buffer[i];
						if (event->len) {
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

										INetworkStringTable *downloadables = networkstringtable->FindTable("downloadables");
										bool saved_lock = engine->LockNetworkStringTables(false);
										downloadables->AddString(true, realFileName.c_str());
										engine->LockNetworkStringTables(saved_lock);
										
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
		}
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
		}
	}

	// is FCVAR_NOTIFY even a valid thing for commands...?
	CON_COMMAND_F(sig_util_download_manager_missing_files, "Utility: list missing files", FCVAR_NOTIFY)
	{
		for (auto &value : missing_files) {
			Msg("Missing file: %s\n", value.second.c_str());
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
			std::string oldFullPath = fmt::format("{}/{}", game_path, pOldValue);
			filesystem->RemoveSearchPath(oldFullPath.c_str(), "game");
			filesystem->RemoveSearchPath(oldFullPath.c_str(), "mod");
			filesystem->RemoveSearchPath(oldFullPath.c_str(), "custom");
			
			AddPathToTail(cvar_kvpath.GetString(), "game");
			AddPathToTail(cvar_kvpath.GetString(), "mod");
			AddPathToTail(cvar_kvpath.GetString(), "custom");
			
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
