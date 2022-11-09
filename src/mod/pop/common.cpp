#include "mod/pop/common.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "util/iterate.h"
#include "mod/pop/popmgr_extensions.h"
#include "mod/pop/pointtemplate.h"

std::map<int, std::string> g_Itemnames;
std::map<int, std::string> g_Attribnames;

namespace Mod::Pop::Wave_Extensions
{
	void ParseColorsAndPrint(const char *line, float gameTextDelay, int &linenum, CTFPlayer* player = nullptr);
}

ItemListEntry_Similar::ItemListEntry_Similar(const char *name) : m_strName(name)
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

bool AreItemsSimilar(const CEconItemView *item_view, bool compare_by_log_name, const std::string &base_name, const std::string &log_name, const std::string &base_melee_class, const char *classname, int base_defindex)
{
    return (compare_by_log_name 
        && FStrEq(base_name.c_str(), item_view->GetItemDefinition()->GetKeyValues()->GetString("base_item_name"))
        && ((!base_name.empty() && item_view->m_iItemDefinitionIndex != 772 /* Baby Face's Blaster*/) 
        || FStrEq(log_name.c_str(), item_view->GetItemDefinition()->GetKeyValues()->GetString("item_logname"))))
        || ((item_view->m_iItemDefinitionIndex == 212 || item_view->m_iItemDefinitionIndex == 947 || item_view->m_iItemDefinitionIndex == 297) && base_defindex == 30) // Invis
        || ((item_view->m_iItemDefinitionIndex == 737) && base_defindex == 25) // Build PDA
        || (base_melee_class == classname && FStrEq(item_view->GetItemDefinition()->GetKeyValues()->GetString("base_item_name"), "Frying Pan"));
}

ActionResult<CTFBot> CTFBotMoveTo::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
    this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
    
    return ActionResult<CTFBot>::Continue();
}

std::map<CHandle<CBaseEntity>, std::string> change_attributes_delay;

THINK_FUNC_DECL(ChangeAttributes) {
    CTFBot *bot = reinterpret_cast<CTFBot *>(this);

    std::string str = change_attributes_delay[bot];
    auto attribs = bot->GetEventChangeAttributes(str.c_str());
    if (attribs != nullptr)
        bot->OnEventChangeAttributes(attribs);

    change_attributes_delay.erase(bot);
}

ActionResult<CTFBot> CTFBotMoveTo::Update(CTFBot *actor, float dt)
{
    if (!actor->IsAlive())
        return ActionResult<CTFBot>::Done();
        
    const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
    if (threat != nullptr) {
        actor->EquipBestWeaponForThreat(threat);
    }
    
    Vector pos = vec3_origin;
    if (this->m_hTarget != nullptr) {
        pos = this->m_hTarget->WorldSpaceCenter();
    }
    else {
        pos = this->m_TargetPos;
    }

    Vector look = vec3_origin;
    if (this->m_hTargetAim != nullptr) {
        look = this->m_hTargetAim->WorldSpaceCenter();
    }
    else {
        look = this->m_TargetAimPos;
    }

    auto nextbot = actor->MyNextBotPointer();
    
    bool inRange = actor->GetAbsOrigin().DistToSqr(pos) < m_fDistanceSq && !(look != vec3_origin && !actor->GetVisionInterface()->IsLineOfSightClearToEntity(this->m_hTargetAim, &look));
    if (!inRange) {
        if (this->m_ctRecomputePath.IsElapsed()) {
            this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
            
            if (pos != vec3_origin) {
                CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
                this->m_PathFollower.Compute(nextbot, pos, cost_func, 0.0f, true);
            }
        }
    }
    else {
        this->m_PathFollower.Invalidate();
    }
    if (!m_bDone) {

        if (!m_bWaitUntilDone) {
            m_bDone = true;
        }
        else if (m_bKillLook) {
            m_bDone = this->m_hTargetAim == nullptr || !this->m_hTargetAim->IsAlive();
        }
        else if (pos == vec3_origin || TheNavMesh->GetNavArea(pos) == actor->GetLastKnownArea() || inRange) {
            m_bDone = true;
        }

        this->m_ctDoneAction.Start(m_fDuration);
    }
    if (m_bDone && this->m_ctDoneAction.IsElapsed()) {
        if (this->m_strOnDoneAttributes != "") {
            change_attributes_delay[actor] = this->m_strOnDoneAttributes;
            THINK_FUNC_SET(actor, ChangeAttributes, gpGlobals->curtime);
        }
        if (this->m_strName != "") {
            variant_t variant;
            variant.SetString(AllocPooledString(this->m_strName.c_str()));
            actor->FireCustomOutput<"onactiondone">(actor, actor, variant);
        }
        if (this->m_pNext != nullptr) {
            auto nextAction = this->m_pNext;
            this->m_pNext = nullptr;
            return ActionResult<CTFBot>::ChangeTo(nextAction, "Switch to next interrupt action in queue");
        }
        return ActionResult<CTFBot>::Done( "Successfully moved to area" );
    }

    this->m_PathFollower.Update(nextbot);
    
    if (look != vec3_origin) {
        
        bool look_now = !m_bKillLook || m_bAlwaysLook;
        if (m_bKillLook) {
            look_now |= actor->HasAttribute(CTFBot::ATTR_IGNORE_ENEMIES);
            if ((this->m_hTargetAim != nullptr && actor->GetVisionInterface()->IsLineOfSightClearToEntity(this->m_hTargetAim, &look))) {
                look_now = true;
                if (actor->GetBodyInterface()->IsHeadAimingOnTarget()) {
                    actor->EquipBestWeaponForThreat(nullptr);
			        actor->PressFireButton();
                }
            }
        }

        if (look_now) {
            actor->GetBodyInterface()->AimHeadTowards(look, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.2f, NULL, "Aiming at target we need to destroy to progress");
        }
    }

    return ActionResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotMoveTo::OnCommandString(CTFBot *actor, const char *cmd)
{

    if (V_stricmp(cmd, "stop interrupt action") == 0) {
        return EventDesiredResult<CTFBot>::Done("Stopping interrupt action");
    }
    else if (V_strnicmp(cmd, "interrupt_action_queue", strlen("interrupt_action_queue")) == 0) {
        CTFBotMoveTo *action = this;
        while(action->m_pNext != nullptr) {
            action = action->m_pNext;
        }
        action->m_pNext = CreateInterruptAction(actor, cmd);
        return EventDesiredResult<CTFBot>::Sustain("Add to queue");
    }
    else if (V_stricmp(cmd, "clear_interrupt_action_queue") == 0) {
        delete this->m_pNext;
        return EventDesiredResult<CTFBot>::Sustain("Clear queue");
    }
    else if (V_stricmp(cmd, "remove_interrupt_action_queue_name") == 0) {
        CTFBotMoveTo *action = this;
        CCommand command = CCommand();
        command.Tokenize(cmd);
        while(action->m_pNext != nullptr) {
            if (FStrEq(action->m_pNext->GetName(), command[1])) {
                auto actionToDelete = action->m_pNext;
                action->m_pNext = actionToDelete->m_pNext;
                delete actionToDelete;
                break;
            }
            action = action->m_pNext;
        }
        return EventDesiredResult<CTFBot>::Sustain("Delete from queue");
    }
    
    return EventDesiredResult<CTFBot>::Continue();
}

int INPUT_TYPE_COUNT=7;
const char *INPUT_TYPE[] = {
    "Primary",
    "Secondary",
    "Special",
    "Reload",
    "Jump",
    "Crouch",
    "Action"
};

int GetResponseFor(const char *text) {
    if (FStrEq(text,"Medic"))
        return 0;
    else if (FStrEq(text,"Help"))
        return 20;
    else if (FStrEq(text,"Go"))
        return 2;
    else if (FStrEq(text,"Move up"))
        return 3;
    else if (FStrEq(text,"Left"))
        return 4;
    else if (FStrEq(text,"Right"))
        return 5;
    else if (FStrEq(text,"Yes"))
        return 6;
    else if (FStrEq(text,"No"))
        return 7;
    //else if (FStrEq(text,"Taunt"))
    //	return MP_CONCEPT_PLAYER_TAUNT;
    else if (FStrEq(text,"Incoming"))
        return 10;
    else if (FStrEq(text,"Spy"))
        return 11;
    else if (FStrEq(text,"Thanks"))
        return 1;
    else if (FStrEq(text,"Jeers"))
        return 23;
    else if (FStrEq(text,"Battle cry"))
        return 21;
    else if (FStrEq(text,"Cheers"))
        return 22;
    else if (FStrEq(text,"Charge ready"))
        return 17;
    else if (FStrEq(text,"Activate charge"))
        return 16;
    else if (FStrEq(text,"Sentry here"))
        return 15;
    else if (FStrEq(text,"Dispenser here"))
        return 14;
    else if (FStrEq(text,"Teleporter here"))
        return 13;
    else if (FStrEq(text,"Good job"))
        return 27;
    else if (FStrEq(text,"Sentry ahead"))
        return 12;
    else if (FStrEq(text,"Positive"))
        return 24;
    else if (FStrEq(text,"Negative"))
        return 25;
    else if (FStrEq(text,"Nice shot"))
        return 26;
    return -1;
}

void UpdateDelayedAddConds(std::vector<DelayedAddCond> &delayed_addconds)
{
    for (auto it = delayed_addconds.begin(); it != delayed_addconds.end(); ) {
        const auto& info = *it;
        
        if (info.bot == nullptr || !info.bot->IsAlive()) {
            it = delayed_addconds.erase(it);
            continue;
        }
        
        if (gpGlobals->curtime >= info.when && (info.health_below == 0 || info.bot->GetHealth() <= info.health_below)) {
            info.bot->m_Shared->AddCond(info.cond, info.duration);
            
            it = delayed_addconds.erase(it);
            continue;
        }
        
        ++it;
    }
}

THINK_FUNC_DECL(StopTaunt) {
    const char * commandn = "stop_taunt";
    CCommand command = CCommand();
    command.Tokenize(commandn);
    reinterpret_cast<CTFPlayer *>(this)->ClientCommand(command);
}



class PeriodicTaskTaunt : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Name")) {
            auto item_def = GetItemSchema()->GetItemDefinitionByName(subkey->GetString());
            if (item_def != nullptr)
                this->tauntDef = item_def->m_iItemDefIndex;

            tauntName = subkey->GetString();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        if (tauntDef != 0) {
            CEconItemView *view = CEconItemView::Create();
            view->Init(tauntDef, 6, 9999, 0);
            Mod::Pop::PopMgr_Extensions::AddCustomWeaponAttributes(tauntName, view);
            bot->PlayTauntSceneFromItem(view);
            CEconItemView::Destroy(view);
            THINK_FUNC_SET(bot, StopTaunt, gpGlobals->curtime + duration);
        }
        else {
            
            const char * commandn = "taunt";
            CCommand command = CCommand();
            command.Tokenize(commandn);
            bot->ClientCommand(command);
            //bot->Taunt(TAUNT_BASE_WEAPON, 0);
        }
    }

    int tauntDef = 0;
    std::string tauntName;
};

