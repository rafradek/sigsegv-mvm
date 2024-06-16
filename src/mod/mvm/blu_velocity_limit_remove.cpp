#include "mod.h"

namespace Mod::MvM::Blu_Velocity_Limit_Remove
{
    
	constexpr uint8_t s_Buf_PostThink[] = {
#ifdef PLATFORM_64BITS
		0xe8, 0x32, 0x41, 0xb7, 0xff,        // +0x0000 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
		0x83, 0xf8, 0x03,                    // +0x0005 cmp     eax, 3
		0x0f, 0x85, 0xd0, 0xfe, 0xff, 0xff,  // +0x0008 jnz     loc_11DF4A7
#else
		0xe8, 0x1d, 0x6d, 0xba, 0xff,        // +0x0000 call    _ZNK11CBaseEntity13GetTeamNumberEv; CBaseEntity::GetTeamNumber(void)
		0x83, 0xc4, 0x10,                    // +0x0005 add     esp, 10h
		0x83, 0xf8, 0x03,                    // +0x0008 cmp     eax, 3
		0x0f, 0x85, 0xc3, 0xfe, 0xff, 0xff,  // +0x000b jnz     loc_F1DCA2
#endif
	};
	
	struct CPatch_CTFPlayer_PostThink : public CPatch
	{
		CPatch_CTFPlayer_PostThink() : CPatch(sizeof(s_Buf_PostThink)) {}
		
		virtual const char *GetFuncName() const override { return "CTFPlayer::PostThink"; }
		virtual uint32_t GetFuncOffMin() const override  { return 0x0100; }
		virtual uint32_t GetFuncOffMax() const override  { return 0x0500; }
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_PostThink);
			
#ifdef PLATFORM_64BITS
			mask.SetDword(0x01 + 1, 0x00000000);
			mask.SetDword(0x08 + 2, 0x00000000);
#else
			buf.SetDword(0x00 +1, (uint32_t)AddrManager::GetAddr("CBaseEntity::GetTeamNumber"));
			mask[0x05 + 2] = 0x00;
			
			mask.SetDword(0x0b + 2, 0x00000000);
#endif

			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* replace 'TF_TEAM_BLUE'*/
#ifdef PLATFORM_64BITS
			buf[0x05 + 0] = 0x83;
			buf[0x05 + 1] = 0xF8;
			buf[0x05 + 2] = 0xFF;
			
			mask.SetRange(0x05, 3, 0xff);
#else
			buf[0x08 + 0] = 0x83;
			buf[0x08 + 1] = 0xF8;
			buf[0x08 + 2] = 0xFF;
			
			mask.SetRange(0x08, 3, 0xff);
#endif
			
			return true;
		}
	};

    class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Blu_Velocity_Limit_Remove")
		{
			this->AddPatch(new CPatch_CTFPlayer_PostThink());
        }
    };
	CMod s_Mod;

    ConVar cvar_enable("sig_mvm_blu_velocity_limit_remove", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: remove velocity limit for blu team",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}