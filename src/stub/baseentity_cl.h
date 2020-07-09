#ifndef _INCLUDE_SIGSEGV_STUB_BASEENTITY_CL_H_
#define _INCLUDE_SIGSEGV_STUB_BASEENTITY_CL_H_


#include "link/link.h"
#include "prop.h"


class ClientHandleBase : public CBaseHandle
{
protected:
	/* replacement for CBaseHandle::Get */
	IHandleEntity *ClientGet() const;
	ClientHandleBase(const CBaseHandle& handle) : CBaseHandle(handle) {}
};

template<typename T>
class ClientHandle : public ClientHandleBase
{
public:
	ClientHandle()                                                {}
	ClientHandle(int iEntry, int iSerialNumber)                   { this->Init(iEntry, iSerialNumber); }
	ClientHandle(const CBaseHandle& handle) : ClientHandleBase(handle) {}
	ClientHandle(T *pObj)                                         { this->Term(); this->Set(pObj); }
	
	T *Get() const          { return (T *)this->ClientGet(); }
	void Set(const T *pVal) { CBaseHandle::Set(reinterpret_cast<const IHandleEntity *>(pVal)); }
	
	operator T*()       { return this->Get(); }
	operator T*() const { return this->Get(); }
	
	bool operator!() const                     { return !this->Get(); }
	bool operator==(T *val) const              { return (this->Get() == val); }
	bool operator!=(T *val) const              { return (this->Get() != val); }
	const CBaseHandle& operator=(const T *val) { this->Set(val); return *this; }
	T *operator->() const                      { return this->Get(); }
	
	static ClientHandle<T> FromIndex(int index)
	{
		ClientHandle<T> ret;
		ret.m_Index = index;
		return ret;
	}
};


/* convenience helpers for prop accessors */
template<typename U, T_PARAMS> struct CPropAccessorClientHandle : public CPropAccessorBase<ClientHandle<U>, T_ARGS>
{
	using CPropAccessorBase<ClientHandle<U>, T_ARGS>::operator=;
	
	operator U*() const   { return this->GetRO().Get(); }
	U* operator->() const { return this->GetRO().Get(); }
};
template<typename U, T_PARAMS> struct CPropAccessor<ClientHandle<U>, T_ARGS> : public CPropAccessorClientHandle<U, T_ARGS>
{
	using CPropAccessorClientHandle<U, T_ARGS>::operator=;
};


class C_BaseEntity : public IClientEntity
{
public:
	/* inline */
//	int entindex() const;
	void CalcAbsolutePosition() { this->GetVectors(nullptr, nullptr, nullptr); }
	
	/* getter/setter */
	C_BaseEntity *GetMoveParent() const { return this->m_pMoveParent; }
	
	/* thunk */
	const Vector& GetAbsVelocity() const                                 { return ft_GetAbsVelocity           (this); }
	void GetVectors(Vector *pForward, Vector *pRight, Vector *pUp) const { return ft_GetVectors               (this, pForward, pRight, pUp); }
	matrix3x4_t& GetParentToWorldTransform(matrix3x4_t& tempMatrix)      { return ft_GetParentToWorldTransform(this, tempMatrix); }
	
private:
	DECL_EXTRACT(ClientHandle<C_BaseEntity>, m_pMoveParent);
	
	static MemberFuncThunk<const C_BaseEntity *, const Vector&>                      ft_GetAbsVelocity;
	static MemberFuncThunk<const C_BaseEntity *, void, Vector *, Vector *, Vector *> ft_GetVectors;
	static MemberFuncThunk<      C_BaseEntity *, matrix3x4_t&, matrix3x4_t&>         ft_GetParentToWorldTransform;
};

class C_BaseAnimating : public C_BaseEntity
{
public:
	/* TODO */
	float GetModelScale() const;
	int GetHitboxSet() const;
	CStudioHdr *GetModelPtr() const;
	void GetBoneTransform(int iBone, matrix3x4_t& pBoneToWorld);
	
private:
	// ...
};

#ifdef _WINDOWS
inline float C_BaseAnimating::GetModelScale() const
{
	assert(engine->GetServerVersion() == 4016097);
	return *(float *)((uintptr_t)this + 0x658);
}
inline int C_BaseAnimating::GetHitboxSet() const
{
	assert(engine->GetServerVersion() == 4016097);
	return *(int *)((uintptr_t)this + 0x560);
}
#include <bone_setup.h>
#include "addr/standard.h"
inline void C_BaseAnimating::GetBoneTransform(int iBone, matrix3x4_t& pBoneToWorld)
{
	assert(engine->GetServerVersion() == 4016097);
	
	// really horrible unoptimized and probably unsafe implementation
	
//	static CAddr_FixedAddr("[client] C_BaseAnimating::GetBoneCache", "", 0x00173720, engine->GetServerVersion());
	static MemberFuncThunk<C_BaseAnimating *, CBoneCache *, CStudioHdr *> ft_GetBoneCache("");
	ft_GetBoneCache.ForceLink(LibMgr::GetInfo(Library::CLIENT).BaseAddr() + 0x173720);
	
//	static CAddr_FixedAddr("[client] CBoneCache::GetCachedBone", "", 0x001671d0, engine->GetServerVersion());
	static MemberFuncThunk<CBoneCache *, matrix3x4_t *, int> ft_GetCachedBone("");
	ft_GetCachedBone.ForceLink(LibMgr::GetInfo(Library::CLIENT).BaseAddr() + 0x1671d0);
	
	CBoneCache *pcache = ft_GetBoneCache(this, nullptr);
	matrix3x4_t *pmatrix = ft_GetCachedBone(pcache, iBone);
	
	MatrixCopy(*pmatrix, pBoneToWorld);
}
#endif


#if 0
#error WE ARE UNINTENTIONALLY DOING A VFUNC OVERRIDE HERE
// (and it doesn't actually override since we don't modify the vtable or its ptr at all)
inline int C_BaseEntity::entindex() const
{
	/* hacky; relies on EntitySearchResult being treated as equivalent to C_BaseEntity ptr */
	return clienttools->GetEntIndex(reinterpret_cast<EntitySearchResult>(const_cast<C_BaseEntity *>(this)));
}
#endif


inline IClientEntityList& ClientEntityList()
{
	assert(cl_entitylist != nullptr);
	return *cl_entitylist;
}

inline C_BaseEntity *GetClientEntityByIndex(int index)
{
	IClientEntity *ent = ClientEntityList().GetClientEntity(index);
	if (ent == nullptr) return nullptr;
	return ent->GetBaseEntity();
}


inline IHandleEntity *ClientHandleBase::ClientGet() const
{
	// TODO: THIS REALLY NEEDS TO IMPLEMENT THE STUFF IN CBaseEntityList::LookupEntity!!!
	// WHAT WE HAVE HERE IS A TEMPORARY HACK!!!
	return ClientEntityList().GetClientEntity(this->m_Index);
}


/* this class is a totally hacked-together temporary thing */
class C_BasePlayer : public C_BaseAnimating
{
public:
//	static C_BasePlayer *GetLocalPlayer() { return static_cast<C_BasePlayer *>(GetClientEntityByIndex(engineclient->GetLocalPlayer())); }
	static C_BasePlayer *GetLocalPlayer() { return reinterpret_cast<C_BasePlayer *>(clienttools->GetLocalPlayer()); }
};


#endif
