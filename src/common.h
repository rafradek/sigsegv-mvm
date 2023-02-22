/* precompiled header! */

#ifndef _INCLUDE_SIGSEGV_COMMON_H_
#define _INCLUDE_SIGSEGV_COMMON_H_


#if !defined __clang__ && !defined __GNUC__ && !defined _MSC_VER
	#error WTF compiler are you using???
#endif


#if defined __cplusplus


#if defined __GNUC__ || defined __clang__
	#define PRAGMA(str) _Pragma(#str)
#elif defined _MSC_VER
	#define PRAGMA(str) __pragma(str)
#endif

#if defined __clang__
	#define WARN_RESTORE() PRAGMA(clang diagnostic pop)
#elif defined __GNUC__
	#define WARN_RESTORE() PRAGMA(GCC diagnostic pop)
#else
	#define WARN_RESTORE()
#endif

#if defined __clang__
	#define WARN_IGNORE(x) PRAGMA(clang diagnostic push) PRAGMA(clang diagnostic ignored x)
#elif defined __GNUC__
	#define WARN_IGNORE(x) PRAGMA(GCC diagnostic push) PRAGMA(GCC diagnostic ignored x)
#else
	#define WARN_IGNORE(x)
#endif

#if defined __GNUC__ || defined __clang__
	#define WARN_IGNORE__SIGN_COMPARE() WARN_IGNORE("-Wsign-compare")
#else
	#define WARN_IGNORE__SIGN_COMPARE()
#endif

#if defined __GNUC__ || defined __clang__
	#define WARN_IGNORE__ADDRESS() WARN_IGNORE("-Waddress")
#else
	#define WARN_IGNORE__ADDRESS()
#endif

#if defined __GNUC__ && !defined __clang__
	#define WARN_IGNORE__NONNULL_COMPARE() WARN_IGNORE("-Wnonnull-compare")
#else
	#define WARN_IGNORE__NONNULL_COMPARE()
#endif

#if defined __GNUC__ || defined __clang__
	#define WARN_IGNORE__REORDER() WARN_IGNORE("-Wreorder")
#else
	#define WARN_IGNORE__REORDER()
#endif

#if defined __GNUC__ || defined __clang__
	#define WARN_IGNORE__CONVERSION_NULL() WARN_IGNORE("-Wconversion-null")
#else
	#define WARN_IGNORE__CONVERSION_NULL()
#endif

#if defined __GNUC__ || defined __clang__
	#define WARN_IGNORE__NULL_DEREFERENCE() WARN_IGNORE("-Wnull-dereference")
#else
	#define WARN_IGNORE__NULL_DEREFERENCE()
#endif

#if defined __GNUC__ || defined __clang__
	#define WARN_IGNORE__CLASS_MEMACCESS() WARN_IGNORE("-Wclass-memaccess")
#else
	#define WARN_IGNORE__CLASS_MEMACCESS()
#endif


class IVEngineServer;
class IServerGameDLL;
class IFileSystem;
class IServerGameClients;
class IServer;
class ICvar;
class ISpatialPartition;
class IEngineTrace;
class IStaticPropMgrServer;
class IGameEventManager2;
class INetworkStringTableContainer;
class IEngineSound;
class IVModelInfo;
class IVDebugOverlay;

class IPlayerInfoManager;
class IBotManager;

class IPhysics;
class IPhysicsCollision;

class ISoundEmitterSystemBase;

class IMaterialSystem;

namespace vgui {
	class IVGui;
	class IInput;
	class IPanel;
	class ISchemeManager;
	class ISystem;
	class ILocalize;
	class IInputInternal;
	
	class ISurface;
}
class IMatSystemSurface;

class CGlobalVars;
class CGlobalEntityList;
class CBaseEntityList;

class IVEngineClient;
class IBaseClientDLL;
class IClientEntityList;

class IEngineTool;
class IServerTools;
class IClientTools;

class IVProfExport;

class IDedicatedExports;

class IMDLCache;

namespace SourceMod {
	class IExtensionManager;
}

class IClientMode;


