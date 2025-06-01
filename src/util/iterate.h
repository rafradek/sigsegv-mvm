#ifndef _INCLUDE_SIGSEGV_UTIL_ITERATE_H_
#define _INCLUDE_SIGSEGV_UTIL_ITERATE_H_


#include "stub/baseentity.h"


/* magic template wizardry to make function pointers or C++ lambda closures
 * work as functor parameters, whether they have void or bool return type */
template<typename RET>
struct IterateInternals
{
	template<typename FUNCTOR, typename... ARGS>
	static bool CallFunc(const FUNCTOR& functor, ARGS&&... args);
};
template<> template<typename FUNCTOR, typename... ARGS>
inline bool IterateInternals<void>::CallFunc(const FUNCTOR& func, ARGS&&... args)
{
	func(std::forward<ARGS>(args)...);
	return true;
}
template<> template<typename FUNCTOR, typename... ARGS>
inline bool IterateInternals<bool>::CallFunc(const FUNCTOR& func, ARGS&&... args)
{
	return func(std::forward<ARGS>(args)...);
}
#define CALL_FUNCTOR(...) IterateInternals<std::result_of_t<FUNCTOR(__VA_ARGS__)>>::CallFunc


template<typename FUNCTOR>
inline void ForEachEntity(const FUNCTOR& func)
{
	using T = CBaseEntity;
	
	for (CBaseEntity *ent = servertools->FirstEntity(); ent != nullptr; ent = servertools->NextEntity(ent)) {
		if (!CALL_FUNCTOR(T *)(func, ent)) break;
	}
}


template<typename FUNCTOR>
inline void ForEachEntityByClassname(const char *classname, const FUNCTOR& func)
{
	using T = CBaseEntity;
	
	for (CBaseEntity *ent = servertools->FindEntityByClassname(nullptr, classname); ent != nullptr; ent = servertools->FindEntityByClassname(ent, classname)) {
		if (!CALL_FUNCTOR(T *)(func, ent)) break;
	}
}

template<typename T, typename FUNCTOR>
inline void ForEachEntityByClassnameRTTI(const char *classname, const FUNCTOR& func)
{
	for (CBaseEntity *ent = servertools->FindEntityByClassname(nullptr, classname); ent != nullptr; ent = servertools->FindEntityByClassname(ent, classname)) {
		T *sub = rtti_cast<T *>(ent);
		if (sub == nullptr) continue;
		
		if (!CALL_FUNCTOR(T *)(func, sub)) break;
	}
}

template<typename T, typename FUNCTOR>
inline void ForEachEntityByRTTI(const FUNCTOR& func)
{
	for (CBaseEntity *ent = servertools->FirstEntity(); ent != nullptr; ent = servertools->NextEntity(ent)) {
		T *sub = rtti_cast<T *>(ent);
		if (sub == nullptr) continue;
		
		if (!CALL_FUNCTOR(T *)(func, sub)) break;
	}
}


#ifdef _INCLUDE_SIGSEGV_STUB_BASEPLAYER_H_
template<typename FUNCTOR>
inline void ForEachPlayer(const FUNCTOR& func)
{
	using T = CBasePlayer;
	
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		T *player = UTIL_PlayerByIndex(i);
		if (player == nullptr) continue;
		
		if (!CALL_FUNCTOR(T *)(func, player)) break;
	}
}

template<typename FUNCTOR>
inline void ForEachBot(const FUNCTOR& func)
{
	using T = CBasePlayer;
	
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		T *player = UTIL_PlayerByIndex(i);
		if (player == nullptr) continue;
		if (!player->IsBot())  continue;
		
		if (!CALL_FUNCTOR(T *)(func, player)) break;
	}
}

#ifdef _INCLUDE_SIGSEGV_STUB_TEAM_H_
template<typename FUNCTOR>
inline void ForEachPlayerOnTeam(CTeam *team, const FUNCTOR& func)
{
	using T = CBasePlayer;
	
	for (int i = team->GetNumPlayers() - 1; i >= 0; --i) {
		T *player = team->GetPlayer(i);
		if (player == nullptr) continue;
		
		if (!CALL_FUNCTOR(T *)(func, player)) break;
	}
}
#endif
#endif


