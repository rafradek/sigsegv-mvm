#include "mod.h"
#include "stub/tfweaponbase.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "stub/objects.h"
#include "stub/gamerules.h"
#include "stub/projectiles.h"
#include "stub/tfbot_behavior.h"
#include "util/clientmsg.h"


namespace Mod::AI::Improved_Targeting
{ 
	ConVar cvar_meele_ignore_healers("sig_ai_melee_ignore_healers", "1", FCVAR_NOTIFY,
		"Mod: melee bots ignore healers");
	ConVar cvar_force_attack_blockers("sig_ai_force_attack_blockers", "1", FCVAR_NOTIFY,
		"Mod: force attacking blocking players");
	ConVar cvar_giant_stomp("sig_ai_giant_stomp", "1", FCVAR_NOTIFY,
		"Mod: Giants stomp players and buildings");
	ConVar cvar_anti_spy_touch("sig_ai_anti_spy_touch", "0", FCVAR_NOTIFY,
		"Mod: make bots more aware of touching spies",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			static ConVarRef spy_touch_interval("tf_bot_suspect_spy_touch_interval");
			if (cvar_anti_spy_touch.GetBool()) {
				spy_touch_interval.SetValue(2);
			}
			else{
				spy_touch_interval.SetValue(5);
			}
		});
	
	ConVar cvar_close_explosive_attack("sig_ai_allow_close_explosive_attack", "200", FCVAR_NOTIFY,
		"Mod: allow bots to self damage themselves if they have more than this amount of health");

	struct DamageTracker {
		CHandle<CBaseEntity> blocker;
		int blocktime = 0;
		float stomptime = 0;
	};

	std::map<CHandle<CTFBot>, DamageTracker> damage_trackers;

	const INextBot *thread_bot_caller = nullptr;
	DETOUR_DECL_MEMBER(const CKnownEntity *, CTFBotMainAction_SelectMoreDangerousThreat, const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2)
	{
		auto action = reinterpret_cast<const CTFBotMainAction *>(this);
		
		if (cvar_meele_ignore_healers.GetBool())
			thread_bot_caller = nextbot;
		
		CBaseEntity *threat1entity = threat1->GetEntity();
		CBaseEntity *threat2entity = threat2->GetEntity();
		if (cvar_force_attack_blockers.GetBool()) {
			CTFPlayer *threat1player = ToTFPlayer(threat1entity);
			CTFPlayer *threat2player = ToTFPlayer(threat2entity);

			if ((threat1player != nullptr && threat1player->IsPlayerClass(TF_CLASS_SPY) && nextbot->IsRangeLessThan(threat1player, 250.0f)) || 
				(threat2player != nullptr && threat2player->IsPlayerClass(TF_CLASS_SPY) && nextbot->IsRangeLessThan(threat2player, 250.0f))) {

			}
			else {
				CTFBot *bot = ToTFBot(nextbot->GetEntity());
				auto find = damage_trackers.find(bot);
				if (find != damage_trackers.end() && gpGlobals->tickcount - find->second.blocktime < 40) {
					CBaseEntity *blocker = find->second.blocker;
					if (blocker == threat1entity) {
						return threat1;
					}
					else if (blocker == threat2entity){
						return threat2;
					}
				}
			}
		}

		// Ignore dispensers and teleporters if there are better targets to shoot
		bool threat1nonsentrybuilding = threat1entity->IsBaseObject() && ToBaseObject(threat1entity)->GetType() != OBJ_SENTRYGUN;
		bool threat2nonsentrybuilding = threat2entity->IsBaseObject() && ToBaseObject(threat2entity)->GetType() != OBJ_SENTRYGUN;

		if (threat1nonsentrybuilding && !threat2nonsentrybuilding) {
			return threat2;
		}
		else if (threat2nonsentrybuilding && !threat1nonsentrybuilding) {
			return threat1;
		}
		auto ret = DETOUR_MEMBER_CALL(CTFBotMainAction_SelectMoreDangerousThreat)(nextbot, them, threat1, threat2);

		thread_bot_caller = nullptr;

		return ret;
	}

	DETOUR_DECL_MEMBER(const CKnownEntity *, CTFBotMainAction_GetHealerOfThreat, const CKnownEntity *threat)
	{
		if (reinterpret_cast<CTFBotMainAction *>(this)->GetActor() == nullptr) return threat;

		if (thread_bot_caller != nullptr) {
			CTFBot *actor = ToTFBot(thread_bot_caller->GetEntity());
			if (actor != nullptr && actor->m_nBotSkill > 0 && actor->IsMiniBoss()) {
				if (actor->GetActiveWeapon() != nullptr && actor->GetActiveWeapon()->IsMeleeWeapon()) {
					return threat;
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFBotMainAction_GetHealerOfThreat)(threat);
	}

	bool callingfromhere = false;
	 
	void OnContactPart1(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) {

	}
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotMainAction_OnContact, CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
	{
		if (gpGlobals->tickcount % 7 == 0 && ent != nullptr && ent != actor && ENTINDEX(ent) != 0 
			&& ent->GetTeamNumber() != actor->GetTeamNumber() && trace != nullptr && trace->startpos.x != 0.0f && trace->fraction == 0.0f) {

			bool jumpstomp = actor->ExtAttr()[CTFBot::ExtendedAttr::JUMP_STOMP];
			CTFPlayer *player = ToTFPlayer(ent);

			bool jump = !(player != nullptr && player->GetModelScale() > 1.0f) && (jumpstomp || (cvar_giant_stomp.GetBool() && actor->GetModelScale() > 1.5f));
			bool stomp = jumpstomp || (jump && !(actor->IsPlayerClass(TF_CLASS_SCOUT) && actor->GetMaxHealth() < 4000));

			if ( (cvar_force_attack_blockers.GetBool() || jumpstomp) && ent != actor->GetGroundEntity()) {
				if (player == nullptr || !(player->m_Shared->IsStealthed() || player->m_Shared->InCond( TF_COND_DISGUISED ))) {
					DamageTracker &tracker = damage_trackers[actor];

					INextBot *bot = actor->MyNextBotPointer();
					bot->GetVisionInterface()->AddKnownEntity(ent);
					
					if (jump && (actor->GetFlags() & FL_ONGROUND) && bot->GetLocomotionInterface()->GetDesiredSpeed() != 0.0f ) {
						// Give jumping giants extra force so they can stomp players when going upstairs
						float jumpheight = 1.0f;
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(actor, jumpheight, mod_jump_height);

						if (jumpheight == 1.0f)
							actor->AddCustomAttribute("increased jump height", 1.15f, 3600.0f);

						bot->GetLocomotionInterface()->Jump();
					}

					if (gpGlobals->tickcount - tracker.blocktime >= 20) {
						tracker.blocker = ent;
						tracker.blocktime = gpGlobals->tickcount;
					}
				}
			}
			else if (stomp && (ent->IsPlayer() || ent->IsBaseObject())) {

				DamageTracker &tracker = damage_trackers[actor];

				// Super scouts can't stomp
				if (tracker.stomptime + 3.0f < gpGlobals->curtime) {
					tracker.stomptime = gpGlobals->curtime;
					actor->EmitSound("Weapon_Mantreads.Impact");
					CTakeDamageInfo info( actor, actor, nullptr, vec3_origin, actor->GetAbsOrigin(), ent->IsBaseObject() ? ent->GetMaxHealth() * 4 : 750, DMG_FALL, TF_DMG_CUSTOM_BOOTS_STOMP );
					ent->TakeDamage(info);
				}
			}
		}

		if (gpGlobals->tickcount % 3 == 0 && cvar_anti_spy_touch.GetBool() && ent != nullptr && ent != actor && ent->GetTeamNumber() != actor->GetTeamNumber() && ent->IsPlayer()
			 && trace != nullptr && trace->startpos.x != 0.0f && trace->fraction == 0.0f) {
				 
			CTFPlayer *player = ToTFPlayer(ent);
			
			if ( player != nullptr && (player->m_Shared->IsStealthed() || player->m_Shared->InCond( TF_COND_DISGUISED )) )
			{
				callingfromhere = true;
				actor->SuspectSpy(player);
				callingfromhere = false;
			}
			Vector move = trace->endpos - actor->GetAbsOrigin();
		}
		return DETOUR_MEMBER_CALL(CTFBotMainAction_OnContact)(actor, ent, trace);
	}

	RefCount rc_CTFBot_Touch;
	DETOUR_DECL_MEMBER(void, CTFBot_Touch, CBaseEntity *ent)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		SCOPED_INCREMENT_IF(rc_CTFBot_Touch, cvar_anti_spy_touch.GetBool());
		DETOUR_MEMBER_CALL(CTFBot_Touch)(ent);
	}

	DETOUR_DECL_MEMBER(void, CTFBot_SuspectSpy, CTFPlayer *spy)
	{
		if (rc_CTFBot_Touch && !callingfromhere) 
			return;
		
		DETOUR_MEMBER_CALL(CTFBot_SuspectSpy)(spy);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBot_IsExplosiveProjectileWeapon, CTFWeaponBase *weapon)
	{
		if (cvar_close_explosive_attack.GetInt() > 0) {
			auto bot = reinterpret_cast<CTFBot *>(this);
			if (bot->GetHealth() >= cvar_close_explosive_attack.GetInt())
			{
				return false;
			}
		}

		return DETOUR_MEMBER_CALL(CTFBot_IsExplosiveProjectileWeapon)(weapon);
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMedicHeal_Update, CTFBot *me, float interval)
	{
		auto result = DETOUR_MEMBER_CALL(CTFBotMedicHeal_Update)(me, interval);
		auto medigun = rtti_cast<CWeaponMedigun *>(me->GetActiveTFWeapon());
		auto medigunHasTarget = medigun != nullptr && medigun->GetHealTarget() == reinterpret_cast<CTFBotMedicHeal *>(this)->GetPatient();
		if (medigun && medigunHasTarget && me->ExtAttr()[CTFBot::ExtendedAttr::MEDIC_LOOK_AT_THREATS]) {
			const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat(false);
			if (threat != nullptr && threat->GetEntity() != nullptr) {
				me->GetBodyInterface()->AimHeadTowards(threat->GetEntity(), IBody::LookAtPriorityType::OVERRIDE_ALL, 0.15f, nullptr, "Look at threat");
			}
		}
		return result;
	}
	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("AI:Improved_Targeting")
		{
			/* Don't allow melee bots to chase medic healers */
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_GetHealerOfThreat,         "CTFBotMainAction::GetHealerOfThreat");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_SelectMoreDangerousThreat, "CTFBotMainAction::SelectMoreDangerousThreat");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_OnContact,                 "CTFBotMainAction::OnContact");
			MOD_ADD_DETOUR_MEMBER(CTFBot_Touch,                               "CTFBot::Touch");
			MOD_ADD_DETOUR_MEMBER(CTFBot_SuspectSpy,                          "CTFBot::SuspectSpy");
			MOD_ADD_DETOUR_MEMBER(CTFBot_IsExplosiveProjectileWeapon,         "CTFBot::IsExplosiveProjectileWeapon");
			MOD_ADD_DETOUR_MEMBER(CTFBotMedicHeal_Update,                     "CTFBotMedicHeal::Update");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void FrameUpdatePostEntityThink() override
		{
			if (gpGlobals->tickcount % 25 == 0) {
				for (auto it = damage_trackers.begin(); it != damage_trackers.end();) {
					if (it->first == nullptr || !it->first->IsAlive()) {
						it = damage_trackers.erase(it);
					}
					else {
						it++;
					}
				}
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_improved_targeting", "0", FCVAR_NOTIFY,
		"Mod: use improved targeting system for bots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
