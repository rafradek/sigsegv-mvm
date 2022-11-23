#ifndef _INCLUDE_SIGSEGV_STUB_TEMPENT_H_
#define _INCLUDE_SIGSEGV_STUB_TEMPENT_H_

#include "link/link.h"
class CBaseTempEntity
{
public:
	const char *GetName() const { return m_pszName; }
	CBaseTempEntity *GetNext() const { return m_pNext; }

	ServerClass *GetServerClass() const { return vt_GetServerClass(this); }
	void Create(IRecipientFilter *filter, float delay = 0.0) const { return vt_Create(this, filter, delay); }
	
	
private:
	static MemberVFuncThunk<const CBaseTempEntity *, ServerClass *> vt_GetServerClass;
	static MemberVFuncThunk<const CBaseTempEntity *, void, IRecipientFilter *, float> vt_Create;
	
	uintptr_t vtable;
	const char *m_pszName;
	CBaseTempEntity	*m_pNext;
};

#endif