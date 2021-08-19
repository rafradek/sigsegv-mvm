#include "prop.h"
#include "mem/extract.h"
#include "stub/baseentity.h"
#include "util/misc.h"
#include "util/demangle.h"


void ListProps(bool fail_only)
{
	size_t len_obj = 0;
//	size_t len_mem = 0;
	for (auto prop : AutoList<IProp>::List()) {
		len_obj = Max(len_obj, strlen(prop->GetObjectName()));
//		len_mem = Max(len_mem, strlen(prop->GetMemberName()));
	}
	
	std::vector<IProp *> props_sorted;
	for (auto prop : AutoList<IProp>::List()) {
		props_sorted.push_back(prop);
	}
	std::sort(props_sorted.begin(), props_sorted.end(), [](IProp *lhs, IProp *rhs){
		std::string obj_lhs = lhs->GetObjectName();
		std::string obj_rhs = rhs->GetObjectName();
		if (obj_lhs != obj_rhs) return (obj_lhs < obj_rhs);
		
		int off_lhs = INT_MAX; lhs->GetOffset(off_lhs);
		int off_rhs = INT_MAX; rhs->GetOffset(off_rhs);
		if (off_lhs != off_rhs) return (off_lhs < off_rhs);
		
		std::string mem_lhs = lhs->GetMemberName();
		std::string mem_rhs = rhs->GetMemberName();
		return (mem_lhs < mem_rhs);
	});
	
	MAT_SINGLE_THREAD_BLOCK {
		Msg("%-8s  %7s  %-*s  %s\n", "KIND", "VALUE", len_obj, "CLASS", "MEMBER");
		for (auto prop : props_sorted) {
			const char *n_obj = prop->GetObjectName();
			const char *n_mem = prop->GetMemberName();
			const char *kind  = prop->GetKind();
			
			switch (prop->GetState()) {
			case IProp::State::INITIAL:
			{	
				if (!fail_only)
					Msg("%-8s  %7s  %-*s  %s\n", kind, "INIT", len_obj, n_obj, n_mem);
				break;
			}
			case IProp::State::OK:
			{
				int off = -1;
				prop->GetOffset(off);
				if (!fail_only)
					Msg("%-8s  +0x%04x  %-*s  %s\n", kind, off, len_obj, n_obj, n_mem);
				break;
			}
			case IProp::State::FAIL:
				Msg("%-8s  %7s  %-*s  %s\n", kind, "FAIL", len_obj, n_obj, n_mem);
				break;
			}
		}
	}
}

void CC_ListProps(const CCommand& cmd)
{
	ListProps(false);
}

static ConCommand ccmd_list_props("sig_list_props", &CC_ListProps,
	"List props and show their status", FCVAR_NONE);


namespace Prop
{
	void PreloadAll()
	{
		DevMsg("Prop::PreloadAll\n");
		for (auto prop : AutoList<IProp>::List()) {
			prop->Preload();
		}
		
		ListProps(true);
	}
	
	
	bool FindOffset(int& off, const char *obj, const char *var)
	{
		for (auto prop : AutoList<IProp>::List()) {
			if (strcmp(obj, prop->GetObjectName()) == 0 && strcmp(var, prop->GetMemberName()) == 0) {
				return prop->GetOffset(off);
			}
		}
		
		return false;
	}
	
	int FindOffsetAssert(const char *obj, const char *var)
	{
		int off = -1;
		assert(FindOffset(off, obj, var));
		return off;
	}
	
	
#if 0
	static std::unordered_map<std::string, const datamap_t *> datamaps_by_classname;
	static std::unordered_map<std::string, std::string>       classnames_by_rtti_name;
	
