#include "link/link.h"
#include "factory.h"
#include "util/misc.h"


//#if defined _WINDOWS
#define LINK_NONFATAL
//#endif


namespace Link
{
	bool link_finished = false;

	bool InitAll()
	{
		vtable_staticprop = RTTI::GetVTable("11CStaticProp");
		DevMsg("Link::InitAll BEGIN\n");
		
		for (auto link : AutoListNoDelete<ILinkage>::List()) {
			if (ClientFactory() == nullptr && link->ClientSide()) continue;

			link->InvokeLink();
			
			if (!link->IsLinked()) {
#if !defined LINK_NONFATAL
				DevMsg("Link::InitAll FAIL\n");
				return false;
#endif
			}
		}
		link_finished = true;
		DevMsg("Link::InitAll OK\n");
		return true;
	}

	void ListLinks()
	{
		size_t len_obj = 0;
	//	size_t len_mem = 0;
		for (auto prop : AutoListNoDelete<ILinkage>::List()) {
			if (prop->GetNameDebug() == nullptr) continue;
			len_obj = Max(len_obj, strlen(prop->GetNameDebug()));
	//		len_mem = Max(len_mem, strlen(prop->GetMemberName()));
		}
		
		std::vector<ILinkage *> links_sorted;
		for (auto prop : AutoListNoDelete<ILinkage>::List()) {
			if (prop->GetNameDebug() == nullptr) continue;
			links_sorted.push_back(prop);
		}
		// std::sort(links_sorted.begin(), links_sorted.end(), [](ILinkage *lhs, ILinkage *rhs){
		// 	std::string obj_lhs = lhs->GetNameDebug();
		// 	std::string obj_rhs = rhs->GetNameDebug();
		// 	if (obj_lhs != obj_rhs) return (obj_lhs < obj_rhs);
			
		// 	int off_lhs = INT_MAX; lhs->GetAddressDebug();
		// 	int off_rhs = INT_MAX; rhs->GetAddressDebug();
		// 	if (off_lhs != off_rhs) return (off_lhs < off_rhs);
		// });
		
		MAT_SINGLE_THREAD_BLOCK {
			Msg("%-12s  %11s  %-*s\n", "KIND", "ADDRESS", len_obj, "NAME");
			for (auto prop : links_sorted) {
				const char *n_name = prop->GetNameDebug();
				uintptr_t  n_addr = prop->GetAddressDebug();
				const char *kind  = prop->GetTypeDebug();

				Msg("%-12s  0x%09x  %-*s\n", kind, n_addr, len_obj, n_name);
			}
		}
	}

	void CC_ListLinks(const CCommand& cmd)
	{
		ListLinks();
	}
	static ConCommand ccmd_list_props("sig_list_linkage", &CC_ListLinks,
		"List functions/globals linkage", FCVAR_NONE);
}
