#include "stub/nextbot_cc.h"


#if defined _LINUX

static constexpr uint8_t s_Buf_CTFTankBoss_m_pBodyInterface[] = {
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                   // +0003  mov eax,[ebp+this]
	0x5d,                               // +0006  pop ebp
	0x8b, 0x80, 0xd0, 0x09, 0x00, 0x00, // +0007  mov eax,[eax+0xVVVVVVVV]
	0xc3,                               // +000D  ret
};

struct CExtract_CTFTankBoss_m_pBodyInterface : public IExtract<IBody **>
{
	CExtract_CTFTankBoss_m_pBodyInterface() : IExtract<IBody **>(sizeof(s_Buf_CTFTankBoss_m_pBodyInterface)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFTankBoss_m_pBodyInterface);
		
		mask.SetRange(0x07 + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFTankBoss::GetBodyInterface"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
};

#elif defined _WINDOWS

using CExtract_CTFTankBoss_m_pBodyInterface = IExtractStub;

#endif

#if defined _LINUX

static constexpr uint8_t s_Buf_CZombie_m_pIntentionInterface[] = {
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                   // +0003  mov eax,[ebp+this]
	0x5d,                               // +0006  pop ebp
	0x8b, 0x80, 0xfc, 0x08, 0x00, 0x00, // +0007  mov eax,[eax+0xVVVVVVVV]
	0xc3,                               // +000D  ret
};

struct CExtract_CZombie_m_pIntentionInterface : public IExtract<IBody **>
{
	CExtract_CZombie_m_pIntentionInterface() : IExtract<IBody **>(sizeof(s_Buf_CZombie_m_pIntentionInterface)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CZombie_m_pIntentionInterface);
		
		mask.SetRange(0x07 + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CZombie::GetIntentionInterface"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
};

#elif defined _WINDOWS

using CExtract_CZombie_m_pIntentionInterface = IExtractStub;

#endif

#if defined _LINUX

static constexpr uint8_t s_Buf_CBotNPCArcher_m_pIntentionInterface[] = {
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                   // +0003  mov eax,[ebp+this]
	0x5d,                               // +0006  pop ebp
	0x8b, 0x80, 0xfc, 0x08, 0x00, 0x00, // +0007  mov eax,[eax+0xVVVVVVVV]
	0xc3,                               // +000D  ret
};

struct CExtract_CBotNPCArcher_m_pIntentionInterface : public IExtract<IBody **>
{
	CExtract_CBotNPCArcher_m_pIntentionInterface() : IExtract<IBody **>(sizeof(s_Buf_CBotNPCArcher_m_pIntentionInterface)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CBotNPCArcher_m_pIntentionInterface);
		
		mask.SetRange(0x07 + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CBotNPCArcher::GetIntentionInterface"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
};

#elif defined _WINDOWS

using CExtract_CBotNPCArcher_m_pIntentionInterface = IExtractStub;

#endif
MemberVFuncThunk<CTFBaseBoss *, void> CTFBaseBoss::vt_UpdateCollisionBounds(TypeName<CTFBaseBoss>(), "CTFBaseBoss::UpdateCollisionBounds");


IMPL_EXTRACT (IBody *,             CTFTankBoss, m_pBodyInterface,  new CExtract_CTFTankBoss_m_pBodyInterface());
IMPL_RELATIVE(CHandle<CPathTrack>, CTFTankBoss, m_hCurrentNode,    m_pBodyInterface, 0x0c);
IMPL_RELATIVE(CUtlVector<float>,   CTFTankBoss, m_NodeDists,       m_pBodyInterface, 0x10);
IMPL_RELATIVE(float,               CTFTankBoss, m_flTotalDistance, m_pBodyInterface, 0x24);
IMPL_RELATIVE(int,                 CTFTankBoss, m_iCurrentNode,    m_pBodyInterface, 0x28);
IMPL_RELATIVE(int,                 CTFTankBoss, m_iModelIndex,     m_pBodyInterface, 0x44);


StaticFuncThunk<CHalloweenBaseBoss *, CHalloweenBaseBoss::HalloweenBossType, const Vector&, int, CBaseEntity *> CHalloweenBaseBoss::ft_SpawnBossAtPos("CHalloweenBaseBoss::SpawnBossAtPos");


IMPL_EXTRACT (IIntention *,            CZombie, m_pIntentionInterface,  new CExtract_CZombie_m_pIntentionInterface());
IMPL_RELATIVE(ILocomotion *, CZombie, m_pLocomotionInterface, m_pIntentionInterface, sizeof(uintptr_t));
IMPL_RELATIVE(IBody *, CZombie, m_pBodyInterface, m_pIntentionInterface, sizeof(uintptr_t) * 2);

IMPL_REL_BEFORE(CZombie::SkeletonType_t, CZombie, m_iSkeletonType, m_flHeadScale);
IMPL_SENDPROP  (float,                   CZombie, m_flHeadScale,   CZombie);

StaticFuncThunk<CZombie *, const Vector&, float, int, CBaseEntity *, CZombie::SkeletonType_t> CZombie::ft_SpawnAtPos("CZombie::SpawnAtPos");


GlobalThunk<CUtlVector<IZombieAutoList *>> IZombieAutoList::m_IZombieAutoListAutoList("IZombieAutoList::m_IZombieAutoListAutoList");


IMPL_EXTRACT (IIntention *,            CBotNPCArcher, m_pIntentionInterface,  new CExtract_CBotNPCArcher_m_pIntentionInterface());
IMPL_RELATIVE(ILocomotion *, CBotNPCArcher, m_pLocomotionInterface, m_pIntentionInterface, sizeof(uintptr_t));
IMPL_RELATIVE(IBody *, CBotNPCArcher, m_pBodyInterface, m_pIntentionInterface, sizeof(uintptr_t) * 2);
IMPL_RELATIVE(CBaseAnimating *, CBotNPCArcher, m_bow, m_pIntentionInterface, sizeof(uintptr_t) * 3);
IMPL_RELATIVE(Vector, CBotNPCArcher, m_eyeOffset, m_pIntentionInterface, sizeof(uintptr_t) * 4);