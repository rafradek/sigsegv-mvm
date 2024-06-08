#include "mod.h"
#include "stub/tfplayer.h"
#include "util/scope.h"


namespace Mod::Sound::Unused_Credits
{
	RefCount rc_CCurrencyPack_MyTouch;
	DETOUR_DECL_MEMBER(bool, CCurrencyPack_MyTouch, CBasePlayer *pPlayer)
	{
		SCOPED_INCREMENT(rc_CCurrencyPack_MyTouch);
		return DETOUR_MEMBER_CALL(pPlayer);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_SpeakConceptIfAllowed, int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (iConcept == MP_CONCEPT_MVM_MONEY_PICKUP && rc_CCurrencyPack_MyTouch > 0 && player != nullptr) {
			switch (player->GetPlayerClass()->GetClassIndex()) {
			case TF_CLASS_HEAVYWEAPONS:
			case TF_CLASS_ENGINEER:
				/* no normal response; unused response(s) exist; so 100% chance to use the unused ones */
				iConcept = MP_CONCEPT_MVM_ENCOURAGE_MONEY;
				break;
			case TF_CLASS_SOLDIER:
				/* 2 normal responses; 2 unused responses; so 50% chance to use the unused ones */
				if (RandomInt(0, 3) < 2) {
					iConcept = MP_CONCEPT_MVM_ENCOURAGE_MONEY;
				}
				break;
			case TF_CLASS_MEDIC:
				/* 1 normal response; 3 unused responses; so 75% chance to use the unused ones */
				if (RandomInt(0, 3) < 3) {
					iConcept = MP_CONCEPT_MVM_ENCOURAGE_MONEY;
				}
				break;
			}
		}
		
		return DETOUR_MEMBER_CALL(iConcept, modifiers, pszOutResponseChosen, bufsize, filter);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Sound:Unused_Credits")
		{
			MOD_ADD_DETOUR_MEMBER(CCurrencyPack_MyTouch,           "CCurrencyPack::MyTouch");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_SpeakConceptIfAllowed, "CTFPlayer::SpeakConceptIfAllowed");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_sound_unused_credits", "0", FCVAR_NOTIFY,
		"Mod: enable unused TLK_MVM_ENCOURAGE_MONEY voice response when picking up credits",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
