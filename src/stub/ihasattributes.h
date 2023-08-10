#ifndef _INCLUDE_SIGSEGV_STUB_IHASATTRIBUTES_H_
#define _INCLUDE_SIGSEGV_STUB_IHASATTRIBUTES_H_

class CAttributeManager;
class CAttributeContainer;
class CBaseEntity;
class CAttributeList;

class IHasAttributes
{
public:
    virtual CAttributeManager	*GetAttributeManager() = 0;
	virtual CAttributeContainer	*GetAttributeContainer() = 0;
	virtual CBaseEntity			*GetAttributeOwner() = 0;
	virtual CAttributeList		*GetAttributeList() = 0;
	virtual void				ReapplyProvision() = 0;
};

#endif