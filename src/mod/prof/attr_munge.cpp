#include "mod.h"


namespace Mod::Prof::Attr_Munge
{
#undef VPROF
#undef VPROF_BUDGET
#undef VPROF_ENTER_SCOPE
#undef VPROF_EXIT_SCOPE
	
	
	/* EVIL: subclass CVProfile so we can set a couple of protected members */
	class CVProfile2 : public CVProfile
	{
	public:
		void SetCurrentNode(CVProfNode *pNode) { this->m_pCurNode = pNode; }
		void SetAtRoot(bool bAtRoot)           { this->m_fAtRoot = bAtRoot; }
	};
	static_assert(sizeof(CVProfile2) == sizeof(CVProfile));
	
	
	bool ShouldEnterScope(const char *name, const char *group)
	{
		auto& prof = *static_cast<CVProfile2 *>(&g_VProfCurrentProfile);
		
		if (strcmp(name, "CAttributeManager::AttribHookValue") == 0) return true;
		
		for (CVProfNode *node = prof.GetCurrentNode(); node != prof.GetRoot(); node = node->GetParent()) {
			if (strcmp(node->GetName(), "CAttributeManager::AttribHookValue") == 0) return true;
		}
		
		return false;
	}
	
	
	void VProfEnter_Real(const char *name, const char *group)
	{
		auto& prof = *static_cast<CVProfile2 *>(&g_VProfCurrentProfile);
		
		if ((prof.IsEnabled() || !prof.AtRoot()) && prof.InTargetThread()) {
			if (strcmp(name, prof.GetCurrentNode()->GetName()) != 0) {
				prof.SetCurrentNode(prof.GetCurrentNode()->GetSubNode(name, 1, group, 0));
			}
			
			prof.GetCurrentNode()->EnterScope();
			prof.SetAtRoot(false);
		}
	}
	
	void VProfExit_Real()
	{
		auto& prof = *static_cast<CVProfile2 *>(&g_VProfCurrentProfile);
		
		if ((prof.IsEnabled() || !prof.AtRoot()) && prof.InTargetThread()) {
			if (prof.GetCurrentNode()->ExitScope()) {
				prof.SetCurrentNode(prof.GetCurrentNode()->GetParent());
			}
			
			prof.SetAtRoot(prof.GetCurrentNode() == prof.GetRoot());
		}
	}
	
	
	class CVProfScope2
	{
	public:
		CVProfScope2(const char *name, const char *group)
		{
			if (g_VProfCurrentProfile.IsEnabled() && ShouldEnterScope(name, group)) {
				this->m_bEnabled = true;
			}
			
			if (this->m_bEnabled) {
				VProfEnter_Real(name, group);
			}
		}
		
		~CVProfScope2()
		{
			if (this->m_bEnabled) {
				VProfExit_Real();
			}
		}
		
	private:
		bool m_bEnabled = false;
	};
#define VPROF_BUDGET(name, group) CVProfScope2 VPROF_VAR_NAME(VPROF_)(name, group)
	
	
	DETOUR_DECL_STATIC(int, CAttributeManager_AttribHookValue_int, int value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
		VPROF_BUDGET("CAttributeManager::AttribHookValue", "Attributes");
		return DETOUR_STATIC_CALL(value, attr, ent, vec, b1);
	}
	
	DETOUR_DECL_STATIC(float, CAttributeManager_AttribHookValue_float, float value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
		VPROF_BUDGET("CAttributeManager::AttribHookValue", "Attributes");
		return DETOUR_STATIC_CALL(value, attr, ent, vec, b1);
	}
	
	
	DETOUR_DECL_STATIC(void, GlobalAttrModifier_TFHalloweenAttribHookMunger, const char *in, CUtlConstStringBase<char> *out)
	{
		VPROF_BUDGET("GlobalAttrModifier_TFHalloweenAttribHookMunger", "Mungers");
		DETOUR_STATIC_CALL(in, out);
	}
	
	DETOUR_DECL_STATIC(void, GlobalAttrModifier_MvMAttribHookMunger, const char *in, CUtlConstStringBase<char> *out)
	{
		VPROF_BUDGET("GlobalAttrModifier_MvMAttribHookMunger", "Mungers");
		DETOUR_STATIC_CALL(in, out);
	}
	
	
	DETOUR_DECL_STATIC(int, V_vsnprintfRet, char *pDest, int maxLenInCharacters, const char *pFormat, va_list params, bool *pbTruncated)
	{
		VPROF_BUDGET("V_vsnprintfRet", "Strings");
		return DETOUR_STATIC_CALL(pDest, maxLenInCharacters, pFormat, params, pbTruncated);
	}
	
	
	DETOUR_DECL_MEMBER(void, CUtlConstStringBase_char_Set, const char *pValue)
	{
		VPROF_BUDGET("CUtlConstStringBase<char>::Set", "Strings");
		DETOUR_MEMBER_CALL(pValue);
	}
	
	
	DETOUR_DECL_MEMBER(void *, IMemAlloc_Alloc, size_t nSize)
	{
		VPROF_BUDGET("IMemAlloc::Alloc", "Memory");
		return DETOUR_MEMBER_CALL(nSize);
	}
	
	DETOUR_DECL_MEMBER(void *, IMemAlloc_Realloc, void *pMem, size_t nSize)
	{
		VPROF_BUDGET("IMemAlloc::Realloc", "Memory");
		return DETOUR_MEMBER_CALL(pMem, nSize);
	}
	
