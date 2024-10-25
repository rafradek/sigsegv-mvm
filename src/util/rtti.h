#ifndef _INCLUDE_SIGSEGV_UTIL_RTTI_H_
#define _INCLUDE_SIGSEGV_UTIL_RTTI_H_


#include "abi.h"
#include "util/demangle.h"

#if defined __clang__
typedef abi::__class_type_info rtti_t;
#elif defined __GNUC__
typedef abi::__class_type_info rtti_t;
#elif defined _MSC_VER
typedef _TypeDescriptor rtti_t;
#endif


#if defined __clang__
#define TYPEID_RAW_NAME(typeinfo) typeinfo.name()
#elif defined __GNUC__
#define TYPEID_RAW_NAME(typeinfo) typeinfo.name()
#elif defined _MSC_VER
#define TYPEID_RAW_NAME(typeinfo) typeinfo.raw_name()
#endif

template<typename T>
const char *TypeName(bool demangle = false)
{
	/* the standard says typeid(type) shouldn't ever actually throw */
	try {
		const std::type_info& typeinfo = typeid(T);
		
		if (demangle) {
			return DemangleTypeName(typeinfo);
		} else {
			return TYPEID_RAW_NAME(typeinfo);
		}
	} catch (const std::bad_typeid& e) {
		Msg("%s: caught std::bad_typeid: %s\n", __PRETTY_FUNCTION__, e.what());
		return "<bad_typeid>";
	}
}

template<typename T>
const char *TypeName(T *t, bool demangle = false)
{
	/* the standard says typeid(expression) will throw if expression is nullptr */
	try {
		const std::type_info& typeinfo = typeid(*t);
		
		if (demangle) {
			return DemangleTypeName(typeinfo);
		} else {
			return TYPEID_RAW_NAME(typeinfo);
		}
	} catch (const std::bad_typeid& e) {
		Msg("%s: with 0x%08X: caught std::bad_typeid: %s\n", __PRETTY_FUNCTION__, (uintptr_t)t, e.what());
		return "<bad_typeid>";
	}
}

#undef TYPEID_NAME


namespace RTTI
{
	struct VTableInfo
	{
		void **vtable;
		size_t size;
	};

	void PreLoad();
	
	const rtti_t    *GetRTTI  (const char *name);
	const void     **GetVTable(const char *name);
	const VTableInfo GetVTableInfo(const char *name);
	
	template<typename T> const rtti_t *GetRTTI()   { return GetRTTI  (TypeName<T>()); }
	template<typename T> const void  **GetVTable() { return GetVTable(TypeName<T>()); }

	const std::unordered_map<std::string, const rtti_t *> &GetAllRTTI();
	const std::unordered_map<std::string, const void **>  &GetAllVTable();
	const std::unordered_map<std::string, VTableInfo>     &GetAllVTableInfo();
}


template<typename TO, typename FROM>
inline TO rtti_cast(const FROM ptr)
{
	if (ptr == nullptr) {
		return nullptr;
	}
	static_assert(std::is_pointer_v<FROM>, "rtti_cast FROM parameter isn't a pointer type");
	static_assert(std::is_pointer_v<TO>,   "rtti_cast TO parameter isn't a pointer type");

	static bool initialized = false;
	static const rtti_t *rtti_from;
	static const rtti_t *rtti_to;

	if (!initialized) {
		initialized = true;	
		rtti_from = RTTI::GetRTTI<std::remove_pointer_t<FROM>>();
		rtti_to   = RTTI::GetRTTI<std::remove_pointer_t<TO>>();
		
		assert(rtti_from != nullptr);
		assert(rtti_to   != nullptr);
	}

#if defined __GNUC__ || defined __clang__
	void *result = (void *)ptr;
	static std::vector<std::pair<void *, uintptr_t>> dynamic_cast_cache;
	void *vtable = *((void**)result);

	int c = dynamic_cast_cache.size();
	for (int i = 0; i < c; i++) {
		auto &pair = dynamic_cast_cache[i];
		if (pair.first == vtable) {
			return pair.second != UINTPTR_MAX ? reinterpret_cast<TO>((uintptr_t)result + (uintptr_t)pair.second) : nullptr;
		}
	}
	if (!static_cast<const std::type_info *>(rtti_from)->__do_upcast(rtti_to, &result))
		result = abi::__dynamic_cast(result, rtti_from, rtti_to, -1);
	dynamic_cast_cache.push_back({vtable, result != nullptr ? (uintptr_t)result - (uintptr_t)ptr : UINTPTR_MAX});

#elif defined _MSC_VER
	/* MSVC's __RTDynamicCast will happily do runtime up-casts and down-casts */
	void *result = __RTDynamicCast((void *)ptr, 0, (void *)rtti_from, (void *)rtti_to, false);
#endif

	return reinterpret_cast<TO>(result);
}

