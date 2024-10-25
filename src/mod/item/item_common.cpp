#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/econ.h"
#include "util/misc.h"
#include "mod/item/item_common.h"
#include "mod/pop/popmgr_extensions.h"
#include "stub/extraentitydata.h"
#include "mod/etc/mapentity_additions.h"

std::vector<ItemDefLanguage> g_ItemLanguages;

// game language name -> sourcemod language name
// First language is default language
std::vector<std::pair<std::string, std::string>> g_LangFileName {

    {"english","english"},
    {"brazilian","brazilian"},
    {"portuguese","portuguese"},
    {"bulgarian","bulgarian"},
    {"czech","czech"},
    {"danish","danish"},
    {"dutch","dutch"},
    {"finnish","finnish"},
    {"french","french"},
    {"german","german"},
    {"greek","greek"},
    {"hungarian","hungarian"},
    {"italian","italian"},
    {"japanese","japanese"},
    {"korean","korean"},
    {"koreana","koreana"},
    {"latam","spanish"},
    {"norwegian","norwegian"},
    {"polish","polish"},
    {"romanian", "romanian"},
    {"russian", "russian"},
    {"schinese", "schinese"},
    {"spanish", "spanish"},
    {"tchinese", "tchinese"},
    {"thai", "thai"},
    {"turkish", "turkish"},
    {"ukrainian", "ukrainian"}
};
ItemDefLanguage *g_ItemDefaultLanguage;

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

const char *GetAttributeName(int attributeIndex, CTFPlayer *player) {

    auto find = GetItemLanguage(player)->m_Attribnames.find(attributeIndex);
    if (find != GetItemLanguage(player)->m_Attribnames.end()) {
        return find->second.c_str();
    }
    else {
        find = GetItemLanguage(nullptr)->m_Attribnames.find(attributeIndex);
        if (find != GetItemLanguage(nullptr)->m_Attribnames.end()) {
            return find->second.c_str();
        }
    }
    return GetItemSchema()->GetAttributeDefinition(attributeIndex)->GetName();
}

const char *GetAttributeShortName(int attributeIndex, CTFPlayer *player) {
    
    auto find = GetItemLanguage(player)->m_AttribnamesShort.find(attributeIndex);
    if (find != GetItemLanguage(player)->m_AttribnamesShort.end()) {
        return find->second.c_str();
    }
    else {
        find = GetItemLanguage(nullptr)->m_AttribnamesShort.find(attributeIndex);
        if (find != GetItemLanguage(nullptr)->m_AttribnamesShort.end()) {
            return find->second.c_str();
        }
    }
    return GetItemSchema()->GetAttributeDefinition(attributeIndex)->GetName();
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

const char *GetItemNameForDisplay(const CEconItemView *view, CTFPlayer *player) {
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
        value = GetItemNameForDisplay(view->GetItemDefIndex(), player);
    }
    return value;
}

const char *GetItemNameForDisplay(int item_defid, CTFPlayer *player) {
    auto &itemNames = GetItemLanguage(player)->m_Itemnames;
    auto find = itemNames.find(item_defid);
    if (find != itemNames.end()) {
        return find->second.c_str();
    }
    else {
        auto &itemNames2 = GetItemLanguage(nullptr)->m_Itemnames;
        find = itemNames2.find(item_defid);
        if (find != itemNames2.end()) {
            return find->second.c_str();
        }

        auto item_def = GetItemSchema()->GetItemDefinition(item_defid);
        if (item_def != nullptr) {
            return item_def->GetName();
        }
        return nullptr;
    }
}

StaticFuncThunk<void *, const void *, unsigned int, unsigned int *, unsigned int> ft_COM_CompressBuffer_LZSS("COM_CompressBuffer_LZSS");
void* COM_CompressBuffer_LZSS( const void *source, unsigned int sourceLen, unsigned int *compressedLen, unsigned int maxCompressedLen )
{
    return ft_COM_CompressBuffer_LZSS(source, sourceLen, compressedLen, maxCompressedLen);
}

StaticFuncThunk<void *, const void *, unsigned int, unsigned int *, unsigned int> ft_COM_CompressBuffer_Snappy("COM_CompressBuffer_Snappy");
void* COM_CompressBuffer_Snappy( const void *source, unsigned int sourceLen, unsigned int *compressedLen, unsigned int maxCompressedLen )
{
    return ft_COM_CompressBuffer_Snappy(source, sourceLen, compressedLen, maxCompressedLen);
}

