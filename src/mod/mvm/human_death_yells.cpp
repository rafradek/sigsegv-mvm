#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/tfweaponbase.h"


namespace Mod::MvM::Human_Death_Yells
{
	constexpr uint8_t s_Buf_CTFPlayer_DeathSound[] = {
		0x80, 0xbb, 0x24, 0x24, 0x00, 0x00, 0x00,  // +0x0000 cmp     byte ptr [ebx+2424h], 0
		0x0f, 0x85, 0x0f, 0xff, 0xff, 0xff,        // +0x0007 jnz     loc_EF3B78
		0x6a, 0x00,                                // +0x000d push    0
		0x6a, 0x00,                                // +0x000f push    0
		0x68, 0xbe, 0xb1, 0x21, 0x01,              // +0x0011 push    121B1BEh; "MVM.PlayerDied"
		0x53,                                      // +0x0016 push    ebx
		0xe8, 0x18, 0x32, 0x8e, 0xff,              // +0x0017 call    _ZN11CBaseEntity9EmitSoundEPKcfPf; CBaseEntity::EmitSound(char const*,float,float *)
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

			mask.SetRange(0x00 + 2, 4, 0x00);
			mask.SetRange(0x07 + 2, 4, 0x00);
			mask.SetRange(0x11 + 1, 4, 0x00);
			mask.SetRange(0x17 + 1, 4, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			// Avoid going into the branch playing mvm sound

			/* make the conditional jump unconditional */
			buf[0x07] = 0x90;
			buf[0x08] = 0xe9;
			
			mask[0x07] = 0xFF;
			mask[0x08] = 0xFF;
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
