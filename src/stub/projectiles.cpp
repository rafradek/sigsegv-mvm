#include "stub/projectiles.h"
#include "mem/extract.h"

#ifdef SE_IS_TF2
#if defined _LINUX

static constexpr uint8_t s_Buf_CTFProjectile_Arrow_ArrowTouch[] = {
#ifdef PLATFORM_64BITS
	0x55,                                            // +0x0000 push    rbp
	0x48, 0x89, 0xe5,                                // +0x0001 mov     rbp, rsp
	0x41, 0x57,                                      // +0x0004 push    r15
	0x41, 0x56,                                      // +0x0006 push    r14
	0x49, 0x89, 0xf6,                                // +0x0008 mov     r14, rsi
	0x41, 0x55,                                      // +0x000b push    r13
	0x41, 0x54,                                      // +0x000d push    r12
	0x49, 0x89, 0xfc,                                // +0x000f mov     r12, rdi
	0x53,                                            // +0x0012 push    rbx
	0x48, 0x81, 0xec, 0x18, 0x02, 0x00, 0x00,        // +0x0013 sub     rsp, 218h
	0x48, 0x8d, 0x1d, 0xe7, 0x5b, 0xa3, 0x00,        // +0x001a lea     rbx, gpGlobals
	0x48, 0x8b, 0x03,                                // +0x0021 mov     rax, [rbx]
	0xf3, 0x0f, 0x10, 0x40, 0x0c,                    // +0x0024 movss   xmm0, dword ptr [rax+0Ch]
	0xf3, 0x0f, 0x5c, 0x87, 0xf0, 0x06, 0x00, 0x00,  // +0x0029 subss   xmm0, dword ptr [rdi+6F0h]
	0x0f, 0x2f, 0x05, 0xcc, 0xcc, 0x28, 0x00,        // +0x0031 comiss  xmm0, cs:dword_148A3F4
	0x0f, 0x83, 0xf2, 0x05, 0x00, 0x00,              // +0x0038 jnb     loc_11FDD20
#else
	0x55,                                           // +0000  push ebp
	0x89, 0xe5,                                     // +0001  mov ebp,esp
	0x57,                                           // +0003  push edi
	0x56,                                           // +0004  push esi
	0x53,                                           // +0005  push ebx
	0x81, 0xec, 0xbc, 0x01, 0x00, 0x00,             // +0006  sub esp,0x1bc
	0x8b, 0x5d, 0x08,                               // +000C  mov ebx,[ebp+this]
	0xa1, 0x30, 0x98, 0x77, 0x01,                   // +000F  mov eax,ds:gpGlobals
	0x8b, 0x7d, 0x0c,                               // +0014  mov edi,[ebp+arg_4]
	0xf3, 0x0f, 0x10, 0x40, 0x0c,                   // +0017  movss xmm0,dword ptr [eax+0xc]
	0xf3, 0x0f, 0x5c, 0x83, 0x10, 0x05, 0x00, 0x00, // +001C  subss xmm0,dword ptr [ebx+0xVVVVVVVV]
	0x0f, 0x2f, 0x05, 0xe4, 0x51, 0x1a, 0x01,       // +0024  comiss xmm0,ds:0xXXXXXXXX
	0x0f, 0x83, 0x9f, 0x03, 0x00, 0x00,             // +002B  jnb +0xXXXXXXXX
#endif
};

struct CExtract_CTFProjectile_Arrow_ArrowTouch : public IExtract<uint32_t>
{
	using T = uint32_t;
	
	CExtract_CTFProjectile_Arrow_ArrowTouch() : IExtract<T>(sizeof(s_Buf_CTFProjectile_Arrow_ArrowTouch)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFProjectile_Arrow_ArrowTouch);
#ifdef PLATFORM_64BITS
		buf.SetDword(0x1a + 3, AddrManager::GetAddrOffset("gpGlobals", "CTFProjectile_Arrow::ArrowTouch", 0x21));
		
		mask.SetRange(0x13 + 3, 4, 0x00);
		mask.SetRange(0x29 + 4, 4, 0x00);
		mask.SetRange(0x31 + 3, 4, 0x00);
		mask.SetRange(0x38 + 2, 4, 0x00);
#else
		buf.SetDword(0x0F + 1, (uint32_t)AddrManager::GetAddr("gpGlobals"));
		
		mask.SetRange(0x06 + 2, 4, 0x00);
		mask.SetRange(0x1c + 4, 4, 0x00);
		mask.SetRange(0x24 + 3, 4, 0x00);
		mask.SetRange(0x2b + 2, 4, 0x00);
#endif
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFProjectile_Arrow::ArrowTouch"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0029 + 4; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x001c + 4; }
#endif
};

