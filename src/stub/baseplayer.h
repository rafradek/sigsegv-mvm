#ifndef _INCLUDE_SIGSEGV_STUB_BASEPLAYER_H_
#define _INCLUDE_SIGSEGV_STUB_BASEPLAYER_H_


#include "stub/baseanimating.h"
#include "stub/server.h"

class CNavArea;
class CBaseCombatWeapon;
class CEconWearable;
class CBaseViewModel;

class CBaseCombatCharacter : public CBaseFlex
{
public:
	enum FieldOfViewCheckType { USE_FOV, DISREGARD_FOV };
	
	CBaseCombatWeapon *GetActiveWeapon() const { return this->m_hActiveWeapon; }
	int WeaponCount() const                    { return MAX_WEAPONS; }
	CBaseCombatWeapon *GetWeapon(int i) const  { return this->m_hMyWeapons[i]; }
	static_assert(MAX_WEAPONS == 48);
	
	void AddGlowEffect()                                                        {        ft_AddGlowEffect     (this); }
	void RemoveGlowEffect()                                                     {        ft_RemoveGlowEffect  (this); }
	bool IsGlowEffectActive()                                                   { return ft_IsGlowEffectActive(this); }
	bool IsAbleToSee(const CBaseEntity *entity, FieldOfViewCheckType checkFOV)  { return ft_IsAbleToSee_ent   (this, entity, checkFOV); }
	bool IsAbleToSee(CBaseCombatCharacter *pBCC, FieldOfViewCheckType checkFOV) { return ft_IsAbleToSee_bcc   (this, pBCC, checkFOV); }
	void SetBloodColor(int nBloodColor)                                         {        ft_SetBloodColor     (this, nBloodColor); }
	bool Weapon_Detach(CBaseCombatWeapon *pWeapon)                              { return ft_Weapon_Detach     (this, pWeapon); }
	bool SwitchToNextBestWeapon(CBaseCombatWeapon *weapon)                      { return ft_SwitchToNextBestWeapon(this, weapon); }
	void SetAmmoCount(int count, int ammoIndex)                                 {        ft_SetAmmoCount      (this, count, ammoIndex); }
	
	CBaseCombatWeapon *Weapon_GetSlot(int slot) const                      { return vt_Weapon_GetSlot     (this, slot); }
	bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon)                    { return vt_Weapon_CanSwitchTo (this, pWeapon); }
	bool Weapon_Switch(CBaseCombatWeapon *pWeapon, int viewmodelindex = 0) { return vt_Weapon_Switch      (this, pWeapon, viewmodelindex); }
	CNavArea *GetLastKnownArea() const                                     { return vt_GetLastKnownArea   (this); }
	void UpdateLastKnownArea()                                             {        vt_UpdateLastKnownArea(this); }
	int GiveAmmo(int iCount, int iAmmoIndex, bool bSuppressSound = false)  { return vt_GiveAmmo           (this, iCount, iAmmoIndex, bSuppressSound); }
	int GetAmmoCount(int iAmmoIndex) const                                 { return vt_GetAmmoCount       (this, iAmmoIndex); }
	bool ShouldGib(const CTakeDamageInfo& info)                            { return vt_ShouldGib          (this, info); }
	int GetBossType()                                                      { return vt_GetBossType        (this); }

