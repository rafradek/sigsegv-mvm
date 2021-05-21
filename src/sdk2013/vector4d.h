//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef VECTOR4D_H
#define VECTOR4D_H

#ifdef _WIN32
#pragma once
#endif

// public/mathlib/vector4d.h
#define _SIGSEGV_SDK2013_OVERRIDE__PUBLIC_MATHLIB_VECTOR4D_H 1
// sigsegv modifications [WRT: Valve SDK2013 0d8dceea 20150909 / AlliedModders 0ef5d3d4 20171105]
// - make ctors constexpr
// - make funcs constexpr
// - make vec4_(origin|invalid) constexpr

#include <math.h>
#include <stdlib.h>		// For rand(). We really need a library!
#include <float.h>
#if !defined( _X360 )
#include <xmmintrin.h>	// For SSE
#endif
#include "basetypes.h"	// For vec_t, put this somewhere else?
#include "tier0/dbg.h"
#include "mathlib/math_pfns.h"

// forward declarations
class Vector;
class Vector2D;

//=========================================================
// 4D Vector4D
//=========================================================

class Vector4D					
{
public:
	// Members
	vec_t x, y, z, w;

	// Construction/destruction
#ifdef _DEBUG
	constexpr Vector4D(void);
#else
	Vector4D(void) = default;
#endif
	constexpr Vector4D(vec_t X, vec_t Y, vec_t Z, vec_t W);
	constexpr Vector4D(const float *pFloat);

	// Initialization
	constexpr void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f, vec_t iw=0.0f);

	// Got any nasty NAN's?
	constexpr bool IsValid() const;

	// array access...
	constexpr vec_t operator[](int i) const;
	constexpr vec_t& operator[](int i);

	// Base address...
	constexpr vec_t* Base();
	constexpr vec_t const* Base() const;

	// Cast to Vector and Vector2D...
	constexpr Vector& AsVector3D();
	constexpr Vector const& AsVector3D() const;

	constexpr Vector2D& AsVector2D();
	constexpr Vector2D const& AsVector2D() const;

	// Initialization methods
	void Random( vec_t minVal, vec_t maxVal );

	// equality
	constexpr bool operator==(const Vector4D& v) const;
	constexpr bool operator!=(const Vector4D& v) const;	

	// arithmetic operations
	constexpr Vector4D&	operator+=(const Vector4D &v);			
	constexpr Vector4D&	operator-=(const Vector4D &v);		
	constexpr Vector4D&	operator*=(const Vector4D &v);			
	constexpr Vector4D&	operator*=(float s);
	constexpr Vector4D&	operator/=(const Vector4D &v);		
	constexpr Vector4D&	operator/=(float s);					

	// negate the Vector4D components
	constexpr void	Negate(); 

	// Get the Vector4D's magnitude.
	constexpr vec_t	Length() const;

	// Get the Vector4D's magnitude squared.
	constexpr vec_t	LengthSqr(void) const;

	// return true if this vector is (0,0,0,0) within tolerance
	constexpr bool IsZero( float tolerance = 0.01f ) const
	{
		return (x > -tolerance && x < tolerance &&
				y > -tolerance && y < tolerance &&
				z > -tolerance && z < tolerance &&
				w > -tolerance && w < tolerance);
	}

	// Get the distance from this Vector4D to the other one.
	constexpr vec_t	DistTo(const Vector4D &vOther) const;

	// Get the distance from this Vector4D to the other one squared.
	constexpr vec_t	DistToSqr(const Vector4D &vOther) const;		

	// Copy
	constexpr void	CopyToArray(float* rgfl) const;	

	// Multiply, add, and assign to this (ie: *this = a + b * scalar). This
	// is about 12% faster than the actual Vector4D equation (because it's done per-component
	// rather than per-Vector4D).
	constexpr void	MulAdd(Vector4D const& a, Vector4D const& b, float scalar);	

	// Dot product.
	constexpr vec_t	Dot(Vector4D const& vOther) const;			

	// No copy constructors allowed if we're in optimal mode
#ifdef VECTOR_NO_SLOW_OPERATIONS
private:
#else
public:
#endif
	constexpr Vector4D(Vector4D const& vOther);

	// No assignment operators either...
	constexpr Vector4D& operator=( Vector4D const& src );
};

