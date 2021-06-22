/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/econ/econ_item_view.h
 */


// sizeof: 0x10
// DT_ScriptCreatedAttribute
class CEconItemAttribute
{
public:
	CEconItemAttribute();
	CEconItemAttribute(unsigned short i1, float f1);
	CEconItemAttribute(unsigned short i1, unsigned int i2);
	
	operator=(const CEconItemAttribute& that);
	
	UNKNOWN GetStaticData() const;
	UNKNOWN Init();
	UNKNOWN SetIntValue(unsigned int i1);
	UNKNOWN SetFloatValue(float f1);
	
private:
	CNetworkVar(unsigned short, m_iAttributeDefinitionIndex); // +0x04
	CNetworkVar(int,            m_iRawValue32);               // +0x08
	CNetworkVar(int,            m_nRefundableCurrency);       // +0x0c
};


// sizeof: 0x1c
// DT_AttributeList
class CAttributeList
{
public:
	CAttributeList();
	~CAttributeList();
	
	operator=(const CAttributeList& that);
	
	UNKNOWN AddAttribute(CEconItemAttribute *attr);
	UNKNOWN DestroyAllAttributes();
	UNKNOWN GetAttributeByID(int id) const;
	UNKNOWN GetAttributeByName(const char *name) const;
	UNKNOWN GetRuntimeAttributeRefundableCurrency(const CEconItemAttributeDefinition *def) const;
	UNKNOWN Init();
	UNKNOWN IterateAttributes(IEconItemAttributeIterator *it) const;
	UNKNOWN NotifyManagerOfAttributeValueChanges();
	UNKNOWN RemoveAttribute(const CEconItemAttributeDefinition *def);
	UNKNOWN RemoveAttributeByIndex(int index);
	UNKNOWN SetManager(CAttributeManager *manager);
	UNKNOWN SetRuntimeAttributeRefundableCurrency(const CEconItemAttributeDefinition *def, int value);
	UNKNOWN SetRuntimeAttributeValue(const CEconItemAttributeDefinition *def, float value);
	
private:
	CUtlVector<CEconItemAttribute> m_Attributes; // +0x04
	CAttributeManager *m_Manager;                // +0x18
};


// sizeof: 0xac
// DT_ScriptCreatedItem
class CEconItemView : public IEconItemInterface
{
public:
	// TODO: ctor
	virtual ~CEconItemView();
	
	/* misspelling in original */
	virtual UNKNOWN GetCustomPainkKitDefinition() const override;
	virtual UNKNOWN GetItemDefinition() const override;
	virtual UNKNOWN GetID() const override;
	virtual UNKNOWN GetAccountID() const override;
	virtual UNKNOWN GetQuality() const override;
	virtual UNKNOWN GetStyle() const override;
	virtual UNKNOWN GetFlags() const override;
	virtual UNKNOWN GetOrigin() const override;
	virtual UNKNOWN GetQuantity() const override;
	virtual UNKNOWN GetItemLevel() const override;
	virtual UNKNOWN GetInUse() const override;
	virtual UNKNOWN GetCustomName() const override;
	virtual UNKNOWN GetCustomDesc() const override;
	virtual UNKNOWN IterateAttributes(IEconItemAttributeIterator *it) const override;
	virtual UNKNOWN GetItemDefIndex() const override;
	
	// TODO
	
private:
	CNetworkVar(unsigned short, m_iItemDefinitionIndex);           // +0x04
	CNetworkVar(int,            m_iEntityQuality);                 // +0x08
	CNetworkVar(int,            m_iEntityLevel);                   // +0x0c
	uint64_t m_iItemID;                                            // +0x10
	CNetworkVar(unsigned int,   m_iItemIDHigh);                    // +0x18
	CNetworkVar(unsigned int,   m_iItemIDLow);                     // +0x1c
	CNetworkVar(unsigned int,   m_iAccountID);                     // +0x20
	// 24 
	// 28 CEconItemHandle
	// 40 
	// 44 
	// 48 
	// 4c 
	// 50 
	// 54 
	// 58 
	// 5c 
	// 60 
	// 64 
	CNetworkVar(int,            m_iTeamNumber);                    // +0x68
	CNetworkVar(bool,           m_bInitialized);                   // +0x6c
	CNetworkVarEmbedded(CAttributeList, m_AttributeList);          // +0x70
	CNetworkVarEmbedded(CAttributeList, m_NetworkedDynamicAttributesForDemos); // +0x8c
	CNetworkVar(bool,           m_bOnlyIterateItemViewAttributes); // +0xa8
};





// C_EconItemView @ osx client 20151223


// sizeof: 0xe0
class C_EconItemView
{
	// 00 vtable1
	// 04 vtable2 CDefaultClientRenderable
	// 08 
	// 0c vtable3 CMaterialOverrideContainer<IEconItemInterface>
	// 10 CMaterialReference
	// 14 CMaterialReference
	// 18 CMaterialReference
	// 1c CMaterialReference
	// 20 m_iItemDefinitionIndex
	// 24 
	// 28 m_iItemLevel
	// 2c 
	// 30 
	// 34 m_iItemIDHigh
	// 38 m_iItemIDLow
	// 3c m_iAccountID
	// 40 
	// 44 CEconItemHandle
	
	// 94 dword
	// 98 byte
	// 
	
	// a4 m_AttributeList
	// c0 m_NetworkedDynamicAttributesForDemos
	// dc byte
};


// sizeof: 0x28
struct Something
{
	// 00 dword
	// 04 dword
	// 08 dword
	// 0c dword
	// 10 dword
	// 14 dword
	// 18 byte
	// 1c dword CHandle<T>?
	// 20 dword CHandle<T>?
	// 24 dword CHandle<T>?
};
// @ CHudUpgradePanel+0x1e0
// @ CHudUpgradePanel+0x208
// @ CHudUpgradePanel+0x230
// @ CHudUpgradePanel+0x258
// @ CHudUpgradePanel+0x280
// @ CHudUpgradePanel+0x2a8
