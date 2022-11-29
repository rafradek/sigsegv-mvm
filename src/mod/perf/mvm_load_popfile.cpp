#include "mod.h"
#include "stub/gamerules.h"
#include "util/scope.h"
#include "util/backtrace.h"


namespace Mod::Perf::MvM_Load_Popfile
{
	RefCount rc_CPopulationManager_SetPopulationFilename;
	DETOUR_DECL_MEMBER(void, CPopulationManager_SetPopulationFilename, const char *filename)
    {
		SCOPED_INCREMENT(rc_CPopulationManager_SetPopulationFilename);
        DETOUR_MEMBER_CALL(CPopulationManager_SetPopulationFilename)(filename);
    }

	RefCount rc_tf_mvm_popfile;
	DETOUR_DECL_STATIC(void, tf_mvm_popfile, const CCommand& args)
	{
		SCOPED_INCREMENT(rc_tf_mvm_popfile);
		DETOUR_STATIC_CALL(tf_mvm_popfile)(args);
	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_ResetMap)
	{
		if (rc_tf_mvm_popfile > 0 && rc_CPopulationManager_SetPopulationFilename == 0) return;
		DETOUR_MEMBER_CALL(CPopulationManager_ResetMap)();
	}
	
	RefCount rc_CPopulationManager_JumpToWave;
	DETOUR_DECL_MEMBER(void, CPopulationManager_JumpToWave, unsigned int wave, float f1)
	{
		SCOPED_INCREMENT_IF(rc_CPopulationManager_JumpToWave, f1 == 0);
		DETOUR_MEMBER_CALL(CPopulationManager_JumpToWave)(wave, f1);
	}
	
	RefCount rc_CTFGameRules_SetupOnRoundStart;
	DETOUR_DECL_MEMBER(void, CTFGameRules_SetupOnRoundStart)
	{
		SCOPED_INCREMENT(rc_CTFGameRules_SetupOnRoundStart);
		DETOUR_MEMBER_CALL(CTFGameRules_SetupOnRoundStart)();
	}

	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
		if (rc_CPopulationManager_JumpToWave > 0 && rc_CTFGameRules_SetupOnRoundStart == 0) return true;
		return DETOUR_MEMBER_CALL(CPopulationManager_Parse)();
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:MvM_Load_Popfile")
		{
			MOD_ADD_DETOUR_STATIC(tf_mvm_popfile,              "tf_mvm_popfile");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CPopulationManager_ResetMap, "CPopulationManager::ResetMap", HIGHEST);
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_SetPopulationFilename, "CPopulationManager::SetPopulationFilename");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CPopulationManager_Parse, "CPopulationManager::Parse", HIGHEST);
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_SetupOnRoundStart, "CTFGameRules::SetupOnRoundStart");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_JumpToWave, "CPopulationManager::JumpToWave");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_mvm_load_popfile", "0", FCVAR_NOTIFY,
		"Mod: eliminate unnecessary duplication of parsing/init code during tf_mvm_popfile",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
