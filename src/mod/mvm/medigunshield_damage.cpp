#include "mod.h"
#include "util/scope.h"
#include "stub/tfentities.h"
#include "util/rtti.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "util/clientmsg.h"


namespace Mod::MvM::MedigunShield_Damage
{
	
	constexpr uint8_t s_Buf[] = {
		0x57,                                // +0x0000 push    edi
		0xe8, 0x87, 0x98, 0x16, 0x00,        // +0x0001 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
		0x83, 0xc4, 0x10,                    // +0x0006 add     esp, 10h
		0x83, 0xf8, 0x03,                    // +0x0009 cmp     eax, 3
		0x0f, 0x85, 0x85, 0xfd, 0xff, 0xff,  // +0x000c jnz     loc_95AFFA
		0xe9, 0x66, 0xff, 0xff, 0xff,        // +0x0012 jmp     loc_95B1E0
	};
	
	struct CPatch_CTFMedigunShield_ShieldTouch : public CPatch
	{
		CPatch_CTFMedigunShield_ShieldTouch() : CPatch(sizeof(s_Buf)) {}
		
		virtual const char *GetFuncName() const override { return "CTFMedigunShield::ShieldTouch"; }
		virtual uint32_t GetFuncOffMin() const override { return 0x02c0; }
		virtual uint32_t GetFuncOffMax() const override { return 0x0400; } // @ 0x2e0
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf);
			
			mask.SetRange(0x01 + 1, 4, 0x00);
			mask.SetRange(0x06 + 2, 1, 0x00);
			mask.SetRange(0x0c + 2, 4, 0x00);
			mask.SetRange(0x12 + 1, 4, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* make the conditional jump unconditional */
			buf.SetDword(0x0c + 0, 0x90);
			buf.SetDword(0x0c + 1, 0xe9);
			mask.SetRange(0x0c, 2, 0xff);
			
			return true;
		}
	};
	
	DETOUR_DECL_STATIC(CTFMedigunShield *, CTFMedigunShield_Create, CTFPlayer *player)
	{
		CTFMedigunShield *shield = DETOUR_STATIC_CALL(CTFMedigunShield_Create)(player);
		float damage = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, damage, mult_dmg_vs_players);
		if (damage > 0.0f) {
			shield->SetRenderColorR(255);
			shield->SetRenderColorG(0);
			shield->SetRenderColorB(255);
		}
		return shield;
	}
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:MedigunShield_Damage")
		{
			MOD_ADD_DETOUR_STATIC(CTFMedigunShield_Create,              "CTFMedigunShield::Create");
			this->AddPatch(new CPatch_CTFMedigunShield_ShieldTouch());
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_medigunshield_damage", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: enable damage and stun for robots' medigun shields",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
