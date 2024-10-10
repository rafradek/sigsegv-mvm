#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"
#include "stub/econ.h"
#include "stub/extraentitydata.h"
#include "stub/lagcompensation.h"
#include "stub/server.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "mod/etc/mapentity_additions.h"
#include "util/iterate.h"
#include "mod/common/commands.h"
#include "stub/particles.h"
#include "collisionutils.h"

namespace Mod::Perf::Func_Optimize
{
    extern float coneOfAttack;
}

namespace Mod::Etc::Detector
{	

    struct ClientCVarCheckInfo
    {
        float min;
        float max;
        ConVar *cvar = nullptr;
        bool cvarFound = false;
    };

    std::unordered_map<std::string, ClientCVarCheckInfo> client_cvar_check_map { 
        {"sensitivity", {FLT_MIN,FLT_MAX}},
        {"sv_cheats", {0,0}},
        {"cl_interpolate", {1,1}},
        {"fov_desired", {20, 90}},
        {"cl_cmdrate", {10,FLT_MAX}},
        {"r_drawothermodels", {1,1}},
        {"snd_show", {0,0}},
        {"snd_visualize", {0,0}},
        {"fog_enable", {1,1}},
        {"cl_thirdperson", {0, 0}},
        {"r_portalsopenall", {0, 0}},
        {"host_timescale", {1,1}},
        {"cl_interp", {0.0f, 0.5f}},
        // cheat cvars
        {"rijin_load", {FLT_MAX, FLT_MIN}},
        {"setcvar", {FLT_MAX, FLT_MIN}},
        {"cat_load", {FLT_MAX, FLT_MIN}},
        {"crash", {FLT_MAX, FLT_MIN}},
        {"windows_speaker_config", {FLT_MAX, FLT_MIN}},
    };

    std::unordered_set<std::string> client_cvar_connect_vars {
        "cl_predictweapons",
        "cl_lagcompensation",
        "cl_predict",
        "cl_interp_ratio",
        "cl_interp",
        "cl_updaterate",
        "cl_interpolate",
    }; 

    int TimeToTicks(float time) {
        return RoundFloatToInt(time / gpGlobals->interval_per_tick);
    }

    ConVar *developer;

    ConVar cvar_bckt_tolerance("sig_etc_detector_bckt_tolerance", "0", FCVAR_NOTIFY, "Tolerance");

    int backtrack_ticks;
    float last_server_lag_time = 0.0f;
    int last_server_lag_tick = 0;

	IForward *notify_forward;

    union ForwardData
    {
        cell_t cell;
        float fl;
    };
    ConVar sig_etc_detector_print_demo("sig_etc_detector_print_demo", "0", FCVAR_NOTIFY, "Print detections to demos");

    void Notify(const std::string &shortReason, const std::string &description, const std::string &params, int tick, const std::vector<ForwardData> &data, CTFPlayer *player, IClient *client) {
        if (client == nullptr || client->IsFakeClient()) return;
        if (developer->GetBool()) {
            Msg("%s - %s - %s\n", shortReason.c_str(), description.c_str(), params.c_str());
            if (player != nullptr) {
                ClientMsg(player, "%s - %s - %s\n", shortReason.c_str(), description.c_str(), params.c_str());
            }
        }
        cell_t result = 0;
        if (tick == -1) {
            tick = gpGlobals->tickcount;
        }
        notify_forward->PushCell(player != nullptr ? player->entindex() : client->GetPlayerSlot() + 1);
        notify_forward->PushString(shortReason.c_str());
        notify_forward->PushString(description.c_str());
        notify_forward->PushString(params.c_str());
        notify_forward->PushCell(tick);
        notify_forward->PushArray(data.capacity() > 0 ? (cell_t *)data.data() : &result, data.size(), 0);
        notify_forward->PushCell(data.size());
        notify_forward->Execute(&result);
    }
    void Notify(const std::string &shortReason, const std::string &description, const std::string &params, CTFPlayer *player, int tick = -1, const std::vector<ForwardData> &data = {}) {
        Notify(shortReason, description, params, tick, data, player, player != nullptr ? sv->GetClient(player->entindex() - 1) : nullptr);
    }
    void Notify(const std::string &shortReason, const std::string &description, const std::string &params, IClient *client, int tick = -1, const std::vector<ForwardData> &data = {}) {
        Notify(shortReason, description, params, tick, data, ToTFPlayer(UTIL_PlayerByIndex(client->GetPlayerSlot()+1)), client);
    }

    bool HasThirdPersonCamera(CTFPlayer *player) {
        auto conds = player->m_Shared->GetCondData();
        return conds.InCond(TF_COND_TAUNTING) || 
            conds.InCond(TF_COND_HALLOWEEN_BOMB_HEAD) ||
            conds.InCond(TF_COND_HALLOWEEN_GIANT) || 
            conds.InCond(TF_COND_HALLOWEEN_TINY) || 
            conds.InCond(TF_COND_HALLOWEEN_GHOST_MODE) || 
            conds.InCond(TF_COND_HALLOWEEN_KART) || 
            conds.InCond(TF_COND_MELEE_ONLY) || 
            conds.InCond(TF_COND_SWIMMING_CURSE) ||
            player->GetObserverMode() != OBS_MODE_NONE ||
            player->m_nForceTauntCam ||
            player->m_hViewEntity != nullptr ||
            player->m_bIsReadyToHighFive;
    }

    float LineDistanceToPoint(const Vector &lineP1, const Vector &lineP2, const Vector &point) {

        return CrossProduct(point - lineP1, lineP2 - lineP1).Length() / (lineP2 - lineP1).Length();
    }

    struct UserCmdInfo
    {
        CUserCmd cmd;
        bool aimEnemyPlayer = false;
        bool aimAtHead = false;
        bool hadForcedAngles = false;
        bool calculatedAim = false;
        bool damagedEnemy = false;
        bool firedWeapon = false;
        float aimDistanceHead = 0.0f;
        float aimDistanceTorso = 0.0f;
        float aimDistanceCenter = 0.0f;
        float speedOurEnemy = 0.0f;
        float dotVelocityPlayerRelative = 0.0f;
        Vector enemyPos = vec3_origin;
        Vector playerPos = vec3_origin;
        int serverTick = 0;
    };

    RefCount rc_CLagCompensationManager_FinishLagCompensation;
    class DetectorPlayerModule : public EntityModule
	{
	public:
		DetectorPlayerModule(CBaseEntity *entity) : EntityModule(entity), player(ToTFPlayer(entity)) {

        }

        void StoreTickcount(int tickcount) {
            prevTickcount = tmpTickcount;
            tmpTickcount = tickcount;
        }

        int CorrectTickcount(int tickcount) {
            if (timeTeleport - gpGlobals->curtime > 2.0f) return tickcount;

            if (engine->GetPlayerNetInfo(player->entindex()) == nullptr) return tickcount;

            if (!ValidTickcount(tickcount) && !InTimeout()) {
                SetInTimeout();
            }

            if (InTimeout()) {
                return SimulateTickcount();
            }
            return tickcount;
        }

        void SetInTimeout() {
            timeTimeout = gpGlobals->curtime + 1.1f;

            int ping = RoundFloatToInt(engine->GetPlayerNetInfo(player->entindex())->GetAvgLatency(FLOW_OUTGOING) / gpGlobals->interval_per_tick);
            int tick = gpGlobals->tickcount - ping;

            diffTickcount = (prevTickcount - tick) + 1;
            
            if (diffTickcount > backtrack_ticks - 3) {
                diffTickcount = backtrack_ticks - 3;
            }
            else if (diffTickcount < ((backtrack_ticks * -1) + 3)) {
                diffTickcount = (backtrack_ticks * -1) + 3;
            }
        }

        int SimulateTickcount() {
            int ping = RoundFloatToInt(engine->GetPlayerNetInfo(player->entindex())->GetAvgLatency(FLOW_OUTGOING) / gpGlobals->interval_per_tick);
            int tick = diffTickcount + (gpGlobals->tickcount - ping) + 1;

            return ((tick > gpGlobals->tickcount) ? gpGlobals->tickcount : tick);
        }