#if 0
	RefCount rc_tf_mvm_popfile;
	int tf_mvm_popfile_resets = 0;
	DETOUR_DECL_STATIC(void, tf_mvm_popfile, const CCommand& args)
	{
		SCOPED_INCREMENT(rc_tf_mvm_popfile);
		tf_mvm_popfile_resets = 0;
		
		DETOUR_STATIC_CALL(tf_mvm_popfile)(args);
	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_ResetMap)
	{
		if (rc_tf_mvm_popfile > 0) {
			switch (++tf_mvm_popfile_resets) {
			case 1:
				Msg("++++ ResetMap #1: calling function as usual.\n");
				DETOUR_MEMBER_CALL(CPopulationManager_ResetMap)();
				return;
			
			// during the 20150716b --> 20150724a update (post-GunMettle), the devs replaced the GR_STATE_RESTART transition
			// in the tf_mvm_popfile command with an inexplicable second call to g_pPopulationManager->ResetMap();
			// this breaks the ability to load popfiles if the initial popfile load for a map fails, until you load an
			// entirely different map; so, to fix this, we essentially restore the second call to the way it was before
			case 2:
				Msg("++++ ResetMap #2: calling TFGameRules()->State_Transition(GR_STATE_RESTART) instead.\n");
				TFGameRules()->State_Transition(GR_STATE_RESTART);
				return;
			
			default:
				Msg("++++ ResetMap #%d: doing nothing whatsoever.\n", tf_mvm_popfile_resets);
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CPopulationManager_ResetMap)();
	}
	
	
	DETOUR_DECL_MEMBER(bool, CPopulationManager_Initialize)
	{
		BACKTRACE();
		return DETOUR_MEMBER_CALL(CPopulationManager_Initialize)();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:MvM_Load_Popfile")
		{
			MOD_ADD_DETOUR_STATIC(tf_mvm_popfile,              "tf_mvm_popfile");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_ResetMap, "CPopulationManager::ResetMap");
			
		//	MOD_ADD_DETOUR_MEMBER(CPopulationManager_Initialize, "CPopulationManager::Initialize");
		}
	};
	CMod s_Mod;
	
	// So the code, as it stands, fixes tf_mvm_popfile in post-zero-wave conditions.
	// And in post-zero-wave loads, the popfile is parsed just once.
	// - Look at the BinDiff and make sure we didn't cause a regression WRT votes (the subject of the change)
	
	// But in post-nonzero-wave loads, the popfile is parsed twice, due to GR_STATE_PREROUND being transitioned to twice.
	// - Check how they did this back in the early days of MvM
	
	// And as for tf_mvm_jump_to_wave, it only parses once, which is fine.
	
	// BUG: apparently, when this thing is enabled, when joining a server,
	// - the mission immediately starts
	// - it uses PvP timers
#endif
}

/*

Latest TF2 game states when MvM map loads:

INIT (right as map load begins)

PREGAME (very very early in map load)

all of the map loading stuff happens here
including the initial attempt at popfile loading

the first player can get through the MOTD screen and arrive at the class selection screen while it's still PREGAME (showing the wrong HUD stuff)

STARTGAME (when first player chooses a class)

PREROUND (almost immediately after STARTGAME)

the "actual" popfile load happens here

BETWEEN_RNDS (very soon after PREROUND)

it's now pre-wave 1 ready-up time and the HUD is correct


when executing a valid tf_mvm_popfile at this point, we get this:
- popfile parse
- BETWEEN_RNDS -> PREROUND
- popfile parse
- PREROUND -> BETWEEN_RNDS
- wave force-end, popfile parse
- BETWEEN_RNDS -> PREROUND
- popfile parse
- PREROUND -> BETWEEN_RNDS

*/



/*

temp textfile from debugging session RE: tf_mvm_popfile stuff
=============================================================

- see if we can determine what the cause of the post-GunMettle "popmgr gets stuck if first popfile load fails" thing is
- server_srv_20170116a.idb

refs to CPopulationManager::FindPopulationFileByShortName
- CMannVsMachineLogic::InitPopulationManager

refs to "tf_mvm_popfile"
- CPopulationManager::LoadMvMMission
- CPopulationManager::CycleMission

Notable Changes:
- CMannVsMachineLogic::InitPopulationManager
  - now calls CPopulationManager::FindDefaultPopulationFileShortNames
- CPopulationManager::Initialize (via CPopulationManager::SetupOnRoundStart via CMannVsMachineLogic::SetupOnRoundStart; possibly others)
  - seems pretty much the same
- CPopulationManager::GameRulesThink
  - called from CTFGameRules::Think; didn't exist before

TODO:
- look for refs to g_hMannVsMachineLogic
- look for refs to g_pPopulationManager

FINDINGS:
- first popfile load occurs with GR_STATE_PREGAME
- first player getting to the class selection screen causes STARTGAME and PREROUND
  - PREROUND transition causes CTFGameRules::SetupOnRoundStart to run
    - this calls CMannVsMachineLogic::SetupOnRoundStart etc
  - after that initialization fails ('Can't open x.pop'), we go to BETWEEN_ROUNDS
- when loading a popfile AFTER the initial failure:
  - we get to CPopulationManager::Initialize
    - we get to the CPopulationManager::ClearCheckpoint call
    - we get to the CPopulationManager::DebugWaveStats call
    - there are zero waves despite the parse not failing!

BREAKPOINTS:
tf_mvm_popfile
CMannVsMachineLogic::SetupOnRoundStart
CMannVsMachineLogic::InitPopulationManager
CPopulationManager::Initialize
CPopulationManager::JumpToWave

SOLUTION:
- The problem is that when loading a new mission, JumpToWave is called with 0 as the wave number. But there are 0 waves in the currently loaded mission, and the checks in JumpToWave don't allow the important stuff (mainly CPopulationManager::Initialize) to run.
- ALSO SUSPICIOUS:
  - JumpToWave got an addition in GunMettle, which is that it sets bool @ TFGameRules+0x500 to 1; this messes with state transitions in CTeamplayRoundBasedRules::State_Enter_PREROUND
  - 

*/

