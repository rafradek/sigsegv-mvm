#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_TRACKERS_TANKS_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_TRACKERS_TANKS_H_


#include "mod/ai/mvm_defender_bots/trackers.h"
#include "stub/tfentities.h"


namespace Mod::AI::MvM_Defender_Bots
{
	struct TankInfo
	{
		float progress;
		
	};
	
	
	class TankTracker : public ITracker
	{
	public:
		virtual int GetFrameInterval() const override { return 1; }
		virtual void Update() override;
		
	private:
		std::map<CHandle<CTFTankBoss>, TankInfo> m_TankInfos;
		
//	public:
//		decltype(m_TankInfos)& GetTankInfos() { return this->m_TankInfos; }
	};
}


#endif
