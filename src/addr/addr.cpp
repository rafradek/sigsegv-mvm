#include "addr/addr.h"
#include "util/misc.h"


void IAddr::Init()
{
	if (this->m_State != State::INITIAL) {
		return;
	}
	
	this->m_State = State::LOADING;
	
	bool result;
	if (LibMgr::HaveLib(this->GetLibrary())) {
		result = this->FindAddrCommon(this->m_iAddr);
	} else {
		DevMsg("IAddr: \"%s\": library not available: %s\n", this->GetName(), LibMgr::Lib_ToString(this->GetLibrary()));
		result = false;
	}
	
	if (result) {
		this->m_State = State::OK;
//		DevMsg("IAddr::Init \"%s\" OK 0x%08x\n", this->GetName(), this->m_iAddr);
	} else {
		this->m_State = State::FAIL;
		DevMsg("IAddr::Init \"%s\" FAIL\n", this->GetName());
	}
}


void AddrManager::Load()
{
	DevMsg("AddrManager::Load BEGIN\n");
	
	for (auto addr : AutoList<IAddr>::List()) {
		std::string name(addr->GetName());
		
		if (s_Addrs.find(name) == s_Addrs.end()) {
			s_Addrs[name] = addr;
		} else {
			DevMsg("AddrManager::Load: duplicate addr for \"%s\"\n", addr->GetName());
		}
	}
	
#if 0
	/* early pass 1: vtables */
	for (auto addr : AutoList<IAddr>::List()) {
		if (dynamic_cast<IAddr_VTable *>(addr) != nullptr) {
			addr->Init();
		}
	}
#endif
	
	s_bLoading = true;
	
	/* main pass */
	for (auto addr : AutoList<IAddr>::List()) {
		addr->Init();
		
		if (addr->GetState() == IAddr::State::OK && addr->GetAddr() != nullptr) {
			s_ReverseAddrs.emplace(addr->GetAddr(), addr->GetName());
		}
	}
	
	s_bLoading = false;
	
	DevMsg("AddrManager::Load END\n");
}


void *AddrManager::GetAddr(const char *name)
{
	auto it = s_Addrs.find(std::string(name));
	if (it == s_Addrs.end()) {
		DevMsg("AddrManager::GetAddr FAIL: no addr exists with name \"%s\"\n", name);
		return nullptr;
	}
	
	IAddr *addr = (*it).second;
	
	/* try to handle dependency ordering somewhat automatically */
	if (s_bLoading && addr->GetState() == IAddr::State::INITIAL) {
		addr->Init();
	}
	
	IAddr::State state = addr->GetState();
	assert(state != IAddr::State::INITIAL);
	assert(state != IAddr::State::LOADING);
	
	if (state == IAddr::State::FAIL) {
		DevMsg("AddrManager::GetAddr FAIL: cannot resolve addr for name \"%s\"\n", name);
		return nullptr;
	}
	
	return addr->GetAddr();
}

const char *AddrManager::ReverseLookup(const void *ptr)
{
	auto [begin, end] = s_ReverseAddrs.equal_range(ptr);
	auto count = std::distance(begin, end);
	
	if (count == 1) {
		return begin->second.c_str();
	} else {
		return nullptr;
	}
}


static bool CompareAddrsForSorting(const IAddr *lhs, const IAddr *rhs)
{
	auto lib_lhs = (int)lhs->GetLibrary();
	auto lib_rhs = (int)rhs->GetLibrary();
	if (lib_lhs != lib_rhs) {
		return (lib_lhs < lib_rhs);
	}
	
	std::string name_lhs(lhs->GetName());
	std::string name_rhs(rhs->GetName());
	if (name_lhs != name_rhs) {
		return (name_lhs < name_rhs);
	}
	
	return ((uintptr_t)lhs < (uintptr_t)rhs);
}

static ConCommand ccmd_list_addrs("sig_list_addrs", &AddrManager::CC_ListAddrs,
	"List addresses and show their status", FCVAR_NONE);
void AddrManager::CC_ListAddrs(const CCommand& cmd)
{
	std::vector<IAddr *> addrs_sorted;
	for (const auto& pair : s_Addrs) {
		addrs_sorted.push_back(pair.second);
	}
	std::sort(addrs_sorted.begin(), addrs_sorted.end(), CompareAddrsForSorting);
	
	size_t max_libname_len = LibMgr::Lib_MaxStringLen();
	
	MAT_SINGLE_THREAD_BLOCK {
		for (auto addr : addrs_sorted) {
			switch (addr->GetState()) {
			case IAddr::State::INITIAL:
				Msg("%-*s  %-8s  %s\n", max_libname_len, LibMgr::Lib_ToString(addr->GetLibrary()), "INITIAL", addr->GetName());
				break;
			case IAddr::State::OK:
				Msg("%-*s  %08x  %s\n", max_libname_len, LibMgr::Lib_ToString(addr->GetLibrary()), (uintptr_t)addr->GetAddr(), addr->GetName());
				break;
			case IAddr::State::FAIL:
				Msg("%-*s  %-8s  %s\n", max_libname_len, LibMgr::Lib_ToString(addr->GetLibrary()), "FAIL", addr->GetName());
				break;
			}
		}
	}
}
