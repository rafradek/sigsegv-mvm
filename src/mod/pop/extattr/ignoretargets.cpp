#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::Pop::ExtAttr::IgnoreTargets
{
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

		return DETOUR_MEMBER_CALL(ent);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Pop:ExtAttr:IgnoreTargets")
		{
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored", HIGH);
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_extattr_ignoretargets", "0", FCVAR_NOTIFY,
		"Extended bot attr: IgnoreTargets",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
