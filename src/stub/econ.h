#ifndef _INCLUDE_SIGSEGV_STUB_ECON_H_
#define _INCLUDE_SIGSEGV_STUB_ECON_H_


#include "stub/tf_shareddefs.h"
#include "weapon_parse.h"
#include "stub/ihasattributes.h"


class CEconItem;

namespace Mod::Perf::Attributes_Optimize
{
	float GetAttribValueOptimizedIfEnabled(float value, const char *attr, CBaseEntity *ent, bool isint, bool literalString);
}

class CAttributeManager
{
public:
	void NetworkStateChanged()           { this->m_hOuter->NetworkStateChanged(); }
	void NetworkStateChanged(void *pVar) { this->m_hOuter->NetworkStateChanged(pVar); }
	
	
	float ApplyAttributeFloatWrapper(float flValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList = nullptr) { return vt_ApplyAttributeFloatWrapper(this, flValue, pInitiator, iszAttribHook, pItemList);}
	float ApplyAttributeFloat( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList = NULL ) { return vt_ApplyAttributeFloat(this, flValue, pInitiator, iszAttribHook, pItemList);}
	string_t ApplyAttributeStringWrapper( string_t strValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList = NULL ) { return ft_ApplyAttributeStringWrapper(this, strValue, pInitiator, iszAttribHook, pItemList);}
	string_t ApplyAttributeString( string_t strValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList = NULL ) { return vt_ApplyAttributeString(this, strValue, pInitiator, iszAttribHook, pItemList);}
	int GetGlobalCacheVersion( ) const                                  { return ft_GetGlobalCacheVersion(this);}
	void ClearCache() { ft_ClearCache(this);}

	void AddProvider(CBaseEntity *provider)    {        ft_AddProvider(this, provider);}
	void RemoveProvider(CBaseEntity *provider) {        ft_RemoveProvider(this, provider);}
	bool IsProvidingTo(CBaseEntity *receiver)  { return ft_IsProvidingTo(this, receiver);}

	float ApplyAttributeFloatWrapperFunc(float flValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList = nullptr) { return ft_ApplyAttributeFloatWrapper(this, flValue, pInitiator, iszAttribHook, pItemList);}

	
	template<typename T>
	static T AttribHookValue(T value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec = nullptr, bool literalString = true);

	template<typename T>
	static T AttribHookValue(T value, string_t attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec = nullptr);


	//The function is virtual originally, but since there are no derivates it should be safe to make it a regular function link, for speed
	static MemberFuncThunk<CAttributeManager *, float, float, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> ft_ApplyAttributeFloatWrapper;
	static MemberFuncThunk<CAttributeManager *, string_t, string_t, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> ft_ApplyAttributeStringWrapper;
	static MemberFuncThunk<const CAttributeManager *, int> ft_GetGlobalCacheVersion;
	static MemberFuncThunk<CAttributeManager *, void> ft_ClearCache;
	static MemberFuncThunk<CAttributeManager *, void, CBaseEntity *> ft_AddProvider;
	static MemberFuncThunk<CAttributeManager *, void, CBaseEntity *> ft_RemoveProvider;
	static MemberFuncThunk<CAttributeManager *, bool, CBaseEntity *> ft_IsProvidingTo;
	
	static MemberVFuncThunk<CAttributeManager *, string_t, string_t, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> vt_ApplyAttributeStringWrapper;
	static MemberVFuncThunk<CAttributeManager *, float, float, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> vt_ApplyAttributeFloatWrapper;
	static MemberVFuncThunk<CAttributeManager *, string_t, string_t, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> vt_ApplyAttributeString;
	static MemberVFuncThunk<CAttributeManager *, float, float, CBaseEntity *, string_t, CUtlVector<CBaseEntity*> *> vt_ApplyAttributeFloat;

	static StaticFuncThunk<int, int, const char *, const CBaseEntity *, CUtlVector<CBaseEntity *> *, bool>     ft_AttribHookValue_int;
	static StaticFuncThunk<float, float, const char *, const CBaseEntity *, CUtlVector<CBaseEntity *> *, bool> ft_AttribHookValue_float;

	struct cached_attribute_t
	{
		string_t attrib;
		float in;
		float out;
	};

	DECL_SENDPROP(CHandle<CBaseEntity>, m_hOuter);
	DECL_SENDPROP(int, m_iReapplyProvisionParity);
	DECL_RELATIVE(CUtlVector<CHandle<CBaseEntity>>, m_Receivers);
	DECL_RELATIVE(CUtlVector<CHandle<CBaseEntity>>, m_Providers);
	DECL_RELATIVE(CUtlVector<cached_attribute_t>, m_CachedResults);
};