        bool InTimeout() {
            return gpGlobals->curtime < timeTimeout;
        }

        bool ValidTickcount(int tickcount) {
            return abs((prevTickcount + 1) - tickcount) <= cvar_bckt_tolerance.GetInt();
        }

        void CalculateCmdAimTarget(CUserCmd &origCmd, UserCmdInfo &info) {
            info.calculatedAim = true;
            bool hasPotentialTargets = false;
            Vector start = player->EyePosition();
            Vector ourPos = player->GetAbsOrigin();
            Vector forward, end;
            AngleVectors(origCmd.viewangles, &forward);
            {
                float backTime = (gpGlobals->tickcount - origCmd.tick_count) * gpGlobals->interval_per_tick + player->m_fLerpTime;

                for (int i = 1; i <= gpGlobals->maxClients; i++) {
                    auto entity = static_cast<CTFPlayer *>(UTIL_EntityByIndex(i));
                    if (entity != nullptr && entity->GetTeamNumber() != player->GetTeamNumber() && entity->IsAlive()) {
                        const Vector &entityPos = entity->GetAbsOrigin();
                        const Vector &velocity = entity->GetAbsVelocity();
                        const float speed = Max(entity->MaxSpeed(), Max(fabs(velocity.x), fabs(velocity.y))* 1.4f);
                        //float closeRange = Max(entity->MaxSpeed(), entity->GetAbsVelocity().Length()) * backTime;
                        Vector range(speed, speed, Max(50.0f, fabs(velocity.z) * 2.0f));
                        range *= backTime;
                        range.x += 15.0f;
                        range.y += 15.0f;

                        auto cprop = entity->CollisionProp(); 
                        if (IsBoxIntersectingRay(entityPos+cprop->OBBMins() - range, entityPos+cprop->OBBMaxs() + range, start, forward * 8192, 0.0f)) {
                            hasPotentialTargets = true;
                            break;
                        }
                    }
                }
            }
            if (!hasPotentialTargets) return;

            CFastTimer timer;
            timer.Start();
            Mod::Perf::Func_Optimize::coneOfAttack = 0.0f;
            lagcompensation->StartLagCompensation(player, &origCmd);
            trace_t trace;
            
            VectorMA(start, 8192, forward, end);
            UTIL_TraceLine(start, end, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, player , COLLISION_GROUP_NONE, &trace);

            //TE_BeamPointsForDebug(trace.startpos, trace.endpos, 5.0f, 0, 0, 255, 255, 12.0f, player);
            //FireEyeTrace(trace, player, 8192, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, COLLISION_GROUP_PLAYER);
            {
                SCOPED_INCREMENT(rc_CLagCompensationManager_FinishLagCompensation);
                lagcompensation->FinishLagCompensation(player);
            }

            info.aimEnemyPlayer = trace.DidHit() && trace.m_pEnt != nullptr && trace.m_pEnt->IsPlayer() && trace.m_pEnt->GetTeamNumber() != player->GetTeamNumber();
            info.aimAtHead = info.aimEnemyPlayer && trace.hitgroup == HITGROUP_HEAD;
            if (info.aimEnemyPlayer) {
                auto aimPlayer = ToTFPlayer(trace.m_pEnt);
                if (aimPlayer->GetModelIndex() != boneIndexCachedModel) {
                    boneIndexCachedModel = aimPlayer->GetModelIndex();
                    boneIndexHead = aimPlayer->LookupBone("bip_head");
                    boneIndexPelvis = aimPlayer->LookupBone("bip_pelvis");
                }
                
                QAngle ang;
                Vector vec;
                info.aimDistanceCenter = LineDistanceToPoint(trace.startpos, trace.endpos, aimPlayer->WorldSpaceCenter());
                if (boneIndexHead != -1) {
                    aimPlayer->GetBonePosition(boneIndexHead, vec, ang);
                    info.aimDistanceHead = LineDistanceToPoint(trace.startpos, trace.endpos, vec);
                }
                else {
                    info.aimDistanceHead = info.aimDistanceCenter;
                }
                if (boneIndexPelvis != -1) {
                    aimPlayer->GetBonePosition(boneIndexPelvis, vec, ang);
                    info.aimDistanceTorso = LineDistanceToPoint(trace.startpos, trace.endpos, vec);
                }
                else {
                    info.aimDistanceTorso = info.aimDistanceCenter;
                }
                vec = (player->GetAbsVelocity() - aimPlayer->GetAbsVelocity());
                info.speedOurEnemy = vec.Length();
                info.dotVelocityPlayerRelative = info.speedOurEnemy == 0 ? 0 : (vec / info.speedOurEnemy).Dot((aimPlayer->GetAbsOrigin() - player->GetAbsOrigin()).Normalized());
                
                if (developer->GetInt() > 1) {
                    ClientMsg(player, "Aim dists: %f %f %f %d %f %f\n",info.aimDistanceCenter, info.aimDistanceHead, info.aimDistanceTorso, info.aimAtHead, info.speedOurEnemy, info.dotVelocityPlayerRelative);
                }
                info.enemyPos = aimPlayer->GetAbsOrigin();
            }
            timer.End();
            frameBudget -= timer.GetDuration().GetSeconds();
        }

        UserCmdInfo *PostRunCommand(CUserCmd &origCmd, CUserCmd* cmd, IMoveHelper* moveHelper) {
            //ClientMsg(player, "Srv tick: %d | Cl tick: %d | cmdnum %d\n", gpGlobals->tickcount, cmd->tick_count, cmd->command_number);
            //Msg("Srv tick: %d | Cl tick: %d | cmdnum %d\n", gpGlobals->tickcount, cmd->tick_count, cmd->command_number);
            if (!player->IsAlive()) {
                return nullptr;
            }
            auto infoptr = std::make_shared<UserCmdInfo>();
            UserCmdInfo &info = *infoptr;
            info.serverTick = gpGlobals->tickcount;
            info.playerPos = player->EyePosition();

            if (frameBudget > 0) {
                CalculateCmdAimTarget(origCmd, info);
            }
            else {
                //Msg("out of budget budget %d\n", frameBudget, cmdHistory.size());
            }

            info.cmd = origCmd;
            info.hadForcedAngles = MayHaveInvalidAngles();

            cmdHistory.push_back(infoptr);
            auto sortedInsertPos = cmdHistorySorted.end();
            for(; sortedInsertPos != cmdHistorySorted.begin(); sortedInsertPos--) {
                auto &val = *(sortedInsertPos-1);
                if (val->cmd.tick_count < origCmd.tick_count) {
                    break;
                }
            }
            cmdHistorySorted.insert(sortedInsertPos, infoptr);
            if (cmdHistory.size() > 110) {
                cmdHistorySorted.erase(std::find(cmdHistorySorted.begin(), cmdHistorySorted.end(), *cmdHistory.begin()));
                cmdHistory.erase(cmdHistory.begin());
            }
            return &info;
        }
        void AddTest(CUserCmd *cmd) {
            if (critTest) {
                if (player->GetActiveTFWeapon() != nullptr && !player->GetActiveTFWeapon()->CalcIsAttackCriticalPoll(false)) {
                    cmd->buttons &= ~IN_ATTACK;
                    player->m_nButtons &= ~IN_ATTACK;
                }
            }
            if (snapTest) {
                snapTest = false;
                ForEachTFPlayer([this, cmd](CTFPlayer *target){
                    if (target->IsAlive() && target->GetTeamNumber() != player->GetTeamNumber()) {
                        Vector delta = target->GetAbsOrigin() - player->GetAbsOrigin();
                
                        QAngle angToTarget;
                        VectorAngles(delta, angToTarget);
                        cmd->viewangles = angToTarget;
                        cmd->buttons |= IN_ATTACK;
                        player->m_nButtons |= IN_ATTACK;
                    }
                });
            }

            if (triggerTest) {
                
                Mod::Perf::Func_Optimize::coneOfAttack = 0.0f;
                lagcompensation->StartLagCompensation(player, cmd);
                trace_t trace;

                Vector start = player->EyePosition();
                Vector end, forward;
                AngleVectors(cmd->viewangles, &forward);
                VectorMA(start, 8192, forward, end);
                UTIL_TraceLine(start, end, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, player , COLLISION_GROUP_PLAYER, &trace);
                lagcompensation->FinishLagCompensation(player);
                if (trace.DidHit() && trace.m_pEnt != nullptr && trace.m_pEnt->IsPlayer() && trace.m_pEnt->GetTeamNumber() != player->GetTeamNumber()) {
                    cmd->buttons |= IN_ATTACK;
                    player->m_nButtons |= IN_ATTACK;
                    //TE_BeamPointsForDebug(trace.startpos, trace.endpos, 5.0f, 255, 0, 0, 255, 3.0f, player);
                }
            }

            if (lockTest) {
                Mod::Perf::Func_Optimize::coneOfAttack = 1.0f;
                lagcompensation->StartLagCompensation(player, cmd);
                ForEachTFPlayer([this, cmd](CTFPlayer *target){
                        if (target->IsAlive() && target->GetTeamNumber() != player->GetTeamNumber()) {
                            Vector pos;
                            QAngle angToTarget;
                            target->GetBonePosition(target->LookupBone("bip_head"), pos, angToTarget);
                            Vector delta = pos - player->EyePosition();
                            VectorAngles(delta, angToTarget);
                            cmd->viewangles = angToTarget;
                            return false;
                        }
                        return true;
                    }
                );
                lagcompensation->FinishLagCompensation(player);
            }
        }

