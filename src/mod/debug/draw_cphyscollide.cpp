#include "mod.h"
#include "link/link.h"


// get the client to draw the CPhysCollide for any IPhysObject:

// see: C_BaseAnimating::DoInternalDrawModel
// see: DebugDrawPhysCollide
// see: CEngineClient::DebugDrawPhysCollide


// maybe hook C_BaseAnimating::DoInternalDrawModel
//   get C_BaseEntity * ptr
//   call: C_BaseAnimating::VPhysicsGetObjectList
//   call: const CPhysCollide *IPhysicsObject::GetCollide() const
//   call: void DebugDrawPhysCollide(const CPhysCollide *pCollide, IMaterial *pMaterial, matrix3x4_t& transform, const color32& color, bool drawAxes)


namespace Mod::Debug::Draw_CPhysCollide
{
	// C_BaseAnimating::VPhysicsGetObjectList: client, virtual
	// 
	
	
	
	// DebugDrawPhysCollide: engine cl, non-virtual
	// 
	
	
#if defined _MSC_VER
	constexpr int VIDX_GetAbsOrigin          = 0x00a;
	constexpr int VIDX_GetAbsAngles          = 0x00b;
	constexpr int VIDX_VPhysicsGetObjectList = 0x03a;
#else
	constexpr int VIDX_GetAbsOrigin          = 0x00b;
	constexpr int VIDX_GetAbsAngles          = 0x00c;
	constexpr int VIDX_VPhysicsGetObjectList = 0x05e;
#endif
	
	
	class C_BaseAnimating
	{
	public:
		const Vector& GetAbsOrigin()
		{
			auto pVT = *reinterpret_cast<void ** const*>(this);
			auto pFunc = MakePtrToMemberFunc<C_BaseAnimating, const Vector&>(pVT[VIDX_GetAbsOrigin]);
			return (this->*pFunc)();
		}
		
		const QAngle& GetAbsAngles()
		{
			auto pVT = *reinterpret_cast<void ** const*>(this);
			auto pFunc = MakePtrToMemberFunc<C_BaseAnimating, const QAngle&>(pVT[VIDX_GetAbsAngles]);
			return (this->*pFunc)();
		}
		
		int VPhysicsGetObjectList(IPhysicsObject **pList, int listMax)
		{
			auto pVT = *reinterpret_cast<void ** const*>(this);
			auto pFunc = MakePtrToMemberFunc<C_BaseAnimating, int, IPhysicsObject **, int>(pVT[VIDX_VPhysicsGetObjectList]);
			return (this->*pFunc)(pList, listMax);
		}
	};
	
	
	DETOUR_DECL_MEMBER(int, C_BaseAnimating_DrawModel, int flags)
	{
		auto anim = reinterpret_cast<C_BaseAnimating *>(this);
		
		IPhysicsObject *pPhysObject = nullptr;
		if (anim->VPhysicsGetObjectList(&pPhysObject, 1) != 0 && pPhysObject != nullptr) {
			DevMsg("C_BaseAnimating::DrawModel [this: %08x] [flags: %08x]\n",
				(uintptr_t)this, flags);
			DevMsg("  pPhysObject: %08x\n", (uintptr_t)pPhysObject);
			
			const CPhysCollide *pCollide = pPhysObject->GetCollide();
			if (pCollide != nullptr) {
				DevMsg("  pCollide: %08x\n", (uintptr_t)pCollide);
				
				matrix3x4_t transform;
				AngleMatrix(anim->GetAbsAngles(), anim->GetAbsOrigin(), transform);
				
				color32 color = { 0xff, 0xff, 0xff, 0xff };
				
				engineclient->DebugDrawPhysCollide(pCollide, nullptr, transform, color);
			}
		}
		
		return DETOUR_MEMBER_CALL(flags);
	}
	
	
	// VPhysicsInitNormal
	// 
	
	
#if 0
	DETOUR_DECL_MEMBER(void, C_BaseAnimating_DoInternalDrawModel, ClientModelRenderInfo_t *pInfo, DrawModelState_t *pState, matrix3x4_t *pBoneToWorldArray)
	{
		DETOUR_MEMBER_CALL(pInfo, pState, pBoneToWorldArray);
		
		
	}
#endif
	
	
#if 0
	DETOUR_DECL_MEMBER(void, C_BaseEntity_VPhysicsUpdate, IPhysicsObject *pPhysics)
	{
		DevMsg("C_BaseEntity::VPhysicsUpdate [this: %08x]\n",
			(uintptr_t)this);
		
		DETOUR_MEMBER_CALL(pPhysics);
		
		const CPhysCollide *pCollide = pPhysics->GetCollide();
		if (pCollide != nullptr) {
			auto ent = reinterpret_cast<C_BaseEntity *>(this);
			
			matrix3x4_t transform;
			AngleMatrix(ent->GetAbsAngles(), ent->GetAbsOrigin(), transform);
			
			color32 color = { 0xff, 0xff, 0xff, 0xff };
			
			engineclient->DebugDrawPhysCollide(pCollide, nullptr, transform, color);
		}
	}
#endif
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Draw_CPhysCollide")
		{
		//	MOD_ADD_DETOUR_MEMBER(C_BaseEntity_VPhysicsUpdate, "[client] C_BaseEntity::VPhysicsUpdate");
			
			MOD_ADD_DETOUR_MEMBER(C_BaseAnimating_DrawModel, "[client] C_BaseAnimating::DrawModel");
		}
		
		virtual bool OnLoad() override
		{
			return (engineclient != nullptr);
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_draw_cphyscollide", "0", FCVAR_NOTIFY,
		"Debug: draw CPhysCollide on all entities",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
