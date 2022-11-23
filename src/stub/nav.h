#ifndef _INCLUDE_SIGSEGV_STUB_NAV_H_
#define _INCLUDE_SIGSEGV_STUB_NAV_H_


#include "prop.h"
#include "link/link.h"
#include "stub/tf_shareddefs.h"
#include "stub/entities.h"


class CBaseObject;
class CTFBotPathCost;


enum NavErrorType : int32_t;


enum GetNavAreaFlags_t : uint32_t
{
	GETNAVAREA_CHECK_LOS           = 0x1,
	GETNAVAREA_ALLOW_BLOCKED_AREAS = 0x2,
	GETNAVAREA_CHECK_GROUND        = 0x4,
};


enum NavAttributeType : uint32_t
{
	NAV_MESH_INVALID		= 0,
	NAV_MESH_CROUCH			= 0x00000001,				// must crouch to use this node/area
	NAV_MESH_JUMP			= 0x00000002,				// must jump to traverse this area (only used during generation)
	NAV_MESH_PRECISE		= 0x00000004,				// do not adjust for obstacles, just move along area
	NAV_MESH_NO_JUMP		= 0x00000008,				// inhibit discontinuity jumping
	NAV_MESH_STOP			= 0x00000010,				// must stop when entering this area
	NAV_MESH_RUN			= 0x00000020,				// must run to traverse this area
	NAV_MESH_WALK			= 0x00000040,				// must walk to traverse this area
	NAV_MESH_AVOID			= 0x00000080,				// avoid this area unless alternatives are too dangerous
	NAV_MESH_TRANSIENT		= 0x00000100,				// area may become blocked, and should be periodically checked
	NAV_MESH_DONT_HIDE		= 0x00000200,				// area should not be considered for hiding spot generation
	NAV_MESH_STAND			= 0x00000400,				// bots hiding in this area should stand
	NAV_MESH_NO_HOSTAGES	= 0x00000800,				// hostages shouldn't use this area
	NAV_MESH_STAIRS			= 0x00001000,				// this area represents stairs, do not attempt to climb or jump them - just walk up
	NAV_MESH_NO_MERGE		= 0x00002000,				// don't merge this area with adjacent areas
	NAV_MESH_OBSTACLE_TOP	= 0x00004000,				// this nav area is the climb point on the tip of an obstacle
	NAV_MESH_CLIFF			= 0x00008000,				// this nav area is adjacent to a drop of at least CliffHeight

	NAV_MESH_FIRST_CUSTOM	= 0x00010000,				// apps may define custom app-specific bits starting with this value
	NAV_MESH_LAST_CUSTOM	= 0x04000000,				// apps must not define custom app-specific bits higher than with this value

	NAV_MESH_FUNC_COST		= 0x20000000,				// area has designer specified cost controlled by func_nav_cost entities
	NAV_MESH_HAS_ELEVATOR	= 0x40000000,				// area is in an elevator's path
	NAV_MESH_NAV_BLOCKER	= 0x80000000				// area is blocked by nav blocker ( Alas, needed to hijack a bit in the attributes to get within a cache line [7/24/2008 tom])
};


/* from mvm-reversed/server/tf/nav_mesh/tf_nav_area.h */
enum TFNavAttributeType : uint32_t
{
	BLOCKED                     = (1 <<  0),
	
	RED_SPAWN_ROOM              = (1 <<  1),
	BLUE_SPAWN_ROOM             = (1 <<  2),
	SPAWN_ROOM_EXIT             = (1 <<  3),
	
	AMMO                        = (1 <<  4),
	HEALTH                      = (1 <<  5),
	
	CONTROL_POINT               = (1 <<  6),
	
	BLUE_SENTRY                 = (1 <<  7),
	RED_SENTRY                  = (1 <<  8),
	
	/* bit  9: unused */
	/* bit 10: unused */
	
	BLUE_SETUP_GATE             = (1 << 11),
	RED_SETUP_GATE              = (1 << 12),
	
	BLOCKED_AFTER_POINT_CAPTURE = (1 << 13),
	BLOCKED_UNTIL_POINT_CAPTURE = (1 << 14),
	
	BLUE_ONE_WAY_DOOR           = (1 << 15),
	RED_ONE_WAY_DOOR            = (1 << 16),
	
	WITH_SECOND_POINT           = (1 << 17),
	WITH_THIRD_POINT            = (1 << 18),
	WITH_FOURTH_POINT           = (1 << 19),
	WITH_FIFTH_POINT            = (1 << 20),
	
	SNIPER_SPOT                 = (1 << 21),
	SENTRY_SPOT                 = (1 << 22),
	
