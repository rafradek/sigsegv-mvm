#include "stub/projectiles.h"
#include "mem/extract.h"

#if defined _LINUX

static constexpr uint8_t s_Buf_CTFProjectile_Arrow_ArrowTouch[] = {
	0x55,                                           // +0000  push ebp
	0x89, 0xe5,                                     // +0001  mov ebp,esp
	0x57,                                           // +0003  push edi
	0x56,                                           // +0004  push esi
	0x53,                                           // +0005  push ebx
	0x81, 0xec, 0xbc, 0x01, 0x00, 0x00,             // +0006  sub esp,0x1bc
	0xa1, 0x30, 0x98, 0x77, 0x01,                   // +000C  mov eax,ds:gpGlobals
	0x8b, 0x5d, 0x08,                               // +0011  mov ebx,[ebp+this]
	0x8b, 0x7d, 0x0c,                               // +0014  mov edi,[ebp+arg_4]
	0xf3, 0x0f, 0x10, 0x40, 0x0c,                   // +0017  movss xmm0,dword ptr [eax+0xc]
	0xf3, 0x0f, 0x5c, 0x83, 0x10, 0x05, 0x00, 0x00, // +001C  subss xmm0,dword ptr [ebx+0xVVVVVVVV]
	0x0f, 0x2f, 0x05, 0xe4, 0x51, 0x1a, 0x01,       // +0024  comiss xmm0,ds:0xXXXXXXXX
	0x0f, 0x83, 0x9f, 0x03, 0x00, 0x00,             // +002B  jnb +0xXXXXXXXX
};

struct CExtract_CTFProjectile_Arrow_ArrowTouch : public IExtract<float *>
{
	using T = float *;
	
	CExtract_CTFProjectile_Arrow_ArrowTouch() : IExtract<T>(sizeof(s_Buf_CTFProjectile_Arrow_ArrowTouch)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFProjectile_Arrow_ArrowTouch);
		
		buf.SetDword(0x0c + 1, (uint32_t)AddrManager::GetAddr("gpGlobals"));
		
		mask.SetRange(0x1c + 4, 4, 0x00);
		mask.SetRange(0x24 + 3, 4, 0x00);
		mask.SetRange(0x2b + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFProjectile_Arrow::ArrowTouch"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x001c + 4; }
};

#elif defined _WINDOWS

using CExtract_CTFProjectile_Arrow_ArrowTouch = IExtractStub;

#endif


IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseProjectile, m_hOriginalLauncher, CBaseProjectile);

MemberVFuncThunk<const CBaseProjectile *, int> CBaseProjectile::vt_GetProjectileType(TypeName<CBaseProjectile>(), "CBaseProjectile::GetProjectileType");
MemberVFuncThunk<const CBaseProjectile *, bool> CBaseProjectile::vt_IsDestroyable(TypeName<CBaseProjectile>(), "CBaseProjectile::IsDestroyable");
MemberVFuncThunk<const CBaseProjectile *, void, bool, bool> CBaseProjectile::vt_Destroy(TypeName<CBaseProjectile>(), "CBaseProjectile::Destroy");
MemberVFuncThunk<CBaseProjectile *, void, CBaseEntity*> CBaseProjectile::vt_SetLauncher(TypeName<CBaseProjectile>(), "CBaseProjectile::SetLauncher");

MemberVFuncThunk<CTFBaseProjectile *, void, float> CTFBaseProjectile::vt_SetDamage(TypeName<CTFBaseProjectile>(), "CTFBaseProjectile::SetDamage");
MemberVFuncThunk<CTFBaseProjectile *, float> CTFBaseProjectile::vt_GetDamage(TypeName<CTFBaseProjectile>(), "CTFBaseProjectile::GetDamage");

MemberFuncThunk<CTFBaseProjectile *, void, CBaseEntity *> CTFBaseProjectile::ft_SetScorer("CTFBaseProjectile::SetScorer");

IMPL_SENDPROP(Vector,               CTFBaseRocket, m_vInitialVelocity, CTFBaseRocket);
IMPL_SENDPROP(int,                  CTFBaseRocket, m_iDeflected,       CTFBaseRocket);
IMPL_SENDPROP(CHandle<CBaseEntity>, CTFBaseRocket, m_hLauncher,        CTFBaseRocket);

MemberVFuncThunk<CTFBaseRocket *, void, trace_t *, CBaseEntity *> CTFBaseRocket::vt_Explode(TypeName<CTFBaseRocket>(), "CTFBaseRocket::Explode");

MemberFuncThunk<const CTFBaseRocket *, CBasePlayer *> CTFBaseRocket::ft_GetOwnerPlayer("CTFBaseRocket::GetOwnerPlayer");

MemberFuncThunk<CTFProjectile_EnergyBall *, void, CBaseEntity *> CTFProjectile_EnergyBall::ft_SetScorer("CTFProjectile_EnergyBall::SetScorer");


MemberFuncThunk<CTFProjectile_Flare *, void, CBaseEntity *> CTFProjectile_Flare::ft_SetScorer("CTFProjectile_Flare::SetScorer");


MemberFuncThunk<CTFProjectile_EnergyRing *, float> CTFProjectile_EnergyRing::ft_GetInitialVelocity("CTFProjectile_EnergyRing::GetInitialVelocity");

MemberFuncThunk<CTFProjectile_Rocket *, void, CBaseEntity *> CTFProjectile_Rocket::ft_SetScorer("CTFProjectile_Rocket::SetScorer");