class PeriodicTaskGiveSpell : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Type")) {
            spellType = subkey->GetInt();
            const char *typen =subkey->GetString();
            for (int i = 0; i < SPELL_TYPE_COUNT_ALL; i++) {
                if(FStrEq(typen,SPELL_TYPE[i])){
                    spellType = i;
                }	
            }
        }
        else if (FStrEq(subkey->GetName(), "Charges")) {
            spellCount = subkey->GetInt();
        }
        else if (FStrEq(subkey->GetName(), "Limit")) {
            maxSpells = subkey->GetInt();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        if (maxSpells == 0) {
            maxSpells = spellCount;
        }
        CTFPlayer *ply = bot;
        for (int i = 0; i < MAX_WEAPONS; ++i) {
            CBaseCombatWeapon *weapon = ply->GetWeapon(i);
            if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) continue;
            
            CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
            if (spellType < SPELL_TYPE_COUNT){
                spellbook->m_iSelectedSpellIndex=spellType;
            }
            else{
                if (spellType == 12) //common spell
                    spellbook->m_iSelectedSpellIndex=RandomInt(0,6);
                else if (spellType == 13) //rare spell
                    spellbook->m_iSelectedSpellIndex=RandomInt(7,11);
                else if (spellType == 14) //all spells
                    spellbook->m_iSelectedSpellIndex=RandomInt(0,11);
            }
            spellbook->m_iSpellCharges += spellCount;
            if (spellbook->m_iSpellCharges > maxSpells)
                spellbook->m_iSpellCharges = maxSpells;
            
                
            DevMsg("Weapon %d %s\n",i , weapon -> GetClassname());
            break;
        }
        DevMsg("Spell task executed %d\n", spellType);
    }
    
    int spellType = 0;
    int spellCount = 1;
    int maxSpells = 0;
};

class PeriodicTaskVoiceCommand : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Type")) {
            type = subkey->GetInt();
            const char *typen =subkey->GetString();
            int resp = GetResponseFor(typen);
            if (resp >= 0)
                type = resp;
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        TFGameRules()->VoiceCommand(reinterpret_cast<CBaseMultiplayerPlayer*>(bot), type / 10, type % 10);
    }

    int type = 0;
};

