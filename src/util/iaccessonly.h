#ifndef _INCLUDE_SIGSEGV_UTIL_IACCESSONLY_H_
#define _INCLUDE_SIGSEGV_UTIL_IACCESSONLY_H_


/* fundamental base class type: ensures that derived classes cannot be created/destroyed/copied/moved */
class IAccessOnly
{
public:
	/* deleted: all possible normal constructors */
	template<typename... ARGS>
	explicit IAccessOnly(ARGS&&...) = delete;
	
	/* deleted: copy and move constructors */
	IAccessOnly(const IAccessOnly& ) = delete;
	IAccessOnly(      IAccessOnly&&) = delete;
	
	/* deleted: copy and move assignment operators */
	IAccessOnly& operator=(const IAccessOnly& ) = delete;
	IAccessOnly& operator=(      IAccessOnly&&) = delete;
	
//	/* deleted: destructor */
//	~IAccessOnly() = delete;
};


#ifdef DEBUG
static_assert( std::is_empty_v                <IAccessOnly>);
static_assert(!std::is_polymorphic_v          <IAccessOnly>);
static_assert(!std::is_default_constructible_v<IAccessOnly>);
static_assert(!std::is_copy_constructible_v   <IAccessOnly>);
static_assert(!std::is_move_constructible_v   <IAccessOnly>);
//static_assert(!std::is_destructible_v         <IAccessOnly>);
static_assert(!std::is_copy_assignable_v      <IAccessOnly>);
static_assert(!std::is_move_assignable_v      <IAccessOnly>);
static_assert(!std::is_swappable_v            <IAccessOnly>);
#endif


#endif