#elif defined _WINDOWS

using CExtract_CTFProjectile_Arrow_ArrowTouch = IExtractStub;

#endif

#endif


IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseProjectile, m_hOriginalLauncher, CBaseProjectile);

MemberVFuncThunk<const CBaseProjectile *, int> CBaseProjectile::vt_GetProjectileType(TypeName<CBaseProjectile>(), "CBaseProjectile::GetProjectileType");
#ifdef SE_IS_TF2
MemberVFuncThunk<const CBaseProjectile *, bool, bool> CBaseProjectile::vt_IsDestroyable(TypeName<CBaseProjectile>(), "CBaseProjectile::IsDestroyable");
#else
MemberVFuncThunk<const CBaseProjectile *, bool> CBaseProjectile::vt_IsDestroyable(TypeName<CBaseProjectile>(), "CBaseProjectile::IsDestroyable");
#endif
MemberVFuncThunk<const CBaseProjectile *, void, bool, bool> CBaseProjectile::vt_Destroy(TypeName<CBaseProjectile>(), "CBaseProjectile::Destroy");
MemberVFuncThunk<CBaseProjectile *, void, CBaseEntity*> CBaseProjectile::vt_SetLauncher(TypeName<CBaseProjectile>(), "CBaseProjectile::SetLauncher");
MemberVFuncThunk<CBaseProjectile *, bool> CBaseProjectile::vt_CanCollideWithTeammates(TypeName<CBaseProjectile>(), "CBaseProjectile::CanCollideWithTeammates");

	
IMPL_SENDPROP(float, CBaseGrenade, m_DmgRadius, CBaseGrenade);

MemberFuncThunk<const CBaseGrenade *, CBaseEntity *> CBaseGrenade::ft_GetThrower("CBaseGrenade::GetThrower");
MemberFuncThunk<CBaseGrenade *, void, CBaseEntity *> CBaseGrenade::ft_SetThrower("CBaseGrenade::SetThrower");

#ifdef SE_IS_TF2
MemberVFuncThunk<CTFBaseProjectile *, void, float> CTFBaseProjectile::vt_SetDamage(TypeName<CTFBaseProjectile>(), "CTFBaseProjectile::SetDamage");
MemberVFuncThunk<CTFBaseProjectile *, float> CTFBaseProjectile::vt_GetDamage(TypeName<CTFBaseProjectile>(), "CTFBaseProjectile::GetDamage");

MemberFuncThunk<CTFBaseProjectile *, void, CBaseEntity *> CTFBaseProjectile::ft_SetScorer("CTFBaseProjectile::SetScorer");

IMPL_SENDPROP(Vector,               CTFBaseRocket, m_vInitialVelocity, CTFBaseRocket);
IMPL_SENDPROP(int,                  CTFBaseRocket, m_iDeflected,       CTFBaseRocket);
IMPL_SENDPROP(CHandle<CBaseEntity>, CTFBaseRocket, m_hLauncher,        CTFBaseRocket);

MemberVFuncThunk<CTFBaseRocket *, void, trace_t *, CBaseEntity *> CTFBaseRocket::vt_Explode    (TypeName<CTFBaseRocket>(), "CTFBaseRocket::Explode");
MemberVFuncThunk<const CTFBaseRocket *, int>                      CTFBaseRocket::vt_GetWeaponID(TypeName<CTFBaseRocket>(), "CTFBaseRocket::GetWeaponID");

MemberFuncThunk<const CTFBaseRocket *, CBasePlayer *> CTFBaseRocket::ft_GetOwnerPlayer("CTFBaseRocket::GetOwnerPlayer");

MemberFuncThunk<CTFProjectile_EnergyBall *, void, CBaseEntity *> CTFProjectile_EnergyBall::ft_SetScorer("CTFProjectile_EnergyBall::SetScorer");


MemberFuncThunk<CTFProjectile_Flare *, void, CBaseEntity *> CTFProjectile_Flare::ft_SetScorer("CTFProjectile_Flare::SetScorer");

IMPL_SENDPROP(bool, CTFProjectile_Flare, m_bCritical, CTFProjectile_Flare);


MemberFuncThunk<CTFProjectile_EnergyRing *, float> CTFProjectile_EnergyRing::ft_GetInitialVelocity("CTFProjectile_EnergyRing::GetInitialVelocity");