template<> inline int CAttributeManager::AttribHookValue<int>(int value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool literalString)
{ 
	if (ent == nullptr)
		return value;

	return RoundFloatToInt(Mod::Perf::Attributes_Optimize::GetAttribValueOptimizedIfEnabled(value, attr, const_cast<CBaseEntity *>(ent), true, literalString));
	//
}
template<> inline float CAttributeManager::AttribHookValue<float>(float value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool literalString)
{ 
	if (ent == nullptr)
		return value;
		
	return Mod::Perf::Attributes_Optimize::GetAttribValueOptimizedIfEnabled(value, attr, const_cast<CBaseEntity *>(ent), false, literalString);
	//return CAttributeManager::ft_AttribHookValue_float(value, attr, ent, vec, b1); 
}

template<typename T> inline void _CallAttribHookRef(T& value, const char *pszClass, const CBaseEntity *pEntity) { value = CAttributeManager::AttribHookValue<T>(value, pszClass, pEntity); }
#define CALL_ATTRIB_HOOK_INT(                value, name) _CallAttribHookRef<int>  (value, #name, this)
#define CALL_ATTRIB_HOOK_INT_ON_OTHER(  ent, value, name) _CallAttribHookRef<int>  (value, #name, ent )
#define CALL_ATTRIB_HOOK_FLOAT(              value, name) _CallAttribHookRef<float>(value, #name, this)
#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(ent, value, name) _CallAttribHookRef<float>(value, #name, ent )

class CAttribute_String                 {};
class CAttribute_ItemSlotCriteria       {};
class CAttribute_DynamicRecipeComponent {};
class CAttribute_WorldItemPlacement     {};


#if 0
class attribute_data_union_t
{
public:
	attribute_data_union_t() : m_UInt(0) {}
	
	union
	{
		unsigned int                       m_UInt;
		float                              m_Float;
		uint64                            *m_UInt64;
		CAttribute_String                 *m_String;
		CAttribute_ItemSlotCriteria       *m_ItemSlotCriteria;
		CAttribute_DynamicRecipeComponent *m_DynamicRecipeComponent;
		CAttribute_WorldItemPlacement     *m_WorldItemPlacement;
	};
};
#else
union attribute_data_union_t
{
	unsigned int                       m_UInt;
	float                              m_Float;
	uint64                            *m_UInt64;
	CAttribute_String                 *m_String;
	CAttribute_ItemSlotCriteria       *m_ItemSlotCriteria;
	CAttribute_DynamicRecipeComponent *m_DynamicRecipeComponent;
	CAttribute_WorldItemPlacement     *m_WorldItemPlacement;
};
#endif
static_assert(sizeof(attribute_data_union_t) == sizeof(void *));

struct static_attrib_t
{
	unsigned short m_iAttrIndex;    // +0x00
	attribute_data_union_t m_Value; // +0x04 / +0x08
};
static_assert(sizeof(static_attrib_t) == 0x8);

typedef unsigned short attrib_definition_index_t;
typedef unsigned short	item_definition_index_t;
typedef uint32_t attrib_value_t;
const attrib_definition_index_t INVALID_ATTRIB_DEF_INDEX = ((attrib_definition_index_t)-1);
const item_definition_index_t INVALID_ITEM_DEF_INDEX = ((item_definition_index_t)-1);

struct attribute_t
{
	attrib_definition_index_t m_unDefinitionIndex;			// stored as ints here for memory efficiency on the GC
	attribute_data_union_t m_value;
};

class CEconItemAttributeDefinition;


class IEconItemAttributeIterator
{
public:
	virtual ~IEconItemAttributeIterator() = default;
	
	virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, unsigned int                             value) const = 0;
	virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, float                                    value) const = 0;
	virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const uint64&                            value) const = 0;
	virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String&                 value) const = 0;
	virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value) const = 0;
	virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_ItemSlotCriteria&       value) const = 0;
	virtual bool OnIterateAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const CAttribute_WorldItemPlacement&     value) const = 0;
};


class ISchemaAttributeType
{
	// TODO: use vfunc thunks instead of relying on vtable matching up forever
protected:
	virtual ~ISchemaAttributeType() { assert(false); }
public:
	virtual int GetTypeUniqueIdentifier() const = 0;
	virtual void LoadEconAttributeValue(CEconItem *pItem, const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value) const = 0;
	virtual std::string *ConvertEconAttributeValueToByteStream(const attribute_data_union_t& value, std::string *pString) const = 0;
	virtual bool BConvertStringToEconAttributeValue(const CEconItemAttributeDefinition *pAttrDef, const char *pString, attribute_data_union_t *pValue, bool b1 = true) const = 0;
	virtual std::string *ConvertEconAttributeValueToString(const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *pString) const = 0;
	virtual void LoadByteStreamToEconAttributeValue(CEconItem *pItem, const CEconItemAttributeDefinition *pAttrDef, const std::string& string) const = 0;
	virtual void InitializeNewEconAttributeValue(attribute_data_union_t *pValue) const = 0;
	virtual void UnloadEconAttributeValue(attribute_data_union_t *pValue) const = 0;
	virtual bool OnIterateAttributeValue(IEconItemAttributeIterator *pIterator, const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value) const = 0;
	virtual bool BSupportsGameplayModificationAndNetworking() const = 0;
};

