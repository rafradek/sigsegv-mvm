#include "stub/tfbot.h"
#include "mem/extract.h"

#if defined _LINUX

static constexpr uint8_t s_Buf_CTFBot_m_nMission[] = {
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x53,                               // +0003  push ebx
	0x83, 0xec, 0x00,                   // +0004  sub esp,0xXX
	0x8b, 0x5d, 0x08,                   // +0007  mov ebx,DWORD PTR [ebp+0x8]
	0x8b, 0x45, 0x0c,                   // +000A  mov eax,DWORD PTR [ebp+0xc]
	0x80, 0x7d, 0x10, 0x00,             // +000D  cmp BYTE PTR [ebp+0x10],0x0
	0x8b, 0x93, 0x00, 0x00, 0x00, 0x00, // +0011  mov edx,DWORD PTR [ebx+0xVVVVVVVV]
};

struct CExtract_CTFBot_m_nMission : public IExtract<CTFBot::MissionType *>
{
	CExtract_CTFBot_m_nMission() : IExtract<CTFBot::MissionType *>(sizeof(s_Buf_CTFBot_m_nMission)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFBot_m_nMission);
		
		mask.SetRange(0x04 + 2, 1, 0x00);
		mask.SetRange(0x11 + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFBot::SetMission"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0200; }
	virtual uint32_t GetExtractOffset() const override { return 0x0011 + 2; }
};

#elif defined _WINDOWS

static constexpr uint8_t s_Buf_CTFBot_m_nMission[] = {
	0x83, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x02, // +0000  cmp dword ptr [edi+0xVVVVVVVV],0x2
	0x75, 0x00,                               // +0007  jnz 0xXX
	0x68, 0x00, 0x00, 0x00, 0x00,             // +0009  push offset "mission_sentry_buster"
};

struct CExtract_CTFBot_m_nMission : public IExtract<CTFBot::MissionType *>
{
	using T = CTFBot::MissionType *;
	
	CExtract_CTFBot_m_nMission() : IExtract<T>(sizeof(s_Buf_CTFBot_m_nMission)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		const char *str = Scan::FindUniqueConstStr(Library::SERVER, "mission_sentry_buster");
		if (str == nullptr) return false;
		
		buf.CopyFrom(s_Buf_CTFBot_m_nMission);
		buf.SetDword(0x09 + 1, (uint32_t)str);
		
		mask.SetRange(0x00 + 2, 4, 0x00);
		mask.SetRange(0x07 + 1, 1, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CFuncNavCost::IsApplicableTo"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0200; }
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 2; }
};

#endif


#if defined _LINUX

static constexpr uint8_t s_Buf_CTFBot_m_Tags[] = {
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x53,                               // +0003  push ebx
	0x8b, 0x5d, 0x08,                   // +0004  mov ebx,[ebp+this]
	0x8b, 0x83, 0xd8, 0x2b, 0x00, 0x00, // +0007  mov eax,[ebx+0xVVVVVVVV]
};

struct CExtract_CTFBot_m_Tags : public IExtract<CUtlVector<CFmtStr> *>
{
	using T = CUtlVector<CFmtStr> *;
	
	CExtract_CTFBot_m_Tags() : IExtract<T>(sizeof(s_Buf_CTFBot_m_Tags)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFBot_m_Tags);
		
		mask.SetRange(0x07 + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFBot::ClearTags"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
	
	virtual T AdjustValue(T val) const { return reinterpret_cast<T>((uintptr_t)val - 0xc); }
};

#elif defined _WINDOWS

using CExtract_CTFBot_m_Tags = IExtractStub;

#endif


#if defined _LINUX

static constexpr uint8_t s_Buf_CTFBot_m_nBotAttrs[] = {
	0xc7, 0x83, 0xb8, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // +0000  mov dword ptr [ebx+m_nMission],0x00000000
	0xc7, 0x83, 0xd0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // +000A  mov dword ptr [ebx+m_nBotAttrs],0x00000000
	0x89, 0x5d, 0x08,                                           // +0014  mov [ebp+this],ebx
	0x83, 0xc4, 0x24,                                           // +0017  add esp,0x24
	0x5b,                                                       // +001A  pop ebx
	0x5d,                                                       // +001B  pop ebp
	0xe9,                                                       // +001C  jmp CTFBot::StopIdleSound
};

struct CExtract_CTFBot_m_nBotAttrs : public IExtract<CTFBot::AttributeType *>
{
	using T = CTFBot::AttributeType *;
	
	CExtract_CTFBot_m_nBotAttrs() : IExtract<T>(sizeof(s_Buf_CTFBot_m_nBotAttrs)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFBot_m_nBotAttrs);
		
