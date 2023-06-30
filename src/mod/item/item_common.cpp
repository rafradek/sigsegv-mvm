#include "stub/tfplayer.h"
#include "util/misc.h"
#include "mod/item/item_common.h"
#include "mod/pop/popmgr_extensions.h"

std::map<int, std::string> g_Itemnames;
std::map<int, std::string> g_Attribnames;
std::map<int, std::string> g_AttribnamesShort;

ItemListEntry_Similar::ItemListEntry_Similar(const char *name) : m_strName(name)
{
    auto itemDef = reinterpret_cast<CTFItemDefinition *>(GetItemSchema()->GetItemDefinitionByName(name));
    if (itemDef != nullptr 
#ifndef NO_MVM
    && Mod::Pop::PopMgr_Extensions::GetCustomWeaponItemDef(name) == nullptr
#endif
    ) {
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

const char *GetAttributeName(int attributeIndex) {
    return g_Attribnames[attributeIndex].c_str();
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
                if (str[0] != '\0') {
                    g_Attribnames[i] = strings[KeyValues::CallGetSymbolForString(str, false)];
                    int shortDescSymbol = KeyValues::CallGetSymbolForString(CFmtStr("%s_shortdesc",str), false);
                    auto find = strings.find(shortDescSymbol);
                    if (shortDescSymbol != -1 && find != strings.end()) {
                        g_AttribnamesShort[i] = find->second;
                    }
                    else {
                        g_AttribnamesShort[i] = g_Attribnames[i];
                    }
                }
            }
        }
       // timer3.End();
        //Msg("Def time %.9f\n", timer3.GetDuration().GetSeconds());

        char path_sm[PLATFORM_MAX_PATH];
        g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/sig_item_data3.dat");
        CUtlBuffer fileout( 0, 0, 0 );
        fileout.PutInt64(filesystem->GetFileTime("resource/tf_english.txt", "GAME"));

        fileout.PutInt(g_Itemnames.size());
        fileout.PutInt(g_AttribnamesShort.size());
        fileout.PutInt(g_Attribnames.size());
        for (auto &entry : g_Itemnames) {
            fileout.PutInt(entry.first);
            fileout.PutString(entry.second.c_str());
        }
        
        for (auto &entry : g_AttribnamesShort) {
            fileout.PutUnsignedShort(entry.first);
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
        g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/sig_item_data3.dat");

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
            int num_attrnames_short = file.GetInt();
            int num_attrnames = file.GetInt();
            char buf[256];
            for (int i = 0; i < num_itemnames; i++) {
                int id = file.GetInt();
                file.GetString<256>(buf);
                g_Itemnames[id] = buf;
            }

            for (int i = 0; i < num_attrnames_short; i++) {
                int id = file.GetUnsignedShort();
                file.GetString<256>(buf);
                g_AttribnamesShort[id] = buf;
            }

            for (int i = 0; i < num_attrnames; i++) {
                int id = file.GetUnsignedShort();
                file.GetString<256>(buf);
                g_Attribnames[id] = buf;
                if (!g_AttribnamesShort.contains(id)) {
                   g_AttribnamesShort[id] = buf;
                }
            }
        }
        else {
            GenerateItemNames();
            return;
        }
    }
}

bool FormatAttributeString(std::string &string, CEconItemAttributeDefinition *attr_def, attribute_data_union_t value, bool shortDescription) {
    DevMsg("inspecting attr\n");
    if (attr_def == nullptr)
        return false;
    
    DevMsg("inspecting attr index %d\n", attr_def->GetIndex());
    KeyValues *kv = attr_def->GetKeyValues();
    const char *format = kv->GetString("description_string");
    if ((!shortDescription && kv->GetBool("hidden")) || format == nullptr)
        return false;

    
	char val_buf[256];

    if (attr_def->GetIndex() < 4000) {
        if (format[0] != '#')
            return false;
        
        string = shortDescription ? g_AttribnamesShort[attr_def->GetIndex()] : g_Attribnames[attr_def->GetIndex()];
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

CON_COMMAND(sig_gen_rand, "")
{
    CUtlBuffer file( 0, 0, CUtlBuffer::TEXT_BUFFER );
    for (int i = 253; i < 4000; i++)
    {
        auto def = GetItemSchema()->GetAttributeDefinition(i);
        if (def != nullptr) {
            const char *name = def->GetKeyValues()->GetString("name", "");
            const char *attrClass = def->GetKeyValues()->GetString("attribute_class", "");
            const char *type = def->GetKeyValues()->GetString("description_format", "");
            const char *attrType = def->GetKeyValues()->GetString("attribute_type", "");
            const char *positive = def->GetKeyValues()->GetString("effect_type", "");
            
            if (attrType[0] != '\0') continue;

            file.PutString("    { name = \"");
            file.PutString(name);
            file.PutString("\", type = \"");
            file.PutString(type[0] == '\0' ? "additive" : type + 9);
            file.PutString("\", clas = \"");
            file.PutString(attrClass);
            file.PutString("\", positive = ");
            file.PutString(!FStrEq(positive, "negative") ? "true" : "false");
            file.PutString(", chance = 1.0, ");
            if (FStrEq(positive, "neutral")) {
                file.PutString("neutral = true, ");
            }
            if (FStrEq(type + 9, "additive")) {
                file.PutString("minStrength = 0.25, maxStrength = 0.25, weight0 = 4.0");
            }
            else {
                file.PutString("multStrength = 1.0, weight0 = 1.0");
            }
            file.PutString(", requirements = \n        { }\n    },\n");
        }
    }
    char path_sm[PLATFORM_MAX_PATH];
    g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/sig_gen.nut");
    filesystem->WriteFile(path_sm, "GAME", file);
}