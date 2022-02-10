#include "stub/econ.h"
#include "addr/addr.h"
#include "stub/strings.h"
#include "util/misc.h"
#include "mem/extract.h"

#include <boost/algorithm/string.hpp>

// #include <strcompat.h>


using const_char_ptr = const char *;

#if defined _LINUX

static constexpr uint8_t s_Buf_perteamvisuals_t_m_Sounds[] = {
	0x8b, 0x80, 0x00, 0x00, 0x00, 0x00,       // +0000  mov eax,[eax+m_Visuals]
	0x85, 0xc0,                               // +0006  test eax,eax
	0x74, 0x00,                               // +0008  jz +0x??
	0x83, 0xfb, NUM_SHOOT_SOUND_TYPES,        // +000A  cmp ebx,NUM_SHOOT_SOUND_TYPES
	0x77, 0x00,                               // +000D  ja +0x??
	0x8b, 0x84, 0x98, 0x00, 0x00, 0x00, 0x00, // +000F  mov eax,[eax+ebx*4+m_Sounds]
};

struct CExtract_perteamvisuals_t_m_Sounds : public IExtract<const_char_ptr (*)[NUM_SHOOT_SOUND_TYPES]>
{
	CExtract_perteamvisuals_t_m_Sounds() : IExtract<const_char_ptr (*)[NUM_SHOOT_SOUND_TYPES]>(sizeof(s_Buf_perteamvisuals_t_m_Sounds)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_perteamvisuals_t_m_Sounds);
		
		int off_CEconItemDefinition_m_Visuals;
	//	if (!Prop::FindOffset(off_CEconItemDefinition_m_Visuals, "CEconItemDefinition", "m_Visuals")) return false;
		if (!Prop::FindOffset(off_CEconItemDefinition_m_Visuals, "CEconItemDefinition", "m_Visuals")) {
			DevMsg("Extractor for perteamvisuals_t::m_Sounds: can't find prop offset for CEconItemDefinition::m_Visuals\n");
			return false;
		}
		
		buf.SetDword(0x00 + 2, (uint32_t)off_CEconItemDefinition_m_Visuals);
		
		mask[0x08 + 1] = 0x00;
		mask[0x0d + 1] = 0x00;
		mask.SetDword(0x0f + 3, ~0x000003ff);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFWeaponBase::GetShootSound"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0100; } // @ +0x0056
	virtual uint32_t GetExtractOffset() const override { return 0x000f + 3; }
};

#elif defined _WINDOWS

using CExtract_perteamvisuals_t_m_Sounds = IExtractStub;

#endif


using perteamvisuals_t_ptr = perteamvisuals_t *;

#if defined _LINUX

static constexpr uint8_t s_Buf_CEconItemDefinition_m_Visuals[] = {
	0xc7, 0x84, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // +0000  mov dword ptr [ebx+eax*4+m_Visuals],0x00000000
	0x8b, 0x04, 0x85, 0x00, 0x00, 0x00, 0x00,                         // +000B  mov eax,g_TeamVisualSections[eax*4]
};

struct CExtract_CEconItemDefinition_m_Visuals : public IExtract<perteamvisuals_t_ptr (*)[NUM_VISUALS_BLOCKS]>
{
	CExtract_CEconItemDefinition_m_Visuals() : IExtract<perteamvisuals_t_ptr (*)[NUM_VISUALS_BLOCKS]>(sizeof(s_Buf_CEconItemDefinition_m_Visuals)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CEconItemDefinition_m_Visuals);
		
		buf.SetDword(0x0b + 3, (uint32_t)&g_TeamVisualSections.GetRef());
		
		mask.SetDword(0x00 + 3, ~0x000003ff);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CEconItemDefinition::BInitVisualBlockFromKV"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0100; } // @ +0x0057
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 3; }
};

#elif defined _WINDOWS

using CExtract_CEconItemDefinition_m_Visuals = IExtractStub;

