#include "stub/player_util.h"

void HealPlayer(CTFPlayer *patient, CTFPlayer *healer, CEconEntity *item, float amount, bool isOnHit, int damageType) 
{
    int healthPre = patient->GetHealth();
    patient->TakeHealth(amount, damageType);
    amount = patient->GetHealth() - healthPre;
    if (amount > 0) {

        if (healer != nullptr) {
            IGameEvent *event = gameeventmanager->CreateEvent("player_healed");
            if (event)
            {
                // HLTV event priority, not transmitted
                event->SetInt("priority", 1);	

                // Healed by another player.
                event->SetInt("patient", patient->GetUserID());
                event->SetInt("healer", healer->GetUserID());
                event->SetInt("amount", amount);
                gameeventmanager->FireEvent(event);
            }
        }

        if (isOnHit) {
            IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
            if (event)
            {
                event->SetInt("amount", amount);
                event->SetInt("entindex", ENTINDEX(patient));
                int healingItemDef = INVALID_ITEM_DEF_INDEX;
                if (item && item->GetItem() != nullptr)
                {
                    healingItemDef = item->GetItem()->GetItemDefIndex();
                }
                event->SetInt("weapon_def_index", healingItemDef);
                gameeventmanager->FireEvent(event); 
            }
        }
    }
}