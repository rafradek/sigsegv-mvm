#include "mod.h"
#include "stub/tfbot.h"
#include "mem/extract.h"

namespace Mod::Robot::Underground_Check_Override
{
	constexpr uint8_t s_Buf[] = {
		0xf3, 0x0f, 0x10, 0x45, 0xb8,                   // +0000  movss xmm0,[ebp+var_48]
		0xf3, 0x0f, 0x5c, 0x87, 0xac, 0x02, 0x00, 0x00, // +0005  subss xmm0,dword ptr [edi+0x2ac]
		0x0f, 0x2f, 0x05, 0xd0, 0x2c, 0x06, 0x01,       // +000D  comiss xmm0,ds:[100.0f]
		0x0f, 0x86, 0x36, 0x01, 0x00, 0x00,             // +0014  jbe +0x136
		0x8b, 0x45, 0x0c,                               // +001A  mov eax,[ebp+this]
		0x0f, 0x57, 0xc0,                               // +001D  xorps xmm0,xmm0
		0x0f, 0x2f, 0x40, 0x70,                         // +0020  comiss xmm0,dword ptr [eax+0x70]
	};

	struct CExtract_CTFBotMainAction_m_itUnderground : public IExtract<ptrdiff_t>
	{
		CExtract_CTFBotMainAction_m_itUnderground() : IExtract<ptrdiff_t>(sizeof(s_Buf)) {}
		
		virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf);
			
			int off_m_vecAbsOrigin = 0;
			if (!Prop::FindOffset(off_m_vecAbsOrigin, "CBaseEntity", "m_vecAbsOrigin")) return false;
			
			buf.SetDword(0x05 + 4, off_m_vecAbsOrigin + 8);
			
			mask.SetRange(0x00 + 4, 1, 0x00);
			mask.SetRange(0x0d + 3, 4, 0x00);
			mask.SetRange(0x14 + 2, 4, 0x00);
			
			return true;
		}
		
		virtual const char *GetFuncName() const override            { return "CTFBotMainAction::Update"; }
		virtual uint32_t GetFuncOffMin() const override             { return 0x0000; }
		virtual uint32_t GetFuncOffMax() const override             { return 0x0a00; } // @ 0x0789
		virtual uint32_t GetExtractOffset() const override          { return 0x0020 + 0; }
		virtual ptrdiff_t AdjustValue(ptrdiff_t val) const override { return (ptrdiff_t)(((uint32_t)val & 0xff000000) >> 24); }
	};
	
	
	ptrdiff_t off_CTFBotMainAction_m_itUnderground = 0;
	
	
	RefCount rc_CTFBotMainAction_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		auto m_itUnderground = reinterpret_cast<IntervalTimer *>((uintptr_t)this + off_CTFBotMainAction_m_itUnderground);
		m_itUnderground->Invalidate();
		
		return DETOUR_MEMBER_CALL(CTFBotMainAction_Update)(actor, dt);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Robot:Underground_Check_Override")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update, "CTFBotMainAction::Update");
		}
		
		virtual bool OnLoad() override
		{
			CExtract_CTFBotMainAction_m_itUnderground extractor;
			if (!extractor.Init())  return false;
			if (!extractor.Check()) return false;
			
			off_CTFBotMainAction_m_itUnderground = extractor.Extract();
		//	DevMsg("CTFBotMainAction::m_itUnderground @ +0x%02x\n", off_CTFBotMainAction_m_itUnderground);
			
			return true;
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_robot_underground_check_override", "0", FCVAR_NOTIFY,
		"Mod: disable the faulty underground-detection logic in CTFBotMainAction::Update",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
