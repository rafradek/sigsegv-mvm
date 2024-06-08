#ifndef _INCLUDE_SIGSEGV_MEM_PATCH_H_
#define _INCLUDE_SIGSEGV_MEM_PATCH_H_


#warning TODO: add patch validation code, like we just did with detours!


#warning POSSIBLY PROBLEMATIC: NO PROTECTIONS AGAINST MULTI-PATCHING THE SAME FUNC/AREA!
// i.e. we need an analogous mechanism to CDetouredFunc; CPatchedFunc or whatever
// ...and we'd probably want to check for funcs that are detoured+patched as well, as that's potentially problematic too


#include "util/buf.h"
#include "addr/addr.h"


class IPatch
{
public:
	virtual ~IPatch() {}
	
	virtual bool Verbose() const = 0;
	virtual bool VerifyOnly() const = 0;
	
	virtual bool Init() = 0;
	virtual bool Check() = 0;
	
	virtual void Apply() = 0;
	virtual void UnApply() = 0;
	
protected:
	IPatch() {}
};


class CPatch : public IPatch
{
public:
	virtual ~CPatch() {}
	
	virtual bool Verbose() const override    { return false; }
	virtual bool VerifyOnly() const override { return false; }
	
	virtual bool Init() override final;
	virtual bool Check() override final;
	
	virtual void Apply() override final;
	virtual void UnApply() override final;
	
	int GetLength() const { return this->m_iLength; }
	virtual const char *GetFuncName() const = 0;
	virtual uint32_t GetFuncOffMin() const = 0;
	virtual uint32_t GetFuncOffMax() const = 0;
	
	/* only call these after verification has succeeded! */
	uint32_t GetActualOffset() const;
	void *GetActualLocation() const;
	
protected:
	CPatch(int len) :
		m_iLength(len),
		m_BufVerify(len), m_BufPatch(len),
		m_MaskVerify(len), m_MaskPatch(len),
		m_BufRestore(len) {}
	
	virtual bool PostInit() { return true; }
	
	/* these are both called one time, early, by CPatch::Init */
	virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const = 0;
	virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const = 0;
	
	/* this is called multiple times, late, by CPatch::Apply */
	virtual bool AdjustPatchInfo(ByteBuf& buf) const { return true; }
	
	void *GetFuncAddr() const { return this->m_pFuncAddr; }
	
private:
	const int m_iLength;
	
	const char *m_pszFuncName = nullptr;
	uint32_t m_iFuncOffMin = 0;
	uint32_t m_iFuncOffMax = 0;
	
	bool m_bFoundOffset = false;
	uint32_t m_iFuncOffActual = 0;
	
	ByteBuf m_BufVerify;
	ByteBuf m_BufPatch;
	ByteBuf m_MaskVerify;
	ByteBuf m_MaskPatch;
	
	void *m_pFuncAddr = nullptr;
	
	bool m_bApplied = false;
	ByteBuf m_BufRestore;
};


class CVerify : public CPatch
{
public:
	virtual ~CVerify() {}
	
	virtual bool VerifyOnly() const override final { return true; }
	
protected:
	CVerify(int len) : CPatch(len) {}
	
	virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override final { return true; }
};

class CFuncReplace : public CPatch
{
public:
	CFuncReplace(size_t size, void *func, const char *func_name) : CPatch(size + 32), m_zFuncSize(size), m_szFuncName(func_name), m_pFunc(func) {}

	virtual const char *GetFuncName() const override { return m_szFuncName.c_str(); }
	virtual uint32_t GetFuncOffMin() const override  { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override  { return 0x0000; } // @ +0x00af
	
	virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		auto data = (uint8_t *) m_pFunc;

		buf.CopyFrom(data);
		mask.SetAll(0x00);

		return true;
	}
	
	virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override;
	
	virtual bool AdjustPatchInfo(ByteBuf& buf) const override
	{
		return true;
	}
private:

	size_t m_zFuncSize;
	std::string m_szFuncName;
	void *m_pFunc;
};
#define REPLACE_FUNC_STATIC_ATTRIBUTES(attributes, ret, name, ...) \
	extern char __start_##name[]; \
	extern char __stop_##name[]; \
	\
	__attribute__((noinline, section(#name), attributes)) ret FuncReplace_##name(__VA_ARGS__)

#define REPLACE_FUNC_MEMBER_ATTRIBUTES(attributes, ret, name, ...) \
	class FuncReplaceClass_##name \
	{ \
	public: \
		__attribute__((noinline, section(#name), attributes)) ret callback(__VA_ARGS__); \
	}; \
	\
	extern char __start_##name[]; \
	extern char __stop_##name[]; \
	\
	ret FuncReplaceClass_##name::callback(__VA_ARGS__)


// Replace original function code with provided function. Remember that:
// 1. It must be placed outside of namespace
// 2. The original function code + 16 byte alignment must be larger than the replacement. There is no protection against patching smaller original functions!
#define REPLACE_FUNC_STATIC(ret, name, ...) REPLACE_FUNC_STATIC_ATTRIBUTES(,ret,name,__VA_ARGS__)

// Replace original function code with provided function. Remember that:
// 1. It must be placed outside of namespace
// 2. The original function code + 16 byte alignment must be larger than the replacement. There is no protection against patching smaller original functions!
#define REPLACE_FUNC_MEMBER(ret, name, ...) REPLACE_FUNC_MEMBER_ATTRIBUTES(,ret,name,__VA_ARGS__)

// Same as REPLACE_FUNC_STATIC but the function is space optimized instead
#define REPLACE_FUNC_STATIC_OS(ret, name, ...) REPLACE_FUNC_STATIC_ATTRIBUTES(optimize("Os"),ret,name,__VA_ARGS__)
// Same as REPLACE_FUNC_MEMBER but the function is space optimized instead
#define REPLACE_FUNC_MEMBER_OS(ret, name, ...) REPLACE_FUNC_MEMBER_ATTRIBUTES(optimize("Os"),ret,name,__VA_ARGS__)

#endif
