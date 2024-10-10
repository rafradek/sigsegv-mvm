#include "library.h"

#include <regex>


static std::map<Library, const char *> libnames{
	{ Library::INVALID,            "invalid"            },
	{ Library::THIS,               "sigsegv"            },
	{ Library::SERVER,             "server"             },
	{ Library::ENGINE,             "engine"             },
	{ Library::DEDICATED,          "dedicated"          },
	{ Library::TIER0,              "tier0"              },
	{ Library::CLIENT,             "client"             },
	{ Library::VGUIMATSURFACE,     "vguimatsurface"     },
	{ Library::MATERIALSYSTEM,     "materialsystem"     },
	{ Library::SOUNDEMITTERSYSTEM, "soundemittersystem" },
	{ Library::DATACACHE,          "datacache"          },
	{ Library::VGUI,               "vgui"               },
	{ Library::VPHYSICS,           "vphysics"           },
	{ Library::VSTDLIB,            "vstdlib"            },
	{ Library::SOURCEMODCORE,      "sourcemodcore"      },
	{ Library::VSCRIPT,            "vscript"            },
};

static std::map<Segment, const char *> segnames{
	{ Segment::INVALID, "invalid" },
	{ Segment::TEXT,    "text"    },
	{ Segment::DATA,    "data"    },
	{ Segment::RODATA,  "rodata"  },
	{ Segment::BSS,     "bss"     },
};

static std::map<std::string, Segment> segnames_plat{
#if defined _LINUX
	{ ".text",   Segment::TEXT,   },
	{ ".data",   Segment::DATA,   },
	{ ".rodata", Segment::RODATA, },
	{ ".bss",    Segment::BSS,    },
#elif defined _WINDOWS
	{ ".text",   Segment::TEXT,   },
	{ ".data",   Segment::DATA,   },
	{ ".rdata",  Segment::RODATA, },
	{ ".bss",    Segment::BSS,    },
#elif defined _OSX
	#error TODO
#endif
};


void LibMgr::Load()
{
	for (const auto& pair : libnames) {
		Library lib = pair.first;
		if (lib == Library::INVALID) continue;
		
		if (GetPtr(lib) == nullptr) continue;
		
		void *handle = OpenLibHandle(lib);
		if (handle != nullptr) {
			s_LibHandles[lib] = handle;
			
			/* pre-populate the library info structs */
			(void)GetInfo(lib);
		}
	}
}

void LibMgr::Unload()
{
	for (const auto& pair : s_LibHandles) {
		CloseLibHandle(pair.second);
	}
	s_LibHandles.clear();
}


void *LibMgr::GetPtr(Library lib)
{
	const auto it = s_LibPtrs.find(lib);
	if (it == s_LibPtrs.end()) return nullptr;
	
	return (*it).second;
}


bool LibMgr::HaveLib(Library lib)
{
	if (GetPtr(lib) == nullptr)       return false;
	if (s_LibHandles.count(lib) == 0) return false;
	
	return true;
}


const LibInfo& LibMgr::GetInfo(Library lib)
{
	if (s_LibInfos.find(lib) == s_LibInfos.end()) {
		FindInfo(lib);
	}
	
	return s_LibInfos.at(lib);
}


void LibMgr::FindInfo(Library lib)
{
	ConVarRef developer("developer");
	DynLibInfo dlinfo;
	memset(&dlinfo, 0x00, sizeof(dlinfo));
	assert(g_MemUtils.GetLibraryInfo(GetPtr(lib), dlinfo));
	
	LibInfo lib_info((uintptr_t)dlinfo.baseAddress, (uintptr_t)dlinfo.memorySize);
	
#if defined _LINUX
	g_MemUtils.ForEachSection(s_LibHandles[lib], [&](const ElfSHeader *shdr, const char *name){
		SegInfo seg_info(shdr->sh_addr, shdr->sh_size);
		seg_info.m_LibBaseAddr = lib_info.BaseAddr();
		
		auto it = segnames_plat.find(name);
		if (it != segnames_plat.end()) {
			if (developer.GetBool())
				ConColorMsg(Color(0x00, 0xff, 0x00, 0xff), "Library %s: segment '%s' found: %s\n",
					Lib_ToString(lib), name, Seg_ToString((*it).second));
			auto result = lib_info.m_SegmentsByType.insert(std::make_pair((*it).second, seg_info));
			assert(result.second);
		} else {
			if (developer.GetBool())
				ConColorMsg(Color(0xff, 0x00, 0x00, 0xff), "Library %s: segment '%s' not found!\n",
					Lib_ToString(lib), name);
		}
		
		auto result = lib_info.m_SegmentsByName.insert(std::make_pair(name, seg_info));
		assert(result.second);
	});
#elif defined _WINDOWS
	g_MemUtils.ForEachSection(s_LibHandles[lib], [&](const IMAGE_SECTION_HEADER *pSectHdr){
		SegInfo seg_info(pSectHdr->VirtualAddress, pSectHdr->Misc.VirtualSize);
		seg_info.m_LibBaseAddr = lib_info.BaseAddr();
		
		auto name = (const char *)pSectHdr->Name;
		
		auto it = segnames_plat.find(name);
		if (it != segnames_plat.end()) {
			auto result = lib_info.m_SegmentsByType.insert(std::make_pair((*it).second, seg_info));
			assert(result.second);
		}
		
		auto result = lib_info.m_SegmentsByName.insert(std::make_pair(name, seg_info));
		assert(result.second);
	});
#endif
	
	auto result = s_LibInfos.insert(std::make_pair(lib, lib_info));
	assert(result.second);
	
	DevMsg("Library %-34s [ %08zx %08zx ]\n", libnames.at(lib), lib_info.AddrBegin(), lib_info.AddrEnd());
	for (const auto& pair : lib_info.m_SegmentsByName) {
		DevMsg("  %-40s [ %08zx %08zx ]\n", pair.first.c_str(), pair.second.AddrBegin(), pair.second.AddrEnd());
	}
}


