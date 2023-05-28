#ifndef _INCLUDE_SIGSEGV_STUB_BASEPLAYER_H_
#define _INCLUDE_SIGSEGV_STUB_BASEPLAYER_H_


#include "stub/baseanimating.h"
#include "PlayerState.h"

class CNavArea;
class CBaseCombatWeapon;
class CEconWearable;
class CBaseViewModel;

class CBaseCombatCharacter : public CBaseFlex
{
public:
	enum FieldOfViewCheckType { USE_FOV, DISREGARD_FOV };
	
	CBaseCombatWeapon *GetActiveWeapon() const { return this->m_hActiveWeapon; }
	void SetActiveWeapon(CBaseCombatWeapon * weapon) { this->m_hActiveWeapon = weapon; }
	int WeaponCount() const                    { return MAX_WEAPONS; }
	CBaseCombatWeapon *GetWeapon(int i) const  { return this->m_hMyWeapons[i]; }
	static_assert(MAX_WEAPONS == 48);
	
#ifdef SE_TF2
	void AddGlowEffect()                                                        {        ft_AddGlowEffect     (this); }
	void RemoveGlowEffect()                                                     {        ft_RemoveGlowEffect  (this); }
	bool IsGlowEffectActive()                                                   { return ft_IsGlowEffectActive(this); }
#endif
	bool IsAbleToSee(const CBaseEntity *entity, FieldOfViewCheckType checkFOV)  { return ft_IsAbleToSee_ent   (this, entity, checkFOV); }
	bool IsAbleToSee(CBaseCombatCharacter *pBCC, FieldOfViewCheckType checkFOV) { return ft_IsAbleToSee_bcc   (this, pBCC, checkFOV); }
	void SetBloodColor(int nBloodColor)                                         {        ft_SetBloodColor     (this, nBloodColor); }
	bool Weapon_Detach(CBaseCombatWeapon *pWeapon)                              { return ft_Weapon_Detach     (this, pWeapon); }
	bool SwitchToNextBestWeapon(CBaseCombatWeapon *weapon)                      { return ft_SwitchToNextBestWeapon(this, weapon); }
	void SetAmmoCount(int count, int ammoIndex)                                 {        ft_SetAmmoCount      (this, count, ammoIndex); }
	Vector CalcDamageForceVector(const CTakeDamageInfo &info)                   { return ft_CalcDamageForceVector(this, info); }
	
	CBaseCombatWeapon *Weapon_GetSlot(int slot) const                      { return vt_Weapon_GetSlot     (this, slot); }
	bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon)                    { return vt_Weapon_CanSwitchTo (this, pWeapon); }
	bool Weapon_Switch(CBaseCombatWeapon *pWeapon, int viewmodelindex = 0) { return vt_Weapon_Switch      (this, pWeapon, viewmodelindex); }
	void Weapon_Equip(CBaseCombatWeapon *pWeapon)                          { vt_Weapon_Equip     	      (this, pWeapon); }
	CNavArea *GetLastKnownArea() const                                     { return vt_GetLastKnownArea   (this); }
	void UpdateLastKnownArea()                                             {        vt_UpdateLastKnownArea(this); }
	int GiveAmmo(int iCount, int iAmmoIndex, bool bSuppressSound = false)  { return vt_GiveAmmo           (this, iCount, iAmmoIndex, bSuppressSound); }
	int GetAmmoCount(int iAmmoIndex) const                                 { return vt_GetAmmoCount       (this, iAmmoIndex); }
	bool ShouldGib(const CTakeDamageInfo& info)                            { return vt_ShouldGib          (this, info); }
	bool FInViewCone(const Vector& vec)                                    { return vt_FInViewCone        (this, vec); }
	bool BecomeRagdoll(const CTakeDamageInfo& info, const Vector &force)   { return vt_BecomeRagdoll      (this, info, force); }
	void RemoveAmmo(int amount, int type)                                  { return vt_RemoveAmmo         (this, amount, type); }
	

#ifdef SE_TF2
	int GetBossType()                                                      { return vt_GetBossType        (this); }
#endif
	void ClearLastKnownArea()                                              {        vt_ClearLastKnownArea (this); }
private:
	DECL_SENDPROP(CHandle<CBaseCombatWeapon>,              m_hActiveWeapon);
	DECL_SENDPROP(CHandle<CBaseCombatWeapon>[MAX_WEAPONS], m_hMyWeapons);
	
