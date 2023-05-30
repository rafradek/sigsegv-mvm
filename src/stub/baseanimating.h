#ifndef _INCLUDE_SIGSEGV_STUB_BASEANIMATING_H_
#define _INCLUDE_SIGSEGV_STUB_BASEANIMATING_H_


#include "stub/baseentity.h"
#include "stub/ihasattributes.h"


class CAttributeManager;
class CAttributeContainer;
class CEconItemView;


class CBaseAnimating : public CBaseEntity
{
public:
	float GetModelScale() const                 { return this->m_flModelScale; }
	float GetCycle() const                      { return this->m_flCycle; }
	void SetCycle(float cycle)                  { this->m_flCycle = cycle; }
	int GetHitboxSet() const                    { return this->m_nHitboxSet; }
	int LookupPoseParameter(const char *szName) { return this->LookupPoseParameter(this->GetModelPtr(), szName); }
	
	void SetModelScale(float scale, float change_duration = 0.0f)          {        ft_SetModelScale       (this, scale, change_duration); }
	void DrawServerHitboxes(float duration = 0.0f, bool monocolor = false) {        ft_DrawServerHitboxes  (this, duration, monocolor); }
	void SetBodygroup(int iGroup, int iValue)                              {        ft_SetBodygroup        (this, iGroup, iValue); }
	int GetBodygroup(int iGroup)                                           { return ft_GetBodygroup        (this, iGroup); }
	const char *GetBodygroupName(int iGroup)                               { return ft_GetBodygroupName    (this, iGroup); }
	int FindBodygroupByName(const char *name)                              { return ft_FindBodygroupByName (this, name); }
	int GetBodygroupCount(int iGroup)                                      { return ft_GetBodygroupCount   (this, iGroup); }
	int GetNumBodyGroups()                                                 { return ft_GetNumBodyGroups    (this); }
	int GetNumBones()                                                      { return ft_GetNumBones         (this); }
	void ResetSequenceInfo()                                               {        ft_ResetSequenceInfo   (this); }
	void ResetSequence(int nSequence)                                      {        ft_ResetSequence       (this, nSequence); }
	CStudioHdr *GetModelPtr()                                              { return ft_GetModelPtr         (this); }
	int LookupPoseParameter(CStudioHdr *pStudioHdr, const char *szName)    { return ft_LookupPoseParameter (this, pStudioHdr, szName); }
	float GetPoseParameter(int iParameter)                                 { return ft_GetPoseParameter_int(this, iParameter); }
	float GetPoseParameter(const char *szName)                             { return ft_GetPoseParameter_str(this, szName); }
	float SetPoseParameter(CStudioHdr *pStudioHdr, int iParameter, float value) { return ft_SetPoseParameter(this, pStudioHdr, iParameter, value); }
	void GetBoneTransform(int iBone, matrix3x4_t& pBoneToWorld)            { return ft_GetBoneTransform    (this, iBone, pBoneToWorld); }
	int LookupBone(const char *name)                                       { return ft_LookupBone          (this, name); }
	int LookupAttachment(const char *name)                                 { return ft_LookupAttachment    (this, name); }
	int LookupSequence(const char *name)                                   { return ft_LookupSequence      (this, name); }
	void GetBonePosition(int id, Vector& vec, QAngle& ang)                 {        ft_GetBonePosition     (this, id, vec, ang); }
	bool GetAttachment(int id, Vector& vec, QAngle& ang)                   { return ft_GetAttachment       (this, id, vec, ang); }
	bool GetAttachment(int id, matrix3x4_t &transform)                     { return ft_GetAttachment2       (this, id, transform); }
	int GetAttachmentBone(int id)                                          { return ft_GetAttachmentBone   (this, id); }
	float SequenceDuration(int sequence)                                   { return ft_SequenceDuration    (this, sequence); }
	

	void RefreshCollisionBounds()                                          {        vt_RefreshCollisionBounds(this); }
	void StudioFrameAdvance()                                              {        vt_StudioFrameAdvance    (this); }
	void DispatchAnimEvents(CBaseAnimating *anim)                          {        vt_DispatchAnimEvents    (this, anim); }
	
