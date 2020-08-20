#ifndef _INCLUDE_SIGSEGV_STUB_TFBOT_H_
#define _INCLUDE_SIGSEGV_STUB_TFBOT_H_

#define ADD_EXTATTR

#include "prop.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/econ.h"
#include "util/misc.h"
#include "re/nextbot.h"
#include "util/rtti.h"


class ILocomotion;
class IBody;
class IVision;
class IIntention;


template<typename T>
class NextBotPlayer : public T
{
public:
	void PressFireButton(float duration = -1.0f)        { GetVFT_PressFireButton         ()(this, duration);      }
	void ReleaseFireButton()                            { GetVFT_ReleaseFireButton       ()(this);                }
	void PressAltFireButton(float duration = -1.0f)     { GetVFT_PressAltFireButton      ()(this, duration);      }
	void ReleaseAltFireButton()                         { GetVFT_ReleaseAltFireButton    ()(this);                }
	void PressMeleeButton(float duration = -1.0f)       { GetVFT_PressMeleeButton        ()(this, duration);      }
	void ReleaseMeleeButton()                           { GetVFT_ReleaseMeleeButton      ()(this);                }
	void PressSpecialFireButton(float duration = -1.0f) { GetVFT_PressSpecialFireButton  ()(this, duration);      }
	void ReleaseSpecialFireButton()                     { GetVFT_ReleaseSpecialFireButton()(this);                }
	void PressUseButton(float duration = -1.0f)         { GetVFT_PressUseButton          ()(this, duration);      }
	void ReleaseUseButton()                             { GetVFT_ReleaseUseButton        ()(this);                }
	void PressReloadButton(float duration = -1.0f)      { GetVFT_PressReloadButton       ()(this, duration);      }
	void ReleaseReloadButton()                          { GetVFT_ReleaseReloadButton     ()(this);                }
	void PressForwardButton(float duration = -1.0f)     { GetVFT_PressForwardButton      ()(this, duration);      }
	void ReleaseForwardButton()                         { GetVFT_ReleaseForwardButton    ()(this);                }
	void PressBackwardButton(float duration = -1.0f)    { GetVFT_PressBackwardButton     ()(this, duration);      }
	void ReleaseBackwardButton()                        { GetVFT_ReleaseBackwardButton   ()(this);                }
	void PressLeftButton(float duration = -1.0f)        { GetVFT_PressLeftButton         ()(this, duration);      }
	void ReleaseLeftButton()                            { GetVFT_ReleaseLeftButton       ()(this);                }
	void PressRightButton(float duration = -1.0f)       { GetVFT_PressRightButton        ()(this, duration);      }
	void ReleaseRightButton()                           { GetVFT_ReleaseRightButton      ()(this);                }
	void PressJumpButton(float duration = -1.0f)        { GetVFT_PressJumpButton         ()(this, duration);      }
	void ReleaseJumpButton()                            { GetVFT_ReleaseJumpButton       ()(this);                }
	void PressCrouchButton(float duration = -1.0f)      { GetVFT_PressCrouchButton       ()(this, duration);      }
	void ReleaseCrouchButton()                          { GetVFT_ReleaseCrouchButton     ()(this);                }
	void PressWalkButton(float duration = -1.0f)        { GetVFT_PressWalkButton         ()(this, duration);      }
	void ReleaseWalkButton()                            { GetVFT_ReleaseWalkButton       ()(this);                }
	void SetButtonScale(float forward, float side)      { GetVFT_SetButtonScale          ()(this, forward, side); }
	
private:
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressFireButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseFireButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressAltFireButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseAltFireButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressMeleeButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseMeleeButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressSpecialFireButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseSpecialFireButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressUseButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseUseButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressReloadButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseReloadButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressForwardButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseForwardButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressBackwardButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseBackwardButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressLeftButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseLeftButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressRightButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseRightButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressJumpButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseJumpButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressCrouchButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseCrouchButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float       >& GetVFT_PressWalkButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void              >& GetVFT_ReleaseWalkButton();
	static MemberVFuncThunk<NextBotPlayer<T> *, void, float, float>& GetVFT_SetButtonScale();
};

class CTFBot : public NextBotPlayer<CTFPlayer>
{
public:
	enum AttributeType : uint32_t
	{
		ATTR_NONE                        = 0,
		
