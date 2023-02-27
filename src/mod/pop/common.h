#ifndef _INCLUDE_SIGSEGV_MOD_POP_COMMON_H_
#define _INCLUDE_SIGSEGV_MOD_POP_COMMON_H_

#include "util/autolist.h"
#include "stub/tfbot.h"
#include "stub/strings.h"
#include "mod/item/item_common.h"

#include "re/path.h"

static int SPELL_TYPE_COUNT=12;
static int SPELL_TYPE_COUNT_ALL=15;
static const char *SPELL_TYPE[] = {
    "Fireball",
    "Ball O' Bats",
    "Healing Aura",
    "Pumpkin MIRV",
    "Superjump",
    "Invisibility",
    "Teleport",
    "Tesla Bolt",
    "Minify",
    "Meteor Shower",
    "Summon Monoculus",
    "Summon Skeletons",
    "Common",
    "Rare",
    "All"
};

class ItemListEntry;

struct ForceItem
{
    std::string name;
    CEconItemDefinition *definition;
    std::shared_ptr<ItemListEntry> entry;
};

struct ForceItems
{
	std::vector<ForceItem> items[12] = {};
	std::vector<ForceItem> items_no_remove[12] = {};
    bool parsed = false;

    void Clear() {
        
        for (int i=0; i < 12; i++)
        {
            items[i].clear();
            items_no_remove[i].clear();
        }
        parsed = false;
    }
};

struct AddCond
{
    ETFCond cond   = (ETFCond)-1;
    float duration = -1.0f;
    float delay    =  0.0f;
    int health_below = 0;
    int health_above = 0;
};

enum PeriodicTaskType 
{
    TASK_TAUNT,
    TASK_GIVE_SPELL,
    TASK_VOICE_COMMAND,
    TASK_FIRE_WEAPON,
    TASK_CHANGE_ATTRIBUTES,
    TASK_SPAWN_TEMPLATE,
    TASK_FIRE_INPUT,
    TASK_MESSAGE,
    TASK_WEAPON_SWITCH,
    TASK_ADD_ATTRIBUTE,
    TASK_REMOVE_ATTRIBUTE,
    TASK_SEQUENCE,
    TASK_CLIENT_COMMAND,
    TASK_INTERRUPT_ACTION,
    TASK_SPRAY
};

// struct PeriodicTaskImpl 
// {
//     float when = 10;
//     float cooldown = 10;
//     int repeats = -1;
//     PeriodicTaskType type;
//     int spell_type = 0;
//     int spell_count = 1;
//     int max_spells = 0;
//     float duration = 0.1f;
//     bool if_target = false;
//     int health_below = 0;
//     int health_above = 0;
//     std::string attrib_name;
//     std::string input_name;
//     std::string param;
// };

struct DelayedAddCond
{
    CHandle<CTFBot> bot;
    float when;
    ETFCond cond;
    float duration;
    int health_below = 0;
    int health_above = 0;
    
};

class PeriodicTask
{
public:
    PeriodicTaskType type;
    float delay = 10;
    float cooldown = 10;
    int repeats = 0;
    float duration = 0.1f;
    bool if_target = false;

    int health_below = 0;
    int health_above = 0;

    virtual void Parse(KeyValues *kv) = 0;
    virtual void Update(CTFBot *bot) = 0;
};

class PeriodicTaskImpl
{
public:
    CHandle<CTFBot> bot;
    std::shared_ptr<PeriodicTask> task;
    float nextTaskTime = 0;
    int repeatsLeft = 0;
};

struct ItemAttributes
{
    std::unique_ptr<ItemListEntry> entry;
    std::map<CEconItemAttributeDefinition *, std::string> attributes;
};

void UpdateDelayedAddConds(std::vector<DelayedAddCond> &delayed_addconds);
void UpdatePeriodicTasks(std::vector<PeriodicTaskImpl> &pending_periodic_tasks, bool insideECAttr = false);

void ApplyForceItemsClass(std::vector<ForceItem> &items, CTFPlayer *player, bool no_remove, bool respect_class, bool mark);

static void ApplyForceItems(ForceItems &force_items, CTFPlayer *player, bool mark, bool remove_items_only = false)
{
    DevMsg("Apply item\n");
    int player_class = player->GetPlayerClass()->GetClassIndex();
    ApplyForceItemsClass(force_items.items[0], player, false, false, mark);
    ApplyForceItemsClass(force_items.items[player_class], player, false, false, mark);
    ApplyForceItemsClass(force_items.items[11], player, false, true, mark);

    if (remove_items_only) {
        ApplyForceItemsClass(force_items.items_no_remove[0], player, true, false, mark);
        ApplyForceItemsClass(force_items.items_no_remove[player_class], player, true, false, mark);
        ApplyForceItemsClass(force_items.items_no_remove[11], player, true, true, mark);
    }
}

static std::unique_ptr<ItemListEntry> Parse_ItemListEntry(KeyValues *kv, const char *name, bool parse_default = true) 
{
    return Parse_ItemListEntry(kv->GetName(), kv->GetString(), name, parse_default);
}

