#ifndef _INCLUDE_SIGSEGV_ABI_H_
#define _INCLUDE_SIGSEGV_ABI_H_


#if defined __clang__
#error TODO
#elif defined __GNUC__
#include <cxxabi.h>
#elif defined _MSC_VER
namespace abi
{
	typedef void __class_type_info;
}
#endif


/* GCC ABI */

struct vtable
{
	ptrdiff_t whole_object;
	const abi::__class_type_info *whole_type;
	void *vfptrs[0x1000];
};


/* MSVC ABI */

#if defined _MSC_VER
#pragma warning(disable:4200)
#endif

#if !defined _MSC_VER
struct _PMD
{
	int mdisp;
	int pdisp;
	int vdisp;
};

struct _TypeDescriptor
{
	const void *pVFTable;
	void *spare;
	char name[];
};
#endif

struct __RTTI_BaseClassDescriptor
{
	_TypeDescriptor *pTypeDescriptor;
	unsigned long numContainedBases;
	_PMD where;
	unsigned long attributes;
};

struct __RTTI_BaseClassArray
{
	__RTTI_BaseClassDescriptor *arrayOfBaseClassDescriptors[0];
};

struct __RTTI_ClassHierarchyDescriptor
{
	unsigned long signature;
	unsigned long offset;
	unsigned long numBaseClasses;
	__RTTI_BaseClassArray *pBaseClassArray;
};

struct __RTTI_CompleteObjectLocator
{
	unsigned long signature;
	unsigned long offset;
	unsigned long cdOffset;
	_TypeDescriptor *pTypeDescriptor;
	__RTTI_ClassHierarchyDescriptor *pClassDescriptor;
};

#if defined _MSC_VER
#pragma warning(default:4200)
#endif

#if defined _MSC_VER
/* from VC/crt/src/vcruntime/rtti.cpp */
extern "C" PVOID __CLRCALL_OR_CDECL __RTDynamicCast (
	PVOID inptr,
	LONG VfDelta,
//	const _TypeDescriptor *SrcType,
//	const _TypeDescriptor *TargetType,
	PVOID SrcType,
	PVOID TargetType,
	BOOL isReference
	) throw(...);
#endif


/* calling conventions */

/* common naming for fastcall */
#if !defined __fastcall && defined __GNUC__ && !defined __clang__
#define __fastcall [[gnu::fastcall]]
#endif

/* use EAX/EDX/ECX register calling convention in GCC build ONLY */
#if defined __GNUC__ && !defined __clang__
#define __gcc_regcall [[gnu::regparm(3)]]
#else
#define __gcc_regcall
#endif

/* use thiscall calling convention in MSVC build ONLY */
#if defined _MSC_VER
#define __msvc_thiscall __thiscall
#else
#define __msvc_thiscall
#endif


/* standard-breaking function pointer conversions */

template<class C, typename RET, typename... PARAMS> using MemberPtrType      = RET (C::*)(PARAMS...);
template<class C, typename RET, typename... PARAMS> using MemberPtrTypeConst = RET (C::*)(PARAMS...) const;

template<class C, typename RET, typename... PARAMS> using MemberPtrTypeVa      = RET (C::*)(PARAMS..., ...);
template<class C, typename RET, typename... PARAMS> using MemberPtrTypeVaConst = RET (C::*)(PARAMS..., ...) const;

#if defined __clang__

#error TODO

#elif defined __GNUC__

template<class C, typename RET, typename... PARAMS>
union MemberPtrUnion
{
	MemberPtrType<C, RET, PARAMS...> fptr;
	struct {
		const void *ptr;
		ptrdiff_t delta;
	} guts;
};


template<class C, typename RET, typename... PARAMS>
MemberPtrType<C, RET, PARAMS...> MakePtrToMemberFunc(const void *ptr)
{
	MemberPtrUnion<C, RET, PARAMS...> u;
	
	u.guts.ptr   = ptr;
	u.guts.delta = 0;
	
	return u.fptr;
}
template<class C, typename RET, typename... PARAMS>
MemberPtrTypeConst<C, RET, PARAMS...> MakePtrToConstMemberFunc(const void *ptr)
{
	return reinterpret_cast<MemberPtrTypeConst<C, RET, PARAMS...>>(MakePtrToMemberFunc<C, RET, PARAMS...>(ptr));
}


