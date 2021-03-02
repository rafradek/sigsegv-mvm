#ifndef _INCLUDE_SIGSEGV_STUB_BASEANIMATING_H_
#define _INCLUDE_SIGSEGV_STUB_BASEANIMATING_H_


#include "stub/baseentity.h"


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
	void ResetSequenceInfo()                                               {        ft_ResetSequenceInfo   (this); }
	void ResetSequence(int nSequence)                                      {        ft_ResetSequence       (this, nSequence); }
	CStudioHdr *GetModelPtr()                                              { return ft_GetModelPtr         (this); }
	int LookupPoseParameter(CStudioHdr *pStudioHdr, const char *szName)    { return ft_LookupPoseParameter (this, pStudioHdr, szName); }
	float GetPoseParameter(int iParameter)                                 { return ft_GetPoseParameter_int(this, iParameter); }
	float GetPoseParameter(const char *szName)                             { return ft_GetPoseParameter_str(this, szName); }
	void GetBoneTransform(int iBone, matrix3x4_t& pBoneToWorld)            { return ft_GetBoneTransform    (this, iBone, pBoneToWorld); }
	int LookupBone(const char *name)                                       { return ft_LookupBone          (this, name); }
	int LookupAttachment(const char *name)                                 { return ft_LookupAttachment    (this, name); }
	int LookupSequence(const char *name)                                   { return ft_LookupSequence      (this, name); }
	void GetBonePosition(int id, Vector& vec, QAngle& ang)                 { return ft_GetBonePosition     (this, id, vec, ang); }

	void RefreshCollisionBounds()                                          {        vt_RefreshCollisionBounds       (this); }
	
	DECL_SENDPROP   (int,   m_nSkin);
	DECL_SENDPROP   (int,   m_nBody);
	DECL_SENDPROP   (int,   m_nSequence);
	
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
	static MemberFuncThunk<CBaseAnimating *, void>                            ft_ResetSequenceInfo;
	static MemberFuncThunk<CBaseAnimating *, void, int>                       ft_ResetSequence;
	static MemberFuncThunk<CBaseAnimating *, CStudioHdr *>                    ft_GetModelPtr;
	static MemberFuncThunk<CBaseAnimating *, int, CStudioHdr *, const char *> ft_LookupPoseParameter;
	static MemberFuncThunk<CBaseAnimating *, float, int>                      ft_GetPoseParameter_int;
	static MemberFuncThunk<CBaseAnimating *, float, const char *>             ft_GetPoseParameter_str;
	static MemberFuncThunk<CBaseAnimating *, void, int, matrix3x4_t&>         ft_GetBoneTransform;
	static MemberFuncThunk<CBaseAnimating *, int, const char *>               ft_LookupBone;
	static MemberFuncThunk<CBaseAnimating *, void, int, Vector&, QAngle&>     ft_GetBonePosition;
	static MemberFuncThunk<CBaseAnimating *, int, const char *>               ft_LookupAttachment;
	static MemberFuncThunk<CBaseAnimating *, int, const char *>               ft_LookupSequence;

	static MemberVFuncThunk<CBaseAnimating *, void>               vt_RefreshCollisionBounds;

};

class CBaseAnimatingOverlay : public CBaseAnimating {};
class CBaseFlex : public CBaseAnimatingOverlay {};

class CEconEntity : public CBaseAnimating
{
public:
	void DebugDescribe() { ft_DebugDescribe(this); }
	
	// GetAttributeContainer really ought to be in IHasAttributes...
	CAttributeManager *GetAttributeManager()	 { return vt_GetAttributeManager  (this); }
	CAttributeContainer *GetAttributeContainer() { return vt_GetAttributeContainer(this); }
	void GiveTo(CBaseEntity *ent)                {        vt_GiveTo(this, ent); }
	
	CEconItemView *GetItem();
	
	DECL_SENDPROP_RW(bool, m_bValidatedAttachedEntity);
	
private:
	static MemberFuncThunk<CEconEntity *, void> ft_DebugDescribe;
	
	static MemberVFuncThunk<CEconEntity *, CAttributeContainer *> vt_GetAttributeContainer;
	static MemberVFuncThunk<CEconEntity *, CAttributeManager *>   vt_GetAttributeManager;
	static MemberVFuncThunk<CEconEntity *, void, CBaseEntity *>   vt_GiveTo;
};

extern StaticFuncThunk<void, CBaseEntity *, const Vector *, const Vector *> ft_UTIL_SetSize;
inline void UTIL_SetSize( CBaseEntity * entity, const Vector * min, const Vector * max )
{
	ft_UTIL_SetSize(entity, min, max);
}

#endif
