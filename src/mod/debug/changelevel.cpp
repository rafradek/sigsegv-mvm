#include "mod.h"
#include "addr/standard.h"


namespace Mod::Debug::ChangeLevel
{
	// ewww, get rid of this garbage!
	struct Addrs
	{
		CAddr_Sym *addr1, *addr2, *addr3, *addr4;
		
		Addrs()
		{
			
			addr1 = new CAddr_Sym("CChangeLevelIssue::CanCallVote",    "_ZN17CChangeLevelIssue11CanCallVoteEiPKcR20vote_create_failed_tRi");
			addr2 = new CAddr_Sym("CChangeLevelIssue::ExecuteCommand", "_ZN17CChangeLevelIssue14ExecuteCommandEv");
			addr3 = new CAddr_Sym("CVEngineServer::ChangeLevel",       "_ZN14CVEngineServer11ChangeLevelEPKcS1_");
			addr4 = new CAddr_Sym("Host_Changelevel_f",                "_Z18Host_Changelevel_fRK8CCommand");
			
			addr3->SetLibrary(Library::ENGINE);
			addr4->SetLibrary(Library::ENGINE);
		}
		
		~Addrs()
		{
			delete addr1;
			delete addr2;
			delete addr3;
			delete addr4;
		}
	};
//	Addrs addrs;
	
	
	DETOUR_DECL_MEMBER(bool, CChangeLevelIssue_CanCallVote, int i1, const char *s1, int& fail, int& i2)
	{
		auto result = DETOUR_MEMBER_CALL(i1, s1, fail, i2);
		DevMsg("[CChangeLevelIssue::CanCallVote: %s] [i1:%d] [s1:\"%s\"] [fail:%d] [i2:%d]\n",
			(result ? "YES" : " NO"), i1, s1, fail, i2);
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CChangeLevelIssue_ExecuteCommand)
	{
		DevMsg("[CChangeLevelIssue::ExecuteCommand] [@ 0x5c: \"%s\"]\n",
			(const char *)((uintptr_t)this + 0x5c));
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CVEngineServer_ChangeLevel, const char *s1, const char *s2)
	{
		DevMsg("[CVEngineServer::ChangeLevel] [s1:\"%s\"] [s2:\"%s\"]\n", s1, s2);
		DETOUR_MEMBER_CALL(s1, s2);
	}
	
	DETOUR_DECL_STATIC(void, Host_Changelevel_f, const CCommand& args)
	{
		DevMsg("[Host_Changelevel_f] [%s]\n", args.GetCommandString());
		DETOUR_STATIC_CALL(args);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:ChangeLevel")
		{
			MOD_ADD_DETOUR_MEMBER(CChangeLevelIssue_CanCallVote,    "CChangeLevelIssue::CanCallVote");
			MOD_ADD_DETOUR_MEMBER(CChangeLevelIssue_ExecuteCommand, "CChangeLevelIssue::ExecuteCommand");
			MOD_ADD_DETOUR_MEMBER(CVEngineServer_ChangeLevel,       "CVEngineServer::ChangeLevel");
			MOD_ADD_DETOUR_STATIC(Host_Changelevel_f,               "Host_Changelevel_f");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_changelevel", "0", FCVAR_NOTIFY,
		"Debug: diagnose problems with changelevel vote starting around 5/31/2016",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
