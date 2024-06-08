#include "mod.h"


class CVProfRecorder
{
public:
	static void DumpNode(const char *prefix, CVProfNode *pNode)
	{
		DevMsg("[%s] %-40s | %6d | %6u %12llu | %6u %12llu | %6u %12llu | %12llu\n", prefix,
			pNode->m_pszName, pNode->m_nRecursions,
			pNode->m_nCurFrameCalls, pNode->m_CurFrameTime.GetLongCycles(),
			pNode->m_nPrevFrameCalls, pNode->m_PrevFrameTime.GetLongCycles(),
			pNode->m_nTotalCalls, pNode->m_TotalTime.GetLongCycles(),
			pNode->m_PeakTime.GetLongCycles());
	}
};


namespace Mod::Debug::VProf_Record
{
	DETOUR_DECL_MEMBER(void, CVProfRecorder_Record_WriteTimings_R, CVProfNode *pIn)
	{
		CVProfRecorder::DumpNode("record   ", pIn);
		DETOUR_MEMBER_CALL(pIn);
	}
	
	DETOUR_DECL_MEMBER(void, CVProfNode_MarkFrame)
	{
		CVProfRecorder::DumpNode("mark PRE ", reinterpret_cast<CVProfNode *>(this));
		DETOUR_MEMBER_CALL();
		CVProfRecorder::DumpNode("mark POST", reinterpret_cast<CVProfNode *>(this));
	}
	
	
	DETOUR_DECL_STATIC(void, VProfRecord_Snapshot)
	{
		/* skip the call */
	}
	DETOUR_DECL_STATIC(void, VProfRecord_StartOrStop)
	{
		DETOUR_STATIC_CALL();
		DETOUR_STATIC_CALL();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:VProf_Record")
		{
//			MOD_ADD_DETOUR_MEMBER(CVProfRecorder_Record_WriteTimings_R, "CVProfRecorder::Record_WriteTimings_R");
//			MOD_ADD_DETOUR_MEMBER(CVProfNode_MarkFrame,                 "CVProfNode::MarkFrame");
			
			MOD_ADD_DETOUR_STATIC(VProfRecord_Snapshot,    "VProfRecord_Snapshot");
			MOD_ADD_DETOUR_STATIC(VProfRecord_StartOrStop, "VProfRecord_StartOrStop");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_vprof_record", "0", FCVAR_NOTIFY,
		"Debug: figure out why vprof recording is writing zeroes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


// FINDINGS:
// CVProfRecorder::Record_WriteTimings_R is happening AFTER nodes MarkFrame, but
// should be BEFORE MarkFrame occurs