MemberFuncThunk<CTFProjectile_Rocket *, void, CBaseEntity *> CTFProjectile_Rocket::ft_SetScorer("CTFProjectile_Rocket::SetScorer");

IMPL_SENDPROP(bool, CTFProjectile_Rocket, m_bCritical, CTFProjectile_Rocket);


IMPL_EXTRACT(float, CTFProjectile_Arrow, m_flTimeInit, new CExtract_CTFProjectile_Arrow_ArrowTouch());
IMPL_SENDPROP(bool, CTFProjectile_Arrow, m_bCritical, CTFProjectile_Arrow);
IMPL_REL_BEFORE(CUtlVector<int>, CTFProjectile_Arrow, m_HitEntities, m_flTimeInit, 0);
IMPL_REL_BEFORE(EHANDLE, CTFProjectile_Arrow, m_pTrail, m_bCritical, 0, bool, bool);
IMPL_REL_AFTER(bool, CTFProjectile_Arrow, m_bPenetrate, m_bCritical);
IMPL_SENDPROP(bool, CTFProjectile_Arrow, m_bArrowAlight, CTFProjectile_Arrow);

MemberFuncThunk<CTFProjectile_Arrow *, void, CBaseEntity *> CTFProjectile_Arrow::ft_SetScorer("CTFProjectile_Arrow::SetScorer");

//MemberFuncThunk<const CTFProjectile_Arrow *, bool> CTFProjectile_Arrow::ft_CanPenetrate("CTFBaseRocket::GetOwnerPlayer");
//MemberFuncThunk<CTFProjectile_Arrow *, void, bool> CTFProjectile_Arrow::ft_SetPenetrate("CTFBaseRocket::GetOwnerPlayer");

StaticFuncThunk<CTFProjectile_Arrow *,const Vector &, const QAngle &, const float, const float, int, CBaseEntity *, CBaseEntity *> CTFProjectile_Arrow::ft_Create("CTFProjectile_Arrow::Create");


IMPL_SENDPROP(Vector, CTFProjectile_BallOfFire, m_vecInitialVelocity, CTFProjectile_BallOfFire);
IMPL_REL_AFTER(bool, CTFProjectile_BallOfFire, m_bLandedBonusDamage, m_vecInitialVelocity, Vector, bool, bool, bool);


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
IMPL_SENDPROP(bool, CTFGrenadePipebombProjectile, m_bDefensiveBomb, CTFGrenadePipebombProjectile);
IMPL_REL_BEFORE(float, CTFGrenadePipebombProjectile, m_flFullDamage, m_bDefensiveBomb, 0);

MemberVFuncThunk<CTFProjectile_Throwable *, Vector, const Vector &, const Vector &, const Vector &, float> CTFProjectile_Throwable::vt_GetVelocityVector(TypeName<CTFProjectile_Throwable>(), "CTFProjectile_Throwable::GetVelocityVector");
MemberVFuncThunk<CTFProjectile_Throwable *, const AngularImpulse> CTFProjectile_Throwable::vt_GetAngularImpulse(TypeName<CTFProjectile_Throwable>(), "CTFProjectile_Throwable::GetAngularImpulse");

StaticFuncThunk<CTFStunBall *,const Vector &, const QAngle &, CBaseEntity *> CTFStunBall::ft_Create("CTFStunBall::Create");
StaticFuncThunk<CTFBall_Ornament *,const Vector &, const QAngle &, CBaseEntity *> CTFBall_Ornament::ft_Create("CTFBall_Ornament::Create");
StaticFuncThunk<CTFProjectile_Jar *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> CTFProjectile_Jar::ft_Create("CTFProjectile_Jar::Create");
StaticFuncThunk<CTFProjectile_JarMilk *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> CTFProjectile_JarMilk::ft_Create("CTFProjectile_JarMilk::Create");
StaticFuncThunk<CTFProjectile_Cleaver *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &, int> CTFProjectile_Cleaver::ft_Create("CTFProjectile_Cleaver::Create");
StaticFuncThunk<CTFProjectile_JarGas *,const Vector &, const QAngle &, const Vector &, const AngularImpulse &, CBaseCombatCharacter *, const CTFWeaponInfo &> CTFProjectile_JarGas::ft_Create("CTFProjectile_JarGas::Create");
#endif

GlobalThunk<CUtlVector<IBaseProjectileAutoList *>> IBaseProjectileAutoList::m_IBaseProjectileAutoListAutoList("IBaseProjectileAutoList::m_IBaseProjectileAutoListAutoList");
