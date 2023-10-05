#ifndef _INCLUDE_SIGSEGV_STUB_OBJECTS_H_
#define _INCLUDE_SIGSEGV_STUB_OBJECTS_H_


#include "stub/baseentity.h"
#include "stub/tfplayer.h"


class CBaseObject : public CBaseCombatCharacter
{
public:
	int GetType() const               { return this->m_iObjectType; }
	int GetObjectMode() const         { return this->m_iObjectMode; }
	CTFPlayer *GetBuilder() const     { return this->m_hBuilder; }
	CBaseEntity *GetBuiltOnEntity() const      { return this->m_hBuiltOnEntity; }
	void SetBuiltOnEntity(CBaseEntity *entity) { this->m_hBuiltOnEntity = entity; }
	
	// avoid situations where we accidentally do the wrong thing
	void SetHealth(int amt) = delete;
	
	void SetHealth(float amt)              {        ft_SetHealth        (this, amt); }
	void SetPlasmaDisabled(float duration) {        ft_SetPlasmaDisabled(this, duration); }
	bool HasSapper()                       { return ft_HasSapper        (this); }

	bool FindBuildPointOnPlayer(CTFPlayer *pTFPlayer, CBasePlayer *pBuilder, float &flNearestPoint, Vector &vecNearestBuildPoint) { return ft_FindBuildPointOnPlayer(this, pTFPlayer, pBuilder, flNearestPoint, vecNearestBuildPoint); }
	void AttachObjectToObject(CBaseEntity *pEntity, int iPoint, Vector &vecOrigin)                                                {        ft_AttachObjectToObject(this, pEntity, iPoint, vecOrigin); }

	void StartPlacement(CTFPlayer *pPlayer)   {        vt_StartPlacement               (this, pPlayer); }
	bool StartBuilding(CBaseEntity *pBuilder) { return vt_StartBuilding                (this, pBuilder); }
	void DetonateObject()                     {        vt_DetonateObject               (this); }
	void InitializeMapPlacedObject()          {        vt_InitializeMapPlacedObject    (this); }
	void FinishedBuilding()                   {        vt_FinishedBuilding             (this); }
	int GetMiniBuildingStartingHealth()       { return vt_GetMiniBuildingStartingHealth(this); }
	int GetMaxHealthForCurrentLevel()         { return vt_GetMaxHealthForCurrentLevel  (this); }

	bool IsFunctional()                    { return !m_bBuilding && !m_bDisabled && !m_bPlacing && !m_bCarried; }
	bool IsTargetable()                    { return !m_bPlacing && !m_bCarried; }
	
	DECL_DATAMAP(int, m_nDefaultUpgradeLevel);
	
	DECL_SENDPROP(int,                m_iUpgradeLevel);
	DECL_SENDPROP(bool,               m_bMiniBuilding);
	DECL_SENDPROP(bool,               m_bDisposableBuilding);
	DECL_SENDPROP(bool,               m_bBuilding);
	DECL_SENDPROP(bool,               m_bDisabled);
	DECL_SENDPROP(bool,               m_bPlacing);
	DECL_SENDPROP(bool,               m_bCarried);
	DECL_SENDPROP(bool,               m_bCarryDeploy);
	DECL_SENDPROP(int,                m_iKills);
	DECL_SENDPROP(Vector,             m_vecBuildMaxs);
	DECL_RELATIVE(Vector,             m_vecBuildOrigin);
	
	
private:
	DECL_SENDPROP(int,                m_iObjectType);
	DECL_SENDPROP(int,                m_iObjectMode);
	DECL_SENDPROP(CHandle<CTFPlayer>, m_hBuilder);
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hBuiltOnEntity);
	
	static MemberFuncThunk<CBaseObject *, void, float> ft_SetHealth;
	static MemberFuncThunk<CBaseObject *, void, float> ft_SetPlasmaDisabled;
	static MemberFuncThunk<CBaseObject *, bool>        ft_HasSapper;

	static MemberFuncThunk<CBaseObject *, bool, CTFPlayer *, CBasePlayer *, float &, Vector &> ft_FindBuildPointOnPlayer;
	static MemberFuncThunk<CBaseObject *, void, CBaseEntity *, int, Vector &> ft_AttachObjectToObject;
	
	static MemberVFuncThunk<CBaseObject *, void, CTFPlayer *>   vt_StartPlacement;
	static MemberVFuncThunk<CBaseObject *, bool, CBaseEntity *> vt_StartBuilding;
	static MemberVFuncThunk<CBaseObject *, void>                vt_DetonateObject;
	static MemberVFuncThunk<CBaseObject *, void>                vt_InitializeMapPlacedObject;
	static MemberVFuncThunk<CBaseObject *, void>                vt_FinishedBuilding;
	static MemberVFuncThunk<CBaseObject *, int>                 vt_GetMiniBuildingStartingHealth;
	static MemberVFuncThunk<CBaseObject *, int>                 vt_GetMaxHealthForCurrentLevel;
};


