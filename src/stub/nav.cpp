#include "stub/nav.h"
#include "mem/extract.h"

IMPL_DATAMAP(int,                   CFuncNavCost, m_team);
IMPL_DATAMAP(bool,                  CFuncNavCost, m_isDisabled);
IMPL_DATAMAP(string_t,              CFuncNavCost, m_iszTags);
IMPL_REL_AFTER(CUtlVector<CFmtStr>, CFuncNavCost, m_tags, m_iszTags);

MemberFuncThunk<const CFuncNavCost *, bool, const char *> CFuncNavCost::ft_HasTag("CFuncNavCost::HasTag");


#if defined _LINUX

static constexpr uint8_t s_Buf_CNavArea_m_funcNavCostVector[] = {
#ifdef PLATFORM_64BITS
	0x81, 0x67, 0x64, 0xff, 0xff, 0xff, 0xdf,                    // +0x0000 and     dword ptr [rdi+64h], 0DFFFFFFFh
	0xc7, 0x87, 0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // +0x0007 mov     dword ptr [rdi+1F8h], 0
#else
	0x55,                                                       // +0000  push ebp
	0x89, 0xe5,                                                 // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                                           // +0003  mov eax,[ebp+this]
	0x81, 0x60, 0x54, 0xff, 0xff, 0xff, 0xdf,                   // +0006  and dword ptr [eax+0x????????],0xdfffffff
	0xc7, 0x80, 0x58, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // +000D  mov dword ptr [eax+0xVVVVVVVV],0x00000000
	0x5d,                                                       // +0017  pop ebp
	0xc3,                                                       // +0018  ret
#endif
};

struct CExtract_CNavArea_m_funcNavCostVector : public IExtract<int32_t>
{
	using T = int32_t;
	
	CExtract_CNavArea_m_funcNavCostVector() : IExtract<T>(sizeof(s_Buf_CNavArea_m_funcNavCostVector)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CNavArea_m_funcNavCostVector);
		
#ifdef PLATFORM_64BITS
		mask.SetRange(0x00 + 2, 1, 0x00);
		mask.SetRange(0x07 + 2, 4, 0x00);
#else
		mask.SetRange(0x06 + 2, 1, 0x00);
		mask.SetRange(0x0d + 2, 4, 0x00);
#endif
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CNavArea::ClearAllNavCostEntities"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x000d + 2; }
#endif
	virtual T AdjustValue(T val) const override        { return reinterpret_cast<T>((int32_t)val - (int32_t)sizeof(CUtlMemory<int>)); }
};

#elif defined _WINDOWS

using CExtract_CNavArea_m_funcNavCostVector = IExtractStub;

#endif


#if defined _LINUX

// CNavArea::Shift
// CNavArea::CheckFloor
// CNavArea::CheckWaterLevel
// CNavArea::ComputeVisibility

