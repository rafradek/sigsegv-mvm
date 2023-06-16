#ifndef _INCLUDE_SIGSEGV_MOD_AI_NPC_NEXTBOT
#define _INCLUDE_SIGSEGV_MOD_AI_NPC_NEXTBOT
namespace Mod::AI::NPC_Nextbot
{
	using MyNextbotEntity = CBotNPCArcher;
	void LoadBodyHooks(void **vtable);
	void LoadLocomotionHooks(void **vtable);
	void LoadVisionHooks(void **vtable);

	
	
	#define MAX_LAYER_RECORDS 15

	struct LayerRecord
	{
		int m_sequence;
		float m_cycle;
		float m_weight;
		int m_order;

		LayerRecord()
		{
			m_sequence = 0;
			m_cycle = 0;
			m_weight = 0;
			m_order = 0;
		}

		LayerRecord( const LayerRecord& src )
		{
			m_sequence = src.m_sequence;
			m_cycle = src.m_cycle;
			m_weight = src.m_weight;
			m_order = src.m_order;
		}
	};

	struct LagRecord
	{
	public:
		LagRecord()
		{
			m_fFlags = 0;
			m_vecOrigin.Init();
			m_vecAngles.Init();
			m_vecMinsPreScaled.Init();
			m_vecMaxsPreScaled.Init();
			m_flSimulationTime = -1;
			m_masterSequence = 0;
			m_masterCycle = 0;
		}

		LagRecord( const LagRecord& src )
		{
			m_fFlags = src.m_fFlags;
			m_vecOrigin = src.m_vecOrigin;
			m_vecAngles = src.m_vecAngles;
			m_vecMinsPreScaled = src.m_vecMinsPreScaled;
			m_vecMaxsPreScaled = src.m_vecMaxsPreScaled;
			m_flSimulationTime = src.m_flSimulationTime;
			for( int layerIndex = 0; layerIndex < MAX_LAYER_RECORDS; ++layerIndex )
			{
				m_layerRecords[layerIndex] = src.m_layerRecords[layerIndex];
			}
			m_masterSequence = src.m_masterSequence;
			m_masterCycle = src.m_masterCycle;
		}

		// Did player die this frame
		int						m_fFlags;

		// Player position, orientation and bbox
		Vector					m_vecOrigin;
		QAngle					m_vecAngles;
		Vector					m_vecMinsPreScaled;
		Vector					m_vecMaxsPreScaled;

		float					m_flSimulationTime;	
		
		// Player animation details, so we can get the legs in the right spot.
		LayerRecord				m_layerRecords[MAX_LAYER_RECORDS];
		int						m_masterSequence;
		float					m_masterCycle;
	};

	class LagCompensatedEntity : public AutoList<LagCompensatedEntity>
	{
	public:
		LagCompensatedEntity(CBaseAnimatingOverlay *entity) { this->entity = entity; }
		virtual bool WantsLagCompensation(CBasePlayer *player, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits);

		CBaseAnimatingOverlay *entity;
		CUtlFixedLinkedList< LagRecord > track;
		bool restore;
		LagRecord restoreData;
		LagRecord changeData;

	};

    class MyNextbotModule : public EntityModule, public LagCompensatedEntity
    {
    public:
        MyNextbotModule(CBaseEntity *entity) : EntityModule(entity), LagCompensatedEntity((CBaseAnimatingOverlay *)entity), m_pEntity((CBotNPCArcher *)entity) {}

		virtual bool WantsLagCompensation(CBasePlayer *player, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits) override;

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
		string_t m_strPlayerClass = MAKE_STRING();
		bool m_bIsRobot = false;
		bool m_bIsRobotGiant = false;
		bool m_bNoFootsteps = false;
		const char *m_strFootstepSound = "";
		string_t m_strModel = NULL_STRING;
		bool m_bUsePlayerBounds = true;
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