#include "extension.h"
#include "library.h"
#include "link/link.h"
#include "mem/alloc.h"
#include "mod.h"
#include "addr/addr.h"
#include "addr/prescan.h"
#include "stub/baseplayer.h"
#include "gameconf.h"
#include "prop.h"
#include "util/pooled_string.h"
#include "util/rtti.h"
//#include "disasm/disasm.h"
#include "factory.h"
#include "concolor.h"
#ifdef SE_IS_TF2
#include "re/nextbot.h"
#endif
#include "version.h"
#include "convar_restore.h"
#include "vscript/ivscript.h"
//#include "entity.h"

CExtSigsegv g_Ext;
SMEXT_LINK(&g_Ext);

IFileSystem *filesystem                          = nullptr;
IServerGameClients *serverGameClients            = nullptr;
IServerGameEnts *serverGameEnts                  = nullptr;
ICvar *icvar                                     = nullptr;
IServer *sv                                      = nullptr;
ISpatialPartition *partition                     = nullptr;
IEngineTrace *enginetrace                        = nullptr;
IStaticPropMgrServer *staticpropmgr              = nullptr;
IGameEventManager2 *gameeventmanager             = nullptr;
INetworkStringTableContainer *networkstringtable = nullptr;
IEngineSound *enginesound                        = nullptr;
IVModelInfo *modelinfo                           = nullptr;
IVDebugOverlay *debugoverlay                     = nullptr;

IPlayerInfoManager *playerinfomanager = nullptr;
IBotManager *botmanager               = nullptr;

IPhysics *physics                = nullptr;
IPhysicsCollision *physcollision = nullptr;
IPhysicsSurfaceProps *physprops  = nullptr;

ISoundEmitterSystemBase *soundemitterbase = nullptr;

IMaterialSystem *g_pMaterialSystem = nullptr;

vgui::IVGui *g_pVGui                       = nullptr;
vgui::IInput *g_pVGuiInput                 = nullptr;
vgui::IPanel *g_pVGuiPanel                 = nullptr;
vgui::ISchemeManager *g_pVGuiSchemeManager = nullptr;
vgui::ISystem *g_pVGuiSystem               = nullptr;
vgui::ILocalize *g_pVGuiLocalize           = nullptr;
vgui::IInputInternal *g_pVGuiInputInternal = nullptr;

vgui::ISurface *g_pVGuiSurface         = nullptr;
IMatSystemSurface *g_pMatSystemSurface = nullptr;

CGlobalVars *gpGlobals         = nullptr;
CGlobalEntityList *gEntList    = nullptr;
CBaseEntityList *g_pEntityList = nullptr;

IVEngineClient *engineclient     = nullptr;
IBaseClientDLL *clientdll        = nullptr;
IClientEntityList *cl_entitylist = nullptr;

IEngineTool *enginetools  = nullptr;
IServerTools *servertools = nullptr;
IClientTools *clienttools = nullptr;
IGameMovement *g_pGameMovement = nullptr;

IVProfExport *vprofexport = nullptr;

IDedicatedExports *dedicated = nullptr;

IMDLCache *mdlcache = nullptr;

IClientMode *g_pClientMode = nullptr;

IPhraseCollection *phrases = nullptr;
IPhraseFile *phrasesFile = nullptr;
IPhraseCollection *phrasesAttribs = nullptr;
IPhraseFile *phrasesAttribsFile = nullptr;

IScriptManager *scriptManager = nullptr;

extern int laserSprite;
bool CExtSigsegv::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
#ifndef OPTIMIZE_MODS_ONLY
	ColorSpew::Enable();
#endif
	
	if (gameeventmanager != nullptr) {
		gameeventmanager->LoadEventsFromFile("resource/sigsegv_events.res");
	}
	
	this->LoadSoundOverrides();
	
	LibMgr::SetPtr(Library::SOURCEMODCORE,      menus);
	PreScan::DoScans();
	if (!g_GCHook.LoadAll(error, maxlength)) goto fail;
	
	LibMgr::Load();