	static void PreloadDataMaps()
	{
		static bool init = false;
		
		if (!init) {
			assert(datamaps_by_classname  .empty());
			assert(classnames_by_rtti_name.empty());
			
			auto dict = static_cast<CEntityFactoryDictionary *>(servertools->GetEntityFactoryDictionary());
			assert(dict != nullptr);
			
			FOR_EACH_DICT_FAST(dict->m_Factories, i) {
				const char *classname = dict->m_Factories.GetElementName(i);
				assert(classname != nullptr);
				assert(classname[0] != '\0');
				
				IServerNetworkable *sv_networkable = dict->Create(classname);
				assert(sv_networkable != nullptr);
				
				CBaseEntity *entity = sv_networkable->GetBaseEntity();
				assert(entity != nullptr);
				
				const datamap_t *p_datamap = entity->GetDataDescMap();
				assert(p_datamap != nullptr);
				
				const char *rtti_name = TypeName(entity);
				assert(rtti_name != nullptr);
				assert(rtti_name[0] != '\0');
				
				std::string rtti_name_demangled;
				assert(DemangleTypeName(rtti_name, rtti_name_demangled));
				rtti_name = rtti_name_demangled.c_str();
				
			//	servertools->RemoveEntityImmediate(entity);
				
				sm_datatable_info_t info;
				if (gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(entity), "m_iEFlags", &info)) {
					*(int *)((char *)entity + info.actual_offset) |= EFL_KILLME;
				}
				
				ConColorMsg(Color(0x00, 0xff, 0xff, 0xff), "%-60s%s",
					CFmtStr("classname '%s':",  classname).Get(),
					CFmtStr("rtti_name '%s'\n", rtti_name).Get());
				
				auto result1 = datamaps_by_classname.insert(std::make_pair(classname, p_datamap));
				assert(result1.second);
				
				auto result2 = classnames_by_rtti_name.insert(std::make_pair(rtti_name, classname));
			//	assert(result2.second);
			}
			
			//	sm_datatable_info_t info;
			//	for ( int i = dict->m_Factories.First(); i != dict->m_Factories.InvalidIndex(); i = dict->m_Factories.Next( i ) )
			//	{
			//		IServerNetworkable *entity = dict->Create(dict->m_Factories.GetElementName(i));
			//		ServerClass *sclass = entity->GetServerClass();
			//		fprintf(fp,"%s - %s\n",sclass->GetName(), dict->m_Factories.GetElementName(i));
			//		
			//		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(entity->GetBaseEntity()), "m_iEFlags", &info))
			//			continue;
			//		
			//		int *eflags = (int *)((char *)entity->GetBaseEntity() + info.actual_offset);
			//		*eflags |= (1<<0); // EFL_KILLME
			//	}
			
			init = true;
		}
	}
	
	const datamap_t *GetDataMapByClassname(const char *classname)
	{
		PreloadDataMaps();
		
		auto it = datamaps_by_classname.find(classname);
		if (it != datamaps_by_classname.end()) {
			return (*it).second;
		} else {
			return nullptr;
		}
	}
	
	const datamap_t *GetDataMapByRTTIName(const char *rtti_name)
	{
		PreloadDataMaps();
		
		auto it = classnames_by_rtti_name.find(rtti_name);
		if (it != classnames_by_rtti_name.end()) {
			return GetDataMapByClassname((*it).second.c_str());
		} else {
			return nullptr;
		}
	}
#endif
}


bool CProp_SendProp::CalcOffset(int& off) const
{
	ServerClass *sv_class = this->FindServerClass();
	if (sv_class == nullptr) {
		Warning("CProp_SendProp: %s::%s FAIL: can't find ServerClass \"%s\"\n", this->GetObjectName(), this->GetMemberName(), this->m_pszServerClass);
		return false;
	}
	
	if (!this->FindSendProp(off, sv_class->m_pTable)) {
		Warning("CProp_SendProp: %s::%s FAIL: can't find SendProp \"%s\"\n", this->GetObjectName(), this->GetMemberName(), this->GetSendPropMemberName());
		return false;
	}
	
	return true;
}

ServerClass *CProp_SendProp::FindServerClass() const
{
	for (ServerClass *sv_class = gamedll->GetAllServerClasses(); sv_class != nullptr; sv_class = sv_class->m_pNext) {
		if (strcmp(sv_class->GetName(), this->m_pszServerClass) == 0) {
			return sv_class;
		}
	}
	
	return nullptr;
}

bool CProp_SendProp::FindSendProp(int& off, SendTable *s_table) const
{
	for (int i = 0; i < s_table->GetNumProps(); ++i) {
		SendProp *s_prop = s_table->GetProp(i);
		
		if (s_prop->GetName() != nullptr && strcmp(s_prop->GetName(), this->GetSendPropMemberName()) == 0) {
			if (!this->IsSendPropUtlVector(off, s_prop)) {
				off = s_prop->GetOffset();
			}
			return true;
		}
		
		if (s_prop->GetDataTable() != nullptr) {
			if (this->FindSendProp(off, s_prop->GetDataTable())) {
				return true;
			}
		}
	}
	
	return false;
}