#ifdef _INCLUDE_SIGSEGV_STUB_TFPLAYER_H_
template<typename FUNCTOR>
inline void ForEachTFPlayer(const FUNCTOR& func)
{
	using T = CTFPlayer;
	
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		T *player = ToTFPlayer(reinterpret_cast<CBasePlayer *>(INDEXENT(i)->GetUnknown()));
		if (player == nullptr) continue;
		
		if (!CALL_FUNCTOR(T *)(func, player)) break;
	}
}

#ifdef _INCLUDE_SIGSEGV_STUB_ENTITIES_H_
template<typename FUNCTOR>
inline void ForEachTFPlayerEconEntity(CTFPlayer *player, const FUNCTOR& func, bool ignoreHelpers = true)
{
	using T = CEconEntity;
	
	for (int i = 0; i < player->GetNumWearables(); i++) {
		CEconWearable *wearable = player->GetWearable(i);
		if (wearable == nullptr) continue;

		// Ignore helper custom model wearables
		if (ignoreHelpers && wearable->GetItem()->m_iEntityLevel == 414918) continue;
		
		if (!CALL_FUNCTOR(T *)(func, wearable)) break;
	}
	for (int i = 0; i < player->WeaponCount(); i++) {
		CBaseCombatWeapon *weapon = player->GetWeapon(i);
		if (weapon == nullptr) continue;
		
		if (!CALL_FUNCTOR(T *)(func, weapon)) break;
	}
}
#endif

#ifdef _INCLUDE_SIGSEGV_STUB_TEAM_H_
template<typename FUNCTOR>
inline void ForEachTFPlayerOnTeam(CTeam *team, const FUNCTOR& func)
{
	using T = CTFPlayer;
	
	for (int i = team->GetNumPlayers() - 1; i >= 0; --i) {
		T *player = ToTFPlayer(team->GetPlayer(i));
		if (player == nullptr) continue;
		
		if (!CALL_FUNCTOR(T *)(func, player)) break;
	}
}
#endif
#endif


#ifdef _INCLUDE_SIGSEGV_STUB_TFBOT_H_
template<typename FUNCTOR>
inline void ForEachTFBot(const FUNCTOR& func)
{
	using T = CTFBot;
	
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		T *bot = ToTFBot(reinterpret_cast<CBasePlayer *>(INDEXENT(i)->GetUnknown()));
		if (bot == nullptr) continue;
		
		if (!CALL_FUNCTOR(T *)(func, bot)) break;
	}
}
#endif


#ifdef _INCLUDE_SIGSEGV_STUB_MISC_H_
template<typename FUNCTOR>
inline void ForEachMap(const FUNCTOR& func)
{
	using T = const char *;
	
	CMapListManager& mgr = g_MapListMgr;
	
	mgr.RefreshList();
	
	for (int i = 0; i < mgr.GetMapCount(); ++i) {
		const char *map_name = mgr.GetMapName(i);
		if (map_name == nullptr || map_name[0] == '\x00' || strcmp(map_name, "Invalid!!!") == 0) continue;
		
		if (!CALL_FUNCTOR(T *)(func, map_name)) break;
	}
}
#endif


template<typename FUNCTOR>
struct PartitionEnumerator : public IPartitionEnumerator
{
	PartitionEnumerator(const FUNCTOR& func) : m_Func(func) {}
	
	virtual IterationRetval_t EnumElement(IHandleEntity *pHandleEntity) override
	{
		CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);
		
		if (CALL_FUNCTOR(CBaseEntity *)(m_Func, pEntity)) {
			return ITERATION_CONTINUE;
		} else {
			return ITERATION_STOP;
		}
	}
	
	const FUNCTOR& m_Func;
};

template<typename FUNCTOR>
inline void ForEachEntityInSphere(const Vector& center, float radius, const FUNCTOR& func)
{
	PartitionEnumerator<FUNCTOR> enumerator(func);
	partition->EnumerateElementsInSphere(PARTITION_ENGINE_NON_STATIC_EDICTS, center, radius, false, &enumerator);
}


#undef CALL_FUNCTOR


#endif