template<class C, typename RET, typename... PARAMS>
void *GetAddrOfMemberFunc(MemberPtrType<C, RET, PARAMS...> ptr)
{
	MemberPtrUnion<C, RET, PARAMS...> u;
	
	u.fptr = ptr;
	
	assert((uintptr_t)u.guts.ptr % 2 == 0);
	return (void *)u.guts.ptr;
}
template<class C, typename RET, typename... PARAMS>
void *GetAddrOfMemberFunc(MemberPtrTypeConst<C, RET, PARAMS...> ptr)
{
	return GetAddrOfMemberFunc(reinterpret_cast<MemberPtrType<C, RET, PARAMS...>>(ptr));
}
template<class C, typename RET, typename... PARAMS>
void *GetAddrOfMemberFunc(MemberPtrTypeVa<C, RET, PARAMS...> ptr)
{
	return GetAddrOfMemberFunc(reinterpret_cast<MemberPtrType<C, RET, PARAMS...>>(ptr));
}
template<class C, typename RET, typename... PARAMS>
void *GetAddrOfMemberFunc(MemberPtrTypeVaConst<C, RET, PARAMS...> ptr)
{
	return GetAddrOfMemberFunc(reinterpret_cast<MemberPtrType<C, RET, PARAMS...>>(ptr));
}


template<class C, typename RET, typename... PARAMS>
int GetVIdxOfMemberFunc(MemberPtrType<C, RET, PARAMS...> ptr)
{
	MemberPtrUnion<C, RET, PARAMS...> u;
	
	u.fptr = ptr;
	
	assert((uintptr_t)u.guts.ptr % 4 == 1);
	return ((int)u.guts.ptr - 1) / 4;
}
template<class C, typename RET, typename... PARAMS>
int GetVIdxOfMemberFunc(MemberPtrTypeVa<C, RET, PARAMS...> ptr)
{
	return GetVIdxOfMemberFunc(reinterpret_cast<MemberPtrType<C, RET, PARAMS...>>(ptr));
}

#elif defined _MSC_VER

template<class C, typename RET, typename... PARAMS>
union MemberPtrUnion {
	MemberPtrType<C, RET, PARAMS...> fptr;
	struct {
		const void *ptr;
		ptrdiff_t delta;
		ptrdiff_t vtordisp;
		ptrdiff_t vtidx;
	} guts;
};


template<class C, typename RET, typename... PARAMS>
MemberPtrType<C, RET, PARAMS...> MakePtrToMemberFunc(const void *ptr)
{
	MemberPtrUnion<C, RET, PARAMS...> u;
	
	u.guts.ptr      = ptr;
	u.guts.delta    = 0;
	u.guts.vtordisp = 0;
	u.guts.vtidx    = 0;
	
	return u.fptr;
}
template<class C, typename RET, typename... PARAMS>
MemberPtrTypeConst<C, RET, PARAMS...> MakePtrToConstMemberFunc(const void *ptr)
{
	return reinterpret_cast<MemberPtrTypeConst<C, RET, PARAMS...>>(MakePtrToMemberFunc<C, RET, PARAMS...>(ptr));
}


template<class C, typename RET, typename... PARAMS>
void *GetAddrOfMemberFunc(MemberPtrType<C, RET, PARAMS...> ptr)
{
	MemberPtrUnion<C, RET, PARAMS...> u;
	
	u.fptr = ptr;
	
	// incremental linking breaks this, because the func ptr will end up really being a ptr to a jmp thunk
	assert((uintptr_t)u.guts.ptr % 2 == 0);
	return (void *)u.guts.ptr;
}
template<class C, typename RET, typename... PARAMS>
void *GetAddrOfMemberFunc(MemberPtrTypeConst<C, RET, PARAMS...> ptr)
{
	return GetAddrOfMemberFunc(reinterpret_cast<MemberPtrType<C, RET, PARAMS...>>(ptr));
}
template<class C, typename RET, typename... PARAMS>
void *GetAddrOfMemberFunc(MemberPtrTypeVa<C, RET, PARAMS...> ptr)
{
	return GetAddrOfMemberFunc(reinterpret_cast<MemberPtrType<C, RET, PARAMS...>>(ptr));
}
template<class C, typename RET, typename... PARAMS>
void *GetAddrOfMemberFunc(MemberPtrTypeVaConst<C, RET, PARAMS...> ptr)
{
	return GetAddrOfMemberFunc(reinterpret_cast<MemberPtrType<C, RET, PARAMS...>>(ptr));
}


template<class C, typename RET, typename... PARAMS>
int GetVIdxOfMemberFunc(MemberPtrType<C, RET, PARAMS...> ptr)
{
	return -1;
	
	// TODO:
	// make union
	// assign ptr to u.fptr
	// now, u.guts.ptr should point to a __thiscall thunk like this:
	//   mov eax,[ecx]
	//   jmp dword ptr [eax+VTOFF]
	// we need proper disassembler support so we can do this robustly
	// (e.g. [eax] vs [eax+4] is different in bytes, but disasm can handle it seamlessly)
}
template<class C, typename RET, typename... PARAMS>
int GetVIdxOfMemberFunc(MemberPtrTypeVa<C, RET, PARAMS...> ptr)
{
	return GetVIdxOfMemberFunc(reinterpret_cast<MemberPtrType<C, RET, PARAMS...>>(ptr));
}

#endif


#endif