extern IVEngineServer *engine;
extern IServerGameDLL *gamedll;
extern IFileSystem *filesystem;
extern IServerGameClients *serverGameClients;
extern IServer *sv;
extern ICvar *icvar;
extern ISpatialPartition *partition;
extern IEngineTrace *enginetrace;
extern IStaticPropMgrServer *staticpropmgr;
extern IGameEventManager2 *gameeventmanager;
extern INetworkStringTableContainer *networkstringtable;
extern IEngineSound *enginesound;
extern IVModelInfo *modelinfo;
extern IVDebugOverlay *debugoverlay;

extern IPlayerInfoManager *playerinfomanager;
extern IBotManager *botmanager;

extern IPhysics *physics;
extern IPhysicsCollision *physcollision;

extern ISoundEmitterSystemBase *soundemitterbase;

extern IMaterialSystem *g_pMaterialSystem;

extern vgui::IVGui *g_pVGui;
extern vgui::IInput *g_pVGuiInput;
extern vgui::IPanel *g_pVGuiPanel;
extern vgui::ISchemeManager *g_pVGuiSchemeManager;
extern vgui::ISystem *g_pVGuiSystem;
extern vgui::ILocalize *g_pVGuiLocalize;
extern vgui::IInputInternal *g_pVGuiInputInternal;

extern vgui::ISurface *g_pVGuiSurface;
extern IMatSystemSurface *g_pMatSystemSurface;

extern CGlobalVars *gpGlobals;
extern CGlobalEntityList *gEntList;
extern CBaseEntityList *g_pEntityList;

extern IVEngineClient *engineclient;
extern IBaseClientDLL *clientdll;
extern IClientEntityList *cl_entitylist;

extern IEngineTool *enginetools;
extern IServerTools *servertools;
extern IClientTools *clienttools;

extern IVProfExport *vprofexport;

extern IDedicatedExports *dedicated;

extern IMDLCache *mdlcache;

extern IClientMode *g_pClientMode;


/* C standard library */
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cinttypes>
#include <cmath>
#include <cfloat>


/* C++ standard library */
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
//#include <random> // <-- costs  ~89ms per compile
//#include <regex>  // <-- costs ~285ms per compile
#include <string>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <utility>
using namespace std::literals;


/* STL */
#include <deque>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>


/* Boost */


/* Posix specific */
#if defined _LINUX || defined _OSX

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef DEBUG

#include <libunwind.h>

#endif

#define HAVE_DECL_BASENAME 1
#include <libiberty/demangle.h>
#include <libelf.h>

#endif


/* Windows specific */
#if defined _WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma warning(disable:4091)
#include <Dbghelp.h>
#pragma warning(default:4091)

#include <Winsock2.h>
#include <WS2tcpip.h>

/* namespace clash between Windows CreateEvent macro and IGameEventManager2::CreateEvent */
#if defined CreateEvent
#undef CreateEvent
#endif

#endif


/* LZ4 */
//#include <lz4.h>


/* LodePNG */
//#include <lodepng.h>


/* Capstone */
//#include <capstone.h>


/* Approximate Nearest Neighbor */
//#include <ANN/ANN.h>


/* Source's public/video/ivideoservices.h is naughty and uses the preprocessor
 * to override nullptr; so prevent that header from being included */
#define IVIDEOSERVICES_H
class IVideoRecorder;


/* Source SDK */
#define RAD_TELEMETRY_DISABLED
#undef   NO_STRING_T
#undef WEAK_STRING_T
#ifdef CSGO_SEPARETE_
#include <platform.h>
#include <const.h>
#include <interface.h>
#include <commonmacros.h>
#include <basetypes.h>
#include "sdkcsgo/basetypes.h"
#else
#include "sdk2013/platform.h"
#include "sdk2013/const.h"
#include <interface.h>
#include "sdk2013/commonmacros.h"
#include "sdk2013/basetypes.h"
#endif

WARN_IGNORE__ADDRESS()
#include <dbg.h>
WARN_RESTORE()
#undef COMPILE_TIME_ASSERT
#define COMPILE_TIME_ASSERT(pred) static_assert(pred)

