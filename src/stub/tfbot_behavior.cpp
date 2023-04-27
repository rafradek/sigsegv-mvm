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


//static MemberVFuncThunk<CTFBotMedicHeal *, void> ft_CTFBotMedicHeal_ctor(TypeName<CTFBotMedicHeal>(),"CTFBotMedicHeal::CTFBotMedicHeal [C1]");
#if TOOLCHAIN_FIXES
CTFBotMedicHeal *CTFBotMedicHeal::New()
{
	// TODO: verify sizeof(CTFBotMedicHeal) in the game code at runtime
	// TODO: verify that the addr for the ctor actually exists
	
	auto action = new CTFBotMedicHeal();
	
	/* overwrite vtable pointers */
	action->OverwriteVTPtrs<CTFBotMedicHeal, CTFBotMedicHeal, IContextualQuery>();
	return action;
}
#endif


CTFBotMedicRetreat *CTFBotMedicRetreat::New()
{
	// TODO: verify sizeof(CTFBotMedicRetreat) in the game code at runtime
	// TODO: verify that the addr for the vtable actually exists
	
	auto action = new CTFBotMedicRetreat();
	
	/* overwrite vtable pointers */
	action->OverwriteVTPtrs<CTFBotMedicRetreat, CTFBotMedicRetreat, IContextualQuery>();
	return action;
}

// Inlined in the code
// static MemberFuncThunk<CTFBotSniperLurk *, void> ft_CTFBotSniperLurk_ctor("CTFBotSniperLurk::CTFBotSniperLurk [C1]");
CTFBotSniperLurk *CTFBotSniperLurk::New()
{
	// TODO: verify sizeof(CTFBotSniperLurk) in the game code at runtime
	// TODO: verify that the addr for the ctor actually exists
	
	auto action = new CTFBotSniperLurk(); //reinterpret_cast<CTFBotSniperLurk *>(::operator new(sizeof(CTFBotSniperLurk)));
	//memset(action, 0, sizeof(CTFBotSniperLurk));
	action->OverwriteVTPtrs<CTFBotSniperLurk, CTFBotSniperLurk, IContextualQuery>();
	//ft_CTFBotSniperLurk_ctor(action);
	return action;
}


CTFBotSpyInfiltrate *CTFBotSpyInfiltrate::New()
{
	// TODO: verify sizeof(CTFBotSpyInfiltrate) in the game code at runtime
	// TODO: verify that the addr for the vtable actually exists
	
	auto action = new CTFBotSpyInfiltrate();
	action->OverwriteVTPtrs<CTFBotSpyInfiltrate, CTFBotSpyInfiltrate, IContextualQuery>();
	return action;
}


CTFBotEngineerBuild *CTFBotEngineerBuild::New()
{
	// TODO: verify sizeof(CTFBotEngineerBuild) in the game code at runtime
	// TODO: verify that the addr for the vtable actually exists
	
	auto action = new CTFBotEngineerBuild();
	action->OverwriteVTPtrs<CTFBotEngineerBuild, CTFBotEngineerBuild, IContextualQuery>();
	return action;
}


CTFBotDead *CTFBotDead::New()
{
	// TODO: verify sizeof(CTFBotDead) in the game code at runtime
	// TODO: verify that the addr for the vtable actually exists
	
	auto action = new CTFBotDead();
	action->OverwriteVTPtrs<CTFBotDead, CTFBotDead, IContextualQuery>();
	return action;
}


MemberFuncThunk<const CTFBotMainAction *, const CKnownEntity *, CTFBot *, const CKnownEntity *, const CKnownEntity *> CTFBotMainAction::ft_SelectCloserThreat("CTFBotMainAction::SelectCloserThreat");

MemberVFuncThunk<CTFBotEscortSquadLeader *, ActionResult<CTFBot>, CTFBot*, float> CTFBotEscortSquadLeader::vt_Update(TypeName<CTFBotEscortSquadLeader>(),"CTFBotEscortSquadLeader::Update");

StaticFuncThunk<bool, bool, bool, CHandle<CTFBotHintEngineerNest> *> CTFBotMvMEngineerHintFinder::ft_FindHint("CTFBotMvMEngineerHintFinder::FindHint");

MemberFuncThunk<CTFBotTacticalMonitor *, Action<CTFBot> *, CTFBot *> CTFBotTacticalMonitor::ft_InitialContainedAction("CTFBotTacticalMonitor::InitialContainedAction");