class PeriodicTaskFireWeapon : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Type")) {
            type = subkey->GetInt();
            const char *typen =subkey->GetString();
            for (int i = 0; i < INPUT_TYPE_COUNT; i++) {
                if(FStrEq(typen,INPUT_TYPE[i])){
                    type = i;
                }	
            }
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        switch (type) {
        case 0:
            bot->ReleaseFireButton();
            break;
        case 1:
            bot->ReleaseAltFireButton();
            break;
        case 2:
            bot->ReleaseSpecialFireButton();
            break;
        case 3:
            bot->ReleaseReloadButton();
            break;
        case 4:
            bot->ReleaseJumpButton();
            break;
        case 5:
            bot->ReleaseCrouchButton();
            break;
        case 6:
            bot->UseActionSlotItemReleased();
            break;
        }
        if (duration >= 0){

            CTFBot::AttributeType attrs = bot->m_nBotAttrs;

            // Stop SuppressFire bot attribute from preventing to press buttons
            bot->m_nBotAttrs = CTFBot::ATTR_NONE;

            switch (type) {
            case 0:
                bot->PressFireButton(duration);
                break;
            case 1:
                bot->PressAltFireButton(duration);
                break;
            case 2:
                bot->PressSpecialFireButton(duration);
                break;
            case 3:
                bot->PressReloadButton(duration);
                break;
            case 4:
                bot->PressJumpButton(duration);
                break;
            case 5:
                bot->PressCrouchButton(duration);
                break;
            }

            bot->m_nBotAttrs = attrs;
        }
        if (type == 6) {
            bot->UseActionSlotItemPressed();
        }
    }
    
    int type = 0;
};

class PeriodicTaskChangeAttributes : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Name")) {
            name = subkey->GetString();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        const CTFBot::EventChangeAttributes_t *attrib = bot->GetEventChangeAttributes(name.c_str());
        if (attrib != nullptr){
            DevMsg("Attribute exists %s\n", name.c_str());
            bot->OnEventChangeAttributes(attrib);
        }
        DevMsg("Attribute changed %s\n", name.c_str());
    }

    std::string name;
};

class PeriodicTaskFireInput : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Target") || FStrEq(subkey->GetName(), "Name")) {
            target = subkey->GetString();
        }
        else if (FStrEq(subkey->GetName(), "Action") || FStrEq(subkey->GetName(), "Input")) {
            input = subkey->GetString();
        }
        else if (FStrEq(subkey->GetName(), "Param")) {
            param = subkey->GetString();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        variant_t variant1;
        string_t m_iParameter = AllocPooledString(param.c_str());
            variant1.SetString(m_iParameter);
        CEventQueue &que = g_EventQueue;
        que.AddEvent(STRING(AllocPooledString(target.c_str())),STRING(AllocPooledString(input.c_str())),variant1,0,bot,bot,-1);
        std::string targetname = STRING(bot->GetEntityName());
        int findamp = targetname.find('\1');
        if (findamp != -1){
            que.AddEvent(STRING(AllocPooledString((target+targetname.substr(findamp)).c_str())),STRING(AllocPooledString(input.c_str())),variant1,0,bot,bot,-1);
        }
    }

    std::string target;
    std::string input;
    std::string param;
};

class PeriodicTaskMessage : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Name")) {
            message = subkey->GetString();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        int linenum = 0;
        Mod::Pop::Wave_Extensions::ParseColorsAndPrint(message.c_str(), 0.1f, linenum, nullptr);
    }

    std::string message;
};

class PeriodicTaskWeaponSwitch : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Type")) {
            const char *typen = subkey->GetString();
            if (FStrEq(typen, "Primary"))
                type = CTFBot::WeaponRestriction::PRIMARY_ONLY;
            else if (FStrEq(typen, "Secondary"))
                type = CTFBot::WeaponRestriction::SECONDARY_ONLY;
            else if (FStrEq(typen, "Melee"))
                type = CTFBot::WeaponRestriction::MELEE_ONLY;
            else if (FStrEq(typen, "Building"))
                type = CTFBot::WeaponRestriction::BUILDING_ONLY;
            else if (FStrEq(typen, "PDA"))
                type = CTFBot::WeaponRestriction::PDA_ONLY;
            else
                type = subkey->GetInt();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        bot->m_iWeaponRestrictionFlags = (CTFBot::WeaponRestriction)type;
    }

    int type = 0;
};

CAttributeList *GetAttributeListByItemIndex(CTFPlayer *player, int index)
{
    CAttributeList *list = nullptr;
    if (index == -1) { // Player
        list = player->GetAttributeList();
    }
    if (index == -2) { // Active weapon
        CTFWeaponBase *entity = player->GetActiveTFWeapon();
        if (entity != nullptr)
            list = &entity->GetItem()->GetAttributeList();
    }
    else { // Weapon
        CEconEntity *entity = player->GetEconEntityById(index);
        if (entity != nullptr)
            list = &entity->GetItem()->GetAttributeList();
    }
    return list;
}

class PeriodicTaskAddAttribute : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Item")) {
            itemDefIndex=subkey->GetInt();
            const char *typen = subkey->GetString();
            if (FStrEq(typen, "Player")){
                itemDefIndex = -1;
            }
            if (FStrEq(typen, "Active")){
                itemDefIndex= -2;
            }
            else {
                auto item_def = GetItemSchema()->GetItemDefinitionByName(typen);
                if (item_def != nullptr)
                    itemDefIndex = item_def->m_iItemDefIndex;
            }
        }
        else if (FStrEq(subkey->GetName(), "Name")) {
            attrDef = GetItemSchema()->GetAttributeDefinitionByName(subkey->GetString());
            if (attrDef == nullptr) {
                attrDef = GetItemSchema()->GetAttributeDefinition(subkey->GetInt());
            }
        }
        else if (FStrEq(subkey->GetName(), "Value")) {
            param = subkey->GetString();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        auto list = GetAttributeListByItemIndex(bot, itemDefIndex);

        if (list != nullptr && attrDef != nullptr) {
            list->AddStringAttribute(attrDef, param);
        }
    }

    int itemDefIndex = 0;
    CEconItemAttributeDefinition *attrDef = nullptr;
    std::string param;
};

class PeriodicTaskRemoveAttribute : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Item")) {
            itemDefIndex=subkey->GetInt();
            const char *typen = subkey->GetString();
            if (FStrEq(typen, "Player")){
                itemDefIndex = -1;
            }
            if (FStrEq(typen, "Active")){
                itemDefIndex= -2;
            }
            else {
                auto item_def = GetItemSchema()->GetItemDefinitionByName(typen);
                if (item_def != nullptr)
                    itemDefIndex = item_def->m_iItemDefIndex;
            }
        }
        else if (FStrEq(subkey->GetName(), "Name")) {
            attrDef = GetItemSchema()->GetAttributeDefinitionByName(subkey->GetString());
            if (attrDef == nullptr) {
                attrDef = GetItemSchema()->GetAttributeDefinition(subkey->GetInt());
            }
        }
    }
    
    virtual void Update(CTFBot *bot) override
    {
        auto list = GetAttributeListByItemIndex(bot, itemDefIndex);

        if (list != nullptr && attrDef != nullptr) {
            list->RemoveAttribute(attrDef);
        }
    }

    int itemDefIndex = 0;
    CEconItemAttributeDefinition *attrDef = nullptr;
};

class PeriodicTaskSequence : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Name")) {
            sequence = subkey->GetString();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        bot->PlaySpecificSequence(sequence.c_str());
    }

    std::string sequence;
};

class PeriodicTaskClientCommand : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Name")) {
            command = subkey->GetString();
        }
    }
    
    virtual void Update(CTFBot *bot) override
    {
        const char * commandn = command.c_str();
        CCommand command;
        command.Tokenize(commandn);
        bot->ClientCommand(command);
    }

    std::string command;
};