private:
	DECL_SENDPROP(CHandle<CBaseCombatWeapon>,              m_hActiveWeapon);
	DECL_SENDPROP(CHandle<CBaseCombatWeapon>[MAX_WEAPONS], m_hMyWeapons);
	
	static MemberFuncThunk<CBaseCombatCharacter *, void>                                               ft_AddGlowEffect;
	static MemberFuncThunk<CBaseCombatCharacter *, void>                                               ft_RemoveGlowEffect;
	static MemberFuncThunk<CBaseCombatCharacter *, bool>                                               ft_IsGlowEffectActive;
	static MemberFuncThunk<CBaseCombatCharacter *, bool, const CBaseEntity *, FieldOfViewCheckType>    ft_IsAbleToSee_ent;
	static MemberFuncThunk<CBaseCombatCharacter *, bool, CBaseCombatCharacter *, FieldOfViewCheckType> ft_IsAbleToSee_bcc;
	static MemberFuncThunk<CBaseCombatCharacter *, void, int>                                          ft_SetBloodColor;
	static MemberFuncThunk<CBaseCombatCharacter *, bool, CBaseCombatWeapon *>                          ft_Weapon_Detach;
	static MemberFuncThunk<CBaseCombatCharacter *, bool, CBaseCombatWeapon *>                          ft_SwitchToNextBestWeapon;
	static MemberFuncThunk<CBaseCombatCharacter *, void, int, int>                                     ft_SetAmmoCount;
	
	static MemberVFuncThunk<const CBaseCombatCharacter *, CBaseCombatWeapon *, int>       vt_Weapon_GetSlot;
	static MemberVFuncThunk<      CBaseCombatCharacter *, bool, CBaseCombatWeapon *>      vt_Weapon_CanSwitchTo;
	static MemberVFuncThunk<      CBaseCombatCharacter *, bool, CBaseCombatWeapon *, int> vt_Weapon_Switch;
	static MemberVFuncThunk<const CBaseCombatCharacter *, CNavArea *>                     vt_GetLastKnownArea;
	static MemberVFuncThunk<      CBaseCombatCharacter *, void>                           vt_UpdateLastKnownArea;
	static MemberVFuncThunk<      CBaseCombatCharacter *, int, int, int, bool>            vt_GiveAmmo;
	static MemberVFuncThunk<const CBaseCombatCharacter *, int, int>                       vt_GetAmmoCount;
	static MemberVFuncThunk<      CBaseCombatCharacter *, bool, const CTakeDamageInfo&>   vt_ShouldGib;
	static MemberVFuncThunk<      CBaseCombatCharacter *, int>                            vt_GetBossType;
};


class CPlayerLocalData
{
public:
	DECL_SENDPROP(bool,  m_bDucked);
	DECL_SENDPROP(bool,  m_bDucking);
	DECL_SENDPROP(bool,  m_bInDuckJump);
	DECL_SENDPROP(float, m_flDucktime);
	DECL_SENDPROP_RW(float, m_flStepSize);
	DECL_SENDPROP(Vector, m_vecPunchAngle);
	DECL_SENDPROP(Vector, m_vecPunchAngleVel);
	
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
	int GetNumWearables() const             { return this->m_hMyWearables->Count(); }
	CEconWearable *GetWearable(int i) const { return this->m_hMyWearables[i]; }
	
	/* easy-but-slow calls via IPlayerInfo */
	const char *GetNetworkIDString() const { return this->GetPlayerInfo()->GetNetworkIDString(); }
	bool IsConnected() const               { return this->GetPlayerInfo()->IsConnected(); }
	bool IsHLTV() const                    { return this->GetPlayerInfo()->IsHLTV(); }
	bool IsReplay() const                  { return this->GetPlayerInfo()->IsReplay(); }
	bool IsDead() const                    { return this->GetPlayerInfo()->IsDead(); }
	bool IsObserver() const                { return this->GetPlayerInfo()->IsObserver(); }
	const Vector GetPlayerMins() const     { return this->GetPlayerInfo()->GetPlayerMins(); }
	const Vector GetPlayerMaxs() const     { return this->GetPlayerInfo()->GetPlayerMaxs(); }
	const char *GetWeaponName() const      { return this->GetPlayerInfo()->GetWeaponName(); }
	
