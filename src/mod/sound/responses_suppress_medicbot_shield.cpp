#include "mod.h"
#include "stub/tfplayer.h"


namespace Mod::Sound::Suppress_MedicBot_Shield
{
	DETOUR_DECL_MEMBER(bool, CTFPlayer_SpeakConceptIfAllowed, int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (iConcept == MP_CONCEPT_MEDIC_HEAL_SHIELD && player != nullptr && player->IsBot()) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL(iConcept, modifiers, pszOutResponseChosen, bufsize, filter);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Sound:Suppress_MedicBot_Shield")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_SpeakConceptIfAllowed, "CTFPlayer::SpeakConceptIfAllowed");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_sound_suppress_medicbot_shield", "0", FCVAR_NOTIFY,
		"Mod: suppress TLK_MEDIC_HEAL_SHIELD voice response when a bot activates the medigun shield",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
