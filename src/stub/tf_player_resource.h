#ifndef _INCLUDE_SIGSEGV_STUB_TF_PLAYER_RESOURCE_H_
#define _INCLUDE_SIGSEGV_STUB_TF_PLAYER_RESOURCE_H_


#include "stub/baseentity.h"
#include "prop.h"
#include "link/link.h"

class CPlayerResource : public CBaseEntity
{
public:
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iTeam);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_bValid);
	DECL_SENDPROP(uint32[MAX_PLAYERS + 1],          m_iAccountID);
	
	
private:
	
};

class CTFPlayerResource : public CPlayerResource
{
public:

	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iDamage);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iTotalScore);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iDamageBoss);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iHealing);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iCurrencyCollected);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iDamageAssist);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iHealingAssist);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iBonusPoints);
	DECL_SENDPROP(int[MAX_PLAYERS + 1],          m_iDamageBlocked);
	

private:

};


extern GlobalThunk<CPlayerResource *> g_pPlayerResource;
inline CPlayerResource *PlayerResource()   { return g_pPlayerResource; }
inline CTFPlayerResource       *TFPlayerResource() { return reinterpret_cast<CTFPlayerResource *>(PlayerResource()); }


#endif