#ifdef SE_TF2
	static MemberFuncThunk<CBaseCombatCharacter *, void>                                               ft_AddGlowEffect;
	static MemberFuncThunk<CBaseCombatCharacter *, void>                                               ft_RemoveGlowEffect;
	static MemberFuncThunk<CBaseCombatCharacter *, bool>                                               ft_IsGlowEffectActive;
#endif
	static MemberFuncThunk<CBaseCombatCharacter *, bool, const CBaseEntity *, FieldOfViewCheckType>    ft_IsAbleToSee_ent;
	static MemberFuncThunk<CBaseCombatCharacter *, bool, CBaseCombatCharacter *, FieldOfViewCheckType> ft_IsAbleToSee_bcc;
	static MemberFuncThunk<CBaseCombatCharacter *, void, int>                                          ft_SetBloodColor;
	static MemberFuncThunk<CBaseCombatCharacter *, bool, CBaseCombatWeapon *>                          ft_Weapon_Detach;
	static MemberFuncThunk<CBaseCombatCharacter *, bool, CBaseCombatWeapon *>                          ft_SwitchToNextBestWeapon;
	static MemberFuncThunk<CBaseCombatCharacter *, void, int, int>                                     ft_SetAmmoCount;
	static MemberFuncThunk<CBaseCombatCharacter *, Vector, const CTakeDamageInfo &>                    ft_CalcDamageForceVector;
	
	static MemberVFuncThunk<const CBaseCombatCharacter *, CBaseCombatWeapon *, int>       vt_Weapon_GetSlot;
	static MemberVFuncThunk<      CBaseCombatCharacter *, bool, CBaseCombatWeapon *>      vt_Weapon_CanSwitchTo;
	static MemberVFuncThunk<      CBaseCombatCharacter *, bool, CBaseCombatWeapon *, int> vt_Weapon_Switch;
	static MemberVFuncThunk<      CBaseCombatCharacter *, void, CBaseCombatWeapon *>      vt_Weapon_Equip;
	static MemberVFuncThunk<const CBaseCombatCharacter *, CNavArea *>                     vt_GetLastKnownArea;
	static MemberVFuncThunk<      CBaseCombatCharacter *, void>                           vt_UpdateLastKnownArea;
	static MemberVFuncThunk<      CBaseCombatCharacter *, int, int, int, bool>            vt_GiveAmmo;
	static MemberVFuncThunk<const CBaseCombatCharacter *, int, int>                       vt_GetAmmoCount;
	static MemberVFuncThunk<      CBaseCombatCharacter *, bool, const CTakeDamageInfo&>   vt_ShouldGib;
	static MemberVFuncThunk<      CBaseCombatCharacter *, bool, const Vector&>            vt_FInViewCone;
	static MemberVFuncThunk<      CBaseCombatCharacter *, bool, const CTakeDamageInfo&, const Vector&> vt_BecomeRagdoll;
#ifdef SE_TF2
	static MemberVFuncThunk<      CBaseCombatCharacter *, int>                            vt_GetBossType;
#endif
	static MemberVFuncThunk<      CBaseCombatCharacter *, void>                           vt_ClearLastKnownArea;
	static MemberVFuncThunk<      CBaseCombatCharacter *, void, int, int>                 vt_RemoveAmmo;
};

class CAI_BaseNPC : public CBaseCombatCharacter 
{
public:
	static const char *GetActivityName(int id) { return ft_GetActivityName(id); }
	static int GetActivityID(const char *name) { return ft_GetActivityID(name); }
private:
	static StaticFuncThunk<const char *, int> ft_GetActivityName;
	static StaticFuncThunk<int, const char *> ft_GetActivityID;
};

#ifdef SE_TF2
class CMultiPlayerAnimState
{
public:
	void OnNewModel() { ft_OnNewModel(this); }
	uintptr_t vtable;
	bool	m_bForceAimYaw;
	CUtlVector<int> m_aGestureSlots;
	CBasePlayer *m_pPlayer;
	
private:
	static MemberFuncThunk<CMultiPlayerAnimState *, void> ft_OnNewModel;
};
#endif

