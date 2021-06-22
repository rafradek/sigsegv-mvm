/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Player/NextBotPlayer.cpp
 */


ConVar NextBotPlayerStop("nb_player_stop", "0", FCVAR_CHEAT,
	"Stop all NextBotPlayers from updating");
ConVar NextBotPlayerWalk("nb_player_walk", "0", FCVAR_CHEAT,
	"Force bots to walk");
ConVar NextBotPlayerCrouch("nb_player_crouch", "0", FCVAR_CHEAT,
	"Force bots to crouch");
ConVar NextBotPlayerMove("nb_player_move", "0", FCVAR_CHEAT,
	"Prevents bots from moving");


// TODO: these should all actually be inline definitions in NextBotPlayer.h


template<class T> NextBotPlayer<T>::NextBotPlayer()
{
}

template<class T> NextBotPlayer<T>::~NextBotPlayer()
{
}


template<class T> void NextBotPlayer<T>::Spawn()
{
	engine->SetFakeClientConVarValue(this->edict(), "cl_autohelp", "0");
	
	this->m_ctFire.Invalidate();
	this->m_ctAltFire.Invalidate();
	this->m_ctUse.Invalidate();
	this->m_ctReload.Invalidate();
	this->m_ctForward.Invalidate();
	this->m_ctBackward.Invalidate();
	this->m_ctLeft.Invalidate();
	this->m_ctRight.Invalidate();
	this->m_ctJump.Invalidate();
	this->m_ctCrouch.Invalidate();
	this->m_ctWalk.Invalidate();
	this->m_ctScale.Invalidate();
	this->m_itBurning.Invalidate();
	
	this->m_nBotButtons    = 0;
	this->m_nBotButtonsOld = 0;
	
	this->m_flButtonScaleSide    = 0.04f;
	this->m_flButtonScaleForward = 0.04f;
	
	INextBot::Reset();
	T::Spawn();
}

template<class T> void NextBotPlayer<T>::Event_Killed(const CTakeDamageInfo& info)
{
	this->OnKilled(info);
	T::Event_Killed(info);
}

template<class T> INextBot *NextBotPlayer<T>::MyNextBotPointer()
{
	return static_cast<INextBot *>(this);
}

template<class T> bool NextBotPlayer<T>::IsNetClient() const
{
	return false;
}

template<class T> void NextBotPlayer<T>::Touch(CBaseEntity *pOther)
{
	if (this->ShouldTouch(pOther)) {
		/* should have used a reference here; it copies the whole trace_t! */
		trace_t trace = CBaseEntity::GetTouchTrace();
		this->OnContact(pOther, &trace);
	}
	
	T::Touch(pOther);
}

template<class T> void NextBotPlayer<T>::PhysicsSimulate()
{
	VPROF("NextBotPlayer::PhysicsSimulate");
	
	if (engine->IsPaused() ||
		(this->IsDormantWhenDead() && this->m_lifestate == LIFE_DEAD) ||
		NextBotPlayerStop.GetBool()) {
		T::PhysicsSimulate();
		return;
	}
	
	int buttons;
	
	if (INextBot::BeginUpdate()) {
		this->Update();
		
		if (!this->m_ctFire.IsElapsed()) {
			this->m_nBotButtons |= IN_ATTACK;
		}
		if (!this->m_ctAltFire.IsElapsed()) {
			this->m_nBotButtons |= IN_ATTACK2;
		}
		if (!this->m_ctUse.IsElapsed()) {
			this->m_nBotButtons |= IN_USE;
		}
		if (!this->m_ctReload.IsElapsed()) {
			this->m_nBotButtons |= IN_RELOAD;
		}
		if (!this->m_ctForward.IsElapsed()) {
			this->m_nBotButtons |= IN_FORWARD;
		}
		if (!this->m_ctBackward.IsElapsed()) {
			this->m_nBotButtons |= IN_BACK;
		}
		if (!this->m_ctLeft.IsElapsed()) {
			this->m_nBotButtons |= IN_MOVELEFT;
		}
		if (!this->m_ctRight.IsElapsed()) {
			this->m_nBotButtons |= IN_MOVERIGHT;
		}
		if (!this->m_ctJump.IsElapsed()) {
			this->m_nBotButtons |= IN_JUMP;
		}
		if (!this->m_ctCrouch.IsElapsed()) {
			this->m_nBotButtons |= IN_DUCK;
		}
		if (!this->m_ctWalk.IsElapsed()) {
			this->m_nBotButtons |= IN_SPEED;
		}
		
		buttons = this->m_nBotButtons;
		this->m_nBotButtonsOld = buttons;
		
		INextBot::EndUpdate();
	} else {
		this->GetBodyInterface()->Update();
		buttons = this->m_nBotButtonsOld | this->m_nBotButtons;
	}
	
	ILocomotion *loco = this->GetLocomotionInterface();
	
	if (this->GetBodyInterface()->IsActualPosture(IBody::PosureType::CROUCH)) {
		buttons |= IN_DUCK;
	}
	
	float upmove = 0.0f;
	if ((buttons & IN_JUMP) != 0) {
		upmove = loco->GetRunSpeed();
	}
	
	float forwardmove = 0.0f;
	if ((buttons & IN_FORWARD) != 0) {
		forwardmove = loco->GetRunSpeed();
	} else if ((buttons & IN_BACK) != 0) {
		forwardmove = -loco->GetRunSpeed();
	}
	
	float sidemove = 0.0f;
	if ((buttons & IN_MOVELEFT) != 0) {
		sidemove = -loco->GetRunSpeed();
	} else if ((buttons & IN_MOVERIGHT) != 0) {
		sidemove = loco->GetRunSpeed();
	}
	
	if (NextBotPlayerWalk.GetBool()) {
		buttons |= IN_SPEED;
	}
	
	if (NextBotPlayerCrouch.GetBool()) {
		buttons |= IN_DUCK;
	}
	
	/* bug: this doesn't set negative values for backward/leftward movement;
	 * nor does it set zero if not moving */
	if (!this->m_ctScale.IsElapsed()) {
		forwardmove = loco->GetRunSpeed() * this->m_flButtonScaleForward;
		sidemove    = loco->GetRunSpeed() * this->m_flButtonScaleSide;
	}
	
	if (!NextBotPlayerMove.GetBool()) {
		buttons &= ~(
			IN_JUMP |
			IN_FORWARD |
			IN_BACK |
			IN_MOVELEFT |
			IN_MOVERIGHT);
		
		upmove = 0.0f;
		sidemove = 0.0f;
		forwardmove = 0.0f;
	}
	
	CUserCmd usercmd = {
		.viewangles     = this->EyeAngles(),
		.forwardmove    = forwardmove,
		.sidemove       = sidemove,
		.upmove         = upmove,
		.command_number = gpGlobals->tickcount,
		.buttons        = buttons,
		.random_seed    = random_valve->RandomInt(0, INT_MAX),
	};
	this->AvoidPlayers(&usercmd);
	this->ProcessUserCmds(&usercmd, 1, 1, 0, 0);
	
	this->m_nBotButtons = 0;
	
	T::PhysicsSimulate();
}

