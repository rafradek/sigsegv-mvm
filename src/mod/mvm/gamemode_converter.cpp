#include "mod.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"
#include <boost/algorithm/string.hpp>
#include "util/misc.h"


namespace Mod::MvM::Gamemode_Converter
{
	StaticFuncThunk<const char *, const char *, char *> ft_MapEntity_ParseToken("MapEntity_ParseToken");
    inline const char *MapEntity_ParseToken(const char *data, char *token) { return ft_MapEntity_ParseToken(data, token); }

    class CaseInsensitveCompare 
    {
    public:
        bool operator() (const std::string &lhs, const std::string &rhs) const
        {
            return stricmp(lhs.c_str(), rhs.c_str()) == 0;
        }
    };
    
    class CaseInsensitveHash
    {
    public:
        std::size_t operator() (std::string str) const
        {
            boost::algorithm::to_lower(str);

            return std::hash<std::string>{}(str);
        }
    };
    

    using ParsedEntity = std::unordered_multimap<std::string, std::string, CaseInsensitveHash, CaseInsensitveCompare>;

    bool IsMvMMapCheck(const char *entities)
    {
        const char *currentKey = entities;
        char token[2048];
        char keyName[2048];
        while ((currentKey = MapEntity_ParseToken(currentKey, token))) {
            if (stricmp(token, "tf_logic_mann_vs_machine") == 0) {
                return true;
            }
        }
        return false;
    }

    void ParseEntityString(const char *entities, std::list<ParsedEntity> &parsedEntities)
    {
        const char *currentKey = entities;
        char token[2048];
        char keyName[2048];
        bool parsedKey = false;
        ParsedEntity *parsedEntity = nullptr;
        while ((currentKey = MapEntity_ParseToken(currentKey, token))) {
            if (token[0] == '{') {
                
                parsedEntity = &parsedEntities.emplace_back();
                parsedKey = false;
            }
            else if (token[0] == '}') {

            }
            else {
                if (!parsedKey) {
                    parsedKey = true;
                    strncpy(keyName, token, 2048);
                }
                else {
                    parsedKey = false;
                    parsedEntity->emplace(keyName, token);
                }
            }
        }
    }

    void WriteEntityString(std::string &entities, std::list<ParsedEntity> &parsedEntities)
    {
        entities.clear();
        for(auto &parsedEntity : parsedEntities) {
            entities.append("{\n");
            for (auto &entry : parsedEntity) {
                entities.push_back('"');
                entities.append(entry.first);
                entities.append("\" \"");
                entities.append(entry.second);
                entities.append("\"\n");
            }
            entities.append("}\n");
        }
    }

    std::string entitiesStr;