static_assert(offsetof(Vector4D, x) == offsetof(Vector, x));
static_assert(offsetof(Vector4D, y) == offsetof(Vector, y));
static_assert(offsetof(Vector4D, z) == offsetof(Vector, z));

static_assert(offsetof(Vector4D, x) == offsetof(Vector2D, x));
static_assert(offsetof(Vector4D, y) == offsetof(Vector2D, y));

//-----------------------------------------------------------------------------
// SSE optimized routines
//-----------------------------------------------------------------------------

class ALIGN16 Vector4DAligned : public Vector4D
{
public:
	Vector4DAligned(void) : Vector4D() {}
	constexpr Vector4DAligned( vec_t X, vec_t Y, vec_t Z, vec_t W );

	constexpr void Set( vec_t X, vec_t Y, vec_t Z, vec_t W );
	void InitZero( void );

	constexpr __m128 &AsM128() { _CONSTEXPR_RETURN_TYPE_PUN(__m128); }
	constexpr const __m128 &AsM128() const { _CONSTEXPR_RETURN_TYPE_PUN(const __m128); }

private:
	// No copy constructors allowed if we're in optimal mode
	constexpr Vector4DAligned( Vector4DAligned const& vOther );

	// No assignment operators either...
	constexpr Vector4DAligned& operator=( Vector4DAligned const& src );
} ALIGN16_POST;

//-----------------------------------------------------------------------------
// Vector4D related operations
//-----------------------------------------------------------------------------

// Vector4D clear
constexpr void Vector4DClear( Vector4D& a );

// Copy
constexpr void Vector4DCopy( Vector4D const& src, Vector4D& dst );

// Vector4D arithmetic
constexpr void Vector4DAdd( Vector4D const& a, Vector4D const& b, Vector4D& result );
constexpr void Vector4DSubtract( Vector4D const& a, Vector4D const& b, Vector4D& result );
constexpr void Vector4DMultiply( Vector4D const& a, vec_t b, Vector4D& result );
constexpr void Vector4DMultiply( Vector4D const& a, Vector4D const& b, Vector4D& result );
constexpr void Vector4DDivide( Vector4D const& a, vec_t b, Vector4D& result );
constexpr void Vector4DDivide( Vector4D const& a, Vector4D const& b, Vector4D& result );
constexpr void Vector4DMA( Vector4D const& start, float s, Vector4D const& dir, Vector4D& result );

// Vector4DAligned arithmetic
constexpr void Vector4DMultiplyAligned( Vector4DAligned const& a, vec_t b, Vector4DAligned& result );


#define Vector4DExpand( v ) (v).x, (v).y, (v).z, (v).w

// Normalization
constexpr vec_t Vector4DNormalize( Vector4D& v );

// Length
constexpr vec_t Vector4DLength( Vector4D const& v );

// Dot Product
constexpr vec_t DotProduct4D(Vector4D const& a, Vector4D const& b);

// Linearly interpolate between two vectors
constexpr void Vector4DLerp(Vector4D const& src1, Vector4D const& src2, vec_t t, Vector4D& dest );


//-----------------------------------------------------------------------------
//
// Inlined Vector4D methods
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------

#ifdef _DEBUG
// Initialize to NAN to catch errors
constexpr Vector4D::Vector4D(void) : x(VEC_T_NAN), y(VEC_T_NAN), z(VEC_T_NAN), w(VEC_T_NAN) {}
#endif

constexpr Vector4D::Vector4D(vec_t X, vec_t Y, vec_t Z, vec_t W ) : x(X), y(Y), z(Z), w(W)
{
	Assert( IsValid() );
}

constexpr Vector4D::Vector4D(const float *pFloat) : x(pFloat[0]), y(pFloat[1]), z(pFloat[2]), w(pFloat[3])
{
	Assert( pFloat );
	Assert( IsValid() );
}


//-----------------------------------------------------------------------------
// copy constructor
//-----------------------------------------------------------------------------

