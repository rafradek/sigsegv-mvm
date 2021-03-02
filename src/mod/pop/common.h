#ifndef _INCLUDE_SIGSEGV_MOD_POP_COMMON_H_
#define _INCLUDE_SIGSEGV_MOD_POP_COMMON_H_

#include "util/autolist.h"
#include "stub/tfplayer.h"
#include "stub/strings.h"

struct ForceItems
{
	std::vector<std::pair<std::string, int>> items[11] = {};
	std::vector<std::pair<std::string, int>> items_no_remove[11] = {};
};

static void ApplyForceItemsClass(std::vector<std::pair<std::string, int>> &items, CTFPlayer *player, bool no_remove, bool respect_class, bool mark)
{
    for (auto &pair : items) {
        DevMsg("Apply item %s %d\n", pair.first.c_str(), pair.second);
        CEconEntity *entity = static_cast<CEconEntity *>(ItemGeneration()->SpawnItem(pair.second, Vector(0,0,0), QAngle(0,0,0), 1, 6, nullptr));
        if (entity != nullptr) {
            if (mark) {
                entity->GetItem()->GetAttributeList().SetRuntimeAttributeValue(GetItemSchema()->GetAttributeDefinitionByName("is force item"), 1.0f);
            }
            if (!GiveItemToPlayer(player, entity, no_remove, respect_class, pair.first.c_str()))
            {
                entity->Remove();
            }

        }
    }
}

static void ApplyForceItems(ForceItems &force_items, CTFPlayer *player, bool mark)
{
    DevMsg("Apply item\n");
    int player_class = player->GetPlayerClass()->GetClassIndex();
    ApplyForceItemsClass(force_items.items[0], player, false, false, mark);
    ApplyForceItemsClass(force_items.items_no_remove[0], player, true, false, mark);
    ApplyForceItemsClass(force_items.items[player_class], player, false, false, mark);
    ApplyForceItemsClass(force_items.items_no_remove[player_class], player, true, false, mark);
    ApplyForceItemsClass(force_items.items[10], player, false, true, mark);
    ApplyForceItemsClass(force_items.items_no_remove[10], player, true, true, mark);
}

static void Parse_ForceItem(KeyValues *kv, ForceItems &force_items, bool noremove)
{
    
    if (kv->GetString() != nullptr)
    {
        CEconItemDefinition *item_def = GetItemSchema()->GetItemDefinitionByName(kv->GetString());
        
        DevMsg("Parse item %s\n", kv->GetString());
        if (item_def != nullptr) {
            auto &items = noremove ? force_items.items_no_remove : force_items.items;
            items[10].push_back({kv->GetString(), item_def->m_iItemDefIndex});
            DevMsg("Add\n");
        }
    }
    FOR_EACH_SUBKEY(kv, subkey) {
        int classname = 0;
        for(int i=1; i < 10; i++){
            if(FStrEq(g_aRawPlayerClassNames[i],subkey->GetName())){
                classname=i;
                break;
            }
        }
        FOR_EACH_SUBKEY(subkey, subkey2) {
            CEconItemDefinition *item_def = GetItemSchema()->GetItemDefinitionByName(subkey2->GetString());
            if (item_def != nullptr) {
                auto &items = noremove ? force_items.items_no_remove : force_items.items;
                items[classname].push_back({subkey2->GetString(), item_def->m_iItemDefIndex});
            }
        }
    }
    
    DevMsg("Parsed attributes\n");
}
#endif
