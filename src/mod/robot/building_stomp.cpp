#include "mod.h"
#include "util/scope.h"
#include "stub/tfbot.h"


namespace Mod::Robot::Building_Stomp
{
	RefCount rc_CTFBotMainAction_OnContact;
	RefCount rc_CTFBotMainAction_OnStuck;
	
	CTFBot *bot_contact = nullptr;
	CTFBot *bot_stuck   = nullptr;
	
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotMainAction_OnContact, CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
	{
		SCOPED_INCREMENT(rc_CTFBotMainAction_OnContact);
		bot_contact = actor;
		return DETOUR_MEMBER_CALL(actor, ent, trace);
	}
	
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotMainAction_OnStuck, CTFBot *actor)
	{
		SCOPED_INCREMENT(rc_CTFBotMainAction_OnStuck);
		bot_stuck = actor;
		return DETOUR_MEMBER_CALL(actor);
	}
	
	
	// CTFBotMainAction::OnContact: giant bots instant stomp
	// CTFBotMainAction::OnStuck:   all   bots delayed stomp
	
	
	ConVar cvar_contact_nodamage("sig_robot_building_stomp_contact_nodamage", "1", FCVAR_NOTIFY,
		"OnContact stomp (instant, giants only): prevent damage from being done");
	ConVar cvar_contact_addknown("sig_robot_building_stomp_contact_addknown", "1", FCVAR_NOTIFY,
		"OnContact stomp (instant, giants only): make robot immediately aware of building");
	
	ConVar cvar_stuck_nodamage("sig_robot_building_stomp_stuck_nodamage", "1", FCVAR_NOTIFY,
		"OnStuck stomp   (delayed, all robots):  prevent damage from being done");
	ConVar cvar_stuck_addknown("sig_robot_building_stomp_stuck_addknown", "1", FCVAR_NOTIFY,
		"OnStuck stomp   (delayed, all robots):  make robot immediately aware of building");
	
	
	void AddKnownToBot(CTFBot *bot, CBaseEntity *ent)
	{
		IVision *vision = bot->GetVisionInterface();
		
		if (vision->GetKnown(ent) == nullptr) {
			vision->AddKnownEntity(ent);
		}
	}
	
	
	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& info)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (ent->IsBaseObject() && info.GetDamageType() == DMG_BLAST && info.GetDamageCustom() == TF_DMG_CUSTOM_NONE) {
			if (rc_CTFBotMainAction_OnContact > 0 && bot_contact != nullptr && info.GetInflictor() == bot_contact && info.GetAttacker() == bot_contact) {
				if (cvar_contact_addknown.GetBool()) {
					AddKnownToBot(bot_contact, ent);
				}
				
				if (cvar_contact_nodamage.GetBool()) {
				//	DevMsg("Preventing damage due to giant-robot-vs-building instant stomp from CTFBotMainAction::OnContact\n");
					return 0;
				}
			}
			
			if (rc_CTFBotMainAction_OnStuck > 0 && bot_stuck != nullptr && info.GetInflictor() == bot_stuck && info.GetAttacker() == bot_stuck) {
				if (cvar_stuck_addknown.GetBool()) {
					AddKnownToBot(bot_stuck, ent);
				}
				
				if (cvar_stuck_nodamage.GetBool()) {
				//	DevMsg("Preventing damage due to robot-vs-building delayed stomp from CTFBotMainAction::OnStuck\n");
					return 0;
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(info);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Robot:Building_Stomp")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_OnContact, "CTFBotMainAction::OnContact");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_OnStuck,   "CTFBotMainAction::OnStuck");
			
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage, "CBaseEntity::TakeDamage");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_robot_building_stomp", "0", FCVAR_NOTIFY,
		"Mod: replace robots' stuck-with-building-stomp ability with something less stupid",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	// TODO:
	// add a priority boost for entities that have been responsible for recent
	// OnContact or OnStuck events of the bot
}