template<typename T> class ISchemaAttributeTypeBase : public ISchemaAttributeType
{
public:
	virtual void ConvertTypedValueToByteStream(const T& value, std::string *pString) const = 0;
	virtual void ConvertByteStreamToTypedValue(const std::string& string, T *pValue) const = 0;
};

template<typename T> class CSchemaAttributeTypeBase : public ISchemaAttributeTypeBase<T> {};

class CSchemaAttributeType_Default : public CSchemaAttributeTypeBase<unsigned int> {};
class CSchemaAttributeType_Float   : public CSchemaAttributeTypeBase<float>        {};
class CSchemaAttributeType_UInt64  : public CSchemaAttributeTypeBase<uint64>       {};

template<typename T> class CSchemaAttributeTypeProtobufBase : public CSchemaAttributeTypeBase<T> {};

class CSchemaAttributeType_String                            : public CSchemaAttributeTypeProtobufBase<CAttribute_String>                 {};
class CSchemaAttributeType_ItemSlotCriteria                  : public CSchemaAttributeTypeProtobufBase<CAttribute_ItemSlotCriteria>       {};
class CSchemaAttributeType_DynamicRecipeComponentDefinedItem : public CSchemaAttributeTypeProtobufBase<CAttribute_DynamicRecipeComponent> {};
class CSchemaAttributeType_WorldItemPlacement                : public CSchemaAttributeTypeProtobufBase<CAttribute_WorldItemPlacement>     {};


struct attr_type_t
{
	CUtlConstString m_strName;
	ISchemaAttributeType *m_pType;
};
static_assert(sizeof(attr_type_t) == 0x8);

bool LoadAttributeDataUnionFromString(const CEconItemAttributeDefinition *attr_def, attribute_data_union_t &value, const std::string &value_str);

class CEconItemAttribute;

class CAttributeList
{
public:
	CEconItemAttribute *GetAttributeByID(int def_idx) const            { return ft_GetAttributeByID      (this, def_idx); }
	CEconItemAttribute *GetAttributeByName(const char *name) const     { return ft_GetAttributeByName    (this, name); }
	void IterateAttributes(IEconItemAttributeIterator *iter) const     {        ft_IterateAttributes     (this, iter); }
	void AddAttribute(CEconItemAttribute *pAttr)                       {        ft_AddAttribute          (this, pAttr); }
	void RemoveAttribute(const CEconItemAttributeDefinition *pAttrDef) {        ft_RemoveAttribute       (this, pAttrDef); }
	void RemoveAttributeByIndex(int index)                             {        ft_RemoveAttributeByIndex(this, index); }
	void DestroyAllAttributes()                                        {        ft_DestroyAllAttributes  (this); }
	void SetRuntimeAttributeValue(CEconItemAttributeDefinition *pAttrDef, float value) {        ft_SetRuntimeAttributeValue  (this, pAttrDef, value); }
	void SetRuntimeAttributeRefundableCurrency(CEconItemAttributeDefinition *pAttrDef, float value) {        ft_SetRuntimeAttributeRefundableCurrency  (this, pAttrDef, value); }
	void NotifyManagerOfAttributeValueChanges()                        {        ft_NotifyManagerOfAttributeValueChanges  (this); }

	CUtlVector<CEconItemAttribute>& Attributes() { return this->m_Attributes; }
	void AddStringAttribute(CEconItemAttributeDefinition *pAttrDef, std::string value);
	void SetRuntimeAttributeValueByDefID(int def_idx, float value);
	void RemoveAttributeByDefID(int def_idx);
	CAttributeManager *GetManager() { return this->m_pManager; }
	
private:
	int vtable;
	CUtlVector<CEconItemAttribute> m_Attributes; // TODO: should be a netvar!
	CAttributeManager *m_pManager;
	
