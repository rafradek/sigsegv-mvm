#ifndef _INCLUDE_SIGSEGV_STUB_ENTITIES_H_
#define _INCLUDE_SIGSEGV_STUB_ENTITIES_H_

#include "link/link.h"
#include "prop.h"
#include "stub/baseplayer.h"
#include "stub/projectiles.h"

class CPointEntity : public CBaseEntity {};


class CAmbientGeneric : public CPointEntity {};


class CPathTrack : public CPointEntity
{
public:
	CPathTrack *GetNext() { return ft_GetNext(this); }
	
	DECL_DATAMAP(int, m_eOrientationType);
	
private:
	static MemberFuncThunk<CPathTrack *, CPathTrack *> ft_GetNext;
};

class CEnvEntityMaker : public CPointEntity
{
public:
	DECL_DATAMAP(string_t,               m_iszTemplate);
	DECL_DATAMAP(QAngle,                 m_angPostSpawnDirection);
	DECL_DATAMAP(float,                  m_flPostSpawnDirectionVariance);
	DECL_DATAMAP(float,                  m_flPostSpawnSpeed);
	DECL_DATAMAP(bool,                   m_bPostSpawnUseAngles);
};

class CItem : public CBaseAnimating
{
#ifdef SE_TF2
	DECL_DATAMAP(float,                m_flNextResetCheckTime);
#endif
	DECL_DATAMAP(bool,                 m_bActivateWhenAtRest);
	DECL_DATAMAP(Vector,               m_vOriginalSpawnOrigin);
	DECL_DATAMAP(QAngle,               m_vOriginalSpawnAngles);
	DECL_DATAMAP(IPhysicsConstraint *, m_pConstraint);
};

class CBaseProp : public CBaseAnimating {};
class CBreakableProp : public CBaseProp {};
class CDynamicProp : public CBreakableProp {};

class IBody;

class CBaseToggle : public CBaseEntity {};

class CBaseTrigger : public CBaseToggle
{
public:
	bool PassesTriggerFilters(CBaseEntity *entity) { return vt_PassesTriggerFilters(this, entity); }

	DECL_DATAMAP(bool, m_bDisabled);

private:
	static MemberVFuncThunk<CBaseTrigger *, bool, CBaseEntity *> vt_PassesTriggerFilters;
};

class CFuncNavPrerequisite : public CBaseTrigger
{
public:
	enum TaskType : int32_t
	{
		DESTROY_ENTITY = 1,
		MOVE_TO        = 2,
		WAIT           = 3,
	};
	
	DECL_DATAMAP(int,      m_task);
	DECL_DATAMAP(string_t, m_taskEntityName);
	DECL_DATAMAP(float,    m_taskValue);
	DECL_DATAMAP(bool,     m_isDisabled);
	// CHandle<T> @ m_isDisabled+4
};


class CServerOnlyEntity : public CBaseEntity {};
class CLogicalEntity : public CServerOnlyEntity {};

class CLogicCase : public CLogicalEntity
{
public:

	void FireCase(int useCase, CBaseEntity *activator) {
		CBaseEntityOutput *output = nullptr;
		switch (useCase) {
			case 1: output = &m_OnCase01; break;
			case 2: output = &m_OnCase02; break;
			case 3: output = &m_OnCase03; break;
			case 4: output = &m_OnCase04; break;
			case 5: output = &m_OnCase05; break;
			case 6: output = &m_OnCase06; break;
			case 7: output = &m_OnCase07; break;
			case 8: output = &m_OnCase08; break;
			case 9: output = &m_OnCase09; break;
			case 10: output = &m_OnCase10; break;
			case 11: output = &m_OnCase11; break;
			case 12: output = &m_OnCase12; break;
			case 13: output = &m_OnCase13; break;
			case 14: output = &m_OnCase14; break;
			case 15: output = &m_OnCase15; break;
			case 16: output = &m_OnCase16; break;
		}
		if (output != nullptr) {
       		variant_t variant1;
			output->FireOutput(variant1, activator, this);
		}
	}
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase01);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase02);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase03);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase04);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase05);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase06);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase07);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase08);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase09);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase10);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase11);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase12);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase13);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase14);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase15);
	DECL_DATAMAP(CBaseEntityOutput, m_OnCase16);

	DECL_DATAMAP(string_t[16], m_nCase);

	DECL_DATAMAP(CBaseEntityOutput, m_OnDefault);

};

class CBaseFilter : public CLogicalEntity
{
public:
	DECL_DATAMAP(CBaseEntityOutput,      m_OnPass);
	DECL_DATAMAP(CBaseEntityOutput,      m_OnFail);

	bool PassesFilter(CBaseEntity *caller, CBaseEntity *entity) { return ft_PassesFilter(this, caller, entity); }

private:
	static MemberFuncThunk<CBaseFilter *, bool, CBaseEntity *, CBaseEntity *> ft_PassesFilter;
};

class CFilterMultiple : public CBaseFilter
{
public:
	// TODO
};

class CBaseParticleEntity : public CBaseEntity {};

class CSmokeStack : public CBaseParticleEntity
{
public:
	DECL_DATAMAP (string_t, m_strMaterialModel);
	DECL_SENDPROP(int,      m_iMaterialModel);
};

class CGameUI : public CBaseEntity
{
public:
	DECL_DATAMAP (CHandle<CBasePlayer>, m_player);
};

class CTriggerCamera : public CBaseEntity
{
public:
	void Enable() { ft_Enable(this); }
	void Disable() { ft_Disable(this); }

	DECL_DATAMAP (CHandle<CBaseEntity>, m_hPlayer);
	DECL_DATAMAP (CHandle<CBaseEntity>, m_hTarget);
private:
	static MemberFuncThunk<CTriggerCamera *, void> ft_Enable;
	static MemberFuncThunk<CTriggerCamera *, void> ft_Disable;
};

