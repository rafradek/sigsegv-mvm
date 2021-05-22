#ifndef _INCLUDE_SIGSEGV_MOD_POP_COMMON_H_
#define _INCLUDE_SIGSEGV_MOD_POP_COMMON_H_

#include "util/autolist.h"
#include "stub/tfbot.h"
#include "stub/strings.h"

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

struct ForceItems
{
	std::vector<std::pair<std::string, CEconItemDefinition *>> items[11] = {};
	std::vector<std::pair<std::string, CEconItemDefinition *>> items_no_remove[11] = {};
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

void UpdateDelayedAddConds(std::vector<DelayedAddCond> &delayed_addconds);
void UpdatePeriodicTasks(std::vector<PeriodicTask> &pending_periodic_tasks);

static void ApplyForceItemsClass(std::vector<std::pair<std::string, CEconItemDefinition *>> &items, CTFPlayer *player, bool no_remove, bool respect_class, bool mark)
{
    for (auto &pair : items) {
        const char *classname = TranslateWeaponEntForClass_improved(pair.second->GetItemClass(), player->GetPlayerClass()->GetClassIndex());
        CEconEntity *entity = static_cast<CEconEntity *>(ItemGeneration()->SpawnItem(pair.second->m_iItemDefIndex, player->WorldSpaceCenter(), vec3_angle, 1, 6, classname));
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
            items[0].push_back({kv->GetString(), item_def});
            DevMsg("Add\n");
        }
    }
    FOR_EACH_SUBKEY(kv, subkey) {
        int classname = 10;
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
                items[classname].push_back({subkey2->GetString(), item_def});
            }
        }
    }
    
    DevMsg("Parsed attributes\n");
}

void Parse_AddCond(std::vector<AddCond> &addconds, KeyValues *kv);
bool Parse_PeriodicTask(std::vector<PeriodicTaskImpl> &periodic_tasks, KeyValues *kv, const char *type_name);
void ApplyPendingTask(CTFBot *bot, std::vector<PeriodicTaskImpl> &periodic_tasks, std::vector<PeriodicTask> &pending_periodic_tasks);
void ApplyAddCond(CTFBot *bot, std::vector<AddCond> &addconds, std::vector<DelayedAddCond> &delayed_addconds);

bool LoadUserDataFile(CRC32_t &value, const char *filename);
#endif