constexpr Vector4D::Vector4D(const Vector4D &vOther) : x(vOther.x), y(vOther.y), z(vOther.z), w(vOther.w)
{
	Assert( vOther.IsValid() );
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

constexpr void Vector4D::Init( vec_t ix, vec_t iy, vec_t iz, vec_t iw )
{ 
	x = ix; y = iy; z = iz;	w = iw;
	Assert( IsValid() );
}

inline void Vector4D::Random( vec_t minVal, vec_t maxVal )
{
	x = minVal + ((vec_t)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	y = minVal + ((vec_t)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	z = minVal + ((vec_t)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
	w = minVal + ((vec_t)rand() / VALVE_RAND_MAX) * (maxVal - minVal);
}

constexpr void Vector4DClear( Vector4D& a )
{
	a.x = a.y = a.z = a.w = 0.0f;
}

//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------

constexpr Vector4D& Vector4D::operator=(const Vector4D &vOther)	
{
	Assert( vOther.IsValid() );
	x=vOther.x; y=vOther.y; z=vOther.z; w=vOther.w;
	return *this; 
}

//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------

constexpr vec_t& Vector4D::operator[](int i)
{
	Assert( (i >= 0) && (i < 4) );
	return Base()[i];
}

constexpr vec_t Vector4D::operator[](int i) const
{
	Assert( (i >= 0) && (i < 4) );
	return Base()[i];
}

//-----------------------------------------------------------------------------
// Cast to Vector and Vector2D...
//-----------------------------------------------------------------------------

constexpr Vector& Vector4D::AsVector3D()
{
	_CONSTEXPR_RETURN_TYPE_PUN(Vector);
}

constexpr Vector const& Vector4D::AsVector3D() const
{
	_CONSTEXPR_RETURN_TYPE_PUN(const Vector);
}

constexpr Vector2D& Vector4D::AsVector2D()
{
	_CONSTEXPR_RETURN_TYPE_PUN(Vector2D);
}

constexpr Vector2D const& Vector4D::AsVector2D() const
{
	_CONSTEXPR_RETURN_TYPE_PUN(const Vector2D);
}

//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------

constexpr vec_t* Vector4D::Base()
{
	static_assert(offsetof(Vector4D, x) == 0);
	return &x;
}

constexpr vec_t const* Vector4D::Base() const
{
	static_assert(offsetof(Vector4D, x) == 0);
	return &x;
}

//-----------------------------------------------------------------------------
// IsValid?
//-----------------------------------------------------------------------------

constexpr bool Vector4D::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z) && IsFinite(w);
}

//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

constexpr bool Vector4D::operator==( Vector4D const& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (src.x == x) && (src.y == y) && (src.z == z) && (src.w == w);
}

constexpr bool Vector4D::operator!=( Vector4D const& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (src.x != x) || (src.y != y) || (src.z != z) || (src.w != w);
}


//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------

constexpr void Vector4DCopy( Vector4D const& src, Vector4D& dst )
{
	Assert( src.IsValid() );
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
	dst.w = src.w;
}

constexpr void	Vector4D::CopyToArray(float* rgfl) const		
{ 
	Assert( IsValid() );
	Assert( rgfl );
	rgfl[0] = x; rgfl[1] = y; rgfl[2] = z; rgfl[3] = w;
}

//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------

constexpr void Vector4D::Negate()
{ 
	Assert( IsValid() );
	x = -x; y = -y; z = -z; w = -w;
} 

constexpr Vector4D& Vector4D::operator+=(const Vector4D& v)	
{ 
	Assert( IsValid() && v.IsValid() );
	x+=v.x; y+=v.y; z += v.z; w += v.w;	
	return *this;
}

constexpr Vector4D& Vector4D::operator-=(const Vector4D& v)	
{ 
	Assert( IsValid() && v.IsValid() );
	x-=v.x; y-=v.y; z -= v.z; w -= v.w;
	return *this;
}

constexpr Vector4D& Vector4D::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	z *= fl;
	w *= fl;
	Assert( IsValid() );
	return *this;
}

constexpr Vector4D& Vector4D::operator*=(Vector4D const& v)	
{ 
	x *= v.x;
	y *= v.y;
	z *= v.z;
	w *= v.w;
	Assert( IsValid() );
	return *this;
}

constexpr Vector4D& Vector4D::operator/=(float fl)	
{
	Assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	w *= oofl;
	Assert( IsValid() );
	return *this;
}

constexpr Vector4D& Vector4D::operator/=(Vector4D const& v)	
{ 
	Assert( v.x != 0.0f && v.y != 0.0f && v.z != 0.0f && v.w != 0.0f );
	x /= v.x;
	y /= v.y;
	z /= v.z;
	w /= v.w;
	Assert( IsValid() );
	return *this;
}

constexpr void Vector4DAdd( Vector4D const& a, Vector4D const& b, Vector4D& c )
{
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	c.z = a.z + b.z;
	c.w = a.w + b.w;
}

constexpr void Vector4DSubtract( Vector4D const& a, Vector4D const& b, Vector4D& c )
{
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
	c.w = a.w - b.w;
}

constexpr void Vector4DMultiply( Vector4D const& a, vec_t b, Vector4D& c )
{
	Assert( a.IsValid() && IsFinite(b) );
	c.x = a.x * b;
	c.y = a.y * b;
	c.z = a.z * b;
	c.w = a.w * b;
}

constexpr void Vector4DMultiply( Vector4D const& a, Vector4D const& b, Vector4D& c )
{
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x * b.x;
	c.y = a.y * b.y;
	c.z = a.z * b.z;
	c.w = a.w * b.w;
}

constexpr void Vector4DDivide( Vector4D const& a, vec_t b, Vector4D& c )
{
	Assert( a.IsValid() );
	Assert( b != 0.0f );
	vec_t oob = 1.0f / b;
	c.x = a.x * oob;
	c.y = a.y * oob;
	c.z = a.z * oob;
	c.w = a.w * oob;
}

constexpr void Vector4DDivide( Vector4D const& a, Vector4D const& b, Vector4D& c )
{
	Assert( a.IsValid() );
	Assert( (b.x != 0.0f) && (b.y != 0.0f) && (b.z != 0.0f) && (b.w != 0.0f) );
	c.x = a.x / b.x;
	c.y = a.y / b.y;
	c.z = a.z / b.z;
	c.w = a.w / b.w;
}

constexpr void Vector4DMA( Vector4D const& start, float s, Vector4D const& dir, Vector4D& result )
{
	Assert( start.IsValid() && IsFinite(s) && dir.IsValid() );
	result.x = start.x + s*dir.x;
	result.y = start.y + s*dir.y;
	result.z = start.z + s*dir.z;
	result.w = start.w + s*dir.w;
}

// FIXME: Remove
// For backwards compatability
constexpr void	Vector4D::MulAdd(Vector4D const& a, Vector4D const& b, float scalar)
{
	x = a.x + b.x * scalar;
	y = a.y + b.y * scalar;
	z = a.z + b.z * scalar;
	w = a.w + b.w * scalar;
}

constexpr void Vector4DLerp(const Vector4D& src1, const Vector4D& src2, vec_t t, Vector4D& dest )
{
	dest[0] = src1[0] + (src2[0] - src1[0]) * t;
	dest[1] = src1[1] + (src2[1] - src1[1]) * t;
	dest[2] = src1[2] + (src2[2] - src1[2]) * t;
	dest[3] = src1[3] + (src2[3] - src1[3]) * t;
}

//-----------------------------------------------------------------------------
// dot, cross
//-----------------------------------------------------------------------------

constexpr vec_t DotProduct4D(const Vector4D& a, const Vector4D& b) 
{ 
	Assert( a.IsValid() && b.IsValid() );
	return( a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w ); 
}

// for backwards compatability
constexpr vec_t Vector4D::Dot( Vector4D const& vOther ) const
{
	return DotProduct4D( *this, vOther );
}


//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------

constexpr vec_t Vector4DLength( Vector4D const& v )
{				   
	Assert( v.IsValid() );
	return (vec_t)FastSqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);		
}

