#include "link/link.h"
#include "factory.h"


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
		
		for (auto link : AutoList<ILinkage>::List()) {
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

}