	DECL_SENDPROP   (int,   m_nSkin);
	DECL_SENDPROP   (int,   m_nBody);
	DECL_SENDPROP   (int,   m_nSequence);
	DECL_DATAMAP   (bool,   m_bSequenceFinished);
	DECL_DATAMAP   (bool,   m_bSequenceLoops);
	DECL_SENDPROP  (CHandle<CBaseEntity>, m_hLightingOrigin);
	DECL_SENDPROP   (float,   m_flPlaybackRate);
	DECL_DATAMAP   (float,  m_flGroundSpeed);
	DECL_SENDPROP  (float[24],   m_flPoseParameter);
	DECL_RELATIVE   (DataCacheHandle_t,  m_boneCacheHandle);
	DECL_RELATIVE   (unsigned short,  m_fBoneCacheFlags);
	DECL_SENDPROP  (CHandle<CBaseEntity>,  m_hLightingOriginRelative);
	
	
private:
	DECL_SENDPROP   (float, m_flModelScale);
	DECL_SENDPROP_RW(float, m_flCycle);
	DECL_SENDPROP   (int,   m_nHitboxSet);
	
	static MemberFuncThunk<CBaseAnimating *, void, float, float>              ft_SetModelScale;
	static MemberFuncThunk<CBaseAnimating *, void, float, bool>               ft_DrawServerHitboxes;
	static MemberFuncThunk<CBaseAnimating *, void, int, int>                  ft_SetBodygroup;
	static MemberFuncThunk<CBaseAnimating *, int, int>                        ft_GetBodygroup;
	static MemberFuncThunk<CBaseAnimating *, const char *, int>               ft_GetBodygroupName;
	static MemberFuncThunk<CBaseAnimating *, int, const char *>               ft_FindBodygroupByName;
	static MemberFuncThunk<CBaseAnimating *, int, int>                        ft_GetBodygroupCount;
	static MemberFuncThunk<CBaseAnimating *, int>                             ft_GetNumBodyGroups;
	static MemberFuncThunk<CBaseAnimating *, int>                             ft_GetNumBones;
	static MemberFuncThunk<CBaseAnimating *, void>                            ft_ResetSequenceInfo;
	static MemberFuncThunk<CBaseAnimating *, void, int>                       ft_ResetSequence;
	static MemberFuncThunk<CBaseAnimating *, CStudioHdr *>                    ft_GetModelPtr;
	static MemberFuncThunk<CBaseAnimating *, int, CStudioHdr *, const char *> ft_LookupPoseParameter;
	static MemberFuncThunk<CBaseAnimating *, float, int>                      ft_GetPoseParameter_int;
	static MemberFuncThunk<CBaseAnimating *, float, const char *>             ft_GetPoseParameter_str;
	static MemberFuncThunk<CBaseAnimating *, float, CStudioHdr *, int, float>  ft_SetPoseParameter;
	static MemberFuncThunk<CBaseAnimating *, void, int, matrix3x4_t&>         ft_GetBoneTransform;
	static MemberFuncThunk<CBaseAnimating *, int, const char *>               ft_LookupBone;
	static MemberFuncThunk<CBaseAnimating *, void, int, Vector&, QAngle&>     ft_GetBonePosition;
	static MemberFuncThunk<CBaseAnimating *, bool, int, Vector&, QAngle&>     ft_GetAttachment;
	static MemberFuncThunk<CBaseAnimating *, bool, int, matrix3x4_t&>              ft_GetAttachment2;
	static MemberFuncThunk<CBaseAnimating *, int, int>                        ft_GetAttachmentBone;
	static MemberFuncThunk<CBaseAnimating *, int, const char *>               ft_LookupAttachment;
	static MemberFuncThunk<CBaseAnimating *, int, const char *>               ft_LookupSequence;
	static MemberFuncThunk<CBaseAnimating *, float, int>                      ft_SequenceDuration;

	static MemberVFuncThunk<CBaseAnimating *, void>                   vt_RefreshCollisionBounds;
	static MemberVFuncThunk<CBaseAnimating *, void>                   vt_StudioFrameAdvance;
	static MemberVFuncThunk<CBaseAnimating *, void, CBaseAnimating *> vt_DispatchAnimEvents;

};

class CBaseAnimatingOverlay;

class CAnimationLayer {
public:
	int		m_fFlags;
	bool	m_bSequenceFinished;
	bool	m_bLooping;
	int		m_nSequence;
	float	m_flCycle;
	float	m_flPrevCycle;
	float	m_flWeight;
	float	m_flPlaybackRate;
	float	m_flBlendIn;
	float	m_flBlendOut; 
	float	m_flKillRate;
	float	m_flKillDelay;
	float	m_flLayerAnimtime;
	float	m_flLayerFadeOuttime;
	Activity	m_nActivity;
	int		m_nPriority;
	int m_nOrder;
	float	m_flLastEventCheck;
	float	m_flLastAccess;
	CBaseAnimatingOverlay *m_pOwnerEntity;
};