        void ValidateCmd(CUserCmd* cmd) {
            if (!player->IsAlive()) return;

            if (baselineCmdTick == 0 || baselineCmdNum == 0) {
                baselineCmdTick = cmd->tick_count;
                baselineCmdNum = cmd->command_number;
                highestCmdTick = cmd->tick_count;
                highestCmdNum = cmd->command_number;
                return;
            }
            //ClientMsg(player,"%d %d (%d %d)\n", cmd->tick_count, cmd->command_number, cmd->tick_count - baselineCmdTick, cmd->command_number - baselineCmdNum);

            highestCmdTick = Max(highestCmdTick, cmd->tick_count);
            highestCmdNum = Max(highestCmdNum, cmd->command_number);

            if (cmd->tick_count - baselineCmdTick != cmd->command_number - baselineCmdNum) {
                Notify("Invalid usercmd", "Usercmd command number mismatch", std::format("command number delta={},tick count delta={}", cmd->command_number - baselineCmdNum, cmd->tick_count - baselineCmdTick), player);
                baselineCmdTick = cmd->tick_count;
                baselineCmdNum = cmd->command_number;
            }

            if (cmd->tick_count > gpGlobals->tickcount) {
                Notify("Invalid usercmd", "Usercmd tick count higher than server tick count", std::format("usercmd tick count={},server tick count={}", cmd->tick_count, gpGlobals->tickcount), player);
            }
        }

        bool MayHaveInvalidAngles() {
            return HasThirdPersonCamera(player) || gpGlobals->curtime - this->timeTeleport < 2.0f;
        }

        void CheckAngles(CUserCmd* cmd) {
            if (MayHaveInvalidAngles() || !player->IsAlive()) return;

            if (abs(cmd->viewangles[PITCH]) > 89.0001f || abs(cmd->viewangles[ROLL]) > 50.0001f) {
                Notify("Invalid angles", "Player had angles in invalid range",std::format("angles={} {} {}", cmd->viewangles.x, cmd->viewangles.y, cmd->viewangles.z), player);
            }
        }
        