#endif


#if defined _LINUX

static constexpr uint8_t s_Buf_CEconItemDefinition_EquipRegionMasks[] = {
	0xe8, 0x00, 0x00, 0x00, 0x00,       // +0000  call CEconItemSchema::GetEquipRegionBitMaskByName
	0x09, 0x83, 0x00, 0x00, 0x00, 0x00, // +0005  or [ebx+m_nEquipRegionBitMask],eax
	0x83, 0xc6, 0x01,                   // +000B  add esi,1
	0x8b, 0x85, 0x00, 0x00, 0x00, 0x00, // +000E  mov eax,[ebp+0xXXXXXXXX]
	0x09, 0x83, 0x00, 0x00, 0x00, 0x00, // +0014  or [ebx+m_nEquipRegionMask],eax
};

struct CExtract_CEconItemDefinition_EquipRegionMasks : public IExtract<unsigned int *>
{
	CExtract_CEconItemDefinition_EquipRegionMasks() : IExtract<unsigned int *>(sizeof(s_Buf_CEconItemDefinition_EquipRegionMasks)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CEconItemDefinition_EquipRegionMasks);
		
		mask.SetDword(0x0000 + 1, 0x00000000);
		mask.SetDword(0x0005 + 2, 0x00000000);
		mask.SetDword(0x000e + 2, 0x00000000);
		mask.SetDword(0x0014 + 2, 0x00000000);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override { return "CEconItemDefinition::BInitFromKV"; }
	virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override  { return 0x2000; } // @ +0x13da
};

struct CExtract_CEconItemDefinition_m_nEquipRegionBitMask : public CExtract_CEconItemDefinition_EquipRegionMasks
{
	virtual uint32_t GetExtractOffset() const override { return 0x0005 + 2; }
};

struct CExtract_CEconItemDefinition_m_nEquipRegionMask : public CExtract_CEconItemDefinition_EquipRegionMasks
{
	virtual uint32_t GetExtractOffset() const override { return 0x0014 + 2; }
};

#elif defined _WINDOWS

using CExtract_CEconItemDefinition_m_nEquipRegionBitMask = IExtractStub;
using CExtract_CEconItemDefinition_m_nEquipRegionMask    = IExtractStub;

#endif

MemberFuncThunk<CEconItemDefinition *, bool, KeyValues *, CUtlVector<CUtlString> *> CEconItemDefinition::ft_BInitFromKV("CEconItemDefinition::BInitFromKV");

MemberFuncThunk<CAttributeManager *, float, float, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> CAttributeManager::ft_ApplyAttributeFloatWrapper("CAttributeManager::ApplyAttributeFloatWrapper");
MemberFuncThunk<const CAttributeManager *, int> CAttributeManager::ft_GetGlobalCacheVersion("CAttributeManager::GetGlobalCacheVersion");
MemberFuncThunk<CAttributeManager *, void>  CAttributeManager::ft_ClearCache("CAttributeManager::ClearCache [clone]");

MemberVFuncThunk<CAttributeManager *, float, float, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> CAttributeManager::vt_ApplyAttributeFloatWrapper(TypeName<CAttributeManager>(), "CAttributeManager::ApplyAttributeFloatWrapper");
MemberVFuncThunk<CAttributeManager *, float, float, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> CAttributeManager::vt_ApplyAttributeFloat(TypeName<CAttributeManager>(), "CAttributeManager::ApplyAttributeFloat");

StaticFuncThunk<int, int, const char *, const CBaseEntity *, CUtlVector<CBaseEntity *> *, bool>     CAttributeManager::ft_AttribHookValue_int  ("CAttributeManager::AttribHookValue<int>");
StaticFuncThunk<float, float, const char *, const CBaseEntity *, CUtlVector<CBaseEntity *> *, bool> CAttributeManager::ft_AttribHookValue_float("CAttributeManager::AttribHookValue<float>");

