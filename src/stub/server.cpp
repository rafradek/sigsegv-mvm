#include "stub/server.h"
#include "mem/extract.h"

static constexpr uint8_t s_Buf_CBaseServer_m_Clients[] =
{
#ifdef PLATFORM_64BITS
	0x48, 0x8b, 0x87, 0x80, 0x01, 0x00, 0x00,  // +0x0000 mov     rax, [rdi+180h]
	0x89, 0xf6,                                // +0x0007 mov     esi, esi
	0x48, 0x8b, 0x04, 0xf0,                    // +0x0009 mov     rax, [rax+rsi*8]
#else
	0x55,                                // +0x0000 push    ebp
	0x89, 0xe5,                          // +0x0001 mov     ebp, esp
	0x8b, 0x45, 0x08,                    // +0x0003 mov     eax, [ebp+this]
	0x8b, 0x55, 0x0c,                    // +0x0006 mov     edx, [ebp+arg_4]
	0x5d,                                // +0x0009 pop     ebp
	0x8b, 0x80, 0x58, 0x01, 0x00, 0x00,  // +0x000a mov     eax, [eax+158h]
#endif
};

struct CExtract_CBaseServer_m_Clients : public IExtract<uint32_t>
{
	using T = uint32_t;
	
	CExtract_CBaseServer_m_Clients() : IExtract<T>(sizeof(s_Buf_CBaseServer_m_Clients)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CBaseServer_m_Clients);
#ifdef PLATFORM_64BITS
		mask.SetRange(0x00 + 3, 4, 0x00);
#else
		mask.SetRange(0x0a + 2, 4, 0x00);
#endif
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CBaseServer::GetClient"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x00FF; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 3; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x000a + 2; }
#endif
};

GlobalThunk<CHLTVServer *> hltv("hltv");
GlobalThunk<CHostState>	g_HostState("g_HostState");

MemberFuncThunk<CClientFrameManager *, int> CClientFrameManager::ft_CountClientFrames("CClientFrameManager::CountClientFrames");
MemberFuncThunk<CClientFrameManager *, void> CClientFrameManager::ft_RemoveOldestFrame("CClientFrameManager::RemoveOldestFrame");

MemberFuncThunk<CHLTVServer *, void, CBaseClient *> CHLTVServer::ft_StartMaster("CHLTVServer::StartMaster");

MemberFuncThunk<CBaseServer *, CBaseClient *, const char *> CBaseServer::ft_CreateFakeClient("CBaseServer::CreateFakeClient");
MemberFuncThunk<CBaseServer *, void>                   CBaseServer::ft_RunFrame("CBaseServer::RunFrame");

IMPL_EXTRACT(CUtlVector<CBaseClient*>, CBaseServer, m_Clients, new CExtract_CBaseServer_m_Clients());

MemberVFuncThunk<CBaseServer *, float>                  CBaseServer::vt_GetCPUUsage    (TypeName<CBaseServer>(), "CBaseServer::GetCPUUsage");
MemberVFuncThunk<CBaseServer *, void, int>              CBaseServer::vt_UserInfoChanged(TypeName<CBaseServer>(), "CBaseServer::UserInfoChanged");
MemberVFuncThunk<CBaseServer *, void, const netadr_t &, int, const char *> CBaseServer::vt_RejectConnection(TypeName<CBaseServer>(), "CBaseServer::RejectConnection");

MemberVFuncThunk<CBaseClient *, void>                 CBaseClient::vt_UpdateSendState(TypeName<CBaseClient>(), "CBaseClient::UpdateSendState");
MemberVFuncThunk<CBaseClient *, void, CClientFrame *> CBaseClient::vt_SendSnapshot(TypeName<CBaseClient>(), "CBaseClient::SendSnapshot");
MemberVFuncThunk<CBaseClient *, bool, int>            CBaseClient::vt_UpdateAcknowledgedFramecount(TypeName<CBaseClient>(), "CBaseClient::UpdateAcknowledgedFramecount");

MemberFuncThunk<CGameClient *, bool>                   CGameClient::ft_ShouldSendMessages("CGameClient::ShouldSendMessages");
MemberFuncThunk<CGameClient *, void, CFrameSnapshot *> CGameClient::ft_SetupPackInfo("CGameClient::SetupPackInfo");
MemberFuncThunk<CGameClient *, void>                   CGameClient::ft_SetupPrevPackInfo("CGameClient::SetupPrevPackInfo");
MemberFuncThunk<CGameClient *, CClientFrame *>         CGameClient::ft_GetSendFrame("CGameClient::GetSendFrame");

MemberFuncThunk<CNetworkStringTable *, void, int>              CNetworkStringTable::ft_UpdateMirrorTable("CNetworkStringTable::UpdateMirrorTable");
MemberFuncThunk<CNetworkStringTable *, void>                   CNetworkStringTable::ft_DeleteAllStrings("CNetworkStringTable::DeleteAllStrings");

MemberFuncThunk<CNetChan *, bool, const char *> CNetChan::ft_IsFileInWaitingList    ("CNetChan::IsFileInWaitingList");
MemberFuncThunk<CNetChan *, void, bool>         CNetChan::ft_SetFileTransmissionMode("CNetChan::SetFileTransmissionMode");
StaticFuncThunk<bool, const char*>              CNetChan::ft_IsValidFileForTransfer ("CNetChan::IsValidFileForTransfer");