        void AnalizeCmds() {
            constexpr int frameCheckCount = 16;
            constexpr int cmdCheckDelay = 80;

            if (cmdHistory.size() < frameCheckCount) return;

            if (gpGlobals->tickcount - cmdHistory[0]->serverTick < cmdCheckDelay) return;

            while (gpGlobals->tickcount - cmdHistory[0]->serverTick >= cmdCheckDelay && cmdHistory.size() >= frameCheckCount) {
                auto &cmd0 = *cmdHistorySorted[0];
                auto &cmd1 = *cmdHistorySorted[1];
                auto &cmd2 = *cmdHistorySorted[2];
                cmdHistory.erase(std::find(cmdHistory.begin(), cmdHistory.end(), cmdHistorySorted[0]));

                bool skip = false;
                for (int i = 0; i < frameCheckCount; i++) {
                    if (cmdHistorySorted[i]->hadForcedAngles) {
                        skip = true;
                        break;
                    }
                }
                if (skip) {
                    cmdHistorySorted.erase(cmdHistorySorted.begin());
                    return;
                }

                int measuredClientCmds = 0;
                int measuredCommandNumbers = 0;

                int unorder = 0;
                int latencyTotal = 0;
                int latencyMax = 0;
                std::shared_ptr<UserCmdInfo> prevptr;
                for (auto &info : cmdHistorySorted) {
                    if (prevptr) {
                        if (info->cmd.command_number != prevptr->cmd.command_number + 1 || info->cmd.tick_count != prevptr->cmd.tick_count + 1) {
                            unorder++;
                        }
                    }
                    latencyTotal += info->serverTick - info->cmd.tick_count;
                    latencyMax = Max(latencyMax, Max(info->serverTick - info->cmd.tick_count, 0));
                    prevptr = info;
                }
                int latencyAvg = latencyTotal/cmdHistorySorted.size();


                if (cmd0.cmd.command_number > cmd1.cmd.command_number && cmd0.cmd.tick_count < cmd1.cmd.tick_count) {
                    Notify("Invalid usercmd", "Usercmd command number from previous tick higher than a command number from next tick", std::format("previous command number={},previous tick={},next command number={},next tick={},cmd unorder={},cmd latency avg/max={}/{}", cmd0.cmd.command_number, cmd0.cmd.tick_count, cmd1.cmd.command_number, cmd1.cmd.tick_count, unorder, latencyAvg, latencyMax), player, cmd0.serverTick);
                }

                if (cmd0.cmd.command_number == cmd1.cmd.command_number && cmd0.cmd.tick_count == cmd1.cmd.tick_count && cmd0.cmd.viewangles != cmd1.cmd.viewangles && cmd0.cmd.buttons != cmd1.cmd.buttons) {
                    Notify("Invalid usercmd", "Usercmd duplicated command number and tick count, but with different data", std::format("command number={},tick count={},cmd unorder={},cmd latency avg/max={}/{}", cmd0.cmd.command_number, cmd0.cmd.tick_count, unorder, latencyAvg, latencyMax), player, cmd0.serverTick);
                }

                bool displayBoxHead = false;
                UserCmdInfo *traceCmd = nullptr;
                Vector boxColor = Vector(255,255,255);
                Vector traceColor = Vector(255,255,255);

                bool triggerBotStartCheck = false;
                bool triggerBotHeadStartCheck = false;
                bool triggerBotEndCheck = false;
                bool triggerBotHeadEndCheck = false;
                if (!cmd0.aimAtHead && cmd1.aimAtHead && !(cmd0.cmd.buttons & IN_ATTACK) && (cmd1.cmd.buttons & IN_ATTACK)) {
                    triggerBotHeadStartCheck = true;
                }
                else if (!cmd0.aimEnemyPlayer && cmd1.aimEnemyPlayer && !(cmd0.cmd.buttons & IN_ATTACK) && (cmd1.cmd.buttons & IN_ATTACK)) {
                    triggerBotStartCheck = true;
                }

                if (cmd1.aimAtHead && !cmd2.aimAtHead && (cmd1.cmd.buttons & IN_ATTACK) && !(cmd2.cmd.buttons & IN_ATTACK)) {
                    triggerBotHeadEndCheck = true;
                }
                else if (cmd1.aimEnemyPlayer && !cmd2.aimEnemyPlayer && (cmd1.cmd.buttons & IN_ATTACK) && !(cmd2.cmd.buttons & IN_ATTACK)) {
                    triggerBotEndCheck = true;
                }
                if ((triggerBotStartCheck || triggerBotHeadStartCheck) && (triggerBotEndCheck || triggerBotHeadEndCheck)) {
                    displayBoxHead = triggerBotHeadStartCheck;
                    boxColor = traceColor = Vector(255, 0, 0);
                    traceCmd = &cmd1;
                    Notify("Triggerbot start+end", "Player pressed attack button same tick as they aimed at enemy target, then unpressed it next tick as target was dead", std::format("aimed at head={},fired={},damaged={},cmd unorder={},cmd latency avg/max/shot={}/{}/{}", triggerBotHeadStartCheck, cmd1.firedWeapon, cmd1.damagedEnemy, unorder, latencyAvg, latencyMax, Max(cmd1.serverTick - cmd1.cmd.tick_count,0)), player, cmd1.serverTick);
                }
                else if ((triggerBotStartCheck || triggerBotHeadStartCheck) && !(triggerBotEndCheck || triggerBotHeadEndCheck)) {
                    displayBoxHead = triggerBotHeadStartCheck;
                    boxColor = traceColor = Vector(255, 128, 0);
                    traceCmd = &cmd1;
                    Notify("Triggerbot start", "Player pressed attack button same tick as they aimed at enemy target", std::format("aimed at head={},fired={},damaged={},cmd unorder={},cmd latency avg/max/shot={}/{}/{}", triggerBotHeadStartCheck, cmd1.firedWeapon, cmd1.damagedEnemy, unorder, latencyAvg, latencyMax, Max(cmd1.serverTick - cmd1.cmd.tick_count,0)), player, cmd1.serverTick);
                }
                else if (!(triggerBotStartCheck || triggerBotHeadStartCheck) && (triggerBotEndCheck || triggerBotHeadEndCheck)) {
                    displayBoxHead = triggerBotHeadEndCheck;
                    boxColor = traceColor = Vector(255, 192, 0);
                    traceCmd = &cmd1;
                    Notify("Triggerbot end", "Player unpressed attack button same tick as they stopped aiming at enemy target or the target was dead", std::format("aimed at head={},fired={},damaged={},cmd unorder={},cmd latency avg/max/shot={}/{}/{}", triggerBotHeadEndCheck, cmd1.firedWeapon, cmd1.damagedEnemy, unorder, latencyAvg, latencyMax, Max(cmd1.serverTick - cmd1.cmd.tick_count,0)), player, cmd1.serverTick);
                }
                auto ang0 = cmd0.cmd.viewangles;
                auto ang1 = cmd1.cmd.viewangles;
                auto ang2 = cmd2.cmd.viewangles;
                float delta01 = abs(AngleDiff(ang0.x, ang1.x)) + abs(AngleDiff(ang0.y, ang1.y));
                float delta12 = abs(AngleDiff(ang1.x, ang2.x)) + abs(AngleDiff(ang1.y, ang2.y));
                float delta02 = abs(AngleDiff(ang0.x, ang2.x)) + abs(AngleDiff(ang0.y, ang2.y));

                if (!cmd0.aimEnemyPlayer && cmd1.aimEnemyPlayer && !cmd2.aimEnemyPlayer && !(cmd0.cmd.buttons & IN_ATTACK) && (cmd1.cmd.buttons & IN_ATTACK)) {
                    if (delta01 > 10.0f && delta12 > 10.0f && delta02 < 2.0f) {
                        displayBoxHead = cmd1.aimAtHead;
                        boxColor = traceColor = Vector(255, 255, 0);
                        traceCmd = &cmd1;
                        Notify("Silent aim", "Player snapped aim in one tick to an enemy player, attacked, then reverted back to previous aim angles", std::format("aimed at head={},fired={},damaged={},aim delta 01={:.3},delta 12={:.3},delta 02={:.3},cmd unorder={},cmd latency avg/max/shot={}/{}/{}", cmd1.aimAtHead, cmd1.firedWeapon, cmd1.damagedEnemy, delta01, delta12, delta02, unorder, latencyAvg, latencyMax, Max(cmd1.serverTick - cmd1.cmd.tick_count,0)), player, cmd1.serverTick);
                    }
                }
                //if (developer->GetBool()) {
                //    ClientMsg(player, "buttons %d %d %d %d\n", cmd0.cmd.buttons, cmd1.cmd.buttons, cmd0.aimEnemyPlayer, cmd1.aimEnemyPlayer);
                //}
                int framesAiming = 0;
                float minDistanceDiffTotal = 0;
                bool aimedAtHead = false;
                bool aimedAtTorso = false;
                float angMoveTotalNonAimed = 0;
                int nonAimedTickCount = 0;
                Vector aimDeltaVecNonAimedTotal = vec3_origin;
                Vector aimDeltaVecAimed = vec3_origin;
                UserCmdInfo *aimPlayerCmd = nullptr; 
                float maxAngDeltaWhenAimingAtEnemy = 0;

                float totalVelocityPlayerRelativeToEnemyDot = 0;

                for (int i = 0; i < frameCheckCount - 1; i++) {
                    auto &cmd0 = *cmdHistorySorted[i];
                    auto &cmd1 = *cmdHistorySorted[i+1];
                    float angDelta = abs(AngleDiff(cmd0.cmd.viewangles.x, cmd1.cmd.viewangles.x)) + abs(AngleDiff(cmd0.cmd.viewangles.y, cmd1.cmd.viewangles.y));
                    if (cmd1.aimEnemyPlayer && !cmd0.aimEnemyPlayer) {
                        if (angDelta > maxAngDeltaWhenAimingAtEnemy) {
                            maxAngDeltaWhenAimingAtEnemy = angDelta;
                            aimDeltaVecAimed = Vector(AngleDiff(cmd0.cmd.viewangles.x, cmd1.cmd.viewangles.x), AngleDiff(cmd0.cmd.viewangles.y, cmd1.cmd.viewangles.y), 0);
                        }
                    }
                    else {
                        angMoveTotalNonAimed += angDelta;
                        aimDeltaVecNonAimedTotal += Vector(AngleDiff(cmd0.cmd.viewangles.x, cmd1.cmd.viewangles.x), AngleDiff(cmd0.cmd.viewangles.y, cmd1.cmd.viewangles.y), 0);
                        nonAimedTickCount++;
                    }
                    if (cmd0.cmd.viewangles != cmd1.cmd.viewangles && cmd1.aimEnemyPlayer && cmd1.speedOurEnemy > 50.0f && abs(cmd1.dotVelocityPlayerRelative) < 0.99f) {
                        framesAiming++;
                        totalVelocityPlayerRelativeToEnemyDot += abs(cmd1.dotVelocityPlayerRelative);
                        float minDist0 = Min(cmd0.aimDistanceHead, Min(cmd0.aimDistanceTorso, cmd0.aimDistanceCenter));
                        float minDist1 = Min(cmd1.aimDistanceHead, Min(cmd1.aimDistanceTorso, cmd1.aimDistanceCenter));
                        if (cmd1.aimAtHead) {
                            aimedAtHead = true;
                        }
                        else {
                            aimedAtTorso = true;
                        }
                        aimPlayerCmd = &cmd1;
                        minDistanceDiffTotal += abs(minDist0 - minDist1);
                    }
                }
                if (developer->GetInt() > 1){
                    Vector aimDeltaVec01 = Vector(AngleDiff(cmd0.cmd.viewangles.x, cmd1.cmd.viewangles.x), AngleDiff(cmd0.cmd.viewangles.y, cmd1.cmd.viewangles.y), 0).Normalized();
                    Vector aimDeltaVec12 = Vector(AngleDiff(cmd1.cmd.viewangles.x, cmd2.cmd.viewangles.x), AngleDiff(cmd1.cmd.viewangles.y, cmd2.cmd.viewangles.y), 0).Normalized();
                    //ClientMsg(player, "aim delta dot %f %f %f %f %d %d %d %d\n", RAD2DEG(acos(DotProductAbs(aimDeltaVec01, aimDeltaVec12))), delta01, delta12, delta02, cmd0.aimEnemyPlayer, cmd1.aimEnemyPlayer, cmd2.aimEnemyPlayer, cmd1.cmd.buttons & IN_ATTACK);
                }
                
                if (!cmd0.aimEnemyPlayer && !cmd1.aimEnemyPlayer && cmd2.aimEnemyPlayer) {
                    // Look at sharp turns with fast speed
                    if (delta12 > 10.0f) {
                        Vector aimDeltaVec01 = Vector(AngleDiff(cmd0.cmd.viewangles.x, cmd1.cmd.viewangles.x), AngleDiff(cmd0.cmd.viewangles.y, cmd1.cmd.viewangles.y), 0).Normalized();
                        Vector aimDeltaVec12 = Vector(AngleDiff(cmd1.cmd.viewangles.x, cmd2.cmd.viewangles.x), AngleDiff(cmd1.cmd.viewangles.y, cmd2.cmd.viewangles.y), 0).Normalized();
                        if (delta01 < 1.0f || delta12 / delta01 > 10 || DotProductAbs(aimDeltaVec01, aimDeltaVec12) < 0.7) { // 45 degress
                            displayBoxHead = cmd2.aimAtHead;
                            boxColor = traceColor = Vector(0, 0, 255);
                            traceCmd = &cmd2;
                            Notify("Aim snap", "Player snapped aim to an enemy player with many times above average angular velocity", std::format("aimed at head={},fired={},damaged={},avg delta={:.3},aim snap delta={:.3},cmd unorder={},cmd latency avg/max/shot={}/{}/{}", cmd2.firedWeapon, cmd2.damagedEnemy, cmd2.aimAtHead, maxAngDeltaWhenAimingAtEnemy / (angMoveTotalNonAimed / (frameCheckCount - 1)), angMoveTotalNonAimed / (frameCheckCount - 1), unorder, latencyAvg, latencyMax, (float)Max(cmd2.serverTick - cmd2.cmd.tick_count,0)), player, cmd2.serverTick);
                        }
                    }
                }
                aimDeltaVecAimed.NormalizeInPlace();
                aimDeltaVecNonAimedTotal.NormalizeInPlace();
                //ClientMsg(player, "%f %f | %f %f | %f\n", aimDeltaVecAimed.x, aimDeltaVecAimed.y, aimDeltaVecNonAimedTotal.x, aimDeltaVecNonAimedTotal.y, RAD2DEG(acos(DotProductAbs(aimDeltaVecAimed, aimDeltaVecNonAimedTotal))));
                //ClientMsg(player, "aim angle delta: %f, avg: %f, cur %f, dot %f | avg aim dist: %f %d | \n", maxAngDeltaWhenAimingAtEnemy, angMoveTotalNonAimed / (frameCheckCount - 1), delta01, DotProduct(aimDeltaVecAimed.Normalized(), aimDeltaVecNonAimedTotal.Normalized()), minDistanceDiffTotal / framesAiming, framesAiming);
                if (framesAiming > frameCheckCount / 4) {
                    //ClientMsg(player, "avg aim %f %d\n", minDistanceDiffTotal / framesAiming, framesAiming);
                    if (minDistanceDiffTotal / framesAiming < 0.045f) {
                        if (aimPlayerCmd != nullptr) {
                            displayBoxHead = aimedAtHead;
                            boxColor = traceColor = Vector(0, 255, 255);
                            traceCmd = aimPlayerCmd;
                        }
                        Notify("Aim lock", "Player locked aim to head or torso", std::format("aimed at head={},aimed at torso={},avg distance to head/torso bone={:.3},angle to enemy movement={:.3},cmd unorder={},cmd latency avg/max={}/{}", aimedAtHead, aimedAtTorso, minDistanceDiffTotal / framesAiming, RAD2DEG(acos(totalVelocityPlayerRelativeToEnemyDot / framesAiming)), unorder, latencyAvg, latencyMax), player, cmd0.serverTick);
                    }
                }

                if (traceCmd != nullptr) {
                    CBasePlayer *sourceTVClient = nullptr;
                    ForEachPlayer([&](CBasePlayer *pl) {
                        if (pl->IsHLTV()) {
                            sourceTVClient = pl;
                        }
                    });
                    if (sourceTVClient != nullptr && sig_etc_detector_print_demo.GetBool()) {
                        lagcompensation->StartLagCompensation(player, &traceCmd->cmd);
                        trace_t trace;
                        Vector start = traceCmd->playerPos;
                        Vector end, forward;
                        AngleVectors(traceCmd->cmd.viewangles, &forward);
                        VectorMA(start, 8192, forward, end);
                        UTIL_TraceLine(start, end, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, player , COLLISION_GROUP_NONE, &trace);
                        
                        TE_BeamPointsForDebug(traceCmd->playerPos, end, 5.0f, traceColor.x, traceColor.y, traceColor.z, 255, displayBoxHead ? 6 : 3, sourceTVClient);
                        if (traceCmd->enemyPos != vec3_origin) {
                            TE_BBoxForDebug(traceCmd->enemyPos + VEC_HULL_MIN, traceCmd->enemyPos + VEC_HULL_MAX, 5.0f, boxColor.x, boxColor.y, boxColor.z, 255, displayBoxHead ? 6 : 3, sourceTVClient);
                        }
                        lagcompensation->FinishLagCompensation(player);
                    }
                }
                cmdHistorySorted.erase(cmdHistorySorted.begin());
            }
        }