class PeriodicTaskInterruptAction : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Target")) {
            target = subkey->GetString();
        }
        else if (FStrEq(subkey->GetName(), "AimTarget")) {
            aimTarget = subkey->GetString();
        }
        else if (FStrEq(subkey->GetName(), "KillAimTarget")) {
            killAimTarget = subkey->GetBool();
        }
        else if (FStrEq(subkey->GetName(), "WaitUntilDone")) {
            waitUntilDone = subkey->GetFloat();
        }
        else if (FStrEq(subkey->GetName(), "AlwaysLook")) {
            alwaysLook = subkey->GetBool();
        }
        else if (FStrEq(subkey->GetName(), "OnDoneChangeAttributes")) {
            onDoneChangeAttributes = subkey->GetString();
        }
        else if (FStrEq(subkey->GetName(), "Name")) {
            name = subkey->GetString();
        }
        else if (FStrEq(subkey->GetName(), "AddToQueue")) {
            addToQueue = subkey->GetBool();
        }
        else if (FStrEq(subkey->GetName(), "StopCurrentInterruptAction")) {
            stopCurrentInterruptAction = subkey->GetBool();
        }
        else if (FStrEq(subkey->GetName(), "Distance")) {
            distance = subkey->GetFloat();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        if (stopCurrentInterruptAction) {
            bot->MyNextBotPointer()->OnCommandString("stop interrupt action");
        }
        std::string command = addToQueue ? "interrupt_action_queue" : "interrupt_action";

        if (!target.empty()) {
            float posx, posy, posz;

            if (sscanf(target.c_str(),"%f %f %f", &posx, &posy, &posz) == 3) {
                command += CFmtStr(" -pos %f %f %f", posx, posy, posz);
            }
            else {
                command += CFmtStr(" -posent %s", target.c_str());
            }
        }
        if (!aimTarget.empty()) {
            float posx, posy, posz;

            if (sscanf(aimTarget.c_str(),"%f %f %f", &posx, &posy, &posz) == 3) {
                command += CFmtStr(" -lookpos %f %f %f", posx, posy, posz);
            }
            else {
                command += CFmtStr(" -lookposent %s", aimTarget.c_str());
            }
        }
        
        if (!onDoneChangeAttributes.empty()) {
            command += " -ondoneattributes " + onDoneChangeAttributes;
        }
        
        if (!name.empty()) {
            command += " -name " + name;
        }

        command += CFmtStr(" -duration %f", duration);

        if (waitUntilDone != 0)
            command += " -waituntildone";

        if (killAimTarget)
            command += " -killlook";

        command += CFmtStr(" -distance %f", distance);
        
        bot->MyNextBotPointer()->OnCommandString(command.c_str());
    }

    std::string target;
    std::string aimTarget;
    bool killAimTarget = false;
    bool alwaysLook = false;
    float waitUntilDone = 0.0f;
    std::string onDoneChangeAttributes;
    std::string name;
    bool addToQueue = false;
    bool stopCurrentInterruptAction = false;
    float distance = 0.0f;
};

class PeriodicTaskSpray : public PeriodicTask
{
    virtual void Parse(KeyValues *subkey) override 
    {
        if (FStrEq(subkey->GetName(), "Target")) {
            target = subkey->GetString();
        }
        else if (FStrEq(subkey->GetName(), "TargetAngle")) {
            targetAngle = subkey->GetString();
        }
    }

    virtual void Update(CTFBot *bot) override
    {
        Vector forward;
        trace_t	tr;	

        Vector origin;
        QAngle angles;
        if (sscanf(target.c_str(),"%f %f %f", &origin.x, &origin.y, &origin.z) != 3) {
            origin = bot->WorldSpaceCenter() + Vector (0 , 0 , 32);
        }
        if (sscanf(targetAngle.c_str(),"%f %f %f", &angles.x, &angles.y, &angles.z) != 3) {
            angles = bot->EyeAngles();
        }
        bot->EmitSound("SprayCan.Paint");
        AngleVectors(angles, &forward );
        UTIL_TraceLine(origin, origin + forward * 1280, 
            MASK_SOLID_BRUSHONLY, bot, COLLISION_GROUP_NONE, &tr);

        DevMsg("Spraying %f %f %f %f %f %d %d\n", origin.x, angles.x, tr.endpos.x, tr.endpos.y, tr.endpos.z, tr.DidHit(), ENTINDEX(tr.m_pEnt) == 0);
        CBroadcastRecipientFilter filter;

        //ForEachTFPlayer([&](CTFPlayer *player){
        //    if (player->GetTeamNumber() == TF_TEAM_RED)
                TE_PlayerDecal(filter, 0.0f, &tr.endpos, ENTINDEX(bot), ENTINDEX(tr.m_pEnt));
        //});
    }

    std::string target;
    std::string targetAngle;
};

void UpdatePeriodicTasks(std::vector<PeriodicTaskImpl> &pending_periodic_tasks, bool insideECAttr)
{
    for (auto it = pending_periodic_tasks.begin(); it != pending_periodic_tasks.end(); ) {
        auto& pending_task_impl = *it;
        auto& pending_task = *pending_task_impl.task;
        auto pending_task_ptr = pending_task_impl.task.get();

        CTFBot *bot = pending_task_impl.bot;
        if (bot == nullptr || !bot->IsAlive()) {
            it = pending_periodic_tasks.erase(it);
            continue;
        }

        if (pending_task.health_below != 0 
            && bot->GetHealth() >= pending_task.health_below) {
            pending_task_impl.nextTaskTime = gpGlobals->curtime + pending_task.delay;
        }

        if (gpGlobals->curtime >= pending_task_impl.nextTaskTime && (pending_task.health_below == 0 || bot->GetHealth() <= pending_task.health_below)) {
            const CKnownEntity *threat;
            if ((pending_task.health_above > 0 && bot->GetHealth() <= pending_task.health_above) || (
                    pending_task.if_target && ((threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(true)) == nullptr || threat->GetEntity() == nullptr ))) {
                if (pending_task.health_below > 0)
                    pending_task_impl.nextTaskTime = gpGlobals->curtime;

                pending_task_impl.nextTaskTime+=pending_task.cooldown;
                continue;
            }

            size_t sizePre = pending_periodic_tasks.size();
            size_t posPre = it - pending_periodic_tasks.begin();
            pending_task.Update(bot);

            // Changing attributes might clear the task table
            if (!bot->IsAlive() || sizePre != pending_periodic_tasks.size() || pending_task_ptr != pending_periodic_tasks[posPre].task.get()) {
                it = pending_periodic_tasks.begin();
                continue;
            }

            //info.Execute(pending_task.bot);
            DevMsg("Periodic task executed %f, %f\n", pending_task.delay,gpGlobals->curtime);
            if (--pending_task_impl.repeatsLeft == 0) {
                it =  pending_periodic_tasks.erase(it);
                continue;
            }
            else{
                pending_task_impl.nextTaskTime+=pending_task.cooldown;

            }
        }
        
        ++it;
    }
}

