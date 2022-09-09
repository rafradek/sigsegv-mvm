#include "link/link.h"
#include "abi.h"


#if defined __GNUC__

class Dummy {};


static MemberFuncThunk<Dummy *, void> ft_PathFollower_ctor_C1("PathFollower::PathFollower [C1]");
static MemberFuncThunk<Dummy *, void> ft_PathFollower_dtor_D1("PathFollower::~PathFollower [D1]");

static MemberFuncThunk<Dummy *, void> ft_Action_dtor_D2("Action<CTFBot>::~Action [D2]");

extern "C"
{
	/* ctors */
	void _ZN12PathFollowerC1Ev(void *_this) { ft_PathFollower_ctor_C1(reinterpret_cast<Dummy *>(_this)); }
	
	/* dtors */
	void _ZN12PathFollowerD1Ev(void *_this) { ft_PathFollower_dtor_D1(reinterpret_cast<Dummy *>(_this)); }
	void _ZN6ActionI6CTFBotED2Ev(void *_this) { ft_Action_dtor_D2(reinterpret_cast<Dummy *>(_this)); }
}


/* the definition of the vtable/RTTI symbols can't be in a translation unit that
 * includes the actual class definition */

#define THUNK_VTABLE(name, size) \
	extern "C" { uint8_t _ZTV##name[size]; } \
	static VTableThunk<sizeof(_ZTV##name)> thunk_vtable_##name(#name, _ZTV##name);

#define THUNK_TYPEINFO_BASE(name) \
	extern "C" { uint8_t _ZTI##name[sizeof(abi::__class_type_info)]; } \
	static TypeInfoThunk<sizeof(_ZTI##name)> thunk_typeinfo_##name(#name, _ZTI##name);
#define THUNK_TYPEINFO_SINGLE(name) \
	extern "C" { uint8_t _ZTI##name[sizeof(abi::__si_class_type_info)]; } \
	static TypeInfoThunk<sizeof(_ZTI##name)> thunk_typeinfo_##name(#name, _ZTI##name);
#define THUNK_TYPEINFO_VMULTI(name, nbases) \
	extern "C" { uint8_t _ZTI##name[sizeof(abi::__vmi_class_type_info) + ((nbases - 1) * sizeof(abi::__base_class_type_info))]; } \
	static TypeInfoThunk<sizeof(_ZTI##name)> thunk_typeinfo_##name(#name, _ZTI##name);


THUNK_TYPEINFO_BASE(12CKnownEntity);
THUNK_VTABLE(12CKnownEntity, 0x80);

THUNK_TYPEINFO_BASE(22INextBotEventResponder);
THUNK_VTABLE(22INextBotEventResponder, 0x100);

THUNK_TYPEINFO_BASE(16IContextualQuery);
THUNK_VTABLE(16IContextualQuery, 0x40);

THUNK_TYPEINFO_SINGLE(8INextBot);
THUNK_VTABLE(8INextBot, 0x200);

#elif defined _MSC_VER

#include "re/nextbot.h"
#include "re/path.h"

static MemberFuncThunk<Action<CTFBot> *, void> ft_Action_dtor_D2("Action<CTFBot>::~Action [D2]");

/* oh god... good luck declaring mangled symbols in C for MSVC++, given that
 * they use wonderful characters like "@$?"... */

/* nesting the dtor thunk inside an actual dtor is a bad idea, but it's better
 * than the horrible alternative */
template<> Action<CTFBot>::~Action() { ft_Action_dtor_D2(this); }

/* not implemented */
INextBotEventResponder::~INextBotEventResponder() { assert(false); }
IContextualQuery::~IContextualQuery()             { assert(false); }
//Path::Path()                                      { assert(false); }
//Path::~Path()                                     { assert(false); }
//PathFollower::PathFollower()                      { assert(false); }
//PathFollower::~PathFollower()                     { assert(false); }


// TODO: why didn't we have to hook up the RTTI?
// will dynamic_cast's be broken?

#endif