IMPL_SENDPROP(CHandle<CBaseEntity>, CAttributeManager, m_hOuter, CEconEntity);
IMPL_SENDPROP(int, CAttributeManager, m_iReapplyProvisionParity, CEconEntity);
IMPL_RELATIVE(CUtlVector<CHandle<CBaseEntity>>, CAttributeManager, m_Receivers, m_iReapplyProvisionParity, -sizeof(CUtlVector<CHandle<CBaseEntity>>));
IMPL_RELATIVE(CUtlVector<CHandle<CBaseEntity>>, CAttributeManager, m_Providers, m_iReapplyProvisionParity, -sizeof(CUtlVector<CHandle<CBaseEntity>>) * 2);
IMPL_RELATIVE(CUtlVector<cached_attribute_t>, CAttributeManager, m_CachedResults, m_hOuter, +16);

IMPL_EXTRACT(const char *[NUM_SHOOT_SOUND_TYPES], perteamvisuals_t, m_Sounds, new CExtract_perteamvisuals_t_m_Sounds());


IMPL_EXTRACT(perteamvisuals_t *[NUM_VISUALS_BLOCKS], CEconItemDefinition, m_Visuals,             new CExtract_CEconItemDefinition_m_Visuals());
IMPL_EXTRACT(unsigned int,                           CEconItemDefinition, m_nEquipRegionBitMask, new CExtract_CEconItemDefinition_m_nEquipRegionBitMask());
IMPL_EXTRACT(unsigned int,                           CEconItemDefinition, m_nEquipRegionMask,    new CExtract_CEconItemDefinition_m_nEquipRegionMask());


MemberFuncThunk<const CTFItemDefinition *, int, int> CTFItemDefinition::ft_GetLoadoutSlot("CTFItemDefinition::GetLoadoutSlot");


IMPL_DATAMAP(short,          CEconItemView, m_iItemDefinitionIndex);
IMPL_DATAMAP(int,            CEconItemView, m_iEntityQuality);
IMPL_DATAMAP(int,            CEconItemView, m_iEntityLevel);
IMPL_DATAMAP(int64,          CEconItemView, m_iItemID);
IMPL_DATAMAP(bool,           CEconItemView, m_bInitialized);
IMPL_DATAMAP(CAttributeList, CEconItemView, m_AttributeList);
IMPL_DATAMAP(CAttributeList, CEconItemView, m_NetworkedDynamicAttributesForDemos);
IMPL_DATAMAP(bool,           CEconItemView, m_bOnlyIterateItemViewAttributes);
IMPL_SENDPROP(uint32,        CEconItemView, m_iAccountID, CEconEntity);

MemberFuncThunk<      CEconItemView *, void>                              CEconItemView::ft_ctor         ("CEconItemView::CEconItemView [C1]");
MemberFuncThunk<      CEconItemView *, void, int, int, int, unsigned int> CEconItemView::ft_Init         ("CEconItemView::Init");
MemberFuncThunk<const CEconItemView *, CTFItemDefinition *>               CEconItemView::ft_GetStaticData("CEconItemView::GetStaticData");
MemberFuncThunk<const CEconItemView *, CEconItem *>                       CEconItemView::ft_GetSOCData   ("CEconItemView::GetSOCData");
MemberFuncThunk<const CEconItemView *, const char *, int, int>            CEconItemView::ft_GetPlayerDisplayModel("CEconItemView::GetPlayerDisplayModel");

MemberVFuncThunk<const CEconItemView *, int> CEconItemView::vt_GetItemDefIndex(TypeName<CEconItemView>(), "CEconItemView::GetItemDefIndex");


IMPL_SENDPROP(CEconItemView, CAttributeContainer, m_Item, CEconEntity);


