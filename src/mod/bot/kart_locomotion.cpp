#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::Bot::Kart_Locomotion
{
	/* don't do dodge strafing */
	DETOUR_DECL_MEMBER(void, CTFBotMainAction_Dodge, CTFBot *actor)
	{
		if (actor->m_Shared->InCond(TF_COND_HALLOWEEN_KART)) {
			return;
		}
		
		DETOUR_MEMBER_CALL(CTFBotMainAction_Dodge)(actor);
	}
	
	/* don't strafe and jump around when stuck */
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotMainAction_OnStuck, CTFBot *actor)
	{
		if (actor->m_Shared->InCond(TF_COND_HALLOWEEN_KART)) {
			return EventDesiredResult<CTFBot>::Continue();
		}
		
		return DETOUR_MEMBER_CALL(CTFBotMainAction_OnStuck)(actor);
	}
	
	
	/* rework how bots do WASD */
	DETOUR_DECL_MEMBER(void, PlayerLocomotion_Approach, const Vector& dst, float f1)
	{
		auto loco = reinterpret_cast<ILocomotion *>(this);
		CTFBot *bot = ToTFBot(loco->GetBot()->GetEntity());
		
		DETOUR_MEMBER_CALL(PlayerLocomotion_Approach)(dst, f1);
		
		if (bot != nullptr && bot->m_Shared->InCond(TF_COND_HALLOWEEN_KART)) {
			bot->ReleaseForwardButton();
			bot->ReleaseBackwardButton();
			bot->ReleaseLeftButton();
			bot->ReleaseRightButton();
			
			Vector dir = (dst - bot->GetAbsOrigin());
			dir.z = 0.0f;
			dir.NormalizeInPlace();
			
			Vector eye; bot->EyeVectors(&eye);
			eye.z = 0.0f;
			eye.NormalizeInPlace();
			
			Vector eye_90 = Vector(eye.y, -eye.x, 0.0f);
			
			float fwd  = dir.Dot(eye);
			float side = dir.Dot(eye_90);
			
			if (fwd > 0.50f) {
				bot->PressForwardButton();
			}/* else if (fwd < -0.50f) {
				bot->PressBackwardButton();
			}*/
			
			const float cos_3deg = std::cos(DEG2RAD(3.0f));
			
			if (fwd > 0.0f && fwd < cos_3deg) {
				if (side > 0.0f) {
					bot->PressRightButton();
				} else {
					bot->PressLeftButton();
				}
			} else if (fwd > -cos_3deg) {
				if (side > 0.0f) {
					bot->PressLeftButton();
				} else {
					bot->PressRightButton();
				}
			}
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:Kart_Locomotion")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Dodge,   "CTFBotMainAction::Dodge");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_OnStuck, "CTFBotMainAction::OnStuck");
			
			MOD_ADD_DETOUR_MEMBER(PlayerLocomotion_Approach, "PlayerLocomotion::Approach");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_kart_locomotion", "0", FCVAR_NOTIFY,
		"Mod: make bots do delta-yaw movement instead of strafe movement when in TF_COND_HALLOWEEN_KART",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
