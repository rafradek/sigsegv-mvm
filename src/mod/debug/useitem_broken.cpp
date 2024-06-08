#include "mod.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "util/trace.h"
#include "util/backtrace.h"
#include "util/misc.h"
#include "util/clientmsg.h"

#include <tier1/utlstack.h>


//#include "../mvm-reversed/server/tf/bot/behavior/tf_bot_use_item.h"
class CTFBotUseItem : public Action<CTFBot>
{
public:
	CHandle<CTFWeaponBase> m_hItem;
	CountdownTimer m_ctInitialDelay;
};


namespace Mod::Debug::UseItem_Broken
{
	// here's the problem:
	// the initial delay is calculated based on the required item's m_flNextPrimaryAttack
	// however, m_flNextPrimaryAttack isn't set to something reasonable until the weapon has actually been deployed
	// so our initial delay is set to 100ms or something dumb
	// and then as soon as we EquipRequiredWeapon, m_flNextPrimaryAttack is updated to 500ms from now
	// but we've already decided we'll be done after 100ms
	
	
	const char *WeaponID_ToString(int id)
	{
		static const char *const names[] = {
			"TF_WEAPON_NONE",
			"TF_WEAPON_BAT",
			"TF_WEAPON_BAT_WOOD",
			"TF_WEAPON_BOTTLE",
			"TF_WEAPON_FIREAXE",
			"TF_WEAPON_CLUB",
			"TF_WEAPON_CROWBAR",
			"TF_WEAPON_KNIFE",
			"TF_WEAPON_FISTS",
			"TF_WEAPON_SHOVEL",
			"TF_WEAPON_WRENCH",
			"TF_WEAPON_BONESAW",
			"TF_WEAPON_SHOTGUN_PRIMARY",
			"TF_WEAPON_SHOTGUN_SOLDIER",
			"TF_WEAPON_SHOTGUN_HWG",
			"TF_WEAPON_SHOTGUN_PYRO",
			"TF_WEAPON_SCATTERGUN",
			"TF_WEAPON_SNIPERRIFLE",
			"TF_WEAPON_MINIGUN",
			"TF_WEAPON_SMG",
			"TF_WEAPON_SYRINGEGUN_MEDIC",
			"TF_WEAPON_TRANQ",
			"TF_WEAPON_ROCKETLAUNCHER",
			"TF_WEAPON_GRENADELAUNCHER",
			"TF_WEAPON_PIPEBOMBLAUNCHER",
			"TF_WEAPON_FLAMETHROWER",
			"TF_WEAPON_GRENADE_NORMAL",
			"TF_WEAPON_GRENADE_CONCUSSION",
			"TF_WEAPON_GRENADE_NAIL",
			"TF_WEAPON_GRENADE_MIRV",
			"TF_WEAPON_GRENADE_MIRV_DEMOMAN",
			"TF_WEAPON_GRENADE_NAPALM",
			"TF_WEAPON_GRENADE_GAS",
			"TF_WEAPON_GRENADE_EMP",
			"TF_WEAPON_GRENADE_CALTROP",
			"TF_WEAPON_GRENADE_PIPEBOMB",
			"TF_WEAPON_GRENADE_SMOKE_BOMB",
			"TF_WEAPON_GRENADE_HEAL",
			"TF_WEAPON_GRENADE_STUNBALL",
			"TF_WEAPON_GRENADE_JAR",
			"TF_WEAPON_GRENADE_JAR_MILK",
			"TF_WEAPON_PISTOL",
			"TF_WEAPON_PISTOL_SCOUT",
			"TF_WEAPON_REVOLVER",
			"TF_WEAPON_NAILGUN",
			"TF_WEAPON_PDA",
			"TF_WEAPON_PDA_ENGINEER_BUILD",
			"TF_WEAPON_PDA_ENGINEER_DESTROY",
			"TF_WEAPON_PDA_SPY",
			"TF_WEAPON_BUILDER",
			"TF_WEAPON_MEDIGUN",
			"TF_WEAPON_GRENADE_MIRVBOMB",
			"TF_WEAPON_FLAMETHROWER_ROCKET",
			"TF_WEAPON_GRENADE_DEMOMAN",
			"TF_WEAPON_SENTRY_BULLET",
			"TF_WEAPON_SENTRY_ROCKET",
			"TF_WEAPON_DISPENSER",
			"TF_WEAPON_INVIS",
			"TF_WEAPON_FLAREGUN",
			"TF_WEAPON_LUNCHBOX",
			"TF_WEAPON_JAR",
			"TF_WEAPON_COMPOUND_BOW",
			"TF_WEAPON_BUFF_ITEM",
			"TF_WEAPON_PUMPKIN_BOMB",
			"TF_WEAPON_SWORD",
			"TF_WEAPON_DIRECTHIT",
			"TF_WEAPON_LIFELINE",
			"TF_WEAPON_LASER_POINTER",
			"TF_WEAPON_DISPENSER_GUN",
			"TF_WEAPON_SENTRY_REVENGE",
			"TF_WEAPON_JAR_MILK",
			"TF_WEAPON_HANDGUN_SCOUT_PRIMARY",
			"TF_WEAPON_BAT_FISH",
			"TF_WEAPON_CROSSBOW",
			"TF_WEAPON_STICKBOMB",
			"TF_WEAPON_HANDGUN_SCOUT_SEC",
			"TF_WEAPON_SODA_POPPER",
			"TF_WEAPON_SNIPERRIFLE_DECAP",
			"TF_WEAPON_RAYGUN",
			"TF_WEAPON_PARTICLE_CANNON",
			"TF_WEAPON_MECHANICAL_ARM",
			"TF_WEAPON_DRG_POMSON",
			"TF_WEAPON_BAT_GIFTWRAP",
			"TF_WEAPON_GRENADE_ORNAMENT",
			"TF_WEAPON_RAYGUN_REVENGE",
			"TF_WEAPON_PEP_BRAWLER_BLASTER",
			"TF_WEAPON_CLEAVER",
			"TF_WEAPON_GRENADE_CLEAVER",
			"TF_WEAPON_STICKY_BALL_LAUNCHER",
			"TF_WEAPON_GRENADE_STICKY_BALL",
			"TF_WEAPON_SHOTGUN_BUILDING_RESCUE",
			"TF_WEAPON_CANNON",
			"TF_WEAPON_THROWABLE",
			"TF_WEAPON_GRENADE_THROWABLE",
			"TF_WEAPON_PDA_SPY_BUILD",
			"TF_WEAPON_GRENADE_WATERBALLOON",
			"TF_WEAPON_HARVESTER_SAW",
			"TF_WEAPON_SPELLBOOK",
			"TF_WEAPON_SPELLBOOK_PROJECTILE",
			"TF_WEAPON_SNIPERRIFLE_CLASSIC",
			"TF_WEAPON_PARACHUTE",
			"TF_WEAPON_GRAPPLINGHOOK",
			"TF_WEAPON_PASSTIME_GUN",
			"TF_WEAPON_CHARGED_SMG",
		};
		
		if (id >= 0 && (size_t)id < countof(names)) {
			return names[id];
		} else {
			return "???";
		}
	}
	
	
	const char *ActionResult_ToString(const ActionResult<CTFBot>& result)
	{
		static char buf[1024];
		
		switch (result.transition) {
		case ActionTransition::CONTINUE:
			snprintf(buf, sizeof(buf), "CONTINUE");
			break;
		case ActionTransition::CHANGE_TO:
			snprintf(buf, sizeof(buf), "CHANGE_TO: %s", result.action->GetName());
			break;
		case ActionTransition::SUSPEND_FOR:
			snprintf(buf, sizeof(buf), "SUSPEND_FOR: %s", result.action->GetName());
			break;
		case ActionTransition::DONE:
			snprintf(buf, sizeof(buf), "DONE");
			break;
		default:
			snprintf(buf, sizeof(buf), "???");
			break;
		}
		
		return buf;
	}
	
	
	Action<CTFBot> *FindBehavior_Recursive(INextBotEventResponder *node, const char *name)
	{
		auto action = rtti_cast<Action<CTFBot> *>(node);
		if (action != nullptr && strcmp(action->GetName(), name) == 0) {
			return action;
		}
		
		for (auto child = node->FirstContainedResponder(); child != nullptr; child = node->NextContainedResponder(child)) {
			action = FindBehavior_Recursive(child, name);
			if (action != nullptr) {
				return action;
			}
		}
		
		return nullptr;
	}
	
