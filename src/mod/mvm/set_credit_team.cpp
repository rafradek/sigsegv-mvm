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
		0x8b, 0x06,                         // +0000  mov eax,[esi]
		0x89, 0x34, 0x24,                   // +0002  mov [esp],esi
		0xff, 0x90, 0x00, 0x00, 0x00, 0x00, // +0005  call dword ptr [eax+VToff(CBasePlayer::IsBot)]
	};
	
	struct CPatch_CCurrencyPack_MyTouch : public CPatch
	{
		CPatch_CCurrencyPack_MyTouch() : CPatch(sizeof(s_Buf_CCurrencyPack_MyTouch)) {}
		
		virtual const char *GetFuncName() const override { return "CCurrencyPack::MyTouch"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0070; } // @ +0x0052
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CCurrencyPack_MyTouch);
			
			static VTOffFinder vtoff_CBasePlayer_IsBot(TypeName<CBasePlayer>(), "CBasePlayer::IsBot");
			if (!vtoff_CBasePlayer_IsBot.IsValid()) return false;
			
			buf.SetDword(0x05 + 2, vtoff_CBasePlayer_IsBot);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* overwrite the call with 'xor eax, eax; nop; nop; nop; nop' */
			buf[0x05 + 0] = 0x31;
			buf[0x05 + 1] = 0xc0;
			buf[0x05 + 2] = 0x90;
			buf[0x05 + 3] = 0x90;
			buf[0x05 + 4] = 0x90;
			buf[0x05 + 5] = 0x90;
			
			mask.SetRange(0x05, 6, 0xff);
			
			return true;
		}
	};
	
	
	constexpr uint8_t s_Buf_CTFPowerup_ValidTouch[] = {
		0x89, 0x34, 0x24,             // +0000  mov [esp],esi
		0xe8, 0x00, 0x00, 0x00, 0x00, // +0003  call CBaseEntity::GetTeamNumber
		0x83, 0xf8, 0x03,             // +0008  cmp eax,TF_TEAM_BLUE
		0x0f, 0x84,                   // +000B  jz -0xXXXXXXXX
	};
	
	struct CPatch_CTFPowerup_ValidTouch : public CPatch
	{
		CPatch_CTFPowerup_ValidTouch() : CPatch(sizeof(s_Buf_CTFPowerup_ValidTouch)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPowerup::ValidTouch"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x00c0; } // @ +0x00a8
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFPowerup_ValidTouch);
			
			mask.SetDword(0x03 + 1, 0x00000000);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the comparison operand with TEAM_INVALID */
			buf[0x08 + 2] = (uint8_t)TEAM_INVALID;
			
			/* reverse the jump condition from 'jz' to 'jnz' */
			buf[0x0b + 1] = 0x85;
			
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
		0x8b, 0x87, 0x00, 0x00, 0x00, 0x00, // +0000  mov exx,[exx+m_pOuter]
		0x89, 0x04, 0x24,                   // +0006  mov [esp],exx
		0xe8, 0x00, 0x00, 0x00, 0x00,       // +0009  call CBaseEntity::GetTeamNumber
		0x83, 0xf8, 0x02,                   // +000E  cmp eax,TF_TEAM_RED
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
			
			/* allow any 3-bit source register code */
			mask[0x06 + 1] = 0b11000111;
			
			mask.SetDword(0x09 + 1, 0x00000000);
			
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
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +0000  mov dword ptr [esp+0xc],false
		0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // +0008  mov dword ptr [esp+0x8],false
		0xc7, 0x44, 0x24, 0x04, 0x02, 0x00, 0x00, 0x00, // +0010  mov dword ptr [esp+0x4],TF_TEAM_RED
		0x89, 0x04, 0x24,                               // +0018  mov [esp],exx
		0x89, 0x45, 0xc0,                               // +001B  mov [ebp-0xXX],exx
		0xe8,                                           // +001E  call CollectPlayers<CTFPlayer>
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
			
			/* allow any 3-bit source register code */
			mask[0x18 + 1] = 0b11000111;
			mask[0x1b + 1] = 0b11000111;
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the teamnum argument with TEAM_INVALID */
			buf.SetDword(0x10 + 4, TEAM_INVALID);
			
			mask.SetDword(0x10 + 4, 0xffffffff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			/* set the teamnum argument to the actual user-requested teamnum */
			buf.SetDword(0x10 + 4, GetCreditTeamNum());
			
			return true;
		}
	};
	
	
	constexpr uint8_t s_Buf_CPopulationManager_OnCurrencyCollected[] = {
		0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +0000  mov dword ptr [esp+0xc],false
		0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // +0008  mov dword ptr [esp+0x8],false
		0xc7, 0x44, 0x24, 0x04, 0x02, 0x00, 0x00, 0x00, // +0010  mov dword ptr [esp+0x4],TF_TEAM_RED
		0x89, 0x04, 0x24,                               // +0018  mov [esp],exx
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +001B  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0022  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0029  mov [ebp-0xXXX],0x00000000
		0xc7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00,       // +0030  mov [ebp-0xXXX],0x00000000
		0xe8,                                           // +0037  call CollectPlayers<CTFPlayer>
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
			
			/* allow any 3-bit source register code */
			mask[0x18 + 1] = 0b11000111;
			
			mask.SetDword(0x1b + 2, 0xfffff003);
			mask.SetDword(0x22 + 2, 0xfffff003);
			mask.SetDword(0x29 + 2, 0xfffff003);
			mask.SetDword(0x30 + 2, 0xfffff003);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* for now, replace the teamnum argument with TEAM_INVALID */
			buf.SetDword(0x10 + 4, TEAM_INVALID);
			
			mask.SetDword(0x10 + 4, 0xffffffff);
			
			return true;
		}
		
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			/* set the teamnum argument to the actual user-requested teamnum */
			buf.SetDword(0x10 + 4, GetCreditTeamNum());
			
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
