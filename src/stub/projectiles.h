#ifndef _INCLUDE_SIGSEGV_STUB_PROJECTILES_H_
#define _INCLUDE_SIGSEGV_STUB_PROJECTILES_H_


#include "link/link.h"
#include "stub/baseanimating.h"


class CBaseProjectile : public CBaseAnimating
{
public:
	CBaseEntity *GetOriginalLauncher() const { return this->m_hOriginalLauncher; }

	int GetProjectileType() const            { return vt_GetProjectileType(this); }
#ifdef SE_TF2
	bool IsDestroyable(bool flag) const            { return vt_IsDestroyable(this, flag); }
#else
	bool IsDestroyable() const            { return vt_IsDestroyable(this); }
#endif
	void Destroy(bool blinkOut = true, bool breakRocket = false) const            { return vt_Destroy(this, blinkOut, breakRocket); }
	void SetLauncher(CBaseEntity *launcher)  { vt_SetLauncher(this, launcher); }
	bool CanCollideWithTeammates()  { return vt_CanCollideWithTeammates(this); }
	
private:
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hOriginalLauncher);
	
	static MemberVFuncThunk<const CBaseProjectile *, int> vt_GetProjectileType;
#ifdef SE_TF2
	static MemberVFuncThunk<const CBaseProjectile *, bool, bool> vt_IsDestroyable;
#else
	static MemberVFuncThunk<const CBaseProjectile *, bool> vt_IsDestroyable;
#endif
	static MemberVFuncThunk<const CBaseProjectile *, void, bool, bool> vt_Destroy;
	static MemberVFuncThunk<CBaseProjectile *, void, CBaseEntity*> vt_SetLauncher;
	static MemberVFuncThunk<CBaseProjectile *, bool> vt_CanCollideWithTeammates;
};

class CBaseGrenade : public CBaseProjectile 
{
public:
	CBaseEntity *GetThrower() const { return ft_GetThrower(this); }
	void SetThrower(CBaseEntity *entity) { ft_SetThrower(this, entity); }

private:
	static MemberFuncThunk<const CBaseGrenade *, CBaseEntity *> ft_GetThrower;
	static MemberFuncThunk<CBaseGrenade *, void, CBaseEntity *> ft_SetThrower;
};

#ifdef SE_TF2
class CThrownGrenade : public CBaseGrenade {};
class CBaseGrenadeConcussion : public CBaseGrenade {};
class CBaseGrenadeContact : public CBaseGrenade {};
class CBaseGrenadeTimed : public CBaseGrenade {};

class CTFBaseProjectile : public CBaseProjectile {
public:
	void SetDamage(float damage) { vt_SetDamage(this, damage); }
	float GetDamage() { return vt_GetDamage(this); }

	void SetScorer(CBaseEntity *scorer) { ft_SetScorer(this, scorer); }
private:
	static MemberVFuncThunk<CTFBaseProjectile *, void, float> vt_SetDamage;
	static MemberVFuncThunk<CTFBaseProjectile *, float> vt_GetDamage;
	
	static MemberFuncThunk<CTFBaseProjectile *, void, CBaseEntity *> ft_SetScorer;
};

class CTFBaseRocket  : public CBaseProjectile
{
public:
	CBaseEntity *GetLauncher() const { return this->m_hLauncher; }

	void Explode( trace_t *pTrace, CBaseEntity *pOther ) {        vt_Explode(this, pTrace, pOther); }
	int GetWeaponID() const                              { return vt_GetWeaponID(this); }

	CBasePlayer *GetOwnerPlayer() const { return ft_GetOwnerPlayer(this); }
	
	DECL_SENDPROP(Vector, m_vInitialVelocity);
	DECL_SENDPROP(int,    m_iDeflected);
	
private:
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hLauncher);
	
	static MemberVFuncThunk<CTFBaseRocket *, void, trace_t *, CBaseEntity *> vt_Explode;
	static MemberVFuncThunk<const CTFBaseRocket *, int>                      vt_GetWeaponID;

	static MemberFuncThunk<const CTFBaseRocket *, CBasePlayer *> ft_GetOwnerPlayer;
};

class CTFFlameRocket : public CTFBaseRocket {};

class CTFProjectile_Syringe : public CTFBaseProjectile {};

class CTFProjectile_EnergyRing : public CTFBaseProjectile
{
public:
	float GetInitialVelocity()  { return ft_GetInitialVelocity(this); }

private:
	static MemberFuncThunk<CTFProjectile_EnergyRing *, float> ft_GetInitialVelocity;
};

class CTFProjectile_Rocket : public CTFBaseRocket
{
public:
	bool IsCritical() const          { return this->m_bCritical; }
	void SetCritical(bool bCritical) { this->m_bCritical = bCritical; }

	void SetScorer(CBaseEntity *scorer) { ft_SetScorer(this, scorer); }
	
private:
	DECL_SENDPROP(bool, m_bCritical);

