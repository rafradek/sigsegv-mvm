#include "mod.h"
#include "util/scope.h"


namespace Mod::Sound::Suppress_Giant_SentryBuster
{
	RefCount rc_CMissionPopulator_UpdateMissionDestroySentries;
	DETOUR_DECL_MEMBER(bool, CMissionPopulator_UpdateMissionDestroySentries)
	{
		SCOPED_INCREMENT(rc_CMissionPopulator_UpdateMissionDestroySentries);
		return DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_SpeakConceptIfAllowed, int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
	{
		if (iConcept == MP_CONCEPT_MVM_GIANT_CALLOUT && rc_CMissionPopulator_UpdateMissionDestroySentries > 0) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL(iConcept, modifiers, pszOutResponseChosen, bufsize, filter);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Sound:Suppress_Giant_SentryBuster")
		{
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_UpdateMissionDestroySentries, "CMissionPopulator::UpdateMissionDestroySentries");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_SpeakConceptIfAllowed,                "CTFPlayer::SpeakConceptIfAllowed");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_sound_suppress_giant_sentrybuster", "0", FCVAR_NOTIFY,
		"Mod: suppress TLK_MVM_GIANT_CALLOUT voice response when a sentry buster spawns",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