#if 0

notes from 20171007

multi-popfile-parse issues:
- tf_mvm_popfile calls CPopulationManager::ResetMap, but it's a redundant call because CPopulationManager::SetPopulationFilename already calls it
- CPopulationManager::ResetMap calls CPopulationManager::JumpToWave
  - JumpToWave calls CPopulationManager::Initialize, which calls CPopulationManager::Parse
  - JumpToWave calls TFGameRules()->State_Transition(GR_STATE_PREROUND)
    - which calls TFGameRules()->RoundRespawn
      - which calls TFGameRules()->SetupOnRoundStart
        - which calls CPopulationManager::Initialize, which calls CPopulationManager::Parse


HOW TO FIX:
- tf_mvm_popfile's call to CPopulationManager::ResetMap needs to be removed
- 

QUESTIONS:
- does this fix the "missing default popfile fucks up the state really badly" issue?

#endif

#if 0

backtraces from 20171007

Thread 1 "srcds_linux" hit Breakpoint 1, 0xf14a9256 in CPopulationManager::Parse() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#0  0xf14a9256 in CPopulationManager::Parse() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#1  0xf14ad7b7 in CPopulationManager::Initialize() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#2  0xf14ac203 in CPopulationManager::JumpToWave(unsigned int, float) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#3  0xf14ac4d6 in CPopulationManager::ResetMap() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#4  0xf14ac54a in CPopulationManager::SetPopulationFilename(char const*) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#5  0xf0e3119a in tf_mvm_popfile(CCommand const&) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#6  0xe899f454 in __SourceHook_FHCls_ConCommandDispatchfalse::Func (this=0xf1d9af60 <tf_mvm_popfile_command>, p1=...) at /pool/Game/SourceMod/sourcemod/core/GameHooks.cpp:45
#7  0xf5d325f7 in Cmd_ExecuteCommand(CCommand const&, cmd_source_t, int) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#8  0xf5d32702 in Cbuf_ExecuteCommand(CCommand const&, cmd_source_t) [clone .constprop.56] () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#9  0xf5d330a5 in Cbuf_Execute() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#10 0xf5e18f89 in CServerRemoteAccess::WriteDataRequest(CRConServer*, unsigned int, void const*, int) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#11 0xf5e15c5b in CRConServer::RunFrame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#12 0xf5dc4393 in NET_RunFrame(double) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#13 0xf5d7f55b in _Host_RunFrame(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#14 0xf5d8d477 in CHostState::State_Run(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#15 0xf5d8d7d6 in CHostState::FrameUpdate(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#16 0xf5d8d81d in HostState_Frame(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#17 0xf5e249f3 in CEngine::Frame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#18 0xf5e21896 in CDedicatedServerAPI::RunFrame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#19 0xf6b8d0fa in RunServer() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#20 0xf5e2198d in CModAppSystemGroup::Main() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#21 0xf5e6c528 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#22 0xf5e2267d in CDedicatedServerAPI::ModInit(ModInfo_t&) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#23 0xf6b8cda3 in CDedicatedAppSystemGroup::Main() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#24 0xf6c72e28 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#25 0xf6c72e28 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#26 0xf6b3c488 in main () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#27 0x080489cb in main ()