void Parse_AddCond(std::vector<AddCond> &addconds, KeyValues *kv)
{
    AddCond addcond;
    
    bool got_cond     = false;
    bool got_duration = false;
    bool got_delay    = false;
    
    FOR_EACH_SUBKEY(kv, subkey) {
        const char *name = subkey->GetName();
        
        if (FStrEq(name, "Index")) {
            addcond.cond = (ETFCond)subkey->GetInt();
            got_cond = true;
        } else if (FStrEq(name, "Name")) {
            ETFCond cond = GetTFConditionFromName(subkey->GetString());
            if (cond != -1) {
                addcond.cond = cond;
                got_cond = true;
            } else {
                Warning("Unrecognized condition name \"%s\" in AddCond block.\n", subkey->GetString());
            }
            
        } else if (FStrEq(name, "Duration")) {
            addcond.duration = subkey->GetFloat();
            got_duration = true;
        } else if (FStrEq(name, "Delay")) {
            addcond.delay = subkey->GetFloat();
            got_delay = true;
        } else if (FStrEq(name, "IfHealthBelow")) {
            addcond.health_below = subkey->GetInt();
        } else if (FStrEq(name, "IfHealthAbove")) {
            addcond.health_above = subkey->GetInt();
        }
            else {
            Warning("Unknown key \'%s\' in AddCond block.\n", name);
        }
    }
    
    if (!got_cond) {
        Warning("Could not find a valid condition index/name in AddCond block.\n");
        return;
    }
    
    DevMsg("CTFBotSpawner %08x: add AddCond(%d, %f)\n", (uintptr_t)&addconds, addcond.cond, addcond.duration);
    addconds.push_back(addcond);
}

bool Parse_PeriodicTask(std::vector<std::shared_ptr<PeriodicTask>> &periodic_tasks, KeyValues *kv, const char *type_name)
{
    std::shared_ptr<PeriodicTask> taskPtr;
    if (FStrEq(type_name, "Taunt")) {
        taskPtr = std::make_shared<PeriodicTaskTaunt>();
    } else if (FStrEq(type_name, "Spell")) {
        taskPtr = std::make_shared<PeriodicTaskGiveSpell>();
    } else if (FStrEq(type_name, "VoiceCommand")) {
        taskPtr = std::make_shared<PeriodicTaskVoiceCommand>();
    } else if (FStrEq(type_name, "FireWeapon")) {
        taskPtr = std::make_shared<PeriodicTaskFireWeapon>();
    } else if (FStrEq(type_name, "ChangeAttributes")) {
        taskPtr = std::make_shared<PeriodicTaskChangeAttributes>();
    } else if (FStrEq(type_name, "FireInput")) {
        taskPtr = std::make_shared<PeriodicTaskFireInput>();
    } else if (FStrEq(type_name, "Message")) {
        taskPtr = std::make_shared<PeriodicTaskMessage>();
    } else if (FStrEq(type_name, "WeaponSwitch")) {
        taskPtr = std::make_shared<PeriodicTaskWeaponSwitch>();
    } else if (FStrEq(type_name, "AddAttribute")) {
        taskPtr = std::make_shared<PeriodicTaskAddAttribute>();
    } else if (FStrEq(type_name, "RemoveAttribute")) {
        taskPtr = std::make_shared<PeriodicTaskRemoveAttribute>();
    } else if (FStrEq(type_name, "Sequence")) {
        taskPtr = std::make_shared<PeriodicTaskSequence>();
    } else if (FStrEq(type_name, "ClientCommand")) {
        taskPtr = std::make_shared<PeriodicTaskClientCommand>();
    } else if (FStrEq(type_name, "InterruptAction")) {
        taskPtr = std::make_shared<PeriodicTaskInterruptAction>();
    } else if (FStrEq(type_name, "Spray")) {
        taskPtr = std::make_shared<PeriodicTaskSpray>();
    } else {
        return false;
    }

    const char *name;
    PeriodicTask &task = *taskPtr;
    FOR_EACH_SUBKEY(kv, subkey) {
        const char *name = subkey->GetName();
        
        if (FStrEq(name, "Cooldown")) {
            task.cooldown = Max(0.015f,subkey->GetFloat());
        } else if (FStrEq(name, "Repeats")) {
            task.repeats = subkey->GetInt();
        } else if (FStrEq(name, "Delay")) {
            task.delay = subkey->GetFloat();
        }
        else if (FStrEq(name, "Duration")) {
            task.duration=subkey->GetFloat();
        }
        else if (FStrEq(name, "IfSeeTarget")) {
            task.if_target=subkey->GetBool();
        }
        else if (FStrEq(name, "IfHealthBelow")) {
            task.health_below=subkey->GetInt();
        }
        else if (FStrEq(name, "IfHealthAbove")) {
            task.health_above=subkey->GetInt();
        }
        else {
            task.Parse(subkey);
        }
        //else if (FStrEq(name, "SpawnTemplate")) {
        //	spawners[spawner].periodic_templ.push_back(Parse_SpawnTemplate(subkey));
        //	task.spell_type = spawners[spawner].periodic_templ.size()-1;
        //}
    }
    DevMsg("CTFBotSpawner %08x: add periodic(%f, %f)\n", (uintptr_t)&periodic_tasks, task.cooldown, task.delay);
    periodic_tasks.push_back(std::move(taskPtr));
    return true;
}

void ApplyAddCond(CTFBot *bot, std::vector<AddCond> &addconds, std::vector<DelayedAddCond> &delayed_addconds)
{
    for (auto addcond : addconds) {
        if (addcond.delay == 0.0f && addcond.health_below == 0) {
            DevMsg("CTFBotSpawner %08x: applying AddCond(%d, %f)\n", (uintptr_t)bot, addcond.cond, addcond.duration);
            bot->m_Shared->AddCond(addcond.cond, addcond.duration);
        } else {
            delayed_addconds.push_back({
                bot,
                gpGlobals->curtime + addcond.delay,
                addcond.cond,
                addcond.duration,
                addcond.health_below,
                addcond.health_above
            });
        }
    }
}

void ApplyPendingTask(CTFBot *bot, std::vector<std::shared_ptr<PeriodicTask>> &periodic_tasks, std::vector<PeriodicTaskImpl> &pending_periodic_tasks)
{
    for (auto &taskParse : periodic_tasks) {
        PeriodicTaskImpl newTask;
        newTask.bot = bot;
        newTask.nextTaskTime = taskParse->delay + gpGlobals->curtime;
        newTask.task = taskParse;
        newTask.repeatsLeft = taskParse->repeats;
        pending_periodic_tasks.push_back(std::move(newTask));
    }
}
		
