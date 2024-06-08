#include "mod.h"
#include "util/misc.h"
#include "stub/misc.h"


class IVProfExport;


enum ServerDataRequestType_t : int32_t
{
	SERVERDATA_REQUESTVALUE,
	SERVERDATA_SETVALUE,
	SERVERDATA_EXECCOMMAND,
	SERVERDATA_AUTH, // special RCON command to authenticate a connection
	SERVERDATA_VPROF, // subscribe to a vprof stream
	SERVERDATA_REMOVE_VPROF, // unsubscribe from a vprof stream
	SERVERDATA_TAKE_SCREENSHOT,
	SERVERDATA_SEND_CONSOLE_LOG,
};


/* TODO: move this to a stub file */
class CRConClient
{
public:
	uint8_t pad_00[0x08];   // +0x00
	uint32_t m_VProfExport; // +0x08
	// size???
	// ...
	// +0x34 CSocketCreator m_Socket
	// ...
	
	bool IsConnected() const { return true; } // STUB
	bool ConnectSocket()     { return true; } // STUB
	
	void SendResponse(CUtlBuffer& response, bool bAutoAuthenticate = true)                                            { ft_SendResponse(this, response, bAutoAuthenticate); }
	void BuildResponse(CUtlBuffer& response, ServerDataRequestType_t msg, const char *pString1, const char *pString2) { ft_BuildResponse(this, response, msg, pString1, pString2); }
	
private:
	static MemberFuncThunk<CRConClient *, void, CUtlBuffer&, bool>                                                ft_SendResponse;
	static MemberFuncThunk<CRConClient *, void, CUtlBuffer&, ServerDataRequestType_t, const char *, const char *> ft_BuildResponse;
};
MemberFuncThunk<CRConClient *, void, CUtlBuffer&, bool>                                                CRConClient::ft_SendResponse ("CRConClient::SendResponse");
MemberFuncThunk<CRConClient *, void, CUtlBuffer&, ServerDataRequestType_t, const char *, const char *> CRConClient::ft_BuildResponse("CRConClient::BuildResponse");


namespace Mod::Util::VProf_Remote
{
	FnCommandCallback_t *GetConCommandCallback(ConCommand *cmd)
	{
		return (FnCommandCallback_t *)((uintptr_t)cmd + sizeof(ConCommandBase));
	}
	
	
	IVProfExport *& GetVProfExportPtr()
	{
		assert(vprofexport != nullptr);
		
#if defined _WINDOWS
		/* we're making assumptions about memory layout here */
		return *reinterpret_cast<IVProfExport **>((uintptr_t)vprofexport - 0x4);
#else
		assert(false);
#endif
	}
	
	
	void OverrideVProfExport(IVProfExport *pExport)
	{
		auto& g_pVProfExport = GetVProfExportPtr();
		
		if (g_pVProfExport == vprofexport) {
			g_pVProfExport = pExport;
		}
	}
	void ResetVProfExport(IVProfExport *pExport)
	{
		auto& g_pVProfExport = GetVProfExportPtr();
		
		if (g_pVProfExport == pExport) {
			g_pVProfExport = vprofexport;
		}
	}
	
	
	/* TODO: implement these stubs if possible!
	 * CRConClient::IsConnected
	 * CRConClient::ConnectSocket
	 */
	
	
	void Override_Start(const CCommand& cmd)
	{
		auto& rcon = RCONClient();
		
		if (!rcon.IsConnected()) {
			if (!rcon.ConnectSocket()) {
				return;
			}
		}
		
		OverrideVProfExport(reinterpret_cast<IVProfExport *>(&rcon.m_VProfExport));
		
		CUtlBuffer response;
		rcon.BuildResponse(response, SERVERDATA_VPROF, "", "");
		rcon.SendResponse(response);
	}
	
