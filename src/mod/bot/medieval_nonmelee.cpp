#include "mod.h"
#include "prop.h"
#include "stub/gamerules.h"
#include "stub/econ.h"
#include "stub/tfplayer.h"

namespace Mod::Bot::Medieval_NonMelee
{
	constexpr uint8_t s_Buf[] = {
#ifdef PLATFORM_64BITS
		0x48, 0x8d, 0x05, 0x10, 0xe2, 0xb2, 0x00,  // +0x0000 lea     rax, g_pGameRules
		0x48, 0x8b, 0x00,                          // +0x0007 mov     rax, [rax]
		0x0f, 0xb6, 0x80, 0x1b, 0x0d, 0x00, 0x00,  // +0x000a movzx   eax, byte ptr [rax+0D1Bh]
		0x84, 0xc0,                                // +0x0011 test    al, al
		0x75, 0x2a,                                // +0x0013 jnz     short loc_1038FF0
#else
		0xa1, 0x00, 0x00, 0x00, 0x00,             // +0000  mov eax,[g_pGameRules]
		0x0f, 0xb6, 0x80, 0x00, 0x00, 0x00, 0x00, // +0005  movzx eax,byte ptr [eax+m_bPlayingMedieval]
		0x84, 0xc0,                               // +000C  test al,al
		0x75, 0x00,                               // +000E  jnz +0xXX
#endif
	};
	
	struct CPatch_CTFBot_EquipRequiredWeapon : public CPatch
	{
		CPatch_CTFBot_EquipRequiredWeapon() : CPatch(sizeof(s_Buf)) {}
		
		virtual const char *GetFuncName() const override { return "CTFBot::EquipRequiredWeapon"; }
		
#if defined _LINUX
		virtual uint32_t GetFuncOffMin() const override { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override { return 0x0120; } // @ 0x00b0
#elif defined _WINDOWS
		virtual uint32_t GetFuncOffMin() const override { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override { return 0x00c0; } // @ 0x0071
#endif
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf);
			
			int off_CTFGameRules_m_bPlayingMedieval;

			if (!Prop::FindOffset(off_CTFGameRules_m_bPlayingMedieval, "CTFGameRules", "m_bPlayingMedieval")) return false;
			
#ifdef PLATFORM_64BITS
			buf.SetDword(0x0a + 3, (uint32_t)off_CTFGameRules_m_bPlayingMedieval);
			
			mask.SetRange(0x00 + 3, 4, 0x00);
			mask.SetRange(0x03 + 1, 4, 0x00);
			mask.SetRange(0x13 + 1, 1, 0x00);
#else
			buf.SetDword(0x00 + 1, (uint32_t)&g_pGameRules.GetRef());
			buf.SetDword(0x05 + 3, (uint32_t)off_CTFGameRules_m_bPlayingMedieval);
			
			mask.SetRange(0x0e + 1, 1, 0x00);
#endif			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* NOP out the conditional jump for Medieval mode */
#ifdef PLATFORM_64BITS
			buf .SetRange(0x13, 2, 0x90);
			mask.SetRange(0x13, 2, 0xff);
#else
			buf .SetRange(0x0e, 2, 0x90);
			mask.SetRange(0x0e, 2, 0xff);
#endif
			return true;
		}
	};
	
	DETOUR_DECL_MEMBER(void, CTFBot_OnEventChangeAttributes, void *ecattr)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		bool medieval = TFGameRules()->IsInMedievalMode();

		if (TFGameRules()->IsMannVsMachineMode() && player->IsBot()) {
			TFGameRules()->Set_m_bPlayingMedieval(false);
		}

		DETOUR_MEMBER_CALL(ecattr);
		
		if (TFGameRules()->IsMannVsMachineMode() && player->IsBot()) {
			TFGameRules()->Set_m_bPlayingMedieval(medieval);
		}
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:Medieval_NonMelee")
		{
			this->AddPatch(new CPatch_CTFBot_EquipRequiredWeapon());
			MOD_ADD_DETOUR_MEMBER(CTFBot_OnEventChangeAttributes,                       "CTFBot::OnEventChangeAttributes");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_medieval_nonmelee", "0", FCVAR_NOTIFY,
		"Mod: allow bots in Medieval Mode to use weapons in non-melee slots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
