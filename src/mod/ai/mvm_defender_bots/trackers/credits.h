#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_TRACKERS_CREDITS_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_TRACKERS_CREDITS_H_


#include "mod/ai/mvm_defender_bots/trackers.h"
#include "stub/tfplayer.h"
#include "stub/tfentities.h"


namespace Mod::AI::MvM_Defender_Bots
{
	struct CreditInfo
	{
		std::map<CHandle<CBasePlayer>, float> t_lastseen;
		
		bool WasEverSeen() const
		{
			return !t_lastseen.empty();
		}
		
		float GetTimeSinceLastSeen() const
		{
			float t_least = FLT_MAX;
			
			for (const auto& pair : t_lastseen) {
				t_least = Min(t_least, gpGlobals->curtime - pair.second);
			}
			
			return t_least;
		}
	};
	
	
	class CreditTracker : public ITracker
	{
	public:
		virtual int GetFrameInterval() const override { return 1; }
		virtual void Update() override;
		
		CCurrencyPack *SelectCurrencyPack() const;
		
	private:
		std::map<CHandle<CCurrencyPack>, CreditInfo> m_CreditInfos;
		
//	public:
//		decltype(m_CreditInfos)& GetCreditInfos() { return this->m_CreditInfos; }
	};
}


#endif
