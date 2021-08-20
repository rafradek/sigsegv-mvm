#include "mod/pop/common.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "util/iterate.h"

std::map<int, std::string> g_Itemnames;
std::map<int, std::string> g_Attribnames;

namespace Mod::Pop::Wave_Extensions
{
	void ParseColorsAndPrint(const char *line, float gameTextDelay, int &linenum, CTFPlayer* player = nullptr);
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
    
    if (this->m_ctRecomputePath.IsElapsed()) {
        this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
        
        if (pos != vec3_origin) {
            CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
            this->m_PathFollower.Compute(nextbot, pos, cost_func, 0.0f, true);
        }
    }
    
    if (!m_bDone) {

        if (!m_bWaitUntilDone) {
            m_bDone = true;
        }
        else if (m_bKillLook) {
            m_bDone = this->m_hTargetAim == nullptr || !this->m_hTargetAim->IsAlive();
        }
        else if (pos == vec3_origin || TheNavMesh->GetNavArea(pos) == actor->GetLastKnownArea()) {
            m_bDone = true;
        }

        this->m_ctDoneAction.Start(m_fDuration);
    }
    if (m_bDone && this->m_ctDoneAction.IsElapsed()) {
        if (this->m_strOnDoneAttributes != "") {
            change_attributes_delay[actor] = this->m_strOnDoneAttributes;
            THINK_FUNC_SET(actor, ChangeAttributes, gpGlobals->curtime);
        }
        return ActionResult<CTFBot>::Done( "Successfully moved to area" );
    }

    this->m_PathFollower.Update(nextbot);
    
    if (look != vec3_origin) {
        
        bool look_now = !m_bKillLook;
        if (m_bKillLook) {
            look_now = actor->HasAttribute(CTFBot::ATTR_IGNORE_ENEMIES);
            if ((this->m_hTargetAim != nullptr && actor->GetVisionInterface()->IsLineOfSightClearToEntity(this->m_hTargetAim, &look))) {
                look_now = true;
                if (actor->GetBodyInterface()->IsHeadAimingOnTarget()) {
                    actor->EquipBestWeaponForThreat(nullptr);
			        actor->PressFireButton();
                }
            }
        }

        if (look_now) {
            actor->GetBodyInterface()->AimHeadTowards(look, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.1f, NULL, "Aiming at target we need to destroy to progress");
        }
    }

