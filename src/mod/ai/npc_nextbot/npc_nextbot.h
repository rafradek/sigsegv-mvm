#ifndef _INCLUDE_SIGSEGV_MOD_AI_NPC_NEXTBOT
#define _INCLUDE_SIGSEGV_MOD_AI_NPC_NEXTBOT
namespace Mod::AI::NPC_Nextbot
{
	using MyNextbotEntity = CBotNPCArcher;
	void LoadBodyHooks(void **vtable);
	void LoadLocomotionHooks(void **vtable);
	void LoadVisionHooks(void **vtable);

	
    class MyNextbotModule : public EntityModule
    {
    public:
        MyNextbotModule(CBaseEntity *entity) : EntityModule(entity), m_pEntity((CBotNPCArcher *)entity) {}

		void SetShooting(bool shoot) { this->m_bShouldShoot = shoot; }

		float GetAttackRange();

		void Update();

		void TransformToClass(int classIndex);
		
		void SetEyeOffset();

		MyNextbotEntity *m_pEntity;

		// Body interface
		IBody::PostureType m_posture = IBody::PostureType::STAND;

		Vector m_lookAtPos = vec3_origin;					// if m_lookAtSubject is non-NULL, it continually overwrites this position with its own
		EHANDLE m_lookAtSubject;
		Vector m_lookAtVelocity = vec3_origin;			// world velocity of lookat point, for tracking moving subjects
		CountdownTimerInline m_lookAtTrackingTimer;	
		
		IBody::LookAtPriorityType m_lookAtPriority = IBody::LookAtPriorityType::BORING;
		CountdownTimerInline m_lookAtExpireTimer;		// how long until this lookat expired
		IntervalTimerInline m_lookAtDurationTimer;	// how long have we been looking at this target
		INextBotReply *m_lookAtReplyWhenAimed = nullptr;
		bool m_isSightedIn = false;					// true if we are looking at our last lookat target
		bool m_hasBeenSightedIn = false;			// true if we have hit the current lookat target

		IntervalTimerInline m_headSteadyTimer;
		QAngle m_priorAngles = vec3_angle;				// last update's head angles
		QAngle m_desiredAngles = vec3_angle;

		CountdownTimerInline m_anchorRepositionTimer;	// the time is takes us to recenter our virtual mouse
		Vector m_anchorForward = vec3_origin;

		float m_flMaxHeadAngularVelocity = 1000;
		float m_flHeadTrackingInterval = 0.01f;
		float m_flHeadAimLeadTime = 0.0f;

		int m_iBodyYawParam = -1;
		int m_iBodyPitchParam = -1;
		int m_iBodyMoveYawParam = -1;
		int m_iBodyMoveScaleParam = -1;

		QAngle m_currentAimAngles = vec3_angle;
		Vector m_currentAimForward = vec3_origin;
		Activity m_forcedActivity = ACT_INVALID;
		Activity m_untranslatedActivity = ACT_INVALID;
		uint m_iForcedActivityFlags = 0U;
		bool m_bInAirWalk = false;

		bool m_bDuckForced = false;

		bool m_bShouldShoot = false;
		bool m_bShootingForced = false;
		bool m_bShootingDisabled = false;
		bool m_bWaitUntilFullReload = false;
		bool m_bFreeze = false;
		bool m_bNotSolidToPlayers = false;
		bool m_bIgnoreClips = false;
		bool m_bIgnoreEnemies = false;

		// Locomotion
		float m_flAcceleration = 3000.0f;
		float m_flDeceleration = 3000.0f;
		float m_flMaxRunningSpeed = 300.0f;
		float m_flDuckSpeed = 100.0f;
		float m_flJumpHeight = 72.0f;
		float m_flStepHeight = 18.0f;
		float m_flGravity = 800.0f;
		float m_flAutoJumpMinTime = 0.0f;
		float m_flAutoJumpMaxTime = 0.0f;
		bool m_bDodge = false;
		CountdownTimerInline m_autoJumpTimer;

		bool m_bAllowTakeFriendlyFire = false;

		float m_flHandAttackDamage = 50.0f;
		float m_flHandAttackTime = 0.0f;
		float m_flHandAttackRange = 50.0f;
		float m_flHandAttackSize = 20.0f;
		float m_flHandAttackSmackTime = 0.2f;
		float m_flHandAttackForce = 100.0f;
		const char *m_strHandAttackIcon = "fists";
		bool m_bHandAttackCleave = false;
		const char *m_strHandAttackSound = "Weapon_Fist.HitFlesh";
		const char *m_strHandAttackSoundMiss = "Weapon_Fist.Miss";
		bool m_bHandAttackSmack = false;
		CountdownTimerInline m_handAttackTimer;
		CountdownTimerInline m_handAttackSmackTimer;

		float m_flLastDamageTime = 0.0f;
		float m_flNextHurtSoundTime = 0.0f;
		const char *m_strHurtSound = "Sniper.PainSharp01";
		const char *m_strDeathSound = "Sniper.Death";

		int m_iPlayerClass = TF_CLASS_SNIPER;
		string_t m_strPlayerClass;
		bool m_bIsRobot = false;
		bool m_bIsRobotGiant = false;
		bool m_bNoFootsteps = false;
		const char *m_strFootstepSound = "";
		bool m_bIsZombie = false;
		CHandle<CBaseAnimating> m_hZombieCosmetic;

		bool m_bCrit = false;

		bool m_bForceEyeOffset = false;
		Vector m_ForceEyeOffset;

		enum DeathEffectType {
			NONE,
			DEFAULT,
			GIB,
			RAGDOLL
		};
		DeathEffectType m_DeathEffectType = DEFAULT;

		Activity m_LastAttackGestureActivity;
		int m_LastAttackGestureLayer;

		CountdownTimerInline m_VisionScanTime;

		const char *m_strCurAction = nullptr;
    };
	
    inline MyNextbotModule *GetNextbotModule(MyNextbotEntity *ent)
    {
        return *((MyNextbotModule **)&ent->m_bow.Get());
    }

	extern INextBot *my_current_nextbot;
	extern IBody *my_current_nextbot_body;
	extern ILocomotion *my_current_nextbot_locomotion;
	extern IVision *my_current_nextbot_vision;
	extern MyNextbotModule *my_current_nextbot_module;
	extern MyNextbotEntity *my_current_nextbot_entity;
	extern bool upkeep_current_nextbot;
	
	extern ConVar *nb_head_aim_steady_max_rate;
	extern ConVar *nb_head_aim_resettle_angle;
	extern ConVar *nb_head_aim_resettle_time;
	extern ConVar *nb_head_aim_settle_duration;

	extern const char *current_kill_icon;
	
}
#endif