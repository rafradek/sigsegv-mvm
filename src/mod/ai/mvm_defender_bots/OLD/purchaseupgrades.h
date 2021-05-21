#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_PURCHASEUPGRADES_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_PURCHASEUPGRADES_H_


#include "re/nextbot.h"
#include "stub/tfbot.h"


namespace Mod::AI::MvM_Defender_Bots
{
	struct UpgradeInfo
	{
		UpgradeInfo(int pclass, int slot, int index) :
			pclass(pclass), slot(slot), index(index)
		{
			this->random = RandomInt(INT_MIN, INT_MAX);
		}
		
		int pclass;
		int slot;
		int index;
		int random;
	};
	
	
	/* stand at an upgrade station and purchase upgrades */
	class CTFBotPurchaseUpgrades : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotPurchaseUpgrades() {}
		
		virtual const char *GetName() const override { return "PurchaseUpgrades"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
		
	private:
		float GetUpgradeInterval() const
		{
			constexpr float interval = 2.0f;
			constexpr float variance = 0.2f;
			
			return RandomFloat(interval - variance, interval + variance);
		}
		
		bool IsStandingAtUpgradeStation() const
		{
			return this->GetActor()->m_Shared->m_bInUpgradeZone;
		}
		
		void CollectUpgrades();
		const UpgradeInfo *ChooseUpgrade();
		void PurchaseUpgrade(const UpgradeInfo *info);
		
		void KV_MvM_UpgradesBegin()
		{
			auto kv = new KeyValues("MvM_UpgradesBegin");
			serverGameClients->ClientCommandKeyValues(this->GetActor()->GetNetworkable()->GetEdict(), kv);
		}
		void KV_MvM_UpgradesDone()
		{
			auto kv = new KeyValues("MvM_UpgradesDone");
			kv->SetInt("num_upgrades", this->m_nPurchasedUpgrades);
			serverGameClients->ClientCommandKeyValues(this->GetActor()->GetNetworkable()->GetEdict(), kv);
		}
		void KV_MVM_Upgrade(int count, int slot, int index)
		{
			auto kv = new KeyValues("MVM_Upgrade");
			auto subkey = kv->FindKey("upgrade", true);
			subkey->SetInt("count",    count);
			subkey->SetInt("itemslot", slot);
			subkey->SetInt("upgrade",  index);
			serverGameClients->ClientCommandKeyValues(this->GetActor()->GetNetworkable()->GetEdict(), kv);
		}
		
		CountdownTimer m_ctNextUpgrade;
		std::vector<UpgradeInfo> m_Upgrades;
		int m_nPurchasedUpgrades = 0;
	};
}


#endif