class CFuncRotating : public CBaseEntity
{
public:
	void SetTargetSpeed(float speed) { ft_SetTargetSpeed(this, speed); }

	DECL_DATAMAP (bool, m_bReversed);
	DECL_DATAMAP (float, m_flMaxSpeed);
	DECL_DATAMAP (bool, m_bStopAtStartPos);
	DECL_DATAMAP (QAngle, m_vecMoveAng);
	DECL_DATAMAP (float, m_flTargetSpeed);

private:
	static MemberFuncThunk<CFuncRotating *, void, float> ft_SetTargetSpeed;
};

class CBaseServerVehicle
{
public:
	void HandleEntryExitFinish(bool bExitAnimOn, bool bResetAnim) { return vt_HandleEntryExitFinish(this, bExitAnimOn, bResetAnim); }
	void SetupMove(CBasePlayer *player, CUserCmd *ucmd, void *pHelper, void *move) { return vt_SetupMove(this, player, ucmd, pHelper, move); }
	bool HandlePassengerExit(CBaseCombatCharacter *pPassenger)  { return vt_HandlePassengerExit(this, pPassenger); }
	void HandlePassengerEntry(CBaseCombatCharacter *pPassenger, bool allowAnyPosition)  { vt_HandlePassengerEntry(this, pPassenger, allowAnyPosition); }
	CBaseEntity *GetDriver()  { return vt_GetDriver(this); }
	CBaseEntity *GetVehicleEnt()  { return vt_GetVehicleEnt(this); }

private:
	static MemberVFuncThunk<CBaseServerVehicle *, void, bool, bool> vt_HandleEntryExitFinish;
	static MemberVFuncThunk<CBaseServerVehicle *, void, CBasePlayer *, CUserCmd *,  void *, void *> vt_SetupMove;
	static MemberVFuncThunk<CBaseServerVehicle *, bool, CBaseCombatCharacter *> vt_HandlePassengerExit;
	static MemberVFuncThunk<CBaseServerVehicle *, void, CBaseCombatCharacter *, bool> vt_HandlePassengerEntry;
	static MemberVFuncThunk<CBaseServerVehicle *, CBaseEntity *> vt_GetDriver;
	static MemberVFuncThunk<CBaseServerVehicle *, CBaseEntity *> vt_GetVehicleEnt;
};

class CPropVehicle : public CBaseAnimating
{
public:
	DECL_DATAMAP (unsigned int, m_nVehicleType);
	DECL_DATAMAP (string_t, m_vehicleScript);
	DECL_DATAMAP (CHandle<CBasePlayer>, m_hPhysicsAttacker);
	DECL_DATAMAP (float, m_flLastPhysicsInfluenceTime);
	
};

class CPropVehicleDriveable : public CPropVehicle
{
public:
	DECL_DATAMAP (bool, m_bEnterAnimOn);
	DECL_DATAMAP (bool, m_bExitAnimOn);
	DECL_DATAMAP (float, m_flMinimumSpeedToEnterExit);
	DECL_DATAMAP (CBaseServerVehicle *, m_pServerVehicle);
	DECL_DATAMAP (CHandle<CBasePlayer>, m_hPlayer);
	DECL_DATAMAP (float, m_nSpeed);
	DECL_DATAMAP (float, m_bLocked);

	
};

class CMathCounter : public CLogicalEntity
{

};

class CSprite : public CBaseEntity
{
	
};

class CSpriteTrail : public CSprite
{
public:
	DECL_SENDPROP(float, m_flLifeTime);
};

extern GlobalThunk<CBaseEntity *> g_WorldEntity;

inline CBaseEntity* GetWorldEntity() {
	return g_WorldEntity;
}

class CParticleSystem : public CBaseEntity
{
public:
	DECL_DATAMAP (bool, m_bStartActive);
	DECL_DATAMAP (string_t, m_iszEffectName);
	DECL_SENDPROP(CHandle<CBaseEntity>[63], m_hControlPointEnts);
};


// 20151007a

// CTFPlayer::Event_Killed
// - CTFReviveMarker::Create
//   - CBaseEntity::Create
//     - ctor
//       - m_nRevives = 0
//     - CTFReviveMarker::Spawn
//       - m_iHealth = 1
//       - set model, solid, collision, move, effects, sequence, think, etc
//   - SetOwner
//     - m_hOwner = player
//     - change team to match player
//     - set bodygroup based on player class
//     - set angles to match player
//   - change team to match player
// - player->handle_0x2974 = marker

// CTFReviveMarker::ReviveThink (100 ms interval)
// - if m_hOwner == null: UTIL_Remove(this)
// - if m_hOwner not in same team: UTIL_Remove(this)
// - if m_iMaxHealth == 0:
//   - maxhealth = (float)(player->GetMaxHealth() / 2)
//     - TODO: does CTFPlayer::GetMaxHealth factor in items and stuff?
//   - if (stats = CTF_GameStats.FindPlayerStats(player)) != null:
//     - m_nRevives = (probably round stat TFSTAT_DEATHS)
//     - maxhealth += 10 * old value of m_nRevives
//   - m_iMaxHealth = (int)maxhealth
// - if on the ground and haven't been yet
//   - set the bool for that (+0x4a0)
//   - if landed in a trigger_hurt: UTIL_Remove(this)
// - if m_nRevives > 7:   DispatchParticleEffect("speech_revivecall_hard")
// - elif m_nRevives > 3: DispatchParticleEffect("speech_revivecall_medium")
// - else:                DispatchParticleEffect("speech_revivecall")
// - TODO: stuff related to "revive_player_stopped"

// CTFReviveMarker::AddMarkerHealth
// - 


#endif
