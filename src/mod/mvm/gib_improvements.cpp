#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
#include "util/scope.h"


namespace Mod::MvM::Gib_Improvements
{
	constexpr uint8_t s_Buf[] = {
		0xa1, 0x5c, 0x6c, 0x70, 0x01,             // +0000  mov eax,ds:g_pGameRules
		0x80, 0xb8, 0x66, 0x09, 0x00, 0x00, 0x00, // +0005  cmp byte ptr [eax+m_bPlayingMannVsMachine],0x0
		0x0f, 0x85, 0x7e, 0x03, 0x00, 0x00        // +000C  jnz +0xXXXXXXXX
	};
	
	struct CPatch_CTFPlayer_ShouldGib : public CPatch
	{
		CPatch_CTFPlayer_ShouldGib() : CPatch(sizeof(s_Buf)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayer::ShouldGib"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0080; } // @ 0x0028
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf);
			
			void *addr__g_pGameRules = AddrManager::GetAddr("g_pGameRules");
			if (addr__g_pGameRules == nullptr) return false;
			
			int off__CTFGameRules_m_bPlayingMannVsMachine;
			if (!Prop::FindOffset(off__CTFGameRules_m_bPlayingMannVsMachine, "CTFGameRules", "m_bPlayingMannVsMachine")) return false;
			
			buf.SetDword(0x00 + 1, (uint32_t)addr__g_pGameRules);
			buf.SetDword(0x05 + 2, (uint32_t)off__CTFGameRules_m_bPlayingMannVsMachine);
			
			mask.SetRange(0x0c + 2, 4, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* NOP out the conditional jump for MvM mode */
			buf .SetRange(0x0c, 6, 0x90);
			mask.SetRange(0x0c, 6, 0xff);
			
			return true;
		}
	};
	
	
//	ConVar cvar_human_enable("sig_mvm_gib_improvements_human_enable", "0", FCVAR_NOTIFY,
//		"Mod: enable gibbing for human players in MvM mode");
//	
//	ConVar cvar_robot_enable("sig_mvm_gib_improvements_robot_enable", "0", FCVAR_NOTIFY,
//		"Mod: enable gibbing for non-giant robots in MvM mode");
//	
//	ConVar cvar_plasma_fix("sig_mvm_gib_improvements_plasma_fix", "0", FCVAR_NOTIFY,
//		"Mod: fix ragdoll/dissolve problems caused by weapons like Cow Mangler and medigun shield");
	
	
	bool ClassHasDecentGibs(const CTFPlayer *pPlayer)
	{
		switch (pPlayer->GetPlayerClass()->GetClassIndex()) {
		default:
			return true;
			
		case TF_CLASS_ENGINEER: // no gibs
		case TF_CLASS_MEDIC:    // head gib only
		case TF_CLASS_SNIPER:   // head gib only
		case TF_CLASS_SPY:      // head gib only
			return false;
		}
	}
	
	
	int eh_tick = -1;
	std::set<CHandle<CTFPlayer>> eh_victims;
	
	RefCount rc_CTFSniperRifle_ExplosiveHeadShot;
	DETOUR_DECL_MEMBER(void, CTFSniperRifle_ExplosiveHeadShot, CTFPlayer *pAttacker, CTFPlayer *pVictim)
	{
		SCOPED_INCREMENT(rc_CTFSniperRifle_ExplosiveHeadShot);
		DETOUR_MEMBER_CALL(CTFSniperRifle_ExplosiveHeadShot)(pAttacker, pVictim);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_StunPlayer, float duration, float slowdown, int flags, CTFPlayer *attacker)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		if (rc_CTFSniperRifle_ExplosiveHeadShot > 0) {
			if (eh_tick != gpGlobals->tickcount) {
				eh_victims.clear();
				eh_tick = gpGlobals->tickcount;
			}
			
			eh_victims.insert(shared->GetOuter());
		}
		
		DETOUR_MEMBER_CALL(CTFPlayerShared_StunPlayer)(duration, slowdown, flags, attacker);
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFBot_ShouldGib, const CTakeDamageInfo& info)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		// TODO: determine if the current model of the bot is actually a robot model or not
		// rather than just assuming that (MvM mode + TFBot = robot)
		if (TFGameRules()->IsMannVsMachineMode()) {
			/* giants: always gib */
			if (bot->IsMiniBoss() || bot->GetModelScale() > 1.0f) {
				return true;
			}
			
			/* commons: never gib if they lack a decent number of gibs */
			if (!ClassHasDecentGibs(bot)) {
				return false;
			}
		}
		
		/* can't use the vfunc thunk for this call because we specifically
		 * want to call the implementation in CTFPlayer */
		static auto p_CTFPlayer_ShouldGib = MakePtrToMemberFunc<CTFPlayer, bool, const CTakeDamageInfo&>(AddrManager::GetAddr("CTFPlayer::ShouldGib"));
		return (bot->*p_CTFPlayer_ShouldGib)(info);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ShouldGib, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		//Allow player miniboss to be gibbed
		if (player->IsMiniBoss())
			return true;
		/* explosive headshot gibbing :) */
		if (eh_tick == gpGlobals->tickcount && eh_victims.count(player) != 0) {
			return true;
		}
		bool ret = DETOUR_MEMBER_CALL(CTFPlayer_ShouldGib)(info);
		DevMsg("damage info %d %d\n", ret, info.GetDamageType());
		return ret;
	}
	
	
	// Problems:
	// 1. TF_CUSTOM_PLASMA (Cow Mangler and medic shield) turns giants into
	//    small ragdolls that don't do the dissolving effect
	// 2. TF_CUSTOM_PLASMA_CHARGED (Cow Mangler charged shot) makes robots gib,
	//    which looks silly for certain bot classes that only have 0-1 gibs
	// 
	// Solutions:
	// 1. For giants, change PLASMA -> PLASMA_CHARGED
	// 2. For classes missing gibs, change PLASMA_CHARGED -> PLASMA
	DETOUR_DECL_MEMBER(void, CTFPlayer_CreateRagdollEntity, bool bShouldGib, bool bBurning, bool bUberDrop, bool bOnGround, bool bYER, bool bGold, bool bIce, bool bAsh, int iCustom, bool bClassic)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		CTFBot *bot;
		if (TFGameRules()->IsMannVsMachineMode() && (bot = ToTFBot(player)) != nullptr) {
			if (bot->IsMiniBoss() || bot->GetModelScale() > 1.0f) {
				if (iCustom == TF_DMG_CUSTOM_PLASMA) {
					iCustom = TF_DMG_CUSTOM_PLASMA_CHARGED;
				}
			} else {
				if (iCustom == TF_DMG_CUSTOM_PLASMA_CHARGED && !ClassHasDecentGibs(bot)) {
					iCustom = TF_DMG_CUSTOM_PLASMA;
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_CreateRagdollEntity)(bShouldGib, bBurning, bUberDrop, bOnGround, bYER, bGold, bIce, bAsh, iCustom, bClassic);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Gib_Improvements")
		{
			this->AddPatch(new CPatch_CTFPlayer_ShouldGib());
			
			MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_ExplosiveHeadShot, "CTFSniperRifle::ExplosiveHeadShot");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_StunPlayer,       "CTFPlayerShared::StunPlayer");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_ShouldGib,    "CTFBot::ShouldGib");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldGib, "CTFPlayer::ShouldGib");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CreateRagdollEntity, "CTFPlayer::CreateRagdollEntity [args]");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_gib_improvements", "0", FCVAR_NOTIFY,
		"Mod: improve various aspects of human and robot gibbing in MvM mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