	static inline MemberFuncThunk<const CAttributeList *, CEconItemAttribute *, int>                  ft_GetAttributeByID      { "CAttributeList::GetAttributeByID"       };
	static inline MemberFuncThunk<const CAttributeList *, CEconItemAttribute *, const char *>         ft_GetAttributeByName    { "CAttributeList::GetAttributeByName"     };
	static inline MemberFuncThunk<const CAttributeList *, void, IEconItemAttributeIterator *>         ft_IterateAttributes     { "CAttributeList::IterateAttributes"      };
	static inline MemberFuncThunk<      CAttributeList *, void, CEconItemAttribute *>                 ft_AddAttribute          { "CAttributeList::AddAttribute"           };
	static inline MemberFuncThunk<      CAttributeList *, void, const CEconItemAttributeDefinition *> ft_RemoveAttribute       { "CAttributeList::RemoveAttribute"        };
	static inline MemberFuncThunk<      CAttributeList *, void, int>                                  ft_RemoveAttributeByIndex{ "CAttributeList::RemoveAttributeByIndex" };
	static inline MemberFuncThunk<      CAttributeList *, void>                                       ft_DestroyAllAttributes  { "CAttributeList::DestroyAllAttributes"   };
	static inline MemberFuncThunk<      CAttributeList *, void, CEconItemAttributeDefinition *, float>ft_SetRuntimeAttributeValue  { "CAttributeList::SetRuntimeAttributeValue"   };
	static inline MemberFuncThunk<      CAttributeList *, void, CEconItemAttributeDefinition *, float>ft_SetRuntimeAttributeRefundableCurrency  { "CAttributeList::SetRuntimeAttributeRefundableCurrency"   };
	static inline MemberFuncThunk<      CAttributeList *, void>                                       ft_NotifyManagerOfAttributeValueChanges  { "CAttributeList::NotifyManagerOfAttributeValueChanges"   };

};
static_assert(sizeof(CAttributeList) == 0x1c);

// static inline StaticFuncThunk<bool, const CAttributeList *, const CEconItemAttributeDefinition *, float *>    ft_FindAttribute_CAttributeList { "FindAttribute_UnsafeBitwiseCast<uint, float, CAttributeList>" };

// template<> inline bool FindAttribute<float, CAttributeList>(const CAttributeList *attrList, const CEconItemAttributeDefinition *attrDef, float *out)
// { return ft_FindAttribute_CAttributeList(attrList, attrDef, out); }

constexpr int NUM_VISUALS_BLOCKS    = 5;


struct perteamvisuals_t
{
	DECL_EXTRACT(const char *[NUM_SHOOT_SOUND_TYPES], m_Sounds);
};


class CEconItemDefinition
{
public:
	// TODO: MAKE THIS STUFF PRIVATE AND USE ACCESSORS INSTEAD!
	uint32_t vtable;
	KeyValues *m_pKV;
	item_definition_index_t m_iItemDefIndex;
	bool m_bEnabled;
	byte m_iMinILevel;
	byte m_iMaxILevel;
	byte m_iItemQuality;
	byte m_iForcedItemQuality;
	byte m_iItemRarity;
	short m_iDefaultDropQuantity;
	byte m_iItemSeries;
	// ...
	
	DECL_EXTRACT(perteamvisuals_t *[NUM_VISUALS_BLOCKS], m_Visuals);
	
	// END TODO
	
	KeyValues *GetKeyValues() const { return this->m_pKV; }
	
	unsigned int GetEquipRegionBitMask() const { return this->m_nEquipRegionBitMask; }
	unsigned int GetEquipRegionMask()    const { return this->m_nEquipRegionMask;    }
	
	const char *GetItemName (const char *fallback = nullptr) const { return this->GetKVString("item_name",  fallback); }
	const char *GetName     (const char *fallback = nullptr) const { return this->GetKVString("name",       fallback); }
	const char *GetItemClass(const char *fallback = nullptr) const { return this->GetKVString("item_class", fallback); }

	bool BInitFromKV(KeyValues *keyvalues, CUtlVector<CUtlString> *errors) { return ft_BInitFromKV(this, keyvalues, errors); }
	void IterateAttributes(IEconItemAttributeIterator *iterator) const { ft_IterateAttributes(this, iterator); }
	
private:
	const char *GetKVString(const char *str, const char *fallback) const
	{
		if (this->m_pKV == nullptr) return nullptr;
		return this->m_pKV->GetString(str, fallback);
	}
	
	DECL_EXTRACT(unsigned int, m_nEquipRegionBitMask);
	DECL_EXTRACT(unsigned int, m_nEquipRegionMask);

	static MemberFuncThunk<CEconItemDefinition *, bool, KeyValues *, CUtlVector<CUtlString> *> ft_BInitFromKV;
	static MemberFuncThunk<const CEconItemDefinition *, void, IEconItemAttributeIterator *>    ft_IterateAttributes;
};

class CTFItemDefinition : public CEconItemDefinition
{
public:
	int GetLoadoutSlot(int class_idx) const { return ft_GetLoadoutSlot(this, class_idx); }
	
private:
	static MemberFuncThunk<const CTFItemDefinition *, int, int> ft_GetLoadoutSlot;
};

