#include "mod.h"
#include "stub/baseentity.h"
#include "stub/populators.h"
#include "util/iterate.h"


// Gun Mettle MvM Breakage Timeline
// 
// 20150624a
// 20150702a
// - Gun Mettle Update
// 20150703a
// - Patch Note: Players no longer drop their weapons in MvM
// 20150704a
// 20150704b
// 20150704c
// 20150707a
// 20150707b
// - Patch Note: Fixed a case where weapons could still be dropped in Mann vs. Machine
// 20150709a
// - Patch Note: Vaccinator Über bubbles once again work on Reanimators in Mann vs Machine, but now instantly fills them by 90%
// 20150709b
// 20150715a
// 20150716a
// 20150716b
// 20150724a
// - Patch Note: Fixed tf_bot_join_after_player preventing the bots from joining a team after the player joins team Spectator
// - Patch Note: Fixed Mann vs. Machine maps not resetting properly after a successful vote to change the challenge
// 20150724b


// okay so here's what's specifically happening:
// - given server_srv.so base addr 0xf091d000
// - here's the meaningful part of the call stack:
//     0xf0fa95c3 in CBaseEntity::SetNextThink(float, char const*)
//     0xf0fa9679 in CBaseEntity::SetNextThink(int, float)
//     0xf1057c6b in CBaseEntity::PhysicsRunSpecificThink(int, void (CBaseEntity::*)())
//     0xf1058002 in CBaseEntity::PhysicsRunThink(CBaseEntity::thinkmethods_t) [clone .part.442]
//     0xf1545a28 in Physics_SimulateEntity(CBaseEntity*)
//     0xf1545d51 in Physics_RunThinkFunctions(bool)
//     0xf146d8a4 in CServerGameDLL::GameFrame(bool)
// - in CBaseEntity::SetNextThink(float, char const*),
//   the vcall to CBaseEntity::NetworkStateChanged_m_nNextThinkTick(void *) [+0xC8] is jumping to wherever-the-fuck
// - this happens BEFORE the actual call to CBaseEntity::PhysicsDispatchThink!
// - based on postmortem stack frame analysis:
//   - the entity ptr (this) was a valid ptr to what appeared to be an already-destructed entity
//     - the vtable ptr was set to `vtable for'IHandleEntity
//     - (NOTE: m_Network @ CBaseEntity+0x14)
//     - [+0x020] m_Network.m_pPev:                            nullptr
//     - [+0x044] m_Network.m_pServerClass:                    0xf209bce0 [TODO: .bss:0177ECE0]
//     - [+0x048] m_Network.m_hParent:                         -1
//     - [+0x064] m_iClassname:                                "func_nav_avoid" <-- !!!!!
//     - [+0x068] m_iGlobalname:                               nullptr
//     - [+0x06C] m_iParent:                                   nullptr
//     - [+0x098] m_flSimulationTime:                          0.0f
//     - [+0x0A0] m_nLastThinkTick:                            820524
//     - [+0x0AC] m_aThinkFunctions [CUtlVector<thinkfunc_t>]: <empty>
//     - [+0x0D8] m_nNextThinkTick:                            820537
//     - [+0x120] m_nSimulationTick:                           820524
//     - [+0x124] m_spawnflags:                                0
//     - [+0x128] m_iEFlags:                                   0x00840001 [TODO]
//     - [+0x12C] m_fFlags:                                    0x08000000 [TODO]
//     - [+0x130] m_iName:                                     nullptr
//     - [+0x15C] m_hParent:                                   -1
//     - [+0x344] m_RefEHandle:                                -1
//   - in the attempted call to CBaseEntity::NetworkStateChanged_m_nNextThinkTick:
//     - pVar properly pointed to this->m_nNextThinkTick
//   - in the call to CBaseEntity::SetNextThink(float, char const*):
//     - thinkTime was -1.0f
//     - szContext was nullptr
//   - in the call to CBaseEntity::SetNextThink(int, float):
//     - nContextIndex was -1
//     - thinkTime was -1.0f
//   - TODO: look at the args to PhysicsRunSpecificThink
//     - 
//   - TODO: look at the args to PhysicsRunThink
//     - 



