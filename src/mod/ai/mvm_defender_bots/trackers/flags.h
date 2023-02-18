#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_TRACKERS_FLAGS_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_TRACKERS_FLAGS_H_


#include "mod/ai/mvm_defender_bots/trackers.h"
#include "stub/tfentities.h"


namespace Mod::AI::MvM_Defender_Bots
{
	struct FlagInfo
	{
		float hatch_path_dist;
		
	};
	
	
	class FlagTracker : public ITracker
	{
	public:
		virtual int GetFrameInterval() const override { return 1; }
		virtual void Update() override;
		
		CCaptureFlag *GetFrontFlag() const;
		
	private:
		std::map<CHandle<CCaptureFlag>, FlagInfo> m_FlagInfos;
		
//	public:
//		decltype(m_FlagInfos)& GetFlagInfos() { return this->m_FlagInfos; }
	};
}


#endif
