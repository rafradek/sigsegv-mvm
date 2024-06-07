#include "gameconf.h"
#include "addr/standard.h"


#define DEBUG_GC 0


static const char *const configs[] = {
	"sigsegv/datamaps",
	"sigsegv/entities",
	"sigsegv/globals",
	"sigsegv/NextBotKnownEntity",
	"sigsegv/NextBotContextualQueryInterface",
	"sigsegv/NextBotEventResponderInterface",
	"sigsegv/NextBotInterface",
	"sigsegv/NextBotBodyInterface",
	"sigsegv/NextBotLocomotionInterface",
	"sigsegv/NextBotVisionInterface",
	"sigsegv/NextBotIntentionInterface",
	"sigsegv/NextBotBehavior",
	"sigsegv/NextBotPlayer",
	"sigsegv/NextBotPlayerBody",
	"sigsegv/NextBotPlayerLocomotion",
	"sigsegv/NextBotPath",
	"sigsegv/NextBotPathFollow",
	"sigsegv/NextBotChasePath",
	"sigsegv/NextBotManager",
	"sigsegv/population",
	"sigsegv/nav",
	"sigsegv/tfplayer",
	"sigsegv/tfbot",
	"sigsegv/tfbot_body",
	"sigsegv/tfbot_locomotion",
	"sigsegv/tfbot_vision",
	"sigsegv/tfbot_behavior",
	"sigsegv/misc",
	"sigsegv/debugoverlay",
	"sigsegv/client",
	"sigsegv/convars",
	"sigsegv/fugue",
	nullptr,
};


CSigsegvGameConf g_GCHook;

inline const std::string &GetSymbolName(const std::map<std::string, std::string> &kv) {
#ifdef PLATFORM_64BITS
	auto sym64 = kv.find("sym64");
	if (sym64 != kv.end()) return sym64->second;
#endif

	return kv.at("sym");
}

bool CSigsegvGameConf::LoadAll(char *error, size_t maxlen)
{
	gameconfs->AddUserConfigHook("sigsegv", this);
	
	for (const char *const *c_name = configs; *c_name != nullptr; ++c_name) {
		IGameConfig *conf = nullptr;
		
		if (!gameconfs->LoadGameConfigFile(*c_name, &conf, error, maxlen)) {
			/* failed config loads still require a close operation, otherwise
			 * they'll end up as stale entries in the gameconf manager cache */
			gameconfs->CloseGameConfigFile(conf);
			return false;
		} else if (conf == nullptr) {
			return false;
		}
		
		this->m_GameConfs.push_back(conf);
	}
	
	return true;
}

void CSigsegvGameConf::UnloadAll()
{
	for (auto conf : this->m_GameConfs) {
		gameconfs->CloseGameConfigFile(conf);
	}
	this->m_GameConfs.clear();
	
	gameconfs->RemoveUserConfigHook("sigsegv", this);
}


void CSigsegvGameConf::ReadSMC_ParseStart()
{
#if DEBUG_GC
	DevMsg("GC ParseStart\n");
#endif
	
	this->m_Section = ParseSection::ROOT;
}

void CSigsegvGameConf::ReadSMC_ParseEnd(bool halted, bool failed)
{
#if DEBUG_GC
	DevMsg("GC ParseEnd\n");
#endif
}

SMCResult CSigsegvGameConf::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
#if DEBUG_GC
	DevMsg("GC NewSection \"%s\"\n", name);
#endif
	
	switch (this->m_Section) {
	case ParseSection::ROOT:
		if (strcmp(name, "addrs") == 0) {
			this->m_Section = ParseSection::ADDRS;
			return SMCResult_Continue;
		} else if (strcmp(name, "addrs_group") == 0) {
			this->m_Section = ParseSection::ADDRS_GROUP;
			return this->AddrGroup_Start();
		} else {
			DevMsg("GameData error: unknown section at root level: \"%s\"\n", name);
			return SMCResult_HaltFail;
		}
	case ParseSection::ADDRS:
		this->m_Section = ParseSection::ADDRS_ENTRY;
		return this->AddrEntry_Start(name);
	case ParseSection::ADDRS_ENTRY:
		DevMsg("GameData error: sections not expected within addr entry: \"%s\"\n", name);
		return SMCResult_HaltFail;
	case ParseSection::ADDRS_GROUP:
		if (strcmp(name, "[common]") == 0) {
			this->m_Section = ParseSection::ADDRS_GROUP_COMMON;
			return this->AddrGroup_Common_Start();
		} else {
			DevMsg("GameData error: unknown section within addrs_group section: \"%s\"\n", name);
			return SMCResult_HaltFail;
		}
	case ParseSection::ADDRS_GROUP_COMMON:
		DevMsg("GameData error: sections not expected within addrs_group common section: \"%s\"\n", name);
		return SMCResult_HaltFail;
	}
	
	DevMsg("GameData error: section state %d not covered in ReadSMC_NewSection: \"%s\"\n", (int)this->m_Section, name);
	return SMCResult_HaltFail;
}

