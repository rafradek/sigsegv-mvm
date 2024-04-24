#include "mod.h"
#include "stub/baseplayer.h"


namespace Mod::MvM::Set_Credit_Team
{
	int GetCreditTeamNum()
	{
		extern ConVar cvar_enable;
		return cvar_enable.GetInt();
	}
	
	
	constexpr uint8_t s_Buf_CCurrencyPack_MyTouch[] = {
		0xff, 0x90, 0x4c, 0x01, 0x00, 0x00,  // +0x0001 call    dword ptr [eax+14Ch]
	};
	
	struct CPatch_CCurrencyPack_MyTouch : public CPatch
	{
		CPatch_CCurrencyPack_MyTouch() : CPatch(sizeof(s_Buf_CCurrencyPack_MyTouch)) {}
		
		virtual const char *GetFuncName() const override { return "CCurrencyPack::MyTouch"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0200; } // @ +0x0052
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CCurrencyPack_MyTouch);
			
			static VTOffFinder vtoff_CBasePlayer_IsBot(TypeName<CBasePlayer>(), "CBasePlayer::IsBot", 1);
			if (!vtoff_CBasePlayer_IsBot.IsValid()) return false;
			
			buf.SetDword(0x00 + 2, vtoff_CBasePlayer_IsBot);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* overwrite the call with 'xor eax, eax; nop; nop; nop; nop' */
			buf[0x00 + 0] = 0x31;
			buf[0x00 + 1] = 0xc0;
			buf[0x00 + 2] = 0x90;
			buf[0x00 + 3] = 0x90;
			buf[0x00 + 4] = 0x90;
			buf[0x00 + 5] = 0x90;
			
			mask.SetRange(0x00, 6, 0xff);
			