class CPlayerLocalData
{
public:
	DECL_SENDPROP(bool,  m_bDucked);
	DECL_SENDPROP(bool,  m_bDucking);
	DECL_SENDPROP(bool,  m_bInDuckJump);
	DECL_SENDPROP(float, m_flDucktime);
	DECL_SENDPROP(float, m_flStepSize);
	DECL_SENDPROP(Vector, m_vecPunchAngle);
	DECL_SENDPROP(Vector, m_vecPunchAngleVel);
	DECL_SENDPROP(bool,  m_bDrawViewmodel);
	DECL_SENDPROP(int,   m_iHideHUD);
	DECL_SENDPROP(float,   m_flFallVelocity);
	
	inline void NetworkStateChanged()           { }
	inline void NetworkStateChanged(void *pVar) { }

};

enum PlayerPhysFlag_e : uint32_t
{
	PFLAG_DIROVERRIDE               = (1 << 0),
	PFLAG_DUCKING                   = (1 << 1),
	PFLAG_USING                     = (1 << 2),
	PFLAG_OBSERVER                  = (1 << 3),
	PFLAG_VPHYSICS_MOTIONCONTROLLER = (1 << 4),
	PFLAG_GAMEPHYSICS_ROTPUSH       = (1 << 5),
};


class CBasePlayer : public CBaseCombatCharacter
{
public:
	float GetDeathTime() const              { return this->m_flDeathTime; }
	void SetDeathTime(float time)           {        this->m_flDeathTime = time; }
	int GetObserverMode() const             { return this->m_iObserverMode; }
	const char *GetPlayerName()             { return this->m_szNetname; }
	float MaxSpeed() const                  { return this->m_flMaxspeed; }
	int GetUserID()                         { return engine->GetPlayerUserId(this->edict()); }
#ifdef SE_TF2
	int GetNumWearables() const             { return this->m_hMyWearables->Count(); }
	CEconWearable *GetWearable(int i) const { return this->m_hMyWearables[i]; }
#endif
	
	/* easy-but-slow calls via IPlayerInfo */
	const char *GetNetworkIDString() const { return this->GetPlayerInfo()->GetNetworkIDString(); }
	bool IsConnected() const               { return this->GetPlayerInfo()->IsConnected(); }
	bool IsHLTV() const                    { return this->pl->hltv; }
	bool IsReplay() const                  { return this->pl->replay; }
	bool IsDead() const                    { return this->GetPlayerInfo()->IsDead(); }
	bool IsObserver() const                { return this->GetPlayerInfo()->IsObserver(); }
	const Vector GetPlayerMins() const     { return this->GetPlayerInfo()->GetPlayerMins(); }
	const Vector GetPlayerMaxs() const     { return this->GetPlayerInfo()->GetPlayerMaxs(); }
	const char *GetWeaponName() const      { return this->GetPlayerInfo()->GetWeaponName(); }
	
	void EyeVectors(Vector *pForward, Vector *pRight = nullptr, Vector *pUp = nullptr) { return ft_EyeVectors    (this, pForward, pRight, pUp); }
	bool GetSteamID(CSteamID *pID)                                                     { return ft_GetSteamID    (this, pID); }
	CSteamID GetSteamID()                                                              { CSteamID id; this->GetSteamID(&id); return id; }
	void SetPlayerName(const char *name)                                               {        ft_SetPlayerName (this, name); }
	CBaseViewModel *GetViewModel(int viewmodelindex = 0, bool bObserverOK = true)      { return ft_GetViewModel  (this, viewmodelindex, bObserverOK); }
	void DisableButtons(int nButtons)                                                  {        ft_DisableButtons(this, nButtons); }
	void EnableButtons(int nButtons)                                                   {        ft_EnableButtons (this, nButtons); }
	void ForceButtons(int nButtons)                                                    {        ft_ForceButtons  (this, nButtons); }
	void UnforceButtons(int nButtons)                                                  {        ft_UnforceButtons(this, nButtons); }
	void SnapEyeAngles(const QAngle& viewAngles)                                       {        ft_SnapEyeAngles (this, viewAngles); }
	
#ifdef SE_TF2
	void EquipWearable(CEconWearable *wearable)									{ vt_EquipWearable     (this, wearable); }
	void RemoveWearable(CEconWearable *wearable)                         {        vt_RemoveWearable      (this, wearable); }
#endif
	