	static MemberFuncThunk<CTFProjectile_Rocket *, void, CBaseEntity *> ft_SetScorer;
};
class CTFProjectile_SentryRocket : public CTFProjectile_Rocket {};

class CTFProjectile_BallOfFire : public CTFProjectile_Rocket {};

class CTFProjectile_Flare : public CTFBaseRocket {
public:
	void SetScorer(CBaseEntity *scorer) { ft_SetScorer(this, scorer); }
private:
	static MemberFuncThunk<CTFProjectile_Flare *, void, CBaseEntity *> ft_SetScorer;
};
class CTFProjectile_EnergyBall : public CTFBaseRocket 
{
public:
	void SetScorer(CBaseEntity *scorer) { ft_SetScorer(this, scorer); }
private:
	static MemberFuncThunk<CTFProjectile_EnergyBall *, void, CBaseEntity *> ft_SetScorer;
};

class CTFProjectile_Arrow : public CTFBaseRocket
{
public:

	//bool CanPenetrate() const { return ft_CanPenetrate(this); }
	//void SetPenetrate(bool penetrate) { ft_SetPenetrate(this, penetrate); }

	DECL_EXTRACT(float, m_flTimeInit);
	DECL_SENDPROP(bool, m_bCritical);
	DECL_RELATIVE(CUtlVector<int>, m_HitEntities);
	DECL_RELATIVE(EHANDLE, m_pTrail);
	DECL_RELATIVE(bool, m_bPenetrate);
	DECL_SENDPROP(bool, m_bArrowAlight);
	

	void SetScorer(CBaseEntity *scorer) { ft_SetScorer(this, scorer); }

	static CTFProjectile_Arrow *Create(const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, int projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer) {return ft_Create(vecOrigin, vecAngles, fSpeed, fGravity, projectileType, pOwner, pScorer);}
private:
	static StaticFuncThunk<CTFProjectile_Arrow *,const Vector &, const QAngle &, const float, const float, int, CBaseEntity *, CBaseEntity *> ft_Create;

	static MemberFuncThunk<CTFProjectile_Arrow *, void, CBaseEntity *> ft_SetScorer;
	//static MemberFuncThunk<const CTFProjectile_Arrow *, bool> ft_CanPenetrate;
	//static MemberFuncThunk<CTFProjectile_Arrow *, void, bool> ft_SetPenetrate;
};

class CTFProjectile_HealingBolt : public CTFProjectile_Arrow {};
class CTFProjectile_GrapplingHook : public CTFProjectile_Arrow {};

class CTFWeaponInfo;

class CTFWeaponBaseGrenadeProj : public CBaseGrenade
{
public:
	int GetWeaponID() const { return vt_GetWeaponID(this); }

	void SetDetonateTimerLength(float time) const { ft_SetDetonateTimerLength(this, time); }
	void Explode(trace_t *pTrace, int bitsDamageType)  { ft_Explode(this, pTrace, bitsDamageType); }
	void InitGrenade(const Vector &velocity, const Vector &impulse, CBaseCombatCharacter *owner, const CTFWeaponInfo &info)  { ft_InitGrenade(this, velocity, impulse, owner, info); }
	

	DECL_SENDPROP(int,    m_iDeflected);
	DECL_SENDPROP(bool,   m_bCritical);
	DECL_SENDPROP(Vector, m_vInitialVelocity);
	
private:
	static MemberVFuncThunk<const CTFWeaponBaseGrenadeProj *, int> vt_GetWeaponID;

	static MemberFuncThunk<const CTFWeaponBaseGrenadeProj *, void, float> ft_SetDetonateTimerLength;
	static MemberFuncThunk<CTFWeaponBaseGrenadeProj *, void, trace_t *, int> ft_Explode;
	static MemberFuncThunk<CTFWeaponBaseGrenadeProj *, void, const Vector &, const Vector &, CBaseCombatCharacter *, const CTFWeaponInfo &> ft_InitGrenade;
};

class CTFGrenadePipebombProjectile : public CTFWeaponBaseGrenadeProj
{
public:
	CBaseEntity *GetLauncher() const { return this->m_hLauncher; }

	void SetPipebombMode(int mode = 0) { return vt_SetPipebombMode(this, mode); }

	DECL_SENDPROP(bool, m_bTouched);
	DECL_SENDPROP(int, m_iType);
	DECL_SENDPROP(bool, m_bDefensiveBomb);
	DECL_RELATIVE(float, m_flFullDamage);
	
private:
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hLauncher);
	
	static MemberVFuncThunk<CTFGrenadePipebombProjectile *, void, int> vt_SetPipebombMode;
};
class CTFProjectile_MechanicalArmOrb : public CTFProjectile_Rocket {};
class CTFWeaponBaseMerasmusGrenade : public CTFWeaponBaseGrenadeProj {};