constexpr vec_t Vector4D::LengthSqr(void) const	
{ 
	Assert( IsValid() );
	return (x*x + y*y + z*z + w*w);		
}

constexpr vec_t Vector4D::Length(void) const	
{
	return Vector4DLength( *this );
}


//-----------------------------------------------------------------------------
// Normalization
//-----------------------------------------------------------------------------

// FIXME: Can't use until we're un-macroed in mathlib.h
constexpr vec_t Vector4DNormalize( Vector4D& v )
{
	Assert( v.IsValid() );
	vec_t l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		v.x = v.y = v.z = v.w = 0.0f;
	}
	return l;
}

//-----------------------------------------------------------------------------
// Get the distance from this Vector4D to the other one 
//-----------------------------------------------------------------------------

constexpr vec_t Vector4D::DistTo(const Vector4D &vOther) const
{
	Vector4D delta = *this;
	delta -= vOther;
	return delta.Length();
}

constexpr vec_t Vector4D::DistToSqr(const Vector4D &vOther) const
{
	Vector4D delta = *this;
	delta -= vOther;
	return delta.LengthSqr();
}


//-----------------------------------------------------------------------------
// Vector4DAligned routines
//-----------------------------------------------------------------------------

constexpr Vector4DAligned::Vector4DAligned( vec_t X, vec_t Y, vec_t Z, vec_t W ) : Vector4D(X, Y, Z, W) {}

