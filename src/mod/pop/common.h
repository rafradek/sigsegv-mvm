#ifndef _INCLUDE_SIGSEGV_MOD_POP_COMMON_H_
#define _INCLUDE_SIGSEGV_MOD_POP_COMMON_H_

#include "util/autolist.h"
#include "stub/tfbot.h"
#include "stub/strings.h"

#include "re/path.h"


	
static int GetSlotFromString(const char *string) {
    int slot = -1;
    if (V_stricmp(string, "Primary") == 0)
        slot = 0;
    else if (V_stricmp(string, "Secondary") == 0)
        slot = 1;
    else if (V_stricmp(string, "Melee") == 0)
        slot = 2;
    else if (V_stricmp(string, "Utility") == 0)
        slot = 3;
    else if (V_stricmp(string, "Building") == 0)
        slot = 4;
    else if (V_stricmp(string, "PDA") == 0)
        slot = 5;
    else if (V_stricmp(string, "PDA2") == 0)
        slot = 6;
    else if (V_stricmp(string, "Head") == 0)
        slot = 7;
    else if (V_stricmp(string, "Misc") == 0)
        slot = 8;
    else if (V_stricmp(string, "Action") == 0)
        slot = 9;
    else if (V_stricmp(string, "Misc2") == 0)
        slot = 10;
    else
        slot = strtol(string, nullptr, 10);
    return slot;
}

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

extern std::map<int, std::string> g_Itemnames;
extern std::map<int, std::string> g_Attribnames;

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

struct PeriodicTaskImpl 
{
    float when = 10;
    float cooldown = 10;
    int repeats = -1;
    PeriodicTaskType type;
    int spell_type = 0;
    int spell_count = 1;
    int max_spells = 0;
    float duration = 0.1f;
    bool if_target = false;
    int health_below = 0;
    int health_above = 0;
    std::string attrib_name;
    std::string input_name;
    std::string param;
};

struct DelayedAddCond
{
    CHandle<CTFBot> bot;
    float when;
    ETFCond cond;
    float duration;
    int health_below = 0;
    int health_above = 0;
    
};

struct PeriodicTask
{
    CHandle<CTFBot> bot;
    PeriodicTaskType type;
    float delay = 10;
    float when = 10;
    float cooldown = 10;
    int repeats = 0;
    float duration = 0.1f;
    bool if_target = false;
    std::string attrib_name;
    std::string input_name;
    std::string param;

    int spell_type=0;
    int spell_count=1;
    int max_spells=0;

    int health_below = 0;
    int health_above = 0;
};

class CTFBotMoveTo : public IHotplugAction<CTFBot>
{
public:
    CTFBotMoveTo() {}
    virtual ~CTFBotMoveTo() {
        
    }

    void SetTargetPos(Vector &target_pos)
    {
        this->m_TargetPos = target_pos;
    }

    void SetTargetPosEntity(CBaseEntity *target)
    {
        this->m_hTarget = target;
    }

    void SetTargetAimPos(Vector &target_aim)
    {
        this->m_TargetAimPos = target_aim;
    }

    void SetTargetAimPosEntity(CBaseEntity *target)
    {
        this->m_hTargetAim = target;
    }

    void SetDuration(float duration)
    {
        this->m_fDuration = duration;
    }

    void SetKillLook(bool kill_look)
    {
        this->m_bKillLook = kill_look;
    }
    
    void SetWaitUntilDone(bool wait_until_done)
    {
        this->m_bWaitUntilDone = wait_until_done;
    }

    void SetOnDoneAttributes(std::string on_done_attributes)
    {
        this->m_strOnDoneAttributes = on_done_attributes;
    }

    virtual const char *GetName() const override { return "Interrupt Action"; }

    virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
    virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
    virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd) override;

private:

    Vector m_TargetPos = vec3_origin;
    Vector m_TargetAimPos = vec3_origin;
    
    CHandle<CBaseEntity> m_hTarget;
    CHandle<CBaseEntity> m_hTargetAim;

    float m_fDuration = 0.0f;
    bool m_bDone = false;
    bool m_bWaitUntilDone = false;

    bool m_bKillLook = false;
    std::string m_strOnDoneAttributes = "";
    PathFollower m_PathFollower;
    CountdownTimer m_ctRecomputePath;
    CountdownTimer m_ctDoneAction;
};

const char *GetItemName(int item_defid);

class ItemListEntry
{
public:
    virtual ~ItemListEntry() = default;
    virtual bool Matches(const char *classname, const CEconItemView *item_view) const = 0;
    virtual const char *GetInfo() const = 0;
};

class ItemListEntry_Classname : public ItemListEntry
{
public:
    ItemListEntry_Classname(const char *classname) : m_strClassname(classname) 
    {
        wildcard = !m_strClassname.empty() && m_strClassname[m_strClassname.size() - 1] == '*';
    }
    
    virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
    {
        if (classname == nullptr) return false;
        if (wildcard)
            return strnicmp(classname, m_strClassname.c_str(), m_strClassname.size() - 1) == 0;
        else
            return FStrEq(this->m_strClassname.c_str(), classname);
    }
    
    virtual const char *GetInfo() const override
    {
        static char buf[64];
        if (strnicmp(m_strClassname.c_str(), "tf_weapon_", strlen("tf_weapon_")) == 0) {
            snprintf(buf, sizeof(buf), "Weapon type: %s", m_strClassname.c_str() + strlen("tf_weapon_"));
        }
        else {
            snprintf(buf, sizeof(buf), "Item type: %s", m_strClassname.c_str());
        }
        
        return buf;
    }

private:
    bool wildcard;
    std::string m_strClassname;
};