SMCResult CSigsegvGameConf::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
#if DEBUG_GC
	DevMsg("GC KeyValue \"%s\" \"%s\"\n", key, value);
#endif
	
	switch (this->m_Section) {
	case ParseSection::ROOT:
		DevMsg("GameData error: keyvalues not expected at root level: \"%s\" \"%s\"\n", key, value);
		return SMCResult_HaltFail;
	case ParseSection::ADDRS:
		DevMsg("GameData error: keyvalues not expected in addrs section: \"%s\" \"%s\"\n", key, value);
		return SMCResult_HaltFail;
	case ParseSection::ADDRS_ENTRY:
		return this->AddrEntry_KeyValue(key, value);
	case ParseSection::ADDRS_GROUP:
		return this->AddrGroup_KeyValue(key, value);
	case ParseSection::ADDRS_GROUP_COMMON:
		return this->AddrGroup_Common_KeyValue(key, value);
	}
	
	DevMsg("GameData error: section state %d not covered in ReadSMC_KeyValue: \"%s\" \"%s\"\n", (int)this->m_Section, key, value);
	return SMCResult_HaltFail;
}

SMCResult CSigsegvGameConf::ReadSMC_LeavingSection(const SMCStates *states)
{
#if DEBUG_GC
	DevMsg("GC LeavingSection\n");
#endif
	
	switch (this->m_Section) {
	case ParseSection::ADDRS:
		this->m_Section = ParseSection::ROOT;
		return SMCResult_Continue;
	case ParseSection::ADDRS_ENTRY:
		this->m_Section = ParseSection::ADDRS;
		return this->AddrEntry_End();
	case ParseSection::ADDRS_GROUP:
		this->m_Section = ParseSection::ROOT;
		return this->AddrGroup_End();
	case ParseSection::ADDRS_GROUP_COMMON:
		this->m_Section = ParseSection::ADDRS_GROUP;
		return this->AddrGroup_Common_End();
	}
	
	return SMCResult_HaltFail;
}


