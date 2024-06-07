#include "mod.h"
#include "re/nextbot.h"
#include "stub/tfplayer.h"
#include "stub/tfbot.h"

namespace Mod::Bot::RunFast
{
	constexpr uint8_t s_Buf_Verify[] = {
#ifdef PLATFORM_64BITS
		0xe8, 0x1e, 0x11, 0xfe, 0xff,                    // +0x0000 call    _ZN4Path7ComputeI14CTFBotPathCostEEbP8INextBotRK6VectorRT_fb; Path::Compute<CTFBotPathCost>(INextBot *,Vector const&,CTFBotPathCost &,float,bool)
		0xf3, 0x0f, 0x10, 0x0d, 0xb2, 0x73, 0x48, 0x00,  // +0x0005 movss   xmm1, cs:dword_148A3DC
		0xf3, 0x0f, 0x10, 0x05, 0xb6, 0x73, 0x48, 0x00,  // +0x000d movss   xmm0, cs:dword_148A3E8
		0xe8, 0x19, 0x44, 0x6c, 0xff,                    // +0x0015 call    _RandomFloat
#else
		0xE8, 0xFA, 0x16, 0xFE, 0xFF, // +0000  call Path::Compute<CTFBotPathCost>
		0x83, 0xC4, 0x18,             // +0005  add esp, 18h
		0x68, 0x00, 0x00, 0x00, 0x40, // +0008  push 2.0f
		0x68, 0x00, 0x00, 0x80, 0x3F, // +000D  push 1.0f
		0xe8, 0xdc, 0xd8, 0xae, 0x00, // +0012  call RandomFloat
#endif
	};
	
	struct CPatch_CTFBotPushToCapturePoint_Update : public CPatch
	{
		CPatch_CTFBotPushToCapturePoint_Update() : CPatch(sizeof(s_Buf_Verify)) {}
		
		virtual const char *GetFuncName() const override { return "CTFBotPushToCapturePoint::Update"; }
		virtual uint32_t GetFuncOffMin() const override { return 0x0200; }
		virtual uint32_t GetFuncOffMax() const override { return 0x0680; } // @ 0x01d3
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_Verify);
			
#ifdef PLATFORM_64BITS
			mask.SetRange(0x00 + 1, 4, 0x00);
			mask.SetRange(0x05 + 4, 4, 0x00);
			mask.SetRange(0x0d + 4, 4, 0x00);
			mask.SetRange(0x15 + 1, 4, 0x00);
#else
			mask.SetRange(0x00 + 1, 4, 0x00);
			mask.SetRange(0x05 + 2, 1, 0x00);
			mask.SetRange(0x12 + 1, 4, 0x00);
#endif
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
#ifdef PLATFORM_64BITS
			buf.SetRange(0x15, 5, 0x90);
			mask.SetRange(0x15, 5, 0xff);
#else
			buf.SetRange(0x12, 5, 0x90);
			mask.SetRange(0x12, 5, 0xff);
#endif
			
			return true;
		}
	};
	
	struct NextBotData
    {
        void *vtable;
        INextBotComponent *m_ComponentList;              // +0x04
        PathFollower *m_CurrentPath;                     // +0x08
        int m_iManagerIndex;                             // +0x0c
        bool m_bScheduledForNextTick;                    // +0x10
        int m_iLastUpdateTick;                           // +0x14
    };
	
	DETOUR_DECL_MEMBER(float, CTFBotLocomotion_GetRunSpeed)
	{
		auto loco = reinterpret_cast<ILocomotion *>(this);
		CTFBot *bot = ToTFBot(loco->GetBot()->GetEntity());
		
		if (bot != nullptr) {
			return bot->MaxSpeed();
		} else {
			Warning("Can't determine actual max speed in detour for CTFBotLocomotion::GetRunSpeed!\n");
			return 3200.0f;
		}
	}

	ConVar cvar_fast_update("sig_bot_runfast_fast_update", "0", FCVAR_GAMEDLL,
		"Etc: player movement speed limit when overridden (use -1 for no limit)");

	RefCount rc_CTFBotMainAction_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_CTFBotMainAction_Update); 

		// Force update for fast bots
		if (cvar_fast_update.GetBool() && actor->MaxSpeed() >= (actor->IsMiniBoss() ? 350.0f : 520.0f) ) {
			reinterpret_cast<NextBotData *>(actor->MyNextBotPointer())->m_bScheduledForNextTick = true;
		}

		return DETOUR_MEMBER_CALL(CTFBotMainAction_Update)(actor, dt);
	}
	
	ConVar cvar_jump("sig_bot_runfast_allowjump", "0", FCVAR_GAMEDLL,
		"Etc: player movement speed limit when overridden (use -1 for no limit)");

	DETOUR_DECL_MEMBER(void, PlayerLocomotion_Jump)
	{
		/* don't let bots try to jump without autojump, it just slows them down */
		if ( rc_CTFBotMainAction_Update || cvar_jump.GetBool() || reinterpret_cast<ILocomotion *>(this)->GetRunSpeed() < 350.0f)
			DETOUR_MEMBER_CALL(PlayerLocomotion_Jump)();
	}
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:RunFast")
		{
			this->AddPatch(new CPatch_CTFBotPushToCapturePoint_Update());
			MOD_ADD_DETOUR_MEMBER(CTFBotLocomotion_GetRunSpeed, "CTFBotLocomotion::GetRunSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update,      "CTFBotMainAction::Update");
			MOD_ADD_DETOUR_MEMBER(PlayerLocomotion_Jump,        "PlayerLocomotion::Jump");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_runfast", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: make super-fast bot locomotion feasible",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_bot_runfast_config, "Auto-set a bunch of convars for better results")
	{
		static ConVarRef nb_update_frequency             ("nb_update_frequency");
		static ConVarRef nb_update_framelimit            ("nb_update_framelimit");
		static ConVarRef nb_update_maxslide              ("nb_update_maxslide");
		static ConVarRef nb_last_area_update_tolerance   ("nb_last_area_update_tolerance");
		static ConVarRef nb_player_move_direct           ("nb_player_move_direct");
		static ConVarRef nb_path_segment_influence_radius("nb_path_segment_influence_radius");
		static ConVarRef tf_bot_path_lookahead_range     ("tf_bot_path_lookahead_range");
		
		nb_update_frequency.SetValue(0.001f);
		nb_update_framelimit.SetValue(1000);
		nb_update_maxslide.SetValue(0);
		
		nb_last_area_update_tolerance.SetValue(1.0f);
		nb_player_move_direct.SetValue(1);
	//	nb_path_segment_influence_radius.SetValue(100);
		tf_bot_path_lookahead_range.SetValue(600);
	}
}