void CEconItemAttributeDefinition::ConvertValueToString(attribute_data_union_t& value, char *buf, size_t buf_len)
{
	/* if BConvertStringToEconAttributeValue was called with b1 = true, then
	 * calling ConvertEconAttributeValueToString will render the stored-as-float
	 * value as an integer, which looks horribly wrong */
	if (this->IsType<CSchemaAttributeType_Default>()) {
		if (this->IsStoredAsInteger())
			snprintf(buf, buf_len, "%d", RoundFloatToInt(value.m_Float));
		else
			snprintf(buf, buf_len, "%f", value.m_Float);
	}
	else if (this->IsType<CSchemaAttributeType_Float>()) {
		snprintf(buf, buf_len, "%f", value.m_Float);
	}
	else if (this->IsType<CSchemaAttributeType_String>()) {
		const char *pstr;
		CopyStringAttributeValueToCharPointerOutput(value.m_String, &pstr);
		V_strncpy(buf, pstr, buf_len);
	}
	
	//void *str = strcompat_alloc();
	//this->GetType()->ConvertEconAttributeValueToString(this, value, &str/*reinterpret_cast<std::string *>(str)*/);
	//strcompat_get(str, buf, buf_len);
	//strcompat_free(str);
}


MemberFuncThunk<      CEconItemAttribute *, void>                           CEconItemAttribute::ft_ctor         ("CEconItemAttribute::CEconItemAttribute [C1]");
MemberFuncThunk<const CEconItemAttribute *, CEconItemAttributeDefinition *> CEconItemAttribute::ft_GetStaticData("CEconItemAttribute::GetStaticData");



MemberFuncThunk<const CEconItemSchema *, CEconItemDefinition *, int>                   CEconItemSchema::ft_GetItemDefinition           ("CEconItemSchema::GetItemDefinition");
MemberFuncThunk<const CEconItemSchema *, CEconItemDefinition *, const char *>          CEconItemSchema::ft_GetItemDefinitionByName     ("CEconItemSchema::GetItemDefinitionByName");
MemberFuncThunk<const CEconItemSchema *, CEconItemAttributeDefinition *, int>          CEconItemSchema::ft_GetAttributeDefinition      ("CEconItemSchema::GetAttributeDefinition");
MemberFuncThunk<const CEconItemSchema *, CEconItemAttributeDefinition *, const char *> CEconItemSchema::ft_GetAttributeDefinitionByName("CEconItemSchema::GetAttributeDefinitionByName");
MemberFuncThunk<CEconItemSchema *, bool, KeyValues *, CUtlVector<CUtlString> *>        CEconItemSchema::ft_BInitAttributes			   ("CEconItemSchema::BInitAttributes");
MemberFuncThunk<CEconItemSchema *, void, int, int, KeyValues *>                        CEconItemSchema::ft_ItemTesting_CreateTestDefinition("CEconItemSchema::ItemTesting_CreateTestDefinition");


static StaticFuncThunk<CTFItemSchema *> ft_GetItemSchema("GetItemSchema");
CTFItemSchema *GetItemSchema() { return ft_GetItemSchema(); }

MemberFuncThunk<CEconItem *, attribute_t &>                       CEconItem::ft_AddDynamicAttributeInternal("CEconItem::AddDynamicAttributeInternal");


static StaticFuncThunk<CItemGeneration *> ft_ItemGeneration("ItemGeneration");
CItemGeneration *ItemGeneration() { return ft_ItemGeneration(); }

MemberFuncThunk<CItemGeneration *, CBaseEntity *, CEconItemView const*, Vector const&, QAngle const&, char const*> CItemGeneration::ft_SpawnItem("CItemGeneration::SpawnItem");
MemberFuncThunk<CItemGeneration *, CBaseEntity *, int, Vector const&, QAngle const&, int, int, char const*> CItemGeneration::ft_SpawnItem_defid("CItemGeneration::SpawnItem [defIndex]");
MemberFuncThunk<CItemGeneration *, CBaseEntity *, CEconItemView const*, Vector const&, QAngle const&, char const*> CItemGeneration::ft_GenerateItemFromScriptData("CItemGeneration::GenerateItemFromScriptData");

