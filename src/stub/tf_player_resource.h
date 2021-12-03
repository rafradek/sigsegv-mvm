#ifndef _INCLUDE_SIGSEGV_STUB_TF_PLAYER_RESOURCE_H_
#define _INCLUDE_SIGSEGV_STUB_TF_PLAYER_RESOURCE_H_


#include "stub/baseentity.h"
#include "prop.h"
#include "link/link.h"

class CPlayerResource : public CBaseEntity
{
public:
	DECL_SENDPROP_RW(int[34],          m_iTeam);
	DECL_SENDPROP_RW(int[34],          m_bValid);
	
private:
	
};

class CTFPlayerResource : public CPlayerResource
{
public:

private:

};


extern GlobalThunk<CPlayerResource *> g_pPlayerResource;
inline CPlayerResource *PlayerResource()   { return g_pPlayerResource; }
inline CTFPlayerResource       *TFPlayerResource() { return reinterpret_cast<CTFPlayerResource *>(PlayerResource()); }


#endif