		ATTR_REMOVE_ON_DEATH             = (1 << 0),
		ATTR_AGGRESSIVE                  = (1 << 1),
		// 2?
		ATTR_SUPPRESS_FIRE               = (1 << 3),
		ATTR_DISABLE_DODGE               = (1 << 4),
		ATTR_BECOME_SPECTATOR_ON_DEATH   = (1 << 5),
		// 6?
		ATTR_RETAIN_BUILDINGS            = (1 << 7),
		ATTR_SPAWN_WITH_FULL_CHARGE      = (1 << 8),
		ATTR_ALWAYS_CRIT                 = (1 << 9),
		ATTR_IGNORE_ENEMIES              = (1 << 10),
		ATTR_HOLD_FIRE_UNTIL_FULL_RELOAD = (1 << 11),
		// 12?
		ATTR_ALWAYS_FIRE_WEAPON          = (1 << 13),
		ATTR_TELEPORT_TO_HINT            = (1 << 14),
		ATTR_MINI_BOSS                   = (1 << 15),
		ATTR_USE_BOSS_HEALTH_BAR         = (1 << 16),
		ATTR_IGNORE_FLAG                 = (1 << 17),
		ATTR_AUTO_JUMP                   = (1 << 18),
		ATTR_AIR_CHARGE_ONLY             = (1 << 19),
		ATTR_VACCINATOR_BULLETS          = (1 << 20),
		ATTR_VACCINATOR_BLAST            = (1 << 21),
		ATTR_VACCINATOR_FIRE             = (1 << 22),
		ATTR_BULLET_IMMUNE               = (1 << 23),
		ATTR_BLAST_IMMUNE                = (1 << 24),
		ATTR_FIRE_IMMUNE                 = (1 << 25),
	};
	
	enum MissionType : int32_t
	{
		MISSION_NONE             = 0,
		// ?
		MISSION_DESTROY_SENTRIES = 2,
		MISSION_SNIPER           = 3,
		MISSION_SPY              = 4,
		MISSION_ENGINEER         = 5,
		MISSION_REPROGRAMMED     = 6,
	};
	
	class SuspectedSpyInfo_t
	{
	public:
		void Suspect()              {        ft_Suspect             (this); } 
		bool IsCurrentlySuspected() { return ft_IsCurrentlySuspected(this); }
		bool TestForRealizing()     { return ft_TestForRealizing    (this); }
		
	private:
		static MemberFuncThunk<SuspectedSpyInfo_t *, void> ft_Suspect;
		static MemberFuncThunk<SuspectedSpyInfo_t *, bool> ft_IsCurrentlySuspected;
		static MemberFuncThunk<SuspectedSpyInfo_t *, bool> ft_TestForRealizing;
	};
	
	struct DelayedNoticeInfo
	{
		CHandle<CBaseEntity> m_hEnt;
		float m_flWhen;
	};
	
	enum DifficultyType : int32_t;
	enum WeaponRestriction : uint32_t;
	
	struct EventChangeAttributes_t
	{
		struct item_attributes_t
		{
			CUtlString m_strItemName;                    // +0x00
			CCopyableUtlVector<static_attrib_t> m_Attrs; // +0x04
		};
		
		CUtlString m_strName;                      // +0x00
		DifficultyType m_iSkill;                   // +0x04
		WeaponRestriction m_nWeaponRestrict;       // +0x08
		MissionType m_nMission;                    // +0x0c
		uint32_t pad_10; // TODO: 0x10
		AttributeType m_nBotAttrs;                 // +0x14
		float m_flVisionRange;                     // +0x18
		CUtlStringList m_ItemNames;                // +0x1c
		CUtlVector<item_attributes_t> m_ItemAttrs; // +0x30
		CUtlVector<static_attrib_t> m_CharAttrs;   // +0x44
		CUtlStringList m_Tags;                     // +0x58
	};
	SIZE_CHECK(EventChangeAttributes_t, 0x6c);
	
#ifdef ADD_EXTATTR
	/* custom */
	class ExtraData
	{
	public:
		ExtraData()
		{
			this->Reset();
		}
		
		void Reset()
		{
			this->m_bAlwaysFireWeaponAlt = false;
			this->m_bTargetStickies      = false;
			this->m_bTauntAfterEveryKill = false;
			this->m_nSplitCurrencyPacks  = 1;
		}
		
		ExtraData& operator=(const ExtraData& that) = default;
		
		bool GetAlwaysFireWeaponAlt() const   { return this->m_bAlwaysFireWeaponAlt; }
		void SetAlwaysFireWeaponAlt(bool val) { this->m_bAlwaysFireWeaponAlt = val; }
		
		bool GetTargetStickies() const   { return this->m_bTargetStickies; }
		void SetTargetStickies(bool val) { this->m_bTargetStickies = val; }
		
		bool GetTauntAfterEveryKill() const   { return this->m_bTauntAfterEveryKill; }
		void SetTauntAfterEveryKill(bool val) { this->m_bTauntAfterEveryKill = val; }
		
		int GetSplitCurrencyPacks() const   { return this->m_nSplitCurrencyPacks; }
		void SetSplitCurrencyPacks(int val) { this->m_nSplitCurrencyPacks = val; }
		