	DETOUR_DECL_MEMBER(void, IMemAlloc_Free, void *pMem)
	{
		VPROF_BUDGET("IMemAlloc::Free", "Memory");
		DETOUR_MEMBER_CALL(pMem);
	}
	
	
	DETOUR_DECL_STATIC(char *, d_strdup, const char *s)
	{
		VPROF_BUDGET("strdup", "glibc");
		return DETOUR_STATIC_CALL(s);
	}
	
	DETOUR_DECL_STATIC(void *, d_malloc, size_t size)
	{
		VPROF_BUDGET("malloc", "glibc");
		return DETOUR_STATIC_CALL(size);
	}
	
	DETOUR_DECL_STATIC(void, d_free, void *ptr)
	{
		VPROF_BUDGET("free", "glibc");
		DETOUR_STATIC_CALL(ptr);
	}
	
	
	DETOUR_DECL_MEMBER(void, CVProfile_EnterScope, const char *pszName, int detailLevel, const char *pBudgetGroupName, bool bAssertAccounted, int budgetFlags)
	{
		/* block everything */
	}
	
	DETOUR_DECL_MEMBER(void, CVProfile_ExitScope)
	{
		/* block everything */
	}
	
	
	std::map<std::string, Color> s_GroupColors;
	
	CON_COMMAND(sig_prof_attr_munge_color, "")
	{
		if (args.ArgC() < 3) {
			Msg("Not enough arguments.\n");
			return;
		}
		
		const char *group = args[1];
		uint32_t rgba = std::strtoll(args[2], nullptr, 16);
		
		s_GroupColors[group] = Color(
			(rgba >> 24) & 0xff,
			(rgba >> 16) & 0xff,
			(rgba >>  8) & 0xff,
			(rgba >>  0) & 0xff);
	}
	
	
	DETOUR_DECL_MEMBER(void, CVProfile_GetBudgetGroupColor, int budgetGroupID, int& r, int& g, int& b, int& a)
	{
		auto prof = reinterpret_cast<CVProfile2 *>(this);
		
		const char *name = prof->GetBudgetGroupName(budgetGroupID);
		
		if (s_GroupColors.count(name) != 0) {
			Color& c = s_GroupColors[name];
			r = c.r(); g = c.g(); b = c.b(); a = c.a();
		} else {
			r = 0x00; g = 0x00; b = 0x00; a = 0x00;
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Prof:Attr_Munge")
		{
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_int,   "CAttributeManager::AttribHookValue<int>");
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_float, "CAttributeManager::AttribHookValue<float>");
			
			MOD_ADD_DETOUR_STATIC(GlobalAttrModifier_TFHalloweenAttribHookMunger, "GlobalAttrModifier_TFHalloweenAttribHookMunger");
			MOD_ADD_DETOUR_STATIC(GlobalAttrModifier_MvMAttribHookMunger,         "GlobalAttrModifier_MvMAttribHookMunger");
			
			MOD_ADD_DETOUR_STATIC(V_vsnprintfRet, "V_vsnprintfRet");
			
			// inlined on Windows
		//	MOD_ADD_DETOUR_MEMBER(CUtlConstStringBase_char_Set, "CUtlConstStringBase<char>::Set");
			
			MOD_ADD_DETOUR_MEMBER(IMemAlloc_Alloc,   "IMemAlloc::Alloc");
			MOD_ADD_DETOUR_MEMBER(IMemAlloc_Realloc, "IMemAlloc::Realloc");
			MOD_ADD_DETOUR_MEMBER(IMemAlloc_Free,    "IMemAlloc::Free");
			
		//	this->AddDetour(new CDetour("strdup", reinterpret_cast<void *>(&strdup), GET_STATIC_CALLBACK(d_strdup), GET_STATIC_INNERPTR(d_strdup)));
		//	this->AddDetour(new CDetour("malloc", reinterpret_cast<void *>(&malloc), GET_STATIC_CALLBACK(d_malloc), GET_STATIC_INNERPTR(d_malloc)));
		//	this->AddDetour(new CDetour("free",   reinterpret_cast<void *>(&free),   GET_STATIC_CALLBACK(d_free),   GET_STATIC_INNERPTR(d_free)));
			
			this->AddDetour(new CDetour("CVProfile::EnterScope", GetAddrOfMemberFunc<CVProfile, void, const char *, int, const char *, bool, int>(&CVProfile::EnterScope), GET_MEMBER_CALLBACK(CVProfile_EnterScope), GET_MEMBER_INNERPTR(CVProfile_EnterScope)));
			this->AddDetour(new CDetour("CVProfile::ExitScope",  GetAddrOfMemberFunc                                                             (&CVProfile::ExitScope),  GET_MEMBER_CALLBACK(CVProfile_ExitScope),  GET_MEMBER_INNERPTR(CVProfile_ExitScope)));
			
			this->AddDetour(new CDetour("CVProfile::GetBudgetGroupColor", GetAddrOfMemberFunc(&CVProfile::GetBudgetGroupColor), GET_MEMBER_CALLBACK(CVProfile_GetBudgetGroupColor), GET_MEMBER_INNERPTR(CVProfile_GetBudgetGroupColor)));
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_prof_attr_munge", "0", FCVAR_NOTIFY,
		"Mod: profile heap thrashing in attribute mungers",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
