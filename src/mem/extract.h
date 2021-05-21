#ifndef _INCLUDE_SIGSEGV_MEM_EXTRACT_H_
#define _INCLUDE_SIGSEGV_MEM_EXTRACT_H_


#include "util/buf.h"


class IExtractBase
{
public:
	virtual ~IExtractBase() {}
	
	virtual const char *GetFuncName() const = 0;
	virtual void *GetFuncAddr() const { return nullptr; }
	virtual uint32_t GetFuncOffMin() const = 0;
	virtual uint32_t GetFuncOffMax() const = 0;
	virtual uint32_t GetExtractOffset() const = 0;
	virtual int ExtractPtrAsInt() const = 0;
	
	bool Init();
	bool Check();
	
protected:
	IExtractBase(int len) :
		m_iLength(len),
		m_BufExtract(len), m_MaskExtract(len) {}
	
	virtual size_t GetSize() const = 0;
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const = 0;
	
	virtual bool Validate(const uint8_t *ptr) const { return true; }
	
	const int m_iLength;
	
	const char *m_pszFuncName = nullptr;
	uint32_t m_iFuncOffMin = 0;
	uint32_t m_iFuncOffMax = 0;
	uint32_t m_iExtractOffset = 0;
	
	bool m_bFoundOffset = false;
	uint32_t m_iFuncOffActual = 0;
	
	ByteBuf m_BufExtract;
	ByteBuf m_MaskExtract;
	
	void *m_pFuncAddr = nullptr;
};


template<typename T>
class IExtract : public IExtractBase
{
public:
	T Extract() const;
	
	virtual T AdjustValue(T val) const { return val; }
	
protected:
	IExtract(int len) :
		IExtractBase(len) {}
	
	virtual int ExtractPtrAsInt() const override;
	
	virtual size_t GetSize() const override { return sizeof(T); }
};


template<typename T>
T IExtract<T>::Extract() const
{
	assert(this->m_bFoundOffset);
	T val = *reinterpret_cast<T *>((uintptr_t)this->m_pFuncAddr +
		this->m_iFuncOffActual + this->m_iExtractOffset);
	
	return this->AdjustValue(val);
}


template<typename T>
int IExtract<T>::ExtractPtrAsInt() const
{
	if constexpr (std::is_pointer_v<T>) {
		return (int)this->Extract();
	} else {
		assert(false);
		return 0;
	}
}


class IExtractStub final
{
public:
	template<typename... ARGS> IExtractStub(ARGS&&...) {}
};


#endif
