#include "stub/server.h"


GlobalThunk<CHLTVServer *> hltv("hltv");

MemberFuncThunk<CHLTVServer *, void, CBaseClient *> CHLTVServer::ft_StartMaster("CHLTVServer::StartMaster");
MemberFuncThunk<CHLTVServer *, int> CHLTVServer::ft_CountClientFrames("CClientFrameManager::CountClientFrames");
MemberFuncThunk<CHLTVServer *, void> CHLTVServer::ft_RemoveOldestFrame("CClientFrameManager::RemoveOldestFrame");


MemberFuncThunk<CBaseServer *, CBaseClient *, const char *> CBaseServer::ft_CreateFakeClient("CBaseServer::CreateFakeClient");

MemberVFuncThunk<CBaseServer *, float>                  CBaseServer::vt_GetCPUUsage(TypeName<CBaseServer>(), "CBaseServer::GetCPUUsage");


MemberFuncThunk<CGameClient *, bool>              CGameClient::ft_ShouldSendMessages("CGameClient::ShouldSendMessages");