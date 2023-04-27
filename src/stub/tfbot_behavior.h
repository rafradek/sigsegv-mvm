#ifndef _INCLUDE_SIGSEGV_STUB_TFBOT_BEHAVIOR_H_
#define _INCLUDE_SIGSEGV_STUB_TFBOT_BEHAVIOR_H_


#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"


class CTFBotMvMEngineerHintFinder
{
public:
	static bool FindHint(bool box_check, bool out_of_range_ok, CHandle<CTFBotHintEngineerNest> *the_hint) {return ft_FindHint(box_check,out_of_range_ok,the_hint);}
private:
	static StaticFuncThunk<bool, bool, bool, CHandle<CTFBotHintEngineerNest> *>ft_FindHint;
};

class CTFBotHint;


class CTFBotAttack : public ActionStub<CTFBot>
{
public:
	static CTFBotAttack *New();
	
protected:
	CTFBotAttack() = delete;
	
private:
	PathFollower   m_PathFollower;    // +0x0034
	ChasePath      m_ChasePath;       // +0x4808
	CountdownTimer m_ctRecomputePath; // +0x9008
};
SIZE_CHECK(CTFBotAttack, 0x9014);


class CTFBotSeekAndDestroy : public ActionStub<CTFBot>
{
public:
	static CTFBotSeekAndDestroy *New(float duration = -1.0f);
	
protected:
	CTFBotSeekAndDestroy() = delete;
	
private:
	CTFNavArea    *m_GoalArea;         // +0x0034
	bool           m_bPointLocked;     // +0x0038
	PathFollower   m_PathFollower;     // +0x003c
	CountdownTimer m_ctUnknown1;       // +0x4810
	CountdownTimer m_ctActionDuration; // +0x481c
};
SIZE_CHECK(CTFBotSeekAndDestroy, 0x4828);


class CTFBotFetchFlag : public ActionStub<CTFBot>
{
public:
	static CTFBotFetchFlag *New(bool give_up_when_done = false);
	
protected:
	CTFBotFetchFlag() = delete;
	
private:
	                                  //     GCC |    MSVC
	bool           m_bGiveUpWhenDone; // +0x0032 | +0x0034
	PathFollower   m_PathFollower;    // +0x0034 | +0x0038
	CountdownTimer m_ctRecomputePath; // +0x4808 | +0x480c
};
#if defined _MSC_VER
SIZE_CHECK(CTFBotFetchFlag, 0x4818);
#else
SIZE_CHECK(CTFBotFetchFlag, 0x4814);
#endif


class CTFBotPushToCapturePoint : public ActionStub<CTFBot>
{
public:
	static CTFBotPushToCapturePoint *New(Action<CTFBot> *done_action);
	
protected:
	CTFBotPushToCapturePoint() = delete;
	
private:
	PathFollower    m_PathFollower;    // +0x0034
	CountdownTimer  m_ctRecomputePath; // +0x4808
	Action<CTFBot> *m_DoneAction;      // +0x4814
};
SIZE_CHECK(CTFBotPushToCapturePoint, 0x4818);


class CTFBotMedicHeal : public ActionStub<CTFBot>
{
public:
#if TOOLCHAIN_FIXES
	static CTFBotMedicHeal *New();
#endif
	
protected:
	CTFBotMedicHeal() = default;
	
private:
	ChasePath          m_ChasePath;          // +0x0034
	CountdownTimer     m_ctUnknown1;         // +0x4834
	CountdownTimer     m_ctUberDelay;        // +0x4840
	CHandle<CTFPlayer> m_hPatient;           // +0x484c
	Vector             m_vecPatientPosition; // +0x4850
	CountdownTimer     m_ctUnknown3;         // +0x485c
	uint32_t           m_Dword4868;          // +0x4868
	CountdownTimer     m_ctUnknown4;         // +0x486c
	PathFollower       m_PathFollower;       // +0x4878
	Vector             m_vecFollowPosition;  // +0x904c
};
SIZE_CHECK(CTFBotMedicHeal, 0x9058);


class CTFBotSniperLurk : public ActionStub<CTFBot>
{
public:
	static CTFBotSniperLurk *New();

	
protected:
	CTFBotSniperLurk() = default;
	//CTFBotSniperLurk() = delete;
	
private:
	CountdownTimer           m_ctPatience;      // +0x0034
	CountdownTimer           m_ctRecomputePath; // +0x0040
	PathFollower             m_PathFollower;    // +0x004c
	int                      m_nImpatience;     // +0x4820
	Vector                   m_vecHome;         // +0x4824
	bool                     m_bHasHome;        // +0x4830
	bool                     m_bNearHome;       // +0x4831
	CountdownTimer           m_ctFindNewHome;   // +0x4834
	bool                     m_bOpportunistic;  // +0x4840
	CUtlVector<CTFBotHint *> m_Hints;           // +0x4844
	CHandle<CTFBotHint>      m_hHint;           // +0x4858
};
SIZE_CHECK(CTFBotSniperLurk, 0x485c);