bool LoadUserDataFile(CRC32_t &value, const char *filename) {
    if (!CRC_File(&value, filename)) {
        return false;
    }

    char tmpfilename[MAX_PATH];
    char hex[16];
    Q_binarytohex( (byte *)&value, sizeof( value ), hex, sizeof( hex ) );
    Q_snprintf( tmpfilename, sizeof( tmpfilename ), "user_custom/%c%c/%s.dat", hex[0], hex[1], hex );

    DevMsg("spray file %s %s\n", filename, tmpfilename);
    bool copy = true;

    char szAbsFilename[MAX_PATH];
    if (filesystem->RelativePathToFullPath(tmpfilename, "game", szAbsFilename, sizeof(szAbsFilename), FILTER_CULLPACK)) {
        // check if existing file already has same CRC, 
        // then we don't need to copy it anymore
        CRC32_t test;
        CRC_File(&test, szAbsFilename);
        if (test == value)
            copy = false;
    }

    if (copy)
    {
        // Copy it over under the new name
        
        DevMsg("Copying to temp dir\n");
        // Load up the file
        CUtlBuffer buf;
        if (!filesystem->ReadFile(filename, "game", buf))
        {
            DevMsg("Cannot find original file\n");
            return true;
        }

        // Make sure dest directory exists
        char szParentDir[MAX_PATH];
        V_ExtractFilePath(tmpfilename, szParentDir, sizeof(szParentDir));
        filesystem->CreateDirHierarchy(szParentDir, "download");

        // Save it
        if (!filesystem->WriteFile(tmpfilename, "download", buf) )
        {
            DevMsg("Copying to temp dir failed\n");
            return true;
        }
    }
    return true;
}

void GenerateItemNames() {
    KeyValues *kvin = new KeyValues("Lang");
    kvin->UsesEscapeSequences(true);

    CUtlBuffer file( 0, 0, CUtlBuffer::TEXT_BUFFER );
    filesystem->ReadFile("resource/tf_english.txt", "GAME", file);
    
    char buf[4000000];
    _V_UCS2ToUTF8( (const ucs2*) (file.String() + 2), buf, 4000000 );

    if (kvin->LoadFromBuffer("english", buf)/**/) {

        KeyValues *tokens = kvin->FindKey("Tokens");
        std::unordered_map<int, std::string> strings;

        FOR_EACH_SUBKEY(tokens, subkey) {
            strings[subkey->GetNameSymbol()] = subkey->GetString();
        }

        for (int i = 0; i < 40000; i++)
        {
            CEconItemDefinition *def = GetItemSchema()->GetItemDefinition(i);
            if (def != nullptr && !FStrEq(def->GetItemName(""), "#TF_Default_ItemDef") && strncmp(def->GetItemClass(), "tf_", 3) == 0) {
                const char *item_slot = def->GetKeyValues()->GetString("item_slot", nullptr);
                if (item_slot != nullptr && !FStrEq(item_slot, "misc") && !FStrEq(item_slot, "hat") && !FStrEq(item_slot, "head")) {
                    std::string name = strings[KeyValues::CallGetSymbolForString(def->GetItemName("#")+1, false)];
                    g_Itemnames[i] = name;
                }
            }
        }
        for (int i = 0; i < 4000; i++)
        {
            auto def = GetItemSchema()->GetAttributeDefinition(i);
            if (def != nullptr) {
                const char *str = def->GetKeyValues()->GetString("description_string", "#")+1;
                if (str[0] != '\0')
                    g_Attribnames[i] = strings[KeyValues::CallGetSymbolForString(str, false)];
            }
        }
       // timer3.End();
        //Msg("Def time %.9f\n", timer3.GetDuration().GetSeconds());

        char path_sm[PLATFORM_MAX_PATH];
        g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/sig_item_data.dat");
        CUtlBuffer fileout( 0, 0, 0 );
        fileout.PutInt64(filesystem->GetFileTime("resource/tf_english.txt", "GAME"));

        fileout.PutInt(g_Itemnames.size());
        fileout.PutInt(g_Attribnames.size());
        for (auto &entry : g_Itemnames) {
            fileout.PutInt(entry.first);
            fileout.PutString(entry.second.c_str());
        }
        
        for (auto &entry : g_Attribnames) {
            fileout.PutUnsignedShort(entry.first);
            fileout.PutString(entry.second.c_str());
        }

        filesystem->WriteFile(path_sm, "GAME", fileout);
        
    }
    kvin->deleteThis();
}

void LoadItemNames() {
    if (g_Itemnames.empty() || g_Attribnames.empty()) {
        char path_sm[PLATFORM_MAX_PATH];
        g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/sig_item_data.dat");

        long time = filesystem->GetFileTime("resource/tf_english.txt", "GAME");
        CUtlBuffer file( 0, 0, 0 );

        if (filesystem->ReadFile(path_sm, "GAME", file)) {
            int64 timewrite = file.GetInt64();
            if (timewrite != time) {
                Msg("diff time\n");
                GenerateItemNames();
                return;
            }
            int num_itemnames = file.GetInt();
            int num_attrnames = file.GetInt();
            char buf[256];
            for (int i = 0; i < num_itemnames; i++) {
                int id = file.GetInt();
                file.GetString<256>(buf);
                g_Itemnames[id] = buf;
            }

            for (int i = 0; i < num_attrnames; i++) {
                int id = file.GetUnsignedShort();
                file.GetString<256>(buf);
                g_Attribnames[id] = buf;
            }
        }
        else {
            GenerateItemNames();
            return;
        }
    }
}