StaticFuncThunk<void, const CAttribute_String *, const char **> ft_CopyStringAttributeValueToCharPointerOutput("CopyStringAttributeValueToCharPointerOutput");

GlobalThunk<const char *[NUM_VISUALS_BLOCKS]> g_TeamVisualSections("g_TeamVisualSections");


static const auto& GetLoadoutSlotNameMap()
{
	static std::map<loadout_positions_t, std::string> s_Map;
	
	if (s_Map.empty()) {
		for (int i = LOADOUT_POSITION_INVALID; i < LOADOUT_POSITION_COUNT; ++i) {
			auto slot = static_cast<loadout_positions_t>(i);
			
			s_Map.emplace(slot, boost::algorithm::to_lower_copy(std::string(GetLoadoutPositionName(i) + strlen("LOADOUT_POSITION_"))));
		}
	}
	
	return s_Map;
}


int GetNumberOfLoadoutSlots()
{
	return LOADOUT_POSITION_COUNT;
}

bool IsValidLoadoutSlotNumber(int num)
{
	return (num > LOADOUT_POSITION_INVALID && num < LOADOUT_POSITION_COUNT);
}

loadout_positions_t ClampLoadoutSlotNumber(int num)
{
	return static_cast<loadout_positions_t>(Clamp(num, LOADOUT_POSITION_INVALID + 1, LOADOUT_POSITION_COUNT - 1));
}


const char *GetLoadoutSlotName(loadout_positions_t slot)
{
//	if (!IsValidLoadoutSlotNumber(slot)) {
//		return nullptr;
//	}
	
	return GetLoadoutSlotNameMap().at(slot).c_str();
}

loadout_positions_t GetLoadoutSlotByName(const char *name)
{
	const auto& map = GetLoadoutSlotNameMap();
	
	for (int i = LOADOUT_POSITION_INVALID + 1; i < LOADOUT_POSITION_COUNT; ++i) {
		auto slot = static_cast<loadout_positions_t>(i);
		
		if (FStrEq(map.at(slot).c_str(), name)) {
			return slot;
		}
	}
	
	return LOADOUT_POSITION_INVALID;
}


static StaticFuncThunk<const char *, loadout_positions_t> ft_GetLoadoutPositionName("GetLoadoutPositionName");
const char *GetLoadoutPositionName(loadout_positions_t slot) { return ft_GetLoadoutPositionName(slot); }

static StaticFuncThunk<loadout_positions_t, const char *> ft_GetLoadoutPositionByName("GetLoadoutPositionByName");
loadout_positions_t GetLoadoutPositionByName(const char *name) { return ft_GetLoadoutPositionByName(name); }


MemberVFuncThunk<      CPlayerInventory *, void, bool> CPlayerInventory::vt_DumpInventoryToConsole(TypeName<CPlayerInventory>(), "CPlayerInventory::DumpInventoryToConsole");
MemberVFuncThunk<const CPlayerInventory *, int>        CPlayerInventory::vt_GetMaxItemCount       (TypeName<CPlayerInventory>(), "CPlayerInventory::GetMaxItemCount");

MemberFuncThunk< CPlayerInventory *, CEconItemView *, int, int *>         CPlayerInventory::ft_GetItemByPosition("CPlayerInventory::_GetItemByPosition");


MemberFuncThunk<CTFInventoryManager *, CTFPlayerInventory *, const CSteamID&> CTFInventoryManager::ft_GetInventoryForPlayer("CTFInventoryManager::GetInventoryForPlayer");
MemberFuncThunk<CTFInventoryManager *, CEconItemView *, int, int> CTFInventoryManager::ft_GetBaseItemForClass("CTFInventoryManager::GetBaseItemForClass");


static StaticFuncThunk<CInventoryManager *> ft_InventoryManager("InventoryManager");
CInventoryManager *InventoryManager() { return ft_InventoryManager(); }

