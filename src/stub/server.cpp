#include "stub/server.h"


GlobalThunk<CHLTVServer *> hltv("hltv");
GlobalThunk<CHostState>	g_HostState("g_HostState");

MemberFuncThunk<CClientFrameManager *, int> CClientFrameManager::ft_CountClientFrames("CClientFrameManager::CountClientFrames");
MemberFuncThunk<CClientFrameManager *, void> CClientFrameManager::ft_RemoveOldestFrame("CClientFrameManager::RemoveOldestFrame");

MemberFuncThunk<CHLTVServer *, void, CBaseClient *> CHLTVServer::ft_StartMaster("CHLTVServer::StartMaster");

MemberFuncThunk<CBaseServer *, CBaseClient *, const char *> CBaseServer::ft_CreateFakeClient("CBaseServer::CreateFakeClient");
MemberFuncThunk<CBaseServer *, void>                   CBaseServer::ft_RunFrame("CBaseServer::RunFrame");

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