IMPL_SENDPROP(bool, CTFProjectile_Rocket, m_bCritical, CTFProjectile_Rocket);


IMPL_EXTRACT(float, CTFProjectile_Arrow, m_flTimeInit, new CExtract_CTFProjectile_Arrow_ArrowTouch());
IMPL_SENDPROP(bool, CTFProjectile_Arrow, m_bCritical, CTFProjectile_Arrow);
IMPL_RELATIVE(CUtlVector<int>, CTFProjectile_Arrow, m_HitEntities, m_flTimeInit, -sizeof(CUtlVector<int>));

MemberFuncThunk<CTFProjectile_Arrow *, void, CBaseEntity *> CTFProjectile_Arrow::ft_SetScorer("CTFProjectile_Arrow::SetScorer");

//MemberFuncThunk<const CTFProjectile_Arrow *, bool> CTFProjectile_Arrow::ft_CanPenetrate("CTFBaseRocket::GetOwnerPlayer");
//MemberFuncThunk<CTFProjectile_Arrow *, void, bool> CTFProjectile_Arrow::ft_SetPenetrate("CTFBaseRocket::GetOwnerPlayer");

StaticFuncThunk<CTFProjectile_Arrow *,const Vector &, const QAngle &, const float, const float, int, CBaseEntity *, CBaseEntity *> CTFProjectile_Arrow::ft_Create("CTFProjectile_Arrow::Create");

IMPL_SENDPROP(int,    CTFWeaponBaseGrenadeProj, m_iDeflected,       CTFWeaponBaseGrenadeProj);
IMPL_SENDPROP(bool,   CTFWeaponBaseGrenadeProj, m_bCritical,        CTFWeaponBaseGrenadeProj);
IMPL_SENDPROP(Vector, CTFWeaponBaseGrenadeProj, m_vInitialVelocity, CTFWeaponBaseGrenadeProj);

MemberFuncThunk<CTFWeaponBaseGrenadeProj *, void, const Vector &, const Vector &, CBaseCombatCharacter *, const CTFWeaponInfo &> CTFWeaponBaseGrenadeProj::ft_InitGrenade("CTFWeaponBaseGrenadeProj::InitGrenade");

MemberVFuncThunk<const CTFWeaponBaseGrenadeProj *, int> CTFWeaponBaseGrenadeProj::vt_GetWeaponID(TypeName<CTFWeaponBaseGrenadeProj>(), "CTFWeaponBaseGrenadeProj::GetWeaponID");

MemberFuncThunk<const CTFWeaponBaseGrenadeProj *, void, float> CTFWeaponBaseGrenadeProj::ft_SetDetonateTimerLength("CTFWeaponBaseGrenadeProj::SetDetonateTimerLength");
MemberFuncThunk<CTFWeaponBaseGrenadeProj *, void, trace_t *, int> CTFWeaponBaseGrenadeProj::ft_Explode("CTFWeaponBaseGrenadeProj::Explode");


MemberVFuncThunk<CTFGrenadePipebombProjectile *, void, int> CTFGrenadePipebombProjectile::vt_SetPipebombMode(TypeName<CTFGrenadePipebombProjectile>(), "CTFGrenadePipebombProjectile::SetPipebombMode");

IMPL_SENDPROP(CHandle<CBaseEntity>, CTFGrenadePipebombProjectile, m_hLauncher, CTFGrenadePipebombProjectile);
IMPL_SENDPROP(bool, CTFGrenadePipebombProjectile, m_bTouched, CTFGrenadePipebombProjectile);
IMPL_SENDPROP(int, CTFGrenadePipebombProjectile, m_iType, CTFGrenadePipebombProjectile);

MemberVFuncThunk<CTFProjectile_Throwable *, Vector, const Vector &, const Vector &, const Vector &, float> CTFProjectile_Throwable::vt_GetVelocityVector(TypeName<CTFProjectile_Throwable>(), "CTFProjectile_Throwable::GetVelocityVector");
MemberVFuncThunk<CTFProjectile_Throwable *, const AngularImpulse> CTFProjectile_Throwable::vt_GetAngularImpulse(TypeName<CTFProjectile_Throwable>(), "CTFProjectile_Throwable::GetAngularImpulse");

StaticFuncThunk<CTFStunBall *,const Vector &, const QAngle &, CBaseEntity *> CTFStunBall::ft_Create("CTFStunBall::Create");
StaticFuncThunk<CTFBall_Ornament *,const Vector &, const QAngle &, CBaseEntity *> CTFBall_Ornament::ft_Create("CTFBall_Ornament::Create");
StaticFuncThunk<CTFProjectile_Jar *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> CTFProjectile_Jar::ft_Create("CTFProjectile_Jar::Create");
StaticFuncThunk<CTFProjectile_JarMilk *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> CTFProjectile_JarMilk::ft_Create("CTFProjectile_JarMilk::Create");
StaticFuncThunk<CTFProjectile_Cleaver *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &, int> CTFProjectile_Cleaver::ft_Create("CTFProjectile_Cleaver::Create");
StaticFuncThunk<CTFProjectile_JarGas *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> CTFProjectile_JarGas::ft_Create("CTFProjectile_JarGas::Create");


GlobalThunk<CUtlVector<IBaseProjectileAutoList *>> IBaseProjectileAutoList::m_IBaseProjectileAutoListAutoList("IBaseProjectileAutoList::m_IBaseProjectileAutoListAutoList");