template<typename TO, typename FROM, typename BASE>
inline TO rtti_scast(const BASE ptr)
{
	return rtti_scast<TO>(static_cast<FROM>(ptr));
}

template<typename TO, typename FROM>
inline TO rtti_scast(const FROM ptr)
{
	static_assert(std::is_pointer_v<FROM>, "rtti_cast FROM parameter isn't a pointer type");
	static_assert(std::is_pointer_v<TO>,   "rtti_cast TO parameter isn't a pointer type");

	static bool initialized = false;
	static const rtti_t *rtti_from;
	static const rtti_t *rtti_to;
#if defined __GNUC__ || defined __clang__
	static uintptr_t upcast_offset = 0;
#endif

	if (!initialized && ptr != nullptr) {
		initialized = true;	
		rtti_from = RTTI::GetRTTI<std::remove_pointer_t<FROM>>();
		rtti_to   = RTTI::GetRTTI<std::remove_pointer_t<TO>>();
		
		assert(rtti_from != nullptr);
		assert(rtti_to   != nullptr);
	
#if defined __GNUC__ || defined __clang__
	/* GCC's __dynamic_cast is grumpy and won't do up-casts at runtime, so we
	 * have to manually take care of up-casting ourselves */
	 	
		void *upcast_ptr = (void *)ptr;
	
		if (static_cast<const std::type_info *>(rtti_from)->__do_upcast(rtti_to, &upcast_ptr)) {
			upcast_offset = (uintptr_t)upcast_ptr - (uintptr_t)ptr;
		}
		else if (static_cast<const std::type_info *>(rtti_to)->__do_upcast(rtti_from, &upcast_ptr)){
			upcast_offset = -(uintptr_t)upcast_ptr + (uintptr_t)ptr;
		}
#endif
	}

#if defined __GNUC__ || defined __clang__
	void *result = (void *)ptr;
	result = (void *) ((uintptr_t)result + upcast_offset);

#elif defined _MSC_VER
	/* MSVC's __RTDynamicCast will happily do runtime up-casts and down-casts */
	void *result = __RTDynamicCast((void *)ptr, 0, (void *)rtti_from, (void *)rtti_to, false);
#endif

	return reinterpret_cast<TO>(result);
}

#if 0
template<class TO, class FROM>
inline TO __fastcall jit_cast(const FROM ptr);

template<class TO, class FROM>
inline TO __fastcall jit_cast(const FROM ptr)
{
	
#if defined __GNUC__
	__asm__ volatile ("nop; nop; nop; nop; nop" : : : "memory");
	
	// INITIAL: nop pad
	// LATER:   compare with nullptr and conditionally do the add/subtract adjustment
#elif defined _MSC_VER
	__asm
	{
		nop
		nop
		nop
		nop
		nop
	}
	
	// INITIAL: nop pad
	// LATER:   compare with nullptr and conditionally do the add/subtract adjustment
#else
#error
#endif
	
	return (TO)((uintptr_t)ptr + 0x20);
	
	// TODO: use "long nop" instead of multiple short ones
	
	// for the actual execution here, either just return (for offset == 0)
	// or do one signed 32-bit addition and return
	
	// if execution reaches here, then we need to JIT the front of the function
	// and then probably jmp or call back to the start of it
}

// TODO: to avoid repeatedly flushing the cache each time we JIT a new jit_cast instance,
// we might consider pre-JIT'ing every possible cast combination ahead of time
// (perhaps even just the ones that are actually used by the code, if possible)

// TODO: we need to be absolutely sure that we clean up any JIT pages we allocate (if even applicable)

// TODO: if modifying our own executable pages, make sure to un-write-protect, then JIT, then re-write-protect
#endif


#endif
