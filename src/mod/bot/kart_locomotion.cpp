#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::Bot::Kart_Locomotion
{
	struct NextBotData
    {
        void *vtable;
        INextBotComponent *m_ComponentList;              // +0x04
        PathFollower *m_CurrentPath;                     // +0x08
        int m_iManagerIndex;                             // +0x0c
        bool m_bScheduledForNextTick;                    // +0x10
        int m_iLastUpdateTick;                           // +0x14
        int m_Dword18;                                   // +0x18 (reset to 0 in INextBot::Reset)
        int m_iDebugTextOffset;                          // +0x1c
        Vector m_vecLastPosition;                        // +0x20
        CountdownTimer m_ctImmobileCheck;                // +0x2c
        IntervalTimer m_itImmobileEpoch;                 // +0x38
        ILocomotion *m_LocoInterface;                    // +0x3c
        IBody *m_BodyInterface;                          // +0x40
        IIntention *m_IntentionInterface;                // +0x44
        IVision *m_VisionInterface;                      // +0x48
        CUtlVector<void *> m_DebugLines; // +0x4c
        int off;
        int inputButtons;
        int prevInputButtons;

    };

	/* don't do dodge strafing */
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CTFBotMainAction_Dodge)
	{
		auto actor = reinterpret_cast<CTFPlayer *>(this);
		if (actor->m_Shared->InCond(TF_COND_HALLOWEEN_KART) || actor->m_Shared->InCond(TF_COND_TAUNTING) || actor->m_hVehicle != nullptr) {
			return;
		}
		
		DETOUR_MEMBER_CALL();
	}
	
	/* don't strafe and jump around when stuck */
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotMainAction_OnStuck, CTFBot *actor)
	{
		if (actor->m_Shared->InCond(TF_COND_HALLOWEEN_KART) || actor->m_Shared->InCond(TF_COND_TAUNTING) || actor->m_hVehicle != nullptr) {
			return EventDesiredResult<CTFBot>::Continue();
		}
		
		return DETOUR_MEMBER_CALL(actor);
	}
	
	
	/* rework how bots do WASD */
	DETOUR_DECL_MEMBER(void, PlayerLocomotion_Approach, const Vector& dst, float f1)
	{
		auto loco = reinterpret_cast<ILocomotion *>(this);
		CTFBot *bot = ToTFBot(loco->GetBot()->GetEntity());
		
		DETOUR_MEMBER_CALL(dst, f1);
		
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

		if (bot != nullptr && bot->m_hVehicle != nullptr ) {

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
			
			if (fwd > 0.0f) {
				bot->PressForwardButton();
			} else {// if (fwd < -0.50f) {
				bot->PressBackwardButton();
			}
			
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
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Dodge,   "CTFBotMainAction::Dodge [clone]");
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