	void EyeVectors(Vector *pForward, Vector *pRight = nullptr, Vector *pUp = nullptr) { return ft_EyeVectors    (this, pForward, pRight, pUp); }
	bool GetSteamID(CSteamID *pID)                                                     { return ft_GetSteamID    (this, pID); }
	void SetPlayerName(const char *name)                                               {        ft_SetPlayerName (this, name); }
	CBaseViewModel *GetViewModel(int viewmodelindex = 0, bool bObserverOK = true)      { return ft_GetViewModel  (this, viewmodelindex, bObserverOK); }
	void DisableButtons(int nButtons)                                                  {        ft_DisableButtons(this, nButtons); }
	void EnableButtons(int nButtons)                                                   {        ft_EnableButtons (this, nButtons); }
	void ForceButtons(int nButtons)                                                    {        ft_ForceButtons  (this, nButtons); }
	void UnforceButtons(int nButtons)                                                  {        ft_UnforceButtons(this, nButtons); }
	void SnapEyeAngles(const QAngle& viewAngles)                                       {        ft_SnapEyeAngles (this, viewAngles); }
	void Weapon_Equip(CBaseCombatWeapon *pWeapon)                               { vt_Weapon_Equip     	      (this, pWeapon); }
	void EquipWearable(CEconWearable *wearable)									{ vt_EquipWearable     (this, wearable); }
	
	bool IsBot() const                                                   { return vt_IsBot               (this); }
	bool IsBotOfType(int type) const                                     { return vt_IsBotOfType         (this, type); }
	bool IsFakeClient() const                                            { return vt_IsFakeClient        (this); }
	void CommitSuicide(bool bExplode = false, bool bForce = false)       {        vt_CommitSuicide       (this, bExplode, bForce); }
	void ForceRespawn()                                                  {        vt_ForceRespawn        (this); }
	Vector Weapon_ShootPosition()                                        { return vt_Weapon_ShootPosition(this); }
	float GetPlayerMaxSpeed()                                            { return vt_GetPlayerMaxSpeed   (this); }
	void RemoveWearable(CEconWearable *wearable)                         {        vt_RemoveWearable      (this, wearable); }
	void ChangeTeam(int iTeamNum, bool bAutoTeam, bool bSilent, bool b3) {        vt_ChangeTeam_bool3    (this, iTeamNum, bAutoTeam, bSilent, b3); }
	void ChangeTeamBase(int iTeamNum, bool bAutoTeam, bool bSilent, bool b3) {    ft_ChangeTeam_base    (this, iTeamNum, bAutoTeam, bSilent, b3); }
	
	
	DECL_SENDPROP_RW(CPlayerLocalData, m_Local);
	DECL_SENDPROP(int, m_nTickBase);
	DECL_DATAMAP (bool,      m_bAllowInstantSpawn);
	DECL_DATAMAP (CHandle<CBaseEntity>, m_hViewEntity);
	DECL_DATAMAP (CHandle<CBaseEntity>, m_hVehicle);
	
	
private:
	IPlayerInfo *GetPlayerInfo() const { return playerinfomanager->GetPlayerInfo(this->edict()); }
	
	DECL_SENDPROP(float,                              m_flDeathTime);
	DECL_SENDPROP(int,                                m_iObserverMode);
	DECL_SENDPROP(float,                              m_flMaxspeed);
	DECL_SENDPROP(CUtlVector<CHandle<CEconWearable>>, m_hMyWearables);
	
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
	static MemberVFuncThunk<      CBasePlayer *, void, CBaseCombatWeapon *>   vt_Weapon_Equip;
	static MemberVFuncThunk<      CBasePlayer *, void, CEconWearable *>       vt_EquipWearable;
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

inline CBasePlayer *UTIL_PlayerBySteamID(const CSteamID &steamid) {
	if (steamid.ConvertToUint64() == 0LL)
		return nullptr;

	int clients = sv->GetClientCount();
	for (int i = 0; i < clients; i++) {
		auto client = sv->GetClient(i);
		if (client != nullptr) {
			if (static_cast<CBaseClient *>(client)->m_SteamID == steamid) {
				return UTIL_PlayerByIndex(i+1);
			}
		}
	}
	return nullptr;
}

#endif