StaticFuncThunk<bool, void *, unsigned int *, const void *, unsigned int> ft_COM_BufferToBufferDecompress("COM_BufferToBufferDecompress");
bool COM_BufferToBufferDecompress( void *dest, unsigned int *destLen, const void *source, unsigned int sourceLen )
{
    return ft_COM_BufferToBufferDecompress(dest, destLen, source, sourceLen);
}

StaticFuncThunk<unsigned int , const void *, unsigned int> ft_COM_GetUncompressedSize("COM_GetUncompressedSize");
unsigned int COM_GetUncompressedSize( const void *source, unsigned int sourceLen)
{
    return ft_COM_GetUncompressedSize(source, sourceLen);
}

CON_COMMAND(sig_gen_extra_attributes, "")
{
    CUtlBuffer buf(0,0, CUtlBuffer::TEXT_BUFFER);
    buf.PutString("Phrases\n{\n");
    for (int i = 4324; i < 6000; i++) {
        auto def = GetItemSchema()->GetAttributeDefinition(i);
        if (def != nullptr) {
            const char *str = def->GetKeyValues()->GetString("description_string", "");
            if (str[0] != '\0') {
                buf.PutString(CFmtStr("	\"%s\"\n", def->GetName()));
                buf.PutString("	{\n");
                buf.PutString(CFmtStr("		\"en\" \"%s\"\n", str));
                buf.PutString("	}\n");
            }
        }
    }
    buf.PutString("}");
    filesystem->WriteFile("sigsegvattributes.phrases.txt", nullptr, buf);
}

CON_COMMAND(sig_gen_extra_names, "")
{
    const char *namesToExport[] {
        // "Store_ItemTypeFilterLabel",
        // "Store_ClassFilterLabel",
        // "Store_ItemDesc_AllClasses",
        // "TF_PVE_UpgradeRespec",
        // "TF_PVE_Upgrades",
        // "TF_PVE_Active_Upgrades",
        // "TF_PVE_UpgradeTitle",
        // "TF_PVE_UpgradeAttrib",
        // "TF_ImportPreview_LoadoutLabel",
        // "Store_Sorter_PlayerClassName",
        // "Rarity_Common",
        // "MMenu_BuyNow",
        "TF_Charge",
        "TR_Demo_TargetSlot2Title",
        "TF_playerid_mediccharge",
        "TF_Wearable_Shield",
        "TF_PyroRage",
        "TF_Rage",
        "TF_Hype",
        "TF_KILLS",
        "TF_SniperRage",
        "TF_SmgCharge",
        "TF_CRITS",
        "TF_Rescue"
    };

    
    std::map<std::string, std::map<std::string, std::string>> namesExported; 
    std::map<std::string, std::string> classNames[TF_CLASS_COUNT]; 
    std::map<std::string, std::string> loadoutNames[LOADOUT_POSITION_COUNT]; 

    for (auto &[langFileName, langName] : g_LangFileName) {
        KeyValues *kvin = new KeyValues("Lang");
        kvin->UsesEscapeSequences(true);

        CUtlBuffer file( 0, 0, CUtlBuffer::TEXT_BUFFER );
        filesystem->ReadFile(CFmtStr("resource/tf_%s.txt", langFileName.c_str()), "GAME", file);
        
        char *buf = new char[file.TellPut() * 2];
        int sizec = _V_UCS2ToUTF8( (const ucs2*) (file.String() + 2), buf, file.TellPut() * 2 );

        if (kvin->LoadFromBuffer(langFileName.c_str(), buf)/**/) {
            uint lindex = 0;
            translator->GetLanguageByName(langName.c_str(), &lindex);

            KeyValues *tokens = kvin->FindKey("Tokens");
            std::unordered_map<int, std::string> strings;

            FOR_EACH_SUBKEY(tokens, subkey) {
                strings[subkey->GetNameSymbol()] = subkey->GetString();
            }

            const char *langCode;
            translator->GetLanguageInfo(lindex, &langCode, nullptr);

            for (int i = 0; i < LOADOUT_POSITION_COUNT; i++) {
                loadoutNames[i][langCode] = strings[KeyValues::CallGetSymbolForString(g_szLoadoutStringsForDisplay[i]+1, false)];
            }

            for (int i = 0; i < TF_CLASS_COUNT; i++) {
                classNames[i][langCode] = strings[KeyValues::CallGetSymbolForString(g_aPlayerClassNames[i]+1, false)];
            }

            for (auto &exportName : namesToExport) {
                namesExported[exportName][langCode] = strings[KeyValues::CallGetSymbolForString(exportName, false)];
            }
        }
    }
    
    for (int i = 0; i < LOADOUT_POSITION_COUNT; i++) {
        Msg("\"%s\"\n",loadoutStrings[i]);
        Msg("{\n");
        for (auto &[code, name] : loadoutNames[i]) {
            Msg("	\"%s\" \"%s\"\n", code.c_str(), name.c_str());
        }
        Msg("}\n");
        Msg("\n");
    }
    
    for (int i = 0; i < TF_CLASS_COUNT; i++) {
        Msg("\"%s\"\n",g_aPlayerClassNames_NonLocalized[i]);
        Msg("{\n");
        for (auto &[code, name] : classNames[i]) {
            Msg("	\"%s\" \"%s\"\n", code.c_str(), name.c_str());
        }
        Msg("}\n");
        Msg("\n");
    }
    
    for (auto &[exportName, codeMap] : namesExported) {
        Msg("\"%s\"\n",exportName.c_str());
        Msg("{\n");
        for (auto &[code, name] : codeMap) {
            Msg("	\"%s\" \"%s\"\n", code.c_str(), name.c_str());
        }
        Msg("}\n");
        Msg("\n");
    }
}

