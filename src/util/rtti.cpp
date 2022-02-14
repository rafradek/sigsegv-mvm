#include "util/rtti.h"
#include "library.h"
#include "mem/scan.h"
#include "util/prof.h"

#include <regex>


namespace RTTI
{
	// Windows: str typeinfo(T).raw_name() => _TypeDescriptor*
	// Linux:   str typeinfo(T).name()     => abi::__class_type_info*
	static std::unordered_map<std::string, const rtti_t *> s_RTTI;
	
	// Windows: str typeinfo(T).raw_name() => void **
	// Linux:   str typeinfo(T).name()     => void **
	static std::unordered_map<std::string, const void **> s_VT;
	
	
	void PreLoad()
	{
		DevMsg("RTTI::PreLoad BEGIN\n");
		
		s_RTTI.clear();
		s_VT.clear();
		
#if defined __GNUC__
		
		for (auto lib : {Library::SERVER, Library::ENGINE, Library::TIER0}) {
			LibMgr::ForEachSym(lib, [](const Symbol& sym){
				const char *buf = sym.name.c_str();
				size_t buf_len  = sym.name.size();
				char name[4096];
				
				if (buf_len >= 4 && memcmp(buf, "_ZT", 3) == 0) {
					bool is_rtti = (buf[3] == 'I');
					bool is_vt   = (buf[3] == 'V');
					
					if (is_rtti || is_vt) {
						size_t len = buf_len - 4;
						len = Min(len, sizeof(name) - 1);
						
						memcpy(name, buf + 4, len);
						name[len] = '\0';
						
						if (is_rtti) {
							std::string key(name);
							auto addr = (const rtti_t *)(sym.addr);
							
							if (s_RTTI.find(key) != s_RTTI.end()) {
								DevWarning("RTTI::PreLoad: duplicate symbol \"_ZTI%s\"\n", name);
							} else {
								s_RTTI[key] = addr;
							//	DevMsg("RTTI: %08x \"%s\"\n", (uintptr_t)addr, name);
							}
						} else if (is_vt) {
							std::string key(name);
							auto addr = (const void **)((uintptr_t)(sym.addr) + offsetof(vtable, vfptrs));
							
							if (s_VT.find(key) != s_VT.end()) {
								DevWarning("RTTI::PreLoad: duplicate symbol \"_ZTV%s\"\n", name);
							} else {
								s_VT[key] = addr;
							//	DevMsg("VT: %08x \"%s\"\n", (uintptr_t)addr, name);
							}
						}
					}
				}
				
				return true;
			});
		}

		
#elif defined _MSC_VER
		
		// COL scan profiling results:
		// 
		// FIRST:
		// FWD  202-219 ms
		// REV  7-9 ms
		// 
		// ALL:
		// FWD  278-292 ms
		// REV  220-235 ms
		
		// VT scan profiling results:
		// 
		// FIRST:
		// FWD  172-180 ms
		// REV  176-197 ms
		// 
		// ALL:
		// FWD  296-303 ms
		// REV  282-305 ms
		
		using TDScanner  = CStringPrefixScanner<ScanDir::FORWARD, ScanResults::ALL, 0x1>;
		using COLScanner = CAlignedTypeScanner <ScanDir::REVERSE, ScanResults::ALL, const _TypeDescriptor *>;
		using VTScanner  = CAlignedTypeScanner <ScanDir::FORWARD, ScanResults::ALL, const __RTTI_CompleteObjectLocator *>;
		
		int n_total = 0;
		int n_skip  = 0;
		int n_add   = 0;
		
		std::vector<std::regex> exclude;
		exclude.emplace_back(R"(^.*@std@@$)");
		exclude.emplace_back(R"(^.*@CryptoPP@@$)");
		exclude.emplace_back(R"(^.*@protobuf@google@@$)");
		exclude.emplace_back(R"(^.*@GCSDK@@$)");
		exclude.emplace_back(R"(^\.\?AVCGC.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CGC.*$)");
		exclude.emplace_back(R"(^\.\?AVCMsg.*$)");
		exclude.emplace_back(R"(^\.\?AVCSO.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CCallback.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CCallResult.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CUtl\w+DataOps.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CEntityFactory@.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CEntityDataInstantiator@.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CEntityClassList@.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CFmtStrN.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CUtlVector.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CCopyableUtlVector.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CFunctor.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CMemberFunctor.*$)");
		exclude.emplace_back(R"(^\.\?AVNetworkVar_.*$)");
		exclude.emplace_back(R"(^\.\?AVCAI_.*$)");
		exclude.emplace_back(R"(^\.\?AV\?\$CAI_.*$)");
		exclude.emplace_back(R"(^\.\?AVCEconTool_.*$)");
		exclude.emplace_back(R"(^\.\?AVCTFQuest.*$)");
		exclude.emplace_back(R"(^\.\?AVCWorkshop_.*$)");
		
		for (auto lib : {Library::SERVER, Library::ENGINE, Library::TIER0, Library::CLIENT}) {
			if (!LibMgr::HaveLib(lib)) continue;
			
			Prof::Begin();
			CScan<TDScanner> scan1(CLibSegBounds(lib, Segment::DATA), ".?AV");
			Prof::End("TD scan");
			Prof::Begin();
			for (auto match : scan1.Matches()) {
				auto name = (const char *)match;
				auto addr = (const rtti_t *)((uintptr_t)match - offsetof(_TypeDescriptor, name));
				
				++n_total;
				bool skip = false;
				for (auto& filter : exclude) {
					if (std::regex_match(name, filter, std::regex_constants::match_any)) {
						skip = true;
						break;
					}
				}
				if (skip) {
					++n_skip;
					continue;
				}
				++n_add;
				
				std::string key(name);
				if (s_RTTI.find(key) != s_RTTI.end()) {
				//	DevWarning("RTTI::PreLoad: duplicate RTTI str \"%s\"\n", name);
				} else {
					s_RTTI[key] = addr;
				//	DevMsg("\"%s\" TD @ %08x\n", name, (uintptr_t)s_RTTI[key]);
				}
			}
			DevMsg("RTTI::PreLoad:\n"
				"total %d\n"
				"skip  %d\n"
				"add   %d\n",
				n_total, n_skip, n_add);
			Prof::End("TD post");
			
			
			Prof::Begin();
			std::unordered_map<COLScanner *, std::string> scannermap_COL;
			std::vector<COLScanner> scanners_COL;
			for (const auto& pair : s_RTTI) {
				auto name = pair.first;
				auto p_TD = pair.second;
				
				auto& scanner = scanners_COL.emplace_back(CLibSegBounds(lib, Segment::RODATA), p_TD);
				scannermap_COL[&scanner] = name;
			}
			Prof::End("COL pre");
			Prof::Begin();
			CMultiScan scan_COL(scanners_COL);
			Prof::End("COL scan");
			
			Prof::Begin();
			std::unordered_map<std::string, const __RTTI_CompleteObjectLocator *> results_COL;
			for (const auto& scanner : scanners_COL) {
				auto& name = scannermap_COL[&scanner];
				
				std::vector<const __RTTI_CompleteObjectLocator *> matches;
				for (auto match : scanner.Matches()) {
					auto p_COL = (const __RTTI_CompleteObjectLocator *)((uintptr_t)match - offsetof(__RTTI_CompleteObjectLocator, pTypeDescriptor));
					
					if (p_COL->signature == 0x00000000 && p_COL->offset == 0x00000000 && p_COL->cdOffset == 0x00000000) {
						matches.push_back(p_COL);
					}
				}
				
				if (matches.size() != 1) {
				//	DevMsg("RTTI::PreLoad: %u TD refs for \"%s\"\n", scanner.Matches().size(), name.c_str());
					continue;
				}
				
				results_COL[name] = matches[0];
			//	DevMsg("\"%s\" COL @ %08x\n", name.c_str(), (uintptr_t)results_COL[name]);
			}
			Prof::End("COL post");
			
			
			Prof::Begin();
			std::unordered_map<VTScanner *, std::string> scannermap_VT;
			std::vector<VTScanner> scanners_VT;
			for (const auto& pair : results_COL) {
				auto name  = pair.first;
				auto p_COL = pair.second;
				
				auto& scanner = scanners_VT.emplace_back(CLibSegBounds(lib, Segment::RODATA), p_COL);
				scannermap_VT[&scanner] = name;
			}
			Prof::End("VT pre");
			Prof::Begin();
			CMultiScan scan_VT(scanners_VT);
			Prof::End("VT scan");
			
			Prof::Begin();
			for (const auto& scanner : scanners_VT) {
				auto& name = scannermap_VT[&scanner];
				
				if (!scanner.ExactlyOneMatch()) {
				//	DevMsg("RTTI::PreLoad: %u COL refs for \"%s\"\n", scanner.Matches().size(), name.c_str());
					continue;
				}
				
				s_VT[name] = (const void **)((uintptr_t)scanner.FirstMatch() + 0x4);
			//	DevMsg("\"%s\" VT @ %08x\n", name.c_str(), (uintptr_t)s_VT[name]);
			}
			Prof::End("VT post");
		}
		
		
#if 0
		std::multimap<uintptr_t, std::string> addrmap;
		for (const auto& pair : s_RTTI) {
			addrmap.emplace((uintptr_t)pair.second, "TD   " + pair.first);
		}
		for (const auto& pair : results_COL) {
			addrmap.emplace((uintptr_t)pair.second, "COL  " + pair.first);
		}
		for (const auto& pair : s_VT) {
			addrmap.emplace((uintptr_t)pair.second, "VT   " + pair.first);
		}
		
		const LibInfo& info = LibMgr::GetInfo(Library::SERVER);
		for (const auto& pair : info.segs) {
			const SegInfo& seg = pair.second;
			
			addrmap.emplace(info.baseaddr + seg.off, "SEG< " + pair.first);
			addrmap.emplace(info.baseaddr + seg.off + seg.len, ">SEG " + pair.first);
		}
		
		for (const auto& pair : addrmap) {
			DevMsg("%08x %s\n", pair.first - info.baseaddr, pair.second.c_str());
		}
#endif
		
#endif
		
		
		DevMsg("RTTI::PreLoad: found %u RTTI\n", s_RTTI.size());
		DevMsg("RTTI::PreLoad: found %u VT\n", s_VT.size());
		
		// RTTI
		// Linux: find all symbols of format "_ZTI"
		// Windows: find typename strings, find refs, subtract 8 bytes
		
		// VTable
		// Linux: find all symbols of format "_ZTV", add 8 bytes
		// Windows: do some fancy backtracking from RTTI info
	}
	
	
	const rtti_t *GetRTTI(const char *name)
	{
		auto it = s_RTTI.find(std::string(name));
		if (it == s_RTTI.end()) {
			DevMsg("RTTI::GetRTTI FAIL: no RTTI addr for name \"%s\"\n", name);
			return nullptr;
		}
		
		return (*it).second;
	}
	
	const void **GetVTable(const char *name)
	{
		auto it = s_VT.find(std::string(name));
		if (it == s_VT.end()) {
			DevMsg("RTTI::GetVTable FAIL: no VT addr for name \"%s\"\n", name);
			return nullptr;
		}
		
		return (*it).second;
	}

	const std::unordered_map<std::string, const rtti_t *> &GetAllRTTI()
	{
		return s_RTTI;
	}
}