static void Parse_ForceItem(KeyValues *kv, ForceItems &force_items, bool noremove)
{
    force_items.parsed = true;
    if (kv->GetString() != nullptr)
    {
        CEconItemDefinition *item_def = GetItemSchema()->GetItemDefinitionByName(kv->GetString());
        
        DevMsg("Parse item %s\n", kv->GetString());
        if (item_def != nullptr) {
            auto &items = noremove ? force_items.items_no_remove : force_items.items;
            items[0].push_back({kv->GetString(), item_def});
            DevMsg("Add\n");
        }
    }
    FOR_EACH_SUBKEY(kv, subkey) {
        int classname = 11;
        for(int i=1; i < 11; i++){
            if(FStrEq(g_aRawPlayerClassNames[i],subkey->GetName())){
                classname=i;
                break;
            }
        }
        FOR_EACH_SUBKEY(subkey, subkey2) {
            
            if (subkey2->GetFirstSubKey() != nullptr) {
                CEconItemDefinition *item_def = GetItemSchema()->GetItemDefinitionByName(subkey2->GetName());
                if (item_def != nullptr) {
                    auto &items = noremove ? force_items.items_no_remove : force_items.items;
                    items[classname].push_back({subkey2->GetString(), item_def, Parse_ItemListEntry(subkey2->GetFirstSubKey(), "ForceItem")});
                }
            }
            else {
                CEconItemDefinition *item_def = GetItemSchema()->GetItemDefinitionByName(subkey2->GetString());
                if (item_def != nullptr) {
                    auto &items = noremove ? force_items.items_no_remove : force_items.items;
                    items[classname].push_back({subkey2->GetString(), item_def, nullptr});
                }
            }
        }
    }
    
    DevMsg("Parsed attributes\n");
}

static void Parse_ItemAttributes(KeyValues *kv, std::vector<ItemAttributes> &attibs)
{
    ItemAttributes item_attributes;// = state.m_ItemAttributes.emplace_back();
    bool hasname = false;

    FOR_EACH_SUBKEY(kv, subkey) {
        //std::unique_ptr<ItemListEntry> key=std::make_unique<ItemListEntry_Classname>("");
        if (strnicmp(subkey->GetName(), "ItemEntry", strlen("ItemEntry")) == 0) {
            Parse_ItemAttributes(subkey, attibs);
        } else if (FStrEq(subkey->GetName(), "Classname")) {
            DevMsg("ItemAttrib: Add Classname entry: \"%s\"\n", subkey->GetString());
            hasname = true;
            item_attributes.entry = std::make_unique<ItemListEntry_Classname>(subkey->GetString());
        } else if (FStrEq(subkey->GetName(), "ItemName")) {
            hasname = true;
            DevMsg("ItemAttrib: Add Name entry: \"%s\"\n", subkey->GetString());
            item_attributes.entry = std::make_unique<ItemListEntry_Name>(subkey->GetString());
        } else if (FStrEq(subkey->GetName(), "SimilarToItem")) {
            hasname = true;
            DevMsg("ItemAttrib: Add SimilarTo entry: \"%s\"\n", subkey->GetString());
            item_attributes.entry = std::make_unique<ItemListEntry_Similar>(subkey->GetString());
        } else if (FStrEq(subkey->GetName(), "DefIndex")) {
            hasname = true;
            DevMsg("ItemAttrib: Add DefIndex entry: %d\n", subkey->GetInt());
            item_attributes.entry = std::make_unique<ItemListEntry_DefIndex>(subkey->GetInt());
        } else if (FStrEq(subkey->GetName(), "ItemSlot")) {
            hasname = true;
            DevMsg("ItemAttrib: Add ItemSlot entry: %s\n", subkey->GetString());
            item_attributes.entry = std::make_unique<ItemListEntry_ItemSlot>(subkey->GetString());
        } else {
            CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(subkey->GetName());
            
            if (attr_def == nullptr) {
                Warning("[popmgr_extensions] Error: couldn't find any attributes in the item schema matching \"%s\".\n", subkey->GetName());
            }
            else
                item_attributes.attributes[attr_def] = subkey->GetString();
        }
    }
    if (hasname) {

        attibs.push_back(std::move(item_attributes));//erase(item_attributes);
    }
}

static void ApplyItemAttributes(CEconItemView *item_view, CTFPlayer *player, std::vector<ItemAttributes> &item_attribs_vec) {

    // Item attributes are ignored when picking up dropped weapons
    float dropped_weapon_attr = 0.0f;
    FindAttribute(&item_view->GetAttributeList(), GetItemSchema()->GetAttributeDefinitionByName("is dropped weapon"), &dropped_weapon_attr);
    if (dropped_weapon_attr != 0.0f)
        return;

    DevMsg("ReapplyItemUpgrades %f\n", dropped_weapon_attr);

    bool found = false;
    const char *classname = item_view->GetItemDefinition()->GetItemClass();
    std::map<CEconItemAttributeDefinition *, std::string> *attribs;
    for (auto& item_attributes : item_attribs_vec) {
        if (item_attributes.entry->Matches(classname, item_view)) {
            attribs = &(item_attributes.attributes);
            found = true;
            break;
        }
    }
    if (found && attribs != nullptr) {
        CEconItemView *view = item_view;
        for (auto& entry : *attribs) {
            view->GetAttributeList().AddStringAttribute(entry.first, entry.second);
        }
    }
}

void Parse_AddCond(std::vector<AddCond> &addconds, KeyValues *kv);
bool Parse_PeriodicTask(std::vector<std::shared_ptr<PeriodicTask>> &periodic_tasks, KeyValues *kv, const char *type_name);
void ApplyPendingTask(CTFBot *bot, std::vector<std::shared_ptr<PeriodicTask>> &periodic_tasks, std::vector<PeriodicTaskImpl> &pending_periodic_tasks);
void ApplyAddCond(CTFBot *bot, std::vector<AddCond> &addconds, std::vector<DelayedAddCond> &delayed_addconds);

bool LoadUserDataFile(CRC32_t &value, const char *filename);

class ShootTemplateData;
CBaseAnimating * TemplateShootSpawn(std::vector<ShootTemplateData> &templates, CTFPlayer *player, CTFWeaponBase *weapon, bool &stopproj, std::function<CBaseAnimating *()> origShootFunc);

#endif