#ifndef CSGO_SEPARETE_
#include "sdk2013/Color.h"
#include "sdk2013/threadtools.h"
#include "sdk2013/vector2d.h"
#include "sdk2013/vector.h"
#include "sdk2013/vector4d.h"
#include "sdk2013/mathlib.h"
#else
#include <Color.h>
#include "sdkcsgo/threadtools.h"
#include <vector2d.h>
#include <vector.h>
#include <vector4d.h>
#include <mathlib.h>
#endif

#ifdef SE_TF2
#include "sdk2013/basehandle.h"
#else
#include <basehandle.h>
#endif
#include "sdk2013/string_t.h"

#ifndef CSGO_SEPARETE_
#include <annotations.h>
#endif

WARN_IGNORE__CLASS_MEMACCESS()
#include <utlmemory.h>
WARN_RESTORE()
#include <utlstring.h>
#define swap V_swap
#include <utlvector.h>
#include <UtlSortVector.h>
#undef swap
WARN_IGNORE__REORDER()
#include <utlrbtree.h>
WARN_RESTORE()
#include <utlmap.h>
WARN_IGNORE__SIGN_COMPARE()
#include <utlbuffer.h>
WARN_RESTORE()
#include <shareddefs.h>
#include <icvar.h>
#ifndef CSGO_SEPARETE_
#include "sdk2013/convar.h"
#else
#include <convar.h>
#endif

#include <fmtstr.h>
#include <KeyValues.h>
#include <ehandle.h>
#include <datamap.h>
#include <predictioncopy.h>
#include "sdk2013/takedamageinfo.h"
//#include <takedamageinfo.h>
#include <iserverentity.h>
WARN_IGNORE__ADDRESS()
WARN_IGNORE__NONNULL_COMPARE()
#ifndef CSGO_SEPARETE_
#include <edict.h>
#else
#include "sdkcsgo/edict.h"
#endif
WARN_RESTORE()
WARN_RESTORE()
#include <IEngineTrace.h>
#include <IStaticPropMgr.h>
#include <ivdebugoverlay.h>
#include "sdk2013/debugoverlay_shared.h"
#include <dt_send.h>
#include <util_shared.h>
#undef EntityFromEntityHandle
#include "sdk2013/ientityhandle.h"
#define EntityFromEntityHandle MyEntityFromEntityHandle
#include <ai_activity.h>
#include <usercmd.h>
#ifndef CSGO_SEPARETE_
#include <npcevent.h>
#endif
#include <bitbuf.h>
#ifdef SE_TF2
#include "sdk2013/eiface.h"
#else
#include <eiface.h>
#endif
#ifndef CSGO_SEPARETE_
#include "sdk2013/imaterialsystem_V081.h"
#include <materialsystem/imaterial.h>
#endif
#ifndef CSGO_SEPARETE_
#include <toolframework/itoolentity.h>
#else
#include "sdkcsgo/itoolentity.h"
#endif
#include <datamap.h>
#include <ispatialpartition.h>
#include <random.h>
#include <ai_activity.h>
#include <igameevents.h>
#include <inetmessage.h>
#ifdef SE_TF2
#include "sdk2013/inetchannel.h"
#else
#include <inetchannel.h>
#endif
#include <irecipientfilter.h>
//#include <usermessages.h>
#ifndef CSGO_SEPARETE_
#include <cdll_int.h>
#endif
#include <iclientnetworkable.h>
#include <utldict.h>
#include <mp_shareddefs.h>
WARN_IGNORE__REORDER()
#include <bitmap/imageformat.h>
WARN_RESTORE()
//#include <materialsystem/imaterialsystem.h>
#include <particle_parse.h>
#include <effect_dispatch_data.h>
#include <SoundEmitterSystem/isoundemittersystembase.h>
#include <IEngineSound.h>
#include <igamesystem.h>
#include <vphysics_interface.h>
#ifndef CSGO_SEPARETE_
WARN_IGNORE__CLASS_MEMACCESS()
#include <studio.h>
WARN_RESTORE()
WARN_IGNORE__REORDER()
#include <istudiorender.h>
WARN_RESTORE()
#include <ivmodelrender.h>
#endif
#include <vcollide_parse.h>
#include <steam/steamclientpublic.h>
#include <netadr.h>
WARN_IGNORE__NULL_DEREFERENCE()
#include <iclientrenderable.h>
WARN_RESTORE()
#include <toolframework/ienginetool.h>
WARN_IGNORE__REORDER()
#include <vprof.h>
WARN_RESTORE()
#include <networkstringtabledefs.h>

