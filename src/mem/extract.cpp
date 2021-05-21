#include "mem/extract.h"
#include "mem/scan.h"
#include "addr/addr.h"


bool IExtractBase::Init()
{
	this->m_pszFuncName = this->GetFuncName();
	if (this->m_pszFuncName != nullptr) {
		this->m_pFuncAddr = AddrManager::GetAddr(this->m_pszFuncName);
	} else {
		this->m_pFuncAddr = this->GetFuncAddr();
	}
	
	if (this->m_pFuncAddr == nullptr) {
		return false;
	}
	
	this->m_iFuncOffMin = this->GetFuncOffMin();
	this->m_iFuncOffMax = this->GetFuncOffMax();
	this->m_iExtractOffset = this->GetExtractOffset();
	
	assert(this->m_iExtractOffset + this->GetSize() <= (unsigned int)this->m_iLength);
	
	this->m_MaskExtract.SetAll(0xff);
	
	if (!this->GetExtractInfo(this->m_BufExtract, this->m_MaskExtract)) {
		return false;
	}
	
	return true;
}

bool IExtractBase::Check()
{
	using ExtractScanner = CMaskedScanner<ScanDir::FORWARD, ScanResults::ALL, 1>;
	
	uintptr_t addr_min = (uintptr_t)this->m_pFuncAddr + this->m_iFuncOffMin;
	uintptr_t addr_max = (uintptr_t)this->m_pFuncAddr + this->m_iFuncOffMax + this->m_iLength;
	
//	DevMsg("addr_min: %08x\n", addr_min);
//	DevMsg("addr_max: %08x\n", addr_max);
//	DevMsg("length:   %08x\n", this->m_iLength);
//	
//	DevMsg("m_BufExtract:\n");
//	for (int i = 0; i < this->m_iLength; ++i) {
//		DevMsg(" %02x", this->m_BufExtract[i]);
//	}
//	DevMsg("\nm_MaskExtract:\n");
//	for (int i = 0; i < this->m_iLength; ++i) {
//		DevMsg(" %02x", this->m_MaskExtract[i]);
//	}
//	DevMsg("\n");
	
	CScan<ExtractScanner> scan(CAddrAddrBounds((void *)addr_min, (void *)addr_max), this->m_BufExtract, this->m_MaskExtract);
	if (!scan.ExactlyOneMatch()) {
		DevMsg("IExtract::Check: FAIL: \"%s\": found %u matching regions:\n", this->m_pszFuncName, scan.Matches().size());
		for (auto match : scan.Matches()) {
			DevMsg("  +0x%04x\n", (uintptr_t)match - (uintptr_t)this->m_pFuncAddr);
		}
		return false;
	}
	
	if (!this->Validate((const uint8_t *)scan.FirstMatch())) {
		DevMsg("IExtract::Check: FAIL: \"%s\": found 1 matching region (+0x%04x), but it was rejected by Validate()\n",
			this->m_pszFuncName, ((uintptr_t)scan.FirstMatch() - (uintptr_t)this->m_pFuncAddr));
		return false;
	}
	
	this->m_bFoundOffset = true;
	this->m_iFuncOffActual = (uintptr_t)scan.FirstMatch() - (uintptr_t)this->m_pFuncAddr;
	
	DevMsg("IExtract::Check: OK: \"%s\": actual offset +0x%04x\n", this->m_pszFuncName, this->m_iFuncOffActual);
	
	return true;
}
