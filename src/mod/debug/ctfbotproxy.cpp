#include "mod.h"
#include "util/misc.h"
#include "util/rtti.h"
#include "util/iterate.h"

////////////////////////////////////////////////////////////////////////////////
#define typeof decltype
#include <../server/variant_t.h>
#define private public
#include <../server/eventqueue.h>
#undef private
#undef typeof

const char *variant_t::ToString( void ) const
{
	COMPILE_TIME_ASSERT( sizeof(string_t) == sizeof(int) );

	static char szBuf[512];

	switch (fieldType)
	{
	case FIELD_STRING:
		{
			return(STRING(iszVal));
		}

	case FIELD_BOOLEAN:
		{
			if (bVal == 0)
			{
				Q_strncpy(szBuf, "false",sizeof(szBuf));
			}
			else
			{
				Q_strncpy(szBuf, "true",sizeof(szBuf));
			}
			return(szBuf);
		}

	case FIELD_INTEGER:
		{
			Q_snprintf( szBuf, sizeof( szBuf ), "%i", iVal );
			return(szBuf);
		}

	case FIELD_FLOAT:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%g", flVal);
			return(szBuf);
		}

	case FIELD_COLOR32:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
			return(szBuf);
		}

	case FIELD_VECTOR:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
			return(szBuf);
		}

	case FIELD_VOID:
		{
			szBuf[0] = '\0';
			return(szBuf);
		}

	case FIELD_EHANDLE:
		{
			const char *pszName = (Entity()) ? STRING(Entity()->GetEntityName()) : "<<null entity>>";
			Q_strncpy( szBuf, pszName, 512 );
			return (szBuf);
		}
	}

	return("No conversion to string");
}
////////////////////////////////////////////////////////////////////////////////


class IMapEntityFilter {};


class CTFBotProxy : public CBaseEntity {};


namespace Mod::Debug::CTFBotProx
{
	const char *MakeEscapedVersion(const char *str)
	{
		static char buf[0x100000];
		size_t pos = 0;
		
		for (const char *c = str; *c != '\0'; ++c) {
			if (isprint(*c)) {
				buf[pos++] = *c;
			} else {
				char tmp[0x10];
				V_sprintf_safe(tmp, "%02X", *(const uint8_t *)c);
				
				buf[pos++] = '\\';
				buf[pos++] = 'x';
				buf[pos++] = tmp[0];
				buf[pos++] = tmp[1];
			}
		}
		buf[pos] = '\0';
		
		return buf;
	}
	
	
	void PrintLineBrokenEscapedVersion(const char *str)
	{
		bool done = false;
		
		do {
			static char buf[0x10000];
			size_t pos = 0;
			
			for ( ; ; ++str) {
				if (*str == '\0') {
					done = true;
					break;
				}
				
				if (isprint(*str)) {
					buf[pos++] = *str;
				} else {
					char tmp[0x10];
					V_sprintf_safe(tmp, "%02X", *(const uint8_t *)str);
					
					buf[pos++] = '\\';
					buf[pos++] = 'x';
					buf[pos++] = tmp[0];
					buf[pos++] = tmp[1];
				}
				
				if (*str == '\n') {
					++str;
					break;
				}
			}
			
			buf[pos++] = '\n';
			buf[pos++] = '\0';
			
			Msg("%s", buf);
		} while (!done);
	}
	
	
	DETOUR_DECL_STATIC(string_t, AllocPooledString, const char *pszValue)
	{
		ConColorMsg(Color(0xff, 0xff, 0x00, 0xff), "AllocPooledString(%s)\n", pszValue);
		return DETOUR_STATIC_CALL(pszValue);
	}
	