void GenerateItemNames() {
    bool firstLang = true;
    
    char path_sm[PLATFORM_MAX_PATH];
    g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/itemlanguages/");
    mkdir(path_sm, 0766);

    std::vector<std::pair<int, const char *>> item_def_names;
    std::vector<std::pair<int, const char *>> attr_names;
    std::vector<std::pair<int, const char *>> attr_names_sig;

    for (int i = 0; i < UINT16_MAX; i++)
    {
        CEconItemDefinition *def = GetItemSchema()->GetItemDefinition(i);
        if (def != nullptr && !FStrEq(def->GetItemName(""), "#TF_Default_ItemDef") && strncmp(def->GetItemClass(), "tf_", 3) == 0) {
            const char *item_slot = def->GetKeyValues()->GetString("item_slot", nullptr);
            if (item_slot != nullptr) {
                item_def_names.emplace_back(i, def->GetItemName("#")+1);
            }
        }
    }

    for (int i = 0; i < 4000; i++)
    {
        auto def = GetItemSchema()->GetAttributeDefinition(i);
        if (def != nullptr) {
            const char *str = def->GetKeyValues()->GetString("description_string", "#")+1;
            if (str[0] != '\0') {
                attr_names.emplace_back(i, str);
            }
        }
    }

    for (auto &[langFileName, langName] : g_LangFileName) {
        KeyValues *kvin = new KeyValues("Lang");
        kvin->UsesEscapeSequences(true);

        CUtlBuffer file( 0, 0, CUtlBuffer::TEXT_BUFFER );
        filesystem->ReadFile(CFmtStr("resource/tf_%s.txt", langFileName.c_str()), "GAME", file);
        
        char *buf = new char[file.TellPut() * 2];
        int sizec = _V_UCS2ToUTF8( (const ucs2*) (file.String() + 2), buf, file.TellPut() * 2 );

        if (kvin->LoadFromBuffer(langFileName.c_str(), buf)/**/) {
            uint lindex = 0;
            translator->GetLanguageByName(langName.c_str(), &lindex);
            auto &lang = g_ItemLanguages[lindex];
            lang.m_bInitialized = true;
            lang.m_bLoaded = true;
            lang.m_Name = langName;

            int64_t time = filesystem->GetFileTime(CFmtStr("resource/tf_%s.txt", langFileName.c_str()), "GAME");
            if (firstLang) {
                g_ItemDefaultLanguage = &lang;
                firstLang = false;
            }
            KeyValues *tokens = kvin->FindKey("Tokens");
            std::unordered_map<int, std::string> strings;

            FOR_EACH_SUBKEY(tokens, subkey) {
                strings[subkey->GetNameSymbol()] = subkey->GetString();
            }

            for (auto &[itemDefIndex, itemDefName] : item_def_names) {
                lang.m_Itemnames[itemDefIndex] = strings[KeyValues::CallGetSymbolForString(itemDefName, false)];
            }

            for (auto &[attrDefIndex, attrDefName] : attr_names) {
                lang.m_Attribnames[attrDefIndex] = strings[KeyValues::CallGetSymbolForString(attrDefName, false)];
                int shortDescSymbol = KeyValues::CallGetSymbolForString(CFmtStr("%s_shortdesc",attrDefName), false);
                auto find = strings.find(shortDescSymbol);
                if (shortDescSymbol != -1 && find != strings.end()) {
                    lang.m_AttribnamesShort[attrDefIndex] = find->second;
                }
                else {
                    lang.m_AttribnamesShort[attrDefIndex] = lang.m_Attribnames[attrDefIndex];
                }
            }

            g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/itemlanguages/%s.dat", langName.c_str());
            CUtlBuffer fileout( 0, 0, 0 );
            fileout.PutInt64(time);
            fileout.PutInt(lang.m_Itemnames.size());
            fileout.PutInt(lang.m_AttribnamesShort.size());
            fileout.PutInt(lang.m_Attribnames.size());
            for (auto &entry : lang.m_Itemnames) {
                fileout.PutInt(entry.first);
                fileout.PutString(entry.second.c_str());
            }
            
            for (auto &entry : lang.m_AttribnamesShort) {
                fileout.PutUnsignedShort(entry.first);
                fileout.PutString(entry.second.c_str());
            }
            
            for (auto &entry : lang.m_Attribnames) {
                fileout.PutUnsignedShort(entry.first);
                fileout.PutString(entry.second.c_str());
            }
            uint outputSize;
            void *outputCompress = COM_CompressBuffer_Snappy(fileout.Base(), fileout.TellPut(), &outputSize, UINT_MAX);

            auto fileh = filesystem->Open(path_sm,"wb", "GAME");
            if (fileh != nullptr) {
                filesystem->Write(outputCompress, outputSize, fileh);
                filesystem->Close(fileh);
            }
            free(outputCompress);

        }
        kvin->deleteThis();
        delete[] buf;
    }
}