static constexpr uint8_t s_Buf_CNavArea_m_center[] = {
#ifdef PLATFORM_64BITS
	0xf3, 0x0f, 0x10, 0x47, 0x30,  // +0x0000 movss   xmm0, dword ptr [rdi+30h]
	0xf3, 0x0f, 0x58, 0x06,        // +0x0005 addss   xmm0, dword ptr [rsi]
	0xf3, 0x0f, 0x11, 0x47, 0x30,  // +0x0009 movss   dword ptr [rdi+30h], xmm0
	0xf3, 0x0f, 0x10, 0x47, 0x34,  // +0x000e movss   xmm0, dword ptr [rdi+34h]
	0xf3, 0x0f, 0x58, 0x46, 0x04,  // +0x0013 addss   xmm0, dword ptr [rsi+4]
	0xf3, 0x0f, 0x11, 0x47, 0x34,  // +0x0018 movss   dword ptr [rdi+34h], xmm0
	0xf3, 0x0f, 0x10, 0x47, 0x38,  // +0x001d movss   xmm0, dword ptr [rdi+38h]
	0xf3, 0x0f, 0x58, 0x46, 0x08,  // +0x0022 addss   xmm0, dword ptr [rsi+8]
	0xf3, 0x0f, 0x11, 0x47, 0x38,  // +0x0027 movss   dword ptr [rdi+38h], xmm0
	0xc3,                          // +0x002c retn
#else
	0xf3, 0x0f, 0x10, 0x40, 0x2c, // +0000  movss xmm0,dword ptr [eax+m_center.x]
	0xf3, 0x0f, 0x58, 0x02,       // +0005  addss xmm0,dword ptr [edx.x]
	0xf3, 0x0f, 0x11, 0x40, 0x2c, // +0009  movss dword ptr [eax+m_center.x],xmm0
	0xf3, 0x0f, 0x10, 0x40, 0x30, // +000E  movss xmm0,dword ptr [eax+m_center.y]
	0xf3, 0x0f, 0x58, 0x42, 0x04, // +0013  addss xmm0,dword ptr [edx.y]
	0xf3, 0x0f, 0x11, 0x40, 0x30, // +0018  movss dword ptr [eax+m_center.y],xmm0
	0xf3, 0x0f, 0x10, 0x40, 0x34, // +001D  movss xmm0,dword ptr [eax+m_center.z]
	0xf3, 0x0f, 0x58, 0x42, 0x08, // +0022  addss xmm0,dword ptr [edx.z]
	0xf3, 0x0f, 0x11, 0x40, 0x34, // +0027  movss dword ptr [eax+m_center.z],xmm0
	0x5d,                         // +002C  pop ebp
	0xc3,                         // +002D  ret
#endif
};

struct CExtract_CNavArea_m_center : public IExtract<int8_t>
{
	using T = int8_t;
	
	CExtract_CNavArea_m_center() : IExtract<T>(sizeof(s_Buf_CNavArea_m_center)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CNavArea_m_center);
		
		mask.SetRange(0x00 + 4, 1, 0x00);
		mask.SetRange(0x09 + 4, 1, 0x00);
		mask.SetRange(0x0e + 4, 1, 0x00);
		mask.SetRange(0x18 + 4, 1, 0x00);
		mask.SetRange(0x1d + 4, 1, 0x00);
		mask.SetRange(0x27 + 4, 1, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CNavArea::Shift"; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetFuncOffMin() const override    { return 0x0058; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0058; }
#else
	virtual uint32_t GetFuncOffMin() const override    { return 0x0061; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0061; }
#endif
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 4; }
};

#elif defined _WINDOWS

using CExtract_CNavArea_m_center = IExtractStub;

#endif


#if defined _LINUX

static constexpr uint8_t s_Buf_CNavArea_m_attributeFlags[] = {
#ifdef PLATFORM_64BITS
	0x81, 0x67, 0x64, 0xff, 0xff, 0xff, 0xdf,                    // +0x0000 and     dword ptr [rdi+64h], 0DFFFFFFFh
	0xc7, 0x87, 0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // +0x0007 mov     dword ptr [rdi+1F8h], 0
#else
	0x55,                                                       // +0000  push ebp
	0x89, 0xe5,                                                 // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                                           // +0003  mov eax,[ebp+this]
	0x81, 0x60, 0x54, 0xff, 0xff, 0xff, 0xdf,                   // +0006  and dword ptr [eax+0x????????],0xdfffffff
	0xc7, 0x80, 0x58, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // +000D  mov dword ptr [eax+0xVVVVVVVV],0x00000000
	0x5d,                                                       // +0017  pop ebp
	0xc3,                                                       // +0018  ret
#endif
};

struct CExtract_CNavArea_m_attributeFlags : public IExtract<uint8_t>
{
	using T = uint8_t;
	