	Action<CTFBot> *FindBehavior(CTFBot *actor, const char *name)
	{
		return FindBehavior_Recursive(actor->GetIntentionInterface(), name);
	}
	
	
	RefCount rc_CTFBotUseItem_OnStart;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotUseItem_OnStart, CTFBot *actor, Action<CTFBot> *action)
	{
		auto useitem = reinterpret_cast<CTFBotUseItem *>(this);
		
		ClientMsgAll(" \n[%8.3f] CTFBotUseItem::OnStart(#%d)\n", gpGlobals->curtime, ENTINDEX(actor));
		
		SCOPED_INCREMENT(rc_CTFBotUseItem_OnStart);
		auto result = DETOUR_MEMBER_CALL(actor, action);
		ClientMsgAll("%*sItem's m_flNextPrimaryAttack @ %.3f\n", 13, " ", (float)useitem->m_hItem->m_flNextPrimaryAttack);
		ClientMsgAll("%*sInitial delay timer set to %.3f sec duration; will elapse @ %.3f\n", 13, " ", useitem->m_ctInitialDelay.GetCountdownDuration(), gpGlobals->curtime + useitem->m_ctInitialDelay.GetRemainingTime());
		ClientMsgAll("%*s%s\n", 13, " ", ActionResult_ToString(result));
		return result;
	}
	
	RefCount rc_CTFBotUseItem_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotUseItem_Update, CTFBot *actor, float dt)
	{
		auto useitem = reinterpret_cast<CTFBotUseItem *>(this);
		
		ClientMsgAll(" \n[%8.3f] CTFBotUseItem::Update(#%d)\n", gpGlobals->curtime, ENTINDEX(actor));
		
		if (useitem->m_hItem != nullptr) {
			ClientMsgAll("%*s[m_bRageDraining: %s]\n",                 13, " ", (actor->m_Shared->m_bRageDraining ? "true" : "false"));
			ClientMsgAll("%*s[m_flRageMeter: %6.2f]\n",                13, " ", (float)actor->m_Shared->m_flRageMeter);
			ClientMsgAll("%*s[m_flEnergyDrinkMeter: %6.2f]\n",         13, " ", (float)actor->m_Shared->m_flEnergyDrinkMeter);
			ClientMsgAll("%*s[InCond(TF_COND_TAUNTING): %s]\n",        13, " ", (actor->m_Shared->InCond(TF_COND_TAUNTING) ? "true" : "false"));
			ClientMsgAll("%*s[item->HasAmmo(): %s]\n",                 13, " ", (useitem->m_hItem->HasAmmo() ? "true" : "false"));
			ClientMsgAll("%*s[item->m_flLastFireTime: %8.3f]\n",       13, " ", (float)useitem->m_hItem->m_flLastFireTime);
			ClientMsgAll("%*s[item->m_flEffectBarRegenTime: %8.3f]\n", 13, " ", (float)useitem->m_hItem->m_flEffectBarRegenTime);
		}
		
		if (useitem->m_ctInitialDelay.HasStarted() && useitem->m_ctInitialDelay.IsElapsed()) {
			ClientMsgAll("%*sInitial delay elapsed; pressing +attack now\n", 13, " ");
		}
		
		SCOPED_INCREMENT(rc_CTFBotUseItem_Update);
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		ClientMsgAll("%*s%s\n", 13, " ", ActionResult_ToString(result));
		return result;
	}
	
	RefCount rc_CTFBotUseItem_OnEnd;
	DETOUR_DECL_MEMBER(void, CTFBotUseItem_OnEnd, CTFBot *actor, Action<CTFBot> *action)
	{
		ClientMsgAll(" \n[%8.3f] CTFBotUseItem::OnEnd(#%d)\n", gpGlobals->curtime, ENTINDEX(actor));
		
		SCOPED_INCREMENT(rc_CTFBotUseItem_OnEnd);
		DETOUR_MEMBER_CALL(actor, action);
		
		ClientMsgAll(" \n================================================================================\n");
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBot_PushRequiredWeapon, CTFWeaponBase *weapon)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		/* TODO: remove this garbage! */
		constexpr int OFF_m_RequiredWeapons = 0x25c4;
		auto m_RequiredWeapons = reinterpret_cast<CUtlStack<CHandle<CTFWeaponBase>> *>((uintptr_t)bot + OFF_m_RequiredWeapons);
		
		DETOUR_MEMBER_CALL(weapon);
		if (rc_CTFBotUseItem_OnStart > 0) {
			ClientMsgAll("%*sCTFBot::PushRequiredWeapon: stack:", 13, " ");
			FOR_EACH_VEC((*m_RequiredWeapons), i) {
				ClientMsgAll(" %s%s", (i == m_RequiredWeapons->Count() - 1 ? "*" : ""),
					WeaponID_ToString((*m_RequiredWeapons)[i]->GetWeaponID()));
			}
			if (m_RequiredWeapons->Count() == 0) {
				ClientMsgAll(" (empty)\n");
			} else {
				ClientMsgAll("\n");
			}
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_PopRequiredWeapon)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		/* TODO: remove this garbage! */
		constexpr int OFF_m_RequiredWeapons = 0x25c4;
		auto m_RequiredWeapons = reinterpret_cast<CUtlStack<CHandle<CTFWeaponBase>> *>((uintptr_t)bot + OFF_m_RequiredWeapons);
		
		DETOUR_MEMBER_CALL();
		if (rc_CTFBotUseItem_OnEnd > 0) {
			ClientMsgAll("%*sCTFBot::PopRequiredWeapon: stack:", 13, " ");
			FOR_EACH_VEC((*m_RequiredWeapons), i) {
				ClientMsgAll(" %s%s", (i == m_RequiredWeapons->Count() - 1 ? "*" : ""),
					WeaponID_ToString((*m_RequiredWeapons)[i]->GetWeaponID()));
			}
			if (m_RequiredWeapons->Count() == 0) {
				ClientMsgAll(" (empty)\n");
			} else {
				ClientMsgAll("\n");
			}
		}
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFBot_EquipRequiredWeapon)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		const char *before = (bot->GetActiveTFWeapon() != nullptr ? WeaponID_ToString(bot->GetActiveTFWeapon()->GetWeaponID()) : "nullptr");
		auto result = DETOUR_MEMBER_CALL();
		const char *after  = (bot->GetActiveTFWeapon() != nullptr ? WeaponID_ToString(bot->GetActiveTFWeapon()->GetWeaponID()) : "nullptr");
		
		if (strcmp(before, after) != 0) {
			ClientMsgAll(" \n[%8.3f] CTFBot::EquipRequiredWeapon(#%d): %s -> %s\n", gpGlobals->curtime, ENTINDEX(bot), before, after);
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:UseItem_Broken")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotUseItem_OnStart, "CTFBotUseItem::OnStart");
			MOD_ADD_DETOUR_MEMBER(CTFBotUseItem_Update,  "CTFBotUseItem::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotUseItem_OnEnd,   "CTFBotUseItem::OnEnd");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_PushRequiredWeapon, "CTFBot::PushRequiredWeapon");
			MOD_ADD_DETOUR_MEMBER(CTFBot_PopRequiredWeapon,  "CTFBot::PopRequiredWeapon");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_EquipRequiredWeapon, "CTFBot::EquipRequiredWeapon");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_useitem_broken", "0", FCVAR_NOTIFY,
		"Debug: brokenness of CTFBotUseItem since Tough Break update",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