class CTFProjectile_Jar : public CTFGrenadePipebombProjectile 
{
public:
	static CTFProjectile_Jar *Create(const Vector &position, const QAngle &angles, 
												const Vector &velocity, const AngularImpulse &angVelocity, 
												CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo) {return ft_Create(position, angles, velocity, angVelocity, pOwner, weaponInfo);}
private:
	static StaticFuncThunk<CTFProjectile_Jar *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> ft_Create;
};
class CTFProjectile_JarMilk : public CTFProjectile_Jar 
{
public:
	static CTFProjectile_JarMilk *Create(const Vector &position, const QAngle &angles, 
												const Vector &velocity, const AngularImpulse &angVelocity, 
												CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo) {return ft_Create(position, angles, velocity, angVelocity, pOwner, weaponInfo);}
private:
	static StaticFuncThunk<CTFProjectile_JarMilk *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> ft_Create;
};
class CTFProjectile_Cleaver : public CTFProjectile_Jar 
{
public:
	static CTFProjectile_Cleaver *Create(const Vector &position, const QAngle &angles, 
												const Vector &velocity, const AngularImpulse &angVelocity, 
												CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int skin) {return ft_Create(position, angles, velocity, angVelocity, pOwner, weaponInfo, skin);}
private:
	static StaticFuncThunk<CTFProjectile_Cleaver *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &, int> ft_Create;
};
class CTFProjectile_JarGas : public CTFProjectile_Jar 
{
public:
	static CTFProjectile_JarGas *Create(const Vector &position, const QAngle &angles, 
												const Vector &velocity, const AngularImpulse &angVelocity, 
												CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo) {return ft_Create(position, angles, velocity, angVelocity, pOwner, weaponInfo);}
private:
	static StaticFuncThunk<CTFProjectile_JarGas *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> ft_Create;
};

class CTFProjectile_Throwable : public CTFProjectile_Jar 
{
public:
	Vector GetVelocityVector(const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, float flCharge) { return vt_GetVelocityVector(this, vecForward, vecRight, vecUp, flCharge); }
	const AngularImpulse GetAngularImpulse() { return vt_GetAngularImpulse(this); }
	

private:
	static MemberVFuncThunk<CTFProjectile_Throwable *, Vector, const Vector &, const Vector &, const Vector &, float> vt_GetVelocityVector;
	static MemberVFuncThunk<CTFProjectile_Throwable *, const AngularImpulse> vt_GetAngularImpulse;
};
class CTFProjectile_ThrowableBreadMonster : public CTFProjectile_Throwable {};
class CTFProjectile_ThrowableBrick : public CTFProjectile_Throwable {};
class CTFProjectile_ThrowableRepel : public CTFProjectile_Throwable {};

class CTFStunBall : public CTFGrenadePipebombProjectile 
{
public:
	static CTFStunBall *Create(const Vector &vecOrigin, const QAngle &vecAngles,  CBaseEntity *pOwner) {return ft_Create(vecOrigin, vecAngles, pOwner); }
private:
	static StaticFuncThunk<CTFStunBall *,const Vector &, const QAngle &, CBaseEntity *> ft_Create;
};
class CTFBall_Ornament : public CTFStunBall
{
public:
	static CTFBall_Ornament *Create(const Vector &vecOrigin, const QAngle &vecAngles,  CBaseEntity *pOwner) {return ft_Create(vecOrigin, vecAngles, pOwner); }
private:
	static StaticFuncThunk<CTFBall_Ornament *,const Vector &, const QAngle &, CBaseEntity *> ft_Create;
};

class CTFProjectile_SpellFireball : public CTFProjectile_Rocket {};
class CTFProjectile_SpellBats : public CTFProjectile_Jar {};
class CTFProjectile_SpellMirv : public CTFProjectile_SpellBats {};
class CTFProjectile_SpellMeteorShower : public CTFProjectile_SpellBats {};
class CTFProjectile_SpellPumpkin : public CTFProjectile_SpellBats {};
class CTFProjectile_SpellTransposeTeleport : public CTFProjectile_SpellBats {};
class CTFProjectile_SpellSpawnBoss : public CTFProjectile_SpellBats {};
class CTFProjectile_SpellSpawnZombie : public CTFProjectile_SpellBats {};
class CTFProjectile_SpellSpawnHorde : public CTFProjectile_SpellBats {};
class CTFProjectile_SpellLightningOrb : public CTFProjectile_SpellFireball {};
class CTFProjectile_SpellKartOrb : public CTFProjectile_SpellFireball {};
class CTFProjectile_SpellKartBats : public CTFProjectile_SpellBats {};

#endif
class IBaseProjectileAutoList
{
public:
	static const CUtlVector<IBaseProjectileAutoList *>& AutoList() { return m_IBaseProjectileAutoListAutoList; }
private:
	static GlobalThunk<CUtlVector<IBaseProjectileAutoList *>> m_IBaseProjectileAutoListAutoList;
};


#endif
