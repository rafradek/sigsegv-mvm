#include "stub/baseanimating.h"
#ifdef SE_TF2
#include "stub/econ.h"
#include "stub/tfweaponbase.h"
#endif


IMPL_SENDPROP(int,   CBaseAnimating, m_nSkin,        CBaseAnimating);
IMPL_SENDPROP(int,   CBaseAnimating, m_nBody,        CBaseAnimating);
IMPL_SENDPROP(int,   CBaseAnimating, m_nSequence,    CBaseAnimating);
IMPL_SENDPROP(float, CBaseAnimating, m_flModelScale, CBaseAnimating);
IMPL_SENDPROP(float, CBaseAnimating, m_flCycle,      CBaseAnimating);
IMPL_SENDPROP(int,   CBaseAnimating, m_nHitboxSet,   CBaseAnimating);
IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseAnimating, m_hLightingOrigin, CBaseAnimating);
IMPL_SENDPROP(float, CBaseAnimating, m_flPlaybackRate, CBaseAnimating);
IMPL_SENDPROP(float[24], CBaseAnimating, m_flPoseParameter, CBaseAnimating);
IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseAnimating, m_hLightingOriginRelative,  CBaseAnimating);

IMPL_DATAMAP(bool,   CBaseAnimating, m_bSequenceFinished);
IMPL_DATAMAP(bool,   CBaseAnimating, m_bSequenceLoops);
IMPL_DATAMAP(float,  CBaseAnimating, m_flGroundSpeed);

IMPL_RELATIVE   (DataCacheHandle_t, CBaseAnimating, m_boneCacheHandle, m_hLightingOriginRelative, sizeof(string_t) * 2);
IMPL_RELATIVE   (unsigned short, CBaseAnimating, m_fBoneCacheFlags, m_hLightingOriginRelative, sizeof(string_t) * 2 + sizeof(DataCacheHandle_t));

