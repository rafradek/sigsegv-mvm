#include "stub/objects.h"
#include "mem/extract.h"

#if defined _LINUX

static constexpr uint8_t s_Buf_CObjectSentrygun_FireRocket[] = {
    
    0x55,                                           // +0000
    0x89, 0xE5,                                     // +0001
    0x57,                                           // +0003
    0x56,                                           // +0004
    0x53,                                           // +0005
    0x81, 0xEC, 0x9C, 0x01, 0x00, 0x00,             // +0006
    0x8B, 0x5D, 0x08,                               // +000C
    0xA1, 0x30, 0x5F, 0x79, 0x01,                   // +000F
    0xF3, 0x0F, 0x10, 0x83, 0x08, 0x0B, 0x00, 0x00, // +0014
    0x0F, 0x2F, 0x40, 0x0C                          // +001C
};

struct CExtract_CObjectSentrygun_FireRocket : public IExtract<float *>
{
	using T = float *;
	
	CExtract_CObjectSentrygun_FireRocket() : IExtract<T>(sizeof(s_Buf_CObjectSentrygun_FireRocket)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CObjectSentrygun_FireRocket);
		
		buf.SetDword(0x0F + 1, (uint32_t)AddrManager::GetAddr("gpGlobals"));
		
		mask.SetRange(0x14 + 4, 4, 0x00);
		mask.SetRange(0x1c + 3, 1, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CObjectSentrygun::FireRocket"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0060; }
	virtual uint32_t GetExtractOffset() const override { return 0x0014 + 4; }
};

#elif defined _WINDOWS

using CExtract_CObjectSentrygun_FireRocket = IExtractStub;

#endif


IMPL_DATAMAP (int,                CBaseObject, m_nDefaultUpgradeLevel);

IMPL_SENDPROP(int,                CBaseObject, m_iUpgradeLevel,       CBaseObject);
IMPL_SENDPROP(int,                CBaseObject, m_iObjectType,         CBaseObject);
IMPL_SENDPROP(int,                CBaseObject, m_iObjectMode,         CBaseObject);
IMPL_SENDPROP(CHandle<CTFPlayer>, CBaseObject, m_hBuilder,            CBaseObject);
IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseObject, m_hBuiltOnEntity,      CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bMiniBuilding,       CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bDisposableBuilding, CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bBuilding, CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bDisabled, CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bCarried, CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bCarryDeploy, CBaseObject);
IMPL_SENDPROP(int,                CBaseObject, m_iKills, CObjectSentrygun);

IMPL_EXTRACT(float, CObjectSentrygun, m_flNextRocketFire, new CExtract_CObjectSentrygun_FireRocket());

MemberFuncThunk<CBaseObject *, void, float> CBaseObject::ft_SetHealth        ("CBaseObject::SetHealth");
MemberFuncThunk<CBaseObject *, void, float> CBaseObject::ft_SetPlasmaDisabled("CBaseObject::SetPlasmaDisabled");
MemberFuncThunk<CBaseObject *, bool>        CBaseObject::ft_HasSapper        ("CBaseObject::HasSapper");

MemberVFuncThunk<CBaseObject *, void, CTFPlayer *>   CBaseObject::vt_StartPlacement               (TypeName<CBaseObject>(), "CBaseObject::StartPlacement");
MemberVFuncThunk<CBaseObject *, bool, CBaseEntity *> CBaseObject::vt_StartBuilding                (TypeName<CBaseObject>(), "CBaseObject::StartBuilding");
MemberVFuncThunk<CBaseObject *, void>                CBaseObject::vt_DetonateObject               (TypeName<CBaseObject>(), "CBaseObject::DetonateObject");
MemberVFuncThunk<CBaseObject *, void>                CBaseObject::vt_InitializeMapPlacedObject    (TypeName<CBaseObject>(), "CBaseObject::InitializeMapPlacedObject");
MemberVFuncThunk<CBaseObject *, void>                CBaseObject::vt_FinishedBuilding             (TypeName<CBaseObject>(), "CBaseObject::FinishedBuilding");
MemberVFuncThunk<CBaseObject *, int>                 CBaseObject::vt_GetMiniBuildingStartingHealth(TypeName<CBaseObject>(), "CBaseObject::GetMiniBuildingStartingHealth");
MemberVFuncThunk<CBaseObject *, int>                 CBaseObject::vt_GetMaxHealthForCurrentLevel  (TypeName<CBaseObject>(), "CBaseObject::GetMaxHealthForCurrentLevel");


GlobalThunk<CUtlVector<IBaseObjectAutoList *>> IBaseObjectAutoList::m_IBaseObjectAutoListAutoList("IBaseObjectAutoList::m_IBaseObjectAutoListAutoList");


MemberVFuncThunk<CObjectDispenser *, float> CObjectDispenser::vt_GetDispenserRadius(TypeName<CObjectDispenser>(), "CObjectDispenser::GetDispenserRadius");

MemberVFuncThunk<CObjectSentrygun *, QAngle &> CObjectSentrygun::vt_GetTurretAngles(TypeName<CObjectSentrygun>(), "CObjectSentrygun::GetTurretAngles");
MemberFuncThunk<CObjectSentrygun *, void> CObjectSentrygun::ft_SentryThink("CObjectSentrygun::SentryThink");