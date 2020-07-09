//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef VECTOR2D_H
#define VECTOR2D_H

#ifdef _WIN32
#pragma once
#endif

// public/mathlib/vector2d.h
#define _SIGSEGV_SDK2013_OVERRIDE__PUBLIC_MATHLIB_VECTOR2D_H 1
// sigsegv modifications [WRT: Valve SDK2013 0d8dceea 20150909 / AlliedModders 0ef5d3d4 20171105]
// - make ctors constexpr
// - make funcs constexpr
// - make vec2_(origin|invalid) constexpr

#include <math.h>
#include <float.h>

// For vec_t, put this somewhere else?
#include "tier0/basetypes.h"

// For rand(). We really need a library!
#include <stdlib.h>

#include "tier0/dbg.h"
#include "mathlib/math_pfns.h"

//=========================================================
// 2D Vector2D
//=========================================================

class Vector2D					
{
public:
	// Members
	vec_t x, y;

	// Construction/destruction
#ifdef _DEBUG
	constexpr Vector2D(void);
#else
	Vector2D(void) = default;
#endif
	constexpr Vector2D(vec_t X, vec_t Y);
	constexpr Vector2D(const float *pFloat);

	// Initialization
	constexpr void Init(vec_t ix=0.0f, vec_t iy=0.0f);

	// Got any nasty NAN's?
	constexpr bool IsValid() const;

	// array access...
	constexpr vec_t operator[](int i) const;
	constexpr vec_t& operator[](int i);

	// Base address...
	constexpr vec_t* Base();
	constexpr vec_t const* Base() const;

	// Initialization methods
	void Random( float minVal, float maxVal );

	// equality
	constexpr bool operator==(const Vector2D& v) const;
	constexpr bool operator!=(const Vector2D& v) const;	

	// arithmetic operations
	constexpr Vector2D&	operator+=(const Vector2D &v);			
	constexpr Vector2D&	operator-=(const Vector2D &v);		
	constexpr Vector2D&	operator*=(const Vector2D &v);			
	constexpr Vector2D&	operator*=(float s);
	constexpr Vector2D&	operator/=(const Vector2D &v);		
	constexpr Vector2D&	operator/=(float s);					

	// negate the Vector2D components
	constexpr void	Negate(); 

	// Get the Vector2D's magnitude.
	constexpr vec_t	Length() const;

	// Get the Vector2D's magnitude squared.
	constexpr vec_t	LengthSqr(void) const;

	// return true if this vector is (0,0) within tolerance
	constexpr bool IsZero( float tolerance = 0.01f ) const
	{
		return (x > -tolerance && x < tolerance &&
				y > -tolerance && y < tolerance);
	}

	// Normalize in place and return the old length.
	constexpr vec_t	NormalizeInPlace();

	// Compare length.
	constexpr bool	IsLengthGreaterThan( float val ) const;
	constexpr bool	IsLengthLessThan( float val ) const;

	// Get the distance from this Vector2D to the other one.
	constexpr vec_t	DistTo(const Vector2D &vOther) const;

	// Get the distance from this Vector2D to the other one squared.
	constexpr vec_t	DistToSqr(const Vector2D &vOther) const;		

	// Copy
	constexpr void	CopyToArray(float* rgfl) const;	

	// Multiply, add, and assign to this (ie: *this = a + b * scalar). This
	// is about 12% faster than the actual Vector2D equation (because it's done per-component
	// rather than per-Vector2D).
	constexpr void	MulAdd(const Vector2D& a, const Vector2D& b, float scalar);	

	// Dot product.
	constexpr vec_t	Dot(const Vector2D& vOther) const;			

	// assignment
	constexpr Vector2D& operator=(const Vector2D &vOther);

#ifndef VECTOR_NO_SLOW_OPERATIONS
	// copy constructors
	constexpr Vector2D(const Vector2D &vOther);

	// arithmetic operations
	constexpr Vector2D	operator-(void) const;
				
	constexpr Vector2D	operator+(const Vector2D& v) const;	
	constexpr Vector2D	operator-(const Vector2D& v) const;	
	constexpr Vector2D	operator*(const Vector2D& v) const;	
	constexpr Vector2D	operator/(const Vector2D& v) const;	
	constexpr Vector2D	operator*(float fl) const;
	constexpr Vector2D	operator/(float fl) const;			
	
	// Cross product between two vectors.
	constexpr Vector2D	Cross(const Vector2D &vOther) const;		

