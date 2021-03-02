#include "stub/tfbot_behavior.h"
#include "mem/patch.h"
#include "mem/scan.h"
#include "util/rtti.h"


#if defined _MSC_VER
#error
#endif


//template<typename T> Action<T>::~Action() {}


#if 0
constexpr uint8_t buf_CTFBotSeekAndDestroy[] = {
	0xc7, 0x04, 0x24, 0x28, 0x48, 0x00, 0x00,       // +0000  mov dword ptr [esp],0x4828
	0xe8, 0xf4, 0x00, 0xca, 0xff,                   // +0007  call operator new
	0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x80, 0xbf, // +000C  mov dword ptr [esp+0x4],-1.0f
	0x89, 0xc3,                                     // +0014  mov ebx,eax
	0x89, 0x04, 0x24,                               // +0016  mov [esp],eax
	0xe8, 0x12, 0xda, 0xff, 0xff,                   // +0019  call CTFBotSeekAndDestroy::CTFBotSeekAndDestroy [C1]
};

struct IVerify_CTFBotSeekAndDestroy : public IVerify
{
	IVerify_CTFBotSeekAndDestroy() : IVerify(sizeof(buf_CTFBotSeekAndDestroy)) {}
	
	virtual const char *GetFuncName() const override { return "TODO"; }
	
	virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		
	}
};
#endif


template<typename T>
static uint32_t FindAdditionalVTable(const void **vt, ptrdiff_t diff)
{
	using VTScanner = CBasicScanner<ScanDir::FORWARD, ScanResults::ALL, 0x4>;
	
	const rtti_t *rtti = RTTI::GetRTTI<T>();
	
	uint32_t seek[2] = {
		diff,
		(uint32_t)rtti,
	};
	
	CScan<VTScanner> scan(CAddrAddrBounds((void *)vt, (void *)((uintptr_t)vt + 0x2000)), seek, sizeof(seek));
	assert(scan.ExactlyOneMatch());
	
	return (uint32_t)scan.FirstMatch() + 0x8;
}


static MemberFuncThunk<CTFBotAttack *, void> ft_CTFBotAttack_ctor("CTFBotAttack::CTFBotAttack [C1]");
CTFBotAttack *CTFBotAttack::New()
{
	// TODO: verify sizeof(CTFBotAttack) in the game code at runtime
	// TODO: verify that the addr for the ctor actually exists
	
	auto action = reinterpret_cast<CTFBotAttack *>(::operator new(sizeof(CTFBotAttack)));
	ft_CTFBotAttack_ctor(action);
	return action;
}


static MemberFuncThunk<CTFBotSeekAndDestroy *, void, float> ft_CTFBotSeekAndDestroy_ctor("CTFBotSeekAndDestroy::CTFBotSeekAndDestroy [C1]");
CTFBotSeekAndDestroy *CTFBotSeekAndDestroy::New(float duration)
{
	// TODO: verify sizeof(CTFBotSeekAndDestroy) in the game code at runtime
	// TODO: verify that the addr for the ctor actually exists
	
	auto action = reinterpret_cast<CTFBotSeekAndDestroy *>(::operator new(sizeof(CTFBotSeekAndDestroy)));
	ft_CTFBotSeekAndDestroy_ctor(action, duration);
	return action;
}


static MemberFuncThunk<CTFBotFetchFlag *, void, bool> ft_CTFBotFetchFlag_ctor("CTFBotFetchFlag::CTFBotFetchFlag [C1]");
CTFBotFetchFlag *CTFBotFetchFlag::New(bool give_up_when_done)
{
	// TODO: verify sizeof(CTFBotFetchFlag) in the game code at runtime
	// TODO: verify that the addr for the ctor actually exists
	
	auto action = reinterpret_cast<CTFBotFetchFlag *>(::operator new(sizeof(CTFBotFetchFlag)));
	ft_CTFBotFetchFlag_ctor(action, give_up_when_done);
	return action;
}


static MemberFuncThunk<CTFBotPushToCapturePoint *, void, Action<CTFBot> *> ft_CTFBotPushToCapturePoint_ctor("CTFBotPushToCapturePoint::CTFBotPushToCapturePoint [C1]");
CTFBotPushToCapturePoint *CTFBotPushToCapturePoint::New(Action<CTFBot> *done_action)
{
	// TODO: verify sizeof(CTFBotPushToCapturePoint) in the game code at runtime
	// TODO: verify that the addr for the ctor actually exists
	
	auto action = reinterpret_cast<CTFBotPushToCapturePoint *>(::operator new(sizeof(CTFBotPushToCapturePoint)));
	ft_CTFBotPushToCapturePoint_ctor(action, done_action);
	return action;
}