bool FormatAttributeString(std::string &string, CEconItemAttributeDefinition *attr_def, attribute_data_union_t value) {
    DevMsg("inspecting attr\n");
    if (attr_def == nullptr)
        return false;
    
    DevMsg("inspecting attr index %d\n", attr_def->GetIndex());
    KeyValues *kv = attr_def->GetKeyValues();
    const char *format = kv->GetString("description_string");
    if (kv->GetBool("hidden") || format == nullptr)
        return false;

    
	char val_buf[256];

    if (attr_def->GetIndex() < 4000) {
        if (format[0] != '#')
            return false;
        
        string = g_Attribnames[attr_def->GetIndex()];
        int val_pos = string.find("%s1");
        if (val_pos != -1) {
            const char *desc_format = kv->GetString("description_format");
            bool is_percentage = FStrEq(desc_format, "value_is_percentage");
            bool is_additive = FStrEq(desc_format, "value_is_additive");
            bool is_additive_percentage = FStrEq(desc_format, "value_is_additive_percentage");
            bool is_inverted_percentage = FStrEq(desc_format, "value_is_inverted_percentage");

            float float_value = value.m_Float;

            if (attr_def->IsType<CSchemaAttributeType_String>()) {
                const char *pstr = "";
                if (value.m_String != nullptr) {
                    CopyStringAttributeValueToCharPointerOutput(value.m_String, &pstr);
                }
                V_strncpy(val_buf, pstr, sizeof(val_buf));
            }
            else {
                if (!is_percentage && !is_additive && !is_additive_percentage && !is_inverted_percentage)
                    return false;
                    
                if (attr_def->IsStoredAsInteger()) {
                    float_value = RoundFloatToInt(value.m_Float);
                }
                if (!is_additive) {
                    if (is_inverted_percentage) {
                        float_value -= 1.0f;
                        float_value = -float_value;
                    }
                    else if (!is_additive_percentage) {
                        float_value -= 1.0f;
                    }
                    
                }
                int display_value = RoundFloatToInt(float_value * 100.0f);
                if (!is_additive) {
                    snprintf(val_buf, sizeof(val_buf), "%d", display_value);
                }
                else {
                    if (display_value % 100 == 0) {
                        snprintf(val_buf, sizeof(val_buf), "%d", display_value/100);
                    }
                    else {
                        snprintf(val_buf, sizeof(val_buf), "%d.%.2g", display_value/100, (float) (abs(display_value) % 100) / 100.0f);
                    }
                }
                string.replace(val_pos, 3, val_buf);
            }
        }
    }
    else {

        string = format;
        bool is_percentage = false;
        int val_pos = string.find("%d");
        if (val_pos == -1) {
            val_pos = string.find("%p");
            is_percentage = true;
        }

        if (val_pos != -1) {
            
            const char *desc_format = kv->GetString("description_format");
            bool is_additive = FStrEq(desc_format, "value_is_additive");
            bool is_inverted_percentage = FStrEq(desc_format, "value_is_inverted_percentage");

            float float_value = value.m_Float;


            if (attr_def->IsType<CSchemaAttributeType_String>()) {
                const char *pstr = "";
                if (value.m_String != nullptr) {
                    CopyStringAttributeValueToCharPointerOutput(value.m_String, &pstr);
                }
                V_strncpy(val_buf, pstr, sizeof(val_buf));
            }
            else {
                if (attr_def->IsStoredAsInteger()) {
                    float_value = RoundFloatToInt(value.m_Float);
                }
                if (is_percentage) {
                    if (is_inverted_percentage) {
                        float_value -= 1.0f;
                        float_value = -float_value;
                    }
                    else if (!is_additive) {
                        float_value -= 1.0f;
                    }
                }
                int display_value = RoundFloatToInt(float_value * 100.0f);
                if (is_percentage) {
                    snprintf(val_buf, sizeof(val_buf), "%d", display_value);
                }
                else {
                    if (display_value % 100 == 0) {
                        snprintf(val_buf, sizeof(val_buf), "%d", display_value/100);
                    }
                    else {
                        snprintf(val_buf, sizeof(val_buf), "%d.%.2g", display_value/100, (float) (abs(display_value) % 100) / 100.0f);
                    }
                }
            }

            string.replace(val_pos, 2, val_buf);

            int sign_pos = string.find("(+-)");
            if (sign_pos != -1) {
                if (float_value > 0)
                    string.replace(sign_pos, 4, "+");
                else
                    string.replace(sign_pos, 4, "");
            }
        }
    }

    return true;
}
const char *GetItemName(const CEconItemView *view) {
    bool val;
    return GetItemName(view, val);
}

const char *GetItemName(const CEconItemView *view, bool &is_custom) {
    static int custom_weapon_def = -1;
    if (custom_weapon_def == -1) {
        auto attr = GetItemSchema()->GetAttributeDefinitionByName("custom weapon name");
        if (attr != nullptr)
            custom_weapon_def = attr->GetIndex();
    }
        
    auto attr = view->GetAttributeList().GetAttributeByID(custom_weapon_def);
    const char *value = nullptr;
    if (attr != nullptr && attr->GetValuePtr()->m_String != nullptr) {
        CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &value);
        is_custom = true;
    }
    else {
        value = view->GetStaticData()->GetName("");
        is_custom = false;
    }
    return value;
}

const char *GetItemNameForDisplay(const CEconItemView *view) {
    static int custom_weapon_def = -1;
    if (custom_weapon_def == -1) {
        auto attr = GetItemSchema()->GetAttributeDefinitionByName("custom weapon name");
        if (attr != nullptr)
            custom_weapon_def = attr->GetIndex();
    }
        
    auto attr = view->GetAttributeList().GetAttributeByID(custom_weapon_def);
    const char *value = nullptr;
    if (attr != nullptr && attr->GetValuePtr()->m_String != nullptr) {
        CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &value);
    }
    // Also check custom item name from name tag
    else if((attr = view->GetAttributeList().GetAttributeByID(500 /*custom name attr*/)) != nullptr) {
        CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &value);
        std::string buf = "''"s + value + "''"s; 
        return STRING(AllocPooledString(buf.c_str()));
    }
    else {
        value = GetItemNameForDisplay(view->GetItemDefIndex());
    }
    return value;
}

const char *GetItemNameForDisplay(int item_defid) {
    auto find = g_Itemnames.find(item_defid);
    if (find != g_Itemnames.end()) {
        return find->second.c_str();
    }
    else {
        auto item_def = GetItemSchema()->GetItemDefinition(item_defid);
        if (item_def != nullptr) {
            return item_def->GetName();
        }
        return nullptr;
    }
}

void ApplyForceItemsClass(std::vector<ForceItem> &items, CTFPlayer *player, bool no_remove, bool respect_class, bool mark)
{
    for (auto &pair : items) {
        if (pair.entry != nullptr) {
            bool found = false;
            ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                if (entity->GetItem() != nullptr && pair.entry->Matches(entity->GetItem()->GetItemDefinition()->GetItemClass(), entity->GetItem())) {
                    found = true;
                    return false;
                }
                return true;
            });
            if (!found)
                return;
        }
        CEconEntity *entity = GiveItemByName(player, pair.name.c_str(), no_remove, respect_class);
        
        if (entity != nullptr) {
            // 1 - ForceItem from Wave, can replace extra loadout item
            // 2 - ForceItem from WaveSchedule, cannot replace extra loadout item
            entity->GetItem()->GetAttributeList().SetRuntimeAttributeValue(GetItemSchema()->GetAttributeDefinitionByName("is force item"), mark ? 1.0f : 2.0f);
        }
    }
}

CBaseAnimating * TemplateShootSpawn(std::vector<ShootTemplateData> &templates, CTFPlayer *player, CTFWeaponBase *weapon, bool &stopproj, std::function<CBaseAnimating *()> origShootFunc)
{
    stopproj = false;
    for(auto it = templates.begin(); it != templates.end(); it++) {
        ShootTemplateData &temp_data = *it;
        if (temp_data.weapon_classname != "" && !FStrEq(weapon->GetClassname(), temp_data.weapon_classname.c_str()))
            continue;

        if (temp_data.weapon != "" && !FStrEq(GetItemName(weapon->GetItem()), temp_data.weapon.c_str()))
            continue;

            // bool name_correct = FStrEq(weapon->GetItem()->GetStaticData()->GetName(), temp_data.weapon.c_str());

            // if (!name_correct) {
            // 	static int custom_weapon_def = -1;
            // 	if (custom_weapon_def == -1) {
            // 		auto attr = GetItemSchema()->GetAttributeDefinitionByName("custom weapon name");
            // 		if (attr != nullptr)
            // 			custom_weapon_def = attr->GetIndex();
            // 	}
            // 	auto attr = weapon->GetItem()->GetAttributeList().GetAttributeByID(custom_weapon_def);
            // 	const char *value = nullptr;
            // 	if (attr != nullptr && attr->GetValuePtr()->m_String != nullptr) {
            // 		CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &value);
            // 	}
            // 	if (value == nullptr || strcmp(value, temp_data.weapon.c_str()) != 0) {
            // 		continue;
            // 	}
            // }
        //}

        if (temp_data.parent_to_projectile) {
            CBaseAnimating *proj = origShootFunc();
            if (proj != nullptr) {
                Vector vec = temp_data.offset;
                QAngle ang = temp_data.angles;
                auto inst = temp_data.templ->SpawnTemplate(proj, vec, ang, true, nullptr, false, temp_data.parameters);
            }
            
            return proj;
        }
        
        stopproj = temp_data.Shoot(player, weapon) | stopproj;
    }
    return nullptr;
}