template<class T> void NextBotPlayer<T>::HandleAnimEvent(animevent_t *pEvent)
{
	this->OnAnimationEvent(pEvent);
	T::HandleAnimEvent(pEvent);
}

template<class T> void NextBotPlayer<T>::Weapon_Equip(CBaseCombatWeapon *pWeapon)
{
	this->OnPickUp(pWeapon, nullptr);
	T::Weapon_Equip(pWeapon);
}

template<class T> void NextBotPlayer<T>::Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pVecTarget = nullptr, const Vector *pVelocity = nullptr)
{
	this->OnDrop(pWeapon);
	T::Weapon_Drop(pWeapon, pVecTarget, pVelocity);
}

template<class T> int NextBotPlayer<T>::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	if ((info->GetDamageType() & DMG_BURN) != 0) {
		if (!this->m_itBurning.HasStarted ||
			this->m_itBurning.IsGreaterThan(1.0f)) {
			this->OnIgnite();
			this->m_itBurning.Start();
		}
	}
	
	this->OnInjured(info);
	return T::OnTakeDamage_Alive(info);
}

template<class T> int NextBotPlayer<T>::OnTakeDamage_Dying(const CTakeDamageInfo& info)
{
	if ((info->GetDamageType() & DMG_BURN) != 0) {
		if (!this->m_itBurning.HasStarted ||
			this->m_itBurning.IsGreaterThan(1.0f)) {
			this->OnIgnite();
			this->m_itBurning.Start();
		}
	}
	
	this->OnInjured(info);
	return T::OnTakeDamage_Dying(info);
}

template<class T> void NextBotPlayer<T>::OnNavAreaChanged(CNavArea *enteredArea, CNavArea *leftArea)
{
	FOR_EACH_RESPONDER {
		responder->OnNavAreaChanged(enteredArea, leftArea);
	}
	
	T::OnNavAreaChanged(enteredArea, leftArea);
}

template<class T> bool NextBotPlayer<T>::IsFakeClient() const
{
	return true;
}

template<class T> CBaseEntity *NextBotPlayer<T>::EntSelectSpawnPoint()
{
	CBaseEntity *ent = this->m_hSpawnPoint();
	
	if (ent != nullptr) {
		return ent;
	} else {
		return T::EntSelectSpawnPoint();
	}
}

template<class T> bool NextBotPlayer<T>::IsBot() const
{
	return true;
}


template<class T> void NextBotPlayer<T>::Update()
{
	if (!this->IsAlive() && this->IsDormantWhenDead()) {
		return;
	}
	
	if (NextBotPlayerStop.GetBool()) {
		return;
	}
	
	INextBot::Update();
}

template<class T> bool NextBotPlayer<T>::IsRemovedOnReset() const
{
	return false;
}

template<class T> CBaseCombatCharacter *NextBotPlayer<T>::GetEntity() const
{
	return this;
}


