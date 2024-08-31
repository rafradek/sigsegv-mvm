#ifndef _INCLUDE_SIGSEGV_MOD_ITEM_ITEM_COMMON_H_
#define _INCLUDE_SIGSEGV_MOD_ITEM_ITEM_COMMON_H_

#include "stub/strings.h"
#include "stub/misc.h"

const char *GetItemName(const CEconItemView *view);
const char *GetItemName(const CEconItemView *view, bool &is_custom);
const char *GetItemNameForDisplay(int item_defid, CTFPlayer *player = nullptr);
const char *GetItemNameForDisplay(const CEconItemView *view, CTFPlayer *player = nullptr);
const char *GetAttributeName(int attributeIndex, CTFPlayer *player = nullptr);
void LoadItemNames(uint langNum);
bool FormatAttributeString(std::string &string, CEconItemAttributeDefinition *attr_def, attribute_data_union_t value, CTFPlayer *player = nullptr, bool shortDescription = false);

class ItemDefLanguage
{
public:
    bool m_bInitialized = false;
    bool m_bLoaded = false;
    std::string m_Name;
    std::unordered_map<int, std::string> m_Itemnames;
    std::unordered_map<int, std::string> m_Attribnames;
    std::unordered_map<int, std::string> m_AttribnamesShort;
};

extern std::vector<ItemDefLanguage> g_ItemLanguages;
extern ItemDefLanguage *g_ItemDefaultLanguage;

static const char *loadoutStrings[] = 
{
    // Weapons & Equipment
    "Primary",		// LOADOUT_POSITION_PRIMARY = 0,
    "Secondary",	// LOADOUT_POSITION_SECONDARY,
    "Melee",		// LOADOUT_POSITION_MELEE,
    "Utility",		// LOADOUT_POSITION_UTILITY,
    "Building",		// LOADOUT_POSITION_BUILDING,
    "PDA",			// LOADOUT_POSITION_PDA,
    "PDA 2",			// LOADOUT_POSITION_PDA2,

    // Wearables
    "Head",			// LOADOUT_POSITION_HEAD,
    "Misc",			// LOADOUT_POSITION_MISC,
    "Action",		// LOADOUT_POSITION_ACTION,
    "Misc 2",   	// LOADOUT_POSITION_MISC2

    "Taunt 1",		// LOADOUT_POSITION_TAUNT
    "Taunt 2",		// LOADOUT_POSITION_TAUNT2
    "Taunt 3",		// LOADOUT_POSITION_TAUNT3
    "Taunt 4",		// LOADOUT_POSITION_TAUNT4
    "Taunt 5",		// LOADOUT_POSITION_TAUNT5
    "Taunt 6",		// LOADOUT_POSITION_TAUNT6
    "Taunt 7",		// LOADOUT_POSITION_TAUNT7
    "Taunt 8",		// LOADOUT_POSITION_TAUNT8
};
	
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
    else if (V_stricmp(string, "Taunt") == 0)
        slot = 11;
    else if (V_stricmp(string, "Taunt2") == 0)
        slot = 12;
    else if (V_stricmp(string, "Taunt3") == 0)
        slot = 13;
    else if (V_stricmp(string, "Taunt4") == 0)
        slot = 14;
    else if (V_stricmp(string, "Taunt5") == 0)
        slot = 15;
    else if (V_stricmp(string, "Taunt6") == 0)
        slot = 16;
    else if (V_stricmp(string, "Taunt7") == 0)
        slot = 17;
    else if (V_stricmp(string, "Taunt8") == 0)
        slot = 18;
    else
        slot = strtol(string, nullptr, 10);
    return slot;
}

inline ItemDefLanguage *GetItemLanguage(CTFPlayer *player) {
    uint langNum = player == nullptr ? translator->GetServerLanguage() : translator->GetClientLanguage(player->entindex());
    auto lang = &g_ItemLanguages[langNum];
    if (!lang->m_bLoaded) {
        LoadItemNames(langNum);
    }
    return lang;
}

class ItemListEntry
{
public:
    virtual ~ItemListEntry() = default;
    virtual bool Matches(const char *classname, const CEconItemView *item_view) const = 0;
    virtual const char *GetInfo(CTFPlayer *player) const = 0;
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
        
        if (item_view != nullptr) {
            bool isCustom = false;
            GetItemName(item_view, isCustom);
            if (isCustom) return false;
        }

        if (wildcard)
            return strnicmp(classname, m_strClassname.c_str(), m_strClassname.size() - 1) == 0;
        else
            return FStrEq(this->m_strClassname.c_str(), classname);
    }
    
    virtual const char *GetInfo(CTFPlayer *player) const override
    {
        static char buf[128];
        auto item_def = GetItemSchema()->GetItemDefinitionByName(m_strClassname.c_str());
        if (strnicmp(m_strClassname.c_str(), "tf_weapon_", strlen("tf_weapon_")) == 0) {
            strcpy(buf, FormatTextForPlayerSM(player, 2, "%t", "Weapon type:", item_def != nullptr ? GetItemNameForDisplay(item_def->m_iItemDefIndex, player) : m_strClassname.c_str() + strlen("tf_weapon_")));
        }
        else {
            strcpy(buf, FormatTextForPlayerSM(player, 2, "%t", "Item type:", item_def != nullptr ? GetItemNameForDisplay(item_def->m_iItemDefIndex, player) : m_strClassname.c_str()));
        }
        V_StrSubst(buf,"_", " ", buf, 128);
        
        return buf;
    }