//static inline StaticFuncThunk<bool, const CTFItemDefinition *, const CEconItemAttributeDefinition *, float *> ft_FindAttribute_CTFItemDefinition { "FindAttribute_UnsafeBitwiseCast<uint, float, CTFItemDefinition>" };

//template<> inline bool FindAttribute<float, CTFItemDefinition>(const CTFItemDefinition *attrList, const CEconItemAttributeDefinition *attrDef, float *out)
//{ return ft_FindAttribute_CTFItemDefinition(attrList, attrDef, out); }

class CEconItemView // DT_ScriptCreatedItem
{
public:
	void NetworkStateChanged()           {}
	void NetworkStateChanged(void *pVar) {}
	
	static CEconItemView *Create()
	{
		/* using an arbitrary buffer size that's basically guaranteed to always
		 * be big enough, even if CEconItemView grows in the future */
		auto ptr = reinterpret_cast<CEconItemView *>(::operator new(0x200));
		ft_ctor(ptr);
		return ptr;
	}
	static void Destroy(CEconItemView *ptr)
	{
		if (ptr == nullptr) return;
		
		ptr->~CEconItemView();
		::operator delete(reinterpret_cast<void *>(ptr));
	}
	
	CAttributeList& GetAttributeList() const { return this->m_AttributeList; }
	
	void Init(int iItemDefIndex, int iQuality = 9999, int iLevel = 9999, unsigned int iAccountID = 0) {        ft_Init         (this, iItemDefIndex, iQuality, iLevel, iAccountID); }
	CTFItemDefinition *GetStaticData() const                                                          { return ft_GetStaticData(this); }
	CTFItemDefinition *GetItemDefinition() const                                                      { return ft_GetStaticData(this); }
	CEconItem *GetSOCData() const                                                      				  { return ft_GetSOCData(this); }
	void IterateAttributes(IEconItemAttributeIterator *iter) const                                    {        ft_IterateAttributes     (this, iter); }
	const char *GetPlayerDisplayModel(int classindex, int team) const                                      { return ft_GetPlayerDisplayModel(this, classindex, team); }

	DECL_DATAMAP(item_definition_index_t, m_iItemDefinitionIndex);
	DECL_DATAMAP(int,            m_iEntityQuality);
	DECL_DATAMAP(int,            m_iEntityLevel);
	DECL_DATAMAP(int64,          m_iItemID);
	DECL_DATAMAP(bool,           m_bInitialized);
	DECL_DATAMAP(CAttributeList, m_AttributeList);
	DECL_DATAMAP(CAttributeList, m_NetworkedDynamicAttributesForDemos);
	DECL_DATAMAP(bool,           m_bOnlyIterateItemViewAttributes);
	DECL_SENDPROP(uint32,        m_iAccountID);
	DECL_SENDPROP(int,           m_iTeamNumber);
	
	int GetItemDefIndex() const { return vt_GetItemDefIndex(this); }
	
private:
	virtual ~CEconItemView() { assert(false); }
	
	
	static MemberFuncThunk<      CEconItemView *, void>                              ft_ctor;
	static MemberFuncThunk<      CEconItemView *, void, int, int, int, unsigned int> ft_Init;
	static MemberFuncThunk<const CEconItemView *, CTFItemDefinition *>               ft_GetStaticData;
	static MemberFuncThunk<const CEconItemView *, CEconItem *>                       ft_GetSOCData;
	static MemberFuncThunk<const CEconItemView *, const char *, int, int>            ft_GetPlayerDisplayModel;

	static inline MemberFuncThunk<const CEconItemView *, void, IEconItemAttributeIterator *>         ft_IterateAttributes     { "CEconItemView::IterateAttributes"      };
	
	static MemberVFuncThunk<const CEconItemView *, int> vt_GetItemDefIndex;
};

void ForEachItemAttribute(CEconItemView *item, std::function<bool (const CEconItemAttributeDefinition *, attribute_data_union_t)> onNormal, std::function<bool (const CEconItemAttributeDefinition *, const char*)> onString);
void ForEachItemAttribute(CEconItemView *item, std::function<bool (const CEconItemAttributeDefinition *, attribute_data_union_t)> onAttrib);

class CEconItem
{
public:
	attribute_t &AddDynamicAttributeInternal()                                                      				  { return ft_AddDynamicAttributeInternal(this); }
private:
	static MemberFuncThunk<CEconItem *, attribute_t &>                       ft_AddDynamicAttributeInternal;
};

enum AttributeDescriptionFormat
{
	ATTDESCFORM_VALUE_IS_PERCENTAGE,
	ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE,
	ATTDESCFORM_VALUE_IS_ADDITIVE,
	ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE,
};

