#include "mod.h"
#include "factory.h"


class CMoveData;


// on a listen server, this mod will only override stuff in the client DLL


namespace Mod::Etc::Override_Move_Speed
{
	bool IsClient()
	{
		return (ClientFactory() != nullptr);
	}
	
	
	constexpr uint8_t s_Buf_Server[] = {
		0xc7, 0x47, 0x3c, 0x00, 0x00, 0x02, 0x44, // +0000 mov dword ptr [edi+0x3c],520.0f
	};
	
	struct CPatch_CTFGameMovement_ProcessMovement_Server : public CPatch
	{
		CPatch_CTFGameMovement_ProcessMovement_Server() : CPatch(sizeof(s_Buf_Server)) {}
		
		virtual const char *GetFuncName() const override { return "CTFGameMovement::ProcessMovement"; }
		virtual uint32_t GetFuncOffMin() const override { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override { return 0x0100; } // @ 0x0067
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_Server);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* NOP out the assignment of pMove->m_flMaxSpeed */
			buf.SetRange(0x00, 7, 0x90);
			mask.SetRange(0x00, 7, 0xff);
			
			return true;
		}
	};
	
	
	constexpr uint8_t s_Buf_Client[] = {
		0xc7, 0x43, 0x3c, 0x00, 0x00, 0x02, 0x44, // +0000 mov dword ptr [ebx+0x3c],520.0f
	};
	
	struct CPatch_CTFGameMovement_ProcessMovement_Client : public CPatch
	{
		CPatch_CTFGameMovement_ProcessMovement_Client() : CPatch(sizeof(s_Buf_Client)) {}
		
		virtual const char *GetFuncName() const override { return "[client] CTFGameMovement::ProcessMovement"; }
		virtual uint32_t GetFuncOffMin() const override { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override { return 0x0100; } // @ 0x0046
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_Client);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			/* NOP out the assignment of pMove->m_flMaxSpeed */
			buf.SetRange(0x00, 7, 0x90);
			mask.SetRange(0x00, 7, 0xff);
			
			return true;
		}
	};
	
	
	/* offsetof(CMoveData, m_flMaxSpeed) has changed since the last time the
	 * 2013 SDK was updated, so we have to use this kludge instead */
	void CMoveData_Set_MaxSpeed(CMoveData *pMove, float m_flMaxSpeed)
	{
		*(float *)((uintptr_t)pMove + 0x3c) = m_flMaxSpeed;
	}
	
	
	ConVar cvar_limit("sig_etc_override_speed_limit_value", "-1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Etc: player movement speed limit when overridden (use -1 for no limit)");
	
	
	DETOUR_DECL_MEMBER(void, CTFGameMovement_ProcessMovement, CBasePlayer *pBasePlayer, CMoveData *pMove)
	{
		if (cvar_limit.GetFloat() >= 0.0f) {
			CMoveData_Set_MaxSpeed(pMove, cvar_limit.GetFloat());
		}
		
		DETOUR_MEMBER_CALL(CTFGameMovement_ProcessMovement)(pBasePlayer, pMove);
	}
	
	DETOUR_DECL_MEMBER(void, CTFGameMovement_ProcessMovement_client, CBasePlayer *pBasePlayer, CMoveData *pMove)
	{
		if (cvar_limit.GetFloat() >= 0.0f) {
			CMoveData_Set_MaxSpeed(pMove, cvar_limit.GetFloat());
		}
		
		DETOUR_MEMBER_CALL(CTFGameMovement_ProcessMovement_client)(pBasePlayer, pMove);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Override_Move_Speed")
		{
			if (IsClient()) {
				this->AddPatch(new CPatch_CTFGameMovement_ProcessMovement_Client());
				MOD_ADD_DETOUR_MEMBER(CTFGameMovement_ProcessMovement_client, "[client] CTFGameMovement::ProcessMovement");
			} else {
				this->AddPatch(new CPatch_CTFGameMovement_ProcessMovement_Server());
				MOD_ADD_DETOUR_MEMBER(CTFGameMovement_ProcessMovement, "CTFGameMovement::ProcessMovement");
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_override_speed_limit", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Etc: override the 520 HU/s player move speed limit",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