Thread 1 "srcds_linux" hit Breakpoint 1, 0xf14a9256 in CPopulationManager::Parse() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#0  0xf14a9256 in CPopulationManager::Parse() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#1  0xf14ad7b7 in CPopulationManager::Initialize() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#2  0xf0e648c9 in CTFGameRules::SetupOnRoundStart() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#3  0xf0ddd163 in CTeamplayRoundBasedRules::RoundRespawn() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#4  0xf0e58876 in CTFGameRules::RoundRespawn() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#5  0xf0dddc94 in CTeamplayRoundBasedRules::State_Enter_PREROUND() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#6  0xf14ac0f8 in CPopulationManager::JumpToWave(unsigned int, float) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#7  0xf14ac4d6 in CPopulationManager::ResetMap() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#8  0xf14ac54a in CPopulationManager::SetPopulationFilename(char const*) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#9  0xf0e3119a in tf_mvm_popfile(CCommand const&) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#10 0xe899f454 in __SourceHook_FHCls_ConCommandDispatchfalse::Func (this=0xf1d9af60 <tf_mvm_popfile_command>, p1=...) at /pool/Game/SourceMod/sourcemod/core/GameHooks.cpp:45
#11 0xf5d325f7 in Cmd_ExecuteCommand(CCommand const&, cmd_source_t, int) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#12 0xf5d32702 in Cbuf_ExecuteCommand(CCommand const&, cmd_source_t) [clone .constprop.56] () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#13 0xf5d330a5 in Cbuf_Execute() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#14 0xf5e18f89 in CServerRemoteAccess::WriteDataRequest(CRConServer*, unsigned int, void const*, int) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#15 0xf5e15c5b in CRConServer::RunFrame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#16 0xf5dc4393 in NET_RunFrame(double) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#17 0xf5d7f55b in _Host_RunFrame(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#18 0xf5d8d477 in CHostState::State_Run(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#19 0xf5d8d7d6 in CHostState::FrameUpdate(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#20 0xf5d8d81d in HostState_Frame(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#21 0xf5e249f3 in CEngine::Frame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#22 0xf5e21896 in CDedicatedServerAPI::RunFrame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#23 0xf6b8d0fa in RunServer() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#24 0xf5e2198d in CModAppSystemGroup::Main() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#25 0xf5e6c528 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#26 0xf5e2267d in CDedicatedServerAPI::ModInit(ModInfo_t&) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#27 0xf6b8cda3 in CDedicatedAppSystemGroup::Main() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#28 0xf6c72e28 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#29 0xf6c72e28 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#30 0xf6b3c488 in main () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#31 0x080489cb in main ()