void LoadItemNames(uint langNum) {
    auto &lang = g_ItemLanguages[langNum];
    
    if (!lang.m_bLoaded) {
        lang.m_bLoaded = true;
        const char *langName, *langCode;
        translator->GetLanguageInfo(langNum, &langCode, &langName);

        auto nameFind = std::find_if(g_LangFileName.begin(), g_LangFileName.end(), [langName](auto &pair){ return pair.second == langName;});
        if (nameFind == g_LangFileName.end()) {
            return;
        }

        char path_sm[PLATFORM_MAX_PATH];
        g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/itemlanguages/");
        mkdir(path_sm, 0766);

        g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"data/itemlanguages/%s.dat", langName);

        CUtlBuffer fileCompressed( 0, 0, 0 );
        CUtlBuffer file( 0, 0, 0 );
        if (filesystem->ReadFile(path_sm, "GAME", fileCompressed)) {
            file.EnsureCapacity(COM_GetUncompressedSize(fileCompressed.Base(), fileCompressed.TellPut()));
            uint fileDecompressSize = file.Size();
            file.SeekPut(CUtlBuffer::SEEK_HEAD, fileDecompressSize);
            if (!COM_BufferToBufferDecompress(file.Base(), &fileDecompressSize, fileCompressed.Base(), fileCompressed.TellPut())) {
                file.SetExternalBuffer(fileCompressed.Base(), fileCompressed.Size(), fileCompressed.TellPut(), 0);
            }

            lang.m_bInitialized = true;
            int64 timewrite = file.GetInt64();
        
            int64_t time = filesystem->GetFileTime(CFmtStr("resource/tf_%s.txt", nameFind->first.c_str()), "GAME");

            if (timewrite != time) {
                GenerateItemNames();
                return;
            }

            char buf[256];
            int num_itemnames = file.GetInt();
            int num_attrnames_short = file.GetInt();
            int num_attrnames = file.GetInt();
            lang.m_Itemnames.reserve(num_itemnames);
            lang.m_AttribnamesShort.reserve(num_attrnames_short);
            lang.m_Attribnames.reserve(num_attrnames);
            for (int i = 0; i < num_itemnames; i++) {
                int id = file.GetInt();
                file.GetString<256>(buf);
                lang.m_Itemnames[id] = buf;
            }

            for (int i = 0; i < num_attrnames_short; i++) {
                int id = file.GetUnsignedShort();
                file.GetString<256>(buf);
                lang.m_AttribnamesShort[id] = buf;
            }

            for (int i = 0; i < num_attrnames; i++) {
                int id = file.GetUnsignedShort();
                file.GetString<256>(buf);
                lang.m_Attribnames[id] = buf;
                if (!lang.m_AttribnamesShort.contains(id)) {
                lang.m_AttribnamesShort[id] = buf;
                }
            }
        }
        else {
            GenerateItemNames();
            return;
        }
    }
}

