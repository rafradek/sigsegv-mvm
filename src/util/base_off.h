#ifndef _INCLUDE_SIGSEGV_UTIL_BASE_OFF_H_
#define _INCLUDE_SIGSEGV_UTIL_BASE_OFF_H_


/* for complete types where the definitions are known, these functions make it
 * easy to determine the offset of a particular base class in a derived class
 * (for multiple inheritance situations, of course) */


// needs a ptr; the second template argument is auto-deduced from the ptr type
template<typename BASE, typename DERIVED>
FORCEINLINE ptrdiff_t base_off(const DERIVED *derived)
{
	static_assert(std::is_base_of_v<BASE, DERIVED>);
	
	auto base = static_cast<const BASE *>(derived);
	return ((uintptr_t)base - (uintptr_t)derived);
}

// doesn't need a ptr; but needs both template arguments
template<typename DERIVED, typename BASE>
FORCEINLINE ptrdiff_t base_off()
{
	/* invent a fake ptr as far from nullptr as possible (0x8000'0000 for 32-bit) */
	return base_off<BASE>((const DERIVED *)((UINTPTR_MAX / 2) + 1));
}


#endif