		mask.SetRange(0x00 + 2, 4, 0x00);
		mask.SetRange(0x0a + 2, 4, 0x00);
		mask.SetRange(0x17 + 2, 1, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFBot::ChangeTeam"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0080; }
	virtual uint32_t GetExtractOffset() const override { return 0x000a + 2; }
};

#elif defined _WINDOWS

using CExtract_CTFBot_m_nBotAttrs = IExtractStub;

#endif


// the line below is broken; it once worked but then stopped working for some reason
//#define NEXTBOTPLAYER_GETVFT(NAME, ...) template<> MemberVFuncThunk<NextBotPlayer<CTFPlayer> *, __VA_ARGS__>& NextBotPlayer<CTFPlayer>::GetVFT_##NAME() { static MemberVFuncThunk<NextBotPlayer<CTFPlayer> *, __VA_ARGS__> vft(TypeName<NextBotPlayer<CTFPlayer>>(), "NextBotPlayer<CTFPlayer>::" #NAME); return vft; }

/* we have to do this nonsense due to restrictions on having static data members
 * in _specialized_ class templates */
#define NEXTBOTPLAYER_GETVFT(NAME, ...) \
	static MemberVFuncThunk<NextBotPlayer<CTFPlayer> *, __VA_ARGS__> NextBotPlayer_CTFPlayer_##NAME(TypeName<NextBotPlayer<CTFPlayer>>(), "NextBotPlayer<CTFPlayer>::" #NAME); \
	template<> MemberVFuncThunk<NextBotPlayer<CTFPlayer> *, __VA_ARGS__>& NextBotPlayer<CTFPlayer>::GetVFT_##NAME() { return NextBotPlayer_CTFPlayer_##NAME; }
NEXTBOTPLAYER_GETVFT(PressFireButton,          void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseFireButton,        void              );
NEXTBOTPLAYER_GETVFT(PressAltFireButton,       void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseAltFireButton,     void              );
NEXTBOTPLAYER_GETVFT(PressMeleeButton,         void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseMeleeButton,       void              );
NEXTBOTPLAYER_GETVFT(PressSpecialFireButton,   void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseSpecialFireButton, void              );
NEXTBOTPLAYER_GETVFT(PressUseButton,           void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseUseButton,         void              );
NEXTBOTPLAYER_GETVFT(PressReloadButton,        void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseReloadButton,      void              );
NEXTBOTPLAYER_GETVFT(PressForwardButton,       void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseForwardButton,     void              );
NEXTBOTPLAYER_GETVFT(PressBackwardButton,      void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseBackwardButton,    void              );
NEXTBOTPLAYER_GETVFT(PressLeftButton,          void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseLeftButton,        void              );
NEXTBOTPLAYER_GETVFT(PressRightButton,         void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseRightButton,       void              );
NEXTBOTPLAYER_GETVFT(PressJumpButton,          void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseJumpButton,        void              );
NEXTBOTPLAYER_GETVFT(PressCrouchButton,        void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseCrouchButton,      void              );
NEXTBOTPLAYER_GETVFT(PressWalkButton,          void, float       );
NEXTBOTPLAYER_GETVFT(ReleaseWalkButton,        void              );
NEXTBOTPLAYER_GETVFT(SetButtonScale,           void, float, float);


MemberFuncThunk<CTFBot::SuspectedSpyInfo_t *, void> CTFBot::SuspectedSpyInfo_t::ft_Suspect             ("CTFBot::SuspectedSpyInfo_t::Suspect");
MemberFuncThunk<CTFBot::SuspectedSpyInfo_t *, bool> CTFBot::SuspectedSpyInfo_t::ft_IsCurrentlySuspected("CTFBot::SuspectedSpyInfo_t::IsCurrentlySuspected");
MemberFuncThunk<CTFBot::SuspectedSpyInfo_t *, bool> CTFBot::SuspectedSpyInfo_t::ft_TestForRealizing    ("CTFBot::SuspectedSpyInfo_t::TestForRealizing");


IMPL_EXTRACT(CTFBot::MissionType,   CTFBot, m_nMission,  new CExtract_CTFBot_m_nMission());
#if !defined _WINDOWS
#if TOOLCHAIN_FIXES
IMPL_EXTRACT(CUtlVector<CFmtStr>,   CTFBot, m_Tags,      new CExtract_CTFBot_m_Tags());
#endif
IMPL_EXTRACT(CTFBot::AttributeType, CTFBot, m_nBotAttrs, new CExtract_CTFBot_m_nBotAttrs());
IMPL_RELATIVE(CTFBot::WeaponRestriction, CTFBot, m_iWeaponRestrictionFlags,    m_nBotAttrs, -0x04);
#endif

MemberFuncThunk<const CTFBot *, ILocomotion *                            > CTFBot::ft_GetLocomotionInterface      ("CTFBot::GetLocomotionInterface");
MemberFuncThunk<const CTFBot *, IBody *                                  > CTFBot::ft_GetBodyInterface            ("CTFBot::GetBodyInterface");
MemberFuncThunk<const CTFBot *, IVision *                                > CTFBot::ft_GetVisionInterface          ("CTFBot::GetVisionInterface");
MemberFuncThunk<const CTFBot *, IIntention *                             > CTFBot::ft_GetIntentionInterface       ("CTFBot::GetIntentionInterface");
MemberFuncThunk<const CTFBot *, float                                    > CTFBot::ft_GetDesiredPathLookAheadRange("CTFBot::GetDesiredPathLookAheadRange");
MemberFuncThunk<      CTFBot *, void, CTFWeaponBase *                    > CTFBot::ft_PushRequiredWeapon          ("CTFBot::PushRequiredWeapon");
MemberFuncThunk<      CTFBot *, void                                     > CTFBot::ft_PopRequiredWeapon           ("CTFBot::PopRequiredWeapon");
MemberFuncThunk<const CTFBot *, bool, const Vector&                      > CTFBot::ft_IsLineOfFireClear_vec       ("CTFBot::IsLineOfFireClear_vec");
MemberFuncThunk<const CTFBot *, bool, CBaseEntity *                      > CTFBot::ft_IsLineOfFireClear_ent       ("CTFBot::IsLineOfFireClear_ent");
MemberFuncThunk<const CTFBot *, bool, const Vector&, const Vector&       > CTFBot::ft_IsLineOfFireClear_vec_vec   ("CTFBot::IsLineOfFireClear_vec_vec");
MemberFuncThunk<const CTFBot *, bool, const Vector&, CBaseEntity *       > CTFBot::ft_IsLineOfFireClear_vec_ent   ("CTFBot::IsLineOfFireClear_vec_ent");
MemberFuncThunk<      CTFBot *, CTFBot::SuspectedSpyInfo_t *, CTFPlayer *> CTFBot::ft_IsSuspectedSpy              ("CTFBot::IsSuspectedSpy");
MemberFuncThunk<      CTFBot *, void, CTFPlayer *                        > CTFBot::ft_SuspectSpy                  ("CTFBot::SuspectSpy");
MemberFuncThunk<      CTFBot *, void, CTFPlayer *                        > CTFBot::ft_StopSuspectingSpy           ("CTFBot::StopSuspectingSpy");
MemberFuncThunk<const CTFBot *, bool, CTFPlayer *                        > CTFBot::ft_IsKnownSpy                  ("CTFBot::IsKnownSpy");
MemberFuncThunk<      CTFBot *, void, CTFPlayer *                        > CTFBot::ft_RealizeSpy                  ("CTFBot::RealizeSpy");
MemberFuncThunk<      CTFBot *, void, CTFPlayer *                        > CTFBot::ft_ForgetSpy                   ("CTFBot::ForgetSpy");
MemberFuncThunk<      CTFBot *, void, const char *                       > CTFBot::ft_AddItem                     ("CTFBot::AddItem");
MemberFuncThunk<const CTFBot *, float                                    > CTFBot::ft_GetDesiredAttackRange       ("CTFBot::GetDesiredAttackRange");
MemberFuncThunk<      CTFBot *, void, const CKnownEntity *               > CTFBot::ft_EquipBestWeaponForThreat    ("CTFBot::EquipBestWeaponForThreat");
MemberFuncThunk<      CTFBot *, bool                                     > CTFBot::ft_EquipRequiredWeapon         ("CTFBot::EquipRequiredWeapon");
MemberFuncThunk<      CTFBot *, CTFPlayer *                              > CTFBot::ft_SelectRandomReachableEnemy  ("CTFBot::SelectRandomReachableEnemy");
MemberFuncThunk<      CTFBot *, bool                                     > CTFBot::ft_ShouldAutoJump              ("CTFBot::ShouldAutoJump");
MemberFuncThunk<const CTFBot *, const CTFBot::EventChangeAttributes_t *, const char*     > CTFBot::ft_GetEventChangeAttributes    ("CTFBot::GetEventChangeAttributes");
MemberFuncThunk<      CTFBot *, void, const CTFBot::EventChangeAttributes_t *            > CTFBot::ft_OnEventChangeAttributes     ("CTFBot::OnEventChangeAttributes");
MemberFuncThunk<      CTFBot *, float, float, int          > CTFBot::ft_TransientlyConsistentRandomValue ("CTFBot::TransientlyConsistentRandomValue");
MemberFuncThunk<      CTFBot *, void                       > CTFBot::ft_LeaveSquad                       ("CTFBot::LeaveSquad");
#ifdef ADD_EXTATTR
std::map<CHandle<CTFBot>, CTFBot::ExtendedAttr> CTFBot::s_ExtAttrs;
#endif

StaticFuncThunk<CTFBot *, CBaseEntity *> ft_ToTFBot("ToTFBot");

StaticFuncThunk<CTFBot *, const char *, bool> ft_NextBotCreatePlayerBot_CTFBot("NextBotCreatePlayerBot<CTFBot>");
template<> CTFBot *NextBotCreatePlayerBot<CTFBot>(const char *name, bool fake_client) { return ft_NextBotCreatePlayerBot_CTFBot(name, fake_client); }