    return ActionResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotMoveTo::OnCommandString(CTFBot *actor, const char *cmd)
{

    if (V_stricmp(cmd, "stop interrupt action") == 0) {
        return EventDesiredResult<CTFBot>::Done("Stopping interrupt action");
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


std::unordered_map<std::string, CEconItemView *> taunt_item_view;

void UpdatePeriodicTasks(std::vector<PeriodicTask> &pending_periodic_tasks)
{
    for (auto it = pending_periodic_tasks.begin(); it != pending_periodic_tasks.end(); ) {
        auto& pending_task = *it;

        if (pending_task.bot == nullptr || !pending_task.bot->IsAlive()) {
            it = pending_periodic_tasks.erase(it);
            continue;
        }
        CTFBot *bot = pending_task.bot;

        if (pending_task.health_below != 0 && bot->GetHealth() >= pending_task.health_below) {
            pending_task.when = gpGlobals->curtime + pending_task.delay;
        }

        if (gpGlobals->curtime >= pending_task.when && (pending_task.health_below == 0 || bot->GetHealth() <= pending_task.health_below)) {
            const CKnownEntity *threat;
            if ((pending_task.health_above > 0 && bot->GetHealth() <= pending_task.health_above) || (
                    pending_task.if_target && ((threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(true)) == nullptr || threat->GetEntity() == nullptr ))) {
                if (pending_task.health_below > 0)
                    pending_task.when = gpGlobals->curtime;

                pending_task.when+=pending_task.cooldown;
                continue;
            }
            if (pending_task.type==TASK_TAUNT) {
                if (!pending_task.attrib_name.empty()) {

                    CEconItemView *view = taunt_item_view[pending_task.attrib_name];
                    
                    if (view == nullptr) {
                        auto item_def = GetItemSchema()->GetItemDefinitionByName(pending_task.attrib_name.c_str());
                        if (item_def != nullptr) {
                            view = CEconItemView::Create();
                            view->Init(item_def->m_iItemDefIndex, 6, 9999, 0);
                            taunt_item_view[pending_task.attrib_name] = view;
                        }
                    }

                    if (view != nullptr) {
                        bot->PlayTauntSceneFromItem(view);
                        THINK_FUNC_SET(bot, StopTaunt, gpGlobals->curtime + pending_task.duration);
                    }
                    
                }
                else {
                    
                    const char * commandn = "taunt";
                    CCommand command = CCommand();
                    command.Tokenize(commandn);
                    bot->ClientCommand(command);
                    //bot->Taunt(TAUNT_BASE_WEAPON, 0);
                }
            }
            else if (pending_task.type==TASK_GIVE_SPELL) {
                CTFPlayer *ply = bot;
                for (int i = 0; i < MAX_WEAPONS; ++i) {
                    CBaseCombatWeapon *weapon = ply->GetWeapon(i);
                    if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) continue;
                    
                    CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
                    if (pending_task.spell_type < SPELL_TYPE_COUNT){
                        spellbook->m_iSelectedSpellIndex=pending_task.spell_type;
                    }
                    else{
                        if (pending_task.spell_type == 12) //common spell
                            spellbook->m_iSelectedSpellIndex=RandomInt(0,6);
                        else if (pending_task.spell_type == 13) //rare spell
                            spellbook->m_iSelectedSpellIndex=RandomInt(7,11);
                        else if (pending_task.spell_type == 14) //all spells
                            spellbook->m_iSelectedSpellIndex=RandomInt(0,11);
                    }
                    spellbook->m_iSpellCharges+=pending_task.spell_count;
                    if (spellbook->m_iSpellCharges > pending_task.max_spells)
                        spellbook->m_iSpellCharges = pending_task.max_spells;
                    
                        
                    DevMsg("Weapon %d %s\n",i , weapon -> GetClassname());
                    break;
                }
                DevMsg("Spell task executed %d\n", pending_task.spell_type);
            }
            else if (pending_task.type==TASK_VOICE_COMMAND) {
                TFGameRules()->VoiceCommand(reinterpret_cast<CBaseMultiplayerPlayer*>(bot), pending_task.spell_type / 10, pending_task.spell_type % 10);
                //ToTFPlayer(bot)->SpeakConceptIfAllowed(pending_task.spell_type);
                /*const char * commandn3= "voicemenu 0 0";
                CCommand command3 = CCommand();
                command3.Tokenize(commandn3);
                DevMsg("Command test 1 %d\n", command3.ArgC());
                for (int i = 0; i < command3.ArgC(); i++){
                    DevMsg("%d. Argument %s\n",i, command3[i]);
                }

                DevMsg("\n");
                ToTFPlayer(bot)->ClientCommand(command);*/
                DevMsg("Voice command executed %d\n", pending_task.spell_type);
            }
            else if (pending_task.type==TASK_FIRE_WEAPON) {

                switch (pending_task.spell_type) {
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
                if (pending_task.duration >= 0){

                    CTFBot::AttributeType attrs = bot->m_nBotAttrs;

                    bot->m_nBotAttrs = CTFBot::ATTR_NONE;

                    switch (pending_task.spell_type) {
                    case 0:
                        bot->PressFireButton(pending_task.duration);
                        break;
                    case 1:
                        bot->PressAltFireButton(pending_task.duration);
                        break;
                    case 2:
                        bot->PressSpecialFireButton(pending_task.duration);
                        break;
                    case 3:
                        bot->PressReloadButton(pending_task.duration);
                        break;
                    case 4:
                        bot->PressJumpButton(pending_task.duration);
                        break;
                    case 5:
                        bot->PressCrouchButton(pending_task.duration);
                        break;
                    }

                    bot->m_nBotAttrs = attrs;
                }
                if (pending_task.spell_type == 6) {
                    bot->UseActionSlotItemPressed();
                }
                
                DevMsg("FIRE_WEAPON_ %d\n", pending_task.spell_type);
            }
            else if (pending_task.type==TASK_CHANGE_ATTRIBUTES) {
                
                const CTFBot::EventChangeAttributes_t *attrib = bot->GetEventChangeAttributes(pending_task.attrib_name.c_str());
                if (attrib != nullptr){
                    DevMsg("Attribute exists %s\n", pending_task.attrib_name.c_str());
                    bot->OnEventChangeAttributes(attrib);
                }
                DevMsg("Attribute changed %s\n", pending_task.attrib_name.c_str());
            }
            else if (pending_task.type==TASK_FIRE_INPUT) {
                variant_t variant1;
                string_t m_iParameter = AllocPooledString(pending_task.param.c_str());
                    variant1.SetString(m_iParameter);
                CEventQueue &que = g_EventQueue;
                que.AddEvent(STRING(AllocPooledString(pending_task.attrib_name.c_str())),STRING(AllocPooledString(pending_task.input_name.c_str())),variant1,0,bot,bot,-1);
                std::string targetname = STRING(bot->GetEntityName());
                int findamp = targetname.find('&');
                if (findamp != -1){
                    que.AddEvent(STRING(AllocPooledString((pending_task.attrib_name+targetname.substr(findamp)).c_str())),STRING(AllocPooledString(pending_task.input_name.c_str())),variant1,0,bot,bot,-1);
                }

                //trigger->AcceptInput("trigger", this->parent, this->parent ,variant1,-1);
            }
            else if (pending_task.type == TASK_MESSAGE) {
                int linenum = 0;
                Mod::Pop::Wave_Extensions::ParseColorsAndPrint(pending_task.attrib_name.c_str(), 0.1f, linenum, nullptr);
            }
            else if (pending_task.type == TASK_WEAPON_SWITCH) {
                bot->m_iWeaponRestrictionFlags = (CTFBot::WeaponRestriction)pending_task.spell_type;
            }
            else if (pending_task.type == TASK_ADD_ATTRIBUTE || pending_task.type == TASK_REMOVE_ATTRIBUTE) {
                CAttributeList *list = nullptr;

                /*if (pending_task.spell_type == -2) { // TFBot
                    CTFBot::EventChangeAttributes_t ecattr;
                    KeyValues *kv = new KeyValues();
                    kv->SetString(pending_task.attrib_name, pending_task.param);
                    ParseDynamicAttributes(ecattr, kv);
                    kv->deleteThis();
                    if (FStrEq(pending_task.attrib_name, "MaxVisionRange"))
                }*/
                if (pending_task.spell_type == -1) { // Player
                    list = bot->GetAttributeList();
                }
                if (pending_task.spell_type == -2) { // Active weapon
                    CTFWeaponBase *entity = bot->GetActiveTFWeapon();
                    if (entity != nullptr)
                        list = &entity->GetItem()->GetAttributeList();
                }
                else { // Weapon
                    CEconEntity *entity = bot->GetEconEntityById(pending_task.spell_type);
                    if (entity != nullptr)
                        list = &entity->GetItem()->GetAttributeList();
                }

                if (list != nullptr) {
                    auto attr_def = GetItemSchema()->GetAttributeDefinitionByName(pending_task.attrib_name.c_str());
                    if (attr_def != nullptr) {
                        if (pending_task.type == TASK_ADD_ATTRIBUTE)
                            list->AddStringAttribute(attr_def, pending_task.param);
                        else if (pending_task.type == TASK_REMOVE_ATTRIBUTE)
                            list->RemoveAttribute(attr_def);
                    }
                }
            }
            else if (pending_task.type == TASK_SEQUENCE) {
                bot->PlaySpecificSequence(pending_task.attrib_name.c_str());
            }
            else if (pending_task.type == TASK_CLIENT_COMMAND) {
                const char * commandn = pending_task.attrib_name.c_str();
                CCommand command;
                command.Tokenize(commandn);
                bot->ClientCommand(command);
            }
            else if (pending_task.type == TASK_INTERRUPT_ACTION) {
                std::string command = "interrupt_action";

                if (!pending_task.attrib_name.empty()) {
                    float posx, posy, posz;

                    if (sscanf(pending_task.attrib_name.c_str(),"%f %f %f", &posx, &posy, &posz) == 3) {
                        command += CFmtStr(" -pos %f %f %f", posx, posy, posz);
                    }
                    else {
                        command += CFmtStr(" -posent %s", pending_task.attrib_name.c_str());
                    }
                }
                if (!pending_task.input_name.empty()) {
                    float posx, posy, posz;

                    if (sscanf(pending_task.input_name.c_str(),"%f %f %f", &posx, &posy, &posz) == 3) {
                        command += CFmtStr(" -lookpos %f %f %f", posx, posy, posz);
                    }
                    else {
                        command += CFmtStr(" -lookposent %s", pending_task.input_name.c_str());
                    }
                }
                
                if (!pending_task.param.empty()) {
                    command += " -ondoneattributes " + pending_task.param;
                }

                command += CFmtStr(" -duration %f", pending_task.duration);

                if (pending_task.spell_count != 0)
                    command += " -waituntildone";

                if (pending_task.spell_type != 0)
                    command += " -killlook";
                
                bot->MyNextBotPointer()->OnCommandString(command.c_str());
            }
            else if (pending_task.type == TASK_SPRAY) {
                Vector forward;
                trace_t	tr;	

                Vector origin;
                QAngle angles;
                if (sscanf(pending_task.attrib_name.c_str(),"%f %f %f", &origin.x, &origin.y, &origin.z) != 3) {
                    origin = bot->WorldSpaceCenter() + Vector (0 , 0 , 32);
                }
                if (sscanf(pending_task.input_name.c_str(),"%f %f %f", &angles.x, &angles.y, &angles.z) != 3) {
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

            //info.Execute(pending_task.bot);
            DevMsg("Periodic task executed %f, %f\n", pending_task.when,gpGlobals->curtime);
            if (--pending_task.repeats == 0) {
                it =  pending_periodic_tasks.erase(it);
                continue;
            }
            else{
                pending_task.when+=pending_task.cooldown;

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

bool Parse_PeriodicTask(std::vector<PeriodicTaskImpl> &periodic_tasks, KeyValues *kv, const char *type_name)
{
    PeriodicTaskType type;
    if (FStrEq(type_name, "Taunt")) {
        type = TASK_TAUNT;
    } else if (FStrEq(type_name, "Spell")) {
        type = TASK_GIVE_SPELL;
    } else if (FStrEq(type_name, "VoiceCommand")) {
        type = TASK_VOICE_COMMAND;
    } else if (FStrEq(type_name, "FireWeapon")) {
        type = TASK_FIRE_WEAPON;
    } else if (FStrEq(type_name, "ChangeAttributes")) {
        type = TASK_CHANGE_ATTRIBUTES;
    } else if (FStrEq(type_name, "FireInput")) {
        type = TASK_FIRE_INPUT;
    } else if (FStrEq(type_name, "Message")) {
        type = TASK_MESSAGE;
    } else if (FStrEq(type_name, "WeaponSwitch")) {
        type = TASK_WEAPON_SWITCH;
    } else if (FStrEq(type_name, "AddAttribute")) {
        type = TASK_ADD_ATTRIBUTE;
    } else if (FStrEq(type_name, "RemoveAttribute")) {
        type = TASK_REMOVE_ATTRIBUTE;
    } else if (FStrEq(type_name, "Sequence")) {
        type = TASK_SEQUENCE;
    } else if (FStrEq(type_name, "ClientCommand")) {
        type = TASK_CLIENT_COMMAND;
    } else if (FStrEq(type_name, "InterruptAction")) {
        type = TASK_INTERRUPT_ACTION;
    } else if (FStrEq(type_name, "Spray")) {
        type = TASK_SPRAY;
    } else {
        return false;
    }

    const char *name;
    PeriodicTaskImpl task;
    task.type = type;
    FOR_EACH_SUBKEY(kv, subkey) {
        const char *name = subkey->GetName();
        
        if (FStrEq(name, "Cooldown")) {
            task.cooldown = Max(0.015f,subkey->GetFloat());
        } else if (FStrEq(name, "Repeats")) {
            task.repeats = subkey->GetInt();
        } else if (FStrEq(name, "Delay")) {
            task.when = subkey->GetFloat();
        }
        else if (FStrEq(name, "Type")) {
            task.spell_type=subkey->GetInt();

            if (type == TASK_GIVE_SPELL){
                const char *typen =subkey->GetString();
                for (int i = 0; i < SPELL_TYPE_COUNT_ALL; i++) {
                    if(FStrEq(typen,SPELL_TYPE[i])){
                        task.spell_type = i;
                    }	
                }
            }
            else if (type == TASK_VOICE_COMMAND){
                const char *typen =subkey->GetString();
                int resp = GetResponseFor(typen);
                if (resp >= 0)
                    task.spell_type = resp;
            }
            else if (type == TASK_FIRE_WEAPON){
                const char *typen =subkey->GetString();
                for (int i = 0; i < INPUT_TYPE_COUNT; i++) {
                    if(FStrEq(typen,INPUT_TYPE[i])){
                        task.spell_type = i;
                    }	
                }
            }
            else if (type == TASK_TAUNT) {
                task.spell_type = subkey->GetInt();
            }
            else if (type == TASK_WEAPON_SWITCH) {
                
                const char *typen = subkey->GetString();
                if (FStrEq(typen, "Primary"))
                    task.spell_type = 2;
                else if (FStrEq(typen, "Secondary"))
                    task.spell_type = 4;
                else if (FStrEq(typen, "Melee"))
                    task.spell_type = 1;
            }
        }
        else if (FStrEq(name, "Item")) {
            task.spell_type=subkey->GetInt();
            if (type == TASK_ADD_ATTRIBUTE || type == TASK_REMOVE_ATTRIBUTE) {
                const char *typen = subkey->GetString();
                if (FStrEq(typen, "Player")){
                    task.spell_type = -1;
                }
                if (FStrEq(typen, "Active")){
                    task.spell_type = -2;
                }
                else {
                    auto item_def = GetItemSchema()->GetItemDefinitionByName(typen);
                    if (item_def != nullptr)
                        task.spell_type = item_def->m_iItemDefIndex;
                }
            }
        }
        else if (FStrEq(name, "Limit")) {
            task.max_spells=subkey->GetInt();
        }
        else if (FStrEq(name, "Charges")) {
            task.spell_count=subkey->GetInt();
        }
        else if (FStrEq(name, "Duration")) {
            task.duration=subkey->GetFloat();
        }
        else if (FStrEq(name, "IfSeeTarget")) {
            task.if_target=subkey->GetBool();
        }
        else if (FStrEq(name, "Name") || FStrEq(name, "Target")) {
            task.attrib_name=subkey->GetString();
        }
        else if (FStrEq(name, "Action") || FStrEq(name, "AimTarget") || FStrEq(name, "TargetAngles")) {
            task.input_name=subkey->GetString();
        }
        else if (FStrEq(name, "Param") || FStrEq(name, "Value") || FStrEq(name, "OnDoneChangeAttributes")) {
            task.param=subkey->GetString();
        }
        else if (FStrEq(name, "IfHealthBelow")) {
            task.health_below=subkey->GetInt();
        }
        else if (FStrEq(name, "IfHealthAbove")) {
            task.health_above=subkey->GetInt();
        }
        else if (FStrEq(name, "WaitForDone")) {
            task.spell_count=subkey->GetInt();
        }
        else if (FStrEq(name, "KillAimTarget")) {
            task.spell_type=subkey->GetInt();
        }
        //else if (FStrEq(name, "SpawnTemplate")) {
        //	spawners[spawner].periodic_templ.push_back(Parse_SpawnTemplate(subkey));
        //	task.spell_type = spawners[spawner].periodic_templ.size()-1;
        //}
    }
    if (task.max_spells == 0)
        task.max_spells = task.spell_count;
    DevMsg("CTFBotSpawner %08x: add periodic(%f, %f)\n", (uintptr_t)&periodic_tasks, task.cooldown, task.when);
    periodic_tasks.push_back(task);
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

void ApplyPendingTask(CTFBot *bot, std::vector<PeriodicTaskImpl> &periodic_tasks, std::vector<PeriodicTask> &pending_periodic_tasks)
{
    for (auto task_spawner : periodic_tasks) {
        PeriodicTask ptask;
        ptask.bot=bot;
        ptask.type = task_spawner.type;
        ptask.delay = task_spawner.when;
        ptask.when = task_spawner.when + gpGlobals->curtime;
        ptask.repeats = task_spawner.repeats;
        ptask.cooldown = task_spawner.cooldown;
        ptask.spell_type = task_spawner.spell_type;
        ptask.spell_count = task_spawner.spell_count;
        ptask.max_spells = task_spawner.max_spells;
        ptask.duration = task_spawner.duration;
        ptask.if_target = task_spawner.if_target;
        ptask.attrib_name = task_spawner.attrib_name;
        ptask.health_below = task_spawner.health_below;
        ptask.health_above = task_spawner.health_above;
        ptask.input_name = task_spawner.input_name;
        ptask.param = task_spawner.param;
        pending_periodic_tasks.push_back(ptask);
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
                const char *pstr;
                CopyStringAttributeValueToCharPointerOutput(value.m_String, &pstr);
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
                const char *pstr;
                CopyStringAttributeValueToCharPointerOutput(value.m_String, &pstr);
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

const char *GetItemName(int item_defid) {
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
            });
            if (!found)
                return;
        }
        const char *classname = TranslateWeaponEntForClass_improved(pair.definition->GetItemClass(), player->GetPlayerClass()->GetClassIndex());
        CEconEntity *entity = static_cast<CEconEntity *>(ItemGeneration()->SpawnItem(pair.definition->m_iItemDefIndex, player->WorldSpaceCenter(), vec3_angle, 1, 6, classname));
        if (entity != nullptr) {
            if (mark) {
                entity->GetItem()->GetAttributeList().SetRuntimeAttributeValue(GetItemSchema()->GetAttributeDefinitionByName("is force item"), 1.0f);
            }
            if (!GiveItemToPlayer(player, entity, no_remove, respect_class, pair.name.c_str()))
            {
                entity->Remove();
            }
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