MemberFuncThunk<CBaseAnimating *, void, float, float>              CBaseAnimating::ft_SetModelScale       ("CBaseAnimating::SetModelScale");
MemberFuncThunk<CBaseAnimating *, void, float, bool>               CBaseAnimating::ft_DrawServerHitboxes  ("CBaseAnimating::DrawServerHitboxes");
MemberFuncThunk<CBaseAnimating *, void, int, int>                  CBaseAnimating::ft_SetBodygroup        ("CBaseAnimating::SetBodygroup");
MemberFuncThunk<CBaseAnimating *, int, int>                        CBaseAnimating::ft_GetBodygroup        ("CBaseAnimating::GetBodygroup");
MemberFuncThunk<CBaseAnimating *, const char *, int>               CBaseAnimating::ft_GetBodygroupName    ("CBaseAnimating::GetBodygroupName");
MemberFuncThunk<CBaseAnimating *, int, const char *>               CBaseAnimating::ft_FindBodygroupByName ("CBaseAnimating::FindBodygroupByName");
MemberFuncThunk<CBaseAnimating *, int, int>                        CBaseAnimating::ft_GetBodygroupCount   ("CBaseAnimating::GetBodygroupCount");
MemberFuncThunk<CBaseAnimating *, int>                             CBaseAnimating::ft_GetNumBodyGroups    ("CBaseAnimating::GetNumBodyGroups");
MemberFuncThunk<CBaseAnimating *, int>                             CBaseAnimating::ft_GetNumBones         ("CBaseAnimating::GetNumBones");
MemberFuncThunk<CBaseAnimating *, void>                            CBaseAnimating::ft_ResetSequenceInfo   ("CBaseAnimating::ResetSequenceInfo");
MemberFuncThunk<CBaseAnimating *, void, int>                       CBaseAnimating::ft_ResetSequence       ("CBaseAnimating::ResetSequence");
MemberFuncThunk<CBaseAnimating *, CStudioHdr *>                    CBaseAnimating::ft_GetModelPtr         ("CBaseAnimating::GetModelPtr");
MemberFuncThunk<CBaseAnimating *, int, CStudioHdr *, const char *> CBaseAnimating::ft_LookupPoseParameter ("CBaseAnimating::LookupPoseParameter");
MemberFuncThunk<CBaseAnimating *, float, int>                      CBaseAnimating::ft_GetPoseParameter_int("CBaseAnimating::GetPoseParameter [int]");
MemberFuncThunk<CBaseAnimating *, float, const char *>             CBaseAnimating::ft_GetPoseParameter_str("CBaseAnimating::GetPoseParameter [str]");
MemberFuncThunk<CBaseAnimating *, float, CStudioHdr *, int, float> CBaseAnimating::ft_SetPoseParameter    ("CBaseAnimating::SetPoseParameter");
MemberFuncThunk<CBaseAnimating *, void, int, matrix3x4_t&>         CBaseAnimating::ft_GetBoneTransform    ("CBaseAnimating::GetBoneTransform");
MemberFuncThunk<CBaseAnimating *, int, const char *>               CBaseAnimating::ft_LookupBone          ("CBaseAnimating::LookupBone");
MemberFuncThunk<CBaseAnimating *, int, const char *>               CBaseAnimating::ft_LookupAttachment    ("CBaseAnimating::LookupAttachment");
MemberFuncThunk<CBaseAnimating *, int, const char *>               CBaseAnimating::ft_LookupSequence      ("CBaseAnimating::LookupSequence");
MemberFuncThunk<CBaseAnimating *, void, int, Vector&, QAngle&>     CBaseAnimating::ft_GetBonePosition     ("CBaseAnimating::GetBonePosition");
MemberFuncThunk<CBaseAnimating *, bool, int, Vector&, QAngle&>     CBaseAnimating::ft_GetAttachment       ("CBaseAnimating::GetAttachment");
MemberFuncThunk<CBaseAnimating *, bool, int, matrix3x4_t&>         CBaseAnimating::ft_GetAttachment2      ("CBaseAnimating::GetAttachment [matrix]");
MemberFuncThunk<CBaseAnimating *, int, int>                        CBaseAnimating::ft_GetAttachmentBone   ("CBaseAnimating::GetAttachmentBone");
#ifdef SE_TF2
MemberFuncThunk<CBaseAnimating *, float, int>                      CBaseAnimating::ft_SequenceDuration    ("CBaseAnimating::SequenceDuration");
#endif
MemberFuncThunk<CBaseAnimating *, void>                            CBaseAnimating::ft_InvalidateBoneCache ("CBaseAnimating::InvalidateBoneCache");

MemberVFuncThunk<CBaseAnimating *, void>                   CBaseAnimating::vt_StudioFrameAdvance     (TypeName<CBaseAnimating>(), "CBaseAnimating::StudioFrameAdvance");
MemberVFuncThunk<CBaseAnimating *, void>                   CBaseAnimating::vt_RefreshCollisionBounds (TypeName<CBaseAnimating>(), "CBaseAnimating::RefreshCollisionBounds");
MemberVFuncThunk<CBaseAnimating *, void, CBaseAnimating *> CBaseAnimating::vt_DispatchAnimEvents     (TypeName<CBaseAnimating>(), "CBaseAnimating::DispatchAnimEvents");


IMPL_SENDPROP (CUtlVector<CAnimationLayer>, CBaseAnimatingOverlay, m_AnimOverlay, CBaseAnimatingOverlay);

