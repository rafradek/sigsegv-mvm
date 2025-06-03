#include "mod.h"
#include "stub/tfbot.h"
#include "util/iterate.h"


namespace Mod::Pop::ExtAttr::IgnoreTargets
{
	class TakeDamageFromModule : public EntityModule
	{
	public:
		TakeDamageFromModule(CBaseEntity *entity) : EntityModule(entity) {}
		std::unordered_set<int> tookDamageFrom;
		std::unordered_set<int> allyTookDamageFrom;
		float nextNotifyAlliesHurt = 0.0f;
		float nextNotifyHurt = 0.0f;
	};

	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		IVision *vision = reinterpret_cast<IVision *>(this);
		CTFBot *actor = static_cast<CTFBot *>(vision->GetBot()->GetEntity());
		
		auto attrs = actor->ExtAttr();
		if (attrs[CTFBot::ExtendedAttr::IGNORE_BUILDINGS] && ent->IsBaseObject()) {
			return true;
		}
		else if (attrs[CTFBot::ExtendedAttr::IGNORE_PLAYERS] && ent->IsPlayer()) {
			return true;
		}
		else if (attrs[CTFBot::ExtendedAttr::IGNORE_REAL_PLAYERS] && ent->IsPlayer() && ent->MyNextBotPointer() == nullptr) {
			return true;
		}
		else if (attrs[CTFBot::ExtendedAttr::IGNORE_BOTS] && ent->IsPlayer() && ent->MyNextBotPointer() != nullptr) {
			return true;
		}
		else if (attrs[CTFBot::ExtendedAttr::IGNORE_NPC] && !ent->IsPlayer() && ent->MyNextBotPointer() != nullptr) {
			return true;
		}
		else if (attrs[CTFBot::ExtendedAttr::IGNORE_NO_DAMAGE]) {
			return !actor->GetOrCreateEntityModule<TakeDamageFromModule>("takedamagefrom")->tookDamageFrom.contains(ent->GetRefEHandle().ToInt());
		}
		else if (attrs[CTFBot::ExtendedAttr::IGNORE_NO_DAMAGE_ALLY]) {
			return !actor->GetOrCreateEntityModule<TakeDamageFromModule>("takedamagefrom")->allyTookDamageFrom.contains(ent->GetRefEHandle().ToInt());
		}

		return DETOUR_MEMBER_CALL(ent);
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage_Alive, const CTakeDamageInfo &info)
	{
		
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
		auto result = DETOUR_MEMBER_CALL(info);
		auto bot = ToTFBot(player);
		if (info.GetAttacker() != nullptr && bot != nullptr && info.GetAttacker()->IsCombatCharacter()) {
			auto mod = bot->GetOrCreateEntityModule<TakeDamageFromModule>("takedamagefrom");
			if (bot->ExtAttr()[CTFBot::ExtendedAttr::IGNORE_NO_DAMAGE]) {
				if (gpGlobals->curtime > mod->nextNotifyHurt) {
					mod->nextNotifyHurt = gpGlobals->curtime + 0.15f;
					mod->tookDamageFrom.insert(info.GetAttacker()->GetRefEHandle().ToInt());
				}
			}
			if (gpGlobals->curtime > mod->nextNotifyAlliesHurt) {
				int handleNum = info.GetAttacker()->GetRefEHandle().ToInt();
				ForEachTFBot([&](CTFBot *ally) {
					if (bot->GetTeamNumber() == ally->GetTeamNumber() && ally->IsAlive() && ally->ExtAttr()[CTFBot::ExtendedAttr::IGNORE_NO_DAMAGE_ALLY] && ally->GetVisionInterface()->IsAbleToSee(bot, IVision::FieldOfViewCheckType::DISREGARD_FOV, nullptr) && ally->GetVisionInterface()->IsAbleToSee(info.GetAttacker(), IVision::FieldOfViewCheckType::DISREGARD_FOV, nullptr)) {
						auto modAlly = ally->GetOrCreateEntityModule<TakeDamageFromModule>("takedamagefrom");
						modAlly->allyTookDamageFrom.insert(handleNum);
					}
				});
				mod->nextNotifyAlliesHurt = gpGlobals->curtime + 0.5f;
			}
		}

		return result;
	}

    DETOUR_DECL_MEMBER(void, CTFPlayer_Spawn)
	{
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this); 
		
		DETOUR_MEMBER_CALL();

		int handleNum = player->GetRefEHandle().ToInt();
		ForEachTFBot([&](CTFPlayer *other) {
			auto mod = other->GetEntityModule<TakeDamageFromModule>("takedamagefrom");
			if (mod != nullptr && other->GetTeamNumber() != player->GetTeamNumber() && other->IsAlive()) {
				mod->allyTookDamageFrom.erase(handleNum);
				mod->tookDamageFrom.erase(handleNum);
			}
		});

		auto mod = player->GetEntityModule<TakeDamageFromModule>("takedamagefrom");
		if (mod != nullptr) {
			mod->allyTookDamageFrom.clear();
			mod->tookDamageFrom.clear();
		}
    }

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Pop:ExtAttr:IgnoreTargets")
		{
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored", HIGH);
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage_Alive, "CTFPlayer::OnTakeDamage_Alive");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Spawn, "CTFPlayer::Spawn");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_extattr_ignoretargets", "0", FCVAR_NOTIFY,
		"Extended bot attr: IgnoreTargets",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