private:
    bool wildcard;
    std::string m_strClassname;
};

// Item is similar, if:
// Name matches
// The item is not custom, and:
// base_item_name matches
// base_item_name is not empty or item_logname matches
// Or if the the item_view is an all class melee weapon and is compared to a base class melee weapon
bool AreItemsSimilar(const CEconItemView *item_view, bool compare_by_log_name, const std::string &base_name, const std::string &log_name, const std::string &base_melee_class, const char *classname, int base_defindex);

class ItemListEntry_Similar : public ItemListEntry
{
public:
    ItemListEntry_Similar(const char *name);
    virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
    {
        if (item_view == nullptr) return false;

        bool is_custom = false;
        const char *name =  GetItemName(item_view, is_custom);

        if (FStrEq(this->m_strName.c_str(),name)) return true;

        return !is_custom && AreItemsSimilar(item_view, m_bCanCompareByLogName, m_strBaseName, m_strLogName, m_strBaseClassMelee, classname, m_iBaseDefIndex);
    }

    virtual const char *GetInfo(CTFPlayer *player) const override
    {
        auto item_def = GetItemSchema()->GetItemDefinitionByName(m_strName.c_str());
        
        if (item_def != nullptr) {
            return GetItemNameForDisplay(item_def->m_iItemDefIndex, player);
        }
        return m_strName.c_str();
    }
    
private:

    std::string m_strName;
    bool m_bCanCompareByLogName;
    std::string m_strLogName;
    std::string m_strBaseName;
    std::string m_strBaseClassMelee;
    int m_iBaseDefIndex;
};

class ItemListEntry_Name : public ItemListEntry
{
public:
    ItemListEntry_Name(const char *name) : m_strName(name) {}
    
    virtual bool Matches(const char *classname, const CEconItemView *item_view) const override
    {
        if (item_view == nullptr) return false;

        return FStrEq(this->m_strName.c_str(), GetItemName(item_view)); 
    }

    virtual const char *GetInfo(CTFPlayer *player) const override
    {
        auto item_def = GetItemSchema()->GetItemDefinitionByName(m_strName.c_str());
        
        if (item_def != nullptr) {
            return GetItemNameForDisplay(item_def->m_iItemDefIndex, player);
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
    
    virtual const char *GetInfo(CTFPlayer *player) const override
    {
        static char buf[6];
        const char *name = GetItemNameForDisplay(m_iDefIndex, player);
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
    
    virtual const char *GetInfo(CTFPlayer *player) const override
    {
        static char buf[64];
        if (m_iSlot >= 0) {
            strcpy(buf, FormatTextForPlayerSM(player, 2, "%t %t", "Weapon in slot:", loadoutStrings[m_iSlot]));
        }
        else {
            return "Null";
        }
        
        return buf;
    }
    
private:
    int m_iSlot;
};

static std::unique_ptr<ItemListEntry> Parse_ItemListEntry(const char* type, const char* value, const char *name, bool parse_default = true) 
{
    std::unique_ptr<ItemListEntry> ptr;
    bool fallback = false;
    if (FStrEq(type, "Classname")) {
        ptr = std::make_unique<ItemListEntry_Classname>(value);
    } else if (FStrEq(type, "Name") || FStrEq(type, "ItemName") || FStrEq(type, "Item")) {
        ptr = std::make_unique<ItemListEntry_Name>(value);
    } else if (FStrEq(type, "SimilarToItem")) {
        ptr = std::make_unique<ItemListEntry_Similar>(value);
    } else if (FStrEq(type, "DefIndex")) {
        ptr = std::make_unique<ItemListEntry_DefIndex>(std::stoi(value));
    } else if (FStrEq(type, "ItemSlot")) {
        ptr = std::make_unique<ItemListEntry_ItemSlot>(value);
    } else {
        if (parse_default && name != nullptr) {
            DevMsg("%s: Found DEPRECATED entry with key \"%s\"; treating as Classname entry: \"%s\"\n", name, type, value);
        }
        ptr = parse_default ? std::make_unique<ItemListEntry_Classname>(value) : std::unique_ptr<ItemListEntry>(nullptr);
    }
    if (name != nullptr) {
        if (ptr) {
            DevMsg("%s: Add %s entry: \"%s\"\n", name, type, value);
        }
        else {
            DevMsg("%s: Cannot add item %s entry: \"%s\"\n", name, type, value);
        }
    }
    return ptr;
}

CTFWeaponBase *CreateCustomWeaponModelPlaceholder(CTFPlayer *owner, CTFWeaponBase *weapon, int modelIndex);
void CopyVisualAttributes(CTFPlayer *player, CEconEntity *copyFrom, CEconEntity *copyTo);
#endif