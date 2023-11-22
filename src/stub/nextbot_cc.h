#ifndef _INCLUDE_SIGSEGV_STUB_NEXTBOT_CC_H_
#define _INCLUDE_SIGSEGV_STUB_NEXTBOT_CC_H_


#include "link/link.h"
#include "stub/baseplayer.h"
#include "re/nextbot.h"

class CPathTrack;


class NextBotCombatCharacter : public CBaseCombatCharacter {
public:
	ILocomotion *GetLocomotionInterface() const                        { return this->MyNextBotPointer()->GetLocomotionInterface(); }
	IBody *GetBodyInterface() const                                    { return this->MyNextBotPointer()->GetBodyInterface(); }
	IVision *GetVisionInterface() const                                { return this->MyNextBotPointer()->GetVisionInterface(); }
	IIntention *GetIntentionInterface() const                          { return this->MyNextBotPointer()->GetIntentionInterface(); }

private:
};


class CTFBaseBoss : public NextBotCombatCharacter
{
public:
	void UpdateCollisionBounds() { vt_UpdateCollisionBounds(this); }
	int GetCurrencyValue() { return vt_GetCurrencyValue(this); }
	
private:
	static MemberVFuncThunk<CTFBaseBoss *, void> vt_UpdateCollisionBounds;
	static MemberVFuncThunk<CTFBaseBoss *, int> vt_GetCurrencyValue;
};

class CTFTankBoss : public CTFBaseBoss
{
public:
	DECL_EXTRACT (IBody *,             m_pBodyInterface);
	DECL_RELATIVE(CHandle<CPathTrack>, m_hCurrentNode);
	DECL_RELATIVE(CUtlVector<float>,   m_NodeDists);
	DECL_RELATIVE(float,               m_flTotalDistance);
	DECL_RELATIVE(int,                 m_iCurrentNode);
	DECL_RELATIVE(int,                 m_iModelIndex);
};


class CHalloweenBaseBoss : public NextBotCombatCharacter
{
public:
	enum HalloweenBossType : int32_t
	{
		INVALID         = 0,
		HEADLESS_HATMAN = 1,
		EYEBALL_BOSS    = 2,
		MERASMUS        = 3,
	};
	
	static CHalloweenBaseBoss *SpawnBossAtPos(HalloweenBossType type, const Vector& pos, int team, CBaseEntity *owner) { return ft_SpawnBossAtPos(type, pos, team, owner); }
	
private:
	static StaticFuncThunk<CHalloweenBaseBoss *, HalloweenBossType, const Vector&, int, CBaseEntity *> ft_SpawnBossAtPos;
};

class CHeadlessHatman : public CHalloweenBaseBoss {};
class CMerasmus       : public CHalloweenBaseBoss {};
class CEyeballBoss    : public CHalloweenBaseBoss {};


class CZombie : public NextBotCombatCharacter
{
public:
	enum SkeletonType_t : int32_t
	{
		SKELETON_NORMAL = 0,
		SKELETON_KING   = 1,
		SKELETON_SMALL  = 2,
	};
	
	ILocomotion *GetLocomotionInterface() const                        { return this->m_pLocomotionInterface; }
	IBody *GetBodyInterface() const                                    { return this->m_pBodyInterface; }
	IIntention *GetIntentionInterface() const                          { return this->m_pIntentionInterface; }

	static CZombie *SpawnAtPos(const Vector& pos, float duration, int team, CBaseEntity *owner, SkeletonType_t type) { return ft_SpawnAtPos(pos, duration, team, owner, type); }
	
	DECL_EXTRACT (IIntention *, m_pIntentionInterface);
	DECL_RELATIVE(ILocomotion *, m_pLocomotionInterface);
	DECL_RELATIVE(IBody *, m_pBodyInterface);
	// ???                 +0x8BC
	// m_pIntention        +0x8C0
	// m_pLocomotion       +0x8C4
	// m_pBody             +0x8C8
	// m_iSkeletonType     +0x8CC
	DECL_RELATIVE(SkeletonType_t, m_iSkeletonType);
	DECL_SENDPROP(float,          m_flHeadScale);
	// m_hHat              +0x8D4
	// ??? float           +0x8D8
	// ??? float           +0x8DC
	// ??? byte            +0x8E0
	// m_bOverLimitSuicide +0x8E1
	// m_ctLifetime        +0x8E4
	
private:
	static StaticFuncThunk<CZombie *, const Vector&, float, int, CBaseEntity *, SkeletonType_t> ft_SpawnAtPos;
};

class IZombieAutoList
{
public:
	static const CUtlVector<IZombieAutoList *>& AutoList() { return m_IZombieAutoListAutoList; }
private:
	static GlobalThunk<CUtlVector<IZombieAutoList *>> m_IZombieAutoListAutoList;
};

class CBotNPCArcher : public NextBotCombatCharacter
{	
public:
	ILocomotion *GetLocomotionInterface() const                        { return this->m_pLocomotionInterface; }
	IBody *GetBodyInterface() const                                    { return this->m_pBodyInterface; }
	IIntention *GetIntentionInterface() const                          { return this->m_pIntentionInterface; }
	
	DECL_EXTRACT (IIntention *, m_pIntentionInterface);
	DECL_RELATIVE(ILocomotion *, m_pLocomotionInterface);
	DECL_RELATIVE(IBody *, m_pBodyInterface);
	DECL_RELATIVE(CBaseAnimating *, m_bow);
	DECL_RELATIVE(Vector, m_eyeOffset);
};
#endif