class CTFBotMedicRetreat : public ActionStub<CTFBot>
{
public:
	static CTFBotMedicRetreat *New();
	
protected:
	CTFBotMedicRetreat() = default;
	
private:
	PathFollower   m_PathFollower;      // +0x0034
	CountdownTimer m_ctLookForPatients; // +0x4808
};
SIZE_CHECK(CTFBotMedicRetreat, 0x4814);


class CTFBotSpyInfiltrate : public ActionStub<CTFBot>
{
public:
	static CTFBotSpyInfiltrate *New();
	
protected:
	CTFBotSpyInfiltrate() = default;
	
private:
	CountdownTimer m_ctRecomputePath;  // +0x0034
	PathFollower   m_PathFollower;     // +0x0040
	CTFNavArea    *m_HidingArea;       // +0x4814
	CountdownTimer m_ctFindHidingArea; // +0x4818
	CountdownTimer m_ctWait;           // +0x4824
	bool           m_bCloaked;         // +0x4830
};
SIZE_CHECK(CTFBotSpyInfiltrate, 0x4834); // +0x4831


class CTFBotEngineerBuild : public ActionStub<CTFBot>
{
public:
	static CTFBotEngineerBuild *New();
	
protected:
	CTFBotEngineerBuild() = default;
};
SIZE_CHECK(CTFBotEngineerBuild, 0x0034);


class CTFBotDead : public ActionStub<CTFBot>
{
public:
	static CTFBotDead *New();
	
protected:
	CTFBotDead() = default;
	
private:
	IntervalTimer m_itDeathEpoch; // +0x0034
};
SIZE_CHECK(CTFBotDead, 0x0038);


class CTFBotUseItem : public ActionStub<CTFBot>
{
protected:
	CTFBotUseItem() = delete;
	
public: // accessed by Debug:UseItem_Broken and AI:Improved_UseItem mods
	CHandle<CTFWeaponBase> m_hItem;          // +0x0034
	CountdownTimer         m_ctInitialDelay; // +0x0038
};
SIZE_CHECK(CTFBotUseItem, 0x0044);


class CTFBotMissionSuicideBomber : public ActionStub<CTFBot>
{
public:
	static CTFBotMissionSuicideBomber *New();
	
protected:
	CTFBotMissionSuicideBomber() = delete;
	
public: // accessed by Debug:Suicide_Bomber mod
	CHandle<CBaseEntity> m_hTarget;                  // +0x0034
	Vector               m_vecTargetPos;             // +0x0038
	PathFollower         m_PathFollower;             // +0x0044
	CountdownTimer       m_ctRecomputePath;          // +0x4818
	CountdownTimer       m_ctPlaySound;              // +0x4824
	CountdownTimer       m_ctDetonation;             // +0x4830
	bool                 m_bDetonating;              // +0x483c
	bool                 m_bDetReachedGoal;          // +0x483d
	bool                 m_bDetLostAllHealth;        // +0x483e
	int                  m_nConsecutivePathFailures; // +0x4840
	Vector               m_vecDetonatePos;           // +0x4844
};
SIZE_CHECK(CTFBotMissionSuicideBomber, 0x4850);


class CTFBotMainAction : public ActionStub<CTFBot>
{
public:
	const CKnownEntity *SelectCloserThreat(CTFBot *actor, const CKnownEntity *threat1, const CKnownEntity *threat2) const { return ft_SelectCloserThreat(this, actor, threat1, threat2); }
	
protected:
//	CTFBotMainAction() = default;
	CTFBotMainAction() = delete;
	
private:
	static MemberFuncThunk<const CTFBotMainAction *, const CKnownEntity *, CTFBot *, const CKnownEntity *, const CKnownEntity *> ft_SelectCloserThreat;
};
// TODO: SIZE_CHECK etc

class CTFBotMeleeAttack : public ActionStub<CTFBot>
{
private:
	float m_giveUpRange;			// if non-negative and if threat is farther than this, give up our melee attack
	ChasePath m_path;
};

class CTFBotEscortSquadLeader : public ActionStub<CTFBot>
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

class CTFBotAttackFlagDefenders : public CTFBotAttack
{
public:
	CountdownTimer m_minDurationTimer;
	CountdownTimer m_watchFlagTimer;
	CHandle< CTFPlayer > m_chasePlayer;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};

class CTFBotScenarioMonitor : public ActionStub<CTFBot>
{

};


class CTFBotTacticalMonitor : public ActionStub<CTFBot>
{
public:
	Action<CTFBot> *InitialContainedActionTactical(CTFBot *actor) { return ft_InitialContainedAction(this, actor); }
private:
	static MemberFuncThunk<CTFBotTacticalMonitor *, Action<CTFBot> *, CTFBot *> ft_InitialContainedAction;
};

#endif
