#include "mem/patch.h"
#include "mem/scan.h"
#include "mem/protect.h"
#include "util/rtti.h"


bool CPatch::Init()
{
	this->m_pszFuncName = this->GetFuncName();
	this->m_iFuncOffMin = this->GetFuncOffMin();
	this->m_iFuncOffMax = this->GetFuncOffMax();
	
	if (this->Verbose()) {
		DevMsg("CPatch::Init: \"%s\" %s\n", this->m_pszFuncName, TypeName(this));
	}
	
	this->m_pFuncAddr = AddrManager::GetAddr(this->m_pszFuncName);
	if (this->m_pFuncAddr == nullptr) {
		DevMsg("CPatch::Init: FAIL: \"%s\": can't find func addr\n", this->m_pszFuncName);
		return false;
	}
	
	if (!this->PostInit()) {
		DevMsg("CPatch::Init: FAIL: \"%s\": PostInit returned false\n", this->m_pszFuncName);
		return false;
	}
	
	this->m_MaskVerify.SetAll(0xff);
	this->m_MaskPatch.SetAll(0x00);
	
	if (!this->GetVerifyInfo(this->m_BufVerify, this->m_MaskVerify)) {
		DevMsg("CPatch::Init: FAIL: \"%s\": GetVerifyInfo returned false\n", this->m_pszFuncName);
		return false;
	}
	
	this->m_BufPatch.CopyFrom(this->m_BufVerify);
	if (!this->GetPatchInfo(this->m_BufPatch, this->m_MaskPatch)) {
		DevMsg("CPatch::Init: FAIL: \"%s\": GetPatchInfo returned false\n", this->m_pszFuncName);
		return false;
	}
	
	return true;
}

bool CPatch::Check()
{
	using PatchScanner = CMaskedScanner<ScanDir::FORWARD, ScanResults::ALL, 1>;
	
	uintptr_t addr_min = (uintptr_t)this->m_pFuncAddr + this->m_iFuncOffMin;
	uintptr_t addr_max = (uintptr_t)this->m_pFuncAddr + this->m_iFuncOffMax + this->m_iLength;
	
	if (this->Verbose()) {
		DevMsg("CPatch::Check: \"%s\" %s\n", this->m_pszFuncName, TypeName(this));
		DevMsg("CPatch::Check: func     %08x\n", (uintptr_t)this->m_pFuncAddr);
		DevMsg("CPatch::Check: off_min      %04x\n", this->m_iFuncOffMin);
		DevMsg("CPatch::Check: off_max      %04x\n", this->m_iFuncOffMax);
		DevMsg("CPatch::Check: addr_min %08x\n", addr_min);
		DevMsg("CPatch::Check: addr_max %08x\n", addr_max);
	}
	
	CScan<PatchScanner> scan(CAddrAddrBounds((void *)addr_min, (void *)addr_max), this->m_BufVerify, this->m_MaskVerify);
	if (!scan.ExactlyOneMatch()) {
		DevMsg("CPatch::Check: FAIL: \"%s\": found %u matching regions:\n", this->m_pszFuncName, scan.Matches().size());
		for (auto match : scan.Matches()) {
			DevMsg("  +0x%04x\n", (uintptr_t)match - (uintptr_t)this->m_pFuncAddr);
		}
		return false;
	}
	
	this->m_bFoundOffset = true;
	this->m_iFuncOffActual = (uintptr_t)scan.FirstMatch() - (uintptr_t)this->m_pFuncAddr;
	
	DevMsg("CPatch::Check: OK: \"%s\": actual offset +0x%04x\n", this->m_pszFuncName, this->m_iFuncOffActual);
	
	return true;
}


void CPatch::Apply()
{
	if (this->VerifyOnly()) return;
	
	if (this->Verbose()) {
		DevMsg("CPatch::Apply: \"%s\" %s\n", this->m_pszFuncName, TypeName(this));
	}
	
	if (this->m_bApplied) {
		return;
	}
	
	if (!this->m_bFoundOffset) {
		DevWarning("CPatch::Apply: \"%s\": haven't found actual offset yet!\n", this->m_pszFuncName);
		return;
	}
	
	if (!this->AdjustPatchInfo(this->m_BufPatch)) {
		DevMsg("CPatch::Apply: \"%s\": GetPatchInfo returned false\n", this->m_pszFuncName);
		return;
	}
	
	uint8_t *ptr = (uint8_t *)((uintptr_t)this->m_pFuncAddr + this->m_iFuncOffActual);
	
	{
		MemProtModifier_RX_RWX(ptr, this->m_iLength);
		
		for (int i = 0; i < this->m_iLength; ++i) {
			uint8_t *mem = ptr + i;
			
			this->m_BufRestore[i] = *mem;
			
			uint8_t p_byte = this->m_BufPatch[i];
			uint8_t p_mask = this->m_MaskPatch[i];
			
			*mem &= ~p_mask;
			*mem |= (p_byte & p_mask);
		}
	}
	
	this->m_bApplied = true;
}

void CPatch::UnApply()
{
	if (this->VerifyOnly()) return;
	
	if (this->Verbose()) {
		DevMsg("CPatch::UnApply: \"%s\" %s\n", this->m_pszFuncName, TypeName(this));
	}
	
	if (!this->m_bApplied) {
		return;
	}
	
	if (!this->m_bFoundOffset) {
		DevWarning("CPatch::UnApply: \"%s\": haven't found actual offset yet!\n", this->m_pszFuncName);
		return;
	}
	
	uint8_t *ptr = (uint8_t *)((uintptr_t)this->m_pFuncAddr + this->m_iFuncOffActual);
	
	{
		MemProtModifier_RX_RWX(ptr, this->m_iLength);
		
		for (int i = 0; i < this->m_iLength; ++i) {
			uint8_t *mem = ptr + i;
			
			uint8_t r_byte = this->m_BufRestore[i];
			uint8_t p_mask = this->m_MaskPatch[i];
			
			*mem &= ~p_mask;
			*mem |= (r_byte & p_mask);
		}
	}
	
	this->m_bApplied = false;
}


uint32_t CPatch::GetActualOffset() const
{
//	if (!this->m_bFoundOffset) return -1;
	assert(this->m_bFoundOffset);
	return this->m_iFuncOffActual;
}

void *CPatch::GetActualLocation() const
{
//	if (!this->m_bFoundOffset) return nullptr;
	assert(this->m_bFoundOffset);
	return (void *)((uintptr_t)this->m_pFuncAddr + this->m_iFuncOffActual);
}