	CExtract_CNavArea_m_attributeFlags() : IExtract<T>(sizeof(s_Buf_CNavArea_m_attributeFlags)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CNavArea_m_attributeFlags);
		
#ifdef PLATFORM_64BITS
		mask.SetRange(0x00 + 2, 1, 0x00);
		mask.SetRange(0x07 + 2, 4, 0x00);
#else
		mask.SetRange(0x06 + 2, 1, 0x00);
		mask.SetRange(0x0d + 2, 4, 0x00);
#endif
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CNavArea::ClearAllNavCostEntities"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 2; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x0006 + 2; }
#endif
};

#elif defined _WINDOWS

using CExtract_CNavArea_m_attributeFlags = IExtractStub;

#endif


#if defined _LINUX

static constexpr uint8_t s_Buf_CNavArea_m_costSoFar[] = {
#ifdef PLATFORM_64BITS
	0xf3, 0x0f, 0x59, 0xd2,              // +0x0000 mulss   xmm2, xmm2
	0xf3, 0x0f, 0x59, 0xc0,              // +0x0004 mulss   xmm0, xmm0
	0xf3, 0x0f, 0x58, 0xca,              // +0x0008 addss   xmm1, xmm2
	0xf3, 0x0f, 0x58, 0xc1,              // +0x000c addss   xmm0, xmm1
	0xf3, 0x0f, 0x51, 0xc0,              // +0x0010 sqrtss  xmm0, xmm0
	0xf3, 0x0f, 0x58, 0x43, 0x48,        // +0x0014 addss   xmm0, dword ptr [rbx+48h]
	0xf3, 0x41, 0x0f, 0x11, 0x45, 0x48,  // +0x0019 movss   dword ptr [r13+48h], xmm0
#else
	0xf3, 0x0f, 0x59, 0xd2,       // +0000  mulss xmm2,xmm2
	0xf3, 0x0f, 0x59, 0xc9,       // +0004  mulss xmm1,xmm1
	0xf3, 0x0f, 0x58, 0xc2,       // +0008  addss xmm0,xmm2
	0xf3, 0x0f, 0x58, 0xc1,       // +000C  addss xmm0,xmm1
	0xf3, 0x0f, 0x51, 0xc0,       // +0010  sqrtss xmm0,xmm0
	0xf3, 0x0f, 0x58, 0x42, 0x44, // +0014  addss xmm0,dword ptr [edx+0xVVVVVVVV]
	0xf3, 0x0f, 0x11, 0x46, 0x44, // +0019  movss dword ptr [ebx+0xVVVVVVVV],xmm0
#endif
};

struct CExtract_CNavArea_m_costSoFar : public IExtract<int8_t>
{
	using T = int8_t;
	
	CExtract_CNavArea_m_costSoFar() : IExtract<T>(sizeof(s_Buf_CNavArea_m_costSoFar)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CNavArea_m_costSoFar);
		
		mask.SetRange(0x14 + 4, 1, 0x00);
		mask.SetRange(0x19 + 4, 1, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "ISearchSurroundingAreasFunctor::IterateAdjacentAreas"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x00a0; } // @ 0x00ea
	virtual uint32_t GetFuncOffMax() const override    { return 0x0100; }
	virtual uint32_t GetExtractOffset() const override { return 0x0014 + 4; }
};

#elif defined _WINDOWS

using CExtract_CNavArea_m_costSoFar = IExtractStub;

#endif


#if defined _LINUX

// CTFNavArea::m_nAttributes
// * CTFNavArea::IsValidForWanderingPopulation
// * CTFNavArea::IsBlocked
// * CTFNavMesh::IsSentryGunHere
//   CTFNavMesh::ResetMeshAttributes
//   CTFNavMesh::RemoveAllMeshDecoration
//   CTFNavMesh::ComputeLegalBombDropAreas
//   CTFNavMesh::CollectAndMaskSpawnRoomExits
//   TF_EditClearAllAttributes
//   GetBombInfo