void GenerateReferences()
{
    KeyValues *kvin = new KeyValues("Lang");
    kvin->UsesEscapeSequences(true);

    CUtlBuffer file( 0, 0, CUtlBuffer::TEXT_BUFFER );
    filesystem->ReadFile("resource/tf_english.txt", "GAME", file);
    
    char buf[4000000];
    _V_UCS2ToUTF8( (const ucs2*) (file.String() + 2), buf, 4000000 );

    std::map<std::string, std::string> itemname_global;
    if (kvin->LoadFromBuffer("english", buf)/**/) {

        KeyValues *tokens = kvin->FindKey("Tokens");
        std::unordered_map<int, std::string> strings;

        FOR_EACH_SUBKEY(tokens, subkey) {
            strings[subkey->GetNameSymbol()] = subkey->GetString();
        }

        for (int i = 0; i < 40000; i++)
        {
            CEconItemDefinition *def = GetItemSchema()->GetItemDefinition(i);
            if (def != nullptr && !FStrEq(def->GetItemName(""), "#TF_Default_ItemDef")) {
                itemname_global[def->GetName("")] = strings[KeyValues::CallGetSymbolForString(def->GetItemName("#")+1, false)];
            }
        }
        /*for (int i = 0; i < 4000; i++)
        {
            auto def = GetItemSchema()->GetAttributeDefinition(i);
            if (def != nullptr) {
                const char *str = def->GetKeyValues()->GetString("description_string", "#")+1;
                if (str[0] != '\0')
                    g_Attribnames[i] = strings[KeyValues::CallGetSymbolForString(str, false)];
            }
        }*/

        CUtlBuffer fileouttotal( 0, 0, CUtlBuffer::TEXT_BUFFER );
        fileouttotal.PutString("English name -> Definition name\n");
        for (auto &entry : itemname_global) {
            fileouttotal.PutString(entry.second.c_str());
            fileouttotal.PutString(" -> \"");
            fileouttotal.PutString(entry.first.c_str());
            fileouttotal.PutString("\"\n");
        }
        filesystem->WriteFile("item_names_total.txt", "GAME", fileouttotal);
       // timer3.End();
        //Msg("Def time %.9f\n", timer3.GetDuration().GetSeconds());
    }
    kvin->deleteThis();
}
ConCommand ccmd_generate_references("sig_generate_references", &GenerateReferences, "Generate references for later use");


CBaseEntity *SelectTargetByName(CTFBot *actor, const char *name)
{
    CBaseEntity *target = servertools->FindEntityByName(nullptr, name, actor);
    if (target == nullptr && FStrEq(name,"RandomEnemy")) {
        target = actor->SelectRandomReachableEnemy();
    }
    else if (target == nullptr && FStrEq(name, "ClosestPlayer")) {
        float closest_dist = FLT_MAX;
        ForEachTFPlayer([&](CTFPlayer *player){
            if (player->IsAlive() && !player->IsBot()) {
                float dist = player->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
                if (dist < closest_dist) {
                    closest_dist = dist;
                    target = player;
                }
            }
        });
    }
    else if (target == nullptr) {
        ForEachTFBot([&](CTFBot *bot) {
            if (bot->IsAlive() && FStrEq(bot->GetPlayerName(), name)) {
                target = bot; 
            }
        });
    }
    if (target == nullptr) {
        float closest_dist = FLT_MAX;
        ForEachEntityByClassname(name, [&](CBaseEntity *entity) {
            float dist = entity->GetAbsOrigin().DistToSqr(actor->GetAbsOrigin());
            if (dist < closest_dist) {
                closest_dist = dist;
                target = entity;
            }
        });
    }
    return target;

}

CTFBotMoveTo *CreateInterruptAction(CTFBot *actor, const char *cmd) {
    CCommand command = CCommand();
    command.Tokenize(cmd);
    
    const char *other_target = "";

    auto interrupt_action = new CTFBotMoveTo();
    for (int i = 1; i < command.ArgC(); i++) {
        if (strcmp(command[i], "-name") == 0) {
            interrupt_action->SetName(command[i+1]);
            i++;
        }
        else if (strcmp(command[i], "-pos") == 0) {
            Vector pos;
            pos.x = strtof(command[i+1], nullptr);
            pos.y = strtof(command[i+2], nullptr);
            pos.z = strtof(command[i+3], nullptr);

            interrupt_action->SetTargetPos(pos);
            i += 3;
        }
        else if (strcmp(command[i], "-lookpos") == 0) {
            Vector pos;
            pos.x = strtof(command[i+1], nullptr);
            pos.y = strtof(command[i+2], nullptr);
            pos.z = strtof(command[i+3], nullptr);

            interrupt_action->SetTargetAimPos(pos);
            i += 3;
        }
        else if (strcmp(command[i], "-posent") == 0) {
            if (strcmp(other_target, command[i+1]) == 0) {
                interrupt_action->SetTargetPosEntity(interrupt_action->GetTargetAimPosEntity());
            }
            else {
                CBaseEntity *target = SelectTargetByName(actor, command[i+1]);
                other_target = command[i+1];
                interrupt_action->SetTargetPosEntity(target);
            }
            i++;
        }
        else if (strcmp(command[i], "-lookposent") == 0) {
            if (strcmp(other_target, command[i+1]) == 0) {
                interrupt_action->SetTargetAimPosEntity(interrupt_action->GetTargetPosEntity());
            }
            else {
                CBaseEntity *target = SelectTargetByName(actor, command[i+1]);
                other_target = command[i+1];
                interrupt_action->SetTargetAimPosEntity(target);
            }
            i++;
        }
        else if (strcmp(command[i], "-duration") == 0) {
            interrupt_action->SetDuration(strtof(command[i+1], nullptr));
            i++;
        }
        else if (strcmp(command[i], "-waituntildone") == 0) {
            interrupt_action->SetWaitUntilDone(true);
        }
        else if (strcmp(command[i], "-killlook") == 0) {
            interrupt_action->SetKillLook(true);
        }
        else if (strcmp(command[i], "-alwayslook") == 0) {
            interrupt_action->SetAlwaysLook(true);
        }
        else if (strcmp(command[i], "-ondoneattributes") == 0) {
            interrupt_action->SetOnDoneAttributes(command[i+1]);
            i++;
        }
        else if (strcmp(command[i], "-distance") == 0) {
            interrupt_action->SetMaxDistance(strtof(command[i+1], nullptr));
            i++;
        }
    }	
    return interrupt_action;
}