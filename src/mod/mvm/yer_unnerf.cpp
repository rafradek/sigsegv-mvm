#include "mod.h"
#include "prop.h"
#include "stub/econ.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"

namespace Mod::MvM::YER_Unnerf
{
	constexpr uint8_t s_Buf[] = {
#ifdef PLATFORM_64BITS
		0x48, 0x8d, 0x05, 0xad, 0xf7, 0xf8, 0x00,  // +0x0000 lea     rax, g_pGameRules
		0x48, 0x8b, 0x00,                          // +0x0007 mov     rax, [rax]
		0x48, 0x85, 0xc0,                          // +0x000a test    rax, rax
		0x74, 0x0d,                                // +0x000d jz      short loc_BD7A30
		0x80, 0xb8, 0x1e, 0x0d, 0x00, 0x00, 0x00,  // +0x000f cmp     byte ptr [rax+0D1Eh], 0
		0x0f, 0x85, 0x6d, 0x01, 0x00, 0x00,        // +0x0016 jnz     loc_BD7B9D
#else
		0xa1, 0xfc, 0x53, 0x7f, 0x01,              // +0x0000 mov     eax, ds:g_pGameRules
		0x85, 0xc0,                                // +0x0005 test    eax, eax
		0x74, 0x0d,                                // +0x0007 jz      short loc_DBC173
		0x80, 0xb8, 0x72, 0x0c, 0x00, 0x00, 0x00,  // +0x0009 cmp     byte ptr [eax+0C72h], 0
		0x0f, 0x85, 0x26, 0x01, 0x00, 0x00,        // +0x0010 jnz     loc_DBC299
#endif
	};
	
	struct CPatch_CTFKnife_PrimaryAttack : public CPatch
	{
		CPatch_CTFKnife_PrimaryAttack() : CPatch(sizeof(s_Buf)) {}
		
		virtual const char *GetFuncName() const override { return "CTFKnife::PrimaryAttack"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0800; } // @ 0x0393
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf);
			
			int off__CTFGameRules_m_bPlayingMannVsMachine;
			if (!Prop::FindOffset(off__CTFGameRules_m_bPlayingMannVsMachine, "CTFGameRules", "m_bPlayingMannVsMachine")) return false;

#ifdef PLATFORM_64BITS			
			buf.SetDword(0x0f + 2, (uint32_t)off__CTFGameRules_m_bPlayingMannVsMachine);

			mask.SetRange(0x00 + 3, 4, 0x00);
			mask.SetRange(0x0d + 1, 1, 0x00);
			mask.SetRange(0x16 + 2, 4, 0x00);
#else
			void *addr__g_pGameRules = AddrManager::GetAddr("g_pGameRules");
			if (addr__g_pGameRules == nullptr) return false;
			
			buf.SetDword(0x00 + 1, (uint32_t)addr__g_pGameRules);
			buf.SetDword(0x09 + 2, (uint32_t)off__CTFGameRules_m_bPlayingMannVsMachine);
			
			mask.SetRange(0x07 + 1, 1, 0x00);
			mask.SetRange(0x10 + 1, 1, 0x00);
#endif
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
#ifdef PLATFORM_64BITS
			/* overwrite the CMP and jnz instruction with NOPs */
			buf.SetRange(0x0f, 13, 0x90);
			
			mask.SetRange(0x0f, 13, 0xff);
#else
			/* overwrite the CMP and jnz instruction with NOPs */
			buf.SetRange(0x09, 13, 0x90);
			
			mask.SetRange(0x09, 13, 0xff);
#endif
			return true;
		}
	};
	
	//Yer nerf: -20% damage vs giants
	DETOUR_DECL_MEMBER(float, CTFKnife_GetMeleeDamage, CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage)
	{
		float ret = DETOUR_MEMBER_CALL(pTarget, piDamageType, piCustomDamage);
		if (*piCustomDamage == TF_DMG_CUSTOM_BACKSTAB && pTarget->IsPlayer() && ToTFPlayer(pTarget)->IsMiniBoss()) {
			auto knife = reinterpret_cast<CTFKnife *>(this);

			int disguise_backstab = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( knife, disguise_backstab, set_disguise_on_backstab);
			
			if (disguise_backstab != 0)
				ret *= 0.9f;
		}
		return ret;
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:YER_Unnerf")
		{
		//	MOD_ADD_DETOUR_MEMBER(CTFKnife_GetMeleeDamage, "CTFKnife::GetMeleeDamage");
			this->AddPatch(new CPatch_CTFKnife_PrimaryAttack());
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_yer_unnerf", "0", FCVAR_NOTIFY,
		"Mod: remove the MvM-specific nerfs from Your Eternal Reward",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