bool CProp_SendProp::IsSendPropUtlVector(int& off, SendProp *q_prop) const
{
	SendTable *s_table = q_prop->GetDataTable();
	if (s_table == nullptr) return false;
	
	auto SendProxy_LengthTable = reinterpret_cast<SendTableProxyFn>(AddrManager::GetAddr("SendProxy_LengthTable"));
	assert(SendProxy_LengthTable != nullptr);
	
	for (int i = 0; i < s_table->GetNumProps(); ++i) {
		SendProp *s_prop = s_table->GetProp(i);
		
		if (s_prop->GetName() != nullptr && strcmp(s_prop->GetName(), "lengthproxy") == 0 &&
			s_prop->GetDataTable() != nullptr && s_prop->GetDataTableProxyFn() == SendProxy_LengthTable) {
			auto extra = reinterpret_cast<const CSendPropExtra_UtlVector *>(s_prop->GetExtraData());
			if (extra != nullptr) {
				off = extra->m_Offset;
				return true;
			}
		}
	}
	
	return false;
}

const char *CProp_SendProp::GetSendPropMemberName() const
{
	if (this->m_pszRemoteName != nullptr) {
		return this->m_pszRemoteName;
	} else {
		return this->GetMemberName();
	}
}


bool CProp_DataMap::CalcOffset(int& off) const
{
	char str_DataMap[1024];
	V_sprintf_safe(str_DataMap, "%s::m_DataMap", this->GetObjectName());
	
	auto datamap = (datamap_t *)AddrManager::GetAddr(str_DataMap);
	if (datamap == nullptr) {
		Warning("CProp_DataMap: %s::%s FAIL: no addr for %s\n", this->GetObjectName(), this->GetMemberName(), str_DataMap);
		return false;
	}
	
	sm_datatable_info_t info;
	if (!gamehelpers->FindDataMapInfo(datamap, this->GetMemberName(), &info)) {
		Warning("CProp_DataMap: %s::%s FAIL: in FindDataMapInfo\n", this->GetObjectName(), this->GetMemberName());
		return false;
	}
	
//	const datamap_t *datamap = Prop::GetDataMapByRTTIName(this->GetObjectName());
//	if (datamap == nullptr) {
//		Warning("CProp_DataMap: %s::%s FAIL: can't find datamap for class %s\n", this->GetObjectName(), this->GetMemberName(), this->GetObjectName());
//		return false;
//	}
//	
//	sm_datatable_info_t info;
//	if (!gamehelpers->FindDataMapInfo(const_cast<datamap_t *>(datamap), this->GetMemberName(), &info)) {
//		Warning("CProp_DataMap: %s::%s FAIL: in FindDataMapInfo\n", this->GetObjectName(), this->GetMemberName());
//		return false;
//	}
	
	typedescription_t *td = info.prop;
	off = td->fieldOffset[TD_OFFSET_NORMAL];
	return true;
}


CProp_Extract::~CProp_Extract()
{
	if (this->m_Extractor != nullptr) {
		delete this->m_Extractor;
	}
}


bool CProp_Extract::CalcOffset(int& off) const
{
	if (this->m_Extractor == nullptr) {
		Warning("CProp_Extract: %s::%s FAIL: no extractor provided (nullptr)\n", this->GetObjectName(), this->GetMemberName());
		return false;
	}
	
	if (!this->m_Extractor->Init()) {
		Warning("CProp_Extract: %s::%s FAIL: in extractor Init\n", this->GetObjectName(), this->GetMemberName());
		return false;
	}
	
	if (!this->m_Extractor->Check()) {
		Warning("CProp_Extract: %s::%s FAIL: in extractor Check\n", this->GetObjectName(), this->GetMemberName());
		return false;
	}
	
	off = (int)this->m_Extractor->ExtractPtrAsInt();
	
	return true;
}


bool CProp_Relative::CalcOffset(int& off) const
{
	int base_off = 0;
	
	if (!this->m_RelProp->GetOffset(base_off)) {
		Warning("CProp_Relative: %s::%s FAIL: in base prop GetOffset\n", this->GetObjectName(), this->GetMemberName());
		return false;
	}
	
	off = base_off + this->m_iDiff;
	
	if (this->m_iAlign != 0) {
		int rem;
		switch (this->m_Method) {
		case REL_MANUAL:
			assert(false); // not supported
			break;
		case REL_AFTER:
			rem = (off % this->m_iAlign);
			if (rem != 0) off += (this->m_iAlign - rem);
			break;
		case REL_BEFORE:
			rem = (off % this->m_iAlign);
			if (rem != 0) off -= rem;
			break;
		}
	}
	
	return true;
}
