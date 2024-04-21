#ifndef _INCLUDE_SIGSEGV_SDKTF2_MATHCOMPAT_H_
#define _INCLUDE_SIGSEGV_SDKTF2_MATHCOMPAT_H_

template< class T >
T Clamp( T const &val, T const &minVal, T const &maxVal )
{
	if( val < minVal )
		return minVal;
	else if( val > maxVal )
		return maxVal;
	else
		return val;
}

// This is the preferred Min operator. Using the MIN macro can lead to unexpected
// side-effects or more expensive code.
template< class T >
T Min( T const &val1, T const &val2 )
{
	return val1 < val2 ? val1 : val2;
}

// This is the preferred Max operator. Using the MAX macro can lead to unexpected
// side-effects or more expensive code.
template< class T >
T Max( T const &val1, T const &val2 )
{
	return val1 > val2 ? val1 : val2;
}

#endif