class CEconItemAttributeDefinition
{
public:
	KeyValues *GetKeyValues() const       { return this->m_pKV; }
	unsigned short GetIndex() const       { return this->m_iIndex; }
	ISchemaAttributeType *GetType() const { return this->m_pAttributeType; }
	bool IsStoredAsInteger () const       { return this->m_bStoredAsInteger; }
	
	const char *GetName             (const char *fallback = nullptr) const { return this->GetKVString("name",               fallback); }
	const char *GetAttributeClass   (const char *fallback = nullptr) const { return this->GetKVString("attribute_class",    fallback); }
	int GetDescriptionFormat() const { return m_iDescriptionFormat; }
	bool IsMultiplicative() const { return m_iDescriptionFormat == ATTDESCFORM_VALUE_IS_PERCENTAGE || m_iDescriptionFormat == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE; }
	float GetDefaultValue() const { return IsMultiplicative() ? 1.0f : 0.0f; }
	
	
	template<typename T> bool IsType() const { return (dynamic_cast<T *>(this->GetType()) != nullptr); }
	
	/* do all the libstrcompat junk automatically;
	 * and handle "stored_as_integer" properly when BConvertStringToEconAttributeValue was called with b1 = true */
	void ConvertValueToString(const attribute_data_union_t& value, char *buf, size_t buf_len);
	
private:
	bool GetKVBool(const char *key, bool fallback) const
	{
		if (this->m_pKV == nullptr) return fallback;
		return this->m_pKV->GetBool(key, fallback);
	}
	const char *GetKVString(const char *key, const char *fallback) const
	{
		if (this->m_pKV == nullptr) return fallback;
		return this->m_pKV->GetString(key, fallback);
	}
	
	KeyValues *m_pKV;                       // +0x00
	unsigned short m_iIndex;                // +0x04
	ISchemaAttributeType *m_pAttributeType; // +0x08
	bool		m_bHidden;
	bool		m_pad1;
	bool		m_bStoredAsInteger;
	bool		m_pad2;
	int m_pad3;
	int m_pad4;
	int m_pad5;
	int m_pad6;
	int m_pad7;
	int m_iDescriptionFormat;
	// ...
};

class CEconItemAttribute // DT_ScriptCreatedAttribute
{
public:
	static CEconItemAttribute *Create(short iDefIndex)
	{
		/* using an arbitrary buffer size that's basically guaranteed to always
		 * be big enough, even if CEconItemAttribute grows in the future */
		auto ptr = reinterpret_cast<CEconItemAttribute *>(::operator new(0x20));
		ft_ctor(ptr);
		ptr->m_iAttributeDefinitionIndex = iDefIndex;
		return ptr;
	}
	static void Destroy(CEconItemAttribute *ptr)
	{
		if (ptr == nullptr) return;
		::operator delete(reinterpret_cast<void *>(ptr));
	}
	CEconItemAttribute() 
	{
		ft_ctor(this);
	}

	CEconItemAttribute( const attrib_definition_index_t iAttributeIndex, float flValue ) 
	{
		ft_ctor(this); 
		m_iAttributeDefinitionIndex = iAttributeIndex; 
		m_iRawValue32.m_Float = flValue;
	}
	
	attribute_data_union_t *GetValuePtr() { return &this->m_iRawValue32; }
	attribute_data_union_t GetValue() { return this->m_iRawValue32; }
	
	CEconItemAttributeDefinition *GetStaticData() const { return ft_GetStaticData(this); }

	attrib_definition_index_t GetAttributeDefinitionIndex() const { return m_iAttributeDefinitionIndex; }

	friend class CAttributeList;
private:
	static MemberFuncThunk<      CEconItemAttribute *, void>                           ft_ctor;
	static MemberFuncThunk<const CEconItemAttribute *, CEconItemAttributeDefinition *> ft_GetStaticData;
	
	void *__pad00;
	attrib_definition_index_t m_iAttributeDefinitionIndex;
	attribute_data_union_t m_iRawValue32;
	int m_nRefundableCurrency;
};
static_assert(sizeof(CEconItemAttribute) == 0x10);


class CAttributeContainer : public CAttributeManager
{
public:
	CEconItemView *GetItem() { return &this->m_Item; }
	//void SetItem(CEconItemView &view) { this->m_Item = view; }
private:
	DECL_SENDPROP_RW(CEconItemView, m_Item);
};


class CEconItemSystem
{
public:
	// TODO
	
private:
	// TODO
};
class CTFItemSystem : public CEconItemSystem {};


class CEconItemSchema
{
public:
	struct EquipRegion
	{
		CUtlConstStringBase<char> m_strName;
		unsigned int m_nBitMask;
		unsigned int m_nMask;
	};
	
