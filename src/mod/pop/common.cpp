#include "mod/pop/common.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "util/iterate.h"
#include "mod/pop/popmgr_extensions.h"
#include "mod/pop/pointtemplate.h"


namespace Mod::Pop::Wave_Extensions
{
	void ParseColorsAndPrint(const char *line, float gameTextDelay, int &linenum, CTFPlayer* player = nullptr);
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

bool IsInNavSpawnArea(CTFPlayer *player) {
    CTFNavArea *area = static_cast<CTFNavArea *>(player->GetLastKnownArea());
    if (area != nullptr) {
        return area->HasTFAttributes(player->GetTeamNumber() == TF_TEAM_RED ? RED_SPAWN_ROOM : BLUE_SPAWN_ROOM);
    }
    return false;
}

void UpdateDelayedAddConds(std::vector<DelayedAddCond> &delayed_addconds)
{
    for (auto it = delayed_addconds.begin(); it != delayed_addconds.end(); ) {
        const auto& info = *it;
        
        if (info.bot == nullptr || !info.bot->IsAlive()) {
            it = delayed_addconds.erase(it);
            continue;
        }
        
        if (gpGlobals->curtime >= info.when && (info.health_below == 0 || info.bot->GetHealth() <= info.health_below) && !(info.if_left_spawn && IsInNavSpawnArea(info.bot))) {
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
                    pending_task.if_target && ((threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(true)) == nullptr || threat->GetEntity() == nullptr ))
                    || (pending_task.if_left_spawn && IsInNavSpawnArea(bot))
                    || (pending_task.if_no_target && ((threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(true)) != nullptr && threat->GetEntity() != nullptr ))
                    || (pending_task.if_range_target_min > 0 && ((threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(true)) == nullptr || threat->GetEntity() == nullptr || threat->GetEntity()->GetAbsOrigin().DistTo(bot->GetAbsOrigin()) < pending_task.if_range_target_min ))
                    || (pending_task.if_range_target_max != -1 && ((threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(true)) == nullptr || threat->GetEntity() == nullptr || threat->GetEntity()->GetAbsOrigin().DistTo(bot->GetAbsOrigin()) > pending_task.if_range_target_max))) {
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

    if (kv->GetFirstSubKey() == nullptr) {
        if (kv->GetDataType() == KeyValues::TYPE_INT) {
            addcond.cond = (ETFCond)kv->GetInt();
            got_cond = true;
        }
        else {
            addcond.cond = GetTFConditionFromName(kv->GetString());
            if (addcond.cond != -1) {
                got_cond = true;
            } else {
                Warning("Unrecognized condition name \"%s\" in AddCond block.\n", kv->GetString());
            }
        }
    }
    
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
        } else if (FStrEq(name, "IfLeftSpawn")) {
            addcond.if_left_spawn = subkey->GetBool();
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
        else if (FStrEq(name, "IfNoTarget")) {
            task.if_no_target=subkey->GetBool();
        }
        else if (FStrEq(name, "MinTargetRange")) {
            task.if_range_target_min=subkey->GetFloat();
        }
        else if (FStrEq(name, "MaxTargetRange")) {
            task.if_range_target_max=subkey->GetFloat();
        }
        else if (FStrEq(name, "IfHealthBelow")) {
            task.health_below=subkey->GetInt();
        }
        else if (FStrEq(name, "IfHealthAbove")) {
            task.health_above=subkey->GetInt();
        }
        else if (FStrEq(name, "IfLeftSpawn")) {
            task.if_left_spawn = subkey->GetBool();
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

void Parse_ForceItem(KeyValues *kv, ForceItems &force_items, bool noremove)
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
        int classname = TF_CLASS_COUNT+1;
        for(int i=1; i < TF_CLASS_COUNT; i++){
            if(FStrEq(g_aRawPlayerClassNames[i],subkey->GetName())){
                classname=i;
                break;
            }
        }
        FOR_EACH_SUBKEY(subkey, subkey2) {
            Msg("Class %d\n", classname);
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

void Parse_ItemAttributes(KeyValues *kv, std::vector<ItemAttributes> &attibs)
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
                addcond.health_above,
                addcond.if_left_spawn
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

void ApplyForceItemsClass(std::vector<ForceItem> &items, CTFPlayer *player, bool no_remove, bool no_respect_class, bool mark)
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
        CEconEntity *entity = GiveItemByName(player, pair.name.c_str(), no_remove, no_respect_class);
        
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


void ApplyItemAttributes(CEconItemView *item_view, CTFPlayer *player, std::vector<ItemAttributes> &item_attribs_vec) {

    // Ignore helper custom model wearables
    if (item_view->m_iEntityLevel == 414918) return;
    
    // Item attributes are ignored when picking up dropped weapons
    float dropped_weapon_attr = 0.0f;
    FindAttribute(&item_view->GetAttributeList(), GetItemSchema()->GetAttributeDefinitionByName("is dropped weapon"), &dropped_weapon_attr);

    if (dropped_weapon_attr != 0.0f)
        return;

    // Item attributes are ignored when custom weapon
    float custom_weapon_attr = 0.0f;
    FindAttribute(&item_view->GetAttributeList(), GetItemSchema()->GetAttributeDefinitionByName("custom weapon id"), &custom_weapon_attr);

    if (custom_weapon_attr != 0.0f)
        return;

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