static MemberVFuncThunk<CTFBotMedicHeal *, void> ft_CTFBotMedicHeal_ctor(TypeName<CTFBotMedicHeal>(),"CTFBotMedicHeal::CTFBotMedicHeal [C1]");
//#if TOOLCHAIN_FIXES
CTFBotMedicHeal *CTFBotMedicHeal::New()
{
	// TODO: verify sizeof(CTFBotMedicHeal) in the game code at runtime
	// TODO: verify that the addr for the ctor actually exists
	
	auto action = reinterpret_cast<CTFBotMedicHeal *>(::operator new(sizeof(CTFBotMedicHeal)));
	ft_CTFBotMedicHeal_ctor(action);
	return action;
}
//#endif


CTFBotMedicRetreat *CTFBotMedicRetreat::New()
{
	// TODO: verify sizeof(CTFBotMedicRetreat) in the game code at runtime
	// TODO: verify that the addr for the vtable actually exists
	
	auto action = new CTFBotMedicRetreat();
	
	/* overwrite vtable pointers */
	auto vt = RTTI::GetVTable<CTFBotMedicRetreat>();
	*(uint32_t *)((uintptr_t)action + 0x0000) = (uintptr_t)vt;
	*(uint32_t *)((uintptr_t)action + 0x0004) = FindAdditionalVTable<CTFBotMedicRetreat>(vt, -0x4);
	
	return action;
}


static MemberFuncThunk<CTFBotSniperLurk *, void> ft_CTFBotSniperLurk_ctor("CTFBotSniperLurk::CTFBotSniperLurk [C1]");
//#if TOOLCHAIN_FIXES
CTFBotSniperLurk *CTFBotSniperLurk::New()
{
	// TODO: verify sizeof(CTFBotSniperLurk) in the game code at runtime
	// TODO: verify that the addr for the ctor actually exists
	
	auto action = reinterpret_cast<CTFBotSniperLurk *>(::operator new(sizeof(CTFBotSniperLurk)));
	ft_CTFBotSniperLurk_ctor(action);
	return action;
}
//#endif


CTFBotSpyInfiltrate *CTFBotSpyInfiltrate::New()
{
	// TODO: verify sizeof(CTFBotSpyInfiltrate) in the game code at runtime
	// TODO: verify that the addr for the vtable actually exists
	
	auto action = new CTFBotSpyInfiltrate();
	
	/* overwrite vtable pointers */
	auto vt = RTTI::GetVTable<CTFBotSpyInfiltrate>();
	*(uint32_t *)((uintptr_t)action + 0x0000) = (uintptr_t)vt;
	*(uint32_t *)((uintptr_t)action + 0x0004) = FindAdditionalVTable<CTFBotSpyInfiltrate>(vt, -0x4);
	
	return action;
}


CTFBotEngineerBuild *CTFBotEngineerBuild::New()
{
	// TODO: verify sizeof(CTFBotEngineerBuild) in the game code at runtime
	// TODO: verify that the addr for the vtable actually exists
	
	auto action = new CTFBotEngineerBuild();
	
	/* overwrite vtable pointers */
	auto vt = RTTI::GetVTable<CTFBotEngineerBuild>();
	*(uint32_t *)((uintptr_t)action + 0x0000) = (uintptr_t)vt;
	*(uint32_t *)((uintptr_t)action + 0x0004) = FindAdditionalVTable<CTFBotEngineerBuild>(vt, -0x4);
	
	return action;
}


CTFBotDead *CTFBotDead::New()
{
	// TODO: verify sizeof(CTFBotDead) in the game code at runtime
	// TODO: verify that the addr for the vtable actually exists
	
	auto action = new CTFBotDead();
	
	/* overwrite vtable pointers */
	auto vt = RTTI::GetVTable<CTFBotDead>();
	*(uint32_t *)((uintptr_t)action + 0x0000) = (uintptr_t)vt;
	*(uint32_t *)((uintptr_t)action + 0x0004) = FindAdditionalVTable<CTFBotDead>(vt, -0x4);
	
	return action;
}


MemberFuncThunk<const CTFBotMainAction *, const CKnownEntity *, CTFBot *, const CKnownEntity *, const CKnownEntity *> CTFBotMainAction::ft_SelectCloserThreat("CTFBotMainAction::SelectCloserThreat");

MemberVFuncThunk<CTFBotEscortSquadLeader *, ActionResult<CTFBot>, CTFBot*, float> CTFBotEscortSquadLeader::vt_Update(TypeName<CTFBotEscortSquadLeader>(),"CTFBotEscortSquadLeader::Update");