	bool BInitAttributes(KeyValues *pKVAttributes, CUtlVector<CUtlString> *pVecErrors)    { return ft_BInitAttributes             (this, pKVAttributes, pVecErrors); }
	CEconItemDefinition *GetItemDefinition(int def_idx) const                             { return ft_GetItemDefinition           (this, def_idx); }
	CEconItemDefinition *GetItemDefinitionByName(const char *name) const                  { return ft_GetItemDefinitionByName     (this, name); }
	CEconItemAttributeDefinition *GetAttributeDefinition(int def_idx) const               { return ft_GetAttributeDefinition      (this, def_idx); }
	CEconItemAttributeDefinition *GetAttributeDefinitionByName(const char *name) const    { return ft_GetAttributeDefinitionByName(this, name); }
	void ItemTesting_CreateTestDefinition(int originalId, int newId, KeyValues* kv)       { ft_ItemTesting_CreateTestDefinition   (this, originalId, newId, kv); }
	
private:
	static MemberFuncThunk<CEconItemSchema *, bool, KeyValues *, CUtlVector<CUtlString> *>  ft_BInitAttributes;
	static MemberFuncThunk<const CEconItemSchema *, CEconItemDefinition *, int>                   ft_GetItemDefinition;
	static MemberFuncThunk<const CEconItemSchema *, CEconItemDefinition *, const char *>          ft_GetItemDefinitionByName;
	static MemberFuncThunk<const CEconItemSchema *, CEconItemAttributeDefinition *, int>          ft_GetAttributeDefinition;
	static MemberFuncThunk<const CEconItemSchema *, CEconItemAttributeDefinition *, const char *> ft_GetAttributeDefinitionByName;
	static MemberFuncThunk<CEconItemSchema *, void, int, int, KeyValues *> ft_ItemTesting_CreateTestDefinition;
};
class CTFItemSchema : public CEconItemSchema {};


extern StaticFuncThunk<CTFItemSchema *> ft_GetItemSchema;
inline CTFItemSchema *GetItemSchema() { return ft_GetItemSchema(); }

class CItemGeneration
{
public:
	CBaseEntity *SpawnItem(CEconItemView const* view, Vector const& vec, QAngle const& ang, char const* clname)                         { return ft_SpawnItem          (this, view, vec, ang, clname); }
	CBaseEntity *SpawnItem(int defId, Vector const& vec, QAngle const& ang, int itemLevel, int quality, char const* clname)             { return ft_SpawnItem_defid    (this, defId, vec, ang, itemLevel, quality, clname); }
	CBaseEntity *GenerateItemFromScriptData(CEconItemView const* view, Vector const& vec, QAngle const& ang, char const* clname)        { return ft_GenerateItemFromScriptData          (this, view, vec, ang, clname); }
	
private:
	
	static MemberFuncThunk<CItemGeneration *, CBaseEntity *, CEconItemView const*, Vector const&, QAngle const&, char const*> ft_SpawnItem;
	static MemberFuncThunk<CItemGeneration *, CBaseEntity *, int, Vector const&, QAngle const&, int, int, char const*> ft_SpawnItem_defid;
	static MemberFuncThunk<CItemGeneration *, CBaseEntity *, CEconItemView const*, Vector const&, QAngle const&, char const*> ft_GenerateItemFromScriptData;
};


extern StaticFuncThunk<CItemGeneration *> ft_ItemGeneration;
inline CItemGeneration *ItemGeneration() { return ft_ItemGeneration(); }

extern StaticFuncThunk<void, const CAttribute_String *, const char **> ft_CopyStringAttributeValueToCharPointerOutput;

inline void CopyStringAttributeValueToCharPointerOutput(const CAttribute_String *attr_str, const char **p_cstr) { ft_CopyStringAttributeValueToCharPointerOutput(attr_str, p_cstr); }
inline const char *GetStringAttributeValue(const CAttribute_String *attr_str) 
{ 
	const char *p_cstr;
	ft_CopyStringAttributeValueToCharPointerOutput(attr_str, &p_cstr);
	return  p_cstr;
}


extern GlobalThunk<const char *[NUM_VISUALS_BLOCKS]> g_TeamVisualSections;


int GetNumberOfLoadoutSlots();
bool IsValidLoadoutSlotNumber(int num);
loadout_positions_t ClampLoadoutSlotNumber(int num);

/* deals with un-prefixed loadout position names */
const char *GetLoadoutSlotName(loadout_positions_t slot);
loadout_positions_t GetLoadoutSlotByName(const char *name);

