#include "mod.h"
#include "prop.h"


// the nemesis particles don't show up on the client side for some reason
// from investigating, C_TFPlayer::ShouldShowNemesisIcon returns false
// and the specific part that fails in there is the call to CTFPlayerShared::IsPlayerDominated


namespace Mod::MvM::Dominations
{
	constexpr uint8_t s_Buf[] = {
		0x80, 0xbe, 0x72, 0x09, 0x00, 0x00, 0x00, // +0000  cmp byte ptr [esi+m_bPlayingMannVsMachine],0x0
		0x89, 0xc2,                               // +0007  mov edx,eax
		0x75, 0xcc,                               // +0009  jnz -0x34
	};
	
	struct CPatch_CTFGameRules_CalcDominationAndRevenge : public CPatch
	{
		CPatch_CTFGameRules_CalcDominationAndRevenge() : CPatch(sizeof(s_Buf)) {}
		
		virtual const char *GetFuncName() const override { return "CTFGameRules::CalcDominationAndRevenge [clone]"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0100; } // @ 0x006c
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf);
			
			int off_CTFGameRules_m_bPlayingMannVsMachine;
			if (!Prop::FindOffset(off_CTFGameRules_m_bPlayingMannVsMachine, "CTFGameRules", "m_bPlayingMannVsMachine")) return false;
			
			buf.SetDword(0x00 + 2, (uint32_t)off_CTFGameRules_m_bPlayingMannVsMachine);
			
			mask[0x09 + 1] = 0x00;
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* NOP out the conditional jump for MvM mode */
			buf .SetRange(0x09, 2, 0x90);
			mask.SetRange(0x09, 2, 0xff);
			
			return true;
		}
	};
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Dominations")
		{
			this->AddPatch(new CPatch_CTFGameRules_CalcDominationAndRevenge());
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_dominations", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: enable domination and revenge in MvM mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

