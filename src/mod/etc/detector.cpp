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
#include <fmt/format.h>


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
        {"cl_interp", {0.001f, 0.5f}},
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

    ConVar cvar_bckt_tolerance("sig_etc_detector_bckt_tolerance", "0", FCVAR_NOTIFY, "Tolerance");

    int backtrack_ticks;
    float last_server_lag_time = 0.0f;
    int last_server_lag_tick = 0;

    void Notify(const std::string &shortReason, const std::string &description, CTFPlayer *player, IClient *client) {
        if (client == nullptr || client->IsFakeClient()) return;
        Msg("%s - %s\n", shortReason.c_str(), description.c_str());
        if (player != nullptr) {
            ClientMsg(player, "%s - %s\n", shortReason.c_str(), description.c_str());
        }
    }
    void Notify(const std::string &shortReason, const std::string &description, CTFPlayer *player) {
        Notify(shortReason, description, player, player != nullptr ? sv->GetClient(player->entindex()) : nullptr);
    }
    void Notify(const std::string &shortReason, const std::string &description, IClient *client) {
        Notify(shortReason, description, ToTFPlayer(UTIL_PlayerByIndex(client->GetPlayerSlot()+1)), client);
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
            player->m_nForceTauntCam ||
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
        float aimDistanceHead = 0.0f;
        float aimDistanceTorso = 0.0f;
        float aimDistanceCenter = 0.0f;
        float speedOurEnemy = 0.0f;
    };

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

        void PostRunCommand(CUserCmd &origCmd, CUserCmd* cmd, IMoveHelper* moveHelper) {
            AVERAGE_TIME(PostRunCommand)
            //ClientMsg(player, "Srv tick: %d | Cl tick: %d | cmdnum %d\n", gpGlobals->tickcount, cmd->tick_count, cmd->command_number);
            //Msg("Srv tick: %d | Cl tick: %d | cmdnum %d\n", gpGlobals->tickcount, cmd->tick_count, cmd->command_number);
            lagcompensation->StartLagCompensation(player, cmd);
            trace_t trace;
            FireEyeTrace(trace, player, 8192, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, COLLISION_GROUP_PLAYER);
            lagcompensation->FinishLagCompensation(player);

            UserCmdInfo info;
            info.aimEnemyPlayer = trace.DidHit() && trace.m_pEnt != nullptr && trace.m_pEnt->IsPlayer() && trace.m_pEnt->GetTeamNumber() != player->GetTeamNumber();
            info.aimAtHead = info.aimEnemyPlayer && trace.hitgroup == HITGROUP_HEAD;
            if (info.aimEnemyPlayer) {
                auto aimPlayer = ToTFPlayer(trace.m_pEnt);
                int boneHead = aimPlayer->LookupBone("bip_head");
                int boneTorso = aimPlayer->LookupBone("bip_pelvis");
                QAngle ang;
                Vector vec;
                info.aimDistanceCenter = LineDistanceToPoint(trace.startpos, trace.endpos, aimPlayer->WorldSpaceCenter());
                if (boneHead != -1) {
                    aimPlayer->GetBonePosition(boneHead, vec, ang);
                    info.aimDistanceHead = LineDistanceToPoint(trace.startpos, trace.endpos, vec);
                }
                else {
                    info.aimDistanceHead = info.aimDistanceCenter;
                }
                if (boneTorso != -1) {
                    aimPlayer->GetBonePosition(boneTorso, vec, ang);
                    info.aimDistanceTorso = LineDistanceToPoint(trace.startpos, trace.endpos, vec);
                }
                else {
                    info.aimDistanceTorso = info.aimDistanceCenter;
                }
                info.speedOurEnemy = (player->GetAbsVelocity() - aimPlayer->GetAbsVelocity()).Length();
                ClientMsg(player, "Aim dists: %f %f %f %d %f\n",info.aimDistanceCenter, info.aimDistanceHead, info.aimDistanceTorso, info.aimAtHead, info.speedOurEnemy);
            }
            info.cmd = origCmd;
            info.hadForcedAngles = MayHaveInvalidAngles();
            cmdHistory.push_back(info);
            if (cmdHistory.size() > 80) {
                cmdHistory.erase(cmdHistory.begin());
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

            highestCmdTick = Max(highestCmdTick, cmd->tick_count);
            highestCmdNum = Max(highestCmdNum, cmd->command_number);

            if (cmd->tick_count - baselineCmdTick != cmd->command_number - baselineCmdNum) {
                Notify("Invalid usercmd", fmt::format("Usercmd command number mismatch {} with tick count {}", cmd->command_number - baselineCmdNum, cmd->tick_count - baselineCmdTick), player);
                baselineCmdTick = cmd->tick_count;
                baselineCmdNum = cmd->command_number;
            }

            if (cmd->tick_count > gpGlobals->tickcount) {
                Notify("Invalid usercmd", fmt::format("Usercmd tick count {} higher than server tick count {}", cmd->tick_count, gpGlobals->tickcount), player);
            }

            if (snapTest) {
                snapTest = false;
                ForEachTFPlayer([this, cmd](CTFPlayer *target){
                    if (target->IsAlive() && target->GetTeamNumber() != player->GetTeamNumber()) {
                        Vector delta = target->GetAbsOrigin() - player->GetAbsOrigin();
                
                        QAngle angToTarget;
                        VectorAngles(delta, angToTarget);
                        cmd->viewangles = angToTarget;
                    }
                });
            }

            if (triggerTest) {
                
                lagcompensation->StartLagCompensation(player, cmd);
                trace_t trace;
                FireEyeTrace(trace, player, 8192, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, COLLISION_GROUP_PLAYER);
                lagcompensation->FinishLagCompensation(player);
                if (trace.DidHit() && trace.m_pEnt != nullptr && trace.m_pEnt->IsPlayer() && trace.m_pEnt->GetTeamNumber() != player->GetTeamNumber()) {
                    cmd->buttons |= IN_ATTACK;
                }
            }
        }

        bool MayHaveInvalidAngles() {
            return HasThirdPersonCamera(player) || gpGlobals->curtime - this->timeTeleport < 2.0f;
        }

        void CheckAngles(CUserCmd* cmd) {
            if (MayHaveInvalidAngles()) return;

            if (abs(cmd->viewangles[PITCH]) > 89.0001f || abs(cmd->viewangles[ROLL]) > 50.0001f) {
                Notify("Invalid angles", fmt::format("Player had angles in invalid range {} {} {}", cmd->viewangles.x, cmd->viewangles.y, cmd->viewangles.z), player);
            }
        }
        
        void AnalizeCmds() {
            constexpr int frameCheckCount = 16;
            if (cmdHistory.size() < frameCheckCount) return;

            auto historySorted = cmdHistory;
            std::sort(historySorted.begin(), historySorted.end(), [](auto &info1, auto &info2){
                return info1.cmd.tick_count < info2.cmd.tick_count;
            });

            for (int i = 0; i < frameCheckCount; i++) {
                if (historySorted[i].hadForcedAngles) return;
            }

            if (historySorted[0].cmd.command_number > historySorted[1].cmd.command_number && historySorted[0].cmd.tick_count < historySorted[1].cmd.tick_count) {
                Notify("Invalid usercmd", fmt::format("Usercmd command number {} from tick {} higher than a command number {} from next tick {}", historySorted[0].cmd.command_number, historySorted[0].cmd.tick_count, historySorted[1].cmd.command_number, historySorted[1].cmd.tick_count), player);
            }

            if (historySorted[0].cmd.command_number == historySorted[1].cmd.command_number && historySorted[0].cmd.tick_count == historySorted[1].cmd.tick_count && historySorted[0].cmd.viewangles != historySorted[1].cmd.viewangles && historySorted[0].cmd.buttons != historySorted[1].cmd.buttons) {
                Notify("Invalid usercmd", fmt::format("Usercmd duplicated command number {} and tick count {}, but with different data", historySorted[0].cmd.command_number, historySorted[0].cmd.tick_count), player);
            }

            bool triggerBotStartCheck = false;
            bool triggerBotHeadStartCheck = false;
            bool triggerBotEndCheck = false;
            bool triggerBotHeadEndCheck = false;
            if (!historySorted[0].aimAtHead && historySorted[1].aimAtHead && !(historySorted[0].cmd.buttons & IN_ATTACK) && (historySorted[1].cmd.buttons & IN_ATTACK)) {
                triggerBotHeadStartCheck = true;
            }
            else if (!historySorted[0].aimEnemyPlayer && historySorted[1].aimEnemyPlayer && !(historySorted[0].cmd.buttons & IN_ATTACK) && (historySorted[1].cmd.buttons & IN_ATTACK)) {
                triggerBotStartCheck = true;
            }

            if (historySorted[1].aimAtHead && !historySorted[2].aimAtHead && (historySorted[1].cmd.buttons & IN_ATTACK) && !(historySorted[2].cmd.buttons & IN_ATTACK)) {
                triggerBotHeadEndCheck = true;
            }
            else if (historySorted[1].aimEnemyPlayer && !historySorted[2].aimEnemyPlayer && (historySorted[1].cmd.buttons & IN_ATTACK) && !(historySorted[2].cmd.buttons & IN_ATTACK)) {
                triggerBotEndCheck = true;
            }
            if ((triggerBotStartCheck || triggerBotHeadStartCheck) && (triggerBotEndCheck || triggerBotHeadEndCheck)) {
                Notify("Triggerbot start+end", fmt::format("Player pressed attack button same tick as they aimed at enemy target, then unpressed it next tick as target was dead. Did the player aim at the head specifically: {}", triggerBotHeadStartCheck), player);
            }
            else if ((triggerBotStartCheck || triggerBotHeadStartCheck) && !(triggerBotEndCheck || triggerBotHeadEndCheck)) {
                Notify("Triggerbot start", fmt::format("Player pressed attack button same tick as they aimed at enemy target. Did the player aim at the head specifically: {}", triggerBotHeadStartCheck), player);
            }
            else if (!(triggerBotStartCheck || triggerBotHeadStartCheck) && (triggerBotEndCheck || triggerBotHeadEndCheck)) {
                Notify("Triggerbot end", fmt::format("Player unpressed attack button same tick as they stopped aiming at enemy target or the target was dead. Did the player aim at the head specifically: {}", triggerBotHeadEndCheck), player);
            }
            auto ang0 = historySorted[0].cmd.viewangles;
            auto ang1 = historySorted[1].cmd.viewangles;
            auto ang2 = historySorted[2].cmd.viewangles;
            float delta01 = abs(AngleDiff(ang0.x, ang1.x)) + abs(AngleDiff(ang0.y, ang1.y));
            float delta12 = abs(AngleDiff(ang1.x, ang2.x)) + abs(AngleDiff(ang1.y, ang2.y));
            float delta02 = abs(AngleDiff(ang0.x, ang2.x)) + abs(AngleDiff(ang0.y, ang2.y));

            if (!historySorted[0].aimEnemyPlayer && historySorted[1].aimEnemyPlayer && !historySorted[2].aimEnemyPlayer && !(historySorted[0].cmd.buttons & IN_ATTACK) && (historySorted[1].cmd.buttons & IN_ATTACK)) {
                if (delta01 > 10.0f && delta12 > 10.0f && delta02 < 2.0f) {
                    Notify("Silent aim", fmt::format("Player snapped aim in one tick to an enemy player, attacked, then reverted back to previous aim angles. aim delta 01 = {}, delta 12 = {}, delta 02 = {}. Did the player aim at the head specifically: {}", delta01, delta12, delta02, historySorted[1].aimAtHead), player);
                }
            }
            ClientMsg(player, "buttons %d %d %d %d\n", historySorted[0].cmd.buttons, historySorted[1].cmd.buttons, historySorted[0].aimEnemyPlayer, historySorted[1].aimEnemyPlayer);
            int framesAiming = 0;
            float minDistanceDiffTotal = 0;
            bool aimedAtHead = false;
            bool aimedAtTorso = false;
            float angMoveTotalNonAimed = 0;
            int nonAimedTickCount = 0;
            Vector aimDeltaVecNonAimedTotal = vec3_origin;
            Vector aimDeltaVecAimed = vec3_origin;
            float maxAngDeltaWhenAimingAtEnemy = 0;

            for (int i = 0; i < frameCheckCount - 1; i++) {
                auto &cmd0 = historySorted[i];
                auto &cmd1 = historySorted[i+1];
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
                if (cmd0.cmd.viewangles != cmd1.cmd.viewangles && cmd1.aimEnemyPlayer && cmd1.speedOurEnemy > 50.0f) {
                    framesAiming++;
                    float minDist0 = Min(cmd0.aimDistanceHead, Min(cmd0.aimDistanceTorso, cmd0.aimDistanceCenter));
                    float minDist1 = Min(cmd1.aimDistanceHead, Min(cmd1.aimDistanceTorso, cmd1.aimDistanceCenter));
                    if (cmd1.aimAtHead) {
                        aimedAtHead = true;
                    }
                    else {
                        aimedAtTorso = true;
                    }
                    minDistanceDiffTotal += abs(minDist0 - minDist1);
                }
            }
            Vector aimDeltaVec01 = Vector(AngleDiff(historySorted[0].cmd.viewangles.x, historySorted[1].cmd.viewangles.x), AngleDiff(historySorted[0].cmd.viewangles.y, historySorted[1].cmd.viewangles.y), 0).Normalized();
            Vector aimDeltaVec12 = Vector(AngleDiff(historySorted[1].cmd.viewangles.x, historySorted[2].cmd.viewangles.x), AngleDiff(historySorted[1].cmd.viewangles.y, historySorted[2].cmd.viewangles.y), 0).Normalized();
            
           // ClientMsg(player, "aim delta dot %f %f\n", RAD2DEG(acos(DotProductAbs(aimDeltaVec01, aimDeltaVec12))), delta01);
            if (!historySorted[0].aimEnemyPlayer && !historySorted[1].aimEnemyPlayer && historySorted[2].aimEnemyPlayer) {
                // Look at sharp turns with fast speed
                if (delta12 > 10.0f) {
                    Vector aimDeltaVec01 = Vector(AngleDiff(historySorted[0].cmd.viewangles.x, historySorted[1].cmd.viewangles.x), AngleDiff(historySorted[0].cmd.viewangles.y, historySorted[1].cmd.viewangles.y), 0).Normalized();
                    Vector aimDeltaVec12 = Vector(AngleDiff(historySorted[1].cmd.viewangles.x, historySorted[2].cmd.viewangles.x), AngleDiff(historySorted[1].cmd.viewangles.y, historySorted[2].cmd.viewangles.y), 0).Normalized();
                    if (delta01 < 1.0f || delta12 / delta01 > 10 || DotProductAbs(aimDeltaVec01, aimDeltaVec12) < 0.7) { // 45 degress
                        Notify("Aim snap", fmt::format("Player snapped aim to an enemy player with {} times above average angular velocity. Avg delta: {}. Aim snap delta: {}. Did the player aim at the head specifically: {}", maxAngDeltaWhenAimingAtEnemy / (angMoveTotalNonAimed / (frameCheckCount - 1)), angMoveTotalNonAimed / (frameCheckCount - 1), delta01, maxAngDeltaWhenAimingAtEnemy, historySorted[2].aimAtHead), player);
                    }
                }
            }
            aimDeltaVecAimed.NormalizeInPlace();
            aimDeltaVecNonAimedTotal.NormalizeInPlace();
            //ClientMsg(player, "%f %f | %f %f | %f\n", aimDeltaVecAimed.x, aimDeltaVecAimed.y, aimDeltaVecNonAimedTotal.x, aimDeltaVecNonAimedTotal.y, RAD2DEG(acos(DotProductAbs(aimDeltaVecAimed, aimDeltaVecNonAimedTotal))));
            //ClientMsg(player, "aim angle delta: %f, avg: %f, cur %f, dot %f | avg aim dist: %f %d | \n", maxAngDeltaWhenAimingAtEnemy, angMoveTotalNonAimed / (frameCheckCount - 1), delta01, DotProduct(aimDeltaVecAimed.Normalized(), aimDeltaVecNonAimedTotal.Normalized()), minDistanceDiffTotal / framesAiming, framesAiming);
            if (framesAiming > frameCheckCount / 4) {
                //ClientMsg(player, "avg aim %f %d\n", minDistanceDiffTotal / framesAiming, framesAiming);
                if (minDistanceDiffTotal / framesAiming < 0.05f) {
                    Notify("Aim lock", fmt::format("Player locked aim to head: {} torso: {}. Average distance to head/torso bone: {}", aimedAtHead, aimedAtTorso, minDistanceDiffTotal / framesAiming), player);
                }
            }
        }

        void Reset() {
            cmdHistory.clear();
            baselineCmdTick = 0;
            baselineCmdNum = 0;
            highestCmdTick = 0;
            highestCmdNum = 0;
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

        std::vector<UserCmdInfo> cmdHistory;

        int baselineCmdTick = 0;
        int baselineCmdNum = 0;
        int highestCmdTick = 0;
        int highestCmdNum = 0;

        int prevUpdateRate = 0;

        bool snapTest = false;
        bool triggerTest = false;
	}; 

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
        mod->ValidateCmd(cmd);
        if (player->pl->fixangle != FIXANGLE_NONE || player->GetMoveParent() != nullptr) {
            mod->timeTeleport = gpGlobals->curtime;
        }
        mod->CheckAngles(cmd);

        mod->StoreTickcount(cmd->tick_count);

        cmd->tick_count = mod->CorrectTickcount(cmd->tick_count);

        CUserCmd cmdPre = *cmd;
        DETOUR_MEMBER_CALL(cmd, moveHelper);

        mod->PostRunCommand(cmdPre, cmd, moveHelper);
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
        
        TIME_SCOPE2(CheckClientCVars);
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

            auto mod = player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl");
            mod->lastDamagedPlayerTick = gpGlobals->tickcount;
            mod->lastDamagedPlayerWeaponName = info.GetWeapon()->GetClassnameString();
        }

		return DETOUR_MEMBER_CALL(info);
	}
    DETOUR_DECL_STATIC(void, TE_FireBullets, int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, 
					 int iWeaponID, int	iMode, int iSeed, float flSpread, bool bCritical)
	{
        auto player = UTIL_PlayerByIndex(iPlayerIndex);
        if (player != nullptr) {
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
        auto client = reinterpret_cast<CBaseClient *>(this);
        if (event->GetName() == nullptr) { 
            DETOUR_MEMBER_CALL(event); 
            return;
        }

        DETOUR_MEMBER_CALL(event); 

        if (strcmp(event->GetName(), "achievement_earned") == 0) {
            int index = event->GetInt("player");

            int achieveId = event->GetInt("achievement");
            if (achieveId > 2805 || achieveId < 127) {
                Notify("Invalid achievement ID", fmt::format("Player achieved invalid achievement with ID {}", index), sv->GetClient(index - 1));
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
                    Notify("Client CVar check - cheat cvar detected", fmt::format("Found cvar value for cheat cvar {}", pCvarName), player);
                }
                return;
            }
            else if ((eStatus != eQueryCvarValueStatus_ValueIntact || pCvarValue == nullptr)) {
                Notify("Client CVar check - failure", fmt::format("Cannot check value of cvar {}", pCvarName), player);
            }
            else {
                float cvarValue = strtof(pCvarValue, nullptr);
                if ((cvarValue < bounds.min || cvarValue > bounds.max) && (bounds.cvar == nullptr || bounds.cvar->GetFloat() != cvarValue)) {
                    Notify("Client CVar check - oob", fmt::format("Client cvar {} = {} is out of bounds ({}-{})", pCvarName, cvarValue, bounds.min, bounds.max), player);
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
                Notify("Newline in chat", "Found a newline / carriage return character in chat message", player);
                return;
            }
		}
		DETOUR_STATIC_CALL(edict, args, team);
	}

	DETOUR_DECL_MEMBER(void, CServerPlugin_ClientSettingsChanged, edict_t *edict)
	{
        auto client = (CGameClient *)sv->GetClient(edict->m_EdictIndex - 1);
        if (!client->IsFakeClient() && client->m_ConVars != nullptr) {
            TIME_SCOPE2(ClientSettingsChanged);
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
                        Notify("Clenit CVar check - oob", fmt::format("Client cvar {} = {} is out of bounds ({}-{})", subkey->GetName(), value, bounds.min, bounds.max), client);
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
                        if (find != mod->connectClientCvarPrevValues.end() && find->second != client->m_ConVars->GetFloat(cvarName.c_str())) {
                            Notify("Client CVar check - change while alive", fmt::format("Client cvar {} changed from {} to {} while player was alive", cvarName, find->second, client->m_ConVars->GetFloat(cvarName.c_str())), client);
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
                Notify("Invalid Player Name", "Found a newline / carriage return character in player name", client);
            }
        }
		DETOUR_MEMBER_CALL(newName);
    }

    DETOUR_DECL_MEMBER(void, CTFGameRules_ClientCommandKeyValues, edict_t *pEntity, KeyValues *pKeyValues)
	{
        auto client = (CGameClient *)sv->GetClient(pEntity->m_EdictIndex - 1);
		if (!client->IsFakeClient() && FStrEq(pKeyValues->GetName(), "AchievementEarned")) {
            auto id = pKeyValues->GetInt("achievementID");
            if (id > 2805 || id < 127) {
                Notify("Invalid achievement ID", fmt::format("Player achieved invalid achievement with ID {}", id), client);
            }
		}

		DETOUR_MEMBER_CALL(pEntity, pKeyValues);
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
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayer_PlayerRunCommand, "CTFPlayer::PlayerRunCommand", HIGHEST);
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_Teleport, "CBaseEntity::Teleport");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Spawn, "CTFPlayer::Spawn");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FireGameEvent, "CTFGameRules::FireGameEvent");
			MOD_ADD_DETOUR_MEMBER(CBaseServer_CheckProtocol, "CBaseServer::CheckProtocol");
			MOD_ADD_DETOUR_MEMBER(CServerGameClients_ClientPutInServer, "CServerGameClients::ClientPutInServer");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage_Alive, "CTFPlayer::OnTakeDamage_Alive");
			MOD_ADD_DETOUR_STATIC(TE_FireBullets, "TE_FireBullets");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnConditionAdded, "CTFPlayerShared::OnConditionAdded");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnConditionRemoved, "CTFPlayerShared::OnConditionRemoved");
			MOD_ADD_DETOUR_MEMBER(CServerPlugin_OnQueryCvarValueFinished, "CServerPlugin::OnQueryCvarValueFinished");
			MOD_ADD_DETOUR_MEMBER(CBaseClient_SetName, "CBaseClient::SetName");
			MOD_ADD_DETOUR_MEMBER(CServerPlugin_ClientSettingsChanged, "CServerPlugin::ClientSettingsChanged");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ClientCommandKeyValues, "CTFGameRules::ClientCommandKeyValues");
		}

        virtual bool OnLoad() override
		{
            backtrack_ticks = TimeToTicks(0.2);
            return true;
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
                Msg("server lagged\n");
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
                AVERAGE_TIME(AnalizeCmds);
                ForEachTFPlayer([](CTFPlayer *player){
                    if (!player->IsRealPlayer()) return;
                    player->GetOrCreateEntityModule<DetectorPlayerModule>("detectorpl")->AnalizeCmds();
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

    ConVar cvar_enable("sig_etc_detector", "0", FCVAR_NOTIFY,
		"Mod: Detector",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}