static constexpr uint8_t s_Buf_CTFNavArea_m_nAttributes[] = {
#ifdef PLATFORM_64BITS
	0xf7, 0x87, 0x9c, 0x02, 0x00, 0x00, 0x07, 0x00, 0x00, 0x06,  // +0x0000 test    dword ptr [rdi+29Ch], 6000007h
	0x0f, 0x94, 0xc0,                                            // +0x000a setz    al
#else
	0x55,                                                       // +0000  push ebp
	0x89, 0xe5,                                                 // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                                           // +0003  mov eax,[ebp+this]
	0x5d,                                                       // +0006  pop ebp
	0xf7, 0x80, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x06, // +0007  test DWORD PTR [eax+0xVVVVVVVV],0x06000007
	0x0f, 0x94, 0xc0,                                           // +0011  setz al
	0xc3,                                                       // +0014  ret
#endif
};

struct CExtract_CTFNavArea_m_nAttributes : public IExtract<uint32_t>
{
	CExtract_CTFNavArea_m_nAttributes() : IExtract<uint32_t>(sizeof(s_Buf_CTFNavArea_m_nAttributes)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFNavArea_m_nAttributes);
		
#ifdef PLATFORM_64BITS
		mask.SetRange(0x00 + 2, 4, 0x00);
#else
		mask.SetRange(0x07 + 2, 4, 0x00);
#endif
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFNavArea::IsValidForWanderingPopulation"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0030; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 2; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
#endif
};

#elif defined _WINDOWS

static constexpr uint8_t s_Buf_CTFNavArea_m_nAttributes[] = {
	0x55,                               // +0000  push ebp
	0x8b, 0xec,                         // +0001  mov ebp,esp
	0x8b, 0x91, 0x00, 0x00, 0x00, 0x00, // +0003  mov edx,[ecx+0xVVVVVVVV]
};

struct CExtract_CTFNavArea_m_nAttributes : public IExtract<TFNavAttributeType *>
{
	CExtract_CTFNavArea_m_nAttributes() : IExtract<TFNavAttributeType *>(sizeof(s_Buf_CTFNavArea_m_nAttributes)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFNavArea_m_nAttributes);
		
		mask.SetRange(0x03 + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFNavArea::IsBlocked"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0030; }
	virtual uint32_t GetExtractOffset() const override { return 0x0003 + 2; }
};

#endif


#if defined _LINUX

static constexpr uint8_t s_Buf_CTFNavArea_m_IncursionDistances[] = {
#ifdef PLATFORM_64BITS
	0x48, 0x63, 0xc6,                                      // +0x0000 movsxd  rax, esi
	0xf3, 0x0f, 0x10, 0x84, 0x87, 0x08, 0x02, 0x00, 0x00,  // +0x0003 movss   xmm0, dword ptr [rdi+rax*4+208h]
#else
	0x8b, 0x45, 0x08,                                     // +0000  mov eax,[ebp+this]
	0x8b, 0x7d, 0x0c,                                     // +0003  mov edi,[ebp+arg_4]
	0xf3, 0x0f, 0x10, 0x84, 0xb8, 0x60, 0x01, 0x00, 0x00, // +0006  movss xmm0,dword ptr [eax+edi*4+0xVVVVVVVV]
#endif
};

struct CExtract_CTFNavArea_m_IncursionDistances : public IExtract<uint32_t>
{
	CExtract_CTFNavArea_m_IncursionDistances() : IExtract<uint32_t>(sizeof(s_Buf_CTFNavArea_m_IncursionDistances)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFNavArea_m_IncursionDistances);
		
#ifdef PLATFORM_64BITS
		mask.SetRange(0x03 + 5, 4, 0x00);
#else
		mask.SetRange(0x06 + 5, 4, 0x00);
#endif
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFNavArea::GetNextIncursionArea"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0030; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0003 + 5; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x0006 + 5; }
#endif
};

#elif defined _WINDOWS

using CExtract_CTFNavArea_m_IncursionDistances = IExtractStub;

#endif


#if defined _LINUX

static constexpr uint8_t s_Buf_CNavArea_m_potentiallyVisibleAreas[] = {
#ifdef PLATFORM_64BITS
	0xc7, 0x87, 0xd0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // +0x0000 mov     dword ptr [rdi+1D0h], 0
#else
	0x8B, 0x45, 0x08,
	0xC7, 0x80, 0x3C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#endif
};

struct CExtract_CNavArea_m_potentiallyVisibleAreas : public IExtract<uint32_t>
{
	using T = uint32_t;

