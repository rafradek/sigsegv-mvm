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

IMPL_SENDPROP(Vector,               CTFBaseRocket, m_vInitialVelocity, CTFBaseRocket);
IMPL_SENDPROP(int,                  CTFBaseRocket, m_iDeflected,       CTFBaseRocket);
IMPL_SENDPROP(CHandle<CBaseEntity>, CTFBaseRocket, m_hLauncher,        CTFBaseRocket);

MemberVFuncThunk<CTFBaseRocket *, void, trace_t *, CBaseEntity *> CTFBaseRocket::vt_Explode(TypeName<CTFBaseRocket>(), "CTFBaseRocket::Explode");

MemberFuncThunk<const CTFBaseRocket *, CBasePlayer *> CTFBaseRocket::ft_GetOwnerPlayer("CTFBaseRocket::GetOwnerPlayer");

MemberFuncThunk<CTFProjectile_EnergyRing *, float> CTFProjectile_EnergyRing::ft_GetInitialVelocity("CTFProjectile_EnergyRing::GetInitialVelocity");

IMPL_SENDPROP(bool, CTFProjectile_Rocket, m_bCritical, CTFProjectile_Rocket);

IMPL_EXTRACT(float, CTFProjectile_Arrow, m_flTimeInit, new CExtract_CTFProjectile_Arrow_ArrowTouch());
IMPL_SENDPROP(bool, CTFProjectile_Arrow, m_bCritical, CTFProjectile_Arrow);
IMPL_RELATIVE(CUtlVector<int>, CTFProjectile_Arrow, m_HitEntities, m_flTimeInit, -sizeof(CUtlVector<int>));

//MemberFuncThunk<const CTFProjectile_Arrow *, bool> CTFProjectile_Arrow::ft_CanPenetrate("CTFBaseRocket::GetOwnerPlayer");
//MemberFuncThunk<CTFProjectile_Arrow *, void, bool> CTFProjectile_Arrow::ft_SetPenetrate("CTFBaseRocket::GetOwnerPlayer");

StaticFuncThunk<CTFProjectile_Arrow *,const Vector &, const QAngle &, const float, const float, int, CBaseEntity *, CBaseEntity *> CTFProjectile_Arrow::ft_Create("CTFProjectile_Arrow::Create");

IMPL_SENDPROP(int,    CTFWeaponBaseGrenadeProj, m_iDeflected,       CTFWeaponBaseGrenadeProj);
IMPL_SENDPROP(bool,   CTFWeaponBaseGrenadeProj, m_bCritical,        CTFWeaponBaseGrenadeProj);
IMPL_SENDPROP(Vector, CTFWeaponBaseGrenadeProj, m_vInitialVelocity, CTFWeaponBaseGrenadeProj);

MemberVFuncThunk<const CTFWeaponBaseGrenadeProj *, int> CTFWeaponBaseGrenadeProj::vt_GetWeaponID(TypeName<CTFWeaponBaseGrenadeProj>(), "CTFWeaponBaseGrenadeProj::GetWeaponID");

MemberFuncThunk<const CTFWeaponBaseGrenadeProj *, void, float> CTFWeaponBaseGrenadeProj::ft_SetDetonateTimerLength("CTFWeaponBaseGrenadeProj::SetDetonateTimerLength");
MemberFuncThunk<CTFWeaponBaseGrenadeProj *, void, trace_t *, int> CTFWeaponBaseGrenadeProj::ft_Explode("CTFWeaponBaseGrenadeProj::Explode");

IMPL_SENDPROP(CHandle<CBaseEntity>, CTFGrenadePipebombProjectile, m_hLauncher, CTFGrenadePipebombProjectile);
IMPL_SENDPROP(bool, CTFGrenadePipebombProjectile, m_bTouched, CTFGrenadePipebombProjectile);
IMPL_SENDPROP(int, CTFGrenadePipebombProjectile, m_iType, CTFGrenadePipebombProjectile);


GlobalThunk<CUtlVector<IBaseProjectileAutoList *>> IBaseProjectileAutoList::m_IBaseProjectileAutoListAutoList("IBaseProjectileAutoList::m_IBaseProjectileAutoListAutoList");