//	g_Disasm.Load();
	
	RTTI::PreLoad();
	AddrManager::Load();
	
	g_pClientMode = reinterpret_cast<IClientMode *>(AddrManager::GetAddr("g_pClientMode"));
	
	if (!Link::InitAll()) goto fail;
	
	Prop::PreloadAll();
	
	g_pWorldEdict = engine->PEntityOfEntIndex(0);
	IExecMemManager::Load();
	g_ModManager.Load();
	
	IGameSystem::Add(this);

	ConVar_Restore::Load();
	ConVar_Restore::OnExtLoad();

	phrases = translator->CreatePhraseCollection();
	phrasesFile = phrases->AddPhraseFile("sigsegv.phrases");

	phrasesAttribs = translator->CreatePhraseCollection();
	phrasesAttribsFile = phrases->AddPhraseFile("sigsegvattributes.phrases");

	identity = sharesys->CreateIdentity(sharesys->CreateIdentType("Sigsegv"), this);
	
	laserSprite = CBaseEntity::PrecacheModel("materials/sprites/laser.vmt");
//	for (int i = 0; i < 255; ++i) {
//		ConColorMsg(Color(0xff, i, 0x00), "%02x%02x%02x\n", 0xff, i, 0x00);
//	}
	
	return true;
	
fail:
	g_GCHook.UnloadAll();
	return false;
}

void CExtSigsegv::SDK_OnUnload()
{
	ConVar_Restore::OnExtUnload();

	IGameSystem::Remove(this);
	
#ifdef SE_IS_TF2
	IHotplugActionBase::UnloadAll();
//	IHotplugEntity::UninstallAll();
#endif
	
	g_ModManager.Unload();
	CDetouredFunc::CleanUp();
	IExecMemManager::Unload();
	
	LibMgr::Unload();
	
	g_GCHook.UnloadAll();

#ifndef OPTIMIZE_MODS_ONLY
	ColorSpew::Disable();
#endif

	UnloadAllCustomThinkFunc();
	if (phrases != nullptr) {
		phrases->Destroy();
	}
}

void CExtSigsegv::SDK_OnAllLoaded()
{
}

bool CExtSigsegv::QueryRunning(char *error, size_t maxlength)
{
	return true;
}