			return true;
		}
	};
	
	
	constexpr uint8_t s_Buf_CTFPowerup_ValidTouch[] = {
		0xe8, 0xff, 0x9d, 0xb8, 0xff,  // +0x0000 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
		0x83, 0xc4, 0x10,              // +0x0005 add     esp, 10h
		0x83, 0xf8, 0x03,              // +0x0008 cmp     eax, 3
		0x0f, 0x95, 0xc0,              // +0x000b setnz   al
	};
	
	struct CPatch_CTFPowerup_ValidTouch : public CPatch
	{
		CPatch_CTFPowerup_ValidTouch() : CPatch(sizeof(s_Buf_CTFPowerup_ValidTouch)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPowerup::ValidTouch"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x00d0; } // @ +0x00a8
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFPowerup_ValidTouch);
			
			mask.SetDword(0x00 + 1, 0x00000000);
			mask.SetRange(0x05 + 2, 1, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the comparison operand with TEAM_INVALID */
			buf[0x08 + 2] = (uint8_t)TEAM_INVALID;
			
			/* reverse setnz to setz*/
			buf[0x0b + 1] = 0x94;
			
			mask[0x08 + 2] = 0xff;
			mask[0x0b + 1] = 0xff;
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			/* set the comparison operand to the actual user-requested teamnum */
			buf[0x08 + 2] = (uint8_t)GetCreditTeamNum();
			
			return true;
		}
	};
	
	
	constexpr uint8_t s_Buf_RadiusCurrencyCollectionCheck[] = {
		0xff, 0xb0, 0x8c, 0x01, 0x00, 0x00,  // +0x0000 push    dword ptr [eax+18Ch]
		0xe8, 0xb9, 0x2d, 0x21, 0x00,        // +0x0006 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
		0x83, 0xc4, 0x10,                    // +0x000b add     esp, 10h
		0x83, 0xf8, 0x02,                    // +0x000e cmp     eax, 2
	};
	
	struct CPatch_RadiusCurrencyCollectionCheck : public CPatch
	{
		CPatch_RadiusCurrencyCollectionCheck() : CPatch(sizeof(s_Buf_RadiusCurrencyCollectionCheck)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayerShared::RadiusCurrencyCollectionCheck"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0020; } // @ +0x000c
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_RadiusCurrencyCollectionCheck);
			
			int off_CTFPlayerShared_m_pOuter;
			if (!Prop::FindOffset(off_CTFPlayerShared_m_pOuter, "CTFPlayerShared", "m_pOuter")) return false;
			buf.SetDword(0x00 + 2, off_CTFPlayerShared_m_pOuter);
			
			/* allow any 3-bit source or destination register code */
			mask[0x00 + 1] = 0b11000000;
			
			mask.SetDword(0x06 + 1, 0x00000000);
			mask.SetRange(0x0b + 2, 1 , 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the comparison operand with TEAM_INVALID */
			buf[0x0e + 2] = (uint8_t)TEAM_INVALID;
			
			mask[0x0e + 2] = 0xff;
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			/* set the comparison operand to the actual user-requested teamnum */
			buf[0x0e + 2] = (uint8_t)GetCreditTeamNum();
			
			return true;
		}
	};
	
	
	constexpr uint8_t s_Buf_CTFGameRules_DistributeCurrencyAmount[] = {
		0x6a, 0x00,                    // +0x0000 push    0
		0x6a, 0x00,                    // +0x0002 push    0
		0x6a, 0x02,                    // +0x0004 push    2
		0x50,                          // +0x0006 push    eax
		0xe8,  // +0x0007 call    _Z14CollectPlayersI9CTFPlayerEiP10CUtlVectorIPT_10CUtlMemoryIS3_iEEibb; CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *,CUtlMemory<CTFPlayer *,int>> *,int,bool,bool)
	};
	
	struct CPatch_CTFGameRules_DistributeCurrencyAmount : public CPatch
	{
		CPatch_CTFGameRules_DistributeCurrencyAmount() : CPatch(sizeof(s_Buf_CTFGameRules_DistributeCurrencyAmount)) {}
		
		virtual const char *GetFuncName() const override { return "CTFGameRules::DistributeCurrencyAmount"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0100; } // @ +0x00d3
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFGameRules_DistributeCurrencyAmount);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the teamnum argument with TEAM_INVALID */
			buf[0x04 + 1] = TEAM_INVALID;
			
			mask[0x04 + 1] = 0xFF;
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			/* set the teamnum argument to the actual user-requested teamnum */
			buf[0x04 + 1] = GetCreditTeamNum();
			
			return true;
		}
	};
	
	
	constexpr uint8_t s_Buf_CPopulationManager_OnCurrencyCollected[] = {
		0xba, 0x02, 0x00, 0x00, 0x00,              // +0x0000 mov     edx, 2
		0x89, 0xf8,                                // +0x0005 mov     eax, edi
	};
	
	struct CPatch_CPopulationManager_OnCurrencyCollected : public CPatch
	{
		CPatch_CPopulationManager_OnCurrencyCollected() : CPatch(sizeof(s_Buf_CPopulationManager_OnCurrencyCollected)) {}
		
		virtual const char *GetFuncName() const override { return "CPopulationManager::OnCurrencyCollected"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0200; } // @ +0x00af
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CPopulationManager_OnCurrencyCollected);
			
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the teamnum argument with TEAM_INVALID */
			buf.SetDword(0x00 + 1, TEAM_INVALID);
			
			mask.SetDword(0x0 + 1, 0xffffffff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			/* set the teamnum argument to the actual user-requested teamnum */
			buf.SetDword(0x00 + 1, TEAM_INVALID);
			
			return true;
		}
	};
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Set_Credit_Team")
		{
			/* allow bots to touch currency packs */
			this->AddPatch(new CPatch_CCurrencyPack_MyTouch());
			
			/* set who can touch currency packs */
			this->AddPatch(new CPatch_CTFPowerup_ValidTouch());
			
			/* set who can do radius currency collection */
			this->AddPatch(new CPatch_RadiusCurrencyCollectionCheck());
			
			/* set which team receives credits when they're collected or given out */
			this->AddPatch(new CPatch_CTFGameRules_DistributeCurrencyAmount());
			
			/* set whose respec information gets updated when credits are collected */
			this->AddPatch(new CPatch_CPopulationManager_OnCurrencyCollected());
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_set_credit_team", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: change which team is allowed to collect MvM credits (normally hardcoded to TF_TEAM_RED)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			auto var = static_cast<ConVar *>(pConVar);
			
			/* refresh patches with the new convar value if we do a nonzero --> nonzero transition */
			if (s_Mod.IsEnabled() && var->GetBool()) {
				// REMOVE ME
				ConColorMsg(Color(0xff, 0x00, 0xff, 0xff),
					"sig_mvm_set_credit_team: toggling patches off and back on, for %s --> %s transition.\n",
					pOldValue, var->GetString());
				
				s_Mod.ToggleAllPatches(false);
				s_Mod.ToggleAllPatches(true);
			}
			
			s_Mod.Toggle(var->GetBool());
		});
}