class ItemListEntry_Name : public ItemListEntry
{
public:
    ItemListEntry_Name(const char *name) : m_strName(name) {}
    
    virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
    {
        if (item_view == nullptr) return false;
        return FStrEq(this->m_strName.c_str(), item_view->GetStaticData()->GetName(""));
    }

    virtual const char *GetInfo() const override
    {
        auto item_def = GetItemSchema()->GetItemDefinitionByName(m_strName.c_str());
        
        if (item_def != nullptr) {
            auto find = g_Itemnames.find(item_def->m_iItemDefIndex);
            if (find != g_Itemnames.end()) {
                return find->second.c_str();
            }
        }
        return m_strName.c_str();
    }
    
private:
    std::string m_strName;
};

class ItemListEntry_DefIndex : public ItemListEntry
{
public:
    ItemListEntry_DefIndex(int def_index) : m_iDefIndex(def_index) {}
    
    virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
    {
        if (item_view == nullptr) return false;
        return (this->m_iDefIndex == item_view->GetItemDefIndex());
    }
    
    virtual const char *GetInfo() const override
    {
        static char buf[6];
        const char *name = GetItemName(m_iDefIndex);
        if (name != nullptr) {
            return name;
        }
        snprintf(buf, sizeof(buf), "%d", m_iDefIndex);
        return buf;
    }
    
private:
    int m_iDefIndex;
};

class ItemListEntry_ItemSlot : public ItemListEntry
{
public:
    ItemListEntry_ItemSlot(const char *slot) : m_iSlot(GetSlotFromString(slot)) {}
    
    virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
    {
        if (item_view == nullptr) return false;
        return (this->m_iSlot == item_view->GetItemDefinition()->GetLoadoutSlot(TF_CLASS_UNDEFINED));
    }
    
    virtual const char *GetInfo() const override
    {
        static char buf[64];
        if (m_iSlot >= 0) {
            snprintf(buf, sizeof(buf), "Weapon in slot: %s", g_szLoadoutStrings[m_iSlot]);
        }
        else {
            return "Null";
        }
        
        return buf;
    }
    
private:
    int m_iSlot;
};

struct ItemAttributes
{
    std::unique_ptr<ItemListEntry> entry;
    std::map<CEconItemAttributeDefinition *, std::string> attributes;
};

void UpdateDelayedAddConds(std::vector<DelayedAddCond> &delayed_addconds);
void UpdatePeriodicTasks(std::vector<PeriodicTask> &pending_periodic_tasks);

void ApplyForceItemsClass(std::vector<ForceItem> &items, CTFPlayer *player, bool no_remove, bool respect_class, bool mark);

static void ApplyForceItems(ForceItems &force_items, CTFPlayer *player, bool mark)
{
    DevMsg("Apply item\n");
    int player_class = player->GetPlayerClass()->GetClassIndex();
    ApplyForceItemsClass(force_items.items[0], player, false, false, mark);
    ApplyForceItemsClass(force_items.items_no_remove[0], player, true, false, mark);
    ApplyForceItemsClass(force_items.items[player_class], player, false, false, mark);
    ApplyForceItemsClass(force_items.items_no_remove[player_class], player, true, false, mark);
    ApplyForceItemsClass(force_items.items[11], player, false, true, mark);
    ApplyForceItemsClass(force_items.items_no_remove[11], player, true, true, mark);
}

static std::unique_ptr<ItemListEntry> Parse_ItemListEntry(KeyValues *kv, const char *name) 
{
    if (FStrEq(kv->GetName(), "Classname")) {
        DevMsg("%s: Add Classname entry: \"%s\"\n", name, kv->GetString());
        return std::make_unique<ItemListEntry_Classname>(kv->GetString());
    } else if (FStrEq(kv->GetName(), "Name") || FStrEq(kv->GetName(), "ItemName") || FStrEq(kv->GetName(), "Item")) {
        DevMsg("%s: Add Name entry: \"%s\"\n", name, kv->GetString());
        return std::make_unique<ItemListEntry_Name>(kv->GetString());
    } else if (FStrEq(kv->GetName(), "DefIndex")) {
        DevMsg("%s: Add DefIndex entry: %d\n", name, kv->GetInt());
        return std::make_unique<ItemListEntry_DefIndex>(kv->GetInt());
    } else if (FStrEq(kv->GetName(), "ItemSlot")) {
        DevMsg("%s: Add ItemSlot entry: %s\n", name, kv->GetString());
    return std::make_unique<ItemListEntry_ItemSlot>(kv->GetString());
    } else {
        DevMsg("%s: Found DEPRECATED entry with key \"%s\"; treating as Classname entry: \"%s\"\n", name, kv->GetName(), kv->GetString());
        return std::make_unique<ItemListEntry_Classname>(kv->GetString());
    }
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
bool Parse_PeriodicTask(std::vector<PeriodicTaskImpl> &periodic_tasks, KeyValues *kv, const char *type_name);
void ApplyPendingTask(CTFBot *bot, std::vector<PeriodicTaskImpl> &periodic_tasks, std::vector<PeriodicTask> &pending_periodic_tasks);
void ApplyAddCond(CTFBot *bot, std::vector<AddCond> &addconds, std::vector<DelayedAddCond> &delayed_addconds);

bool LoadUserDataFile(CRC32_t &value, const char *filename);

void LoadItemNames();
bool FormatAttributeString(std::string &string, CEconItemAttributeDefinition *attr_def, attribute_data_union_t value);


#endif