template<class T> void NextBotPlayer<T>::PressFireButton(float duration)
{
	this->m_nBotButtons |= IN_ATTACK;
	this->m_ctFire.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseFireButton()
{
	this->m_nBotButtons &= ~IN_ATTACK;
	this->m_ctFire.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressAltFireButton(float duration)
{
	this->PressMeleeButton(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseAltFireButton()
{
	this->ReleaseMeleeButton();
}

template<class T> void NextBotPlayer<T>::PressMeleeButton(float duration)
{
	this->m_nBotButtons |= IN_ATTACK2;
	this->m_ctAltFire.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseMeleeButton()
{
	this->m_nBotButtons &= ~IN_ATTACK2;
	this->m_ctAltFire.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressUseButton(float duration)
{
	this->m_nBotButtons |= IN_USE;
	this->m_ctUse.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseUseButton()
{
	this->m_nBotButtons &= ~IN_USE;
	this->m_ctUse.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressReloadButton(float duration)
{
	this->m_nBotButtons |= IN_RELOAD;
	this->m_ctReload.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseReloadButton()
{
	this->m_nBotButtons &= ~IN_RELOAD;
	this->m_ctReload.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressForwardButton(float duration)
{
	this->m_nBotButtons |= IN_FORWARD;
	this->m_ctForward.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseForwardButton()
{
	this->m_nBotButtons &= ~IN_FORWARD;
	this->m_ctForward.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressBackwardButton(float duration)
{
	this->m_nBotButtons |= IN_BACK;
	this->m_ctBackward.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseBackwardButton()
{
	this->m_nBotButtons &= ~IN_BACK;
	this->m_ctBackward.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressLeftButton(float duration)
{
	this->m_nBotButtons |= IN_MOVELEFT;
	this->m_ctLeft.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseLeftButton()
{
	this->m_nBotButtons &= ~IN_MOVELEFT;
	this->m_ctLeft.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressRightButton(float duration)
{
	this->m_nBotButtons |= IN_MOVERIGHT;
	this->m_ctRight.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseRightButton()
{
	this->m_nBotButtons &= ~IN_MOVERIGHT;
	this->m_ctRight.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressJumpButton(float duration)
{
	this->m_nBotButtons |= IN_JUMP;
	this->m_ctJump.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseJumpButton()
{
	this->m_nBotButtons &= ~IN_JUMP;
	this->m_ctJump.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressCrouchButton(float duration)
{
	this->m_nBotButtons |= IN_DUCK;
	this->m_ctCrouch.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseCrouchButton()
{
	this->m_nBotButtons &= ~IN_DUCK;
	this->m_ctCrouch.Invalidate();
}

template<class T> void NextBotPlayer<T>::PressWalkButton(float duration)
{
	this->m_nBotButtons |= IN_SPEED;
	this->m_ctWalk.Start(duration);
}

template<class T> void NextBotPlayer<T>::ReleaseWalkButton()
{
	this->m_nBotButtons &= ~IN_SPEED;
	this->m_ctWalk.Invalidate();
}

template<class T> void NextBotPlayer<T>::SetButtonScale(float forward, float side)
{
	this->m_flButtonScaleForward = f1;
	this->m_flButtonScaleSide    = f2;
	this->m_ctScale.Start(0.001f);
}


template<class T> void NextBotPlayer<T>::SetSpawnPoint(CBaseEntity *ent)
{
	this->m_hSpawnPoint = ent;
}

template<class T> bool NextBotPlayer<T>::IsDormantWhenDead() const
{
	return true;
}

template<class T> void NextBotPlayer<T>::OnMainActivityComplete(Activity a1, Activity a2)
{
	this->OnAnimationActivityComplete(a2);
}

template<class T> void NextBotPlayer<T>::OnMainActivityInterrupted(Activity a1, Activity a2)
{
	this->OnAnimationActivityInterrupted(a2);
}

template<class T> void NextBotPlayer<T>::AvoidPlayers(CUserCmd *usercmd)
{
}


template<class T> float NextBotPlayer<T>::GetDistanceBetween(CBaseEntity *ent) const
{
	Vector& pos_me  = this->GetAbsOrigin();
	Vector& pos_ent = ent->GetAbsOrigin();
	
	return pos_me.DistTo(pos_ent);
}


template<class T> T *NextBotCreatePlayerBot(const char *name, bool fakeclient)
{
	ClientPutInServerOverride(T::AllocatePlayerEntity);
	edict_t *client = engine->CreateFakeClientEx(name, fakeclient);
	ClientPutInServerOverride(nullptr);
	
	if (client == nullptr) {
		Msg("CreatePlayerBot: Unable to create bot %s - "
			"CreateFakeClient() returned NULL.\n", name);
		return nullptr;
	}
	
	T *bot = dynamic_cast<T *>(CBaseEntity::Instance(client));
	if (bot == nullptr) {
		Error("CreatePlayerBot: Could not Instance() from the bot edict.\n");
		return nullptr;
	}
	
	bot->SetPlayerName(name);
	bot->ClearFlags();
	bot->AddFlag(FL_CLIENT | FL_FAKECLIENT);
	
	return bot;
}
