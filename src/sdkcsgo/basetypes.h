// This is the preferred Min operator. Using the MIN macro can lead to unexpected
// side-effects or more expensive code.
template< class T >
constexpr T Min( T const &val1, T const &val2 )
{
	return val1 < val2 ? val1 : val2;
}

// This is the preferred Max operator. Using the MAX macro can lead to unexpected
// side-effects or more expensive code.
template< class T >
constexpr T Max( T const &val1, T const &val2 )
{
	return val1 > val2 ? val1 : val2;
}
typedef uint32 HMODULE;