MemberFuncThunk<CBaseAnimatingOverlay *, int, Activity, bool> CBaseAnimatingOverlay::ft_AddGesture("CBaseAnimatingOverlay::AddGesture");
MemberFuncThunk<CBaseAnimatingOverlay *, int, int, bool> CBaseAnimatingOverlay::ft_AddGestureSequence("CBaseAnimatingOverlay::AddGestureSequence");
MemberFuncThunk<CBaseAnimatingOverlay *, void, Activity> CBaseAnimatingOverlay::ft_RemoveGesture("CBaseAnimatingOverlay::RemoveGesture");
MemberFuncThunk<CBaseAnimatingOverlay *, void> CBaseAnimatingOverlay::ft_RemoveAllGestures("CBaseAnimatingOverlay::RemoveAllGestures");
MemberFuncThunk<CBaseAnimatingOverlay *, void, Activity, bool, bool> CBaseAnimatingOverlay::ft_RestartGesture("CBaseAnimatingOverlay::RestartGesture");
MemberFuncThunk<CBaseAnimatingOverlay *, float, int> CBaseAnimatingOverlay::ft_GetLayerDuration("CBaseAnimatingOverlay::GetLayerDuration");
MemberFuncThunk<CBaseAnimatingOverlay *, void, int, float> CBaseAnimatingOverlay::ft_SetLayerDuration("CBaseAnimatingOverlay::SetLayerDuration");


#ifdef SE_TF2
IMPL_SENDPROP(bool, CEconEntity, m_bValidatedAttachedEntity, CEconEntity);

MemberFuncThunk<CEconEntity *, void> CEconEntity::ft_DebugDescribe("CEconEntity::DebugDescribe");

MemberVFuncThunk<CEconEntity *, CAttributeContainer *> CEconEntity::vt_GetAttributeContainer(TypeName<CEconEntity>(), "CEconEntity::GetAttributeContainer");
MemberVFuncThunk<CEconEntity *, CAttributeManager *>   CEconEntity::vt_GetAttributeManager  (TypeName<CEconEntity>(), "CEconEntity::GetAttributeManager");
MemberVFuncThunk<CEconEntity *, void, CBaseEntity *>   CEconEntity::vt_GiveTo               (TypeName<CEconEntity>(), "CEconEntity::GiveTo");
MemberVFuncThunk<CEconEntity *, void>                  CEconEntity::vt_ReapplyProvision     (TypeName<CEconEntity>(), "CEconEntity::ReapplyProvision");
MemberVFuncThunk<CEconEntity *, bool, CBaseCombatCharacter *, bool> CEconEntity::vt_UpdateBodygroups(TypeName<CEconEntity>(), "CEconEntity::UpdateBodygroups");
MemberVFuncThunk<CEconEntity *, Activity, Activity>    CEconEntity::vt_TranslateViewmodelHandActivityInternal(TypeName<CEconEntity>(), "CEconEntity::TranslateViewmodelHandActivityInternal");

CEconItemView *CEconEntity::GetItem()
{
	CAttributeContainer *attr_container = this->GetAttributeContainer();
	assert(attr_container != nullptr);
	
	return attr_container->GetItem();
}

void CEconEntity::Validate()
{
	this->m_bValidatedAttachedEntity = true;
	// make any extra wearable models visible for other players
	auto weapon = rtti_cast<CTFWeaponBase *>(this);
	if (weapon != nullptr) {
		if (weapon->m_hExtraWearable != nullptr) {
			weapon->m_hExtraWearable->m_bValidatedAttachedEntity = true;
		}
		if (weapon->m_hExtraWearableViewModel != nullptr) {
			weapon->m_hExtraWearableViewModel->m_bValidatedAttachedEntity = true;
		}
	}
}
#endif

StaticFuncThunk<void, CBaseEntity *, const Vector *, const Vector *> ft_UTIL_SetSize("UTIL_SetSize");
StaticFuncThunk<float, CBaseFlex *, const char *, EHANDLE *, float, bool, void *, bool, IRecipientFilter *> ft_InstancedScriptedScene("InstancedScriptedScene");
StaticFuncThunk<void, CBaseFlex *, EHANDLE> ft_StopScriptedScene("StopScriptedScene");
StaticFuncThunk<int, CStudioHdr *, int, int> ft_SelectWeightedSequence("SelectWeightedSequence");