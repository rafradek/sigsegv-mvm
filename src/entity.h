#ifndef _INCLUDE_SIGSEGV_ENTITY_H_
#define _INCLUDE_SIGSEGV_ENTITY_H_


#error TODO: MUST call IHotplugEntityFactory::UninstallAll from CExtSigsegv::SDK_OnUnload!!!


#include <../extensions/sdktools/util.h>
#include "util/autolist.h"


// most of this is adapted from game/server/util.h


class IEntityFactory;

class IEntityFactoryDictionary
{
public:
	virtual void InstallFactory(IEntityFactory *pFactory, const char *pClassName) = 0;
	virtual IServerNetworkable *Create(const char *pClassName) = 0;
	virtual void Destroy(const char *pClassName, IServerNetworkable *pNetworkable) = 0;
	virtual IEntityFactory *FindFactory(const char *pClassName) = 0;
	virtual const char *GetCannonicalName(const char *pClassName) = 0;
};

inline IEntityFactoryDictionary *EntityFactoryDictionary()
{
	assert(servertools != nullptr);
	return servertools->GetEntityFactoryDictionary();
}


class IEntityFactory
{
public:
	virtual IServerNetworkable *Create(const char *pClassName) = 0;
	virtual void Destroy(IServerNetworkable *pNetworkable) = 0;
	virtual size_t GetEntitySize() = 0;
};


class IHotplugEntityFactory : public IEntityFactory, AutoList<IHotplugEntityFactory>
{
public:
	// ...
	
	static void UninstallAll()
	{
		//#error TODO: UTIL_RemoveImmediate all entities created by hotplug factories (so we need factories to track entity ptrs they have created)
		//#error TODO: iterate over the AutoList and remove the factories from EntityFactoryDictionary()->m_Factories
	}
}


template<class T>
class CEntityFactory : public IHotplugEntityFactory
{
public:
	CEntityFactory(const char *pClassName)
	{
		//#error TODO
		EntityFactoryDictionary()->InstallFactory(this, pClassName);
	}
	
	virtual IServerNetworkable *Create(const char *pClassName) override
	{
		//#error TODO
		T *pEnt = _CreateEntityTemplate<T>(nullptr, pClassName);
		return pEnt->NetworkProp();
	}
	
	virtual void Destroy(IServerNetworkable *pNetworkable) override
	{
		if (pNetworkable != nullptr) {
			pNetworkable->Release();
		}
	}
	
	virtual size_t GetEntitySize() override
	{
		return sizeof(T);
	}
};

#define LINK_ENTITY_TO_CLASS(mapClassName, DLLClassName) \
	static CEntityFactory<DLLClassName> mapClassName(#mapClassName);


#endif