/* deals with native, LOADOUT_POSISTION_ prefixed loadout position names */
const char *GetLoadoutPositionName(loadout_positions_t slot);
loadout_positions_t GetLoadoutPositionByName(const char *name);

inline const char *GetLoadoutSlotName    (int slot) { return GetLoadoutSlotName    (static_cast<loadout_positions_t>(slot)); }
inline const char *GetLoadoutPositionName(int slot) { return GetLoadoutPositionName(static_cast<loadout_positions_t>(slot)); }


/* GetItemDefinition returns the def for the "default" item in the schema if the index doesn't match anything */
inline CTFItemDefinition *FilterOutDefaultItemDef(CTFItemDefinition *item_def)
{
	if (item_def != nullptr && strcmp(item_def->GetName("default"), "default") == 0) {
		return nullptr;
	}
	
	return item_def;
}


class CPlayerInventory
{
public:
	void DumpInventoryToConsole(bool b1 = true) {        vt_DumpInventoryToConsole(this, b1); }
	int GetMaxItemCount() const                 { return vt_GetMaxItemCount(this); }

	CEconItemView *GetItemByPosition(int position, int *index) { return ft_GetItemByPosition(this, position, index); }
	
	int vtable;
	CSteamID m_OwnerId;
private:
	static MemberVFuncThunk<      CPlayerInventory *, void, bool> vt_DumpInventoryToConsole;
	static MemberVFuncThunk<const CPlayerInventory *, int>        vt_GetMaxItemCount;
	
	static MemberFuncThunk< CPlayerInventory *, CEconItemView *, int, int *>        ft_GetItemByPosition;
};

class CTFPlayerInventory : public CPlayerInventory
{
public:
	
private:
	
};


class CInventoryManager
{
public:
	
private:
	
};

class CTFInventoryManager : public CInventoryManager
{
public:
	CTFPlayerInventory *GetInventoryForPlayer(const CSteamID& steamid) { return ft_GetInventoryForPlayer(this, steamid); }
	CEconItemView *GetBaseItemForClass(int iClass, int iSlot) { return ft_GetBaseItemForClass(this, iClass, iSlot); }
	
private:
	static MemberFuncThunk<CTFInventoryManager *, CTFPlayerInventory *, const CSteamID&> ft_GetInventoryForPlayer;
	static MemberFuncThunk<CTFInventoryManager *, CEconItemView *, int, int> ft_GetBaseItemForClass;
};


CInventoryManager   *InventoryManager();
CTFInventoryManager *TFInventoryManager();

template < typename TActualTypeInMemory, typename TTreatAsThisType = TActualTypeInMemory >
class CAttributeIterator_GetTypedAttributeValue : public IEconItemAttributeIterator
{
public:
	CAttributeIterator_GetTypedAttributeValue( const CEconItemAttributeDefinition *pAttrDef, TTreatAsThisType *outpValue )
		: m_pAttrDef( pAttrDef )
		, m_outpValue( outpValue )
		, m_bFound( false )
	{

		Assert( m_pAttrDef );
		Assert( outpValue );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) const override
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float value ) const override
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const uint64 & value ) const override
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String & value ) const override
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent & value ) const override
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_ItemSlotCriteria & value ) const override
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_WorldItemPlacement & value ) const override
	{
		return OnIterateAttributeValueTyped( pAttrDef, value );
	}

	bool WasFound() const
	{
		return m_bFound;
	}

private:
	template < typename TAnyOtherType >
	bool OnIterateAttributeValueTyped( const CEconItemAttributeDefinition *pAttrDef, const TAnyOtherType& value )
	{
		return true;
	}

	bool OnIterateAttributeValueTyped( const CEconItemAttributeDefinition *pAttrDef, const TActualTypeInMemory& value )
	{

		if ( m_pAttrDef == pAttrDef )
		{
			m_bFound = true;
			CopyAttributeValueToOutput( &value, reinterpret_cast<TTreatAsThisType *>( m_outpValue ) );
		}
			
		return !m_bFound;
	}

private:
	static void CopyAttributeValueToOutput( const TActualTypeInMemory *pValue, TTreatAsThisType *out_pValue )
	{

		*out_pValue = *reinterpret_cast<const TTreatAsThisType *>( pValue );
	}

private:
	const CEconItemAttributeDefinition *m_pAttrDef;
	TTreatAsThisType *m_outpValue;
	bool m_bFound;
};


template<typename TypeOut, typename AttributeHolder>
bool FindAttribute(const AttributeHolder *holder, const CEconItemAttributeDefinition *attrDef, TypeOut *out) {
	if ( !attrDef )
		return false;

	CAttributeIterator_GetTypedAttributeValue<TypeOut, TypeOut> it( attrDef, out );
	holder->IterateAttributes( &it );
	return it.WasFound();
}
#endif