#ifndef CSGO_SEPARETE_
#include <valve_minmax_off.h>
#endif
#include <stringpool.h>
#include <filesystem.h>

#ifndef CSGO_SEPARETE_
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/ILocalize.h>
#include <vgui/IInputInternal.h>
#include <vgui/ISurface.h>
#include <VGuiMatSurface/IMatSystemSurface.h>
#endif

#include <IKeyValuesSystem.h>
#include <model_types.h>
#include <idedicatedexports.h>
#include <icommandline.h>
#ifdef SE_TF2
#include "sdk2013/iserver.h"
#else
#include <iserver.h>
#endif
#include <iclient.h>
#include <datacache/imdlcache.h>
WARN_IGNORE__SIGN_COMPARE()
WARN_IGNORE__REORDER()
#include <materialsystem/imesh.h>
WARN_RESTORE()
WARN_RESTORE()
#include <../server/iplayerinfo.h>
#include <icliententity.h>
#include <client_class.h>
#include <../server/iscorer.h>
#ifndef CSGO_SEPARETE_
#include <../client/iclientmode.h>
WARN_IGNORE__CONVERSION_NULL()
#include <vgui_controls/Frame.h>
#endif
WARN_RESTORE()
#include <soundflags.h>
#include <overlaytext.h>
#include <tier2/meshutils.h>
#include <server_class.h>
#include <dt_utlvector_common.h>
WARN_IGNORE__REORDER()
#include <../server/variant_t.h>
WARN_RESTORE()
#include <icliententity.h>
#include <icliententitylist.h>
#include <gamestringpool.h>

#define DECLARE_PREDICTABLE()
#include <collisionproperty.h>

/* Other utilities */
#include "util/fixed_string.h"


// can't include game/shared/particle_property.h because it tries to include "particles_new.h" which is in game/client
// so we'll just put what we need here... sigh
#define INVALID_PARTICLE_ATTACHMENT -1

/* the order in which headers include each other can be tricky; so for the SDK2013 headers we've messed with, we'll
 * always put a special preprocessor definition INSIDE of the already-included check in our modified version; that way,
 * we can verify that OUR version of the header was encountered first and took precedence, not the original one
 * (if our version did not take precedence, then these static_assert's won't even fail per se, they'll just encounter an
 * undefined name that they can't deal with and the compiler will become angry) */

#ifdef _LINUX
#define LINUX 1
#endif

#ifndef CSGO_SEPARETE_
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_TIER0_PLATFORM_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_CONST_H);
#ifdef SE_TF2
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_BASEHANDLE_H);
#endif
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_TIER0_COMMONMACROS_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_TIER0_BASETYPES_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_COLOR_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_MATHLIB_VECTOR2D_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_MATHLIB_VECTOR_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_MATHLIB_VECTOR4D_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_MATHLIB_MATHLIB_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_TIER1_CONVAR_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_TIER1_CONVAR_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__GAME_SHARED_DEBUGOVERLAY_SHARED_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_MATERIALSYSTEM_IMATERIALSYSTEM_H);
static_assert(_SIGSEGV_SDK2013_OVERRIDE__PUBLIC_STRING_T_H);
#endif


/* AMTL */


/* MetaMod */


/* SourceMod */
#include "sm/MemoryUtils.h"


/* this extension */
#include <smsdk_ext.h>
#include <extension.h>

/* dammit Valve, why do you have to do stupid shit like redefining offsetof?
 * maybe you live in a world where it's impossible to overload operator&, but
 * some of us use non-ancient development tools where a proper, working offsetof
 * is actually important for things to work right */
#undef offsetof
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)


#endif


#endif