	private:
		bool m_bAlwaysFireWeaponAlt;
		bool m_bTargetStickies;
		bool m_bTauntAfterEveryKill;
		int m_nSplitCurrencyPacks;
	};
	const ExtraData& Ext() const;
#endif
	
#ifdef ADD_EXTATTR

	

	class ExtendedAttr
	{
	public:
		
		enum ExtAttr : uint8_t
		{
			ALWAYS_FIRE_WEAPON_ALT = 0,
			TARGET_STICKIES = 1,
			BUILD_DISPENSER_SG = 2,
			BUILD_DISPENSER_TP = 3
		};

		ExtendedAttr& operator=(const ExtendedAttr&) = default;
		
		void Zero() { this->m_nBits = 0; }
		
		void Set(ExtAttr attr, bool on) { on ? this->TurnOn(attr) : this->TurnOff(attr); }
		void TurnOn(ExtAttr attr)       { this->m_nBits |=  (1U << (int)attr); }
		void TurnOff(ExtAttr attr)      { this->m_nBits &= ~(1U << (int)attr); }
		
		bool operator[](ExtAttr attr)       { return ((this->m_nBits & (1U << (int)attr)) != 0); }
		bool operator[](ExtAttr attr) const { return ((this->m_nBits & (1U << (int)attr)) != 0); }
		
		void Dump() const { /*DevMsg("CTFBot::ExtendedAttr::Dump %08x\n", this->m_nBits);*/ }
		
	private:
		uint32_t m_nBits = 0;
	};
	SIZE_CHECK(ExtendedAttr, 0x4);
#endif
	
	bool HasAttribute(AttributeType attr) const { return ((this->m_nBotAttrs & attr) != 0); }
	MissionType GetMission() const   { return this->m_nMission; }
	void SetMission(MissionType val) { this->m_nMission = val; }
	
	/* thunk */
	ILocomotion *GetLocomotionInterface() const                        { return ft_GetLocomotionInterface      (this); }
	IBody *GetBodyInterface() const                                    { return ft_GetBodyInterface            (this); }
	IVision *GetVisionInterface() const                                { return ft_GetVisionInterface          (this); }
	IIntention *GetIntentionInterface() const                          { return ft_GetIntentionInterface       (this); }
	float GetDesiredPathLookAheadRange() const                         { return ft_GetDesiredPathLookAheadRange(this); }
	void PushRequiredWeapon(CTFWeaponBase *weapon)                     { return ft_PushRequiredWeapon          (this, weapon); }
	void PopRequiredWeapon()                                           { return ft_PopRequiredWeapon           (this); }
	bool IsLineOfFireClear(const Vector& to) const                     { return ft_IsLineOfFireClear_vec       (this, to); }
	bool IsLineOfFireClear(CBaseEntity *to) const                      { return ft_IsLineOfFireClear_ent       (this, to); }
	bool IsLineOfFireClear(const Vector& from, const Vector& to) const { return ft_IsLineOfFireClear_vec_vec   (this, from, to); }
	bool IsLineOfFireClear(const Vector& from, CBaseEntity *to) const  { return ft_IsLineOfFireClear_vec_ent   (this, from, to); }
	SuspectedSpyInfo_t *IsSuspectedSpy(CTFPlayer *spy)                 { return ft_IsSuspectedSpy              (this, spy); }
	void SuspectSpy(CTFPlayer *spy)                                    {        ft_SuspectSpy                  (this, spy); }
	void StopSuspectingSpy(CTFPlayer *spy)                             {        ft_StopSuspectingSpy           (this, spy); }
	bool IsKnownSpy(CTFPlayer *spy) const                              { return ft_IsKnownSpy                  (this, spy); }
	void RealizeSpy(CTFPlayer *spy)                                    {        ft_RealizeSpy                  (this, spy); }
	void ForgetSpy(CTFPlayer *spy)                                     {        ft_ForgetSpy                   (this, spy); }
	void AddItem(const char *name)                                     {        ft_AddItem                     (this, name); }
	float GetDesiredAttackRange() const                                { return ft_GetDesiredAttackRange       (this); }
	void EquipBestWeaponForThreat(const CKnownEntity *threat)          {        ft_EquipBestWeaponForThreat    (this, threat); }
	bool EquipRequiredWeapon()                                         { return ft_EquipRequiredWeapon         (this); }
	CTFPlayer *SelectRandomReachableEnemy()                            { return ft_SelectRandomReachableEnemy  (this); }
	bool ShouldAutoJump()                                              { return ft_ShouldAutoJump              (this); }
	const EventChangeAttributes_t *GetEventChangeAttributes(const char *name) const { return ft_GetEventChangeAttributes              (this,name); }
	void OnEventChangeAttributes(const EventChangeAttributes_t *ecattr) { return ft_OnEventChangeAttributes              (this,ecattr); }
	float TransientlyConsistentRandomValue(float time = 1.0f, int seed = 0) { return ft_TransientlyConsistentRandomValue              (this,time,seed); }

#ifdef ADD_EXTATTR
	/* custom: extended attributes */
	ExtendedAttr& ExtAttr()
	{
		CHandle<CTFBot> h_this = this;
		return s_ExtAttrs[h_this];
	}
#endif
	
#if TOOLCHAIN_FIXES
	DECL_EXTRACT(CUtlVector<CFmtStr>, m_Tags);
#endif
	DECL_EXTRACT(AttributeType,       m_nBotAttrs);
	/*uint8_t of[0x3D0];// +0x2830
	float m_flScale; // +0x2bf4
	uint8_t of[0x3D0];// +0x2830
	CHandle<CBaseEntity> m_hSBTarget; // +0x2c00*/

private:
	DECL_EXTRACT(MissionType,         m_nMission);
	
