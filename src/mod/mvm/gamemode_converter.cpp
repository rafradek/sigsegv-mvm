#include "mod.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include "util/misc.h"
#include "stub/nav.h"


namespace Mod::MvM::Gamemode_Converter
{
	StaticFuncThunk<const char *, const char *, char *> ft_MapEntity_ParseToken("MapEntity_ParseToken");
    inline const char *MapEntity_ParseToken(const char *data, char *token) { return ft_MapEntity_ParseToken(data, token); }
    

    using ParsedEntity = std::unordered_multimap<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveCompare>;

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
    bool foundMvmGame = false;

	DETOUR_DECL_STATIC(void, MapEntity_ParseAllEntities, const char *pMapData, void *pFilter, bool bActivateEntities)
	{
        entitiesStr.clear();
        entitiesStr.shrink_to_fit();
        
        foundMvmGame = IsMvMMapCheck(pMapData);

        if (foundMvmGame) {
            DETOUR_STATIC_CALL(pMapData, pFilter, bActivateEntities);
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

        bool shouldPlaceHatch = false;
        Vector hatchPropOrigin = vec3_origin;

        std::list<ParsedEntity>::iterator captureZone = parsedEntities.end();

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
                    captureZone = it;
                    shouldPlaceHatch = true;
                    auto origin = parsedEntity.find("origin");
                    if (origin != parsedEntity.end()) {
                        UTIL_StringToVectorAlt(hatchPropOrigin, origin->second.c_str());
                    }
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
                parsedEntity.erase("filtername");
                parsedEntity.emplace("filtername", "filter_gatebot");
                
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

        Vector lastPointToConvertOrigin = vec3_origin;
        for (auto &it : controlPoints) {
            auto targetname = it->find("targetname");
            if (!controlPointsNames.count(targetname->second)) {
                lastPointTargetname = targetname->second;
                auto origin = it->find("origin");
                if (origin != it->end()) {
                    UTIL_StringToVectorAlt(lastPointToConvertOrigin, origin->second.c_str());
                }
                parsedEntities.erase(it);
                break;
            }
        }
        
        bool foundLastCaptureArea = false;
        // Last capture area becomes the capture zone
        for (auto &it : captureAreas) {
            auto &cpArea = *it;
            auto targetname = cpArea.find("area_cap_point");
            if (targetname != cpArea.end() && targetname->second == lastPointTargetname) {
                cpArea.erase("classname");
                cpArea.emplace("classname", "func_capturezone");
                cpArea.erase("targetname");
                cpArea.emplace("targetname", "bombcapturezone");
                cpArea.erase("model");
                cpArea.emplace("model", "models/weapons/w_models/w_rocket.mdl");
                cpArea.erase("origin");
                cpArea.emplace("origin", CFmtStr("%f %f %f", lastPointToConvertOrigin.x, lastPointToConvertOrigin.y, lastPointToConvertOrigin.z));
                foundLastCaptureArea = true;
                captureZone = it;
                auto &mover = parsedEntities.emplace_back();
                mover.emplace("classname", "logic_relay");
                mover.emplace("OnSpawn", "bombcapturezone,addoutput,solid 2,0,-1");
                mover.emplace("OnSpawn", "bombcapturezone,addoutput,mins -50 -50 -130,0.01,-1");
                mover.emplace("OnSpawn", "bombcapturezone,addoutput,maxs 50 50 130,0.02,-1");
                mover.emplace("OnSpawn", "bombcapturezone,addoutput,effects 32,0.03,-1");
                mover.emplace("OnSpawn", "!self,kill,,0.04,-1");
            }
            else if (targetname != cpArea.end() && targetname->second == lastBluPointTargetname) {
                parsedEntities.erase(it);
            }
            else {
                cpArea.erase("filter");
                cpArea.emplace("filter", "filter_gatebot");
            }
        }

        std::list<ParsedEntity>::iterator capProp;
        bool foundCapProp = false;
        float capPropClosestDist = FLT_MAX;
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
            auto model = it->find("model");
            if (model != it->end() && model->second.find("cap_point_base.mdl") != std::string::npos) {
                auto origin = it->find("origin");
                Vector vecOrigin = vec3_origin;
                if (origin != it->end()) {
                    UTIL_StringToVectorAlt(vecOrigin, origin->second.c_str());
                }
                shouldPlaceHatch = true;
                auto dist = vecOrigin.DistToSqr(lastPointToConvertOrigin);
                foundCapProp = true;
                if (dist < capPropClosestDist) {
                    capPropClosestDist = dist;
                    capProp = it;
                    hatchPropOrigin = vecOrigin;
                }
            }
        }

        if (foundCapProp) {
            parsedEntities.erase(capProp);
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
                        auto cloned6 = parsedEntities.emplace(it, *it);
                        cloned6->emplace("targetname", "tank_path_b_10");
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
            captureZone = (--parsedEntities.end());
            area.emplace("classname", "func_capturezone");
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
        botsWin.emplace("force_map_reset", "1");
        botsWin.emplace("teamnum", "3");

        auto &bossDeployRelay = parsedEntities.emplace_back();
        bossDeployRelay.emplace("classname", "logic_relay");
        bossDeployRelay.emplace("targetname", "boss_deploy_relay");
        bossDeployRelay.emplace("OnTrigger", "hatch_bust_relay,trigger,,0,-1");
        bossDeployRelay.emplace("OnTrigger", "bots_win,RoundWin,,0,-1");

        auto &hol3WayRelay = parsedEntities.emplace_back();
        hol3WayRelay.emplace("classname", "logic_relay");
        hol3WayRelay.emplace("targetname", "holograms_3way_relay");
        hol3WayRelay.emplace("ontrigger", "wave_init_relay,trigger,,0,-1");

        auto &holCenterRelay = parsedEntities.emplace_back();
        holCenterRelay.emplace("classname", "logic_relay");
        holCenterRelay.emplace("targetname", "holograms_centerpath_relay");
        holCenterRelay.emplace("ontrigger", "wave_init_relay,trigger,,0,-1");

        auto &waveInitRelay = parsedEntities.emplace_back();
        waveInitRelay.emplace("classname", "logic_relay");
        waveInitRelay.emplace("targetname", "wave_init_relay");
        waveInitRelay.emplace("ontrigger", "team_control_point,setowner,2,0,-1");

        auto &waveStartRelay = parsedEntities.emplace_back();
        waveStartRelay.emplace("classname", "logic_relay");
        waveStartRelay.emplace("targetname", "wave_start_relay");

        auto &waveFinishedRelay = parsedEntities.emplace_back();
        waveFinishedRelay.emplace("classname", "logic_relay");
        waveFinishedRelay.emplace("targetname", "wave_finished_relay");

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
        
        if (shouldPlaceHatch) {
            auto &hatch = parsedEntities.emplace_back();
            hatch.emplace("classname", "prop_dynamic");
            hatch.emplace("targetname", "hatch_prop");
            hatch.emplace("model", "models/props_mvm/mann_hatch.mdl");
            hatch.emplace("solid", "0");
            hatch.emplace("origin", CFmtStr("%f %f %f", hatchPropOrigin.x, hatchPropOrigin.y, hatchPropOrigin.z));

            auto &hatchBusted = parsedEntities.emplace_back();
            hatchBusted.emplace("classname", "prop_dynamic");
            hatchBusted.emplace("targetname", "hatch_busted");
            hatchBusted.emplace("startdisabled", "1");
            hatchBusted.emplace("model", "models/props_mvm/mann_hatch_destruction_mvm_deathpit.mdl");
            hatchBusted.emplace("solid", "0");
            hatchBusted.emplace("origin", CFmtStr("%f %f %f", hatchPropOrigin.x, hatchPropOrigin.y, hatchPropOrigin.z));

            auto &hatchBoom = parsedEntities.emplace_back();
            hatchBoom.emplace("classname", "info_particle_system");
            hatchBoom.emplace("targetname", "hatch_boom");
            hatchBoom.emplace("effectname", "mvm_hatch_destroy");
            hatchBoom.emplace("origin", CFmtStr("%f %f %f", hatchPropOrigin.x, hatchPropOrigin.y, hatchPropOrigin.z));
            
            auto &hatchBoomSound = parsedEntities.emplace_back();
            hatchBoomSound.emplace("classname", "ambient_generic");
            hatchBoomSound.emplace("targetname", "hatch_boom_sound");
            hatchBoomSound.emplace("message", "MVM.BombExplodes");
            hatchBoomSound.emplace("spawnflags", "49");
            hatchBoomSound.emplace("health", "10");
            hatchBoomSound.emplace("origin", CFmtStr("%f %f %f", hatchPropOrigin.x, hatchPropOrigin.y, hatchPropOrigin.z + 40));

            auto &hatchMagnet = parsedEntities.emplace_back();
            hatchMagnet.emplace("classname", "phys_ragdollmagnet");
            hatchMagnet.emplace("targetname", "hatch_magnet");
            hatchMagnet.emplace("radius", "1376");
            hatchMagnet.emplace("startdisabled", "1");
            hatchMagnet.emplace("force", "-500000");
            hatchMagnet.emplace("axis", "-2320 -2976 448");
            hatchMagnet.emplace("origin", CFmtStr("%f %f %f", hatchPropOrigin.x, hatchPropOrigin.y, hatchPropOrigin.z - 100));

            auto &hatchHurt = parsedEntities.emplace_back();
            hatchHurt.emplace("classname", "func_mutliple");
            hatchHurt.emplace("targetname", "hatch_hurt");
            hatchHurt.emplace("startdisabled", "1");
            hatchHurt.emplace("model", "models/weapons/w_models/w_rocket.mdl");
            hatchHurt.emplace("solid", "2");
            hatchHurt.emplace("mins", "-160 -160 -160");
            hatchHurt.emplace("maxs", "160 160 40");
            hatchHurt.emplace("origin", CFmtStr("%f %f %f", hatchPropOrigin.x, hatchPropOrigin.y, hatchPropOrigin.z));
            hatchHurt.emplace("OnStartTouch", "!activator,SetHealth,-1000,0,-1");

            auto &hatchHurtBig = parsedEntities.emplace_back();
            hatchHurtBig.emplace("classname", "func_mutliple");
            hatchHurtBig.emplace("targetname", "hatch_kill");
            hatchHurtBig.emplace("startdisabled", "1");
            hatchHurtBig.emplace("model", "models/weapons/w_models/w_rocket.mdl");
            hatchHurtBig.emplace("solid", "2");
            hatchHurtBig.emplace("mins", "-600 -600 -160");
            hatchHurtBig.emplace("maxs", "600 600 400");
            hatchHurtBig.emplace("origin", CFmtStr("%f %f %f", hatchPropOrigin.x, hatchPropOrigin.y, hatchPropOrigin.z));
            hatchHurtBig.emplace("OnStartTouch", "!activator,SetHealth,-1000,0,-1");

            auto &hatchNoBuild = parsedEntities.emplace_back();
            hatchNoBuild.emplace("classname", "func_nobuild");
            hatchNoBuild.emplace("targetname", "hatch_nobuild");
            hatchNoBuild.emplace("model", "models/weapons/w_models/w_rocket.mdl");
            hatchNoBuild.emplace("solid", "2");
            hatchNoBuild.emplace("mins", "-200 -200 -50");
            hatchNoBuild.emplace("maxs", "200 200 40");
            hatchNoBuild.emplace("origin", CFmtStr("%f %f %f", hatchPropOrigin.x, hatchPropOrigin.y, hatchPropOrigin.z));

            auto &hatchDestroyAnim = parsedEntities.emplace_back();
            hatchDestroyAnim.emplace("classname", "logic_case");
            hatchDestroyAnim.emplace("targetname", "hatch_animselect");
            hatchDestroyAnim.emplace("OnCase01", "hatch_busted,SetAnimation,deathpit1,0,-1");
            hatchDestroyAnim.emplace("OnCase02", "hatch_busted,SetAnimation,deathpit2,0,-1");
            hatchDestroyAnim.emplace("OnCase03", "hatch_busted,SetAnimation,deathpit3,0,-1");

            auto &hatchDestroyRelay = parsedEntities.emplace_back();
            hatchDestroyRelay.emplace("classname", "logic_relay");
            hatchDestroyRelay.emplace("targetname", "hatch_bust_relay");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_prop,Disable,,0,-1");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_busted,Enable,,0,-1");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_animselect,PickRandom,,0,-1");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_magnet,Enable,,0,-1");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_boom,Start,,0,-1");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_boom_sound,PlaySound,,0,-1");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_kill,Enable,,0,-1");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_hurt,Enable,,0,-1");
            hatchDestroyRelay.emplace("OnTrigger", "hatch_kill,Disable,,0.5,-1");
        }

        if (captureZone != parsedEntities.end()) {
            captureZone->erase("teamnum");
            captureZone->emplace("teamnum", "3");
            captureZone->emplace("OnCapture", "bots_win,RoundWin,,0,-1");
            captureZone->emplace("OnCapture", "hatch_bust_relay,trigger,,1,-1");
        }

        WriteEntityString(entitiesStr, parsedEntities);

        CTFPlayer::PrecacheMvM();
        DETOUR_STATIC_CALL(entitiesStr.c_str(), pFilter, bActivateEntities);
	}

    CountdownTimerInline generateMeshTimer;
    bool generateMesh = false;
	DETOUR_DECL_MEMBER(int, CNavMesh_Load)
	{
        auto value = DETOUR_MEMBER_CALL();
        // Generate nav mesh if missing
        if (value == 1) {
            generateMesh = true;
            generateMeshTimer.Start(1.0f);
        }
        return value;
    }

    DETOUR_DECL_STATIC(void, CPopulationManager_FindDefaultPopulationFileShortNames, CUtlVector<CUtlString> &vec)
	{
		DETOUR_STATIC_CALL(vec);

        if (foundMvmGame) return;

        // Show any popfile that starts with global_ prefix
        FileFindHandle_t popHandle;
        const char *popFileName = filesystem->FindFirstEx( "scripts/population/global_*.pop", "GAME", &popHandle );

        while (popFileName != nullptr && popFileName[0] != '\0') {
            // Skip it if it's a directory or is the folder info
            if (filesystem->FindIsDirectory(popHandle)) {
                popFileName = filesystem->FindNext(popHandle);
                continue;
            }

            char szShortName[MAX_PATH] = { 0 };
            V_StripExtension( popFileName, szShortName, sizeof( szShortName ) );

            if (vec.Find( szShortName ) == vec.InvalidIndex()) {
                vec.AddToTail( szShortName );
            }

            popFileName = filesystem->FindNext( popHandle );
        }

        filesystem->FindClose(popHandle);
	}

	class CMod : public IMod, IFrameUpdatePostEntityThinkListener, IModCallbackListener
	{
	public:
		CMod() : IMod("MvM:Gamemode_Converter")
		{
            MOD_ADD_DETOUR_STATIC(MapEntity_ParseAllEntities, "MapEntity_ParseAllEntities");
            MOD_ADD_DETOUR_MEMBER(CNavMesh_Load, "CNavMesh::Load");
            MOD_ADD_DETOUR_STATIC(CPopulationManager_FindDefaultPopulationFileShortNames, "CPopulationManager::FindDefaultPopulationFileShortNames");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        bool generatingMesh = false;

		virtual void LevelInitPreEntity() override
		{
			generatingMesh = false;
		}

		virtual void FrameUpdatePostEntityThink() override
		{
            if (generateMesh) {
                static ConVarRef sv_cheats("sv_cheats");
                int oldval = sv_cheats.GetInt();
                sv_cheats.SetValue(1);
                engine->ServerCommand("bot -team red; bot -team blue;\n");
                engine->ServerExecute();
                sv_cheats.SetValue(oldval);
                generateMesh = false;
            }
            if (generateMeshTimer.HasStarted() && generateMeshTimer.IsElapsed()) {
                TheNavMesh->BeginGeneration(false);
                generateMeshTimer.Invalidate();
                generatingMesh = true;
            }
            if (generatingMesh) {
                static CountdownTimerInline messageTimer;
                if (messageTimer.IsElapsed()) {
                    messageTimer.Start(5.0f);
                    PrintToChatAll("Generating Navigation data...\n");
                }
            }
		}
	};
	CMod s_Mod;
	
    static ConCommand adm("sig_testnav", [](const CCommand &args) {
        generateMesh = true;
        generateMeshTimer.Start(5.0f);
    });
	
	ConVar cvar_enable("sig_mvm_gamemode_converter", "0", FCVAR_NOTIFY,
		"Mod: Convert other gamemodes into mvm gamemode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}