	DETOUR_DECL_STATIC(void, MapEntity_ParseAllEntities, const char *pMapData, void *pFilter, bool bActivateEntities)
	{
        entitiesStr.clear();
        entitiesStr.shrink_to_fit();
        
        bool foundMvmGame = IsMvMMapCheck(pMapData);

        if (foundMvmGame) {
            DETOUR_STATIC_CALL(MapEntity_ParseAllEntities)(pMapData, pFilter, bActivateEntities);
            return;
        }

        std::list<ParsedEntity> parsedEntities; 

        ParseEntityString(pMapData, parsedEntities);

        std::string lastPointTargetname;
        std::string lastPointOrigin;
        std::string lastBluPointTargetname;
        std::string bombPos;
        std::vector<std::list<ParsedEntity>::iterator> captureAreas;
        std::vector<std::list<ParsedEntity>::iterator> controlPoints;
        std::set<std::string> controlPointsNames;
        std::set<std::string> controlPointsNamesBlu;
        std::vector<std::list<ParsedEntity>::iterator> redSpawnRooms;
        std::vector<std::list<ParsedEntity>::iterator> redResupply;
        std::vector<std::list<ParsedEntity>::iterator> namedPropDynamic;
        std::set<std::string> regenerateModelEntityNames;
        std::string trainEntity;
        std::string pathStart;
        std::string pathEnd;
        bool firstTeamSpawn = true;

        for (auto it = parsedEntities.begin(); it != parsedEntities.end();) {
            auto &parsedEntity = *it;
            auto teamnum = parsedEntity.find("teamnum");
            
            bool isRedTeam = teamnum != parsedEntity.end() && std::stoi(teamnum->second) != 3;

            std::string &classname = parsedEntity.find("classname")->second;

            if (classname == "func_capturezone") {
                // Delete blue capture zone
                if (!isRedTeam) {
                    it = parsedEntities.erase(it);
                    continue;
                }
                // Turn red capture zone into blue zone
                else {
                    parsedEntity.erase("teamnum");
                    parsedEntity.emplace("TeamNum", "3");
                    parsedEntity.emplace("OnCapture", "bots_win,RoundWin,,0,-1");
                }
            }
            if (classname == "item_teamflag") {
                // Delete team flag
                it = parsedEntities.erase(it);
                continue;
            }
            if (classname == "team_control_point") {
                // Turn last capture point to capture zone
                auto previous = parsedEntity.find("team_previouspoint_3_0");
                auto previousRed = parsedEntity.find("team_previouspoint_2_0");
                if (previous != parsedEntity.end() && !previous->second.empty()) {
                    controlPointsNames.insert(previous->second);
                }
                auto pointDefaultOwner = parsedEntity.find("point_default_owner");
                if (pointDefaultOwner != parsedEntity.end() && pointDefaultOwner->second == "3" && ((previousRed != parsedEntity.end() && !previousRed->second.empty()) || parsedEntity.find("point_index")->second == "1") && !(previous != parsedEntity.end() && !previous->second.empty())) {
                    lastBluPointTargetname = parsedEntity.find("targetname")->second;
                    it = parsedEntities.erase(it);
                    continue;
                }
                
                if (parsedEntity.count("targetname")) {
                    controlPoints.push_back(it);
                }
                
                parsedEntity.erase("point_default_owner");
                parsedEntity.emplace("point_default_owner", "2");
                
                parsedEntity.emplace("oncapteam2", "nav_interface_generated,recomputeblockers,,5,-1");

            }
            if (classname == "trigger_capture_area") {
                // Find capture areas, then add them to the list
                captureAreas.push_back(it);

                parsedEntity.erase("team_cancap_2");
                parsedEntity.emplace("team_cancap_2", "0");
                parsedEntity.erase("team_cancap_3");
                parsedEntity.emplace("team_cancap_3", "1");
                
            }
            if (classname == "info_player_teamspawn") {
                //Add spawnbot name to blue spawns
                if (!isRedTeam) {
                    parsedEntity.erase("targetname");
                    if (firstTeamSpawn) {
                        firstTeamSpawn = false;
                        auto cloned = parsedEntities.emplace(it, parsedEntity);
                        cloned->emplace("targetname", "spawnbot_mission_spy");
                        auto cloned2 = parsedEntities.emplace(it, parsedEntity);
                        cloned2->emplace("targetname", "spawnbot_mission_sniper");
                        auto cloned3 = parsedEntities.emplace(it, parsedEntity);
                        cloned3->emplace("targetname", "spawnbot_giant");
                        auto cloned4 = parsedEntities.emplace(it, parsedEntity);
                        cloned4->emplace("targetname", "spawnbot_invasion");
                        auto cloned5 = parsedEntities.emplace(it, parsedEntity);
                        cloned5->emplace("targetname", "spawnbot_lower");
                        auto cloned6 = parsedEntities.emplace(it, parsedEntity);
                        cloned6->emplace("targetname", "spawnbot_left");
                        auto cloned7 = parsedEntities.emplace(it, parsedEntity);
                        cloned7->emplace("targetname", "spawnbot_right");
                        auto cloned8 = parsedEntities.emplace(it, parsedEntity);
                        cloned8->emplace("targetname", "spawnbot_mission_sentry_buster");
                        auto cloned9 = parsedEntities.emplace(it, parsedEntity);
                        cloned9->emplace("targetname", "spawnbot_chief");
                        auto cloned10 = parsedEntities.emplace(it, parsedEntity);
                        cloned10->emplace("targetname", "flankers");
                        

                        if (parsedEntity.count("origin")) {
                            bombPos = parsedEntity.find("origin")->second;
                        }
                    }
                    parsedEntity.emplace("targetname", "spawnbot");
                }
            }
            if (classname == "team_control_point_master") {
                parsedEntity.erase("cpm_restrict_team_cap_win");
                parsedEntity.emplace("cpm_restrict_team_cap_win", "1");
                parsedEntity.erase("custom_position_x");
                parsedEntity.emplace("custom_position_x", "0.4");
            }
            if (classname == "tf_logic_koth") {
                it = parsedEntities.erase(it);
                continue;
            }
            if (classname == "func_regenerate") {
                if (isRedTeam) {
                    redResupply.push_back(it);
                    auto model = parsedEntity.find("associatedmodel");
                    if (model != parsedEntity.end() && !model->second.empty()) {
                        regenerateModelEntityNames.insert(model->second);
                    }
                }
            }
            if (classname == "func_respawnroom") {
                if (isRedTeam) {
                    redSpawnRooms.push_back(it);
                }
            }
            if (classname == "prop_dynamic") {
                auto name = parsedEntity.find("targetname");
                if (name != parsedEntity.end() && !name->second.empty()) {
                    namedPropDynamic.push_back(it);
                }
            }
            if (classname == "team_train_watcher") {
                auto name = parsedEntity.find("train");
                if (name != parsedEntity.end()) {
                    trainEntity = name->second;
                }
                auto start = parsedEntity.find("start_node");
                if (start != parsedEntity.end()) {
                    pathStart = start->second;
                }
                auto end = parsedEntity.find("goal_node");
                if (end != parsedEntity.end()) {
                    pathEnd = end->second;
                }
                it = parsedEntities.erase(it);
                continue;
            }

            it++;
        }

        for (auto &it : controlPoints) {
            auto targetname = it->find("targetname");
            if (!controlPointsNames.count(targetname->second)) {
                lastPointTargetname = targetname->second;
                parsedEntities.erase(it);
                break;
            }
        }
        
        bool foundLastCaptureArea = false;
        // Last capture area becomes the capture zone
        for (auto &it : captureAreas) {
            auto targetname = it->find("area_cap_point");
            if (targetname != it->end() && targetname->second == lastPointTargetname) {
                it->erase("classname");
                it->emplace("classname", "func_capturezone");
                it->erase("teamnum");
                it->emplace("teamnum", "3");
                it->emplace("OnCapture", "bots_win,RoundWin,,0,-1");
                foundLastCaptureArea = true;
            }
            else if (targetname != it->end() && targetname->second == lastBluPointTargetname) {
                parsedEntities.erase(it);
            }
            else {
                it->erase("filter");
                it->emplace("filter", "filter_gatebot");
            }
        }

        for (auto &it : namedPropDynamic) {
            auto targetname = it->find("targetname");
            if (regenerateModelEntityNames.count(targetname->second)) {
                it->erase("model");
                it->emplace("model", "models/props_mvm/mvm_upgrade_center.mdl");
                it->erase("solid");
                it->emplace("solid", "0");
                QAngle angles;
                Vector origin;
                auto anglesstr = it->find("angles");
                auto originstr = it->find("origin");
                if (anglesstr != it->end()) {
                    UTIL_StringToAnglesAlt(angles, anglesstr->second.c_str());
                }
                if (originstr != it->end()) {
                    UTIL_StringToVectorAlt(origin, originstr->second.c_str());
                }
                Vector forward;
                AngleVectors(angles, &forward);
                origin -= forward * 72;
                it->erase("origin");
                it->emplace("origin", CFmtStr("%f %f %f", origin.x, origin.y, origin.z));
            }
        }

        for (auto &it : redResupply) {
            it->erase("classname");
            it->emplace("classname", "func_upgradestation");
        }

        if (redResupply.empty()) {
            for (auto &it : redSpawnRooms) {
                auto upgrade = parsedEntities.emplace(it, *it);
                upgrade->erase("classname");
                upgrade->emplace("classname", "func_upgradestation");
            }
        }

        if (!trainEntity.empty() || !pathStart.empty()) {
            
            for (auto it = parsedEntities.begin(); it != parsedEntities.end();) {
                auto targetname = it->find("targetname");
                if (targetname != it->end() && !targetname->second.empty()) {
                    if (targetname->second == trainEntity) {
                        Msg("Found train");
                        
                        auto &mover = parsedEntities.emplace_back();
                        mover.emplace("classname", "logic_relay");
                        mover.emplace("OnSpawn", CFmtStr("%s,addoutput,origin -20000 -20000 -20000,0,-1", trainEntity.c_str()));
                        mover.emplace("OnSpawn", "!self,kill,,0.1,-1");
                    }
                    if (targetname->second == pathEnd) {
                        Msg("Found path end");
                        lastPointOrigin = it->find("origin")->second;
                    }
                    if (targetname->second == pathStart) {
                        Msg("Found path start");
                        it->erase("targetname");
                        auto cloned = parsedEntities.emplace(it, *it);
                        cloned->emplace("targetname", "boss_path_a1");
                        auto cloned2 = parsedEntities.emplace(it, *it);
                        cloned2->emplace("targetname", "boss_path_b1");
                        auto cloned3 = parsedEntities.emplace(it, *it);
                        cloned3->emplace("targetname", "boss_path_1");
                        auto cloned4 = parsedEntities.emplace(it, *it);
                        cloned4->emplace("targetname", "boss_path2_1");
                        auto cloned5 = parsedEntities.emplace(it, *it);
                        cloned5->emplace("targetname", "tank_path_a_10");
                        it->emplace("targetname", "tank_path_b_10");
                    }
                }
                it++;
            }
        }

        // Haven't found a capture area for the final point, create one
        Msg("Area: %d %s\n", foundLastCaptureArea, lastPointOrigin.c_str());
        if (!foundLastCaptureArea && !lastPointOrigin.empty()) {
            Msg("Create Area");
            auto &area = parsedEntities.emplace_back();
            area.emplace("classname", "func_capturezone");
            area.emplace("teamnum", "3");
            area.emplace("OnCapture", "bots_win,RoundWin,,0,-1");
            area.emplace("origin", lastPointOrigin);
            area.emplace("model", "models/weapons/w_models/w_rocket.mdl");
            area.emplace("solid", "2");
            area.emplace("mins", "-100 -100 -100");
            area.emplace("maxs", "100 100 100");
            area.emplace("targetname", "capture_zone_generated");
            area.emplace("effects", "32");
            auto &mover = parsedEntities.emplace_back();
            mover.emplace("classname", "logic_relay");
            mover.emplace("OnSpawn", "capture_zone_generated,addoutput,solid 2,0,-1");
            mover.emplace("OnSpawn", "capture_zone_generated,addoutput,mins -50 -50 -130,0.01,-1");
            mover.emplace("OnSpawn", "capture_zone_generated,addoutput,maxs 50 50 130,0.02,-1");
            mover.emplace("OnSpawn", "capture_zone_generated,addoutput,effects 32,0.03,-1");
            mover.emplace("OnSpawn", "!self,kill,,0.04,-1");

        }

        auto &mvmLogic = parsedEntities.emplace_back();
        mvmLogic.emplace("classname", "tf_logic_mann_vs_machine");
        
        auto &bomb = parsedEntities.emplace_back();
        bomb.emplace("classname", "item_teamflag");
        bomb.emplace("targetname", "intel");
        bomb.emplace("flag_model", "models/props_td/atom_bomb.mdl");
        bomb.emplace("tags", "bomb_carrier");
        bomb.emplace("ReturnTime", "60000");
        bomb.emplace("GameType", "1");
        bomb.emplace("TeamNum", "3");
        bomb.emplace("origin", bombPos);
        

        auto &botsWin = parsedEntities.emplace_back();
        botsWin.emplace("classname", "game_round_win");
        botsWin.emplace("targetname", "bots_win");
        botsWin.emplace("teamnum", "3");

        auto &bombDeployRelay = parsedEntities.emplace_back();
        bombDeployRelay.emplace("classname", "logic_relay");
        bombDeployRelay.emplace("targetname", "boss_deploy_relay");
        bombDeployRelay.emplace("OnTrigger", "bots_win,RoundWin,,0,-1");

        auto &filterGatebot = parsedEntities.emplace_back();
        filterGatebot.emplace("classname", "filter_tf_bot_has_tag");
        filterGatebot.emplace("targetname", "filter_gatebot");
        filterGatebot.emplace("tags", "bot_gatebot");

        auto &navInterface = parsedEntities.emplace_back();
        navInterface.emplace("classname", "tf_point_nav_interface");
        navInterface.emplace("targetname", "nav_interface_generated");

        auto &spawnTrigger = parsedEntities.emplace_back();
        spawnTrigger.emplace("classname", "logic_relay");
        spawnTrigger.emplace("OnSpawn", "nav_interface_generated,recomputeblockers,,5,-1");
        spawnTrigger.emplace("OnSpawn", "nav_interface_generated,recomputeblockers,,15,-1");
        spawnTrigger.emplace("OnSpawn", "!self,kill,,30,-1");
        
        WriteEntityString(entitiesStr, parsedEntities);

        CTFPlayer::PrecacheMvM();
        DETOUR_STATIC_CALL(MapEntity_ParseAllEntities)(entitiesStr.c_str(), pFilter, bActivateEntities);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Gamemode_Converter")
		{
            MOD_ADD_DETOUR_STATIC(MapEntity_ParseAllEntities, "MapEntity_ParseAllEntities");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_gamemode_converter", "0", FCVAR_NOTIFY,
		"Mod: Convert other gamemodes into mvm gamemode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}