	static MemberFuncThunk<const CTFBot *, ILocomotion *                     > ft_GetLocomotionInterface;
	static MemberFuncThunk<const CTFBot *, IBody *                           > ft_GetBodyInterface;
	static MemberFuncThunk<const CTFBot *, IVision *                         > ft_GetVisionInterface;
	static MemberFuncThunk<const CTFBot *, IIntention *                      > ft_GetIntentionInterface;
	static MemberFuncThunk<const CTFBot *, float                             > ft_GetDesiredPathLookAheadRange;
	static MemberFuncThunk<      CTFBot *, void, CTFWeaponBase *             > ft_PushRequiredWeapon;
	static MemberFuncThunk<      CTFBot *, void                              > ft_PopRequiredWeapon;
	static MemberFuncThunk<const CTFBot *, bool, const Vector&               > ft_IsLineOfFireClear_vec;
	static MemberFuncThunk<const CTFBot *, bool, CBaseEntity *               > ft_IsLineOfFireClear_ent;
	static MemberFuncThunk<const CTFBot *, bool, const Vector&, const Vector&> ft_IsLineOfFireClear_vec_vec;
	static MemberFuncThunk<const CTFBot *, bool, const Vector&, CBaseEntity *> ft_IsLineOfFireClear_vec_ent;
	static MemberFuncThunk<      CTFBot *, SuspectedSpyInfo_t *, CTFPlayer * > ft_IsSuspectedSpy;
	static MemberFuncThunk<      CTFBot *, void, CTFPlayer *                 > ft_SuspectSpy;
	static MemberFuncThunk<      CTFBot *, void, CTFPlayer *                 > ft_StopSuspectingSpy;
	static MemberFuncThunk<const CTFBot *, bool, CTFPlayer *                 > ft_IsKnownSpy;
	static MemberFuncThunk<      CTFBot *, void, CTFPlayer *                 > ft_RealizeSpy;
	static MemberFuncThunk<      CTFBot *, void, CTFPlayer *                 > ft_ForgetSpy;
	static MemberFuncThunk<      CTFBot *, void, const char *                > ft_AddItem;
	static MemberFuncThunk<const CTFBot *, float                             > ft_GetDesiredAttackRange;
	static MemberFuncThunk<      CTFBot *, void, const CKnownEntity *        > ft_EquipBestWeaponForThreat;
	static MemberFuncThunk<      CTFBot *, bool                              > ft_EquipRequiredWeapon;
	static MemberFuncThunk<      CTFBot *, CTFPlayer *                       > ft_SelectRandomReachableEnemy;
	static MemberFuncThunk<      CTFBot *, bool                              > ft_ShouldAutoJump;
	static MemberFuncThunk<const CTFBot *, const EventChangeAttributes_t*, const char*   > ft_GetEventChangeAttributes;
    static MemberFuncThunk<      CTFBot *, void, const EventChangeAttributes_t*          > ft_OnEventChangeAttributes;
    static MemberFuncThunk<      CTFBot *, float, float, int          > ft_TransientlyConsistentRandomValue;
	
#ifdef ADD_EXTATTR
	static std::map<CHandle<CTFBot>, ExtendedAttr> s_ExtAttrs;
#endif
};


inline CTFBot *ToTFBot(CBaseEntity *pEntity)
{
	if (pEntity == nullptr)   return nullptr;
	if (!pEntity->IsPlayer()) return nullptr;
	
	/* not actually correct, but to do this the "right" way we'd need to do an
	 * rtti_cast to CBasePlayer before we can call IsBotOfType, and then we'd
	 * need to do another rtti_cast after that... may as well just do this */
	return rtti_cast<CTFBot *>(pEntity);
}


template<typename T> T *NextBotCreatePlayerBot(const char *name, bool fake_client = true);
template<> CTFBot *NextBotCreatePlayerBot<CTFBot>(const char *name, bool fake_client);


#endif