	/* bit 23: unused */
	/* bit 24: unused */
	
	NO_SPAWNING                 = (1 << 25),
	RESCUE_CLOSET               = (1 << 26),
	BOMB_DROP                   = (1 << 27),
	DOOR_NEVER_BLOCKS           = (1 << 28),
	DOOR_ALWAYS_BLOCKS          = (1 << 29),
	UNBLOCKABLE                 = (1 << 30),
	
	/* bit 31: unused */
};


struct Extent
{
	Vector lo, hi;

	void Init( void )
	{
		lo.Init();
		hi.Init();
	}

	void Init( CBaseEntity *entity )
	{
		entity->CollisionProp()->WorldSpaceSurroundingBounds( &lo, &hi );
	}

	float SizeX( void ) const	{ return hi.x - lo.x; }
	float SizeY( void ) const	{ return hi.y - lo.y; }
	float SizeZ( void ) const	{ return hi.z - lo.z; }
	float Area( void ) const	{ return SizeX() * SizeY(); }

	// Increase bounds to contain the given point
	void Encompass( const Vector &pos )
	{
		for ( int i=0; i<3; ++i )
		{
			if ( pos[i] < lo[i] )
			{
				lo[i] = pos[i];
			}
			else if ( pos[i] > hi[i] )
			{
				hi[i] = pos[i];
			}
		}
	}

	// Increase bounds to contain the given extent
	void Encompass( const Extent &extent )
	{
		Encompass( extent.lo );
		Encompass( extent.hi );
	}

	// return true if 'pos' is inside of this extent
	bool Contains( const Vector &pos ) const
	{
		return (pos.x >= lo.x && pos.x <= hi.x &&
				pos.y >= lo.y && pos.y <= hi.y &&
				pos.z >= lo.z && pos.z <= hi.z);
	}
	
	// return true if this extent overlaps the given one
	bool IsOverlapping( const Extent &other ) const
	{
		return (lo.x <= other.hi.x && hi.x >= other.lo.x &&
				lo.y <= other.hi.y && hi.y >= other.lo.y &&
				lo.z <= other.hi.z && hi.z >= other.lo.z);
	}

	// return true if this extent completely contains the given one
	bool IsEncompassing( const Extent &other, float tolerance = 0.0f ) const
	{
		return (lo.x <= other.lo.x + tolerance && hi.x >= other.hi.x - tolerance &&
				lo.y <= other.lo.y + tolerance && hi.y >= other.hi.y - tolerance &&
				lo.z <= other.lo.z + tolerance && hi.z >= other.hi.z - tolerance);
	}
};
const float StepHeight = 18.0f;

class CFuncNavCost : public CBaseEntity
{
public:
	bool HasTag(const char *groupname) const { return ft_HasTag(this, groupname); }
	
//	CUtlVector<CFmtStr>& GetTagsVector() { return *reinterpret_cast<CUtlVector<CFmtStr> *>((uintptr_t)this->m_iszTags.GetPtr() + sizeof(string_t)); }
	
	DECL_DATAMAP(int,                  m_team);
	DECL_DATAMAP(bool,                 m_isDisabled);
	DECL_DATAMAP(string_t,             m_iszTags);
	DECL_RELATIVE(CUtlVector<CFmtStr>, m_tags);
	
private:
	static MemberFuncThunk<const CFuncNavCost *, bool, const char *> ft_HasTag;
};

class CFuncNavAvoid : public CFuncNavCost {};
class CFuncNavPrefer : public CFuncNavCost {};

struct AreaBindInfo							// for pointer loading and binding
{
	union
	{
		CNavArea *area;
		unsigned int id;
	};

	unsigned char attributes;				// VisibilityType

	bool operator==( const AreaBindInfo &other ) const
	{
		return ( area == other.area );
	}
};

struct NavConnect
{
	union
	{
		unsigned int id;
		CNavArea *area;
	};

	mutable float length;
};

#define MAX_NAV_TEAMS 2
#define NUM_DIRECTIONS 4

class CNavAreaCriticalData
{
protected:
	/* 0  */	Vector m_nwCorner;											// north-west corner position (2D mins)
	/* 12 */	Vector m_seCorner;											// south-east corner position (2D maxs)
	/* 24 */	float m_invDxCorners;
	/* 28 */	float m_invDyCorners;
	/* 32 */	float m_neZ;												// height of the implicit corner defined by (m_seCorner.x, m_nwCorner.y, m_neZ)
	/* 36 */	float m_swZ;												// height of the implicit corner defined by (m_nwCorner.x, m_seCorner.y, m_neZ)
	/* 40 */	Vector m_center;											// centroid of area

