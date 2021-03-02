#ifndef _INCLUDE_SIGSEGV_STUB_TFBOT_BEHAVIOR_H_
#define _INCLUDE_SIGSEGV_STUB_TFBOT_BEHAVIOR_H_


#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"


/* fix undefined-reference linker errors related to ActionStub */
inline INextBotEventResponder::~INextBotEventResponder() {}
inline IContextualQuery::~IContextualQuery() {}
template<typename T> Action<T>::~Action() {}


class ActionStub : public Action<CTFBot>
{
public:
	virtual const char *GetName() const override { return nullptr; }
	
protected:
	ActionStub() {}
};


class CTFBotAttack : public ActionStub
{
public:
	static CTFBotAttack *New();
	
protected:
	CTFBotAttack() = delete;
	
private:
	PathFollower m_PathFollower;      // +0x0034
	ChasePath m_ChasePath;            // +0x4808
	CountdownTimer m_ctRecomputePath; // +0x9008
};
SIZE_CHECK(CTFBotAttack, 0x9014);

class CTFBotAttackFlagDefenders : public CTFBotAttack
{
public:
	CountdownTimer m_minDurationTimer;
	CountdownTimer m_watchFlagTimer;
	CHandle< CTFPlayer > m_chasePlayer;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};

class CTFBotSeekAndDestroy : public ActionStub
{
public:
	static CTFBotSeekAndDestroy *New(float duration = -1.0f);
	
protected:
	CTFBotSeekAndDestroy() = delete;
	
private:
	uint32_t m_Dword0034;        // +0x0034
	uint32_t m_Dword0038;        // +0x0038
	PathFollower m_PathFollower; // +0x003c
	CountdownTimer m_ctUnknown1; // +0x4810
	CountdownTimer m_ctUnknown2; // +0x481c
};
SIZE_CHECK(CTFBotSeekAndDestroy, 0x4828);


class CTFBotFetchFlag : public ActionStub
{
public:
	static CTFBotFetchFlag *New(bool give_up_when_done = false);
	
protected:
	CTFBotFetchFlag() = delete;
	
private:
	                                  //     GCC |    MSVC
	bool m_bGiveUpWhenDone;           // +0x0032 | +0x0034
	PathFollower m_PathFollower;      // +0x0034 | +0x0038
	CountdownTimer m_ctRecomputePath; // +0x4808 | +0x480c
};
#if defined _MSC_VER
SIZE_CHECK(CTFBotFetchFlag, 0x4818);
#else
SIZE_CHECK(CTFBotFetchFlag, 0x4814);
#endif


class CTFBotPushToCapturePoint : public ActionStub
{
public:
	static CTFBotPushToCapturePoint *New(Action<CTFBot> *done_action);
	
protected:
	CTFBotPushToCapturePoint() = delete;
	
private:
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
	Action<CTFBot> *m_DoneAction;     // +0x4814
};
SIZE_CHECK(CTFBotPushToCapturePoint, 0x4818);


class CTFBotMedicHeal : public ActionStub
{
public:
//#if TOOLCHAIN_FIXES
	static CTFBotMedicHeal *New();
//#endif
	
protected:
	CTFBotMedicHeal() = delete;
	
private:
	//ChasePath m_ChasePath;         // +0x0034
	uint8_t pad_34[0x4800];
	CountdownTimer m_ctUnknown1;   // +0x4834
	CountdownTimer m_ctUnknown2;   // +0x4840
	CHandle<CTFPlayer> m_hPatient; // +0x484c
	Vector m_vecUnknown1;          // +0x4850
	CountdownTimer m_ctUnknown3;   // +0x485c
	uint32_t m_Dword4868;          // +0x4868
	CountdownTimer m_ctUnknown4;   // +0x486c
	PathFollower m_PathFollower;   // +0x4878
	Vector m_vecFollowPosition;    // +0x904c
};
SIZE_CHECK(CTFBotMedicHeal, 0x9058);