Thread 1 "srcds_linux" hit Breakpoint 1, 0xf14a9256 in CPopulationManager::Parse() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#0  0xf14a9256 in CPopulationManager::Parse() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#1  0xf14ad7b7 in CPopulationManager::Initialize() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#2  0xf14ac203 in CPopulationManager::JumpToWave(unsigned int, float) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#3  0xf14ac4d6 in CPopulationManager::ResetMap() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#4  0xf0e311a7 in tf_mvm_popfile(CCommand const&) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#5  0xe899f454 in __SourceHook_FHCls_ConCommandDispatchfalse::Func (this=0xf1d9af60 <tf_mvm_popfile_command>, p1=...) at /pool/Game/SourceMod/sourcemod/core/GameHooks.cpp:45
#6  0xf5d325f7 in Cmd_ExecuteCommand(CCommand const&, cmd_source_t, int) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#7  0xf5d32702 in Cbuf_ExecuteCommand(CCommand const&, cmd_source_t) [clone .constprop.56] () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#8  0xf5d330a5 in Cbuf_Execute() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#9  0xf5e18f89 in CServerRemoteAccess::WriteDataRequest(CRConServer*, unsigned int, void const*, int) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#10 0xf5e15c5b in CRConServer::RunFrame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#11 0xf5dc4393 in NET_RunFrame(double) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#12 0xf5d7f55b in _Host_RunFrame(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#13 0xf5d8d477 in CHostState::State_Run(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#14 0xf5d8d7d6 in CHostState::FrameUpdate(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#15 0xf5d8d81d in HostState_Frame(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#16 0xf5e249f3 in CEngine::Frame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#17 0xf5e21896 in CDedicatedServerAPI::RunFrame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#18 0xf6b8d0fa in RunServer() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#19 0xf5e2198d in CModAppSystemGroup::Main() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#20 0xf5e6c528 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#21 0xf5e2267d in CDedicatedServerAPI::ModInit(ModInfo_t&) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#22 0xf6b8cda3 in CDedicatedAppSystemGroup::Main() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#23 0xf6c72e28 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#24 0xf6c72e28 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#25 0xf6b3c488 in main () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#26 0x080489cb in main ()

Thread 1 "srcds_linux" hit Breakpoint 1, 0xf14a9256 in CPopulationManager::Parse() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#0  0xf14a9256 in CPopulationManager::Parse() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#1  0xf14ad7b7 in CPopulationManager::Initialize() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#2  0xf0e648c9 in CTFGameRules::SetupOnRoundStart() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#3  0xf0ddd163 in CTeamplayRoundBasedRules::RoundRespawn() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#4  0xf0e58876 in CTFGameRules::RoundRespawn() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#5  0xf0dddc94 in CTeamplayRoundBasedRules::State_Enter_PREROUND() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#6  0xf14ac0f8 in CPopulationManager::JumpToWave(unsigned int, float) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#7  0xf14ac4d6 in CPopulationManager::ResetMap() () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#8  0xf0e311a7 in tf_mvm_popfile(CCommand const&) () from /pool/Game/SteamCMD/tf2/tf/bin/server_srv.so
#9  0xe899f454 in __SourceHook_FHCls_ConCommandDispatchfalse::Func (this=0xf1d9af60 <tf_mvm_popfile_command>, p1=...) at /pool/Game/SourceMod/sourcemod/core/GameHooks.cpp:45
#10 0xf5d325f7 in Cmd_ExecuteCommand(CCommand const&, cmd_source_t, int) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#11 0xf5d32702 in Cbuf_ExecuteCommand(CCommand const&, cmd_source_t) [clone .constprop.56] () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#12 0xf5d330a5 in Cbuf_Execute() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#13 0xf5e18f89 in CServerRemoteAccess::WriteDataRequest(CRConServer*, unsigned int, void const*, int) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#14 0xf5e15c5b in CRConServer::RunFrame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#15 0xf5dc4393 in NET_RunFrame(double) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#16 0xf5d7f55b in _Host_RunFrame(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#17 0xf5d8d477 in CHostState::State_Run(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#18 0xf5d8d7d6 in CHostState::FrameUpdate(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#19 0xf5d8d81d in HostState_Frame(float) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#20 0xf5e249f3 in CEngine::Frame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#21 0xf5e21896 in CDedicatedServerAPI::RunFrame() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#22 0xf6b8d0fa in RunServer() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#23 0xf5e2198d in CModAppSystemGroup::Main() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#24 0xf5e6c528 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#25 0xf5e2267d in CDedicatedServerAPI::ModInit(ModInfo_t&) () from /pool/Game/SteamCMD/tf2/bin/engine_srv.so
#26 0xf6b8cda3 in CDedicatedAppSystemGroup::Main() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#27 0xf6c72e28 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#28 0xf6c72e28 in CAppSystemGroup::Run() () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#29 0xf6b3c488 in main () from /pool/Game/SteamCMD/tf2/bin/dedicated_srv.so
#30 0x080489cb in main ()


#endif