	CExtract_CNavArea_m_potentiallyVisibleAreas() : IExtract<uint32_t>(sizeof(s_Buf_CNavArea_m_potentiallyVisibleAreas)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CNavArea_m_potentiallyVisibleAreas);
		
#ifdef PLATFORM_64BITS
		mask.SetRange(0x00 + 2, 4, 0x00);
#else
		mask.SetRange(0x03 + 2, 4, 0x00);
#endif
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CNavArea::ResetPotentiallyVisibleAreas"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0030; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 2; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x0003 + 2; }
#endif
	virtual T AdjustValue(T val) const override        { return reinterpret_cast<T>((uint32_t)val - (int32_t)sizeof(CUtlMemoryConservative<int>)); }
};

#elif defined _WINDOWS

using CExtract_CNavArea_m_potentiallyVisibleAreas = IExtractStub;

#endif


IMPL_EXTRACT  (CUtlVector<CHandle<CFuncNavCost>>, CNavArea, m_funcNavCostVector, new CExtract_CNavArea_m_funcNavCostVector());
IMPL_EXTRACT  (Vector,                            CNavArea, m_center,            new CExtract_CNavArea_m_center());
IMPL_EXTRACT  (int,                               CNavArea, m_attributeFlags_a,  new CExtract_CNavArea_m_attributeFlags());
IMPL_REL_BEFORE(uint32,                           CNavArea, m_nVisTestCounter,   m_funcNavCostVector, 0);
IMPL_REL_AFTER(CNavArea *,                        CNavArea, m_parent,            m_attributeFlags_a, CUtlVectorUltraConservative<int>[4], CUtlVectorUltraConservative<int>[2], CUtlVectorUltraConservative<int>, unsigned int);
IMPL_REL_AFTER(int,                               CNavArea, m_parentHow,         m_parent);
//IMPL_EXTRACT  (float,                             CNavArea, m_costSoFar,         new CExtract_CNavArea_m_costSoFar());
IMPL_EXTRACT  (CUtlVectorConservative<AreaBindInfo>, CNavArea, m_potentiallyVisibleAreas, new CExtract_CNavArea_m_potentiallyVisibleAreas());
IMPL_REL_BEFORE(AreaBindInfo,                     CNavArea, m_inheritVisibilityFrom, m_potentiallyVisibleAreas, 0);

MemberFuncThunk <const CNavArea *, void, Extent *>                               CNavArea::ft_GetExtent                            ("CNavArea::GetExtent");
MemberFuncThunk <const CNavArea *, void, const Vector *, Vector *>               CNavArea::ft_GetClosestPointOnArea                ("CNavArea::GetClosestPointOnArea");
MemberFuncThunk <const CNavArea *, float, const CNavArea *>                      CNavArea::ft_ComputeAdjacentConnectionHeightChange("CNavArea::ComputeAdjacentConnectionHeightChange");
MemberFuncThunk <const CNavArea *, float, float, float>                          CNavArea::ft_GetZ                                 ("CNavArea::GetZ [float float]");
MemberVFuncThunk<const CNavArea *, void, int, int, int, int, float, bool, float> CNavArea::vt_DrawFilled                           (TypeName<CNavArea>(), "CNavArea::DrawFilled");
MemberFuncThunk <const CNavArea *, bool, const Vector &>                         CNavArea::ft_Contains                             ("CNavArea::Contains");
MemberFuncThunk <const CNavArea *, bool, const Vector &, float>                  CNavArea::ft_IsOverlapping                        ("CNavArea::IsOverlapping");
MemberVFuncThunk<const CNavArea *, bool, int, bool>                              CNavArea::vt_IsBlocked                            (TypeName<CNavArea>(), "CNavArea::IsBlocked");
MemberFuncThunk <      CNavArea *, void>                                         CNavArea::ft_AddToOpenList                        ("CNavArea::AddToOpenList");
MemberFuncThunk <      CNavArea *, void>                                         CNavArea::ft_RemoveFromOpenList                   ("CNavArea::RemoveFromOpenList");
MemberVFuncThunk<const CNavArea *, bool, CNavArea *>                             CNavArea::vt_IsPotentiallyVisible                 (TypeName<CNavArea>(), "CNavArea::IsPotentiallyVisible");
MemberVFuncThunk<const CNavArea *, bool, int>                                    CNavArea::vt_IsPotentiallyVisibleToTeam           (TypeName<CNavArea>(), "CNavArea::IsPotentiallyVisibleToTeam");
MemberVFuncThunk<const CNavArea *, bool, CNavArea *>                             CNavArea::vt_IsCompletelyVisible                  (TypeName<CNavArea>(), "CNavArea::IsCompletelyVisible");
MemberFuncThunk <      CNavArea *, void, CUtlVector<CNavArea *> *>               CNavArea::ft_CollectAdjacentAreas                 ("CNavArea::CollectAdjacentAreas");

StaticFuncThunk<void>                              CNavArea::ft_ClearSearchLists("CNavArea::ClearSearchLists");

GlobalThunk<uint>       CNavArea::m_masterMarker("CNavArea::m_masterMarker");
GlobalThunk<CNavArea *> CNavArea::m_openList("CNavArea::m_openList");
GlobalThunk<uint32>     CNavArea::s_nCurrVisTestCounter("CNavArea::s_nCurrVisTestCounter");

#ifdef SE_IS_TF2
IMPL_EXTRACT (TFNavAttributeType, CTFNavArea, m_nAttributes,        new CExtract_CTFNavArea_m_nAttributes());
IMPL_EXTRACT (float[4],           CTFNavArea, m_IncursionDistances, new CExtract_CTFNavArea_m_IncursionDistances());
IMPL_REL_AFTER(CUtlVector<CHandle<CBaseCombatCharacter>>[4], CTFNavArea, m_potentiallyVisibleActor, m_nAttributes);

MemberFuncThunk<const CTFNavArea *, float>           CTFNavArea::ft_GetCombatIntensity("CTFNavArea::GetCombatIntensity");
MemberFuncThunk<      CTFNavArea *, void, CBaseCombatCharacter *> CTFNavArea::ft_AddPotentiallyVisibleActor("CTFNavArea::AddPotentiallyVisibleActor");
MemberFuncThunk<      CTFNavArea *, void> CTFNavArea::ft_TFMark("CTFNavArea::TFMark");
MemberFuncThunk<      CTFNavArea *, bool> CTFNavArea::ft_IsTFMarked("CTFNavArea::IsTFMarked");
MemberFuncThunk<      CTFNavArea *, bool> CTFNavArea::ft_IsValidForWanderingPopulation("CTFNavArea::IsValidForWanderingPopulation");

StaticFuncThunk<void>                     CTFNavArea::ft_MakeNewTFMarker("CTFNavArea::MakeNewTFMarker");

#endif


MemberFuncThunk<const CNavMesh *, CNavArea *, const Vector&, float>                        CNavMesh::ft_GetNavArea_vec                          ("CNavMesh::GetNavArea [vec]");
MemberFuncThunk<const CNavMesh *, CNavArea *, CBaseEntity *, int, float>                   CNavMesh::ft_GetNavArea_ent                          ("CNavMesh::GetNavArea [ent]");
MemberFuncThunk<const CNavMesh *, CNavArea *, const Vector&, bool, float, bool, bool, int> CNavMesh::ft_GetNearestNavArea_vec                   ("CNavMesh::GetNearestNavArea [vec]");
MemberFuncThunk<const CNavMesh *, CNavArea *, CBaseEntity *, int, float>                   CNavMesh::ft_GetNearestNavArea_ent                   ("CNavMesh::GetNearestNavArea [ent]");
MemberFuncThunk<const CNavMesh *, bool, const Vector&, float *, Vector *>                  CNavMesh::ft_GetGroundHeight                         ("CNavMesh::GetGroundHeight");
// This is inlined in the code
// MemberFuncThunk<CNavMesh *, void, const Extent&, CUtlVector<CTFNavArea *> *>               CNavMesh::ft_CollectAreasOverlappingExtent_CTFNavArea("CNavMesh::CollectAreasOverlappingExtent<CTFNavArea>");
MemberFuncThunk<CNavMesh *, int>                                                           CNavMesh::ft_Load                                    ("CNavMesh::Load");
MemberFuncThunk<      CNavMesh *, void, bool>                                              CNavMesh::ft_BeginGeneration                         ("CNavMesh::BeginGeneration");


#ifdef SE_IS_TF2
MemberFuncThunk<CTFNavMesh *, void, CUtlVector<CBaseObject *> *, int> CTFNavMesh::ft_CollectBuiltObjects("CTFNavMesh::CollectBuiltObjects");
MemberFuncThunk<CTFNavMesh *, void>                                   CTFNavMesh::ft_RecomputeInternalData("CTFNavMesh::RecomputeInternalData");


GlobalThunk<CTFNavMesh *>             TheNavMesh ("TheNavMesh");
GlobalThunk<CUtlVector<CTFNavArea *>> TheNavAreas("TheNavAreas");


StaticFuncThunk<float, CNavArea *, CNavArea *, CTFBotPathCost&, float>                                        ft_NavAreaTravelDistance_CTFBotPathCost("NavAreaTravelDistance<CTFBotPathCost>");
StaticFuncThunk<bool, CNavArea *, CNavArea *, const Vector *, CTFBotPathCost&, CNavArea **, float, int, bool> ft_NavAreaBuildPath_CTFBotPathCost     ("NavAreaBuildPath<CTFBotPathCost>");

#endif

void CollectSurroundingAreas( CUtlVector< CNavArea * > *nearbyAreaVector, CNavArea *startArea, float travelDistanceLimit, float maxStepUpLimit, float maxDropDownLimit)
{
	nearbyAreaVector->RemoveAll();

	if ( startArea )
	{
		CNavArea::MakeNewMarker();
		CNavArea::ClearSearchLists();

		startArea->AddToOpenList();
		startArea->SetTotalCost( 0.0f );
		startArea->SetCostSoFar( 0.0f );
		startArea->SetParent( NULL );
		startArea->Mark();

		CUtlVector<CNavArea *> areas;
		while( !CNavArea::IsOpenListEmpty() )
		{
			// get next area to check
			CNavArea *area = CNavArea::PopOpenList();

			if ( travelDistanceLimit > 0.0f && area->GetCostSoFar() > travelDistanceLimit )
				continue;

			if ( area->GetParent() )
			{
				float deltaZ = area->GetParent()->ComputeAdjacentConnectionHeightChange( area );

				if ( deltaZ > maxStepUpLimit )
					continue;

				if ( deltaZ < -maxDropDownLimit )
					continue;
			}

			nearbyAreaVector->AddToTail( area );

			// mark here to ensure all marked areas are also valid areas that are in the collection
			area->Mark();

			// search adjacent outgoing connections
			areas.RemoveAll();
			area->CollectAdjacentAreas(&areas);
			for (auto adjArea : areas) {

				if ( adjArea->IsBlocked( TEAM_ANY ) )
				{
					continue;
				}

				if ( !adjArea->IsMarked() )
				{
					adjArea->SetTotalCost( 0.0f );
					adjArea->SetParent( area );

					// compute approximate travel distance from start area of search
					float distAlong = area->GetCostSoFar();
					distAlong += ( adjArea->GetCenter() - area->GetCenter() ).Length();
					adjArea->SetCostSoFar( distAlong );
					adjArea->AddToOpenList();
				}
			}
		}
	}
}