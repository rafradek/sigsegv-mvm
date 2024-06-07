#include "stub/baseentity_cl.h"
#include "mem/extract.h"

//#if defined _LINUX
//
//// TODO
//struct CExtract_C_BaseEntity_m_rgflCoordinateFrame : public IExtract<matrix3x4_t *>
//{
//	CExtract_C_BaseEntity_m_rgflCoordinateFrame() : IExtract<matrix3x4_t *>(0) {}
//	
//	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override { return false; }
//	
//	virtual const char *GetFuncName() const override   { return "[client] C_BaseEntity::GetVectors"; }
//	virtual uint32_t GetFuncOffMin() const override    { return 0xffffffff; }
//	virtual uint32_t GetFuncOffMax() const override    { return 0xffffffff; }
//	virtual uint32_t GetExtractOffset() const override { return 0xffffffff; }
//};
//
//#elif defined _WINDOWS
//
//// TODO
//struct CExtract_C_BaseEntity_m_rgflCoordinateFrame : public IExtract<matrix3x4_t *>
//{
//	CExtract_C_BaseEntity_m_rgflCoordinateFrame() : IExtract<matrix3x4_t *>(0) {}
//	
//	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override { return false; }
//	
//	virtual const char *GetFuncName() const override   { return "[client] C_BaseEntity::GetVectors"; }
//	virtual uint32_t GetFuncOffMin() const override    { return 0xffffffff; }
//	virtual uint32_t GetFuncOffMax() const override    { return 0xffffffff; }
//	virtual uint32_t GetExtractOffset() const override { return 0xffffffff; }
//};
//
//#endif


#if defined _LINUX

// TODO
struct CExtract_C_BaseEntity_m_pMoveParent : public IExtract<int32_t>
{
	CExtract_C_BaseEntity_m_pMoveParent() : IExtract<int32_t>(0) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override { return false; }
	
	virtual const char *GetFuncName() const override   { return "[client] C_BaseEntity::GetParentToWorldTransform"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0xffffffff; }
	virtual uint32_t GetFuncOffMax() const override    { return 0xffffffff; }
	virtual uint32_t GetExtractOffset() const override { return 0xffffffff; }
};

#elif defined _WINDOWS

static constexpr uint8_t s_Buf_C_BaseEntity_GetParentToWorldTransform[] = {
	0x55,                               // +0000  push ebp
	0x8b, 0xec,                         // +0001  mov ebp,esp
	0x83, 0xec, 0x18,                   // +0003  sub esp,0x18
	0x56,                               // +0006  push esi
	0x8b, 0xb1, 0x00, 0x00, 0x00, 0x00, // +0007  mov esi,[ecx+m_pMoveParent]
};

struct CExtract_C_BaseEntity_m_pMoveParent : public IExtract<ClientHandle<C_BaseEntity> *>
{
	CExtract_C_BaseEntity_m_pMoveParent() : IExtract<ClientHandle<C_BaseEntity> *>(sizeof(s_Buf_C_BaseEntity_GetParentToWorldTransform)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_C_BaseEntity_GetParentToWorldTransform);
		
		mask.SetDword(0x07 + 2, 0x00000000);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "[client] C_BaseEntity::GetParentToWorldTransform"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x0007 + 2; }
};

#endif


IMPL_EXTRACT(ClientHandle<C_BaseEntity>, C_BaseEntity, m_pMoveParent, new CExtract_C_BaseEntity_m_pMoveParent());


MemberFuncThunk<const C_BaseEntity *, const Vector&>                      C_BaseEntity::ft_GetAbsVelocity           ("[client] C_BaseEntity::GetAbsVelocity");
MemberFuncThunk<const C_BaseEntity *, void, Vector *, Vector *, Vector *> C_BaseEntity::ft_GetVectors               ("[client] C_BaseEntity::GetVectors");
MemberFuncThunk<      C_BaseEntity *, matrix3x4_t&, matrix3x4_t&>         C_BaseEntity::ft_GetParentToWorldTransform("[client] C_BaseEntity::GetParentToWorldTransform");