static StaticFuncThunk<CTFInventoryManager *> ft_TFInventoryManager("TFInventoryManager");
CTFInventoryManager *TFInventoryManager() { return ft_TFInventoryManager(); }

bool LoadAttributeDataUnionFromString(CEconItemAttributeDefinition *attr_def, attribute_data_union_t &value, std::string &value_str)
{
	//Pool of previously added string values
	static std::map<std::string, attribute_data_union_t> attribute_string_values;

	auto entry = attribute_string_values.find(value_str);

	if (entry == attribute_string_values.end()) {
		//const char *value_cstr = STRING(AllocPooledString(value_str.c_str()));
		
		attr_def->GetType()->InitializeNewEconAttributeValue(&value);
		if (!attr_def->GetType()->BConvertStringToEconAttributeValue(attr_def, value_str.c_str(), &value, true)) {
			attr_def->GetType()->UnloadEconAttributeValue(&value);
			return false;
		}
		attribute_string_values[value_str] = value;
	}
	else {
		value = attribute_string_values[value_str];
	}
	return true;
}

void CAttributeList::AddStringAttribute(CEconItemAttributeDefinition *attr_def, std::string value_str)
{
	//this->RemoveAttribute(attr_def);
	attribute_data_union_t value;
	if (LoadAttributeDataUnionFromString(attr_def, value, value_str)) {
		this->SetRuntimeAttributeValue(attr_def, value.m_Float);
	}
	//CEconItemAttribute *attr = CEconItemAttribute::Create(attr_def->GetIndex());
	//*attr->GetValuePtr() = value;
	//this->AddAttribute(attr);
	//CEconItemAttribute::Destroy(attr);
}
float _CallAttribHookRef_Optimize(float value, string_t pszClass, const CBaseEntity *pEntity)
{ 
	if (pEntity != nullptr) {
		CBaseEntity *entity_cast = const_cast<CBaseEntity *>(pEntity);
		CAttributeManager *mgr = nullptr;
		if (pEntity->IsPlayer()) {
			mgr = reinterpret_cast<CTFPlayer *>(entity_cast)->GetAttributeManager();
		}
		else if (pEntity->IsBaseCombatWeapon() || pEntity->IsWearable()) {
			mgr = reinterpret_cast<CEconEntity *>(entity_cast)->GetAttributeManager();
		}
		if (mgr != nullptr) {
			return mgr->ApplyAttributeFloatWrapperFunc(value, entity_cast, pszClass);
		}
	}
	return value;
}

void CAttributeList::SetRuntimeAttributeValueByDefID(int def_idx, float value)
{
	this->SetRuntimeAttributeValue(GetItemSchema()->GetAttributeDefinition(def_idx), value);
	/*const int iAttributes = this->Attributes().Count();
	for ( int i = 0; i < iAttributes; i++ )
	{
		CEconItemAttribute &pAttribute = this->Attributes()[i];

		if ( pAttribute.m_iAttributeDefinitionIndex == def_idx)
		{
			if (pAttribute.m_iRawValue32.m_Float != value) {
				pAttribute.m_iRawValue32.m_Float = value;
				this->NotifyManagerOfAttributeValueChanges();
			}
			return;
		}
	}
	CEconItemAttribute attribute(def_idx, value);

	this->Attributes().AddToTail( attribute );
	this->NotifyManagerOfAttributeValueChanges();*/
}

void CAttributeList::RemoveAttributeByDefID(int def_idx)
{
	this->RemoveAttribute(GetItemSchema()->GetAttributeDefinition(def_idx));
	/*const int iAttributes = this->Attributes().Count();
	for ( int i = 0; i < iAttributes; i++ )
	{
		CEconItemAttribute &pAttribute = this->Attributes()[i];

		if ( pAttribute.m_iAttributeDefinitionIndex == def_idx )
		{
			this->RemoveAttributeByIndex(i);
			return;
		}
	}*/
}