        void Reset() {
            cmdHistory.clear();
            cmdHistorySorted.clear();
            baselineCmdTick = 0;
            baselineCmdNum = 0;
            highestCmdTick = 0;
            highestCmdNum = 0;
        }

        void RefreshFrameBudget() {
            frameBudget = 0.01f;
        }
		int prevTickcount = 0;
        int diffTickcount = 0;
        int tmpTickcount = 0;
        float timeTimeout = 0.0f;
        float timeTeleport = 0.0f;
        float spawnTime = 0.0f;
        int lastShootTick = 0;
        int lastDamagedPlayerTick = 0;
        string_t lastDamagedPlayerWeaponName;
        float tauntTime = 0.0f;
        
        std::unordered_map<std::string, float> connectClientCvarPrevValues;
        
        CTFPlayer *player;

        std::vector<std::shared_ptr<UserCmdInfo>> cmdHistory;
        std::vector<std::shared_ptr<UserCmdInfo>> cmdHistorySorted;

        int baselineCmdTick = 0;
        int baselineCmdNum = 0;
        int highestCmdTick = 0;
        int highestCmdNum = 0;

        int prevUpdateRate = 0;

        bool snapTest = false;
        bool triggerTest = false;
        bool lockTest = false;
        bool critTest = false;

        int boneIndexCachedModel = 0;
        int boneIndexHead = 0;
        int boneIndexPelvis = 0;
        float frameBudget = 0.1f;
	}; 

    // Stop expensive traces that are unnecesary when doing lag compensation here

	DETOUR_DECL_STATIC(void, UTIL_TraceEntity, CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *pIgnore, int nCollisionGroup, trace_t *ptr)
	{
        if (rc_CLagCompensationManager_FinishLagCompensation) {
            ptr->startsolid = false;
            ptr->allsolid = false;
            ptr->endpos = vecAbsStart;
            return;
        }
		DETOUR_STATIC_CALL(pEntity, vecAbsStart, vecAbsEnd, mask, pIgnore, nCollisionGroup, ptr);
	}