class CBaseAnimatingOverlay : public CBaseAnimating {
public:
	int AddGesture(Activity act, bool autoKill = true) {return ft_AddGesture(this, act, autoKill); }
	int AddGestureSequence(int seq, bool autoKill = true) {return ft_AddGestureSequence(this, seq, autoKill); }
	void RemoveGesture(Activity act) {return ft_RemoveGesture(this, act); }
	void RemoveAllGestures() {return ft_RemoveAllGestures(this); }
	void RestartGesture(Activity act, bool addIfMissing = true, bool autoKill = true) { ft_RestartGesture(this, act, addIfMissing, autoKill); }
	float GetLayerDuration(int layer) { return ft_GetLayerDuration(this, layer); }
	void SetLayerDuration(int layer, float duration) { ft_SetLayerDuration(this, layer, duration); }

	DECL_SENDPROP (CUtlVector<CAnimationLayer>, m_AnimOverlay);
private:
	static MemberFuncThunk<CBaseAnimatingOverlay *, int, Activity, bool> ft_AddGesture;
	static MemberFuncThunk<CBaseAnimatingOverlay *, int, int, bool> ft_AddGestureSequence;
	static MemberFuncThunk<CBaseAnimatingOverlay *, void, Activity> ft_RemoveGesture;
	static MemberFuncThunk<CBaseAnimatingOverlay *, void> ft_RemoveAllGestures;
	static MemberFuncThunk<CBaseAnimatingOverlay *, void, Activity, bool, bool> ft_RestartGesture;
	static MemberFuncThunk<CBaseAnimatingOverlay *, float, int> ft_GetLayerDuration;
	static MemberFuncThunk<CBaseAnimatingOverlay *, void, int, float> ft_SetLayerDuration;
};

class CBaseFlex : public CBaseAnimatingOverlay {};

#ifdef SE_TF2
class CEconEntity : public CBaseAnimating, public IHasAttributes
{
public:
	void DebugDescribe() { ft_DebugDescribe(this); }
	
	// GetAttributeContainer really ought to be in IHasAttributes...
	CAttributeManager *GetAttributeManager()	 { return vt_GetAttributeManager  (this); }
	CAttributeContainer *GetAttributeContainer() { return vt_GetAttributeContainer(this); }
	void GiveTo(CBaseEntity *ent)                {        vt_GiveTo(this, ent); }
	void ReapplyProvision()                      {        vt_ReapplyProvision(this); }
	bool UpdateBodygroups(CBaseCombatCharacter *owner, bool enable) { return vt_UpdateBodygroups(this, owner, enable); }
	
	CEconItemView *GetItem();
	
	DECL_SENDPROP_RW(bool, m_bValidatedAttachedEntity);
	
	// make the model visible for other players
	void Validate();
private:
	static MemberFuncThunk<CEconEntity *, void> ft_DebugDescribe;
	
	static MemberVFuncThunk<CEconEntity *, CAttributeContainer *> vt_GetAttributeContainer;
	static MemberVFuncThunk<CEconEntity *, CAttributeManager *>   vt_GetAttributeManager;
	static MemberVFuncThunk<CEconEntity *, void, CBaseEntity *>   vt_GiveTo;
	static MemberVFuncThunk<CEconEntity *, void>                  vt_ReapplyProvision;
	static MemberVFuncThunk<CEconEntity *, bool, CBaseCombatCharacter *, bool> vt_UpdateBodygroups;
};
#endif

extern StaticFuncThunk<void, CBaseEntity *, const Vector *, const Vector *> ft_UTIL_SetSize;
inline void UTIL_SetSize( CBaseEntity * entity, const Vector * min, const Vector * max )
{
	ft_UTIL_SetSize(entity, min, max);
}

extern StaticFuncThunk<float, CBaseFlex *, const char *, EHANDLE *, float, bool, void *, bool, IRecipientFilter *> ft_InstancedScriptedScene;
inline float InstancedScriptedScene(CBaseFlex *pActor, const char *pszScene,  EHANDLE *phSceneEnt = NULL, float flPostDelay = 0.0f, bool bIsBackground = false, void *response = NULL, bool bMultiplayer = false, IRecipientFilter *filter = NULL)
{
	return ft_InstancedScriptedScene(pActor, pszScene, phSceneEnt, flPostDelay, bIsBackground, response, bMultiplayer, filter);
}

extern StaticFuncThunk<void, CBaseFlex *, EHANDLE> ft_StopScriptedScene;
inline void StopScriptedScene(CBaseFlex *pActor, EHANDLE hSceneEnt)
{
	ft_StopScriptedScene(pActor, hSceneEnt);
}

extern StaticFuncThunk<int, CStudioHdr *, int, int> ft_SelectWeightedSequence;
inline int SelectWeightedSequence(CStudioHdr *model, int activity, int curSequence = -1)
{
	return ft_SelectWeightedSequence(model, activity, curSequence);
}

#endif