	// Returns a Vector2D with the min or max in X, Y, and Z.
	constexpr Vector2D	Min(const Vector2D &vOther) const;
	constexpr Vector2D	Max(const Vector2D &vOther) const;

#else

private:
	// No copy constructors allowed if we're in optimal mode
	constexpr Vector2D(const Vector2D& vOther);
#endif
};

//-----------------------------------------------------------------------------
// Vector2D related operations
//-----------------------------------------------------------------------------

// Vector2D clear
constexpr void Vector2DClear( Vector2D& a );

// Copy
constexpr void Vector2DCopy( const Vector2D& src, Vector2D& dst );

// Vector2D arithmetic
constexpr void Vector2DAdd( const Vector2D& a, const Vector2D& b, Vector2D& result );
constexpr void Vector2DSubtract( const Vector2D& a, const Vector2D& b, Vector2D& result );
constexpr void Vector2DMultiply( const Vector2D& a, vec_t b, Vector2D& result );
constexpr void Vector2DMultiply( const Vector2D& a, const Vector2D& b, Vector2D& result );
constexpr void Vector2DDivide( const Vector2D& a, vec_t b, Vector2D& result );
constexpr void Vector2DDivide( const Vector2D& a, const Vector2D& b, Vector2D& result );
constexpr void Vector2DMA( const Vector2D& start, float s, const Vector2D& dir, Vector2D& result );

// Store the min or max of each of x, y, and z into the result.
constexpr void Vector2DMin( const Vector2D &a, const Vector2D &b, Vector2D &result );
constexpr void Vector2DMax( const Vector2D &a, const Vector2D &b, Vector2D &result );

#define Vector2DExpand( v ) (v).x, (v).y

// Normalization
constexpr vec_t Vector2DNormalize( Vector2D& v );

// Length
constexpr vec_t Vector2DLength( const Vector2D& v );

// Dot Product
constexpr vec_t DotProduct2D(const Vector2D& a, const Vector2D& b);

// Linearly interpolate between two vectors
constexpr void Vector2DLerp(const Vector2D& src1, const Vector2D& src2, vec_t t, Vector2D& dest );


//-----------------------------------------------------------------------------
//
// Inlined Vector2D methods
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------

#ifdef _DEBUG
// Initialize to NAN to catch errors
constexpr Vector2D::Vector2D(void) : x(VEC_T_NAN), y(VEC_T_NAN) {}
#endif

constexpr Vector2D::Vector2D(vec_t X, vec_t Y) : x(X), y(Y)
{
	Assert( IsValid() );
}

constexpr Vector2D::Vector2D(const float *pFloat) : x(pFloat[0]), y(pFloat[1])
{
	Assert( pFloat );
	Assert( IsValid() );
}


//-----------------------------------------------------------------------------
// copy constructor
//-----------------------------------------------------------------------------