	void Override_Stop(const CCommand& cmd)
	{
		auto& rcon = RCONClient();
		
		ResetVProfExport(reinterpret_cast<IVProfExport *>(rcon.m_VProfExport));
		
		if (!rcon.IsConnected()) {
			return;
		}
		
		CUtlBuffer response;
		rcon.BuildResponse(response, SERVERDATA_REMOVE_VPROF, "", "");
		rcon.SendResponse(response);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:VProf_Remote") {}
		
		virtual bool OnLoad() override
		{
			this->m_pCmdStart = g_pCVar->FindCommand("vprof_remote_start");
			this->m_pCmdStop  = g_pCVar->FindCommand("vprof_remote_stop");
			
			return (this->m_pCmdStart != nullptr && this->m_pCmdStop != nullptr);
		}
		
		virtual void OnEnable() override
		{
			/* TODO: remove me */
			assert(!this->IsEnabled());
			
			this->m_pSavedCallbackStart = *GetConCommandCallback(this->m_pCmdStart);
			this->m_pSavedCallbackStop  = *GetConCommandCallback(this->m_pCmdStop);
			
			*GetConCommandCallback(this->m_pCmdStart) = &Override_Start;
			*GetConCommandCallback(this->m_pCmdStop)  = &Override_Stop;
		}
		virtual void OnDisable() override
		{
			/* TODO: remove me */
			assert(this->IsEnabled());
			
			*GetConCommandCallback(this->m_pCmdStart) = this->m_pSavedCallbackStart;
			*GetConCommandCallback(this->m_pCmdStop)  = this->m_pSavedCallbackStop;
		}
		
	private:
		ConCommand *m_pCmdStart = nullptr;
		ConCommand *m_pCmdStop  = nullptr;
		
		FnCommandCallback_t m_pSavedCallbackStart = nullptr;
		FnCommandCallback_t m_pSavedCallbackStop  = nullptr;
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_vprof_remote", "0", FCVAR_NOTIFY,
		"Utility: make vprof_remote_start and vprof_remote_stop work",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
#if 0
	DETOUR_DECL_MEMBER(void, CRConClient_SendCmd, const char *cmd)
	{
		DevMsg("CRConClient::SendCmd detour!\n");
		DevMsg("  this = %08x\n", (uintptr_t)this);
		DevMsg("  cmd  = \"%s\"\n", cmd);
		
		DETOUR_MEMBER_CALL(cmd);
	}
	
	
	CDetour *detour;
	CON_COMMAND(sig_util_vprof_remote_test1, "")
	{
		detour = new CDetour("CRConClient::SendCmd",
			GET_MEMBER_CALLBACK(CRConClient_SendCmd),
			GET_MEMBER_INNERPTR(CRConClient_SendCmd));
		
		detour->Load();
		detour->Enable();
		
		DevMsg("test1: detour created\n");
		
		engineclient->ExecuteClientCmd("rcon helloworld");
		engineclient->ExecuteClientCmd("rcon");
		
		DevMsg("test1: commands executed\n");
	}
	
	CON_COMMAND(sig_util_vprof_remote_test2, "")
	{
		DevMsg("test2\n");
		
		detour->Disable();
		detour->Unload();
		
		delete detour;
	}
#endif
}


namespace Mod::Util::VProf_Remote_Test
{
	typedef unsigned int ra_listener_id;
	DETOUR_DECL_MEMBER(void, CServerRemoteAccess_SendVProfData, ra_listener_id listenerID, bool bGroupData, void *data, int len)
	{
		DevMsg("CServerRemoteAccess::SendVProfData (len: %d)\n", len);
		
		for (int i = 0; i < len; ++i) {
			DevMsg("%02x", ((uint8_t *)data)[i]);
			
			if (i % 16 == 15 || i == len - 1) {
				DevMsg("\n");
			} else if (i % 16 == 7) {
				DevMsg("  ");
			} else {
				DevMsg(" ");
			}
		}
		
		DETOUR_MEMBER_CALL(listenerID, bGroupData, data, len);
	}
	
	
	StaticFuncThunk<void> ft_WriteRemoteVProfData("WriteRemoteVProfData");
	void WriteRemoteVProfData() { ft_WriteRemoteVProfData(); }
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:VProf_Remote_Test")
		{
			MOD_ADD_DETOUR_MEMBER(CServerRemoteAccess_SendVProfData, "CServerRemoteAccess::SendVProfData");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			WriteRemoteVProfData();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_vprof_remote_test", "0", FCVAR_NOTIFY,
		"Utility: test",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	// TODO:
	// we think it's sampling properly
	
	// CHECK:
	// is it not recording detailed numbers because it's not in GUI mode?
	// is the network transmission stuff just broken?
	
	// THINGS THE DEDICATED SERVER DOESN'T DO:
	// vprof_engine.cpp: PreUpdateProfile: doesn't call GetVProfPanel()->UpdateProfile
	// vprof_engine.cpp: ccmd vprof_reset: doesn't call GetVProfPanel()->Reset
	// 
	
	
	// PERHAPS:
	// does the GUI panel just not update properly in these cases even though the data is there?
	// - when playing back files
	// - when receiving remote data
	
	// vprof record seems to be writing a ton of zeroes...
	
	
	// look at vprof_engine.cpp, class CVProfExport, func SnapshotVProfHistory
	
	
	CON_COMMAND(sig_util_vprof_remote_check, "")
	{
		DevMsg("CHECK: g_VProfCurrentProfile\n");
		DevMsg("  IsEnabled:           %s\n", (g_VProfCurrentProfile.IsEnabled() ? "true" : "false"));
		DevMsg("  GetDetailLevel:      %d\n", g_VProfCurrentProfile.GetDetailLevel());
		DevMsg("  NumFramesSampled:    %d\n", g_VProfCurrentProfile.NumFramesSampled());
		DevMsg("  GetPeakFrameTime:    %f\n", g_VProfCurrentProfile.GetPeakFrameTime());
		DevMsg("  GetTotalTimeSampled: %f\n", g_VProfCurrentProfile.GetTotalTimeSampled());
		DevMsg("  GetTimeLastFrame:    %f\n", g_VProfCurrentProfile.GetTimeLastFrame());
		DevMsg("  GetNumBudgetGroups:  %d\n", g_VProfCurrentProfile.GetNumBudgetGroups());
		DevMsg("  GetNumCounters:      %d\n", g_VProfCurrentProfile.GetNumCounters());
		
		DevMsg("\nCHECK: g_VProfExport\n");
		
		CUtlVector<float>& m_Times = *reinterpret_cast<CUtlVector<float> *>((uintptr_t)vprofexport + 0x4);
		DevMsg("  m_Times: count %d\n", m_Times.Count());
	}
}