void *LibMgr::FindSym(Library lib, const char *sym)
{
	if (!HaveLib(lib)) return nullptr;
	
	return g_MemUtils.ResolveSymbol(s_LibHandles.at(lib), sym);
}

std::tuple<bool, std::string, void *> LibMgr::FindSymRegex(Library lib, const char *pattern)
{
	if (!HaveLib(lib)) {
		return std::make_tuple(false, "", nullptr);
	}
	
#ifndef _MSC_VER
	#warning NEED try/catch for std::regex ctor!
#endif
	std::regex filter(pattern, std::regex_constants::ECMAScript);
	
	std::vector<Symbol> matches;
	
	LibMgr::ForEachSym(lib, [&](const Symbol& sym){
		if (std::regex_match(sym.name, filter, std::regex_constants::match_any)) {
			matches.push_back(sym);
			if (matches.size() > 1) return false;
		}
		
		return true;
	});
	
	if (matches.size() == 1) {
		return std::make_tuple(true, matches[0].name, matches[0].addr);
	} else {
		return std::make_tuple(false, "", nullptr);
	}
}


#if defined _LINUX || defined _OSX

void *LibMgr::OpenLibHandle(Library lib)
{
	void *ptr = GetPtr(lib);
	
	Dl_info info;
	if (dladdr(ptr, &info) == 0) {
		DevMsg("LibMgr::OpenLibHandle: can't find library \"%s\" in memory\n"
			"dladdr error:\n%s\n", libnames.at(lib), dlerror());
		return nullptr;
	}
	
	void *handle = dlopen(info.dli_fname, RTLD_NOW);
	if (handle == nullptr) {
		DevMsg("LibMgr::OpenLibHandle: can't load library \"%s\"\n"
			"dlopen error:\n%s\n", libnames.at(lib), dlerror());
	}
	return handle;
}

void LibMgr::CloseLibHandle(void *handle)
{
	if (handle != nullptr) {
		dlclose(handle);
	}
}

#elif defined _WINDOWS

void *LibMgr::OpenLibHandle(Library lib)
{
	void *ptr = GetPtr(lib);
	
	MEMORY_BASIC_INFORMATION mem;
	if (VirtualQuery(ptr, &mem, sizeof(mem)) == 0) {
		char err[4096];
		libsys->GetPlatformError(err, sizeof(err));
		
		DevMsg("LibMgr::OpenLibHandle: can't find library \"%s\" in memory\n"
			"VirtualQuery error:\n%s\n", libnames.at(lib), err);
		return nullptr;
	}
	
	return mem.AllocationBase;
}

void LibMgr::CloseLibHandle(void *handle)
{
}

#endif


Library LibMgr::WhichLibAtAddr(void *ptr)
{
	auto addr = (uintptr_t)ptr;
	
	for (const auto& pair : s_LibHandles) {
		Library lib = pair.first;
		if (lib == Library::INVALID) continue;
		
		const LibInfo& lib_info = GetInfo(lib);
		if (lib_info.ContainsAddr(addr, 1)) {
			return lib;
		}
	}
	
	return Library::INVALID;
}


Library LibMgr::Lib_FromString(const char *str)
{
	for (const auto& pair : libnames) {
		if (stricmp(pair.second, str) == 0) {
			return pair.first;
		}
	}
	
	return Library::INVALID;
}

const char *LibMgr::Lib_ToString(Library lib)
{
	for (const auto& pair : libnames) {
		if (pair.first == lib) {
			return pair.second;
		}
	}
	
	return libnames.at(Library::INVALID);
}

size_t LibMgr::Lib_MaxStringLen()
{
	size_t max = 0;
	
	for (const auto& pair : libnames) {
		max = Max(max, strlen(pair.second));
	}
	
	return max;
}


Segment LibMgr::Seg_FromString(const char *str)
{
	for (const auto& pair : segnames) {
		if (stricmp(pair.second, str) == 0) {
			return pair.first;
		}
	}
	
	return Segment::INVALID;
}

const char *LibMgr::Seg_ToString(Segment seg)
{
	for (const auto& pair : segnames) {
		if (pair.first == seg) {
			return pair.second;
		}
	}
	
	return segnames.at(Segment::INVALID);
}

size_t LibMgr::Seg_MaxStringLen()
{
	size_t max = 0;
	
	for (const auto& pair : segnames) {
		max = Max(max, strlen(pair.second));
	}
	
	return max;
}


CON_COMMAND(sig_list_signatures, "") {
	std::regex filter(args[1], std::regex_constants::ECMAScript);
	for (auto &[lib, libname] : libnames) {
		LibMgr::ForEachSym(lib, [&](const Symbol& sym){
			if (std::regex_match(sym.name, filter, std::regex_constants::match_any)) {
				Msg("Match: %s | %s\n", sym.name.c_str(), libname);
			}
			return true;
		});
	}
	
}