class CTFBotSniperLurk : public ActionStub
{
public:
//#if TOOLCHAIN_FIXES
	static CTFBotSniperLurk *New();
//#endif
	
protected:
	CTFBotSniperLurk() = default;
	
private:
	CountdownTimer m_ctUnknown1; // +0x0034
	CountdownTimer m_ctUnknown2; // +0x0040
	PathFollower m_PathFollower; // +0x004c
	uint8_t pad_4820[0x14];      // +0x4820
	CountdownTimer m_ctUnknown3; // +0x4834
	uint8_t pad_4840[0x1c];      // +0x4840
};
SIZE_CHECK(CTFBotSniperLurk, 0x485c);


class CTFBotMedicRetreat : public ActionStub
{
public:
	static CTFBotMedicRetreat *New();
	
protected:
	CTFBotMedicRetreat() = default;
	
private:
	PathFollower m_PathFollower;        // +0x0034
	CountdownTimer m_ctLookForPatients; // +0x4808
};
SIZE_CHECK(CTFBotMedicRetreat, 0x4814);


class CTFBotSpyInfiltrate : public ActionStub
{
public:
	static CTFBotSpyInfiltrate *New();
	
protected:
	CTFBotSpyInfiltrate() = default;
	
private:
	CountdownTimer m_ctUnknown1; // +0x0034
	PathFollower m_PathFollower; // +0x0040
	uint32_t m_Dword4814;        // +0x4814
	CountdownTimer m_ctUnknown2; // +0x4818
	CountdownTimer m_ctUnknown3; // +0x4824
	uint32_t m_Dword4830;        // +0x4830
};
SIZE_CHECK(CTFBotSpyInfiltrate, 0x4834);


class CTFBotEngineerBuild : public ActionStub
{
public:
	static CTFBotEngineerBuild *New();
	
protected:
	CTFBotEngineerBuild() = default;
};
SIZE_CHECK(CTFBotEngineerBuild, 0x0034);


class CTFBotDead : public ActionStub
{
public:
	static CTFBotDead *New();
	
protected:
	CTFBotDead() = default;
	
private:
	IntervalTimer m_itDeathEpoch; // +0x0034
};
SIZE_CHECK(CTFBotDead, 0x0038);


class CTFBotMainAction : public ActionStub
{
public:
	const CKnownEntity *SelectCloserThreat(CTFBot *actor, const CKnownEntity *threat1, const CKnownEntity *threat2) const { return ft_SelectCloserThreat(this, actor, threat1, threat2); }
	
protected:
//	CTFBotMainAction() = default;
	CTFBotMainAction() = delete;
	
private:
	static MemberFuncThunk<const CTFBotMainAction *, const CKnownEntity *, CTFBot *, const CKnownEntity *, const CKnownEntity *> ft_SelectCloserThreat;
};

class CTFBotMeleeAttack : public ActionStub
{
private:
	float m_giveUpRange;			// if non-negative and if threat is farther than this, give up our melee attack
	ChasePath m_path;
};

// TODO: SIZE_CHECK etc

class CTFBotEscortSquadLeader : public Action<CTFBot>
{
public:
	ActionResult< CTFBot >	Update( CTFBot *me, float interval ) { return vt_Update(this, me, interval); }

	CTFBot *GetActorPublic() { return this->GetActor(); }
protected:
	CTFBotEscortSquadLeader() = delete;

private:
	static MemberVFuncThunk<CTFBotEscortSquadLeader *, ActionResult<CTFBot>, CTFBot*, float> vt_Update;
	
public:
	Action< CTFBot > *m_actionToDoAfterSquadDisbands;
	CTFBotMeleeAttack m_meleeAttackAction;

	PathFollower m_formationPath;
};

struct CTFBotMissionSuicideBomber : public Action<CTFBot>
	{
		CHandle<CBaseEntity> m_hTarget;
		Vector m_vecTargetPos;
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
		CountdownTimer m_ctPlaySound;
		CountdownTimer m_ctDetonation;
		bool m_bDetonating;
		bool m_bDetReachedGoal;
		bool m_bDetLostAllHealth;
		int m_nConsecutivePathFailures;
		Vector m_vecDetonatePos;
	};

#endif
