#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/tfweaponbase.h"


namespace Mod::MvM::Human_Death_Yells
{
	constexpr uint8_t s_Buf_CTFPlayer_DeathSound[] = {
		0x85, 0xc0,                               // +0000  test eax,eax
		0x74, 0x00,                               // +0002  jz +0xXX
		
		0x80, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, // +0004  cmp byte ptr [eax+CTFGameRules::m_bPlayingMannVsMachine],0
		0x0f, 0x85, 0x00, 0x00, 0x00, 0x00,       // +000B  jnz +0xXXXXXXXX
		
		0x8b, 0x83, 0x00, 0x00, 0x00, 0x00,       // +0011  mov eax,[ebx+CTFPlayer::m_LastDamageType]
		
		0xa8, 0x20,                               // +0017  test al,DMG_FALL
		0x0F, 0x85, 0x8C, 0x00, 0x00, 0x00,       // +0019  jnz +0xXX
		
		0xa8, 0x40,                               // +001F  test al,DMG_BLAST
		0x0f, 0x85, 0x00, 0x00, 0x00, 0x00,       // +0021  jnz +0xXXXXXXXX
		
		0xa9, 0x00, 0x00, 0x10, 0x00,             // +0027  test eax,DMG_CRITICAL
		0x0f, 0x85, 0x00, 0x00, 0x00, 0x00,       // +002C  jnz +0xXXXXXXXX
		
		0xa8, 0x80,                               // +0032  test al,DMG_CLUB
		0x74, 0x00,                               // +0034  jz +0xXX
	};
	
	struct CPatch_CTFPlayer_DeathSound : public CPatch
	{
		CPatch_CTFPlayer_DeathSound() : CPatch(sizeof(s_Buf_CTFPlayer_DeathSound)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayer::DeathSound"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0300; } // ServerLinux 20170315a: +0x0089
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFPlayer_DeathSound);
			
			int off__m_bPlayingMannVsMachine;
			if (!Prop::FindOffset(off__m_bPlayingMannVsMachine, "CTFGameRules", "m_bPlayingMannVsMachine")) return false;
			
			buf.SetDword(0x04 + 2, (uint32_t)off__m_bPlayingMannVsMachine);
			
			mask.SetRange(0x02 + 1, 1, 0x00);
			mask.SetRange(0x0b + 2, 4, 0x00);
			mask.SetRange(0x11 + 2, 4, 0x00); // hypothetically we could get the offset for CTFPlayer::m_LastDamageType
			mask.SetRange(0x19 + 2, 4, 0x00);
			mask.SetRange(0x21 + 2, 4, 0x00);
			mask.SetRange(0x27 + 2, 4, 0x00);
			mask.SetRange(0x2c + 2, 4, 0x00);
			mask.SetRange(0x34 + 1, 1, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* always skip over the block for EmitSound("MVM.PlayerDied") */
			buf .SetRange(0x0b, 6, 0x90);
			mask.SetRange(0x0b, 6, 0xff);
			
			return true;
		}
	};
	
	RefCount rc_CTFPlayer_SpyDeadRingerDeath;
	DETOUR_DECL_MEMBER(void, CTFPlayer_SpyDeadRingerDeath, const CTakeDamageInfo& info)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_SpyDeadRingerDeath);
		DETOUR_MEMBER_CALL(CTFPlayer_SpyDeadRingerDeath)(info);
	
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_DeathSound, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DETOUR_MEMBER_CALL(CTFPlayer_DeathSound)(info);
		
		/* these checks are essentially in reverse order */
		if (TFGameRules()->IsMannVsMachineMode() && player->GetTeamNumber() != TF_TEAM_BLUE && !rc_CTFPlayer_SpyDeadRingerDeath) {
			CTFPlayer *attacker = ToTFPlayer(info.GetAttacker());
			if (attacker != nullptr) {
				CTFWeaponBase *weapon = attacker->GetActiveTFWeapon();
				if (weapon == nullptr || !weapon->IsSilentKiller()) {
					player->EmitSound("MVM.PlayerDied");
				}
			}
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Human_Death_Yells")
		{
			/* we need to patch out the block that plays "MVM.PlayerDied"
			 * (because it unconditionally returns and skips the other logic),
			 * and then we need to use a detour to effectively add it back in */
			
			this->AddPatch(new CPatch_CTFPlayer_DeathSound());
			
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFPlayer_DeathSound, "CTFPlayer::DeathSound", LOWEST);
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_SpyDeadRingerDeath, "CTFPlayer::SpyDeadRingerDeath");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_human_death_yells", "0", FCVAR_NOTIFY,
		"Mod: re-enable human death yells and crunching sounds and so forth in MvM mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