	bool IsBot() const                                                   { return vt_IsBot               (this); }
	bool IsBotOfType(int type) const                                     { return vt_IsBotOfType         (this, type); }
	bool IsFakeClient() const                                            { return vt_IsFakeClient        (this); }
	void CommitSuicide(bool bExplode = false, bool bForce = false)       {        vt_CommitSuicide       (this, bExplode, bForce); }
	void ForceRespawn()                                                  {        vt_ForceRespawn        (this); }
	Vector Weapon_ShootPosition()                                        { return vt_Weapon_ShootPosition(this); }
	float GetPlayerMaxSpeed()                                            { return vt_GetPlayerMaxSpeed   (this); }
#ifdef SE_TF2
	void ChangeTeam(int iTeamNum, bool bAutoTeam, bool bSilent, bool b3) {        vt_ChangeTeam_bool3    (this, iTeamNum, bAutoTeam, bSilent, b3); }
	void ChangeTeamBase(int iTeamNum, bool bAutoTeam, bool bSilent, bool b3) {    ft_ChangeTeam_base    (this, iTeamNum, bAutoTeam, bSilent, b3); }
#endif
	CBaseEntity *FindUseEntity()                                         { return vt_FindUseEntity      (this); }
	void LeaveVehicle(const Vector &pos = vec3_origin, const QAngle &ang = vec3_angle) { return vt_LeaveVehicle       (this, pos, ang); }
	void CreateViewModel(int index)                                      { return vt_CreateViewModel(this, index); }
	
	bool IsRealPlayer() const { return !IsFakeClient() && !IsHLTV() && !IsReplay(); }
	
	
	DECL_SENDPROP_RW(CPlayerLocalData, m_Local);
	DECL_SENDPROP(int, m_nTickBase);
	DECL_DATAMAP (bool,      m_bAllowInstantSpawn);
	DECL_DATAMAP (CHandle<CBaseEntity>, m_hViewEntity);
	DECL_DATAMAP (CHandle<CBaseEntity>, m_hVehicle);
	DECL_DATAMAP(int,          m_nButtons);
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hObserverTarget);
	DECL_SENDPROP_RW(CPlayerState, pl);
	DECL_DATAMAP(float,         m_flStepSoundTime);
	DECL_RELATIVE(CUserCmd *,   m_pCurrentCommand);
	DECL_DATAMAP(float,          m_fLerpTime);
	
private:
	IPlayerInfo *GetPlayerInfo() const { return playerinfomanager->GetPlayerInfo(this->edict()); }
	
	DECL_SENDPROP(float,                              m_flDeathTime);
	DECL_SENDPROP(int,                                m_iObserverMode);
	DECL_SENDPROP(float,                              m_flMaxspeed);
#ifdef SE_TF2
	DECL_SENDPROP(CUtlVector<CHandle<CEconWearable>>, m_hMyWearables);
#endif
	
	DECL_DATAMAP(char[32],     m_szNetname);
	DECL_DATAMAP(bool,         m_bDuckToggled);
	DECL_DATAMAP(unsigned int, m_afPhysicsFlags);
	DECL_EXTRACT(int,          m_vphysicsCollisionState);
	
	static MemberFuncThunk<CBasePlayer *, void, Vector *, Vector *, Vector *> ft_EyeVectors;
	static MemberFuncThunk<CBasePlayer *, bool, CSteamID *>                   ft_GetSteamID;
	static MemberFuncThunk<CBasePlayer *, void, const char *>                 ft_SetPlayerName;
	static MemberFuncThunk<CBasePlayer *, CBaseViewModel *, int, bool>        ft_GetViewModel;
	static MemberFuncThunk<CBasePlayer *, void, int>                          ft_DisableButtons;
	static MemberFuncThunk<CBasePlayer *, void, int>                          ft_EnableButtons;
	static MemberFuncThunk<CBasePlayer *, void, int>                          ft_ForceButtons;
	static MemberFuncThunk<CBasePlayer *, void, int>                          ft_UnforceButtons;
	static MemberFuncThunk<CBasePlayer *, void, const QAngle&>                ft_SnapEyeAngles;
	static MemberFuncThunk<      CBasePlayer *, void, int, bool, bool, bool>  ft_ChangeTeam_base;
	
	static MemberVFuncThunk<const CBasePlayer *, bool>                        vt_IsFakeClient;
	static MemberVFuncThunk<const CBasePlayer *, bool>                        vt_IsBot;
	static MemberVFuncThunk<const CBasePlayer *, bool, int>                   vt_IsBotOfType;
	static MemberVFuncThunk<      CBasePlayer *, void, bool, bool>            vt_CommitSuicide;
	static MemberVFuncThunk<      CBasePlayer *, void>                        vt_ForceRespawn;
	static MemberVFuncThunk<      CBasePlayer *, Vector>                      vt_Weapon_ShootPosition;
	static MemberVFuncThunk<      CBasePlayer *, float>                       vt_GetPlayerMaxSpeed;
	static MemberVFuncThunk<      CBasePlayer *, void, CEconWearable *>       vt_RemoveWearable;
	static MemberVFuncThunk<      CBasePlayer *, void, int, bool, bool, bool> vt_ChangeTeam_bool3;
	static MemberVFuncThunk<      CBasePlayer *, void, CEconWearable *>       vt_EquipWearable;
	static MemberVFuncThunk<      CBasePlayer *, CBaseEntity *>               vt_FindUseEntity;
	static MemberVFuncThunk<      CBasePlayer *, void, const Vector &,const QAngle &> vt_LeaveVehicle;
	static MemberVFuncThunk<      CBasePlayer *, void, int>                   vt_CreateViewModel;
};

