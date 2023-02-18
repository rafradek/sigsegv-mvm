#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_TRACKERS_GATES_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_TRACKERS_GATES_H_


#include "mod/ai/mvm_defender_bots/trackers.h"
#include "stub/tfentities.h"


namespace Mod::AI::MvM_Defender_Bots
{
	struct GateInfo
	{
		
	};
	
	
	class GateTracker : public ITracker
	{
	public:
		virtual int GetFrameInterval() const override { return 1; }
		virtual void Update() override;
		
	private:
		std::map<CHandle<CTeamControlPoint>, GateInfo> m_GateInfos;
		
//	public:
//		decltype(m_GateInfos)& GetGateInfos() { return this->m_GateInfos; }
	};
}


#endif