SMCResult CSigsegvGameConf::AddrEntry_Start(const char *name)
{
#if DEBUG_GC
	DevMsg("GC AddrEntry_Start \"%s\"\n", name);
#endif
	
	this->m_AddrEntry_State.m_Name = name;
	this->m_AddrEntry_State.m_KeyValues.clear();
	
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_KeyValue(const char *key, const char *value)
{
#if DEBUG_GC
	DevMsg("GC AddrEntry_KeyValue \"%s\" \"%s\"\n", key, value);
#endif
	
	std::string s_key  (key);
	std::string s_value(value);
	
	this->m_AddrEntry_State.m_KeyValues[s_key] = s_value;
	
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_End()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
#if DEBUG_GC
	DevMsg("GC AddrEntry_End\n");
	
	DevMsg("CSigsegvGameConf: addr \"%s\" {", name.c_str());
	for (const auto& pair : kv) {
		DevMsg(" \"%s\": \"%s\" ", pair.first.c_str(), pair.second.c_str());
	}
	DevMsg("}\n");
#endif
	
	for (const std::string& key : { "type"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const std::string& type = kv.at("type");
	if (this->m_AddrParsers.find(type) == this->m_AddrParsers.end()) {
		DevMsg("GameData error: addr \"%s\" has unknown type \"%s\"\n", name.c_str(), type.c_str());
		return SMCResult_HaltFail;
	}
	
	auto parser = this->m_AddrParsers.at(type);
	return (this->*parser)();
}


SMCResult CSigsegvGameConf::AddrGroup_Start()
{
#if DEBUG_GC
	DevMsg("GC AddrGroup_Start\n");
#endif
	
	this->m_AddrGroup_State.m_CommonKV.clear();
	this->m_AddrGroup_State.m_Entries.clear();
	
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrGroup_KeyValue(const char *key, const char *value)
{
#if DEBUG_GC
	DevMsg("GC AddrGroup_KeyValue \"%s\" \"%s\"\n", key, value);
#endif
	
	std::string s_key  (key);
	std::string s_value(value);
	
	this->m_AddrGroup_State.m_Entries[s_key] = s_value;
	
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrGroup_End()
{
	const auto& common_kv = this->m_AddrGroup_State.m_CommonKV;
	const auto& entries   = this->m_AddrGroup_State.m_Entries;
	
#if DEBUG_GC
	DevMsg("GC AddrGroup_End\n");
	
//	DevMsg("CSigsegvGameConf: addr \"%s\" {", name);
//	for (const auto& pair : kv) {
//		DevMsg(" \"%s\": \"%s\" ", pair.first.c_str(), pair.second.c_str());
//	}
//	DevMsg("}\n");
	#warning move debug dumping into the entry loop
#endif
	
	for (const std::string& key : { "type"s, "lib"s }) {
		if (common_kv.find(key) == common_kv.end()) {
			DevMsg("GameData error: addr group lacks required common key \"%s\"\n", key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& type = common_kv.at("type");
	const auto& lib  = common_kv.at("lib");
	
	if (this->m_AddrParsers.find(type) == this->m_AddrParsers.end()) {
		DevMsg("GameData error: addr group has unknown type \"%s\"\n", type.c_str());
		return SMCResult_HaltFail;
	}
	
	struct HandlerData
	{
		const std::string& key;
		const std::string& value;
		std::string& name;
		std::map<std::string, std::string>& kv;
	};
	
	static std::map<std::string, void (*)(HandlerData)> type_handlers {
		{
			"sym",
			[](HandlerData data){
				data.name      = data.key;
				data.kv["sym"] = data.value;
			}
		},
		{
			"sym regex",
			[](HandlerData data){
				data.name      = data.key;
				data.kv["sym"] = data.value;
			}
		},
		{
			"datamap",
			[](HandlerData data){
				data.name        = data.key + "::m_DataMap";
				data.kv["class"] = data.key;
				data.kv["sym"]   = data.value;
			}
		},
		{
			"convar",
			[](HandlerData data){
				data.name       = data.key;
				data.kv["name"] = data.value;
			}
		},
		{
			"concommand",
			[](HandlerData data){
				data.name       = data.key;
				data.kv["name"] = data.value;
			}
		},
	};
	
	if (type_handlers.find(type) == type_handlers.end()) {
		DevMsg("GameData error: addr group doesn't currently allow type \"%s\"\n", type.c_str());
		return SMCResult_HaltFail;
	}               
	
	auto handler = type_handlers.at(type);
	auto parser  = this->m_AddrParsers.at(type);
	
	for (const auto& pair : entries) {
		const std::string& key   = pair.first;
		const std::string& value = pair.second;
		
		this->m_AddrEntry_State.m_KeyValues = common_kv;
		
		HandlerData data {
			key,
			value,
			this->m_AddrEntry_State.m_Name,
			this->m_AddrEntry_State.m_KeyValues,
		};
		(*handler)(data);
		
		SMCResult result = (this->*parser)();
		if (result != SMCResult_Continue) {
			return SMCResult_HaltFail;
		}
	}
	
	return SMCResult_Continue;
}


SMCResult CSigsegvGameConf::AddrGroup_Common_Start()
{
#if DEBUG_GC
	DevMsg("GC AddrGroup_Common_Start\n");
#endif
	
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrGroup_Common_KeyValue(const char *key, const char *value)
{
#if DEBUG_GC
	DevMsg("GC AddrGroup_Common_KeyValue \"%s\" \"%s\"\n", key, value);
#endif
	
	std::string s_key  (key);
	std::string s_value(value);
	
	this->m_AddrGroup_State.m_CommonKV[s_key] = s_value;
	
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrGroup_Common_End()
{
#if DEBUG_GC
	DevMsg("GC AddrGroup_Common_End\n");
#endif
	
	return SMCResult_Continue;
}


void CSigsegvGameConf::AddrEntry_Load_Common(IAddr *addr)
{
	const auto& kv = this->m_AddrEntry_State.m_KeyValues;
	
	if (kv.find("lib") != kv.end()) {
		addr->SetLibrary(LibMgr::Lib_FromString(kv.at("lib").c_str()));
	}
}


SMCResult CSigsegvGameConf::AddrEntry_Load_Sym()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym = GetSymbolName(kv);
	
	auto a = new CAddr_Sym(name, sym);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Sym_Regex()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym = GetSymbolName(kv);
	
	auto a = new CAddr_Sym_Regex(name, sym);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Fixed()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "addr"s, "build"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym = GetSymbolName(kv);
	int addr        = stoi(kv.at("addr"), nullptr, 0);
	int build       = stoi(kv.at("build"), nullptr, 0);
	
	auto a = new CAddr_FixedAddr(name, sym, addr, build);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Pattern()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "seg"s, "seek"s, "mask"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym  = GetSymbolName(kv);
	const auto& seg  = kv.at("seg");
	const auto& seek = kv.at("seek");
	const auto& mask = kv.at("mask");
	
	auto a = new CAddr_Pattern(name, sym, seg, seek, mask);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_DataDescMap()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "class"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym    = GetSymbolName(kv);
	const auto& c_name = kv.at("class");
	
	auto a = new CAddr_DataDescMap(name, sym, c_name);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Func_KnownVTIdx()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "vtable"s, "idx"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym    = GetSymbolName(kv);
	const auto& vtable = kv.at("vtable");
	int idx            = stoi(kv.at("idx"), nullptr, 0);
	
	auto a = new CAddr_Func_KnownVTIdx(name, sym, vtable, idx);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Func_DataMap_VThunk()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "datamap"s, "func"s, "vtable"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym     = GetSymbolName(kv);
	const auto& datamap = kv.at("datamap");
	const auto& f_name  = kv.at("func");
	const auto& vtable  = kv.at("vtable");
	
	auto a = new CAddr_Func_DataMap_VThunk(name, sym, datamap, f_name, vtable);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Func_EBPPrologue_UniqueRef()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "uniref"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym    = GetSymbolName(kv);
	const auto& uniref = kv.at("uniref");
	
	auto a = new CAddr_Func_EBPPrologue_UniqueRef(name, sym, uniref);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Func_EBPPrologue_UniqueStr()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "unistr"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym    = GetSymbolName(kv);
	const auto& unistr = kv.at("unistr");
	
	auto a = new CAddr_Func_EBPPrologue_UniqueStr(name, sym, unistr);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Func_EBPPrologue_UniqueStr_KnownVTIdx()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "unistr"s, "vtable"s, "idx"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym    = GetSymbolName(kv);
	const auto& unistr = kv.at("unistr");
	const auto& vtable = kv.at("vtable");
	int idx            = stoi(kv.at("idx"), nullptr, 0);
	
	auto a = new CAddr_Func_EBPPrologue_UniqueStr_KnownVTIdx(name, sym, unistr, vtable, idx);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Func_EBPPrologue_NonUniqueStr_KnownVTIdx()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "str"s, "vtable"s, "idx"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym    = GetSymbolName(kv);
	const auto& str    = kv.at("str");
	const auto& vtable = kv.at("vtable");
	int idx            = stoi(kv.at("idx"), nullptr, 0);
	
	auto a = new CAddr_Func_EBPPrologue_NonUniqueStr_KnownVTIdx(name, sym, str, vtable, idx);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_Func_EBPPrologue_VProf()
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "sym"s, "v_name"s, "v_group"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& sym     = GetSymbolName(kv);
	const auto& v_name  = kv.at("v_name");
	const auto& v_group = kv.at("v_group");
	
	auto a = new CAddr_Func_EBPPrologue_VProf(name, sym, v_name, v_group);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}

SMCResult CSigsegvGameConf::AddrEntry_Load_ConCommandBase(bool is_command)
{
	const auto& name = this->m_AddrEntry_State.m_Name;
	const auto& kv   = this->m_AddrEntry_State.m_KeyValues;
	
	for (const std::string& key : { "name"s }) {
		if (kv.find(key) == kv.end()) {
			DevMsg("GameData error: addr \"%s\" lacks required key \"%s\"\n", name.c_str(), key.c_str());
			return SMCResult_HaltFail;
		}
	}
	
	const auto& con_name = kv.at("name");
	
	auto a = new CAddr_ConCommandBase(name, con_name, is_command);
	this->AddrEntry_Load_Common(a);
	this->m_AddrPtrs.push_back(std::unique_ptr<IAddr>(a));
	return SMCResult_Continue;
}