#define GET_IFACE_OPTIONAL(factory, var, name) \
	var = reinterpret_cast<decltype(var)>(ismm->VInterfaceMatch(factory##Factory(), name, -1)); \
	if (var == nullptr) { \
		DevWarning("Could not find optional interface: %s\n", name); \
	}

#define GET_IFACE_REQUIRED(factory, var, name) \
	var = reinterpret_cast<decltype(var)>(ismm->VInterfaceMatch(factory##Factory(), name, -1)); \
	if (var == nullptr) { \
		if (error != nullptr && maxlength != 0) { \
			ismm->Format(error, maxlength, "Could not find required interface: %s", name); \
		} \
		return false; \
	}


bool CExtSigsegv::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late)
{
	Msg("CExtSigsegv: compiled @ %s %s\n", GetBuildDate(), GetBuildTime());
	
	GET_IFACE_REQUIRED(Engine,     engine,            INTERFACEVERSION_VENGINESERVER);
#if SOURCE_ENGINE != SE_L4D
	GET_IFACE_REQUIRED(Server,     gamedll,           INTERFACEVERSION_SERVERGAMEDLL);
#else
	GET_IFACE_REQUIRED(Server,     gamedll,           "ServerGameDLL005");
#endif
	GET_IFACE_REQUIRED(FileSystem, filesystem,        FILESYSTEM_INTERFACE_VERSION);
	GET_IFACE_REQUIRED(Server,     serverGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_IFACE_REQUIRED(Server,     serverGameEnts,    INTERFACEVERSION_SERVERGAMEENTS);
	
	GET_IFACE_REQUIRED(Engine, icvar,              CVAR_INTERFACE_VERSION);
	GET_IFACE_REQUIRED(Engine, partition,          INTERFACEVERSION_SPATIALPARTITION);
	GET_IFACE_REQUIRED(Engine, enginetrace,        INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_IFACE_REQUIRED(Engine, staticpropmgr,      INTERFACEVERSION_STATICPROPMGR_SERVER);
	GET_IFACE_REQUIRED(Engine, gameeventmanager,   INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_IFACE_REQUIRED(Engine, networkstringtable, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	GET_IFACE_REQUIRED(Engine, enginesound,        IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_IFACE_REQUIRED(Engine, modelinfo,          VMODELINFO_SERVER_INTERFACE_VERSION);
	
	GET_IFACE_REQUIRED(Server, playerinfomanager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_IFACE_REQUIRED(Server, botmanager,        INTERFACEVERSION_PLAYERBOTMANAGER);
	GET_IFACE_REQUIRED(Server, servertools,       VSERVERTOOLS_INTERFACE_VERSION);
	GET_IFACE_REQUIRED(Server, g_pGameMovement,   INTERFACENAME_GAMEMOVEMENT);
	
	GET_IFACE_REQUIRED(VPhysics, physics,       VPHYSICS_INTERFACE_VERSION);
	GET_IFACE_REQUIRED(VPhysics, physcollision, VPHYSICS_COLLISION_INTERFACE_VERSION);
	GET_IFACE_REQUIRED(VPhysics, physprops,     VPHYSICS_SURFACEPROPS_INTERFACE_VERSION);
	
	GET_IFACE_OPTIONAL(Engine, debugoverlay, VDEBUG_OVERLAY_INTERFACE_VERSION);
	GET_IFACE_OPTIONAL(Engine, enginetools,  VENGINETOOL_INTERFACE_VERSION);

	if (VScriptManagerFactory() != nullptr) {
		GET_IFACE_OPTIONAL(VScriptManager, scriptManager,  VSCRIPT_INTERFACE_VERSION);
	}
	
	if (SoundEmitterSystemFactory() != nullptr) {
		GET_IFACE_OPTIONAL(SoundEmitterSystem, soundemitterbase, SOUNDEMITTERSYSTEM_INTERFACE_VERSION);
	}
	
	if (MaterialSystemFactory() != nullptr) {
		GET_IFACE_OPTIONAL(MaterialSystem, g_pMaterialSystem, MATERIAL_SYSTEM_INTERFACE_VERSION);
	}
	if (VGUIFactory() != nullptr) {
		GET_IFACE_OPTIONAL(VGUI, g_pVGui,              VGUI_IVGUI_INTERFACE_VERSION);
		GET_IFACE_OPTIONAL(VGUI, g_pVGuiInput,         VGUI_INPUT_INTERFACE_VERSION);
		GET_IFACE_OPTIONAL(VGUI, g_pVGuiPanel,         VGUI_PANEL_INTERFACE_VERSION);
		GET_IFACE_OPTIONAL(VGUI, g_pVGuiSchemeManager, VGUI_SCHEME_INTERFACE_VERSION);
		GET_IFACE_OPTIONAL(VGUI, g_pVGuiSystem,        VGUI_SYSTEM_INTERFACE_VERSION);
		GET_IFACE_OPTIONAL(VGUI, g_pVGuiLocalize,      VGUI_LOCALIZE_INTERFACE_VERSION);
		GET_IFACE_OPTIONAL(VGUI, g_pVGuiInputInternal, VGUI_INPUTINTERNAL_INTERFACE_VERSION);
	}
	
	if (VGUIMatSurfaceFactory() != nullptr) {
		GET_IFACE_OPTIONAL(VGUIMatSurface, g_pVGuiSurface,      VGUI_SURFACE_INTERFACE_VERSION);
		GET_IFACE_OPTIONAL(VGUIMatSurface, g_pMatSystemSurface, MAT_SYSTEM_SURFACE_INTERFACE_VERSION);
	}
	
	if (ClientFactory() != nullptr) {
		GET_IFACE_REQUIRED(Engine, engineclient,  VENGINE_CLIENT_INTERFACE_VERSION);
		GET_IFACE_REQUIRED(Client, clientdll,     CLIENT_DLL_INTERFACE_VERSION);
		GET_IFACE_REQUIRED(Client, cl_entitylist, VCLIENTENTITYLIST_INTERFACE_VERSION);
		GET_IFACE_OPTIONAL(Client, clienttools,   VCLIENTTOOLS_INTERFACE_VERSION);
	}
	
	if (DedicatedFactory() != nullptr) {
		GET_IFACE_OPTIONAL(Dedicated, dedicated, VENGINE_DEDICATEDEXPORTS_API_VERSION);
	}
	
	if (DataCacheFactory() != nullptr) {
		GET_IFACE_OPTIONAL(DataCache, mdlcache, MDLCACHE_INTERFACE_VERSION);
	}
	
	GET_IFACE_REQUIRED(Engine, vprofexport, "VProfExport001");
	
	sv = engine->GetIServer();
	
	g_pCVar = icvar;
	ConVar_Register(0, this);
	
	gpGlobals     = ismm->GetCGlobals();
	gEntList      = servertools->GetEntityList();
	g_pEntityList = gEntList;
	
	LibMgr::SetPtr(Library::THIS,               this);
	LibMgr::SetPtr(Library::SERVER,             ServerFactory());
	LibMgr::SetPtr(Library::ENGINE,             EngineFactory());
	LibMgr::SetPtr(Library::DEDICATED,          DedicatedFactory());
	LibMgr::SetPtr(Library::TIER0,              &MemAllocScratch);
	LibMgr::SetPtr(Library::CLIENT,             ClientFactory());
	LibMgr::SetPtr(Library::VGUIMATSURFACE,     VGUIMatSurfaceFactory());
	LibMgr::SetPtr(Library::MATERIALSYSTEM,     MaterialSystemFactory());
	LibMgr::SetPtr(Library::SOUNDEMITTERSYSTEM, SoundEmitterSystemFactory());
	LibMgr::SetPtr(Library::DATACACHE,          DataCacheFactory());
	LibMgr::SetPtr(Library::VGUI,               VGUIFactory());
	LibMgr::SetPtr(Library::VPHYSICS,           VPhysicsFactory());
	LibMgr::SetPtr(Library::VSTDLIB,            icvar);
	LibMgr::SetPtr(Library::VSCRIPT,            VScriptManagerFactory());
	
	return true;
}

bool CExtSigsegv::SDK_OnMetamodUnload(char *error, size_t maxlength)
{
	return true;
}


bool CExtSigsegv::RegisterConCommandBase(ConCommandBase *pCommand)
{
	// Save only new commands
	if (icvar->FindVar(pCommand->GetName()) == nullptr) {
		ConVar_Restore::Register(pCommand);
	}
	
	META_REGCVAR(pCommand);
	return true;
}


void CExtSigsegv::LevelInitPreEntity()
{
	this->LoadSoundOverrides();
}

void CExtSigsegv::LevelInitPostEntity()
{
	g_pWorldEdict = engine->PEntityOfEntIndex(0);
	laserSprite = CBaseEntity::PrecacheModel("materials/sprites/laser.vmt");
}

void CExtSigsegv::LoadSoundOverrides()
{
	if (soundemitterbase != nullptr) {
		soundemitterbase->AddSoundOverrides("scripts/sigsegv_sound_overrides.txt", true);
	}
}

IdentityToken_t *CExtSigsegv::GetIdentity() const
{
	return identity;
}

//ConVar cvar_build("sig_build", __DATE__ " " __TIME__, FCVAR_NONE);
CON_COMMAND(sig_build, "")
{
	Msg("%s %s\n", GetBuildDate(), GetBuildTime());
}

#ifndef SE_L4D
CON_COMMAND(sig_cpu_usage, "")
{
	Msg("%f\n", GetCPUUsage());
}
#endif

CON_COMMAND(sig_memory_stats, "")
{
    char buffer[1024] = "";

    FILE* file = fopen("/proc/self/status", "r");

	while (fscanf(file, " %1023s", buffer) == 1) {

        if (strcmp(buffer, "VmRSS:") == 0) {
            fscanf(file, " %s", buffer);
			Msg("Current Real Memory: %s KB\n", buffer);
        }
        if (strcmp(buffer, "VmHWM:") == 0) {
            fscanf(file, " %s", buffer);
			Msg("Peak Real Memory: %s KB\n", buffer);
        }
        if (strcmp(buffer, "VmSize:") == 0) {
            fscanf(file, " %s", buffer);
			Msg("Current Virtual Memory: %s KB\n", buffer);
        }
        if (strcmp(buffer, "VmPeak:") == 0) {
            fscanf(file, " %s", buffer);
			Msg("Peak Virtual Memory: %s KB\n", buffer);
        }
    }
}