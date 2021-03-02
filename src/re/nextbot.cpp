#include "re/nextbot.h"
#include "stub/tfbot.h"


void IHotplugAction::UnloadAll()
{
	std::set<CTFBot *> bots;
	
	for (auto action : AutoList<IHotplugAction>::List()) {
		auto actor = action->GetActor();
		if (actor == nullptr) continue;
		
		bots.insert(actor);
	}
	
	Msg("IHotplugAction::UnloadAll: calling Reset() on %zu TFBot intention interfaces\n", bots.size());
	
	for (auto bot : bots) {
		bot->MyNextBotPointer()->GetIntentionInterface()->Reset();
	}
	
	assert(AutoList<IHotplugAction>::List().empty());
}
