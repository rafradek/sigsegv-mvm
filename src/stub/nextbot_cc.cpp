#include "stub/nextbot_cc.h"


#if defined _LINUX

static constexpr uint8_t s_Buf_CTFTankBoss_m_pBodyInterface[] = {
#ifdef PLATFORM_64BITS
	0x48, 0x8b, 0x87, 0x40, 0x0c, 0x00, 0x00,  // +0x0000 mov     rax, [rdi+0C40h]
	0xc3,                                      // +0x0007 retn
#else
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                   // +0003  mov eax,[ebp+this]
	0x5d,                               // +0006  pop ebp
	0x8b, 0x80, 0xd0, 0x09, 0x00, 0x00, // +0007  mov eax,[eax+0xVVVVVVVV]
	0xc3,                               // +000D  ret
#endif
};

struct CExtract_CTFTankBoss_m_pBodyInterface : public IExtract<uint32_t>
{
	CExtract_CTFTankBoss_m_pBodyInterface() : IExtract<uint32_t>(sizeof(s_Buf_CTFTankBoss_m_pBodyInterface)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFTankBoss_m_pBodyInterface);
		
#ifdef PLATFORM_64BITS
		mask.SetRange(0x00 + 3, 4, 0x00);
#else
		mask.SetRange(0x07 + 2, 4, 0x00);
#endif
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFTankBoss::GetBodyInterface"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 3; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
#endif
};

#elif defined _WINDOWS

using CExtract_CTFTankBoss_m_pBodyInterface = IExtractStub;

#endif

#if defined _LINUX

static constexpr uint8_t s_Buf_CZombie_m_pIntentionInterface[] = {
#ifdef PLATFORM_64BITS
	0x48, 0x8b, 0x87, 0x40, 0x0c, 0x00, 0x00,  // +0x0000 mov     rax, [rdi+0C40h]
	0xc3,                                      // +0x0007 retn
#else
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                   // +0003  mov eax,[ebp+this]
	0x5d,                               // +0006  pop ebp
	0x8b, 0x80, 0xd0, 0x09, 0x00, 0x00, // +0007  mov eax,[eax+0xVVVVVVVV]
	0xc3,                               // +000D  ret
#endif
};

struct CExtract_CZombie_m_pIntentionInterface : public IExtract<uint32_t>
{
	CExtract_CZombie_m_pIntentionInterface() : IExtract<uint32_t>(sizeof(s_Buf_CZombie_m_pIntentionInterface)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CZombie_m_pIntentionInterface);
		
#ifdef PLATFORM_64BITS
		mask.SetRange(0x00 + 3, 4, 0x00);
#else
		mask.SetRange(0x07 + 2, 4, 0x00);
#endif
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CZombie::GetIntentionInterface"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 3; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
#endif
};

#elif defined _WINDOWS

using CExtract_CZombie_m_pIntentionInterface = IExtractStub;

#endif

#if defined _LINUX

static constexpr uint8_t s_Buf_CBotNPCArcher_m_pIntentionInterface[] = {
#ifdef PLATFORM_64BITS
	0x48, 0x8b, 0x87, 0x40, 0x0c, 0x00, 0x00,  // +0x0000 mov     rax, [rdi+0C40h]
	0xc3,                                      // +0x0007 retn
#else
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                   // +0003  mov eax,[ebp+this]
	0x5d,                               // +0006  pop ebp
	0x8b, 0x80, 0xd0, 0x09, 0x00, 0x00, // +0007  mov eax,[eax+0xVVVVVVVV]
	0xc3,                               // +000D  ret
#endif
};

struct CExtract_CBotNPCArcher_m_pIntentionInterface : public IExtract<uint32_t>
{
	CExtract_CBotNPCArcher_m_pIntentionInterface() : IExtract<uint32_t>(sizeof(s_Buf_CBotNPCArcher_m_pIntentionInterface)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CBotNPCArcher_m_pIntentionInterface);

#ifdef PLATFORM_64BITS
		mask.SetRange(0x00 + 3, 4, 0x00);
#else
		mask.SetRange(0x07 + 2, 4, 0x00);
#endif
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CBotNPCArcher::GetIntentionInterface"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 3; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
#endif
};

#elif defined _WINDOWS

using CExtract_CBotNPCArcher_m_pIntentionInterface = IExtractStub;

#endif
MemberVFuncThunk<CTFBaseBoss *, void> CTFBaseBoss::vt_UpdateCollisionBounds(TypeName<CTFBaseBoss>(), "CTFBaseBoss::UpdateCollisionBounds");
MemberVFuncThunk<CTFBaseBoss *, int> CTFBaseBoss::vt_GetCurrencyValue(TypeName<CTFBaseBoss>(), "CTFBaseBoss::GetCurrencyValue");


IMPL_EXTRACT (IBody *,             CTFTankBoss, m_pBodyInterface,  new CExtract_CTFTankBoss_m_pBodyInterface());
IMPL_REL_AFTER(CHandle<CPathTrack>,CTFTankBoss, m_hCurrentNode,    m_pBodyInterface, EHANDLE, EHANDLE);
IMPL_REL_AFTER(CUtlVector<float>,  CTFTankBoss, m_NodeDists,       m_hCurrentNode);
IMPL_REL_AFTER(float,              CTFTankBoss, m_flTotalDistance, m_NodeDists);
IMPL_REL_AFTER(int,                CTFTankBoss, m_iCurrentNode,    m_flTotalDistance);
IMPL_REL_AFTER(int,                CTFTankBoss, m_iModelIndex,     m_iCurrentNode, float, bool, float, int, bool, bool, bool, bool, int);


StaticFuncThunk<CHalloweenBaseBoss *, CHalloweenBaseBoss::HalloweenBossType, const Vector&, int, CBaseEntity *> CHalloweenBaseBoss::ft_SpawnBossAtPos("CHalloweenBaseBoss::SpawnBossAtPos");


IMPL_EXTRACT (IIntention *,            CZombie, m_pIntentionInterface,  new CExtract_CZombie_m_pIntentionInterface());
IMPL_REL_AFTER(ILocomotion *, CZombie, m_pLocomotionInterface, m_pIntentionInterface);
IMPL_REL_AFTER(IBody *, CZombie, m_pBodyInterface, m_pLocomotionInterface);

IMPL_REL_BEFORE(CZombie::SkeletonType_t, CZombie, m_iSkeletonType, m_flHeadScale, 0);
IMPL_SENDPROP  (float,                   CZombie, m_flHeadScale,   CZombie);

StaticFuncThunk<CZombie *, const Vector&, float, int, CBaseEntity *, CZombie::SkeletonType_t> CZombie::ft_SpawnAtPos("CZombie::SpawnAtPos");


GlobalThunk<CUtlVector<IZombieAutoList *>> IZombieAutoList::m_IZombieAutoListAutoList("IZombieAutoList::m_IZombieAutoListAutoList");


IMPL_EXTRACT (IIntention *,            CBotNPCArcher, m_pIntentionInterface,  new CExtract_CBotNPCArcher_m_pIntentionInterface());
IMPL_REL_AFTER(ILocomotion *, CBotNPCArcher, m_pLocomotionInterface, m_pIntentionInterface);
IMPL_REL_AFTER(IBody *, CBotNPCArcher, m_pBodyInterface, m_pLocomotionInterface);
IMPL_REL_AFTER(CBaseAnimating *, CBotNPCArcher, m_bow, m_pBodyInterface);
IMPL_REL_AFTER(Vector, CBotNPCArcher, m_eyeOffset, m_bow);