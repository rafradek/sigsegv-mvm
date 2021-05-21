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


#endif