constexpr Vector2D::Vector2D(const Vector2D &vOther) : x(vOther.x), y(vOther.y)
{
	Assert( vOther.IsValid() );
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

constexpr void Vector2D::Init( vec_t ix, vec_t iy )    
{ 
	x = ix; y = iy;
	Assert( IsValid() );
}

inline void Vector2D::Random( float minVal, float maxVal )
{
	x = minVal + ((float)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	y = minVal + ((float)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
}

constexpr void Vector2DClear( Vector2D& a )
{
	a.x = a.y = 0.0f;
}

//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------

constexpr Vector2D& Vector2D::operator=(const Vector2D &vOther)
{
	Assert( vOther.IsValid() );
	x=vOther.x; y=vOther.y;
	return *this; 
}

//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------

constexpr vec_t& Vector2D::operator[](int i)
{
	Assert( (i >= 0) && (i < 2) );
	return ((vec_t*)this)[i];
}

constexpr vec_t Vector2D::operator[](int i) const
{
	Assert( (i >= 0) && (i < 2) );
	return ((vec_t*)this)[i];
}

//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------

constexpr vec_t* Vector2D::Base()
{
	return (vec_t*)&x;
}

constexpr vec_t const* Vector2D::Base() const
{
	return (vec_t const*)&x;
}

//-----------------------------------------------------------------------------
// IsValid?
//-----------------------------------------------------------------------------

constexpr bool Vector2D::IsValid() const
{
	return IsFinite(x) && IsFinite(y);
}

//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

constexpr bool Vector2D::operator==( const Vector2D& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (src.x == x) && (src.y == y);
}

constexpr bool Vector2D::operator!=( const Vector2D& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (src.x != x) || (src.y != y);
}


//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------

constexpr void Vector2DCopy( const Vector2D& src, Vector2D& dst )
{
	Assert( src.IsValid() );
	dst.x = src.x;
	dst.y = src.y;
}

constexpr void	Vector2D::CopyToArray(float* rgfl) const		
{ 
	Assert( IsValid() );
	Assert( rgfl );
	rgfl[0] = x; rgfl[1] = y; 
}

//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------

constexpr void Vector2D::Negate()
{ 
	Assert( IsValid() );
	x = -x; y = -y;
} 

constexpr Vector2D& Vector2D::operator+=(const Vector2D& v)	
{ 
	Assert( IsValid() && v.IsValid() );
	x+=v.x; y+=v.y;	
	return *this;
}

constexpr Vector2D& Vector2D::operator-=(const Vector2D& v)	
{ 
	Assert( IsValid() && v.IsValid() );
	x-=v.x; y-=v.y;	
	return *this;
}

constexpr Vector2D& Vector2D::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	Assert( IsValid() );
	return *this;
}

constexpr Vector2D& Vector2D::operator*=(const Vector2D& v)	
{ 
	x *= v.x;
	y *= v.y;
	Assert( IsValid() );
	return *this;
}

constexpr Vector2D& Vector2D::operator/=(float fl)	
{
	Assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	Assert( IsValid() );
	return *this;
}

constexpr Vector2D& Vector2D::operator/=(const Vector2D& v)	
{ 
	Assert( v.x != 0.0f && v.y != 0.0f );
	x /= v.x;
	y /= v.y;
	Assert( IsValid() );
	return *this;
}

constexpr void Vector2DAdd( const Vector2D& a, const Vector2D& b, Vector2D& c )
{
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x + b.x;
	c.y = a.y + b.y;
}

constexpr void Vector2DSubtract( const Vector2D& a, const Vector2D& b, Vector2D& c )
{
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x - b.x;
	c.y = a.y - b.y;
}

constexpr void Vector2DMultiply( const Vector2D& a, vec_t b, Vector2D& c )
{
	Assert( a.IsValid() && IsFinite(b) );
	c.x = a.x * b;
	c.y = a.y * b;
}

constexpr void Vector2DMultiply( const Vector2D& a, const Vector2D& b, Vector2D& c )
{				  
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x * b.x;
	c.y = a.y * b.y;
}


constexpr void Vector2DDivide( const Vector2D& a, vec_t b, Vector2D& c )
{
	Assert( a.IsValid() );
	Assert( b != 0.0f );
	vec_t oob = 1.0f / b;
	c.x = a.x * oob;
	c.y = a.y * oob;
}

constexpr void Vector2DDivide( const Vector2D& a, const Vector2D& b, Vector2D& c )
{
	Assert( a.IsValid() );
	Assert( (b.x != 0.0f) && (b.y != 0.0f) );
	c.x = a.x / b.x;
	c.y = a.y / b.y;
}

constexpr void Vector2DMA( const Vector2D& start, float s, const Vector2D& dir, Vector2D& result )
{
	Assert( start.IsValid() && IsFinite(s) && dir.IsValid() );
	result.x = start.x + s*dir.x;
	result.y = start.y + s*dir.y;
}

// FIXME: Remove
// For backwards compatability
constexpr void	Vector2D::MulAdd(const Vector2D& a, const Vector2D& b, float scalar)
{
	x = a.x + b.x * scalar;
	y = a.y + b.y * scalar;
}

constexpr void Vector2DLerp(const Vector2D& src1, const Vector2D& src2, vec_t t, Vector2D& dest )
{
	dest[0] = src1[0] + (src2[0] - src1[0]) * t;
	dest[1] = src1[1] + (src2[1] - src1[1]) * t;
}

//-----------------------------------------------------------------------------
// dot, cross
//-----------------------------------------------------------------------------
constexpr vec_t DotProduct2D(const Vector2D& a, const Vector2D& b) 
{ 
	Assert( a.IsValid() && b.IsValid() );
	return( a.x*b.x + a.y*b.y ); 
}

// for backwards compatability
constexpr vec_t Vector2D::Dot( const Vector2D& vOther ) const
{
	return DotProduct2D( *this, vOther );
}


//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------
constexpr vec_t Vector2DLength( const Vector2D& v )
{
	Assert( v.IsValid() );
	return (vec_t)FastSqrt(v.x*v.x + v.y*v.y);		
}

constexpr vec_t Vector2D::LengthSqr(void) const	
{ 
	Assert( IsValid() );
	return (x*x + y*y);		
}

constexpr vec_t Vector2D::NormalizeInPlace()
{
	return Vector2DNormalize( *this );
}

constexpr bool Vector2D::IsLengthGreaterThan( float val ) const
{
	return LengthSqr() > val*val;
}

constexpr bool Vector2D::IsLengthLessThan( float val ) const
{
	return LengthSqr() < val*val;
}

constexpr vec_t Vector2D::Length(void) const	
{
	return Vector2DLength( *this );
}


constexpr void Vector2DMin( const Vector2D &a, const Vector2D &b, Vector2D &result )
{
	result.x = (a.x < b.x) ? a.x : b.x;
	result.y = (a.y < b.y) ? a.y : b.y;
}


constexpr void Vector2DMax( const Vector2D &a, const Vector2D &b, Vector2D &result )
{
	result.x = (a.x > b.x) ? a.x : b.x;
	result.y = (a.y > b.y) ? a.y : b.y;
}


//-----------------------------------------------------------------------------
// Normalization
//-----------------------------------------------------------------------------
constexpr vec_t Vector2DNormalize( Vector2D& v )
{
	Assert( v.IsValid() );
	vec_t l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		v.x = v.y = 0.0f; 
	}
	return l;
}


//-----------------------------------------------------------------------------
// Get the distance from this Vector2D to the other one 
//-----------------------------------------------------------------------------
constexpr vec_t Vector2D::DistTo(const Vector2D &vOther) const
{
	Vector2D delta = Vector2D();
	Vector2DSubtract( *this, vOther, delta );
	return delta.Length();
}

constexpr vec_t Vector2D::DistToSqr(const Vector2D &vOther) const
{
	Vector2D delta;
	Vector2DSubtract( *this, vOther, delta );
	return delta.LengthSqr();
}


//-----------------------------------------------------------------------------
// Computes the closest point to vecTarget no farther than flMaxDist from vecStart
//-----------------------------------------------------------------------------
constexpr void ComputeClosestPoint2D( const Vector2D& vecStart, float flMaxDist, const Vector2D& vecTarget, Vector2D *pResult )
{
	Vector2D vecDelta;
	Vector2DSubtract( vecTarget, vecStart, vecDelta );
	float flDistSqr = vecDelta.LengthSqr();
	if ( flDistSqr <= flMaxDist * flMaxDist )
	{
		*pResult = vecTarget;
	}
	else
	{
		vecDelta /= FastSqrt( flDistSqr );
		Vector2DMA( vecStart, flMaxDist, vecDelta, *pResult );
	}
}



//-----------------------------------------------------------------------------
//
// Slow methods
//
//-----------------------------------------------------------------------------

#ifndef VECTOR_NO_SLOW_OPERATIONS

//-----------------------------------------------------------------------------
// Returns a Vector2D with the min or max in X, Y, and Z.
//-----------------------------------------------------------------------------

constexpr Vector2D Vector2D::Min(const Vector2D &vOther) const
{
	return Vector2D(x < vOther.x ? x : vOther.x, 
		y < vOther.y ? y : vOther.y);
}

constexpr Vector2D Vector2D::Max(const Vector2D &vOther) const
{
	return Vector2D(x > vOther.x ? x : vOther.x, 
		y > vOther.y ? y : vOther.y);
}


//-----------------------------------------------------------------------------
// arithmetic operations
//-----------------------------------------------------------------------------

constexpr Vector2D Vector2D::operator-(void) const
{ 
	return Vector2D(-x,-y);				
}

constexpr Vector2D Vector2D::operator+(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DAdd( *this, v, res );
	return res;	
}

constexpr Vector2D Vector2D::operator-(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DSubtract( *this, v, res );
	return res;	
}

constexpr Vector2D Vector2D::operator*(float fl) const	
{ 
	Vector2D res;
	Vector2DMultiply( *this, fl, res );
	return res;	
}

constexpr Vector2D Vector2D::operator*(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DMultiply( *this, v, res );
	return res;	
}

constexpr Vector2D Vector2D::operator/(float fl) const	
{ 
	Vector2D res;
	Vector2DDivide( *this, fl, res );
	return res;	
}

constexpr Vector2D Vector2D::operator/(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DDivide( *this, v, res );
	return res;	
}

constexpr Vector2D operator*(float fl, const Vector2D& v)	
{ 
	return v * fl; 
}

#endif //slow

constexpr Vector2D vec2_origin(0,0);
constexpr Vector2D vec2_invalid( FLT_MAX, FLT_MAX );

#endif // VECTOR2D_H