	/* 52 */	unsigned char m_playerCount[ MAX_NAV_TEAMS ];				// the number of players currently in this area

	/* 54 */	bool m_isBlocked[ MAX_NAV_TEAMS ];							// if true, some part of the world is preventing movement through this nav area

	/* 56 */	unsigned int m_marker;										// used to flag the area as visited
	/* 60 */	float m_totalCost;											// the distance so far plus an estimate of the distance left
	/* 64 */	float m_costSoFar;											// distance travelled so far

	/* 68 */	CNavArea *m_nextOpen, *m_prevOpen;							// only valid if m_openMarker == m_masterMarker
	/* 76 */	unsigned int m_openMarker;									// if this equals the current marker value, we are on the open list

	/* 80 */	int	m_attributeFlags;										// set of attribute bit flags (see NavAttributeType)
	/* 84 */	CUtlVectorUltraConservative<NavConnect, CUtlVectorUltraConservativeAllocator> m_connect[ NUM_DIRECTIONS ];				// a list of adjacent areas for each direction
};

class CNavArea : public CNavAreaCriticalData
{
public:
	const Vector& GetCenter() const    { return this->m_center; }
	bool HasAttributes(int bits) const { return ((this->m_attributeFlags & bits) != 0); }
	CNavArea *GetParent() const        { return this->m_parent; }
	int GetParentHow() const           { return this->m_parentHow; }
	float GetCostSoFar() const         { return this->m_costSoFar; }
	void SetCostSoFar(float cost)      { this->m_costSoFar = cost; }
	void SetParent(CNavArea *parent)   { this->m_parent = parent; }
	void SetTotalCost(float cost)      { this->m_totalCost = cost; }
	
	int GetAdjacentCount(int dir) const	{ return m_connect[dir].Count(); }
	CNavArea *GetAdjacentArea( int dir, int i ) const {
		if ((i < 0) || (i >= m_connect[dir].Count()))
			return nullptr;
		return m_connect[dir][i].area;
	}
	CUtlVectorUltraConservative<NavConnect, CUtlVectorUltraConservativeAllocator> &GetAdjacentVector(int dir)	{ return m_connect[dir]; }

	void GetExtent(Extent *extent) const                                                            {        ft_GetExtent                            (this, extent); }
	void GetClosestPointOnArea(const Vector& pos, Vector *close) const                              {        ft_GetClosestPointOnArea                (this, &pos, close); }
	float ComputeAdjacentConnectionHeightChange(const CNavArea *destinationArea) const              { return ft_ComputeAdjacentConnectionHeightChange(this, destinationArea); }
	float GetZ(float x, float y) const                                                              { return ft_GetZ                                 (this, x, y); }
	void DrawFilled(int r, int g, int b, int a, float deltaT, bool noDepthTest, float margin) const {        vt_DrawFilled                           (this, r, g, b, a, deltaT, noDepthTest, margin); }
	bool Contains(const Vector &vec) const                                                          { return ft_Contains                             (this, vec); }
	bool IsOverlapping(const Vector &vec, float tolerance) const                                    { return ft_IsOverlapping                        (this, vec, tolerance); }
	void RemoveFromOpenList()                                                                       { return ft_RemoveFromOpenList                   (this); }
	void AddToOpenList()                                                                            { return ft_AddToOpenList                        (this); }
	bool IsBlocked(int teamID, bool ignoreNavBlockers = false) const                                { return vt_IsBlocked                            (this, teamID, ignoreNavBlockers); }
	bool IsPotentiallyVisibleToTeam(int teamID) const                                               { return vt_IsPotentiallyVisibleToTeam           (this, teamID); }
	void CollectAdjacentAreas(CUtlVector<CNavArea *> *vector)                                       { return ft_CollectAdjacentAreas                 (this, vector); }
	

	DECL_EXTRACT(CUtlVector<CHandle<CFuncNavCost>>, m_funcNavCostVector);
	DECL_EXTRACT (CUtlVectorConservative<AreaBindInfo>, m_potentiallyVisibleAreas);
	DECL_RELATIVE(AreaBindInfo, m_inheritVisibilityFrom);

	static void ClearSearchLists()	{ ft_ClearSearchLists(); }
	static bool IsOpenListEmpty()	{ return m_openList.GetRef() == nullptr; }
	static void MakeNewMarker()		{ ++m_masterMarker; if (m_masterMarker == 0) m_masterMarker.GetRef() = 1; }
	void Mark()						{ m_marker = m_masterMarker; }
	BOOL IsMarked() const			{ return (m_marker == m_masterMarker) ? true : false; }
	static GlobalThunk<uint> m_masterMarker;
	static GlobalThunk<CNavArea *> m_openList;
	