// TODO: at start of CMannVsMachineLogic::CMannVsMachineLogic:
// - log w/ timestamp, tick count, etc
// - increment a global refcount; assert(refcount == 1)

// TODO: at start of CMannVsMachineLogic::~CMannVsMachineLogic:
// - log w/ timestamp, tick count, etc
// - decrement a global refcount; assert(refcount == 0)


// TODO: at start of CPopulationManager::CPopulationManager:
// - log w/ timestamp, tick count, etc
// - assert(g_pPopulationManager == nullptr)
// - increment a global refcount; assert(refcount == 1)

// TODO: at start of CPopulationManager::~CPopulationManager:
// - log w/ timestamp, tick count, etc
// - assert(g_pPopulationManager != nullptr)
// - assert(g_pPopulationManager == this)
// - decrement a global refcount; assert(refcount == 0)


// TODO: at start of UTIL_RemoveImmediate:
// - for oldObj nonnull, valid, and classname/rtti_cast 'info_populator'/CPopulationManager:
//   - log whether UTIL_RemoveImmediate is actually allowed presently: (s_RemoveImmediateSemaphore == 0)


// TODO: at start of CBaseEntity::PhysicsRunSpecificThink:
// - for classname/rtti_cast 'info_populator'/CPopulationManager or 'tf_logic_mann_vs_machine'/CMannVsMachineLogic:
//   - verify !this->IsMarkedForDeletion()


// TODO: at start of CBaseEntity::PhysicsDispatchThink:
// - for this nonnull, and classname/rtti_cast 'info_populator'/CPopulationManager or 'tf_logic_mann_vs_machine'/CMannVsMachineLogic:
//   - 


namespace Mod::Etc::Debug_Mission_Change_Crash
{
	// same as CGlobalEntityList::IsEntityPtr
	bool IsEntityPtr(void *ptr)
	{
		if (ptr == nullptr) return false;
		
		for (const CEntInfo *info = g_pEntityList->FirstEntInfo(); info != nullptr; info = info->m_pNext) {
			if (reinterpret_cast<void *>(info->m_pEntity) == ptr) {
				return true;
			}
		}
		
		return false;
	}
	