class CBaseMultiplayerPlayer : public CBasePlayer
{
public:
	bool SpeakConceptIfAllowed(int iConcept, const char *modifiers = nullptr, char *pszOutResponseChosen = nullptr, size_t bufsize = 0, IRecipientFilter *filter = nullptr)
	{
		return vt_SpeakConceptIfAllowed(this, iConcept, modifiers, pszOutResponseChosen, bufsize, filter);
	}
	
private:
	static MemberVFuncThunk<CBaseMultiplayerPlayer *, bool, int, const char *, char *, size_t, IRecipientFilter *> vt_SpeakConceptIfAllowed;
};


inline CBaseCombatCharacter *ToBaseCombatCharacter(CBaseEntity *pEntity)
{
	if (pEntity == nullptr) return nullptr;
	
	return pEntity->MyCombatCharacterPointer();
}

inline CBasePlayer *ToBasePlayer(CBaseEntity *pEntity)
{
	if (pEntity == nullptr)   return nullptr;
	if (!pEntity->IsPlayer()) return nullptr;
	
	return static_cast<CBasePlayer *>(pEntity);
}

inline CBaseMultiplayerPlayer *ToBaseMultiplayerPlayer(CBaseEntity *pEntity)
{
	if (pEntity == nullptr)   return nullptr;
	if (!pEntity->IsPlayer()) return nullptr;
	
	return rtti_cast<CBaseMultiplayerPlayer *>(pEntity);
}


inline CBasePlayer *UTIL_PlayerByIndex(int playerIndex)
{
	CBasePlayer *pPlayer = nullptr;
	
	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients) {
		edict_t *pPlayerEdict = INDEXENT(playerIndex);
		if (pPlayerEdict != nullptr && !pPlayerEdict->IsFree()) {
			pPlayer = reinterpret_cast<CBasePlayer *>(GetContainingEntity(pPlayerEdict));
		}
	}
	
	return pPlayer;
}

inline CBasePlayer *UTIL_PlayerByUserId(int userID)
	{
		CBasePlayer *match = nullptr;
		
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			edict_t *edict = INDEXENT(i);
			if (edict == nullptr) {
			//	Warning("UTIL_PlayerByUserId(%d): #%d: INDEXENT returned nullptr\n", userID, i);
				continue;
			}
			
			int id = engine->GetPlayerUserId(edict);
			if (id == userID) {
			//	Warning("UTIL_PlayerByUserId(%d): #%d: user ID matches!\n", userID, i);
				match = reinterpret_cast<CBasePlayer *>(GetContainingEntity(edict));
			} else {
			//	Warning("UTIL_PlayerByUserId(%d): #%d: non-matching ID: %d\n", userID, i, id);
			}
		}
		
		return match;
	}
template<typename T>
int CollectPlayers(CUtlVector<T *> *playerVector, int team = TEAM_ANY, bool isAlive = false, bool shouldAppend = false);

CBasePlayer *UTIL_PlayerBySteamID(const CSteamID &steamid);

#endif