	DETOUR_DECL_MEMBER(void, CTFBotProxy_C1)
	{
		static uintptr_t prev = 0x00000000;
		
		uintptr_t diff = Max((uintptr_t)this, prev) - Min((uintptr_t)this, prev);
		ConColorMsg(Color(0x00, 0xff, 0xff, 0xff), "CTFBotProxy constructed @ 0x%08x (diff: 0x%08x)\n", (uintptr_t)this, diff);
		prev = (uintptr_t)this;
		
		DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(bool, IServerGameDLL_LevelInit, const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background)
	{
		ConColorMsg(Color(0xff, 0x00, 0xff, 0xff), "DETOUR for IServerGameDLL::LevelInit\n");
		
		for (const uint8_t *c = reinterpret_cast<const uint8_t *>(pMapEntities); *c != '\0'; ++c) {
			if (*c >= 0x80 && *c <= 0xff) {
				Msg("- Detected character %02x @ pMapEntities+0x%x\n", (int)(*c), (uintptr_t)c - (uintptr_t)pMapEntities);
			}
		}
		
		return DETOUR_MEMBER_CALL(pMapName, pMapEntities, pOldLevel, pLandmarkName, loadGame, background);
	}
	
	DETOUR_DECL_STATIC(void, MapEntity_ParseAllEntities, const char *pMapData, IMapEntityFilter *pFilter, bool bActivateEntities)
	{
		printf("MapEntity_ParseAllEntities: %s\n", MakeEscapedVersion(pMapData));
		
		DETOUR_STATIC_CALL(pMapData, pFilter, bActivateEntities);
	}
	
	DETOUR_DECL_STATIC(const char *, MapEntity_ParseEntity, CBaseEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter)
	{
		printf("MapEntity_ParseEntity: %s\n", MakeEscapedVersion(pEntData));
	//	PrintLineBrokenEscapedVersion(pEntData);
		
		return DETOUR_STATIC_CALL(pEntity, pEntData, pFilter);
	}
	
	DETOUR_DECL_STATIC(const char *, MapEntity_ParseToken, const char *data, char *newToken)
	{
		auto result = DETOUR_STATIC_CALL(data, newToken);
		
		int len1 = result - data;
		int len2 = strlen(newToken);
		Msg("MapEntity_ParseToken: %d %d %s\n", len1, len2, MakeEscapedVersion(newToken));
		
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CEventQueue_AddEvent, EventQueuePrioritizedEvent_t *pe)
	{
		auto queue = reinterpret_cast<CEventQueue *>(this);
		
		ConColorMsg(Color(0x00, 0xff, 0x00, 0xff),
			"[%10.6f] AddEvent: [t: %10.6f] [target: '%s' '%s'] [param: '%s'] [activator: '%s'] [caller: '%s']\n",
			engine->GetServerTime(),
			pe->m_flFireTime,
			STRING(pe->m_iTarget),
			STRING(pe->m_iTargetInput),
			pe->m_VariantValue.String(),
			(pe->m_pActivator != nullptr ? STRING(pe->m_pActivator->GetEntityName()) : "none"),
			(pe->m_pCaller    != nullptr ? STRING(pe->m_pCaller   ->GetEntityName()) : "none"));
		
		DETOUR_MEMBER_CALL(pe);
	}
	
	DETOUR_DECL_MEMBER(void, CEventQueue_ServiceEvents)
	{
		auto queue = reinterpret_cast<CEventQueue *>(this);
		
		if (queue->m_Events.m_pNext != nullptr) {
			ConColorMsg(Color(0xff, 0xff, 0x00, 0xff),
				"[%10.6f] CEventQueue::ServiceEvents DUMP:\n",
				engine->GetServerTime());
			
			for (EventQueuePrioritizedEvent_t *pe = queue->m_Events.m_pNext; pe != nullptr; pe = pe->m_pNext) {
				ConColorMsg(Color(0xff, 0xff, 0x00, 0xff),
					"[t: %10.6f] [target: '%s' '%s'] [param: '%s'] [activator: '%s'] [caller: '%s']\n",
					pe->m_flFireTime,
					STRING(pe->m_iTarget),
					STRING(pe->m_iTargetInput),
					pe->m_VariantValue.String(),
					(pe->m_pActivator != nullptr ? STRING(pe->m_pActivator->GetEntityName()) : "none"),
					(pe->m_pCaller    != nullptr ? STRING(pe->m_pCaller   ->GetEntityName()) : "none"));
			}
		}
		
		DETOUR_MEMBER_CALL();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:CTFBotProxy")
		{
			MOD_ADD_DETOUR_STATIC(AllocPooledString, "AllocPooledString");
			MOD_ADD_DETOUR_MEMBER(CTFBotProxy_C1,    "CTFBotProxy::CTFBotProxy [C1]");
			
		//	MOD_ADD_DETOUR_MEMBER(IServerGameDLL_LevelInit,   "IServerGameDLL::LevelInit");
		//	MOD_ADD_DETOUR_STATIC(MapEntity_ParseAllEntities, "MapEntity_ParseAllEntities");
		//	MOD_ADD_DETOUR_STATIC(MapEntity_ParseEntity,      "MapEntity_ParseEntity");
		//	MOD_ADD_DETOUR_STATIC(MapEntity_ParseToken,       "MapEntity_ParseToken");
			
			MOD_ADD_DETOUR_MEMBER(CEventQueue_AddEvent, "CEventQueue::AddEvent [EventQueuePrioritizedEvent_t *]");
			MOD_ADD_DETOUR_MEMBER(CEventQueue_ServiceEvents, "CEventQueue::ServiceEvents");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_ctfbotproxy", "0", FCVAR_NOTIFY,
		"",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	void PrintHeader()
	{
		Msg("\n--------------------------------------------------------------------------------\n");
		Msg("%-10s  %5s  %-12s | %-10s\n", "ADDR", "EIDX", "NAME", "VTADDR");
	}
	
	void PrintProxy(CTFBotProxy *proxy)
	{
		CFmtStr col_addr("0x%08x", (uintptr_t)proxy);
		CFmtStr col_eidx("#%d", ENTINDEX(proxy));
		CFmtStr col_name("%s", STRING(proxy->GetEntityName()));
		CFmtStr col_vt  ("0x%08x", *reinterpret_cast<uintptr_t *>(proxy));
		
		Msg("%-10s  %5s  %-12s | %10s\n", col_addr.Get(), col_eidx.Get(), col_name.Get(), col_vt.Get());
	}
	
	CON_COMMAND(sig_debug_ctfbotproxy_cmd, "")
	{
		static auto addr_VT   = (uintptr_t)RTTI::GetVTable<CTFBotProxy>();
		static auto addr_RTTI = (uintptr_t)RTTI::GetRTTI  <CTFBotProxy>();
		
		static auto addr_s_pTokenBuf = (uintptr_t)AddrManager::GetAddr("s_pTokenBuf");
		
		Msg("\n================================================================================\n");
		Msg("CTFBotProxy VT:   %08x\n", addr_VT);
		Msg("CTFBotProxy RTTI: %08x\n", addr_RTTI);
		Msg("s_pTokenBuf:      %08x\n", addr_s_pTokenBuf);
		
		
		if (args.ArgC() > 1 && FStrEq(args[1], "list")) {
			std::map<uintptr_t,   CTFBotProxy *> proxies_by_addr;
			std::map<int,         CTFBotProxy *> proxies_by_eidx;
			std::map<std::string, CTFBotProxy *> proxies_by_name;
			
			ForEachEntityByClassname("bot_proxy", [&](CBaseEntity *ent){
				auto proxy = static_cast<CTFBotProxy *>(ent);
				
				proxies_by_addr[(uintptr_t)ent]               = proxy;
				proxies_by_eidx[ENTINDEX(ent)]                = proxy;
				proxies_by_name[STRING(ent->GetEntityName())] = proxy;
			});
			
			bool by_addr = (args.ArgC() > 2 && FStrEq(args[2], "addr"));
			bool by_eidx = (args.ArgC() > 2 && FStrEq(args[2], "eidx"));
			bool by_name = (args.ArgC() > 2 && FStrEq(args[2], "name"));
			
			if (by_addr) {
				PrintHeader();
				for (const auto& pair : proxies_by_addr) {
					PrintProxy(pair.second);
				}
			} else if (by_eidx) {
				PrintHeader();
				for (const auto& pair : proxies_by_eidx) {
					PrintProxy(pair.second);
				}
			} else if (by_name) {
				PrintHeader();
				for (const auto& pair : proxies_by_name) {
					PrintProxy(pair.second);
				}
			}
		}
	}
}