	// like CBaseEntityList::GetEntInfoIndex
	int GetEntityIndexOfPtr(void *ptr)
	{
		if (ptr == nullptr) return -1;
		
		for (const CEntInfo *info = g_pEntityList->FirstEntInfo(); info != nullptr; info = info->m_pNext) {
			if (reinterpret_cast<void *>(info->m_pEntity) == ptr) {
				return (info - g_pEntityList->GetEntInfoPtrByIndex(0));
			}
		}
		
		return -1;
	}
	
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_SetNextThink_index, int nContextIndex, float thinkTime)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (ent == nullptr) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(index)] ENTER: this == nullptr (ctx %d, next @ %.3f, reltime %.3f)\n", nContextIndex, thinkTime, thinkTime - gpGlobals->curtime);
		} else if (!IsEntityPtr(ent)) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(index)] ENTER: !IsEntityPtr(this) (ent @ 0x%08x, ctx %d, next @ %.3f, reltime %.3f)\n", (uintptr_t)ent, nContextIndex, thinkTime, thinkTime - gpGlobals->curtime);
		} else if (*reinterpret_cast<void **>(ent) == nullptr) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(index)] ENTER: vtptr == nullptr (ent @ 0x%08x, ent #%d, ctx %d, next @ %.3f, reltime %.3f)\n", (uintptr_t)ent, GetEntityIndexOfPtr(ent), nContextIndex, thinkTime, thinkTime - gpGlobals->curtime);
		}
		
		DETOUR_MEMBER_CALL(nContextIndex, thinkTime);
		
		if (ent == nullptr || *reinterpret_cast<void **>(ent) == nullptr) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(index)] LEAVE\n");
		}
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_SetNextThink_name, float nextThinkTime, const char *szContext)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (ent == nullptr) {
			Msg("[CBaseEntity::SetNextThink(name)] ENTER: this == nullptr (ctx \"%s\", next @ %.3f, reltime %.3f)\n", szContext, nextThinkTime, nextThinkTime - gpGlobals->curtime);
			BACKTRACE();
		} else if (!IsEntityPtr(ent)) {
			Msg("[CBaseEntity::SetNextThink(name)] ENTER: !IsEntityPtr(this) (ent @ 0x%08x, ctx \"%s\", next @ %.3f, reltime %.3f)\n", (uintptr_t)ent, szContext, nextThinkTime, nextThinkTime - gpGlobals->curtime);
			BACKTRACE();
		} else if (*reinterpret_cast<void **>(ent) == nullptr) {
			Msg("[CBaseEntity::SetNextThink(name)] ENTER: vtptr == nullptr (ent @ 0x%08x, ent #%d, ctx \"%s\", next @ %.3f, reltime %.3f)\n", (uintptr_t)ent, GetEntityIndexOfPtr(ent), szContext, nextThinkTime, nextThinkTime - gpGlobals->curtime);
			BACKTRACE();
		}
		
		DETOUR_MEMBER_CALL(nextThinkTime, szContext);
		
		if (ent == nullptr || *reinterpret_cast<void **>(ent) == nullptr) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(name)] LEAVE\n");
		}
	}
	
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_C1)
	{
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);
		
		ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[CPopulationManager C1] ENTER: ent @ 0x%08x\n", (uintptr_t)popmgr);
		DETOUR_MEMBER_CALL(CPopulationManager_C1)();
		ConColorMsg(Color(0xff, 0x00, 0x00, 0xff), "[CPopulationManager C1] LEAVE: ent @ 0x%08x, ent #%d\n", (uintptr_t)popmgr, ENTINDEX(popmgr));
	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_D0)
	{
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);
		
		ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[CPopulationManager D0] ENTER: ent @ 0x%08x, ent #%d\n", (uintptr_t)popmgr, ENTINDEX(popmgr));
		DETOUR_MEMBER_CALL(CPopulationManager_D0)();
		ConColorMsg(Color(0xff, 0x00, 0x00, 0xff), "[CPopulationManager D0] LEAVE: ent @ 0x%08x\n", (uintptr_t)popmgr);
	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_D2)
	{
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);
		
		ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[CPopulationManager D2] ENTER: ent @ 0x%08x, ent #%d\n", (uintptr_t)popmgr, ENTINDEX(popmgr));
		DETOUR_MEMBER_CALL(CPopulationManager_D2)();
		ConColorMsg(Color(0xff, 0x00, 0x00, 0xff), "[CPopulationManager D2] LEAVE: ent @ 0x%08x\n", (uintptr_t)popmgr);
	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_Update)
	{
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);
		
		ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[CPopulationManager Update] ENTER: ent @ 0x%08x, ent #%d\n", (uintptr_t)popmgr, ENTINDEX(popmgr));
		DETOUR_MEMBER_CALL(CPopulationManager_Update)();
		ConColorMsg(Color(0xff, 0x00, 0x00, 0xff), "[CPopulationManager Update] LEAVE: ent @ 0x%08x, ent #%d\n", (uintptr_t)popmgr, ENTINDEX(popmgr));
	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_Spawn)
	{
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);
		
		ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "[CPopulationManager Spawn] ENTER: ent @ 0x%08x, ent #%d\n", (uintptr_t)popmgr, ENTINDEX(popmgr));
		DETOUR_MEMBER_CALL(CPopulationManager_Spawn)();
		ConColorMsg(Color(0xff, 0x00, 0x00, 0xff), "[CPopulationManager Spawn] LEAVE: ent @ 0x%08x, ent #%d\n", (uintptr_t)popmgr, ENTINDEX(popmgr));
	}
	
	
	struct PostMortemData1
	{
		int dirty = 0;
		CBaseEntity *entptr = nullptr;
		int entidx1 = -1;
		int entidx2 = -1;
		char classname[1024];
	};
	PostMortemData1 pmd1;
	
	DETOUR_DECL_STATIC(void, Physics_SimulateEntity, CBaseEntity *pEntity)
	{
		pmd1.dirty = -1;
		{
			pmd1.entptr  = pEntity;
		//	pmd1.entidx1 = ENTINDEX(pEntity);
			pmd1.entidx2 = GetEntityIndexOfPtr(pEntity);
			V_strcpy_safe(pmd1.classname, pEntity->GetClassname());
		}
		pmd1.dirty = 0;
		
		DETOUR_STATIC_CALL(Physics_SimulateEntity)(pEntity);
	}
	
	
	struct PostMortemData2
	{
		int dirty = 0;
		CBaseEntity *entptr = nullptr;
		int ctxidx = -2;
		BASEPTR thinkfunc = nullptr;
		int entidx1 = -1;
		int entidx2 = -1;
		char classname[1024];
	};
	PostMortemData2 pmd2;
	
	
	DETOUR_DECL_MEMBER(bool, CBaseEntity_PhysicsRunSpecificThink, int nContextIndex, BASEPTR thinkFunc)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		pmd2.dirty = -1;
		{
			pmd2.entptr    = ent;
			pmd2.ctxidx    = nContextIndex;
			pmd2.thinkfunc = thinkFunc;
		//	pmd2.entidx1   = ENTINDEX(ent);
			pmd2.entidx2   = GetEntityIndexOfPtr(ent);
			V_strcpy_safe(pmd2.classname, ent->GetClassname());
		}
		pmd2.dirty = 0;
		
		return DETOUR_MEMBER_CALL(CBaseEntity_PhysicsRunSpecificThink)(nContextIndex, thinkFunc);
	}
	
	
	DETOUR_DECL_MEMBER(bool, CPopulationManager_Initialize)
	{
		BACKTRACE();
		return DETOUR_MEMBER_CALL(CPopulationManager_Initialize)();
	}
	
	
	// pretty sure something is getting mangled inside of CPopulationManager::Update
	// we may want to do some logging of stuff happening inside of calls to Update
	
	// FINDING: CPopulationManager does think setup stuff in its ctor...
	// shouldn't it be doing that in its Spawn member func???
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Debug_Mission_Change_Crash")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetNextThink_index, "CBaseEntity::SetNextThink [index]");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetNextThink_name,  "CBaseEntity::SetNextThink [name]");
			
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_C1,     "CPopulationManager::CPopulationManager [C1]");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_D0,     "CPopulationManager::~CPopulationManager [D0]");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_D2,     "CPopulationManager::~CPopulationManager [D2]");
		//	MOD_ADD_DETOUR_MEMBER(CPopulationManager_Update, "CPopulationManager::Update");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Spawn,  "CPopulationManager::Spawn");
			
			MOD_ADD_DETOUR_STATIC(Physics_SimulateEntity, "Physics_SimulateEntity");
			
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PhysicsRunSpecificThink, "CBaseEntity::PhysicsRunSpecificThink");
			
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Initialize, "CPopulationManager::Initialize");
		}
		
		virtual void OnEnable() override
		{
			Msg("PostMortemData1 @ 0x%08x\n", (uintptr_t)&pmd1);
			Msg("PostMortemData2 @ 0x%08x\n", (uintptr_t)&pmd2);
			
			ForEachEntityByRTTI<CPopulationManager>([](CPopulationManager *popmgr){
				Msg("CPopulationManager #%d @ 0x%08x\n", ENTINDEX(popmgr), (uintptr_t)popmgr);
			});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_debug_mission_change_crash", "0", FCVAR_NOTIFY,
		"Mod: debug crash that occurs in CBaseEntity::SetNextThink when an MvM mission ends and the next one starts",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

/*
#0  0xf1026219 in CCollisionProperty::TestCollision(Ray_t const&, unsigned int, CGameTrace&) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#1  0xf100bb43 in CBaseEntity::SetNextThink(float, char const*) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#2  0xf100bbf9 in CBaseEntity::SetNextThink(int, float) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#3  0xf10d864b in CBaseEntity::PhysicsRunSpecificThink(int, void (CBaseEntity::*)()) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#4  0xf10d8b82 in CBaseEntity::PhysicsRunThink(CBaseEntity::thinkmethods_t) [clone .part.436] () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#5  0xf16066e8 in Physics_SimulateEntity(CBaseEntity*) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#6  0xf1606a11 in Physics_RunThinkFunctions(bool) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#7  0xf1529474 in CServerGameDLL::GameFrame(bool) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#8  0xeba31aca in __SourceHook_FHCls_IServerGameDLLGameFramefalse::Func (this=0xf21965fc <g_ServerGameDLL>, p1=true) at /mnt/tmp/sm-build/build-20170507/sourcemod/core/sourcemod.cpp:54
#9  0xf6612fcf in SV_Think(bool) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#10 0xf6614068 in SV_Frame(bool) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#11 0xf6584598 in _Host_RunFrame_Server(bool) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#12 0xf6585a67 in _Host_RunFrame(float) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#13 0xf6593d87 in CHostState::State_Run(float) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#14 0xf65940e6 in CHostState::FrameUpdate(float) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#15 0xf659412d in HostState_Frame(float) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#16 0xf662b4d3 in CEngine::Frame() () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#17 0xf6628376 in CDedicatedServerAPI::RunFrame() () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#18 0xf740c5ea in ?? ()
#19 0xf662846d in CModAppSystemGroup::Main() () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#20 0xf6673108 in CAppSystemGroup::Run() () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#21 0xf662915d in CDedicatedServerAPI::ModInit(ModInfo_t&) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#22 0xf740c293 in ?? ()
#23 0xf74f2418 in ?? ()
#24 0xf74f2418 in ?? ()
#25 0xf73bb968 in ?? ()
#26 0x080489cb in main ()

#0  0x00000000 in ?? ()
#1  0xf0f0bb43 in CBaseEntity::SetNextThink(float, char const*) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#2  0xe805c235 in Mod_Etc_Debug_Mission_Change_Crash::Detour_CBaseEntity_SetNextThink_name::CBaseEntity_SetNextThink_name(float, char const*) () at /pool/Game/SourceMod/sigsegv/src/mod/etc/debug_mission_change_crash.cpp:76
#3  0xf0f0bbf9 in CBaseEntity::SetNextThink(int, float) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#4  0xe805c14a in Mod_Etc_Debug_Mission_Change_Crash::Detour_CBaseEntity_SetNextThink_index::CBaseEntity_SetNextThink_index(int, float) () at /pool/Game/SourceMod/sigsegv/src/mod/etc/debug_mission_change_crash.cpp:53
#5  0xf0fd864b in CBaseEntity::PhysicsRunSpecificThink(int, void (CBaseEntity::*)()) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#6  0xf0fd8b82 in CBaseEntity::PhysicsRunThink(CBaseEntity::thinkmethods_t) [clone .part.436] () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#7  0xf15066e8 in Physics_SimulateEntity(CBaseEntity*) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#8  0xf1506a11 in Physics_RunThinkFunctions(bool) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#9  0xf1429474 in CServerGameDLL::GameFrame(bool) () from /opt/srcds/tf2_normal2/tf/bin/server_srv.so
#10 0xeb996aca in __SourceHook_FHCls_IServerGameDLLGameFramefalse::Func (this=0xf20965fc <g_ServerGameDLL>, p1=true) at /mnt/tmp/sm-build/build-20170507/sourcemod/core/sourcemod.cpp:54
#11 0xf6612fcf in SV_Think(bool) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#12 0xf6614068 in SV_Frame(bool) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#13 0xf6584598 in _Host_RunFrame_Server(bool) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#14 0xf6585a67 in _Host_RunFrame(float) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#15 0xf6593d87 in CHostState::State_Run(float) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#16 0xf65940e6 in CHostState::FrameUpdate(float) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#17 0xf659412d in HostState_Frame(float) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#18 0xf662b4d3 in CEngine::Frame() () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#19 0xf6628376 in CDedicatedServerAPI::RunFrame() () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#20 0xf737f5ea in ?? ()
#21 0xf662846d in CModAppSystemGroup::Main() () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#22 0xf6673108 in CAppSystemGroup::Run() () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#23 0xf662915d in CDedicatedServerAPI::ModInit(ModInfo_t&) () from /opt/srcds/tf2_normal2/bin/engine_srv.so
#24 0xf737f293 in ?? ()
#25 0xf7465418 in ?? ()
#26 0xf7465418 in ?? ()
#27 0xf732e968 in ?? ()
#28 0x080489cb in main ()
*/
