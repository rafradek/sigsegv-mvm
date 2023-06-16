/* implementations for some functions from game/shared/util_shared.cpp */
#include "link/link.h"
#include "stub/baseentity.h"

float UTIL_VecToYaw( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
		return 0;
	
	float yaw = atan2( vec.y, vec.x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}

float UTIL_VecToPitch( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
	{
		if (vec.z < 0)
			return 180.0;
		else
			return -180.0;
	}

	float dist = vec.Length2D();
	float pitch = atan2( -vec.z, dist );

	pitch = RAD2DEG(pitch);

	return pitch;
}

Vector UTIL_YawToVector( float yaw )
{
	Vector ret;
	
	ret.z = 0;
	float angle = DEG2RAD( yaw );
	SinCos( angle, &ret.y, &ret.x );

	return ret;
}

StaticFuncThunk<void, CBaseEntity *, const Vector &, const Vector &, unsigned int, const IHandleEntity *, int, trace_t *> ft_UTIL_TraceEntity("UTIL_TraceEntity [IHandleEntity]");
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *pIgnore, int nCollisionGroup, trace_t *ptr )
{
	ft_UTIL_TraceEntity(pEntity, vecAbsStart, vecAbsEnd, mask, pIgnore, nCollisionGroup, ptr);
}

float IntervalTimer::Now( void ) const
{
	return gpGlobals->curtime;
}

float CountdownTimer::Now( void ) const
{
	return gpGlobals->curtime;
}
