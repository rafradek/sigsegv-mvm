/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/econ/econ_item_schema.h
 */


// TODO: GEconItemSchema
// TODO: GetTeamVisualsFromString

// TODO: struct/class static_attrib_t

// sizeof: 0x174
class CEconItemDefinition
{
public:
	CEconItemDefinition();
	virtual ~CEconItemDefinition();
	
	// TODO
	
private:
	// 004 KeyValues *
	// 008 uint16_t m_iItemDefIndex
	// 00a bool m_bEnabled
	// 00b uint8_t m_iMinILevel
	// 00c uint8_t m_iMaxILevel
	// 00d?
	// 00e? 
	// 00f? 
	// 010 uint16_t m_iDefaultDropQuantity
	// 014 CUtlVector<static_attrib_t> m_Attributes
	// 028 uint8_t m_iPopularitySeed
	// 02c const char *m_pszItemName
	// 030 bool m_bProperName
	// 031? 
	// 032? 
	// 033? 
	// 034 const char *m_pszItemTypeName
	// 038 const char *m_pszItemDescription
	// 03c 
	// 040 const char *m_pszModelInventory
	// 044 const char *m_pszImageInventory
	// 048 CUtlVector<const char *> m_ImageInventoryOverlays
	// 05c int m_iImageInventoryPosX
	// 060 int m_iImageInventoryPosY
	// 064 int m_iImageInventorySizeW
	// 068 int m_iImageInventorySizeH
	// 06c int m_iInspectPanelDist
	// 070 const char *m_pszModelPlayer
	// 074 bool m_bLoadOnDemand
	// 075 
	// 076 bool m_bHideBodygroupsDeployedOnly
	// 077? 
	// 078 const char *m_pszModelWorld
	// 07c const char *m_pszExtraWearable
	// 080 const char *m_pszExtraWearableVM
	// 084 const char *m_pszModelVisionFiltered
	// 088 const char *m_pszCollectionReference
	// 08c bool m_bAttachToHands
	// 08d bool m_bAttachToHandsVMOnly
	// 08e bool m_bFlipViewModel
	// 08f bool m_bActAsWearable
	// 090 bool m_bActAsWeapon
	// 091 bool m_bIsTool
	// 092? 
	// 093? 
	// 094 
	// 098 
	// 09c 
	// 0a0 array of 5x new'd ptr to big structure (related to visual blocks, sizeof is 0x130)
	// 0b4 const char *m_pszBrassEjectModel
	// 0b8 IEconTool *
	// 0bc CUtlVector<CEconItemDefinition *> *, related to bundles
	// 0c0 
	// 0c4 CUtlString
	// 0c8 
	// 0cc const char *m_pszItemClass
	// 0d0 const char *m_pszItemLogName
	// 0d4 const char *m_pszItemIconName
	// 0d8 const char *m_pszName
	// 0dc const char *m_pszDatabaseAuditTable
	// 0e0 bool m_bHidden
	// 0e1 bool m_bShowInArmory
	// 0e2 bool m_bBaseItem
	// 0e3 bool m_bImportFrom
	// 0e4 bool m_bIsPackBundle
	// 0e5? 
	// 0e6? 
	// 0e7? 
	// 0e8 
	// 0ec 
	// 0f0 const char *m_pszArmoryDesc
	// 0f4 const char *m_pszXifierClassRemap
	// 0f8 const char *m_pszBaseItemName
	// 0fc const char *m_pszParticleSuffix
	// 100 
	// 104 
	// 108 const char *m_pszArmoryRemap
	// 10c const char *m_pszStoreRemap
	// 110 const char *m_pszClassTokenID
	// 114 const char *m_pszSlotTokenID
	// 118 int m_iDropType
	// 11c const char *m_pszHolidayRestriction
	// 120 int m_iVisionFilterFlags
	// 124 int m_iSubType
	// 128 
	// 12c int m_iEquipRegionMask1
	// 130 int m_iEquipRegionMask2
	// 134 uint16_t m_iRemapIndex
	// 138 CUtlVector<int> m_Tags
	// 14c CUtlVector<const CEconItemDefinition *>
	// 160 CUtlVector<unsigned int>
};

// sizeof: 0x3c
class CEconItemAttributeDefinition
{
public:
	CEconItemAttributeDefinition();
	
	bool BInitFromKV(KeyValues *kv, CUtlVector<CUtlString> *errors);
	
	// TODO
	
private:
	KeyValues *m_KeyValues;                // +0x00
	unsigned short m_iIndex;               // +0x04
	ISchemaAttributeType *m_AttributeType; // +0x08
	bool m_bHidden;                        // +0x0c
	bool m_bForceOutputDescription;        // +0x0d
	bool m_bStoreAsInteger;                // +0x0e
	bool m_bInstanceData;                  // +0x0f
	bool m_bIsSetBonus;                    // +0x10
	int m_iIsUserGenerated;                // +0x14
	int m_iEffectType;                     // +0x18
	int m_iDescriptionFormat;              // +0x1c
	char *m_pszDescriptionString;          // +0x20
	char *m_pszArmoryDesc;                 // +0x24
	char *m_pszName;                       // +0x28
	char *m_pszAttributeClass;             // +0x2c
	bool m_bCanAffectMarketName;           // +0x30
	bool m_bCanAffectRecipeCompName;       // +0x31
	int m_nTagHandle;                      // +0x34
	// 38 dword
};

// TODO: class CEconItemSeriesDefinition
// TODO: class CEconItemQualityDefinition
// TODO: class CEconItemRarityDefinition
// TODO: class CEconItemSetDefinition
// TODO: class CEconItemCollectionDefinition
// TODO: class CEconItemPaintKitDefinition
// TODO: class CEconColorDefinition
// TODO: class CEconCraftingRecipeDefinition
// TODO: class CEconOperationDefinition
// TODO: class CEconLootListDefinition
// TODO: class CQuestObjectiveDefinition
// TODO: class CTimedItemRewardDefinition
// TODO: class CItemLevelingDefinition

// TODO: class CEconItemSchema

// TODO: static CSchemaFieldHandle<CEconItemAttributeDefinition> CAttributeLineItemLootList::s_pAttrDef_RandomDropLineItems[]
// TODO: static CSchemaFieldHandle<CEconItemAttributeDefinition> CAttributeLineItemLootList::s_pAttrDef_RandomDropLineItemFooterDesc

// TODO: CheckValveSignature