	static CNavArea *PopOpenList() {
		if (m_openList) {
			CNavArea *area = m_openList;
		
			area->RemoveFromOpenList();
			area->m_prevOpen = NULL;
			area->m_nextOpen = NULL;

			return area;
		}
		return nullptr;
	}
private:
	DECL_EXTRACT (Vector,     m_center);
	DECL_EXTRACT (int,        m_attributeFlags_a);
	DECL_RELATIVE(CNavArea *, m_parent);
	DECL_RELATIVE(int,        m_parentHow);
	//DECL_EXTRACT (float,      m_costSoFar);
	
	static MemberFuncThunk <const CNavArea *, void, Extent *>                               ft_GetExtent;
	static MemberFuncThunk <const CNavArea *, void, const Vector *, Vector *>               ft_GetClosestPointOnArea;
	static MemberFuncThunk <const CNavArea *, float, const CNavArea *>                      ft_ComputeAdjacentConnectionHeightChange;
	static MemberFuncThunk <const CNavArea *, float, float, float>                          ft_GetZ;
	static MemberVFuncThunk<const CNavArea *, void, int, int, int, int, float, bool, float> vt_DrawFilled;
	static MemberFuncThunk <const CNavArea *, bool, const Vector &>                         ft_Contains;
	static MemberFuncThunk <const CNavArea *, bool, const Vector &, float>                  ft_IsOverlapping;
	static MemberVFuncThunk<const CNavArea *, bool, int, bool>                              vt_IsBlocked;
	static MemberFuncThunk <      CNavArea *, void>                                         ft_AddToOpenList;
	static MemberFuncThunk <      CNavArea *, void>                                         ft_RemoveFromOpenList;
	static MemberVFuncThunk<const CNavArea *, bool, int>                                    vt_IsPotentiallyVisibleToTeam;
	static MemberFuncThunk <      CNavArea *, void, CUtlVector<CNavArea *> *>               ft_CollectAdjacentAreas;

	static StaticFuncThunk<void>                              ft_ClearSearchLists;
};

class CTFNavArea : public CNavArea
{
public:
	TFNavAttributeType GetTFAttributes() const          { return this->m_nAttributes; }
	bool HasTFAttributes(TFNavAttributeType attr) const { return ((this->m_nAttributes & attr) != 0); }
	bool IsInCombat() const                             { return (this->GetCombatIntensity() > 0.01f); }
	float GetIncursionDistance(int team) const          { return this->m_IncursionDistances[team]; }
	
	float GetCombatIntensity() const                                 { return ft_GetCombatIntensity(this); }
	void AddPotentiallyVisibleActor(CBaseCombatCharacter *actor)     {        ft_AddPotentiallyVisibleActor(this, actor); }
	void TFMark()                                                    {        ft_TFMark(this); }
	bool IsTFMarked()                                                { return ft_IsTFMarked(this); }
	bool IsValidForWanderingPopulation()                             { return ft_IsValidForWanderingPopulation(this); }
	

	static void MakeNewTFMarker() { ft_MakeNewTFMarker(); }

	static StaticFuncThunk<void>                              ft_MakeNewTFMarker;
	
	DECL_RELATIVE_RW(CUtlVector<CHandle<CBaseCombatCharacter>>[4], m_potentiallyVisibleActor);
private:
	DECL_EXTRACT(TFNavAttributeType, m_nAttributes);
	DECL_EXTRACT(float[4],           m_IncursionDistances);
	
	static MemberFuncThunk<const CTFNavArea *, float>           ft_GetCombatIntensity;
	static MemberFuncThunk<      CTFNavArea *, void, CBaseCombatCharacter *> ft_AddPotentiallyVisibleActor;
	static MemberFuncThunk<      CTFNavArea *, void> ft_TFMark;
	static MemberFuncThunk<      CTFNavArea *, bool> ft_IsTFMarked;
	static MemberFuncThunk<      CTFNavArea *, bool> ft_IsValidForWanderingPopulation;
	
};