	DETOUR_DECL_STATIC(void, UTIL_SetOrigin, CBaseEntity *pEntity, const Vector &origin, bool touchTriggers)
	{
        if (rc_CLagCompensationManager_FinishLagCompensation) {
            touchTriggers = false;
        }
		DETOUR_STATIC_CALL(pEntity, origin, touchTriggers);
	}



	DETOUR_DECL_MEMBER(void, CTFPlayer_FireBullet, CTFWeaponBase *weapon, FireBulletsInfo_t& info, bool bDoEffects, int nDamageType, int nCustomDamageType)
	{
		DETOUR_MEMBER_CALL(weapon, info, bDoEffects, nDamageType, nCustomDamageType);

        //TE_BeamPointsForDebug(info.m_vecSrc, info.m_vecSrc + info.m_vecDirShooting.Normalized() * 8192, 5.0f, 0, 255, 0, 255, 3, reinterpret_cast<CTFPlayer *>(this));
	}

    CUserCmd *currCmd = nullptr;
    CUserCmd *currCmdPre = nullptr;
    IMoveHelper *currMoveHelper = nullptr;
    DetectorPlayerModule *currMod = nullptr;
    // Detecting is done in postthink as it is the time when the player is already moved by client cmd and just about to shoot
	DETOUR_DECL_MEMBER(void, CTFPlayer_PostThink)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
        UserCmdInfo *info = nullptr;
        if (currCmd != nullptr) {
            auto mod = currMod;
            mod->AddTest(currCmdPre);
            info = mod->PostRunCommand(*currCmdPre, currCmd, currMoveHelper);
        }
        DETOUR_MEMBER_CALL();
        if (info != nullptr) {
            info->damagedEnemy = currMod->lastDamagedPlayerTick == gpGlobals->tickcount;
            info->firedWeapon = currMod->lastShootTick == gpGlobals->tickcount;
        }
    }

	DETOUR_DECL_MEMBER(void, CTFPlayer_PlayerRunCommand, CUserCmd* cmd, IMoveHelper* moveHelper)
	{
		CTFPlayer* player = reinterpret_cast<CTFPlayer*>(this);
		if (!player->IsRealPlayer()) {
            DETOUR_MEMBER_CALL(cmd, moveHelper);
            return;
        }
        
        auto mod = player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
        if (gpGlobals->curtime - last_server_lag_time < 1.0f) {
            mod->Reset();
            DETOUR_MEMBER_CALL(cmd, moveHelper);
            return;
        }
        CUserCmd cmdPre = *cmd;
        mod->ValidateCmd(cmd);
        if (player->pl->fixangle != FIXANGLE_NONE || player->GetMoveParent() != nullptr) {
            mod->timeTeleport = gpGlobals->curtime;
        }
        mod->CheckAngles(cmd);

        mod->StoreTickcount(cmd->tick_count);

        cmd->tick_count = mod->CorrectTickcount(cmd->tick_count);

        currCmd = cmd;
        currCmdPre = &cmdPre;
        currMoveHelper = moveHelper;
        currMod = mod;
        DETOUR_MEMBER_CALL(cmd, moveHelper);
        currCmd = nullptr;
        currCmdPre = nullptr;

	}


	DETOUR_DECL_MEMBER(void, CBasePlayer_PlayerRunCommand, CUserCmd* cmd, IMoveHelper* moveHelper)
	{

    }

	DETOUR_DECL_MEMBER(void, CBaseEntity_Teleport, const Vector *pos, const QAngle *angle, const Vector *vel)
	{
        auto entity = reinterpret_cast<CBaseEntity *>(this);
        if (entity->IsPlayer() && !(entity->m_fFlags & FL_FAKECLIENT)) {
            auto mod = entity->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
            mod->timeTeleport = gpGlobals->curtime;
        }
		DETOUR_MEMBER_CALL(pos, angle, vel);
	}

    DETOUR_DECL_MEMBER(void, CTFPlayer_Spawn)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this); 
		
		DETOUR_MEMBER_CALL();
        if (!player->IsRealPlayer()) return;
        auto mod = player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
        mod->timeTeleport = gpGlobals->curtime;
        mod->spawnTime = gpGlobals->curtime;
        mod->Reset();
    }

    std::unordered_map<uint, int> ip_connect_counter;
    
    DETOUR_DECL_MEMBER(bool, CBaseServer_CheckProtocol, netadr_t &adr, int nProtocol, int clientChallenge)
	{
		auto *server = reinterpret_cast<CBaseServer *>(this); 
		
        int counter = ip_connect_counter[adr.GetIPHostByteOrder()]++;

        if (counter > 6) {
            server->RejectConnection(adr, clientChallenge, "Too many reconnects. Please connect again in a minute");
            return false;
        }
		return DETOUR_MEMBER_CALL(adr, nProtocol, clientChallenge);
    }

	StaticFuncThunk<int, IClient *, const char *, bool> ft_SendCvarValueQueryToClient("SendCvarValueQueryToClient");
	THINK_FUNC_DECL(CheckClientCVars) {
        
        //TIME_SCOPE2(CheckClientCVars);
        auto player = reinterpret_cast<CTFPlayer *>(this);
        auto client = sv->GetClient(player->entindex() - 1);
        for (auto &entry : client_cvar_check_map) {
            ft_SendCvarValueQueryToClient(client, entry.first.c_str(), true);
        }
        this->SetNextThink(gpGlobals->curtime + RandomFloat(20, 40), "CheckClientCVars");
	};
    
    DETOUR_DECL_MEMBER(void, CServerGameClients_ClientPutInServer, edict_t *edict, const char *playername)
	{
        DETOUR_MEMBER_CALL(edict, playername);
		auto player = ((CTFPlayer*) edict->GetUnknown());
        if (player->IsRealPlayer()) {
            THINK_FUNC_SET(player, CheckClientCVars, gpGlobals->curtime + RandomFloat(30, 45));
        }
    }

    DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage_Alive, const CTakeDamageInfo &info)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
        auto attacker = ToTFPlayer(info.GetAttacker());
        if (info.GetWeapon() == nullptr || attacker == player || attacker == nullptr || !attacker->IsRealPlayer()) return DETOUR_MEMBER_CALL(info);

        if (!(info.GetDamageType() & DMG_IGNITE) && info.GetDamageCustom() != TF_DMG_CUSTOM_BURNING && info.GetDamageCustom() != TF_DMG_CUSTOM_BURNING_FLARE) {

            auto mod = attacker->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
            mod->lastDamagedPlayerTick = gpGlobals->tickcount;
            mod->lastDamagedPlayerWeaponName = info.GetWeapon()->GetClassnameString();
        }

		return DETOUR_MEMBER_CALL(info);
	}
    DETOUR_DECL_STATIC(void, TE_FireBullets, int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, 
					 int iWeaponID, int	iMode, int iSeed, float flSpread, bool bCritical)
	{
        auto player = UTIL_PlayerByIndex(iPlayerIndex);
        if (player != nullptr && player->IsRealPlayer()) {
            auto mod = player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
            mod->lastShootTick = gpGlobals->tickcount;
        }
        DETOUR_STATIC_CALL(iPlayerIndex, vOrigin, vAngles, iWeaponID, iMode, iSeed, flSpread, bCritical);
	}

    DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnConditionAdded, ETFCond cond)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
        auto player = shared->GetOuter();
		DETOUR_MEMBER_CALL(cond);
        if (!player->IsRealPlayer()) return;
        auto mod = player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");

		if (cond == TF_COND_TAUNTING) {
			mod->tauntTime = gpGlobals->curtime;
			return;
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnConditionRemoved, ETFCond cond)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);

		DETOUR_MEMBER_CALL(cond);
	}

    DETOUR_DECL_MEMBER(void, CTFGameRules_FireGameEvent, IGameEvent *event)
	{
        if (event->GetName() == nullptr) { 
            DETOUR_MEMBER_CALL(event); 
            return;
        }

        DETOUR_MEMBER_CALL(event); 

        if (strcmp(event->GetName(), "achievement_earned") == 0) {
            int index = event->GetInt("player");

            int achieveId = event->GetInt("achievement");
            if (achieveId > 2805 || achieveId < 127) {
                Notify("Invalid achievement ID", "Player achieved invalid achievement", std::format("ID={}", achieveId), sv->GetClient(index - 1));
            }
        }
    }

	DETOUR_DECL_MEMBER(void, CServerPlugin_OnQueryCvarValueFinished, QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue)
	{
		DETOUR_MEMBER_CALL(iCookie, pPlayerEntity, eStatus, pCvarName, pCvarValue);

		if (pCvarName == nullptr) return;

        auto find = client_cvar_check_map.find(pCvarName);
        if (find != client_cvar_check_map.end()) {
            auto &bounds = find->second;
            if (!bounds.cvarFound) {
                bounds.cvar = icvar->FindVar(pCvarName);
                bounds.cvarFound = true;
            }
            auto player = ToTFPlayer(GetContainingEntity(pPlayerEntity));
            if (player == nullptr) return;
            if (bounds.min > bounds.max) {
                if (eStatus == eQueryCvarValueStatus_ValueIntact) {
                    Notify("Client CVar check - cheat cvar detected", "Found cvar value for cheat cvar", std::format("cvar={}", pCvarName), player);
                }
                return;
            }
            else if ((eStatus != eQueryCvarValueStatus_ValueIntact || pCvarValue == nullptr)) {
                Notify("Client CVar check - failure", "Cannot check value of cvar", std::format("cvar={}", pCvarName), player);
            }
            else {
                float cvarValue = strtof(pCvarValue, nullptr);
                if ((cvarValue < bounds.min || cvarValue > bounds.max) && (bounds.cvar == nullptr || bounds.cvar->GetFloat() != cvarValue)) {
                    Notify("Client CVar check - oob", "Client cvar is out of bounds", std::format("cvar={},value={},bounds={}-{},reported by=query", pCvarName, cvarValue, bounds.min, bounds.max), player);
                }
            }
        }
    }

    DETOUR_DECL_STATIC(void, Host_Say, edict_t *edict, const CCommand& args, bool team )
	{
		auto *player = (CTFPlayer*) (GetContainingEntity(edict));
		if (player != nullptr && !player->IsRealPlayer()) {
            DETOUR_STATIC_CALL(edict, args, team);
            return;
        }
        if (player != nullptr) {
			const char *p = args.ArgS();
            if(strchr(p,'\n') != nullptr || strchr(p,'\r') != nullptr) {
                Notify("Newline in chat", "Found a newline / carriage return character in chat message", std::format("char={} {}",strchr(p,'\n') != nullptr ? "newline" : "", strchr(p,'\r') != nullptr ? "carriage return" : ""), player);
                return;
            }
		}
		DETOUR_STATIC_CALL(edict, args, team);
	}

	DETOUR_DECL_MEMBER(void, CServerPlugin_ClientSettingsChanged, edict_t *edict)
	{
        auto client = (CGameClient *)sv->GetClient(edict->m_EdictIndex - 1);
        if (!client->IsFakeClient() && client->m_ConVars != nullptr) {
            //TIME_SCOPE2(ClientSettingsChanged);
            int cl_cmdrate_v = client->m_ConVars->GetInt("cl_cmdrate");
            int cl_updaterate_v = client->m_ConVars->GetInt("cl_updaterate");
            int rate_v = client->m_ConVars->GetInt("rate");
            float cl_interp_v = client->m_ConVars->GetFloat("cl_interp");
            float cl_interp_ratio_v = client->m_ConVars->GetFloat("cl_interp_ratio");

            FOR_EACH_SUBKEY(client->m_ConVars, subkey) {
                auto find = client_cvar_check_map.find(subkey->GetName());
                if (find != client_cvar_check_map.end()) {
                    float value = subkey->GetFloat();
                    auto &bounds = find->second;
                    if (value < bounds.min || value > bounds.max) {
                        Notify("Client CVar check - oob", "Client cvar is out of bounds", std::format("cvar={},value={},bounds={}-{},reported by=kv", subkey->GetName(), value, bounds.min, bounds.max), client);
                    }
                }
            }
            static ConVarRef sv_mincmdrate("sv_mincmdrate");
            static ConVarRef sv_maxcmdrate("sv_maxcmdrate");
            static ConVarRef sv_minupdaterate("sv_minupdaterate");
            static ConVarRef sv_maxupdaterate("sv_maxupdaterate");
            static ConVarRef sv_minrate("sv_minrate");
            static ConVarRef sv_maxrate("sv_maxrate");
            static ConVarRef sv_client_min_interp_ratio("sv_client_min_interp_ratio");
            static ConVarRef sv_client_max_interp_ratio("sv_client_max_interp_ratio");

            int clamped_cl_cmdrate = Clamp(cl_cmdrate_v, sv_mincmdrate.GetInt(), sv_maxcmdrate.GetInt());
            int clamped_cl_updaterate = Clamp(cl_updaterate_v, sv_minupdaterate.GetInt(), sv_maxupdaterate.GetInt());
            int clamped_rate = Clamp(rate_v, Max(1000, sv_minrate.GetInt()),  Min(1024 * 1024, sv_maxrate.GetInt()));
            float clamped_cl_interp = Clamp(cl_interp_v, sv_client_min_interp_ratio.GetFloat() / (float)clamped_cl_updaterate, 0.5f);
            float clamped_cl_interp_ratio = Clamp(cl_interp_ratio_v, sv_client_min_interp_ratio.GetFloat(), sv_client_max_interp_ratio.GetFloat());

            client->m_ConVars->SetInt("cl_cmdrate", clamped_cl_updaterate);
            client->m_ConVars->SetInt("cl_updaterate", clamped_cl_updaterate);
            client->m_ConVars->SetInt("rate", clamped_rate);
            client->m_ConVars->SetFloat("cl_interp", clamped_cl_interp);
            client->m_ConVars->SetFloat("cl_interp_ratio", clamped_cl_interp_ratio);

            auto player = (CTFPlayer *)GetContainingEntity(edict);
            if (player != nullptr) {
                bool canChangeNetworkCvar = TFGameRules()->IsConnectedUserInfoChangeAllowed(player);
                auto mod = player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
                if (!canChangeNetworkCvar) {
                    for (auto &cvarName : client_cvar_connect_vars) {
                        auto find = mod->connectClientCvarPrevValues.find(cvarName);
                        if (find != mod->connectClientCvarPrevValues.end() && abs(find->second - client->m_ConVars->GetFloat(cvarName.c_str())) > 0.001) {
                            Notify("Client CVar check - change while alive", "Restricted client cvar changed while player was alive", std::format("cvar={},from={},to={}", cvarName, find->second, client->m_ConVars->GetFloat(cvarName.c_str())), client);
                        }
                    }
                }
                for (auto &cvarName : client_cvar_connect_vars) {
                    mod->connectClientCvarPrevValues[cvarName] = client->m_ConVars->GetFloat(cvarName.c_str());
                }
                if (mod->prevUpdateRate != clamped_cl_updaterate) {
                    mod->Reset();
                    mod->prevUpdateRate = clamped_cl_updaterate;
                }
            }
        }
		DETOUR_MEMBER_CALL(edict);
    }

	DETOUR_DECL_MEMBER(void, CBaseClient_SetName, const char *newName)
	{
        auto client = reinterpret_cast<CBaseClient *>(this);
        if (!client->IsFakeClient()) {
            std::string newNameFixed = newName;
            boost::algorithm::erase_all(newNameFixed, "\n");
            boost::algorithm::erase_all(newNameFixed, "\r");
            boost::algorithm::erase_all(newNameFixed, "\xE2\x80\x8F");
            boost::algorithm::erase_all(newNameFixed, "\xE2\x80\x8E");
            if (newNameFixed.size() != strlen(newName)) {
		        DETOUR_MEMBER_CALL(newNameFixed.c_str());
                Notify("Invalid Player Name", "Found a newline / carriage return character in player name","", client);
            }
        }
		DETOUR_MEMBER_CALL(newName);
    }

    DETOUR_DECL_MEMBER(void, CTFGameRules_ClientCommandKeyValues, edict_t *pEntity, KeyValues *pKeyValues)
	{
        auto client = (CGameClient *)sv->GetClient(pEntity->m_EdictIndex - 1);
        if (!client->IsFakeClient()) {
            if (FStrEq(pKeyValues->GetName(), "AchievementEarned")) {
                auto id = pKeyValues->GetInt("achievementID");
                if (id > 2805 || id < 127) {
                    Notify("Invalid achievement ID", "Player achieved invalid achievement", std::format("ID={}", id), client);
                }
            }
            if (FStrEq(pKeyValues->GetName(), "MVM_Revive_Response")) {
                auto accepted = pKeyValues->GetBool("accepted");
                if (accepted) {
                    Notify("Invalid Revive Response", "Player tried to hack instant revive himself", "", client);
                    return;
                }
            }
        }

		DETOUR_MEMBER_CALL(pEntity, pKeyValues);
	}

    DETOUR_DECL_MEMBER(void, CTFGameStats_Event_PlayerFiredWeapon, CTFPlayer *pPlayer, bool bCritical)
	{
		DETOUR_MEMBER_CALL(pPlayer, bCritical);
        if (pPlayer->IsRealPlayer()) {
            auto mod = pPlayer->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
            mod->lastShootTick = gpGlobals->tickcount;
        }
	}

    

    ConVar sig_etc_detector_srv_choke("sig_etc_detector_srv_choke", "0", FCVAR_NOTIFY, "Choke srv");
	ConCommand ccmd_save("sig_etc_detector_sleep", [](const CCommand &args){
        sleep(atoi(args[1]));
    }, "sleep");

    class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePreEntityThinkListener
	{
	public:
		CMod() : IMod("Etc:Detector")
		{
			MOD_ADD_DETOUR_STATIC(UTIL_TraceEntity, "UTIL_TraceEntity [IHandleEntity]");
			MOD_ADD_DETOUR_STATIC(UTIL_SetOrigin, "UTIL_SetOrigin");

			MOD_ADD_DETOUR_MEMBER(CTFPlayer_PostThink, "CTFPlayer::PostThink");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayer_PlayerRunCommand, "CTFPlayer::PlayerRunCommand", HIGHEST);
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_Teleport, "CBaseEntity::Teleport");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Spawn, "CTFPlayer::Spawn");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FireGameEvent, "CTFGameRules::FireGameEvent");
			MOD_ADD_DETOUR_MEMBER(CBaseServer_CheckProtocol, "CBaseServer::CheckProtocol");
			MOD_ADD_DETOUR_MEMBER(CServerGameClients_ClientPutInServer, "CServerGameClients::ClientPutInServer");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage_Alive, "CTFPlayer::OnTakeDamage_Alive");
			//MOD_ADD_DETOUR_STATIC(TE_FireBullets, "TE_FireBullets");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnConditionAdded, "CTFPlayerShared::OnConditionAdded");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnConditionRemoved, "CTFPlayerShared::OnConditionRemoved");
			MOD_ADD_DETOUR_MEMBER(CServerPlugin_OnQueryCvarValueFinished, "CServerPlugin::OnQueryCvarValueFinished");
			MOD_ADD_DETOUR_MEMBER(CBaseClient_SetName, "CBaseClient::SetName");
			MOD_ADD_DETOUR_MEMBER(CServerPlugin_ClientSettingsChanged, "CServerPlugin::ClientSettingsChanged");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ClientCommandKeyValues, "CTFGameRules::ClientCommandKeyValues");

			MOD_ADD_DETOUR_MEMBER(CTFPlayer_FireBullet, "CTFPlayer::FireBullet");
			MOD_ADD_DETOUR_MEMBER(CTFGameStats_Event_PlayerFiredWeapon, "CTFGameStats::Event_PlayerFiredWeapon");
		}

        virtual bool OnLoad() override
		{
            developer = icvar->FindVar("developer");
			notify_forward = forwards->CreateForward("SIG_DetectNotify", ET_Hook, 7, NULL, Param_Cell, Param_String, Param_String, Param_String, Param_Cell, Param_Array, Param_Cell);
            backtrack_ticks = TimeToTicks(0.2);
            return true;
        }
		
		virtual void OnUnload() override
		{
			forwards->ReleaseForward(notify_forward);
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void FrameUpdatePreEntityThink() override
		{
            static CountdownTimerInline timerIpConnectCounter;
            static CountdownTimerInline sleepTimer;
            static CCycleCount lastLagTime;
            static int lastLagTickcount = -1;

            if (lastLagTickcount == -1) {
                lastLagTime.Sample();
                lastLagTickcount = gpGlobals->tickcount;
            }
            CCycleCount currentUpdateTime;
            currentUpdateTime.Sample();
            
            // Server clock drifted due to lag, reset
            static ConVarRef host_timescale("host_timescale");
            if (host_timescale.GetFloat() != 1.0f || gpGlobals->tickcount - lastLagTickcount < (int)((currentUpdateTime.GetSeconds() - lastLagTime.GetSeconds()) / gpGlobals->interval_per_tick)) {
                last_server_lag_time = gpGlobals->curtime;
                last_server_lag_tick = gpGlobals->tickcount;
                lastLagTime.Sample();
                lastLagTickcount = gpGlobals->tickcount;

                DevMsg("server lagged\n");
            }


            if (timerIpConnectCounter.IsElapsed()) {
                timerIpConnectCounter.Start(5.0f);
                std::erase_if(ip_connect_counter, [](auto &entry){
                    return --entry.second <= 0;
                });
            }
            if (gpGlobals->curtime < last_server_lag_time) {
                last_server_lag_time = 0.0f;
                last_server_lag_tick = 0;
            }
            // if (updateTimeDelta.GetSeconds() > (double)gpGlobals->interval_per_tick * 5.0) {
            //     last_server_lag_time = gpGlobals->curtime;
            //     last_server_lag_tick = gpGlobals->tickcount;
            // }
            //Msg("Time since last tick %f %f\n", updateTimeDelta.GetSeconds(), gpGlobals->frametime );

            if (gpGlobals->curtime - last_server_lag_time > 2.0f) {
                //AVERAGE_TIME(AnalizeCmds);
                ForEachTFPlayer([](CTFPlayer *player){
                    if (!player->IsRealPlayer()) return;
                    auto mod = player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
                    mod->AnalizeCmds();
                    if ((gpGlobals->tickcount + player->entindex() * 9) % 66 == 0) {
                        mod->RefreshFrameBudget();
                    }
                });
            }
            if (sig_etc_detector_srv_choke.GetInt() > 0 && sleepTimer.IsElapsed()) {
                sleepTimer.Start(1.0f);
                timespec time {sig_etc_detector_srv_choke.GetInt()/1000, sig_etc_detector_srv_choke.GetInt() * 1000000};
                nanosleep(&time, nullptr);
                Msg("sleep now\n");
            }
        }
	};
	CMod s_Mod;

    ModCommandClientAdmin sig_snap("sig_snap", [](CCommandPlayer *player, const CCommand &args){
        player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl")->snapTest = true;
    });

    ModCommandClientAdmin sig_trigger("sig_trigger", [](CCommandPlayer *player, const CCommand &args){
        player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl")->triggerTest = !player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl")->triggerTest;
    });

    ModCommandClientAdmin sig_lock("sig_lock", [](CCommandPlayer *player, const CCommand &args){
        player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl")->lockTest = !player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl")->lockTest;
    });

    ModCommandClientAdmin sig_crit("sig_crit", [](CCommandPlayer *player, const CCommand &args){
        player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl")->critTest = !player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl")->critTest;
    });

    ConVar cvar_enable("sig_etc_detector", "0", FCVAR_NOTIFY,
		"Mod: Detector",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}