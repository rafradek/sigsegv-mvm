#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/econ.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "util/iterate.h"
#include "util/misc.h"
#include "mod/pop/common.h"
#include "stub/upgrades.h"
#include "stub/entities.h"
#include "stub/strings.h"
#include "stub/tf_objective_resource.h"

namespace Mod::Pop::PopMgr_Extensions {
    bool ExtendedUpgradesNoUndo();
}

namespace Mod::MvM::Extended_Upgrades
{

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
            return FStrEq(m_name.c_str(), item->GetItemDefinition()->GetName());
        }

        virtual void ParseKey(KeyValues *kv) override {
            if (FStrEq(kv->GetName(), "Name"))
                m_name = kv->GetString();
        }

    private:
        std::string m_name;
    };

    class UpgradeCriteriaHasDamage : public UpgradeCriteria {
    public:
        UpgradeCriteriaHasDamage() {}
        
        virtual bool Matches(CEconEntity *ent, CEconItemView *item, CTFPlayer *owner) override {
            auto weapon = rtti_cast<CTFWeaponBase *>(ent);
            if (weapon != nullptr) {
                int weaponid = weapon->GetWeaponID();
                return !(weaponid == TF_WEAPON_NONE || 
							weaponid == TF_WEAPON_LASER_POINTER || 
							weaponid == TF_WEAPON_MEDIGUN || 
							weaponid == TF_WEAPON_BUFF_ITEM ||
							weaponid == TF_WEAPON_BUILDER ||
							weaponid == TF_WEAPON_PDA_ENGINEER_BUILD ||
							weaponid == TF_WEAPON_INVIS ||
							weaponid == TF_WEAPON_SPELLBOOK);
            }
            return false;
        }

        virtual void ParseKey(KeyValues *kv) override {

        }
    };

    class UpgradeCriteriaWeaponSlot : public UpgradeCriteria {
    public:
        UpgradeCriteriaWeaponSlot(int slot) : slot(slot) {}
        
        virtual bool Matches(CEconEntity *ent, CEconItemView *item, CTFPlayer *owner) override {

            return item->GetItemDefinition()->GetLoadoutSlot(owner->GetPlayerClass()->GetClassIndex()) == this->slot;
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
        std::map<std::string, float> secondary_attributes;
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
        std::map<std::string, float> on_upgrade_outputs = {};
        bool force_enable = false;
    };

    std::vector<UpgradeInfo *> upgrades; 
    std::map<std::string, UpgradeInfo *> upgrades_ref;

    struct UpgradeHistory {
        UpgradeInfo *upgrade;
        int player_class;
        int item_def_index;
        int cost;
    };

    struct PlayerUpgradeHistory {
        std::vector<UpgradeHistory> upgrades;
        int currency_spent;
        CSteamID steam_id;
    };

    std::vector<PlayerUpgradeHistory> upgrade_history;
    std::vector<PlayerUpgradeHistory> upgrade_snapshot;

    std::map<CHandle<CTFPlayer>, IBaseMenu *> select_type_menus;

    std::set<CHandle<CTFPlayer>> in_upgrade_zone;

    int extended_upgrades_start_index = -1;

    bool BuyUpgrade(UpgradeInfo *upgrade,/* CEconEntity *item*/ int slot, CTFPlayer *player, bool free, bool downgrade);

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
                auto kv = new KeyValues("MVM_Respec");
                serverGameClients->ClientCommandKeyValues(player->GetNetworkable()->GetEdict(), kv);
                return;
            }
            else {
                slotid = strtol(info, nullptr, 10);
            }
            StartUpgradeListForPlayer(player, slotid, 0);
        }

        void OnMenuDestroy(IBaseMenu *menu) {
            if (select_type_menus[player] == menu)
                select_type_menus.erase(player);
            delete this;
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
                
            int upgrade_id = strtol(info, nullptr, 10);
            if (upgrade_id < 1000)
                BuyUpgrade(upgrades[upgrade_id], this->slot, this->player, false, false);
            else if ((upgrade_id == 1000) && (!Mod::Pop::PopMgr_Extensions::ExtendedUpgradesNoUndo())) {
                DevMsg("Undoing %d %d\n", extended_upgrades_start_index, CMannVsMachineUpgradeManager::Upgrades().Count());
                for (int i = extended_upgrades_start_index; i < CMannVsMachineUpgradeManager::Upgrades().Count(); i++) {
                    int cur_step;
                    bool over_cap;
                    int max_step = GetUpgradeStepData(this->player, this->slot, i, cur_step, over_cap);
                   // DevMsg("Undoing upgrade %d %d\n", cur_step, max_step);
                    for (int j = 0; j < cur_step; j++) {
                        DevMsg("Undoing upgrade %d %d\n", j, cur_step);
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
            if (reason == MenuEnd_ExitBack) {
                if (select_type_menus[player] == menu)
                    select_type_menus.erase(player);
                StartMenuForPlayer(player);
            }
		}

        void OnMenuDestroy(IBaseMenu *menu) {
            if (select_type_menus[player] == menu)
                select_type_menus.erase(player);
            delete this;
        }

        CHandle<CTFPlayer> player;
        int slot;
    };

    const char* GetIncrementStringForAttribute(CEconItemAttributeDefinition *attr, float value) {
        if (FStrEq(attr->GetDescriptionFormat(), "value_is_percentage")) {
            return CFmtStr("%+2f%% %s", value, attr->GetName());
        }
        else if (FStrEq(attr->GetDescriptionFormat(), "value_is_inverted_percentage")) {
            value *= -1.0f;
            return CFmtStr("%+2f%% %s", value, attr->GetName());
        }
        else if (FStrEq(attr->GetDescriptionFormat(), "value_is_additive")) {
            return CFmtStr("%2f%% %s", value, attr->GetName());
        }
        else if (FStrEq(attr->GetDescriptionFormat(), "value_is_additive_percentage")) {
            value *= 100.0f;
            return CFmtStr("%2f%% %s", value, attr->GetName());
        }
        return "";
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

    void Parse_SecondaryAttributes(KeyValues *kv, std::map<std::string, float> &secondary_attributes) {
        FOR_EACH_SUBKEY(kv, subkey) {
            secondary_attributes[subkey->GetName()] = subkey->GetFloat();
        }
    }

    void Parse_OnUpgradeOutputs(KeyValues* kv, std::map<std::string, float>& outputs){
        FOR_EACH_SUBKEY(kv, subkey) {
            outputs[subkey->GetName()] = subkey->GetFloat();
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
        else if (V_stricmp(string, "Canteen") == 0)
            slot = 9;
        else
            slot = strtol(string, nullptr, 10);
        return slot;
    }

    bool Parse_Criteria(KeyValues *kv, std::vector<std::unique_ptr<UpgradeCriteria>> &criteria) {
        FOR_EACH_SUBKEY(kv, subkey) {
            if (FStrEq(subkey->GetName(), "Classname"))
                criteria.push_back(std::make_unique<UpgradeCriteriaClassname>(subkey->GetString()));
            else if (FStrEq(subkey->GetName(), "ItemName"))
                criteria.push_back(std::make_unique<UpgradeCriteriaName>(subkey->GetString()));
            else if (FStrEq(subkey->GetName(), "DealsDamage"))
                criteria.push_back(std::make_unique<UpgradeCriteriaHasDamage>());
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
            upgrade_game.m_iUIGroup = upgrade->playerUpgrade ? 1 : 0;
            upgrade_game.m_iQuality = 9500;
            upgrade_game.m_iTier = 0;

            for(auto entry : upgrade->secondary_attributes) {
                upgrade_index = CMannVsMachineUpgradeManager::Upgrades().AddToTail();
                CMannVsMachineUpgrades &upgrade_game_child = CMannVsMachineUpgradeManager::Upgrades()[upgrade_index];
                upgrade->secondary_attributes_index.push_back(upgrade_index);
                strcpy(upgrade_game_child.m_szAttribute, entry.first.c_str());

                upgrade_game_child.m_flCap = entry.second > 0 ? 1.0f + entry.second * 64 : entry.second * 64;
                upgrade_game_child.m_flIncrement = entry.second;
                upgrade_game_child.m_nCost = 0;
                upgrade_game_child.m_iUIGroup = upgrade->playerUpgrade ? 1 : 0;
                upgrade_game_child.m_iQuality = 9500;
                upgrade_game_child.m_iTier = 0;
            }
        }
    }

    void Parse_ExtendedUpgrades(KeyValues *kv) {
        FOR_EACH_SUBKEY(kv, subkey) {
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
                    //upgradeinfo->on_upgrade_output = subkey2->GetString();
                    Parse_OnUpgradeOutputs(subkey2, upgradeinfo->on_upgrade_outputs);
                }
                else if (FStrEq(subkey2->GetName(), "ForceEnable")) {
                    upgradeinfo->force_enable = true;
                }
            }

            if (upgradeinfo->attributeDefinition == nullptr) {
                delete upgradeinfo;// upgrades.erase(upgradeinfo);
            }
            else {
                upgrades.push_back(upgradeinfo);
                upgrades_ref[upgradeinfo->ref_name] = upgradeinfo;


                if (upgradeinfo->name == "")
                    upgradeinfo->name = GetIncrementStringForAttribute(upgradeinfo->attributeDefinition, upgradeinfo->increment); //upgradeinfo->attributeDefinition->GetKVString("description_string", );
            }
		}
        RemoveUpgradesFromGameList();
        AddUpgradesToGameList();
    }
    

    void ClearUpgrades() {
        std::vector<CHandle<CTFPlayer>> vecmenus;
        for (auto pair : select_type_menus) {
            vecmenus.push_back(pair.first);
        }

        for (auto player : vecmenus) {
            IBaseMenu *menu = select_type_menus[player];
            if (player != nullptr)
                menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player));
                
            menus->CancelMenu(menu);
            menu->Destroy();
        }
        select_type_menus.clear();

        for (auto upgrade : upgrades) {
            delete upgrade;
        }
        upgrades.clear();
        upgrades_ref.clear();

        upgrade_history.clear();
        upgrade_snapshot.clear();
        RemoveUpgradesFromGameList();
        DevMsg("Total updates count: %d %d\n", CMannVsMachineUpgradeManager::Upgrades().Count(), extended_upgrades_start_index);
    }

    bool IsValidUpgradeForWeapon(UpgradeInfo *upgrade, CEconEntity *item, CTFPlayer *player, char *reason, int reasonlen) {
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
            snprintf(reason, reasonlen, "Available from wave %d", upgrade->allow_wave_min);
            return false;
        }
        else if (wave > upgrade->allow_wave_max) {
            snprintf(reason, reasonlen, "Available up to wave %d", upgrade->allow_wave_max);
            return false;
        }

        for (auto entry : upgrade->required_upgrades) {
            UpgradeInfo *child = upgrades_ref[entry.first];
            if (child != nullptr) {
                int cur_step;
                bool over_cap;
                int max_step = GetUpgradeStepData(player, upgrade->playerUpgrade ? -1 : item->GetItem()->GetItemDefinition()->GetLoadoutSlot(player_class), child->mvm_upgrade_index, cur_step, over_cap);
                if (cur_step < entry.second) {
                    snprintf(reason, reasonlen, "Requires %s upgraded to level %d", child->name.c_str(), entry.second);
                    return false;
                }
            }
        }

        for (auto entry : upgrade->disallowed_upgrades) {
            UpgradeInfo *child = upgrades_ref[entry.first];
            DevMsg("disallowed upgrade %s %d\n",entry.first.c_str(), child != nullptr);
            if (child != nullptr) {
                int cur_step;
                bool over_cap;
                int max_step = GetUpgradeStepData(player, upgrade->playerUpgrade ? -1 : item->GetItem()->GetItemDefinition()->GetLoadoutSlot(player_class), child->mvm_upgrade_index, cur_step, over_cap);
                DevMsg("%d %d\n", cur_step, entry.second);
                if (cur_step >= entry.second) {
                    snprintf(reason, reasonlen, "Incompatible with %s upgrade", child->name.c_str());
                    return false;
                }
            }
        }

        if (!upgrade->required_upgrades_option.empty()) {
            bool found = false;
            for (auto &entry : upgrade->required_upgrades_option) {
                UpgradeInfo *child = upgrades_ref[entry.first];
                if (child != nullptr) {
                    int cur_step;
                    bool over_cap;
                    int max_step = GetUpgradeStepData(player, upgrade->playerUpgrade ? -1 : item->GetItem()->GetItemDefinition()->GetLoadoutSlot(player_class), child->mvm_upgrade_index, cur_step, over_cap);
                    if (cur_step >= entry.second) {
                        found = true;
                    }
                    else {
                        snprintf(reason, reasonlen, "Requires %s upgraded to level %d", child->name.c_str(), entry.second);
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
                    found = criteria->Matches(item_require, item->GetItem(), player);
                    if (found)
                        break;
                }
                if (found)
                        break;
            }
            if (!found) {
                V_strncpy(reason, upgrade->required_weapons_string.c_str(), reasonlen);
                return false;
            }
        }

        return true;
    }

    bool WeaponHasValidUpgrades(CEconEntity *item, CTFPlayer *player) {
        for (int i = 0; i < upgrades.size(); i++) {
            auto upgrade = upgrades[i];
            char reason[1] = "";
            if (IsValidUpgradeForWeapon(upgrade, item, player, reason, sizeof(reason)) || (strlen(reason) > 0 && upgrade->show_requirements)) {
                return true;
            }
        }
        return false;
    }

    /*bool CanBuyUpgrade(UpgradeInfo *upgrade, CEconEntity *item, CTFPlayer *player) {
        if (!IsValidUpgradeForWeapon(upgrade, item, player))
            return false;
        
        return true;
    }*/

    /*std::vector<UpgradeHistory> *GetUpgradeHistory(CTFPlayer *player) {
        CSteamID steam_id;
        if (!player->GetSteamID(&steam_id)) 
            return nullptr;

        for (auto &history : upgrade_history) {
            if (history.steam_id == steam_id) {
                return &history.upgrades;
            }
        }
        
        auto history = upgrade_history.emplace(upgrade_history.end());
        history->steam_id = steam_id;
        history->currency_spent = 0;
        return &history->upgrades;
    }*/

    /*void RememberUpgrade(UpgradeInfo *upgrade, CEconItemView *item, CTFPlayer *player, int cost, bool downgrade) {
        std::vector<UpgradeHistory> *player_history_upgrades = GetUpgradeHistory(player);
        if (player_history_upgrades != nullptr) {

            int item_def_index = -1;
            if (item != nullptr)
                item_def_index = item->GetItemDefIndex();

            if (!downgrade) {
                UpgradeHistory history;
                history.upgrade = upgrade;
                history.cost = cost;
                history.player_class = player->GetPlayerClass()->GetClassIndex();
                history.item_def_index = item_def_index;
                player_history_upgrades->push_back(history);
            }
            else
            {
                for (int i = 0; i < player_history_upgrades->size(); i++) {
                    UpgradeHistory history = *(player_history_upgrades)[i];
                    if (history.upgrade == upgrade && history.item_def_index == item_def_index && history.cost == -cost) {
                        player_history_upgrades->erase(player_history_upgrades->begin()+i);
                        break;
                    }
                }
            }
        }
    }*/

    bool from_buy_upgrade_free = false;
    bool upgrade_success = false;

    bool BuyUpgrade(UpgradeInfo *upgrade,/* CEconEntity *item*/ int slot, CTFPlayer *player, bool free, bool downgrade) {
        if (g_hUpgradeEntity.GetRef() == nullptr) {
            DevMsg("No upgrade entity?\n");
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
                g_hUpgradeEntity->PlayerPurchasingUpgrade(player, override_slot, attr, downgrade, true, false);
            }
            from_buy_upgrade_free = false;
            //if(upgrade->on_upgrade_output != ""){
            for(const auto& [output, delay] : upgrade->on_upgrade_outputs){
                char param_tokenized[2048] = "";
                V_strncpy(param_tokenized, output.c_str(), sizeof(param_tokenized));
                if(strcmp(param_tokenized, "") != 0){
                    char *target = strtok(param_tokenized,",");
                    char *action = NULL;
                    char *value = NULL;
                    if(target != NULL)
                        action = strtok(NULL,",");
                    if(action != NULL)
                        value = strtok(NULL,"");
                    if(value != NULL){
                        CEventQueue &que = g_EventQueue;
                        variant_t actualvalue;
                        string_t stringvalue = AllocPooledString(value);
                        actualvalue.SetString(stringvalue);
                        que.AddEvent(STRING(AllocPooledString(target)), STRING(AllocPooledString(action)), actualvalue, delay, player, player, -1);
                    }
                }
            }
        }
            //if (!free)
            //    player->RemoveCurrency(cost);

            //RememberUpgrade(upgrade, item->GetItem(), player, cost, downgrade);
            //return true;
        //}
        from_buy_upgrade = false;
        return upgrade_success;
    }

    void StartUpgradeListForPlayer(CTFPlayer *player, int slot, int displayitem) {
        DevMsg("Start upgrade list %d\n", slot);
        menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player));
        SelectUpgradeListHandler *handler = new SelectUpgradeListHandler(player, slot);
        IBaseMenu *menu = menus->GetDefaultStyle()->CreateMenu(handler);
        
        CEconEntity *item = GetEconEntityAtLoadoutSlot(player, slot);
        if (slot != -1 && item == nullptr) return;

        if (slot == -1)
            menu->SetDefaultTitle("Player Upgrades");
        else {
            menu->SetDefaultTitle(CFmtStr("Upgrades for %s", g_Itemnames[item->GetItem()->m_iItemDefinitionIndex].c_str()));
        }
        menu->SetMenuOptionFlags(MENUFLAG_BUTTON_EXITBACK);

        for (int i = 0; i < upgrades.size(); i++) {
            auto upgrade = upgrades[i];
            char disabled_reason[255] = "";
            bool enabled = IsValidUpgradeForWeapon(upgrade, item, player, disabled_reason, sizeof(disabled_reason));
            if (upgrade->force_enable) enabled = true;
            if (enabled) {
                int cur_step;
                bool over_cap;
                int max_step = GetUpgradeStepData(player, slot, upgrade->mvm_upgrade_index, cur_step, over_cap);

                char text[128];
                snprintf(text, 128, "%s (%d/%d) $%d", upgrade->name.c_str(), cur_step, max_step, upgrade->cost);

                ItemDrawInfo info1(text, 
                    cur_step >= max_step || player->GetCurrency() < upgrade->cost ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);


                static char buf[4];
                snprintf(buf, sizeof(buf), "%d", (int)i);
                menu->AppendItem(buf, info1);

                if (!upgrade->description.empty()) {
                    ItemDrawInfo infodesc(CFmtStr("^ %s", upgrade->description.c_str()), ITEMDRAW_DISABLED);
                    menu->AppendItem("9999", infodesc);
                }
            }
            else if (upgrade->show_requirements && strlen(disabled_reason) > 0) {
                int cur_step;
                bool over_cap;
                int max_step = GetUpgradeStepData(player, slot, upgrade->mvm_upgrade_index, cur_step, over_cap);

                char text[128];
                snprintf(text, 128, "%s: %s (%d/%d) $%d", upgrade->name.c_str(), disabled_reason, cur_step, max_step, upgrade->cost);

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
            ItemDrawInfo info1("Undo upgrades");
            menu->AppendItem("1000", info1);
        }
        /*if (upgrades.size() == 1) {
            ItemDrawInfo info1(" ", ITEMDRAW_NOTEXT);
            menu->AppendItem(" ", info1);
        }*/

        select_type_menus[player] = menu;
        DevMsg("Displaying %d\n", slot);
        menu->DisplayAtItem(ENTINDEX(player), 0, displayitem);
    }

    void StartMenuForPlayer(CTFPlayer *player) {
        if (select_type_menus.find(player) != select_type_menus.end()) return;
        
        SelectUpgradeWeaponHandler *handler = new SelectUpgradeWeaponHandler(player);
        IBaseMenu *menu = menus->GetDefaultStyle()->CreateMenu(handler);
        
        menu->SetDefaultTitle("Extended Upgrades Menu");
        menu->SetMenuOptionFlags(0);

        ItemDrawInfo info1("Player Upgrades", WeaponHasValidUpgrades(nullptr, player) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
        menu->AppendItem("player", info1);

        for (loadout_positions_t slot : {
			LOADOUT_POSITION_PRIMARY,
			LOADOUT_POSITION_SECONDARY,
			LOADOUT_POSITION_MELEE,
			LOADOUT_POSITION_BUILDING,
			LOADOUT_POSITION_PDA,
			LOADOUT_POSITION_ACTION,
		}) {
            CEconEntity *item = GetEconEntityAtLoadoutSlot(player, (int)slot);
            if (item != nullptr) {
                ItemDrawInfo info2(g_Itemnames[item->GetItem()->m_iItemDefinitionIndex].c_str(), WeaponHasValidUpgrades(item, player) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
                
                static char buf[4];
                snprintf(buf, sizeof(buf), "%d", (int)slot);
                menu->AppendItem(buf, info2);
            }
        }

        ConVarRef tf_mvm_respec_enabled("tf_mvm_respec_enabled");
        ItemDrawInfo info2("Refund Upgrades", tf_mvm_respec_enabled.GetBool() ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
        menu->AppendItem("refund", info2);

        select_type_menus[player] = menu;

        menu->Display(ENTINDEX(player), 0);
    }

    void StopMenuForPlayer(CTFPlayer *player) {
        if (select_type_menus.find(player) == select_type_menus.end()) return;
        DevMsg("Stopped menu\n");
        IBaseMenu *menu = select_type_menus[player];
        menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player));
        menus->CancelMenu(menu);
        menu->Destroy();

        menu = menus->GetDefaultStyle()->CreateMenu(new SelectUpgradeWeaponHandler(player));
        menu->SetDefaultTitle(" ");
        menu->SetMenuOptionFlags(0);
        ItemDrawInfo info1(" ", ITEMDRAW_NOTEXT);
        menu->AppendItem(" ", info1);
        ItemDrawInfo info2(" ", ITEMDRAW_NOTEXT);
        menu->AppendItem(" ", info2);
        menu->Display(ENTINDEX(player), 1);
    }

    DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
        ClearUpgrades();
        return DETOUR_MEMBER_CALL(CPopulationManager_Parse)();
    }

    DETOUR_DECL_MEMBER(void, CMannVsMachineUpgradeManager_LoadUpgradesFileFromPath, const char *path)
	{
        DETOUR_MEMBER_CALL(CMannVsMachineUpgradeManager_LoadUpgradesFileFromPath)(path);
        AddUpgradesToGameList();
    }

    DETOUR_DECL_MEMBER(bool, CTFGameRules_CanUpgradeWithAttrib, CTFPlayer *player, int slot, int defindex, CMannVsMachineUpgrades *upgrade)
	{
        if (upgrade != nullptr && upgrade->m_iQuality == 9500)
            return from_buy_upgrade;

        return DETOUR_MEMBER_CALL(CTFGameRules_CanUpgradeWithAttrib)(player, slot, defindex, upgrade);
    }

    DETOUR_DECL_MEMBER(int, CTFGameRules_GetCostForUpgrade, CMannVsMachineUpgrades* upgrade, int slot, int classindex, CTFPlayer *player)
	{
        if (upgrade != nullptr && upgrade->m_iQuality == 9500)
            return upgrade->m_nCost;

        return DETOUR_MEMBER_CALL(CTFGameRules_GetCostForUpgrade)(upgrade, slot, classindex, player);
    }

    DETOUR_DECL_MEMBER(attrib_definition_index_t, CUpgrades_ApplyUpgradeToItem, CTFPlayer* player, CEconItemView *item, int upgrade, int cost, bool downgrade, bool fresh)
	{
        attrib_definition_index_t result = DETOUR_MEMBER_CALL(CUpgrades_ApplyUpgradeToItem)(player, item, upgrade, cost, downgrade, fresh);
        upgrade_success = result != INVALID_ATTRIB_DEF_INDEX;

        return result;
    }

    RefCount rc_CUpgrades_PlayerPurchasingUpgrade;
    RefCount rc_CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot;
    DETOUR_DECL_MEMBER(int, CTFItemDefinition_GetLoadoutSlot, int classIndex)
	{
		CTFItemDefinition *item_def = reinterpret_cast<CTFItemDefinition *>(this);
		int slot = DETOUR_MEMBER_CALL(CTFItemDefinition_GetLoadoutSlot)(classIndex);
		if (rc_CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot && rc_CUpgrades_PlayerPurchasingUpgrade && slot == -1 && classIndex != TF_CLASS_UNDEFINED)
			slot = item_def->GetLoadoutSlot(TF_CLASS_UNDEFINED);
		return slot;
	}

    DETOUR_DECL_MEMBER(CEconItemView *, CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot, CTFPlayer *player, int slot, CBaseEntity &entity)
	{
        SCOPED_INCREMENT(rc_CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot);
		DETOUR_MEMBER_CALL(CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot)(player, slot, entity);
	}

    DETOUR_DECL_MEMBER(void, CUpgrades_PlayerPurchasingUpgrade, CTFPlayer *player, int itemslot, int upgradeslot, bool sell, bool free, bool b3)
	{
        SCOPED_INCREMENT(rc_CUpgrades_PlayerPurchasingUpgrade);
		DETOUR_MEMBER_CALL(CUpgrades_PlayerPurchasingUpgrade)(player, itemslot, upgradeslot, sell, free, b3);
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
            MOD_ADD_DETOUR_MEMBER(CUpgrades_ApplyUpgradeToItem, "CUpgrades::ApplyUpgradeToItem");
            MOD_ADD_DETOUR_MEMBER(CTFItemDefinition_GetLoadoutSlot, "CTFItemDefinition::GetLoadoutSlot");
            MOD_ADD_DETOUR_MEMBER(CUpgrades_PlayerPurchasingUpgrade, "CUpgrades::PlayerPurchasingUpgrade");
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void FrameUpdatePostEntityThink() override
		{
			if (!TFGameRules()->IsMannVsMachineMode()) return;

            
            for (auto it = in_upgrade_zone.begin(); it != in_upgrade_zone.end();) {
                if ((*it).Get() == nullptr || !(*it).Get()->m_Shared->m_bInUpgradeZone) {
                    if ((*it).Get() != nullptr) {
                        
                        auto kv = new KeyValues("MvM_UpgradesDone");
                        serverGameClients->ClientCommandKeyValues((*it).Get()->GetNetworkable()->GetEdict(), kv);
                    }
                    it = in_upgrade_zone.erase(it);
                }
                else {
                    it++;
                }
            }

            ForEachTFPlayer([&](CTFPlayer *player){
                if (player->IsBot()) return;

                if (player->m_Shared->m_bInUpgradeZone && !upgrades.empty() && in_upgrade_zone.count(player) == 0) {
                    in_upgrade_zone.insert(player);
                    auto kv = new KeyValues("MvM_UpgradesBegin");
                    serverGameClients->ClientCommandKeyValues(player->GetNetworkable()->GetEdict(), kv);
                }

                if (player->m_Shared->m_bInUpgradeZone && !upgrades.empty() && select_type_menus.find(player) == select_type_menus.end()) {
                    bool found = false;
                    bool found_any = false;
                    for (loadout_positions_t slot : {
                        LOADOUT_POSITION_PRIMARY,
                        LOADOUT_POSITION_SECONDARY,
                        LOADOUT_POSITION_MELEE,
                        LOADOUT_POSITION_BUILDING,
                        LOADOUT_POSITION_PDA,
                        LOADOUT_POSITION_ACTION,
                    }) {
                        CBaseCombatWeapon *item = player->Weapon_GetSlot((int)slot);

                        bool found_now = WeaponHasValidUpgrades(item, player);
                        found_any |= found_now;
                        if (found_now && item == player->GetActiveWeapon()) {
                            
                            if (!WeaponHasValidUpgrades(nullptr, player)) {
                                found = true;
                                StartUpgradeListForPlayer(player, (int)slot, 0);
                            }
                            break;
                        }
                    }

                    if (!found) {
                        if (!found_any) {
                            if (WeaponHasValidUpgrades(nullptr, player))
                                StartUpgradeListForPlayer(player, -1, 0);
                        }
                        else {
                            StartMenuForPlayer(player);
                        }
                    }
                }
                else if (!player->m_Shared->m_bInUpgradeZone && select_type_menus.find(player) != select_type_menus.end()) {
                    StopMenuForPlayer(player);
                }
            });
            
		}

        virtual void OnEnable() override
        {
            
            LoadItemNames();
        }

        virtual void LevelInitPreEntity() override
		{
            

            ClearUpgrades();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			ClearUpgrades();
		}

	};
	CMod s_Mod;

	ConVar cvar_enable("sig_mvm_extended_upgrades", "0", FCVAR_NOTIFY,
		"Mod: enable extended upgrades",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
            
		});
}