constexpr void Vector4DAligned::Set( vec_t X, vec_t Y, vec_t Z, vec_t W )
{
	x = X; y = Y; z = Z; w = W;
	Assert( IsValid() );
}

inline void Vector4DAligned::InitZero( void )
{ 
#if !defined( _X360 )
	this->AsM128() = _mm_set1_ps( 0.0f );
#else
	this->AsM128() = __vspltisw( 0 );
#endif
	Assert( IsValid() );
}

constexpr void Vector4DMultiplyAligned( Vector4DAligned const& a, Vector4DAligned const& b, Vector4DAligned& c )
{
	Assert( a.IsValid() && b.IsValid() );
#if !defined( _X360 )
	c.x = a.x * b.x;
	c.y = a.y * b.y;
	c.z = a.z * b.z;
	c.w = a.w * b.w;
#else
	c.AsM128() = __vmulfp( a.AsM128(), b.AsM128() );
#endif
}

constexpr void Vector4DWeightMAD( vec_t w, Vector4DAligned const& vInA, Vector4DAligned& vOutA, Vector4DAligned const& vInB, Vector4DAligned& vOutB )
{
	Assert( vInA.IsValid() && vInB.IsValid() && IsFinite(w) );

#if !defined( _X360 )
	vOutA.x += vInA.x * w;
	vOutA.y += vInA.y * w;
	vOutA.z += vInA.z * w;
	vOutA.w += vInA.w * w;

	vOutB.x += vInB.x * w;
	vOutB.y += vInB.y * w;
	vOutB.z += vInB.z * w;
	vOutB.w += vInB.w * w;
#else
    __vector4 temp;

    temp = __lvlx( &w, 0 );
    temp = __vspltw( temp, 0 );

	vOutA.AsM128() = __vmaddfp( vInA.AsM128(), temp, vOutA.AsM128() );
	vOutB.AsM128() = __vmaddfp( vInB.AsM128(), temp, vOutB.AsM128() );
#endif
}

inline void Vector4DWeightMADSSE( vec_t w, Vector4DAligned const& vInA, Vector4DAligned& vOutA, Vector4DAligned const& vInB, Vector4DAligned& vOutB )
{
	Assert( vInA.IsValid() && vInB.IsValid() && IsFinite(w) );

#if !defined( _X360 )
	// Replicate scalar float out to 4 components
    __m128 packed = _mm_set1_ps( w );

	// 4D SSE Vector MAD
	vOutA.AsM128() = _mm_add_ps( vOutA.AsM128(), _mm_mul_ps( vInA.AsM128(), packed ) );
	vOutB.AsM128() = _mm_add_ps( vOutB.AsM128(), _mm_mul_ps( vInB.AsM128(), packed ) );
#else
    __vector4 temp;

    temp = __lvlx( &w, 0 );
    temp = __vspltw( temp, 0 );

	vOutA.AsM128() = __vmaddfp( vInA.AsM128(), temp, vOutA.AsM128() );
	vOutB.AsM128() = __vmaddfp( vInB.AsM128(), temp, vOutB.AsM128() );
#endif
}

constexpr Vector4D vec4_origin( 0.0f, 0.0f, 0.0f, 0.0f );
constexpr Vector4D vec4_invalid( FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX );

#endif // VECTOR4D_H

