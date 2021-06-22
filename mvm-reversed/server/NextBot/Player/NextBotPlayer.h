/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Player/NextBotPlayer.h
 */


class INextBotPlayerInput
{
public:
	virtual void PressFireButton(float duration = -1.0f) = 0;
	virtual void ReleaseFireButton() = 0;
	virtual void PressAltFireButton(float duration = -1.0f) = 0;
	virtual void ReleaseAltFireButton() = 0;
	virtual void PressMeleeButton(float duration = -1.0f) = 0;
	virtual void ReleaseMeleeButton() = 0;
	virtual void PressUseButton(float duration = -1.0f) = 0;
	virtual void ReleaseUseButton() = 0;
	virtual void PressReloadButton(float duration = -1.0f) = 0;
	virtual void ReleaseReloadButton() = 0;
	virtual void PressForwardButton(float duration = -1.0f) = 0;
	virtual void ReleaseForwardButton() = 0;
	virtual void PressBackwardButton(float duration = -1.0f) = 0;
	virtual void ReleaseBackwardButton() = 0;
	virtual void PressLeftButton(float duration = -1.0f) = 0;
	virtual void ReleaseLeftButton() = 0;
	virtual void PressRightButton(float duration = -1.0f) = 0;
	virtual void ReleaseRightButton() = 0;
	virtual void PressJumpButton(float duration = -1.0f) = 0;
	virtual void ReleaseJumpButton() = 0;
	virtual void PressCrouchButton(float duration = -1.0f) = 0;
	virtual void ReleaseCrouchButton() = 0;
	virtual void PressWalkButton(float duration = -1.0f) = 0;
	virtual void ReleaseWalkButton() = 0;
	virtual void SetButtonScale(float forward, float side) = 0;
};


template<class T>
class NextBotPlayer : public T, public INextBot, public INextBotPlayerInput
{
public:
	NextBotPlayer();
	virtual ~NextBotPlayer();
	
	/* T overrides */
	virtual void Spawn() override;
	virtual void Event_Killed(const CTakeDamageInfo& info) override;
	virtual INextBot *MyNextBotPointer() override;
	virtual bool IsNetClient() const override;
	virtual void Touch(CBaseEntity *pOther) override;
	virtual void PhysicsSimulate() override;
	virtual void HandleAnimEvent(animevent_t *pEvent) override;
	virtual void Weapon_Equip(CBaseCombatWeapon *pWeapon) override;
	virtual void Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pVecTarget = nullptr, const Vector *pVelocity = nullptr) override;
	virtual int OnTakeDamage_Alive(const CTakeDamageInfo& info) override;
	virtual int OnTakeDamage_Dying(const CTakeDamageInfo& info) override;
	virtual void OnNavAreaChanged(CNavArea *enteredArea, CNavArea *leftArea) override;
	virtual bool IsFakeClient() const override;
	virtual CBaseEntity *EntSelectSpawnPoint() override;
	virtual bool IsBot() const override;
	
	/* INextBot overrides */
	virtual void Update() override;
	virtual bool IsRemovedOnReset() const override;
	virtual CBaseCombatCharacter *GetEntity() const override;
	
	/* INextBotPlayerInput overrides */
	virtual void PressFireButton(float duration = -1.0f) override;
	virtual void ReleaseFireButton() override;
	virtual void PressAltFireButton(float duration = -1.0f) override;
	virtual void ReleaseAltFireButton() override;
	virtual void PressMeleeButton(float duration = -1.0f) override;
	virtual void ReleaseMeleeButton() override;
	virtual void PressUseButton(float duration = -1.0f) override;
	virtual void ReleaseUseButton() override;
	virtual void PressReloadButton(float duration = -1.0f) override;
	virtual void ReleaseReloadButton() override;
	virtual void PressForwardButton(float duration = -1.0f) override;
	virtual void ReleaseForwardButton() override;
	virtual void PressBackwardButton(float duration = -1.0f) override;
	virtual void ReleaseBackwardButton() override;
	virtual void PressLeftButton(float duration = -1.0f) override;
	virtual void ReleaseLeftButton() override;
	virtual void PressRightButton(float duration = -1.0f) override;
	virtual void ReleaseRightButton() override;
	virtual void PressJumpButton(float duration = -1.0f) override;
	virtual void ReleaseJumpButton() override;
	virtual void PressCrouchButton(float duration = -1.0f) override;
	virtual void ReleaseCrouchButton() override;
	virtual void PressWalkButton(float duration = -1.0f) override;
	virtual void ReleaseWalkButton() override;
	virtual void SetButtonScale(float forward, float side) override;
	
	virtual void SetSpawnPoint(CBaseEntity *ent);
	virtual bool IsDormantWhenDead() const;
	virtual void OnMainActivityComplete(Activity a1, Activity a2);
	virtual void OnMainActivityInterrupted(Activity a1, Activity a2);
	virtual void AvoidPlayers(CUserCmd *usercmd);
	
	float GetDistanceBetween(CBaseEntity *ent) const;
	
protected:
	int m_nBotButtons;                  // +0x2a34
	int m_nBotButtonsOld;               // +0x2a38
	CountdownTimer m_ctFire;            // +0x2a3c
	CountdownTimer m_ctAltFire;         // +0x2a48
	CountdownTimer m_ctUse;             // +0x2a54
	CountdownTimer m_ctReload;          // +0x2a60
	CountdownTimer m_ctForward;         // +0x2a6c
	CountdownTimer m_ctBackward;        // +0x2a78
	CountdownTimer m_ctLeft;            // +0x2a84
	CountdownTimer m_ctRight;           // +0x2a90
	CountdownTimer m_ctJump;            // +0x2a9c
	CountdownTimer m_ctCrouch;          // +0x2aa8
	CountdownTimer m_ctWalk;            // +0x2ab4
	CountdownTimer m_ctScale;           // +0x2ac0
	IntervalTimer  m_itBurning;         // +0x2acc
	float m_flButtonScaleForward;       // +0x2ad0
	float m_flButtonScaleSide;          // +0x2ad4
	CHandle<CBaseEntity> m_hSpawnPoint; // +0x2ad8
};
//template<> class NextBotPlayer<CTFPlayer>;


template<class T> T *NextBotCreatePlayerBot(const char *name, bool fakeclient);