class CNavMesh
{
public:
	CNavArea *GetNavArea(const Vector& pos, float beneathLimit = 120.0f) const                                                                                             { return ft_GetNavArea_vec                          (this, pos, beneathLimit); }
	CNavArea *GetNavArea(CBaseEntity *pEntity, int nGetNavAreaFlags, float flBeneathLimit = 120.0f) const                                                                  { return ft_GetNavArea_ent                          (this, pEntity, nGetNavAreaFlags, flBeneathLimit); }
	CNavArea *GetNearestNavArea(const Vector& pos, bool anyZ = false, float maxDist = 10000.0f, bool checkLOS = false, bool checkGround = true, int team = TEAM_ANY) const { return ft_GetNearestNavArea_vec                   (this, pos, anyZ, maxDist, checkLOS, checkGround, team); }
	CNavArea *GetNearestNavArea(CBaseEntity *pEntity, int nFlags = GETNAVAREA_CHECK_GROUND, float maxDist = 10000.0f) const                                                { return ft_GetNearestNavArea_ent                   (this, pEntity, nFlags, maxDist); }
	bool GetGroundHeight(const Vector& pos, float *height, Vector *normal = nullptr) const                                                                                 { return ft_GetGroundHeight                         (this, pos, height, normal); }
	int Load()                                                                                                                                                             { return ft_Load                                    (this); }
#if TOOLCHAIN_FIXES
	void CollectAreasOverlappingExtent(const Extent& extent, CUtlVector<CTFNavArea *> *outVector)                                                                          {        ft_CollectAreasOverlappingExtent_CTFNavArea(this, extent, outVector); }
#endif
	
private:
	static MemberFuncThunk<const CNavMesh *, CNavArea *, const Vector&, float>                        ft_GetNavArea_vec;
	static MemberFuncThunk<const CNavMesh *, CNavArea *, CBaseEntity *, int, float>                   ft_GetNavArea_ent;
	static MemberFuncThunk<const CNavMesh *, CNavArea *, const Vector&, bool, float, bool, bool, int> ft_GetNearestNavArea_vec;
	static MemberFuncThunk<const CNavMesh *, CNavArea *, CBaseEntity *, int, float>                   ft_GetNearestNavArea_ent;
	static MemberFuncThunk<const CNavMesh *, bool, const Vector&, float *, Vector *>                  ft_GetGroundHeight;
	// This is inlined in the code
	// static MemberFuncThunk<      CNavMesh *, void, const Extent&, CUtlVector<CTFNavArea *> *>         ft_CollectAreasOverlappingExtent_CTFNavArea;
	static MemberFuncThunk<      CNavMesh *, int>                                                     ft_Load;
};

class CTFNavMesh : public CNavMesh
{
public:
	void CollectBuiltObjects(CUtlVector<CBaseObject *> *objects, int i1) { ft_CollectBuiltObjects(this, objects, i1); }
	void RecomputeInternalData()                                         { ft_RecomputeInternalData(this); }
	
	
private:
	static MemberFuncThunk<CTFNavMesh *, void, CUtlVector<CBaseObject *> *, int> ft_CollectBuiltObjects;
	static MemberFuncThunk<CTFNavMesh *, void>                                   ft_RecomputeInternalData;
};


extern GlobalThunk<CTFNavMesh *>             TheNavMesh;
extern GlobalThunk<CUtlVector<CTFNavArea *>> TheNavAreas;


extern StaticFuncThunk<float, CNavArea *, CNavArea *, CTFBotPathCost&, float> ft_NavAreaTravelDistance_CTFBotPathCost;
template<typename CostFunctor>
inline float NavAreaTravelDistance(CNavArea *startArea, CNavArea *endArea, CostFunctor& costFunc, float maxPathLength = 0.0f)
{
	/* we call NavAreaTravelDistance<CTFBotPathCost> for all functor types; should be okay */
	return ft_NavAreaTravelDistance_CTFBotPathCost(startArea, endArea, *reinterpret_cast<CTFBotPathCost *>(&costFunc), maxPathLength);
}

extern StaticFuncThunk<bool, CNavArea *, CNavArea *, const Vector *, CTFBotPathCost&, CNavArea **, float, int, bool> ft_NavAreaBuildPath_CTFBotPathCost;
template<typename CostFunctor>
inline bool NavAreaBuildPath(CNavArea *startArea, CNavArea *goalArea, const Vector *goalPos, CostFunctor& costFunc, CNavArea **closestArea = nullptr, float maxPathLength = 0.0f, int teamID = TEAM_ANY, bool ignoreNavBlockers = false)
{
	/* we call NavAreaBuildPath<CTFBotPathCost> for all functor types; should be okay */
	return ft_NavAreaBuildPath_CTFBotPathCost(startArea, goalArea, goalPos, *reinterpret_cast<CTFBotPathCost *>(&costFunc), closestArea, maxPathLength, teamID, ignoreNavBlockers);
}

void CollectSurroundingAreas(CUtlVector< CNavArea * > *nearbyAreaVector, CNavArea *startArea, float travelDistanceLimit = 1500.0f, float maxStepUpLimit = StepHeight, float maxDropDownLimit = 100.0f);
#endif