bool FormatAttributeString(std::string &string, CEconItemAttributeDefinition *attr_def, attribute_data_union_t value, CTFPlayer *player, bool shortDescription) {
    //DevMsg("inspecting attr\n");
    if (attr_def == nullptr)
        return false;
    
    //DevMsg("inspecting attr index %d\n", attr_def->GetIndex());
    KeyValues *kv = attr_def->GetKeyValues();
    const char *format = attr_def->GetDescription();
    if ((!shortDescription && attr_def->IsHidden()) || format == nullptr)
        return false;

    
	char val_buf[256];

    if (attr_def->GetIndex() < 4000) {
        if (format[0] != '#')
            return false;
        
        string = shortDescription ? GetAttributeShortName(attr_def->GetIndex(), player) : GetAttributeName(attr_def->GetIndex(), player);
        int val_pos = string.find("%s1");
        if (val_pos != -1) {
            auto desc_format = attr_def->GetDescriptionFormat();
            bool is_percentage = desc_format == ATTDESCFORM_VALUE_IS_PERCENTAGE;
            bool is_additive = desc_format == ATTDESCFORM_VALUE_IS_ADDITIVE;
            bool is_additive_percentage = desc_format == ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE;
            bool is_inverted_percentage = desc_format == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE;

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
        Translation translated;
        if (phrasesAttribsFile->GetTranslation(attr_def->GetName(), translator->GetClientLanguage(ENTINDEX(player)), &translated) == Trans_Okay) {
            string = translated.szPhrase;
        }
        else {
            string = format;
        }

        bool is_percentage = false;
        int val_pos = string.find("%d");
        if (val_pos == -1) {
            val_pos = string.find("%p");
            is_percentage = true;
        }

        if (val_pos != -1) {
            
            auto desc_format = attr_def->GetDescriptionFormat();
            bool is_additive = desc_format == ATTDESCFORM_VALUE_IS_ADDITIVE;
            bool is_inverted_percentage = desc_format == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE;

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

void CopyVisualAttributes(CTFPlayer *player, CEconEntity *copyFrom, CEconEntity *copyTo)
{
    //copyTo->GetItem()->Init(copyFrom->GetItem()->m_iItemDefinitionIndex, 9999, 414918);
    copyTo->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop")->props["m_iItemDefinitionIndex"] = {Variant((int)copyFrom->GetItem()->m_iItemDefinitionIndex), Variant((int)copyFrom->GetItem()->m_iItemDefinitionIndex)};
    copyTo->GetItem()->m_iEntityLevel = 414918;
    copyTo->GetItem()->m_iEntityQuality = copyFrom->GetItem()->m_iItemDefinitionIndex;
    int tint = 0;
    CALL_ATTRIB_HOOK_INT_ON_OTHER(copyFrom, tint, set_item_tint_rgb);
    if (tint != 0)
        copyTo->GetItem()->GetAttributeList().SetRuntimeAttributeValue(GetItemSchema()->GetAttributeDefinitionByName("set item tint rgb"), tint);
    int style = 0;
    CALL_ATTRIB_HOOK_INT_ON_OTHER(copyFrom, style, item_style_override);
    if (style != 0)
        copyTo->GetItem()->GetAttributeList().SetRuntimeAttributeValue(GetItemSchema()->GetAttributeDefinitionByName("item style override"), style);

    auto paintkitAttr = copyFrom->GetItem()->GetAttributeList().GetAttributeByName("paintkit_proto_def_index");
    if (paintkitAttr != nullptr)
        copyTo->GetItem()->GetAttributeList().SetRuntimeAttributeValue(GetItemSchema()->GetAttributeDefinitionByName("paintkit_proto_def_index"), paintkitAttr->GetValue().m_Float);
    float texture = 0;
    CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(copyFrom, texture, set_item_texture_wear);
    if (texture != 0)
        copyTo->GetItem()->GetAttributeList().SetRuntimeAttributeValue(GetItemSchema()->GetAttributeDefinitionByName("set_item_texture_wear"), texture);
    copyTo->UpdateBodygroups(player, true);
}

CTFWeaponBase *CreateCustomWeaponModelPlaceholder(CTFPlayer *owner, CTFWeaponBase *weapon, int modelIndex) 
{
    auto wearable = static_cast<CTFWeaponBase *>(CreateEntityByName("tf_weapon_laser_pointer"));
    wearable->GetItem()->Init(weapon != nullptr ? weapon->GetItem()->m_iItemDefinitionIndex : 492, 9999, 9999, 887);
    if (weapon != nullptr) {
        CopyVisualAttributes(owner, weapon, wearable);
    }
    wearable->m_pfnTouch = nullptr;
    wearable->ChangeTeam(owner->GetTeamNumber());
    wearable->GetItem()->m_iTeamNumber = owner->GetTeamNumber();
    DispatchSpawn(wearable);
    wearable->SetOwner(owner);
    wearable->SetOwnerEntity(owner);
    wearable->SetParent(owner, -1);
    wearable->AddEffects(EF_BONEMERGE|EF_BONEMERGE_FASTCULL);
    wearable->m_iState = WEAPON_IS_ACTIVE;
    wearable->m_iWorldModelIndex = modelIndex;
    wearable->SetModelIndex(modelIndex);
    for (int i = 0; i < MAX_VISION_MODES; ++i) {
    	wearable->SetModelIndexOverride(i, modelIndex);
    }
    wearable->m_bValidatedAttachedEntity = true;
    return wearable;
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

namespace Mod::Item::Common
{   
    
	DETOUR_DECL_MEMBER(int, CEconItemView_GetSkin, int team, bool viewmodel)
	{
        auto view = reinterpret_cast<CEconItemView *>(this);
        if (view->m_iEntityLevel == 414918 && view->m_iItemDefinitionIndex == 0) {
            view->m_iItemDefinitionIndex = view->m_iEntityQuality;
        }
        auto ret = DETOUR_MEMBER_CALL(team, viewmodel);
        if (view->m_iEntityLevel == 414918) {
            view->m_iItemDefinitionIndex = 0;
        }

        return ret;
    }

	DETOUR_DECL_MEMBER(const char *, CEconItemView_GetPlayerDisplayModel, int playerclass, int team)
	{
        auto view = reinterpret_cast<CEconItemView *>(this);
        if (view->m_iEntityLevel == 414918 && view->m_iItemDefinitionIndex == 0) {
            view->m_iItemDefinitionIndex = view->m_iEntityQuality;
        }
        auto ret = DETOUR_MEMBER_CALL(playerclass, team);
        if (view->m_iEntityLevel == 414918) {
            view->m_iItemDefinitionIndex = 0;
        }

        return ret;
    }


    class CMod : public IMod
    {
    public:
        CMod() : IMod("Item:Item_Common")
        {
            MOD_ADD_DETOUR_MEMBER(CEconItemView_GetSkin,                "CEconItemView::GetSkin");
            MOD_ADD_DETOUR_MEMBER(CEconItemView_GetPlayerDisplayModel,  "CEconItemView::GetPlayerDisplayModel");
        }

        virtual void OnEnable() override
        {
            if (g_ItemLanguages.empty()) {
                g_ItemLanguages.resize(translator->GetLanguageCount() * 2 + 1);
            }
            LoadItemNames(translator->GetServerLanguage());
        }
    };
    CMod s_Mod;
}