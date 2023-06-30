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
	std::vector<ForceItem> items[TF_CLASS_COUNT+2] = {};
	std::vector<ForceItem> items_no_remove[TF_CLASS_COUNT+2] = {};
    bool parsed = false;

    void Clear() {
        
        for (int i=0; i < TF_CLASS_COUNT+2; i++)
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
    bool if_no_target = false;
    float if_range_target_min = 0.0f;
    float if_range_target_max = -1.0f;

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

void ApplyForceItemsClass(std::vector<ForceItem> &items, CTFPlayer *player, bool no_remove, bool no_respect_class, bool mark);

static void ApplyForceItems(ForceItems &force_items, CTFPlayer *player, bool mark, bool remove_items_only = false)
{
    DevMsg("Apply item\n");
    int player_class = player->GetPlayerClass()->GetClassIndex();
    ApplyForceItemsClass(force_items.items[0], player, false, false, mark);
    ApplyForceItemsClass(force_items.items[player_class], player, false, true, mark);
    ApplyForceItemsClass(force_items.items[TF_CLASS_COUNT+1], player, false, true, mark);

    if (remove_items_only) {
        ApplyForceItemsClass(force_items.items_no_remove[0], player, true, false, mark);
        ApplyForceItemsClass(force_items.items_no_remove[player_class], player, true, true, mark);
        ApplyForceItemsClass(force_items.items_no_remove[TF_CLASS_COUNT+1], player, true, true, mark);
    }
}

static std::unique_ptr<ItemListEntry> Parse_ItemListEntry(KeyValues *kv, const char *name, bool parse_default = true) 
{
    return Parse_ItemListEntry(kv->GetName(), kv->GetString(), name, parse_default);
}

void Parse_ForceItem(KeyValues *kv, ForceItems &force_items, bool noremove);

void Parse_ItemAttributes(KeyValues *kv, std::vector<ItemAttributes> &attibs);

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
