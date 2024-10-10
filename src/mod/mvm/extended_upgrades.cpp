#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/econ.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "util/iterate.h"
#include "util/misc.h"
#include "mod/pop/common.h"
#include "mod/pop/popmgr_extensions.h"
#include "stub/upgrades.h"
#include "stub/tfentities.h"
#include "stub/strings.h"
#include "stub/tf_objective_resource.h"
#include "stub/populators.h"
#include <fmt/core.h>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include "tier1/strtools.h"
#include "stub/extraentitydata.h"

namespace Mod::MvM::Extended_Upgrades
{
    int GetLoadoutSlotForItem(CTFItemDefinition *item, int class_index);
    void StartMenuForPlayer(CTFPlayer *player);
    void StartUpgradeListForPlayer(CTFPlayer *player, int slot, int displayitem = 0);
    void StopMenuForPlayer(CTFPlayer *player);

    bool from_buy_upgrade = false;
    
    class UpgradeCriteria {
    public:
        virtual bool Matches(CEconEntity *ent, CEconItemView *item, CTFPlayer *owner) = 0;
        virtual void ParseKey(KeyValues *kv) = 0;
    };

    class UpgradeCriteriaClassname : public UpgradeCriteria {
    public:
        UpgradeCriteriaClassname(std::string classname) : m_classname(classname) {}
        
        virtual bool Matches(CEconEntity *ent, CEconItemView *item, CTFPlayer *owner) override {
            return FStrEq(m_classname.c_str(), ent->GetClassname());
        }

        virtual void ParseKey(KeyValues *kv) override {
            if (FStrEq(kv->GetName(), "Classname"))
                m_classname = kv->GetString();
        }

    private:
        std::string m_classname;
    };

    class UpgradeCriteriaName : public UpgradeCriteria {
    public:
        UpgradeCriteriaName(std::string name) : m_name(name) {}
        
        virtual bool Matches(CEconEntity *ent, CEconItemView *item, CTFPlayer *owner) override {
            return FStrEq(m_name.c_str(), GetItemName(item));
        }

        virtual void ParseKey(KeyValues *kv) override {
            if (FStrEq(kv->GetName(), "Name"))
                m_name = kv->GetString();
        }

    private:
        std::string m_name;
        std::string m_strLogName;
    };

    class UpgradeCriteriaSimilar : public UpgradeCriteria
    {
    public:
        UpgradeCriteriaSimilar(const char *name) : m_strName(name)
        {
            auto itemDef = reinterpret_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(name));
            if (itemDef != nullptr && Mod::Pop::PopMgr_Extensions::GetCustomWeaponItemDef(name) == nullptr) {
                m_strLogName = itemDef->GetKeyValues()->GetString("item_logname");
                m_strBaseName = itemDef->GetKeyValues()->GetString("base_item_name");
                m_bCanCompareByLogName = !m_strLogName.empty() || !m_strBaseName.empty();
                m_iBaseDefIndex = itemDef->m_iItemDefIndex;
                m_strBaseClassMelee = itemDef->GetLoadoutSlot(TF_CLASS_UNDEFINED) == LOADOUT_POSITION_MELEE && FStrEq(itemDef->GetKeyValues()->GetString("item_quality"), "normal") ? itemDef->GetItemClass() : "";
            }
        }

        virtual bool Matches(CEconEntity *ent, CEconItemView *item_view, CTFPlayer *owner) override
        {
            if (item_view == nullptr) return false;

            bool is_custom = false;
            const char *name =  GetItemName(item_view, is_custom);
            if (FStrEq(this->m_strName.c_str(),name)) return true;

            return !is_custom && AreItemsSimilar(item_view, m_bCanCompareByLogName, m_strBaseName, m_strLogName, m_strBaseClassMelee, ent->GetClassname(), m_iBaseDefIndex);
        }

        virtual void ParseKey(KeyValues *kv) override {
        }
        
    private:

        std::string m_strName;
        bool m_bCanCompareByLogName;
        std::string m_strLogName;
        std::string m_strBaseName;
        std::string m_strBaseClassMelee;
        int m_iBaseDefIndex;
    };

    class UpgradeCriteriaHasDamage : public UpgradeCriteria {
    public:
        UpgradeCriteriaHasDamage(bool check) : m_bCheckDealsDamage(check) {}
        
        virtual bool Matches(CEconEntity *ent, CEconItemView *item, CTFPlayer *owner) override {
            auto weapon = rtti_cast<CTFWeaponBase *>(ent);
            if (weapon != nullptr) {
                int weaponid = weapon->GetWeaponID();
                bool dealsDamage = !(weaponid == TF_WEAPON_NONE || 
							weaponid == TF_WEAPON_LASER_POINTER || 
							weaponid == TF_WEAPON_MEDIGUN || 
							weaponid == TF_WEAPON_BUFF_ITEM ||
							weaponid == TF_WEAPON_BUILDER ||
							weaponid == TF_WEAPON_PDA_ENGINEER_BUILD ||
							weaponid == TF_WEAPON_PDA_ENGINEER_DESTROY ||
							weaponid == TF_WEAPON_PDA_SPY ||
							weaponid == TF_WEAPON_INVIS ||
							weaponid == TF_WEAPON_SPELLBOOK);
                return m_bCheckDealsDamage == dealsDamage;
            }
            return false;
        }

        virtual void ParseKey(KeyValues *kv) override {

        }
        bool m_bCheckDealsDamage = true;
    };

    class UpgradeCriteriaWeaponSlot : public UpgradeCriteria {
    public:
        UpgradeCriteriaWeaponSlot(int slot) : slot(slot) {}
        
        virtual bool Matches(CEconEntity *ent, CEconItemView *item, CTFPlayer *owner) override {

            return GetLoadoutSlotForItem(item->GetItemDefinition(), owner->GetPlayerClass()->GetClassIndex()) == this->slot;
        }

        virtual void ParseKey(KeyValues *kv) override {

        }
    private:
        int slot;
    };

    struct UpgradeInfo {
        int mvm_upgrade_index;
        CEconItemAttributeDefinition *attributeDefinition = nullptr;
        std::string attribute_name = "";
        int cost = 0;
        float increment = 0.25f;
        float cap = 2.0f;
        std::vector<std::unique_ptr<UpgradeCriteria>> criteria;
        std::vector<std::unique_ptr<UpgradeCriteria>> criteria_negate;
        std::vector<int> player_classes;
        std::string name = "";
        std::string description = "";
        bool playerUpgrade = false;
        bool hidden = false;
        std::string ref_name = "";
        std::vector<std::string> children;
        std::map<CEconItemAttributeDefinition *, float> secondary_attributes;
        std::vector<int> secondary_attributes_index;
        std::map<std::string, int> required_upgrades;
        std::map<std::string, int> required_upgrades_option;
        std::map<std::string, int> disallowed_upgrades;
        std::vector<std::unique_ptr<UpgradeCriteria>> required_weapons;
        int force_upgrade_slot = -2;
        int allow_wave_min = 0;
        int allow_wave_max = 9999;
        std::string required_weapons_string = "";
        bool show_requirements = true;
        //std::string on_upgrade_output = "";
        std::vector<std::string> on_upgrade_outputs = {};
        std::vector<std::string> on_apply_outputs = {};
        std::vector<std::string> on_downgrade_outputs = {};
        std::vector<std::string> on_restore_outputs = {};
        bool force_enable = false;
        int tier = 0;
        int uigroup = 0;
        bool is_v2_upgrade = false;
    };

    std::vector<UpgradeInfo *> upgrades; 
    std::vector<UpgradeInfo *> upgrades_v2; 
    std::map<std::string, UpgradeInfo *> upgrades_ref;
    std::vector<int> max_tier_upgrades;
    std::vector<int> min_tier_upgrades;

    //std::map<CHandle<CTFPlayer>, IBaseMenu *> select_type_menus;

    std::set<CHandle<CTFPlayer>> in_upgrade_zone;

    struct BoughtUpgradeInfo
    {
        int64 itemId = -1L;
        item_definition_index_t itemDefIndex = INVALID_ITEM_DEF_INDEX;
        int playerClassIndex = 0;
        int upgradeId = 0;
        int level = 0;
    };

    std::map<CSteamID, std::vector<BoughtUpgradeInfo>> bought_upgrades_v2;
    std::map<CSteamID, std::vector<BoughtUpgradeInfo>> bought_upgrades_v2_checkpoint;

    int extended_upgrades_start_index = -1;

    BoughtUpgradeInfo *FindUpgradeInfo(CEconEntity *item, CTFPlayer *player, UpgradeInfo *upgrade)
    {
        auto itemId = item != nullptr && item->GetItem() != nullptr ? item->GetItem()->m_iItemID.Get() : -1LL;
        auto itemDef = item != nullptr && item->GetItem() != nullptr ? item->GetItem()->m_iItemDefinitionIndex.Get() : INVALID_ITEM_DEF_INDEX;
        auto playerClass = player->GetPlayerClass()->GetClassIndex();
        for (auto &boughtUpgradeInfo : bought_upgrades_v2[player->GetSteamID()]) {
            if (itemId == boughtUpgradeInfo.itemId && itemDef == boughtUpgradeInfo.itemDefIndex && playerClass == boughtUpgradeInfo.playerClassIndex && upgrade->mvm_upgrade_index == boughtUpgradeInfo.upgradeId) {
                return &boughtUpgradeInfo;
            }
        }
        return nullptr;
    }

    bool BuyUpgrade(UpgradeInfo *upgrade,/* CEconEntity *item*/ int slot, CTFPlayer *player, bool free, bool downgrade);
    
    int GetLoadoutSlotForItem(CTFItemDefinition *item, int class_index) 
    {
        int slot = item->GetLoadoutSlot(class_index);
        if (slot == -1 && class_index != TF_CLASS_UNDEFINED && item->m_iItemDefIndex != 0) {
            slot = item->GetLoadoutSlot(TF_CLASS_UNDEFINED);
        }
        return slot;
    }

    ConVar sig_mvm_extended_upgrades_add_for_uninteded_class("sig_mvm_extended_upgrades_add_for_uninteded_class", "1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: add regular upgrades to extended upgrades menu when upgrading weapons for unintended class"); 

    bool IsWeaponUnintendedForPlayer(CTFPlayer *player, CEconEntity *entity)
    {
        return sig_mvm_extended_upgrades_add_for_uninteded_class.GetBool() && !Mod::Pop::PopMgr_Extensions::ExtendedUpgradesOnly() && entity->GetItem()->GetItemDefinition()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex()) == -1;
    }

    void FireOutputs(std::vector<std::string> &outputs, const variant_t &defaultValue, CBaseEntity *activator, CBaseEntity *caller)
    {
        for(const auto &output : outputs){
            
			char target[256];
            char action[256];
            char value[256];
            char delay[256];
            
			const char *psz = nexttoken(target, output.c_str(), ',');
            psz = nexttoken(action, psz, ',');
            psz = nexttoken(value, psz, ',');
            psz = nexttoken(delay, psz, ',');

            if(target[0] != '\0' && action[0] != '\0') {
                // Use provided default parameter
                variant_t actualvalue = defaultValue;
                if (value[0] != '\0') {
                    string_t stringvalue = AllocPooledString(value);
                    actualvalue.SetString(stringvalue);
                }
                
                CEventQueue &que = g_EventQueue;
                que.AddEvent(STRING(AllocPooledString(target)), STRING(AllocPooledString(action)), actualvalue, delay[0] != '\0' ? strtof(delay, nullptr) : 0, activator, caller, -1);
            }
        }
    }

    class SelectUpgradeWeaponHandler : public IMenuHandler
    {
    public:

        SelectUpgradeWeaponHandler(CTFPlayer * pPlayer) : IMenuHandler() {
            this->player = pPlayer;
            
        }

        void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item) {
            const char *info = menu->GetItemInfo(item, nullptr);
            int slotid = -1;
            if (info == nullptr)
                return;
                
            if (FStrEq(info, "player")) {
                slotid = -1;
            }
            else if (FStrEq(info, "refund")) {

                if (!TFGameRules()->CanPlayerUseRespec(this->player)) {
                    auto &vec = player->m_RefundableUpgrades.Get();
                    auto history = g_pPopulationManager->GetPlayerUpgradeHistory(this->player);
                    for (int i = vec.Count() - 1; i >= 0 ; i--) {
                        bool apply = false;
                        CEconItemView *itemA = nullptr;
                        if (vec[i].m_iPlayerClass == this->player->GetPlayerClass()->GetClassIndex()) {
                            if (vec[i].m_itemDefIndex != INVALID_ITEM_DEF_INDEX) {
                                for (int slot = 0; slot <= LOADOUT_POSITION_ACTION; slot++) {
                                    auto item = GetEconEntityAtLoadoutSlot(player, slot);
                                    if (item != nullptr && item->GetItem() != nullptr && vec[i].m_itemDefIndex == item->GetItem()->GetItemDefIndex()) {
                                        itemA = item->GetItem();
                                        apply = true;
                                    }
                                }
                            }
                            else {
                                apply = true;
                            }
                        }
                        if (apply) {
                            g_hUpgradeEntity->ApplyUpgradeToItem(player, itemA, vec[i].m_upgrade, vec[i].m_nCost, true);
                        }
                        this->player->RemoveCurrency(-vec[i].m_nCost);
                        if (history != nullptr) {
                            for (int j = history->Count() - 1; j >= 0; j-- ) {
                                auto &entryHistory = (*history)[j];
                                if (entryHistory.m_iPlayerClass == vec[i].m_iPlayerClass && entryHistory.m_itemDefIndex == vec[i].m_itemDefIndex && entryHistory.m_nCost == vec[i].m_nCost && entryHistory.m_upgrade == vec[i].m_upgrade) {
                                history->Remove(j);
                                }
                            }
                        }
                    }
                    vec.RemoveAll();
                }
                else {
                    auto kv = new KeyValues("MVM_Respec");
                    serverGameClients->ClientCommandKeyValues(player->edict(), kv);
                }
                return;
            }
            else if (FStrEq(info, "extra")) {
                Mod::Pop::PopMgr_Extensions::DisplayExtraLoadoutItemsClass(player, player->GetPlayerClass()->GetClassIndex(), true);
                return;
            }
            else {
                slotid = strtol(info, nullptr, 10);
            }
            StartUpgradeListForPlayer(player, slotid, 0);
        }

        virtual void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
		{
            HandleSecurity sec(g_Ext.GetIdentity(), g_Ext.GetIdentity());
            handlesys->FreeHandle(menu->GetHandle(), &sec);
        }
        
        void OnMenuDestroy(IBaseMenu *menu) {
            delete this;
        }

        virtual bool OnSetHandlerOption(const char *option, const void *data)
		{
			return FStrEq(option,"IsCustomUpgradesMenu");
		}

        CHandle<CTFPlayer> player;
    };

    class SelectUpgradeListHandler : public IMenuHandler
    {
    public:

        SelectUpgradeListHandler(CTFPlayer * pPlayer, int iSlot) : IMenuHandler() {
            this->player = pPlayer;
            this->slot = iSlot;
            
        }

        void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item) {
            const char *info = menu->GetItemInfo(item, nullptr);
            
            if (info == nullptr)
                return;

            CEconEntity *ent = GetEconEntityAtLoadoutSlot(this->player, this->slot);
            bool unintendedClassWeapon = ent != nullptr && IsWeaponUnintendedForPlayer(this->player, ent);
            
            if (info[0] == 'r') {
                int upgrade_id = strtol(info+1, nullptr, 10);
                g_hUpgradeEntity->PlayerPurchasingUpgrade(this->player, this->slot, upgrade_id, false, false, false);
                StartUpgradeListForPlayer(this->player, this->slot, (item / 7) * 7);
                return;
            }
            int upgrade_id = strtol(info, nullptr, 10);
            if (upgrade_id < 1000)
                BuyUpgrade(upgrades[upgrade_id], this->slot, this->player, false, false);
            else if ((upgrade_id == 1000) && (!Mod::Pop::PopMgr_Extensions::ExtendedUpgradesNoUndo())) {
                DevMsg("Undoing %d %d\n", extended_upgrades_start_index, CMannVsMachineUpgradeManager::Upgrades().Count());
                for (int i = unintendedClassWeapon ? 0 : extended_upgrades_start_index; i < CMannVsMachineUpgradeManager::Upgrades().Count(); i++) {
                    CMannVsMachineUpgrades &upgr = CMannVsMachineUpgradeManager::Upgrades()[i];
                    int cur_step;
                    bool over_cap;
                    int max_step = GetUpgradeStepData(this->player, this->slot, i, cur_step, over_cap);
                    if (upgr.m_flCap == upgr.m_flIncrement && cur_step > 0) 
                        cur_step = 1;

                    DevMsg("Undoing upgrade %d cap %f increment %f steps %d %d\n", i, upgr.m_flCap,  upgr.m_flIncrement, cur_step, max_step);
                    for (int j = 0; j < cur_step; j++) {
                        DevMsg("Undoing upgrade %d %d %d %s %f %f\n", j, max_step, cur_step, upgr.m_szAttribute, upgr.m_flCap,  upgr.m_flIncrement);
                        from_buy_upgrade = true;
                        g_hUpgradeEntity->PlayerPurchasingUpgrade(this->player, this->slot, i, true, false, false);
                        from_buy_upgrade = false;
                    }
                }
            }
                
            StartUpgradeListForPlayer(this->player, this->slot, (item / 7) * 7);
        }
        
        virtual void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
		{
            HandleSecurity sec(g_Ext.GetIdentity(), g_Ext.GetIdentity());
            handlesys->FreeHandle(menu->GetHandle(), &sec);
            if (reason == MenuEnd_ExitBack) {
                StartMenuForPlayer(player);
            }
		}

        void OnMenuDestroy(IBaseMenu *menu) {
            delete this;
        }

        virtual bool OnSetHandlerOption(const char *option, const void *data)
		{
			return FStrEq(option,"IsCustomUpgradesMenu");
		}

        CHandle<CTFPlayer> player;
        int slot;
    };

    bool CurrentMenuIsUpgrade(CTFPlayer *player)
    {
        void *menu = nullptr;
        if (menus->GetDefaultStyle()->GetClientMenu(ENTINDEX(player), &menu) == MenuSource_BaseMenu && menu != nullptr) {
            //auto title = ((IBaseMenu *)menu)->GetDefaultTitle();
            //return title != nullptr && (FStrEq(title, "Player Upgrades") || FStrEq(title, "Extended Upgrades Menu") || StringStartsWith(title, "Upgrades for"));
            auto handler = ((IBaseMenu *)menu)->GetHandler();
            if (handler != nullptr && handler->OnSetHandlerOption("IsCustomUpgradesMenu","")) {
                return true;
            }
        }
        return false;
    }

    void GetIncrementStringForAttribute(CEconItemAttributeDefinition *attr, float value, std::string &string) {
        if (attr->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_PERCENTAGE) {
            string = fmt::format("{:+.2f}% {}", value, attr->GetName());
        }
        else if (attr->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE) {
            value *= -1.0f;
            string = fmt::format("{:+.2f}% {}", value, attr->GetName());
        }
        else if (attr->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_ADDITIVE) {
            string = fmt::format("{:.2f} {}", value, attr->GetName());
        }
        else if (attr->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE) {
            value *= 100.0f;
            string = fmt::format("{:.2f}% {}", value, attr->GetName());
        }
    }

    void Parse_RequiredUpgrades(KeyValues *kv, std::map<std::string, int> &required_upgrades) {
        int level = 1;
        std::string name="";
        FOR_EACH_SUBKEY(kv, subkey) {
            if (FStrEq(subkey->GetName(), "Upgrade"))
                name = subkey->GetString();
            else if (FStrEq(subkey->GetName(), "Level"))
                level = subkey->GetInt();
        }
        if (name != "") {
            required_upgrades[name] = level;
        }
    }

    void Parse_SecondaryAttributes(KeyValues *kv, std::map<CEconItemAttributeDefinition *, float> &secondary_attributes) {
        FOR_EACH_SUBKEY(kv, subkey) {
            attribute_data_union_t value;
            auto attr_def = GetItemSchema()->GetAttributeDefinitionByName(subkey->GetName());
            std::string value_str = subkey->GetString();
            if (attr_def != nullptr && LoadAttributeDataUnionFromString(attr_def, value, value_str)) {
                secondary_attributes[attr_def] = value.m_Float;
            }
        }
    }

    void Parse_OnUpgradeOutputs(KeyValues* kv, std::vector<std::string>& outputs){
        FOR_EACH_SUBKEY(kv, subkey) {
            outputs.push_back(subkey->GetString());
        }
    }

    int GetSlotFromString(const char *string) {
        int slot = -1;
        if (V_stricmp(string, "Primary") == 0)
            slot = 0;
        else if (V_stricmp(string, "Secondary") == 0)
            slot = 1;
        else if (V_stricmp(string, "Melee") == 0)
            slot = 2;
        else if (V_stricmp(string, "PDA") == 0)
            slot = 5;
        else if (V_stricmp(string, "Canteen") == 0 || V_stricmp(string, "Action") == 0)
            slot = 9;
        else
            slot = strtol(string, nullptr, 10);
        return slot;
    }
    
    int GetExtendedUpgradesStartIndex() {
        return extended_upgrades_start_index;
    }

    bool Parse_Criteria(KeyValues *kv, std::vector<std::unique_ptr<UpgradeCriteria>> &criteria) {
        FOR_EACH_SUBKEY(kv, subkey) {
            if (FStrEq(subkey->GetName(), "Classname"))
                criteria.push_back(std::make_unique<UpgradeCriteriaClassname>(subkey->GetString()));
            else if (FStrEq(subkey->GetName(), "ItemName"))
                criteria.push_back(std::make_unique<UpgradeCriteriaName>(subkey->GetString()));
            else if (FStrEq(subkey->GetName(), "SimilarToItem"))
                criteria.push_back(std::make_unique<UpgradeCriteriaSimilar>(subkey->GetString()));
            else if (FStrEq(subkey->GetName(), "DealsDamage"))
                criteria.push_back(std::make_unique<UpgradeCriteriaHasDamage>(subkey->GetBool()));
            else if (FStrEq(subkey->GetName(), "Slot")) {
                criteria.push_back(std::make_unique<UpgradeCriteriaWeaponSlot>(GetSlotFromString(subkey->GetString())));
            }
        }
        return false;
    }

    void RemoveUpgradesFromGameList() {
        if (extended_upgrades_start_index != -1) {
            CMannVsMachineUpgradeManager::Upgrades().SetCountNonDestructively(extended_upgrades_start_index);
        }
        extended_upgrades_start_index = -1;
    }

    void AddUpgradesToGameList() {
        extended_upgrades_start_index = -1;
        for (auto upgrade : upgrades) {
            int upgrade_index = CMannVsMachineUpgradeManager::Upgrades().AddToTail();
            if (extended_upgrades_start_index == -1)
                extended_upgrades_start_index = upgrade_index;

            CMannVsMachineUpgrades &upgrade_game = CMannVsMachineUpgradeManager::Upgrades()[upgrade_index];
            upgrade->mvm_upgrade_index = upgrade_index;
            strcpy(upgrade_game.m_szAttribute, upgrade->attribute_name.c_str());
            upgrade_game.m_flCap = upgrade->cap;
            upgrade_game.m_flIncrement = upgrade->increment;
            upgrade_game.m_nCost = upgrade->cost;
            upgrade_game.m_iUIGroup = upgrade->playerUpgrade ? 1 : upgrade->uigroup;
            upgrade_game.m_iQuality = 9500;
            upgrade_game.m_iTier = 0;
        }
        for (auto upgrade : upgrades) {
            auto mainAttrDef = GetItemSchema()->GetAttributeDefinitionByName(upgrade->attribute_name.c_str());
            int steps = 1;
            if (mainAttrDef != nullptr && !mainAttrDef->IsType<CSchemaAttributeType_String>()) {
                steps = (upgrade->cap - mainAttrDef->GetDefaultValue()) / upgrade->increment;
            }
            upgrade->secondary_attributes_index.clear();
            for(auto entry : upgrade->secondary_attributes) {
                int upgrade_index = CMannVsMachineUpgradeManager::Upgrades().AddToTail();
                CMannVsMachineUpgrades &upgrade_game_child = CMannVsMachineUpgradeManager::Upgrades()[upgrade_index];
                upgrade->secondary_attributes_index.push_back(upgrade_index);
                auto attrDef = entry.first;
                strcpy(upgrade_game_child.m_szAttribute, attrDef->GetName());

                float defValue = 0;
                // String attributes always have cap set the same as increment
                if (attrDef->IsType<CSchemaAttributeType_String>()) {
                    upgrade_game_child.m_flCap = entry.second;
                }
                else {
                    defValue = attrDef->GetDefaultValue();
                    upgrade_game_child.m_flCap = defValue + entry.second * steps;
                }

                upgrade_game_child.m_flIncrement = entry.second;
                upgrade_game_child.m_nCost = 0;
                upgrade_game_child.m_iUIGroup = upgrade->playerUpgrade ? 1 : upgrade->uigroup;
                upgrade_game_child.m_iQuality = 9500;
                upgrade_game_child.m_iTier = 0;
            }
        }
    }

    void Parse_ExtendedUpgrades(KeyValues *kv, bool v2) {
        FOR_EACH_SUBKEY(kv, subkey) {
            if (FStrEq(subkey->GetName(), "MaxUpgradesTier")) {
                FOR_EACH_SUBKEY(subkey, subkey2) {
                    int tier = atoi(subkey2->GetName());
                    int max = subkey2->GetInt();
                    if ((size_t) tier > max_tier_upgrades.size()) {
                        max_tier_upgrades.resize(tier, 1);
                    }
                    max_tier_upgrades[tier - 1] = max;
                }
                continue;
            }
            else if (FStrEq(subkey->GetName(), "MinUpgradesTier")) {
                FOR_EACH_SUBKEY(subkey, subkey2) {
                    int tier = atoi(subkey2->GetName());
                    int max = subkey2->GetInt();
                    if ((size_t) tier > min_tier_upgrades.size()) {
                        min_tier_upgrades.resize(tier, 1);
                    }
                    min_tier_upgrades[tier - 1] = max;
                }
                continue;
            }

            auto upgradeinfo = new UpgradeInfo(); // upgrades.emplace(upgrades.end());
            upgradeinfo->ref_name = subkey->GetName();
            
            FOR_EACH_SUBKEY(subkey, subkey2) {
                if (FStrEq(subkey2->GetName(), "Attribute")) {
                    upgradeinfo->attribute_name = subkey2->GetString();
                    upgradeinfo->attributeDefinition = GetItemSchema()->GetAttributeDefinitionByName(upgradeinfo->attribute_name.c_str());
                }
                else if (FStrEq(subkey2->GetName(), "Cap")) {
                    upgradeinfo->cap = subkey2->GetFloat();
                }
                else if (FStrEq(subkey2->GetName(), "Increment")) {
                    upgradeinfo->increment = subkey2->GetFloat();
                }
                else if (FStrEq(subkey2->GetName(), "Cost")) {
                    upgradeinfo->cost = subkey2->GetInt();
                }
                else if (FStrEq(subkey2->GetName(), "AllowedWeapons"))
                    Parse_Criteria(subkey2, upgradeinfo->criteria);
                else if (FStrEq(subkey2->GetName(), "DisallowedWeapons"))
                    Parse_Criteria(subkey2, upgradeinfo->criteria_negate);
                else if (FStrEq(subkey2->GetName(), "PlayerUpgrade")) {
                    upgradeinfo->playerUpgrade = subkey2->GetBool();
                }
                else if (FStrEq(subkey2->GetName(), "Name"))
                    upgradeinfo->name = subkey2->GetString();
                else if (FStrEq(subkey2->GetName(), "Description"))
                    upgradeinfo->description = subkey2->GetString();
                else if (FStrEq(subkey2->GetName(), "SecondaryAttributes"))
                    Parse_SecondaryAttributes(subkey2, upgradeinfo->secondary_attributes);
                else if (FStrEq(subkey2->GetName(), "Children"))
                    upgradeinfo->children.push_back(subkey2->GetString());
                else if (FStrEq(subkey2->GetName(), "Hidden"))
                    upgradeinfo->hidden = subkey2->GetString();
                else if (FStrEq(subkey2->GetName(), "AllowedMinWave"))
                    upgradeinfo->allow_wave_min = subkey2->GetInt();
                else if (FStrEq(subkey2->GetName(), "AllowedMaxWave"))
                    upgradeinfo->allow_wave_max = subkey2->GetInt();
                else if (FStrEq(subkey2->GetName(), "RequiredUpgrade"))
                    Parse_RequiredUpgrades(subkey2, upgradeinfo->required_upgrades);
                else if (FStrEq(subkey2->GetName(), "RequiredUpgradeOr"))
                    Parse_RequiredUpgrades(subkey2, upgradeinfo->required_upgrades_option);
                else if (FStrEq(subkey2->GetName(), "DisallowedUpgrade"))
                    Parse_RequiredUpgrades(subkey2, upgradeinfo->disallowed_upgrades);
                else if (FStrEq(subkey2->GetName(), "RequiredWeapons"))
                    Parse_Criteria(subkey2, upgradeinfo->required_weapons);
                else if (FStrEq(subkey2->GetName(), "RequiredWeaponsString"))
                    upgradeinfo->required_weapons_string = subkey2->GetString();
                else if (FStrEq(subkey2->GetName(), "ForceUpgradeSlot"))
                    upgradeinfo->force_upgrade_slot = GetSlotFromString(subkey->GetString());
                else if (FStrEq(subkey2->GetName(), "ShowRequirements"))
                    upgradeinfo->show_requirements = subkey2->GetBool();
                else if (FStrEq(subkey2->GetName(), "AllowPlayerClass")) {
                    for(int i=1; i < 10; i++){
                        if(FStrEq(g_aRawPlayerClassNames[i],subkey2->GetString())){
                            upgradeinfo->player_classes.push_back(i);
                            break;
                        }
                    }
                }
                else if (FStrEq(subkey2->GetName(), "OnUpgrade")) {
                    Parse_OnUpgradeOutputs(subkey2, upgradeinfo->on_upgrade_outputs);
                }
                else if (FStrEq(subkey2->GetName(), "OnApply")) {
                    Parse_OnUpgradeOutputs(subkey2, upgradeinfo->on_apply_outputs);
                }
                else if (FStrEq(subkey2->GetName(), "OnDowngrade")) {
                    Parse_OnUpgradeOutputs(subkey2, upgradeinfo->on_downgrade_outputs);
                }
                else if (FStrEq(subkey2->GetName(), "OnRestore")) {
                    Parse_OnUpgradeOutputs(subkey2, upgradeinfo->on_restore_outputs);
                }
                else if (FStrEq(subkey2->GetName(), "ForceEnable")) {
                    upgradeinfo->force_enable = true;
                }
                else if (FStrEq(subkey2->GetName(), "Tier")) {
                    upgradeinfo->tier = subkey2->GetInt();
                }
                else if (FStrEq(subkey2->GetName(), "UIGroup")) {
                    upgradeinfo->uigroup = subkey2->GetInt();
                }
                
            }

            if (upgradeinfo->attributeDefinition == nullptr) {
                delete upgradeinfo;// upgrades.erase(upgradeinfo);
            }
            else {
                if (v2) {
                    upgradeinfo->mvm_upgrade_index = upgrades_v2.size();
                    upgrades_v2.push_back(upgradeinfo);
                }
                else {
                    upgrades.push_back(upgradeinfo);
                }
                upgrades_ref[upgradeinfo->ref_name] = upgradeinfo;


                //if (upgradeinfo->name == "")
                //    GetIncrementStringForAttribute(upgradeinfo->attributeDefinition, upgradeinfo->increment, upgradeinfo->name); //upgradeinfo->attributeDefinition->GetKVString("description_string", );
            }
		}
        RemoveUpgradesFromGameList();
        AddUpgradesToGameList();
    }
    

    void ClearUpgrades() {
        max_tier_upgrades.clear();
        min_tier_upgrades.clear();
        std::vector<CHandle<CTFPlayer>> vecmenus;
        ForEachTFPlayer([](CTFPlayer *player){
            if (player->IsBot()) return;

            if (CurrentMenuIsUpgrade(player)) {
                menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player));
            }
        });
        // for (auto pair : select_type_menus) {
        //     vecmenus.push_back(pair.first);
        // }

        // for (auto player : vecmenus) {
        //     IBaseMenu *menu = select_type_menus[player];
        //     if (player != nullptr)
        //         menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player));
                
        //     menus->CancelMenu(menu);
        //     //menu->Destroy();
        // }
        //select_type_menus.clear();

        for (auto upgrade : upgrades) {
            delete upgrade;
        }
        for (auto upgrade : upgrades_v2) {
            delete upgrade;
        }
        for (auto &[steamid, boughtUpgrades] : bought_upgrades_v2) {
            RemoveIf(boughtUpgrades, [](auto &boughtUpgrade){
                return (size_t)boughtUpgrade.upgradeId >= upgrades_v2.size();
            });
        }
        upgrades.clear();
        upgrades_v2.clear();
        upgrades_ref.clear();

        RemoveUpgradesFromGameList();
        DevMsg("Total updates count: %d %d\n", CMannVsMachineUpgradeManager::Upgrades().Count(), extended_upgrades_start_index);
    }

    int GetCurrentUpgradeLevel(UpgradeInfo *upgrade, CEconEntity *item, CTFPlayer *player) {
        if (upgrade->is_v2_upgrade) {
            auto boughtInfo = FindUpgradeInfo(item, player, upgrade);
            return boughtInfo == nullptr ? 0 : boughtInfo->level;
        }
        else {
            int player_class = player->GetPlayerClass()->GetClassIndex();
            int cur_step;
            bool over_cap;
            int max_step = GetUpgradeStepData(player, upgrade->playerUpgrade || item == nullptr ? -1 : GetLoadoutSlotForItem(item->GetItem()->GetItemDefinition(), player_class), upgrade->mvm_upgrade_index, cur_step, over_cap);
            return cur_step;
        }
    }

    std::string GetUpgradeNameTranslated(CTFPlayer *player, UpgradeInfo *upgrade) {
        std::string displayName = upgrade->name;
        if (displayName.empty()) {
            attribute_data_union_t value;
            value.m_Float = upgrade->increment + upgrade->attributeDefinition->GetDefaultValue();
            FormatAttributeString(displayName, upgrade->attributeDefinition, value, player, true);
        }
        return displayName;
    }

    bool IsValidUpgradeForWeapon(UpgradeInfo *upgrade, CEconEntity *item, CTFPlayer *player, std::string &reason) {
        if (upgrade->hidden)
            return false;

        //Default: upgrade is valid for every weapon;

        if ((upgrade->playerUpgrade && item != nullptr) || (!upgrade->playerUpgrade && item == nullptr))
            return false;

        int wave = TFObjectiveResource()->m_nMannVsMachineWaveCount;

        int player_class = player->GetPlayerClass()->GetClassIndex();

        if (!upgrade->playerUpgrade) {
            bool valid = upgrade->criteria.empty();

            for (auto &criteria : upgrade->criteria) {
                valid |= criteria->Matches(item, item->GetItem(), player);
                if (valid)
                    break;
            }

            for (auto &criteria : upgrade->criteria_negate) {
                valid &= !criteria->Matches(item, item->GetItem(), player);
                if (!valid)
                    break;
            }
            if (!valid)
                return false;
        }
        if (!upgrade->player_classes.empty()) {
            bool found = false;
            for (int classindex : upgrade->player_classes) {
                if (classindex == player_class) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }

        if (wave < upgrade->allow_wave_min) {
            reason = TranslateText(player, "Available from wave", 1, &upgrade->allow_wave_min);
            return false;
        }
        else if (wave > upgrade->allow_wave_max) {
            reason = TranslateText(player, "Available up to wave", 1, &upgrade->allow_wave_max);
            return false;
        }

        for (auto entry : upgrade->required_upgrades) {
            UpgradeInfo *child = upgrades_ref[entry.first];
            if (child != nullptr) {
                // Required upgrade is an item upgrade and we are a player upgrade, then require at least one item with given upgrade
                if (upgrade->playerUpgrade && !child->playerUpgrade) {
                    for (loadout_positions_t slot : {
                    LOADOUT_POSITION_PRIMARY,
                    LOADOUT_POSITION_SECONDARY,
                    LOADOUT_POSITION_MELEE,
                    LOADOUT_POSITION_BUILDING,
                    LOADOUT_POSITION_PDA,
                    LOADOUT_POSITION_ACTION,
                    }) {
                        CEconEntity *item_require = GetEconEntityAtLoadoutSlot(player, (int)slot);
                        if (item_require != nullptr && item_require->GetItem() != nullptr && GetCurrentUpgradeLevel(child, item_require, player) < entry.second) {
                            std::string childName = GetUpgradeNameTranslated(player, child);
                            reason = TranslateText(player, "Requires upgrade", 2, childName.c_str(), &entry.second);
                            return false;
                        }
                    }
                }
                else if (GetCurrentUpgradeLevel(child, item, player) < entry.second) {
                    std::string childName = GetUpgradeNameTranslated(player, child);
                    reason = TranslateText(player, "Requires upgrade", 2, childName.c_str(), &entry.second);
                    return false;
                }
            }
        }

        for (auto entry : upgrade->disallowed_upgrades) {
            UpgradeInfo *child = upgrades_ref[entry.first];
            DevMsg("disallowed upgrade %s %d\n",entry.first.c_str(), child != nullptr);
            // Disallowed upgrade is an item upgrade and we are a player upgrade, then disallow with at least one item with given upgrade
            if (child != nullptr) {
                if (upgrade->playerUpgrade && !child->playerUpgrade) {
                    for (loadout_positions_t slot : {
                    LOADOUT_POSITION_PRIMARY,
                    LOADOUT_POSITION_SECONDARY,
                    LOADOUT_POSITION_MELEE,
                    LOADOUT_POSITION_BUILDING,
                    LOADOUT_POSITION_PDA,
                    LOADOUT_POSITION_ACTION,
                    }) {
                        CEconEntity *item_disallow = GetEconEntityAtLoadoutSlot(player, (int)slot);
                        if (item_disallow != nullptr && item_disallow->GetItem() != nullptr && 
                        GetCurrentUpgradeLevel(child, item_disallow, player) >= entry.second) {
                            std::string childName = GetUpgradeNameTranslated(player, child);
                            reason = TranslateText(player, "Incompatible upgrade", 1, childName.c_str());
                            return false;
                        }
                    }
                }
                else if (GetCurrentUpgradeLevel(child, item, player) >= entry.second) {
                    std::string childName = GetUpgradeNameTranslated(player, child);
                    reason = TranslateText(player, "Incompatible upgrade", 1, childName.c_str());
                    return false;
                }
            }
        }

        if (!upgrade->required_upgrades_option.empty()) {
            bool found = false;
            for (auto &entry : upgrade->required_upgrades_option) {
                UpgradeInfo *child = upgrades_ref[entry.first];
                if (child != nullptr) {
                    if (upgrade->playerUpgrade && !child->playerUpgrade) {
                        for (loadout_positions_t slot : {
                        LOADOUT_POSITION_PRIMARY,
                        LOADOUT_POSITION_SECONDARY,
                        LOADOUT_POSITION_MELEE,
                        LOADOUT_POSITION_BUILDING,
                        LOADOUT_POSITION_PDA,
                        LOADOUT_POSITION_ACTION,
                        }) {
                            CEconEntity *item_require = GetEconEntityAtLoadoutSlot(player, (int)slot);
                            if (item_require != nullptr && item_require->GetItem() != nullptr && GetCurrentUpgradeLevel(child, item_require, player) >= entry.second) {
                                found = true;
                            }
                        }
                    }
                    if (GetCurrentUpgradeLevel(child, item, player) >= entry.second) {
                        found = true;
                    }
                    else {
                        std::string childName = GetUpgradeNameTranslated(player, child);
                        reason = TranslateText(player, "Requires upgrade", 2, childName.c_str(), &entry.second);
                    }
                }
            }
            if (!found) {
                return false;
            }
        }
        
        if (!upgrade->required_weapons.empty()) {
            bool found = false;
            for (auto &criteria : upgrade->required_weapons) {
                for (loadout_positions_t slot : {
                    LOADOUT_POSITION_PRIMARY,
                    LOADOUT_POSITION_SECONDARY,
                    LOADOUT_POSITION_MELEE,
                    LOADOUT_POSITION_BUILDING,
                    LOADOUT_POSITION_PDA,
                    LOADOUT_POSITION_ACTION,
                }) {
                    CEconEntity *item_require = GetEconEntityAtLoadoutSlot(player, (int)slot);
                    if (item_require != nullptr && item_require->GetItem() != nullptr) {
                        found = criteria->Matches(item_require, item_require->GetItem(), player);
                    }
                    if (found)
                        break;
                }
                if (found)
                        break;
            }
            if (!found) {
                reason = upgrade->required_weapons_string;
                return false;
            }
        }

        if (upgrade->tier > 0) {
            int numTierUpgrades = 0;
            int numPrevTierUpgrades = 0;
            for (auto &upgrade2 : upgrades) {
                if (upgrade2 != nullptr && upgrade->playerUpgrade == upgrade2->playerUpgrade) {
                    int cur_step;
                    bool over_cap;
                    int max_step = GetUpgradeStepData(player, upgrade->playerUpgrade ? -1 : GetLoadoutSlotForItem(item->GetItem()->GetItemDefinition(), player_class), upgrade2->mvm_upgrade_index, cur_step, over_cap);
                    if (cur_step > 0) {
                        if (upgrade->tier == upgrade2->tier) {
                            numTierUpgrades+=cur_step;
                        }
                        else if (upgrade->tier == upgrade2->tier + 1) {
                            numPrevTierUpgrades+=cur_step;
                        }
                    }
                }
            }
            for (auto &upgrade2 : upgrades_v2) {
                if (upgrade2 != nullptr && upgrade->playerUpgrade == upgrade2->playerUpgrade) {
                    int level = GetCurrentUpgradeLevel(upgrade2, item, player);
                    if (level > 0) {
                        if (upgrade->tier == upgrade2->tier) {
                            numTierUpgrades+=level;
                        }
                        else if (upgrade->tier == upgrade2->tier + 1) {
                            numPrevTierUpgrades+=level;
                        }
                    }
                }
            }
            int maxUpgradesForThisTier = ((int) max_tier_upgrades.size() > upgrade->tier - 1 ? max_tier_upgrades[upgrade->tier - 1] : 1);
            int minUpgradesForPrevTier = (upgrade->tier > 1 && (int) min_tier_upgrades.size() > upgrade->tier - 2 ? min_tier_upgrades[upgrade->tier - 2] : 1);
            if (numTierUpgrades >= maxUpgradesForThisTier) {
                reason = TranslateText(player, "Tier is closed", 1, &upgrade->tier);
                return false;
            }
            if (upgrade->tier > 1 && numPrevTierUpgrades < minUpgradesForPrevTier) {
                int tierDiff = minUpgradesForPrevTier - numPrevTierUpgrades;
                int prevTier = upgrade->tier - 1;
                reason = TranslateText(player, "Buy more upgrades", 2, &tierDiff, &prevTier);
                return false;
            }
        }

        return true;
    }

    bool WeaponHasValidUpgrades(CEconEntity *item, CTFPlayer *player) {
        int iCannotUpgrade = 0;
        CALL_ATTRIB_HOOK_INT_ON_OTHER (item, iCannotUpgrade, cannot_be_upgraded );
        if (iCannotUpgrade > 0) {
            return false;
        }
        if (item != nullptr && IsWeaponUnintendedForPlayer(player, item)) {
            int slot = GetLoadoutSlotForItem(item->GetItem()->GetItemDefinition(), player->GetPlayerClass()->GetClassIndex());
            auto count = extended_upgrades_start_index == -1 ? CMannVsMachineUpgradeManager::Upgrades().Count() : extended_upgrades_start_index;
            for (int i = 0; i < count; i++) {
                auto &upgrade = CMannVsMachineUpgradeManager::Upgrades()[i];
                auto attrDef = GetItemSchema()->GetAttributeDefinitionByName(upgrade.m_szAttribute);
                if (attrDef != nullptr && upgrade.m_iUIGroup == 0 && TFGameRules()->CanUpgradeWithAttrib(player, slot, attrDef->GetIndex(), &upgrade)) {
                    return true;
                }
            }
        }
        for (size_t i = 0; i < upgrades.size(); i++) {
            auto upgrade = upgrades[i];
            std::string reason = "";
            if (IsValidUpgradeForWeapon(upgrade, item, player, reason) || (reason != "" && upgrade->show_requirements)) {
                return true;
            }
        }
        for (size_t i = 0; i < upgrades_v2.size(); i++) {
            auto upgrade = upgrades_v2[i];
            std::string reason = "";
            if (IsValidUpgradeForWeapon(upgrade, item, player, reason) || (reason != "" && upgrade->show_requirements)) {
                return true;
            }
        }
        return false;
    }

    bool from_buy_upgrade_free = false;
    bool upgrade_success = false;
    bool buying_upgrade = false;

    bool BuyUpgrade(UpgradeInfo *upgrade,/* CEconEntity *item*/ int slot, CTFPlayer *player, bool free, bool downgrade) {
        if (g_hUpgradeEntity.GetRef() == nullptr) {
            DevMsg("No upgrade entity?\n");
            return false;
        }

        std::string reason;
        if (!IsValidUpgradeForWeapon(upgrade, slot == -1 ? nullptr : GetEconEntityAtLoadoutSlot(player, slot), player, reason)) {
            return false;
        }
        /*int cost = upgrade->cost;

        if (downgrade)
            cost *= -1;

        if (!free && cost > player->GetCurrency())
            return false;*/
        from_buy_upgrade = true;
        upgrade_success = false;
        int override_slot = slot;

        if (upgrade->force_upgrade_slot != -2) {
            override_slot = upgrade->force_upgrade_slot;
        }
        buying_upgrade = true;
        g_hUpgradeEntity->PlayerPurchasingUpgrade(player, override_slot, upgrade->mvm_upgrade_index, downgrade, free, false);

        if (upgrade_success) {
            from_buy_upgrade_free = true;
            for (auto childname : upgrade->children) {
                auto child = upgrades_ref[childname];
                if (child != nullptr) {

                    override_slot = slot;
                    if (child->force_upgrade_slot != -2) {
                        override_slot = child->force_upgrade_slot;
                    }
                    g_hUpgradeEntity->PlayerPurchasingUpgrade(player, override_slot, child->mvm_upgrade_index, downgrade, true, false);
                }
            }
            for (int attr : upgrade->secondary_attributes_index) {
                DevMsg("Buy Secondary upgrade %d\n", attr);
                g_hUpgradeEntity->PlayerPurchasingUpgrade(player, override_slot, attr, downgrade, true, false);
            }
            from_buy_upgrade_free = false;
            
            int cur_step;
            bool over_cap;
            int max_step = GetUpgradeStepData(player, slot, upgrade->mvm_upgrade_index, cur_step, over_cap);

            variant_t variant;
            variant.SetInt(cur_step);
            
            auto itemInSlot = GetEconEntityAtLoadoutSlot(player, slot);
            FireOutputs(upgrade->on_upgrade_outputs, variant, player,  itemInSlot == nullptr ? (CBaseEntity *)player : (CBaseEntity *)itemInSlot);
        }
        static ConVarRef sig_mvm_upgradestation_regen_improved("sig_mvm_upgradestation_regen_improved");
        // Give weapons now, so that the secondary upgrades are not applied to switched weapons
        if (!sig_mvm_upgradestation_regen_improved.GetBool()) {
            player->GiveDefaultItems();
        }
        buying_upgrade = false;
            //if (!free)
            //    player->RemoveCurrency(cost);

            //RememberUpgrade(upgrade, item->GetItem(), player, cost, downgrade);
            //return true;
        //}
        from_buy_upgrade = false;
        return upgrade_success;
    }

    void ApplyUpgradeV2(UpgradeInfo *upgrade, CEconEntity *item, CTFPlayer *player, int prevLevel, int newLevel) {
        auto attrs = upgrade->secondary_attributes;
        attrs[upgrade->attributeDefinition] = upgrade->increment;

        for (auto &[attrDef, increment] : attrs) {
            float prevCalcValue = increment * prevLevel;

            float curAttrValue = attrDef->GetDefaultValue();
            if (item != nullptr) {
                ForEachItemAttribute(item->GetItem(), [&](auto pAttrDef, auto value){
                    if (pAttrDef == attrDef) {
                        if (pAttrDef->IsMultiplicative()) {
                            curAttrValue *= value.m_Float;
                        }  
                        else {
                            curAttrValue += value.m_Float;
                        }
                    }
                    return true;
                }, [&](auto pAttrDef, auto value){
                    return true;
                });
            }
            else {
                auto attr = player->GetAttributeList()->GetAttributeByID(attrDef->GetIndex());
                if (attr != nullptr) {
                    if (attrDef->IsMultiplicative()) {
                        curAttrValue *= attr->GetValue().m_Float;
                    }  
                    else {
                        curAttrValue += attr->GetValue().m_Float;
                    }
                }
            }
            float newValue = attrDef->IsMultiplicative() ? curAttrValue * (1.0f + newLevel * increment) / (1.0f + prevLevel * increment) - 1: (newLevel - prevLevel) * increment;
            float valueWithout = attrDef->IsMultiplicative() ? curAttrValue / (1.0f + prevCalcValue) : curAttrValue - prevCalcValue;
            auto &attrList = item != nullptr ? item->GetItem()->GetAttributeList() : *player->GetAttributeList();
            if (newValue == attrDef->GetDefaultValue()) {
                attrList.RemoveAttribute(attrDef);
            }
            else {
                attrList.SetRuntimeAttributeValue(attrDef, newValue);
            }
        }
    }

    bool BuyUpgradeV2(UpgradeInfo *upgrade,/* CEconEntity *item*/ int slot, CTFPlayer *player, bool free, bool downgrade) {
        
        std::string reason;
        auto item = slot == -1 ? nullptr : GetEconEntityAtLoadoutSlot(player, slot);
        if (!IsValidUpgradeForWeapon(upgrade, item, player, reason)) {
            return false;
        }

        int override_slot = slot;
        if (upgrade->force_upgrade_slot != -2) {
            override_slot = upgrade->force_upgrade_slot;
        }

        auto itemId = item != nullptr ? item->GetItem()->m_iItemID : -1LL;
        auto itemDefId = item != nullptr ? item->GetItem()->m_iItemDefinitionIndex : INVALID_ITEM_DEF_INDEX;
        auto info = FindUpgradeInfo(item, player, upgrade);
        auto cost = (free ? 0 : upgrade->cost) * (downgrade ? -1 : 1);

        if (cost > player->GetCurrency()) return false;

        if (info != nullptr) {
            if (upgrade->cap >= info->level) return false;

            info->level += downgrade ? -1 : 1;
        }
        else {
            if (downgrade) return false;

            info = &bought_upgrades_v2[player->GetSteamID()].emplace_back(itemId, itemDefId, player->GetPlayerClass()->GetClassIndex(), upgrade->mvm_upgrade_index, 1);
        }
        player->RemoveCurrency(cost);

        FireOutputs(downgrade ? upgrade->on_downgrade_outputs : upgrade->on_upgrade_outputs, Variant(info->level), player, item == nullptr ? (CBaseEntity *)player : (CBaseEntity *)item);
        ApplyUpgradeV2(upgrade, item, player, info->level + downgrade ? 1 : -1, info->level);
        return true;
    }
    
    void StartUpgradeListForPlayer(CTFPlayer *player, int slot, int displayitem) {
        menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player));
        
        CEconEntity *item = GetEconEntityAtLoadoutSlot(player, slot);
        
        DevMsg("Start upgrade list %d %d\n", slot, item != nullptr);

        if (slot != -1 && item == nullptr) return;

        SelectUpgradeListHandler *handler = new SelectUpgradeListHandler(player, slot);
        IBaseMenu *menu = menus->GetDefaultStyle()->CreateMenu(handler, g_Ext.GetIdentity());

        if (slot == -1)
            menu->SetDefaultTitle(TranslateText(player, "Player upgrades"));
        else {
            menu->SetDefaultTitle(TranslateText(player, "Item upgrades", 1, GetItemNameForDisplay(item->GetItem(), player)));
        }
        menu->SetMenuOptionFlags(MENUFLAG_BUTTON_EXITBACK);

        if (item != nullptr && IsWeaponUnintendedForPlayer(player, item)) {
            auto count = extended_upgrades_start_index == -1 ? CMannVsMachineUpgradeManager::Upgrades().Count() : extended_upgrades_start_index;
            for (int i = 0; i < count; i++) {
                auto &upgrade = CMannVsMachineUpgradeManager::Upgrades()[i];
                auto attrDef = GetItemSchema()->GetAttributeDefinitionByName(upgrade.m_szAttribute);
                if (attrDef != nullptr && upgrade.m_iUIGroup == 0 && TFGameRules()->CanUpgradeWithAttrib(player, slot, attrDef->GetIndex(), &upgrade)) {
                    
                    int cur_step;
                    bool over_cap;
                    int max_step = GetUpgradeStepData(player, slot, i, cur_step, over_cap);

                    std::string name;
                    attribute_data_union_t value;
                    value.m_Float = upgrade.m_flIncrement + attrDef->GetDefaultValue();
                    FormatAttributeString(name, attrDef, value, player, true);
                    std::string line = fmt::format("{} ({}/{}) ${}", name.c_str(), cur_step, max_step, TFGameRules()->GetCostForUpgrade(&upgrade, slot, player->GetPlayerClass()->GetClassIndex(), player));
                    ItemDrawInfo info1(line.c_str(), 
                        cur_step >= max_step || player->GetCurrency() < upgrade.m_nCost ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
                    
                    static char buf[5];
                    snprintf(buf, sizeof(buf), "r%d", (int)i);
                    menu->AppendItem(buf, info1);
                }
            }
        }
        for (size_t i = 0; i < upgrades.size(); i++) {
            auto upgrade = upgrades[i];
            std::string disabled_reason;
            bool enabled = IsValidUpgradeForWeapon(upgrade, item, player, disabled_reason);
            if (upgrade->force_enable) enabled = true;
            
            int cur_step;
            bool over_cap;
            int max_step = GetUpgradeStepData(player, slot, upgrade->mvm_upgrade_index, cur_step, over_cap);

            std::string displayName = GetUpgradeNameTranslated(player, upgrade);

            if (enabled) {

                char text[128];
                if (upgrade->increment != 0 && max_step < 100000 ) {
                    snprintf(text, 128, "%s (%d/%d) $%d", displayName.c_str(), cur_step, max_step, upgrade->cost);
                }
                else { // If increment == 0 or max steps less than 100000, pretend unlimited upgrades
                    snprintf(text, 128, "%s $%d", displayName.c_str(), upgrade->cost);
                }

                ItemDrawInfo info1(text, 
                    cur_step >= max_step || player->GetCurrency() < upgrade->cost ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);

                // Ensure that upgrade and its description will land on the same page
                if (!upgrade->description.empty() && menu->GetItemCount() % 7 == 6) {
                    menu->AppendItem("", ItemDrawInfo("", ITEMDRAW_DISABLED));
                }
                
                static char buf[4];
                snprintf(buf, sizeof(buf), "%d", (int)i);
                menu->AppendItem(buf, info1);

                if (!upgrade->description.empty()) {
                    ItemDrawInfo infodesc(CFmtStr("^ %s", upgrade->description.c_str()), ITEMDRAW_DISABLED);
                    menu->AppendItem("9999", infodesc);
                }
            }
            else if (upgrade->show_requirements && disabled_reason != "") {

                char text[128];
                if (upgrade->increment != 0 && max_step < 100000) {
                    snprintf(text, 128, "%s: %s (%d/%d) $%d", displayName.c_str(), disabled_reason.c_str(), cur_step, max_step, upgrade->cost);
                }
                else { // If increment == 0 or max steps less than 100000, pretend unlimited upgrades
                    snprintf(text, 128, "%s: %s $%d", displayName.c_str(), disabled_reason.c_str(), upgrade->cost);
                }

                ItemDrawInfo info1(text, ITEMDRAW_DISABLED);
                
                static char buf[4];
                snprintf(buf, sizeof(buf), "%d", (int)i);
                menu->AppendItem(buf, info1);

                if (!upgrade->description.empty()) {
                    ItemDrawInfo infodesc(CFmtStr("^ %s", upgrade->description.c_str()), ITEMDRAW_DISABLED);
                    menu->AppendItem("9999", infodesc);
                }
            }
        }

        if(!Mod::Pop::PopMgr_Extensions::ExtendedUpgradesNoUndo()){
            ItemDrawInfo info1(TranslateText(player, "Undo upgrades"));
            menu->AppendItem("1000", info1);
        }
        /*if (upgrades.size() == 1) {
            ItemDrawInfo info1(" ", ITEMDRAW_NOTEXT);
            menu->AppendItem(" ", info1);
        }*/

        //select_type_menus[player] = menu;
        menu->DisplayAtItem(ENTINDEX(player), 0, displayitem);
    }

    // Some items should not be displayed on the upgrade list if there are no upgrades assigned to them
    bool DisplayItemByDefault(CEconEntity *item) {
        auto weapon = rtti_cast<CTFWeaponBase *>(item);
        if (weapon != nullptr) {
            int itemID = weapon->GetWeaponID();
            if (itemID == TF_WEAPON_INVIS)
                return false;
            if (itemID == TF_WEAPON_PDA_ENGINEER_DESTROY)
                return false;
            if (itemID == TF_WEAPON_PDA_SPY)
                return false;
            // Hide engineer builder item
            if (itemID == TF_WEAPON_BUILDER) {
                auto kv = weapon->GetItem()->GetStaticData()->GetKeyValues()->FindKey("used_by_classes");
                return kv != nullptr && kv->FindKey("spy") != nullptr;
            }
        }
        return true;
    }

    void StartMenuForPlayer(CTFPlayer *player) {
        if (menus->GetDefaultStyle()->GetClientMenu(ENTINDEX(player), nullptr) == MenuSource_BaseMenu) return;
        //if (select_type_menus.find(player) != select_type_menus.end()) return;
        
        SelectUpgradeWeaponHandler *handler = new SelectUpgradeWeaponHandler(player);
        IBaseMenu *menu = menus->GetDefaultStyle()->CreateMenu(handler, g_Ext.GetIdentity());
        
        menu->SetDefaultTitle(TranslateText(player, "Extended upgrades menu"));
        menu->SetMenuOptionFlags(0);

        ItemDrawInfo info1(TranslateText(player, "Player upgrades"), WeaponHasValidUpgrades(nullptr, player) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
        menu->AppendItem("player", info1);

        for (loadout_positions_t slot : {
			LOADOUT_POSITION_PRIMARY,
			LOADOUT_POSITION_SECONDARY,
			LOADOUT_POSITION_MELEE,
			LOADOUT_POSITION_BUILDING,
			LOADOUT_POSITION_PDA,
			LOADOUT_POSITION_PDA2,
			LOADOUT_POSITION_ACTION,
		}) {
            CEconEntity *item = GetEconEntityAtLoadoutSlot(player, (int)slot);
            if (item != nullptr) {
                bool hasValidUpgrades = WeaponHasValidUpgrades(item, player);
                if (!hasValidUpgrades && !DisplayItemByDefault(item))
                    continue;

                ItemDrawInfo info2(GetItemNameForDisplay(item->GetItem(), player), hasValidUpgrades ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
                
                static char buf[4];
                snprintf(buf, sizeof(buf), "%d", (int)slot);
                menu->AppendItem(buf, info2);
            }
        }

        static ConVarRef tf_mvm_respec_enabled("tf_mvm_respec_enabled");
        ItemDrawInfo info2(TranslateText(player, "Refund upgrades"), tf_mvm_respec_enabled.GetBool() ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
        menu->AppendItem("refund", info2);

        if (Mod::Pop::PopMgr_Extensions::HasExtraLoadoutItems(player->GetPlayerClass()->GetClassIndex())) {
            
            ItemDrawInfo info3(TranslateText(player, "Extra loadout items"), ITEMDRAW_DEFAULT);
            menu->AppendItem("extra", info3);
        }

        //select_type_menus[player] = menu;

        menu->Display(ENTINDEX(player), 0);
    }
    
    void StopMenuForPlayer(CTFPlayer *player) {
        //if (select_type_menus.find(player) == select_type_menus.end()) return;
        //DevMsg("Stopped menu\n");
        CancelClientMenu(player);

        // menu = menus->GetDefaultStyle()->CreateMenu(new SelectUpgradeWeaponHandler(player), g_Ext.GetIdentity());
        // menu->SetDefaultTitle(" ");
        // menu->SetMenuOptionFlags(0);
        // ItemDrawInfo info1(" ", ITEMDRAW_NOTEXT);
        // menu->AppendItem(" ", info1);
        // ItemDrawInfo info2(" ", ITEMDRAW_NOTEXT);
        // menu->AppendItem(" ", info2);
        // menu->Display(ENTINDEX(player), 1);
    }

    DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
        ClearUpgrades();
        auto ret = DETOUR_MEMBER_CALL();
        
        return ret;
    }

    DETOUR_DECL_MEMBER(void, CMannVsMachineUpgradeManager_LoadUpgradesFileFromPath, const char *path)
	{
        DETOUR_MEMBER_CALL(path);
        AddUpgradesToGameList();
    }

    bool player_is_downgrading = false;
    DETOUR_DECL_MEMBER(bool, CTFGameRules_CanUpgradeWithAttrib, CTFPlayer *player, int slot, int defindex, CMannVsMachineUpgrades *upgrade)
	{
        if (upgrade != nullptr && upgrade->m_iQuality == 9500) {
            return player_is_downgrading || from_buy_upgrade;
        }

        return DETOUR_MEMBER_CALL(player, slot, defindex, upgrade);
    }

    DETOUR_DECL_MEMBER(int, CTFGameRules_GetCostForUpgrade, CMannVsMachineUpgrades* upgrade, int slot, int classindex, CTFPlayer *player)
	{
        if (upgrade != nullptr && upgrade->m_iQuality == 9500)
            return upgrade->m_nCost;

        return DETOUR_MEMBER_CALL(upgrade, slot, classindex, player);
    }

    DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, unsigned short, CUpgrades_ApplyUpgradeToItem, CTFPlayer* player, CEconItemView *item, int upgrade, int cost, bool downgrade, bool fresh)
	{
        attrib_definition_index_t result = DETOUR_MEMBER_CALL(player, item, upgrade, cost, downgrade, fresh);
        upgrade_success = result != INVALID_ATTRIB_DEF_INDEX;

        DevMsg("ApplyUpgradeToItem %d %d %d %d\n", upgrade_success, upgrade, cost, downgrade);
        if (upgrade_success && extended_upgrades_start_index != -1 && upgrade >= extended_upgrades_start_index && upgrade < extended_upgrades_start_index + (int)upgrades.size()) {
            int slot = item == nullptr ? -1 : GetLoadoutSlotForItem(item->GetItemDefinition(), player->GetPlayerClass()->GetClassIndex());
            auto upgradeInfo = upgrades[upgrade - extended_upgrades_start_index];
            int cur_step;
            bool over_cap;
            int max_step = GetUpgradeStepData(player, slot, upgrade, cur_step, over_cap);
            
            DevMsg("^ %d/%d %e %e\n", cur_step, max_step, CMannVsMachineUpgradeManager::Upgrades()[upgrade].m_flCap, CMannVsMachineUpgradeManager::Upgrades()[upgrade].m_flIncrement);

            variant_t variant;
            variant.SetInt(cur_step);
            
            auto item_entity = GetEconEntityAtLoadoutSlot(player, slot);
            if (!downgrade) {
                FireOutputs(upgradeInfo->on_apply_outputs, variant, player, item_entity == nullptr ? (CBaseEntity *) player : item_entity);
            }
            else {
                FireOutputs(upgradeInfo->on_downgrade_outputs, variant, player, item_entity == nullptr ? (CBaseEntity *) player : item_entity);
            }
        }
        return result;
    }

    RefCount rc_CUpgrades_PlayerPurchasingUpgrade;
    RefCount rc_CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot;
    DETOUR_DECL_MEMBER(int, CTFItemDefinition_GetLoadoutSlot, int classIndex)
	{
		CTFItemDefinition *item_def = reinterpret_cast<CTFItemDefinition *>(this);
		int slot = DETOUR_MEMBER_CALL(classIndex);
		if (rc_CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot && item_def->m_iItemDefIndex != 0 && slot == -1 && classIndex != TF_CLASS_UNDEFINED) {
        	slot = item_def->GetLoadoutSlot(TF_CLASS_UNDEFINED);
        }
		return slot;
	}

    DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CUpgrades_PlayerPurchasingUpgrade, CTFPlayer *player, int itemslot, int upgradeslot, bool sell, bool free, bool b3)
	{
        SCOPED_INCREMENT(rc_CUpgrades_PlayerPurchasingUpgrade);
        player_is_downgrading = sell;
		DETOUR_MEMBER_CALL(player, itemslot, upgradeslot, sell, free, b3);
	}

	DETOUR_DECL_MEMBER(void, CPopulationManager_RestoreCheckpoint)
	{
		DETOUR_MEMBER_CALL();

		bought_upgrades_v2 = bought_upgrades_v2_checkpoint;
		ForEachTFPlayer([](CTFPlayer *player) {
            
            for (auto upgrade : upgrades) {
                int cur_step;
                bool over_cap;
                for (int slot = -1 ; slot <= LOADOUT_POSITION_ACTION; slot++) {
                    int max_step = GetUpgradeStepData(player, slot, upgrade->mvm_upgrade_index, cur_step, over_cap);
                    variant_t variant;
                    variant.SetInt(cur_step);
                    auto item = GetEconEntityAtLoadoutSlot(player, (int)slot);
                    FireOutputs(upgrade->on_restore_outputs, variant, player, item == nullptr ? (CBaseEntity *)player : item);
                }
            }
            // For new V2 upgrade type
            auto playerClass = player->GetPlayerClass()->GetClassIndex();
            for (auto &boughtInfo : bought_upgrades_v2[player->GetSteamID()]) {
                if (boughtInfo.playerClassIndex == playerClass) {
                    auto upgrade = upgrades_v2[boughtInfo.upgradeId];
                    if (boughtInfo.itemDefIndex == INVALID_ITEM_DEF_INDEX) {
                        FireOutputs(upgrade->on_restore_outputs, Variant(boughtInfo.level), player, player);
                    }
                    else {
                        ForEachTFPlayerEconEntity(player, [upgrade, player, &boughtInfo](CEconEntity *entity){
                            if (entity->GetItem() == nullptr || boughtInfo.itemId != entity->GetItem()->m_iItemID || boughtInfo.itemDefIndex != entity->GetItem()->m_iItemDefinitionIndex) return;

                            FireOutputs(upgrade->on_restore_outputs, Variant(boughtInfo.level), player, entity);
                        });
                    }
                }
            }
		});
	}

    DETOUR_DECL_MEMBER(void, CPopulationManager_SetCheckpoint, int wave)
	{
		DETOUR_MEMBER_CALL(wave);
        bought_upgrades_v2_checkpoint = bought_upgrades_v2;
	}

    DETOUR_DECL_MEMBER(void, CPopulationManager_ResetMap)
	{
        bought_upgrades_v2_checkpoint.clear();
        bought_upgrades_v2.clear();
		DETOUR_MEMBER_CALL();
	}

    RefCount rc_CUpgrades_GrantOrRemoveAllUpgrades;
    DETOUR_DECL_MEMBER(void, CUpgrades_GrantOrRemoveAllUpgrades, CTFPlayer * player, bool remove, bool refund)
	{
        SCOPED_INCREMENT_IF(rc_CUpgrades_GrantOrRemoveAllUpgrades, remove);
        bool respec = remove && refund;
        std::vector<std::pair<CEconEntity *, CEconItemAttributeDefinition *>> attributesToRemove;
        if (respec) {
            for (int upgrade = 0; upgrade < CMannVsMachineUpgradeManager::Upgrades().Count(); upgrade++) {
                auto &upgradeInfo = CMannVsMachineUpgradeManager::Upgrades()[upgrade];
                auto attribDef = GetItemSchema()->GetAttributeDefinitionByName(upgradeInfo.m_szAttribute);
                if (attribDef != nullptr) {
                    loadout_positions_t nLastLoadoutPos = LOADOUT_POSITION_MISC2;
                    for (int iItemSlot = -1; iItemSlot < nLastLoadoutPos; iItemSlot++) {
                        if (iItemSlot == LOADOUT_POSITION_ACTION) continue;

                        auto item = GetEconEntityAtLoadoutSlot(player, iItemSlot);
                        if ((iItemSlot == -1 && upgradeInfo.m_iUIGroup == 1) || (item != nullptr && item->GetItem() != nullptr && upgradeInfo.m_iUIGroup != 1)) {
                            
                            int currentUpgrade = 0;
                            if (upgrade >= extended_upgrades_start_index && extended_upgrades_start_index != -1 && upgrade < extended_upgrades_start_index + (int)upgrades.size()) {
                                bool overCap = false;
                                GetUpgradeStepData(player, iItemSlot, upgrade, currentUpgrade, overCap);
                            }
                            if (currentUpgrade > 0) {
                                auto upgradeInfoExtra = upgrades[upgrade - extended_upgrades_start_index];
                                if (upgradeInfoExtra->on_downgrade_outputs.size() > 0) {
                                    for (int j = currentUpgrade-1; j >= 0; j--) {
                                        variant_t variant;
                                        variant.SetInt(j);
                                        FireOutputs(upgradeInfoExtra->on_downgrade_outputs, variant, player, iItemSlot == -1 ? (CBaseEntity *)player : item);
                                    }
                                }
                            }
                            if (iItemSlot != -1) {
                                attributesToRemove.push_back({item, attribDef});
                            }
                        }
                    }
                }
            }

            for (auto &[item, attribDef] : attributesToRemove) {
                item->GetItem()->GetAttributeList().RemoveAttribute(attribDef);
            }

            
            // For new V2 upgrade type
            auto playerClass = player->GetPlayerClass()->GetClassIndex();
            for (auto &boughtInfo : bought_upgrades_v2[player->GetSteamID()]) {
                if (boughtInfo.playerClassIndex == playerClass) {
                    auto upgrade = upgrades_v2[boughtInfo.upgradeId];
                    if (boughtInfo.itemDefIndex == INVALID_ITEM_DEF_INDEX) {
                        for (int j = boughtInfo.level-1; j >= 0; j--) {
                            FireOutputs(upgrade->on_downgrade_outputs, Variant(j), player, player);
                        }
                    }
                    else {
                        ForEachTFPlayerEconEntity(player, [player, upgrade, &boughtInfo](CEconEntity *entity){
                            if (entity->GetItem() == nullptr || boughtInfo.itemId != entity->GetItem()->m_iItemID || boughtInfo.itemDefIndex != entity->GetItem()->m_iItemDefinitionIndex) return;

                            for (int j = boughtInfo.level-1; j >= 0; j--) {
                                FireOutputs(upgrade->on_downgrade_outputs, Variant(j), player, entity);
                            }
                            entity->GetItem()->GetAttributeList().RemoveAttribute(upgrade->attributeDefinition);
                            for (auto &[attrDef, value] : upgrade->secondary_attributes) {
                                entity->GetItem()->GetAttributeList().RemoveAttribute(attrDef);
                            }
                        });
                    }
                }
            }
            bought_upgrades_v2.clear();
        }
        DETOUR_MEMBER_CALL(player, remove, refund);
    }

    RefCount rc_GetUpgradeStepData;
    DETOUR_DECL_STATIC(CEconItemView *, CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot, CTFPlayer *player, int slot, CEconEntity **entity)
	{
        SCOPED_INCREMENT(rc_CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot);
        auto result = DETOUR_STATIC_CALL(player, slot, entity);
        if (rc_GetUpgradeStepData && entity != nullptr && rtti_cast<CTFPowerupBottle *>(*entity) != nullptr) {
            *entity = nullptr;
        }
        return result;
    }
    
    DETOUR_DECL_STATIC(int, GetUpgradeStepData, CTFPlayer *player, int slot, int upgrade, int& cur_step, bool& over_cap)
	{
        SCOPED_INCREMENT_IF(rc_GetUpgradeStepData, extended_upgrades_start_index != -1 && upgrade >= extended_upgrades_start_index);
        if (upgrade >= 0 && upgrade <= CMannVsMachineUpgradeManager::Upgrades().Count()) {
            auto &upgradeInfo = CMannVsMachineUpgradeManager::Upgrades()[upgrade];
            auto attribDef = GetItemSchema()->GetAttributeDefinitionByName(upgradeInfo.m_szAttribute);

            if (attribDef != nullptr && attribDef->IsType<CSchemaAttributeType_String>()) {
                CAttributeList *attribList = nullptr;
                if (slot == -1) {
                    attribList = player->GetAttributeList();
                }
                else {
                    auto item = GetEconEntityAtLoadoutSlot(player, (int)slot);
                    if (item != nullptr) {
                        attribList = &item->GetItem()->GetAttributeList();
                    }
                }
                if (attribList != nullptr) {
                    auto attr = attribList->GetAttributeByID(attribDef->GetIndex());
                    
                    over_cap = false;
                    cur_step = attr != nullptr ? 1 : 0;
                    return 1;
                }
            }
        }
		auto result = DETOUR_STATIC_CALL(player, slot, upgrade, cur_step, over_cap);
        // To reduce refund execution time, only count steps that are bought for the item
        if (rc_CUpgrades_GrantOrRemoveAllUpgrades && result > cur_step) {
            return cur_step;
        }
		return result;
	}

    #warning __gcc_regcall detours considered harmful!
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, attrib_definition_index_t, ApplyUpgrade_Default, const CMannVsMachineUpgrades& upgrade, CTFPlayer *pTFPlayer, CEconItemView *pEconItemView, int nCost, bool bDowngrade)
	{
        auto attribDef = GetItemSchema()->GetAttributeDefinitionByName(upgrade.m_szAttribute);
        if (attribDef != nullptr && attribDef->IsType<CSchemaAttributeType_String>()) {
            CAttributeList *attribList = nullptr;
            if (upgrade.m_iUIGroup == 1) {
                attribList = pTFPlayer->GetAttributeList();
            }
            else if (pEconItemView != nullptr) {
                attribList = &pEconItemView->GetAttributeList();
            }
            if (attribList == nullptr) return INVALID_ATTRIB_DEF_INDEX;

            if (bDowngrade) {
                if (attribList->GetAttributeByID(attribDef->GetIndex()) != nullptr) {
                    attribList->RemoveAttribute(attribDef);
                    return (attrib_definition_index_t) attribDef->GetIndex();
                }
                else {
                    return INVALID_ATTRIB_DEF_INDEX;
                }
            }
            else {
                attribList->SetRuntimeAttributeValue(attribDef, upgrade.m_flIncrement);
	            attribList->SetRuntimeAttributeRefundableCurrency(attribDef, nCost);
                return (attrib_definition_index_t) attribDef->GetIndex();
            }
            return INVALID_ATTRIB_DEF_INDEX;
        }
        return DETOUR_STATIC_CALL(upgrade, pTFPlayer, pEconItemView, nCost, bDowngrade);
    }
    DETOUR_DECL_MEMBER(void, CTFPlayer_GiveDefaultItems)
	{
		if (rc_CUpgrades_PlayerPurchasingUpgrade > 0 && buying_upgrade) {
            return;
        }
        DETOUR_MEMBER_CALL();
    }

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("MvM:Extended_Upgrades")
		{
            MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse, "CPopulationManager::Parse");
            MOD_ADD_DETOUR_MEMBER(CMannVsMachineUpgradeManager_LoadUpgradesFileFromPath, "CMannVsMachineUpgradeManager::LoadUpgradesFileFromPath");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_CanUpgradeWithAttrib, "CTFGameRules::CanUpgradeWithAttrib");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetCostForUpgrade, "CTFGameRules::GetCostForUpgrade");
            MOD_ADD_DETOUR_MEMBER(CUpgrades_ApplyUpgradeToItem, "CUpgrades::ApplyUpgradeToItem [clone]");
            MOD_ADD_DETOUR_MEMBER(CTFItemDefinition_GetLoadoutSlot, "CTFItemDefinition::GetLoadoutSlot");
            MOD_ADD_DETOUR_MEMBER(CUpgrades_PlayerPurchasingUpgrade, "CUpgrades::PlayerPurchasingUpgrade [clone]");
            MOD_ADD_DETOUR_MEMBER(CPopulationManager_RestoreCheckpoint, "CPopulationManager::RestoreCheckpoint");

            // Don't reset weapons in the middle of applying secondary attributes
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_GiveDefaultItems, "CTFPlayer::GiveDefaultItems");

            // Fix message overload when refunding
            MOD_ADD_DETOUR_MEMBER(CUpgrades_GrantOrRemoveAllUpgrades, "CUpgrades::GrantOrRemoveAllUpgrades");
            

            // Properly track string attribute upgrades
            MOD_ADD_DETOUR_STATIC(GetUpgradeStepData, "GetUpgradeStepData");
            MOD_ADD_DETOUR_STATIC(ApplyUpgrade_Default, "ApplyUpgrade_Default");

            // Remove hardcoded bottle upgrade steps
            //MOD_ADD_DETOUR_STATIC(CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot, "CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot");
            
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void FrameUpdatePostEntityThink() override
		{
			if (!TFGameRules()->IsMannVsMachineMode()) return;

            
            for (auto it = in_upgrade_zone.begin(); it != in_upgrade_zone.end();) {
                if ((*it).Get() == nullptr || !(*it).Get()->m_Shared->m_bInUpgradeZone) {
                    if ((*it).Get() != nullptr) {
                        
                        auto kv = new KeyValues("MvM_UpgradesDone");
                        serverGameClients->ClientCommandKeyValues((*it).Get()->edict(), kv);
		                kv->deleteThis();
                    }
                    it = in_upgrade_zone.erase(it);
                }
                else {
                    it++;
                }
            }

            ForEachTFPlayer([&](CTFPlayer *player){
                if (player->IsBot()) return;

                if (!player->m_Shared->m_bInUpgradeZone && player->GetCustomVariableBool<"wasinupgradezone">()) {
                    player->SetCustomVariable("wasinupgradezone", Variant(false));
                }
                if (player->m_Shared->m_bInUpgradeZone && !upgrades.empty() && in_upgrade_zone.count(player) == 0) {
                    in_upgrade_zone.insert(player);
                    auto kv = new KeyValues("MvM_UpgradesBegin");
                    serverGameClients->ClientCommandKeyValues(player->edict(), kv);
		            kv->deleteThis();
                }

                int class_index = player->GetPlayerClass()->GetClassIndex();
               
                if (player->m_Shared->m_bInUpgradeZone && menus->GetDefaultStyle()->GetClientMenu(ENTINDEX(player), nullptr) != MenuSource_BaseMenu && g_hUpgradeEntity.GetRef() != nullptr && (!upgrades.empty() || Mod::Pop::PopMgr_Extensions::HasExtraLoadoutItems(class_index))) {
                    bool found = false;
                    bool found_any = false;
                    bool has_extra = Mod::Pop::PopMgr_Extensions::HasExtraLoadoutItems(class_index);

                    for (loadout_positions_t slot : {
                        LOADOUT_POSITION_PRIMARY,
                        LOADOUT_POSITION_SECONDARY,
                        LOADOUT_POSITION_MELEE,
                        LOADOUT_POSITION_BUILDING,
                        LOADOUT_POSITION_PDA,
                        LOADOUT_POSITION_ACTION,
                    }) {
                        auto *item = GetEconEntityAtLoadoutSlot(player, (int)slot);

                        bool found_now = item != nullptr && WeaponHasValidUpgrades(item, player);
                        found_any |= found_now;
                        if (found_now && item == player->GetActiveWeapon()) {
                            
                            if (!WeaponHasValidUpgrades(nullptr, player) && !has_extra) {
                                found = true;
                                StartUpgradeListForPlayer(player, (int)slot, 0);
                            }
                            break;
                        }
                    }

                    if (!found) {
                        if (!found_any && !has_extra) {
                            if (WeaponHasValidUpgrades(nullptr, player))
                                StartUpgradeListForPlayer(player, -1, 0);
                        }
                        else if (!found_any && has_extra && !WeaponHasValidUpgrades(nullptr, player)) {
                            if (!player->GetCustomVariableBool<"wasinupgradezone">()) {
                                player->SetCustomVariable("wasinupgradezone", Variant(true));
                                StartMenuForPlayer(player);
                                Mod::Pop::PopMgr_Extensions::DisplayExtraLoadoutItemsClass(player, class_index, true);
                            }
                        }
                        else {
                            StartMenuForPlayer(player);
                        }
                    }
                }
                else if (!player->m_Shared->m_bInUpgradeZone && CurrentMenuIsUpgrade(player)) {
                    StopMenuForPlayer(player);
                }
            });
            
		}

        virtual void OnEnable() override
        {
        }

        virtual void OnDisable() override
        {
            ForEachTFPlayer([&](CTFPlayer *player){
                if (!player->IsBot())
                    menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player));
            });
        }
        
        virtual void LevelInitPreEntity() override
		{
            

            ClearUpgrades();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			ClearUpgrades();
		}

		virtual std::vector<std::string> GetRequiredMods() { return {"Item:Item_Common"};}
	};
	CMod s_Mod;

	ConVar cvar_enable("sig_mvm_extended_upgrades", "0", FCVAR_NOTIFY,
		"Mod: enable extended upgrades",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
            
		});
}