class IBaseObjectAutoList
{
public:
	static const CUtlVector<IBaseObjectAutoList *>& AutoList() { return m_IBaseObjectAutoListAutoList; }
private:
	static GlobalThunk<CUtlVector<IBaseObjectAutoList *>> m_IBaseObjectAutoListAutoList;
};


class CObjectDispenser : public CBaseObject
{
public:
	float GetDispenserRadius() { return vt_GetDispenserRadius(this); }
	
private:
	static MemberVFuncThunk<CObjectDispenser *, float> vt_GetDispenserRadius;
};

class CObjectCartDispenser : public CObjectDispenser {};


class CBaseObjectUpgrade : public CBaseObject {};
class CObjectSapper : public CBaseObjectUpgrade {};


class CObjectSentrygun : public CBaseObject {
public:
	QAngle &GetTurretAngles()         { return vt_GetTurretAngles  (this); }
	void SentryThink()         { ft_SentryThink  (this); }
	DECL_EXTRACT(float, m_flNextRocketFire);
	DECL_SENDPROP(int, m_iAmmoShells);
	DECL_RELATIVE(int, m_iMaxAmmoShells);
	DECL_SENDPROP(int, m_iAmmoRockets);
	DECL_RELATIVE(int, m_iMaxAmmoRockets);
    DECL_RELATIVE(unsigned int, m_nShieldLevel);
	DECL_SENDPROP(int, m_iState);
	DECL_RELATIVE(int, m_flSentryRange);
	DECL_SENDPROP(int, m_bPlayerControlled);
	DECL_RELATIVE(float, m_flFireRate);
	DECL_RELATIVE(float, m_flNextAttack);
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hEnemy);
	DECL_RELATIVE(float, m_flNextRocketAttack);
private:
	static MemberVFuncThunk<CObjectSentrygun *, QAngle &> vt_GetTurretAngles;
	static MemberFuncThunk<CObjectSentrygun *, void> ft_SentryThink;
};


class CObjectTeleporter : public CBaseObject {
public:
	DECL_DATAMAP(int, m_iTeleportType);
	DECL_SENDPROP(float, m_flRechargeTime);
	DECL_SENDPROP(float, m_flCurrentRechargeDuration);
	DECL_SENDPROP(int, m_iState);
};


/* Source2007: game/shared/tf/ihasbuildpoints.h */
class IHasBuildPoints
{
public:
	// Tell me how many build points you have
	virtual int			GetNumBuildPoints( void ) const = 0;
	// Give me the origin & angles of the specified build point
	virtual bool		GetBuildPoint( int iPoint, Vector &vecOrigin, QAngle &vecAngles ) = 0;
	// If the build point wants to parent built objects to an attachment point on the entity,
	// it'll return a value >= 1 here specifying which attachment to sit on.
	virtual int			GetBuildPointAttachmentIndex( int iPoint ) const = 0;
	// Can I build the specified object on the specified build point?
	virtual bool		CanBuildObjectOnBuildPoint( int iPoint, int iObjectType ) = 0;
	// I've finished building the specified object on the specified build point
	virtual void		SetObjectOnBuildPoint( int iPoint, CBaseObject *pObject ) = 0;
	// Get the number of objects build on this entity
	virtual int			GetNumObjectsOnMe( void ) = 0;
	// Get the first object of type, return NULL if no such type available
	virtual CBaseObject *GetObjectOfTypeOnMe( int iObjectType ) = 0;
	// Remove all objects built on me
	virtual void		RemoveAllObjects( void ) = 0;
	// Return the maximum distance that this entity's build points can be snapped to
	virtual float		GetMaxSnapDistance( int iPoint ) = 0;
	// Return true if it's possible that build points on this entity may move in local space (i.e. due to animation)
	virtual bool		ShouldCheckForMovement( void ) = 0;
	// I've finished building the specified object on the specified build point
	virtual int			FindObjectOnBuildPoint( CBaseObject *pObject ) = 0;
};

inline CBaseObject *ToBaseObject(CBaseEntity *pEntity)
{
	if (pEntity == nullptr)   return nullptr;
	if (!pEntity->IsBaseObject()) return nullptr;
	
	return static_cast<CBaseObject *>(pEntity);
}

#endif
