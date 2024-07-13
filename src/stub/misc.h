#ifndef _INCLUDE_SIGSEGV_STUB_MISC_H_
#define _INCLUDE_SIGSEGV_STUB_MISC_H_


#include "link/link.h"
#include "stub/usermessages_sv.h"
#include "stub/baseplayer.h"


class CTFBotMvMEngineerTeleportSpawn;
class CTFBotMvMEngineerBuildTeleportExit;
class CTFBotMvMEngineerBuildSentryGun;

class CFlaggedEntitiesEnum;

class CRConClient;

class CBaseEntity;
class CBasePlayer;


CRConClient& RCONClient();

typedef struct hudtextparms_s
{
	float		x;
	float		y;
	int			effect;
	byte		r1, g1, b1, a1;
	byte		r2, g2, b2, a2;
	float		fadeinTime;
	float		fadeoutTime;
	float		holdTime;
	float		fxTime;
	int			channel;
} hudtextparms_t;

#define DECL_FT_WRAPPER(name) \
	template<typename... ARGS> \
	decltype(ft_##name)::RetType name(ARGS&&... args) \
	{ return ft_##name(std::forward<ARGS>(args)...); }


extern StaticFuncThunk<void, const Vector&, trace_t&, const Vector&, const Vector&, CBaseEntity *> ft_FindHullIntersection;
DECL_FT_WRAPPER(FindHullIntersection);


extern StaticFuncThunk<int> ft_UTIL_GetCommandClientIndex;
DECL_FT_WRAPPER(UTIL_GetCommandClientIndex);

extern StaticFuncThunk<CBasePlayer *> ft_UTIL_GetCommandClient;
DECL_FT_WRAPPER(UTIL_GetCommandClient);

extern StaticFuncThunk<bool> ft_UTIL_IsCommandIssuedByServerAdmin;
DECL_FT_WRAPPER(UTIL_IsCommandIssuedByServerAdmin);


#if 0
extern StaticFuncThunk<const char *, const char *, int> TranslateWeaponEntForClass;
//const char *TranslateWeaponEntForClass(const char *name, int classnum);
#endif



class CMapListManager
{
public:
	void RefreshList()                      {        ft_RefreshList(this); }
	int GetMapCount() const                 { return ft_GetMapCount(this); }
	int IsMapValid(int index) const         { return ft_IsMapValid (this, index); }
	const char *GetMapName(int index) const { return ft_GetMapName (this, index); }
	
private:
	static MemberFuncThunk<      CMapListManager *, void>              ft_RefreshList;
	static MemberFuncThunk<const CMapListManager *, int>               ft_GetMapCount;
	static MemberFuncThunk<const CMapListManager *, int, int>          ft_IsMapValid;
	static MemberFuncThunk<const CMapListManager *, const char *, int> ft_GetMapName;
};

extern GlobalThunk<CMapListManager> g_MapListMgr;

void PrecacheParticleSystem(const char *name);

bool CRC_File(CRC32_t *crcvalue, const char *pszFileName);

void UTIL_HudMessage(CBasePlayer *player, const hudtextparms_t & params, const char *message);

void UTIL_PlayerDecalTrace(CGameTrace *tr, int playerid);

void UTIL_StringToVector(float *base, const char* string);

void UTIL_ParticleTracer(const char *pszTracerEffectName, const Vector &vecStart, const Vector &vecEnd, 
				 int iEntIndex, int iAttachment, bool bWhiz);

void TE_PlayerDecal(IRecipientFilter& filter, float delay, const Vector* pos, int player, int entity);

void PrintToChatAll(const char *str);

void PrintToChat(const char *str, CBasePlayer *player);

// Format text to internal buffer, uses SourceMod formatting for %t support. Parameters must be pointers, not direct values. If format fails, fmt pointer is returned instead.
const char *FormatTextForPlayerSM(CBasePlayer *player, int paramCount, const char *fmt, ...);

// Translates text for given player
const char *TranslateText(CBasePlayer *player, const char *name, int paramCount = 0, ...);

void PrintToChatSM(CBasePlayer *player, int paramCount, const char *fmt, ...);
void PrintToChatAllSM(int paramCount, const char *fmt, ...);

struct EventQueuePrioritizedEvent_t
{
	float m_flFireTime;
	string_t m_iTarget;
	string_t m_iTargetInput;
	EHANDLE m_pActivator;
	EHANDLE m_pCaller;
	int m_iOutputID;
	EHANDLE m_pEntTarget;  // a pointer to the entity to target; overrides m_iTarget

	variant_t m_VariantValue;	// variable-type parameter

	EventQueuePrioritizedEvent_t *m_pNext;
	EventQueuePrioritizedEvent_t *m_pPrev;
};

class CEventQueue {
public:
	void AddEvent( const char *target, const char *targetInput, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID ) {ft_AddEvent(this,target,targetInput,Value,fireDelay,pActivator,pCaller,outputID);}
	void CancelEvents(CBaseEntity *entity) {ft_CancelEvents(this, entity);}
	
	EventQueuePrioritizedEvent_t m_Events;
private:
	static MemberFuncThunk< CEventQueue*, void, const char*,const char *, variant_t, float, CBaseEntity *, CBaseEntity *, int>   ft_AddEvent;
	static MemberFuncThunk< CEventQueue*, void, CBaseEntity *>   ft_CancelEvents;
};

extern GlobalThunk<CEventQueue> g_EventQueue;

class CSoundEmitterSystemBase : public ISoundEmitterSystemBase
{
public: 
	bool InitSoundInternalParameter(const char *soundname, KeyValues *kv, CSoundParametersInternal &params) { return ft_InitSoundInternalParameter(this, soundname, kv, params); }
	void AddSoundsFromFile(const char *filename, bool bPreload, bool bIsOverride, bool bRefresh)            { return ft_AddSoundsFromFile(this, filename, bPreload, bIsOverride, bRefresh); }

private:
	static MemberFuncThunk<CSoundEmitterSystemBase *, bool, const char *, KeyValues *, CSoundParametersInternal &> ft_InitSoundInternalParameter;
	static MemberFuncThunk<CSoundEmitterSystemBase *, void, const char *, bool, bool, bool>                        ft_AddSoundsFromFile;
};

#endif