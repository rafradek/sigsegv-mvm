#ifndef _INCLUDE_SIGSEGV_UTIL_BUF_H_
#define _INCLUDE_SIGSEGV_UTIL_BUF_H_


class ByteBuf
{
public:
	ByteBuf(size_t size);
	~ByteBuf();
	
	uint8_t& operator[](size_t idx);
	const uint8_t& operator[](size_t idx) const;
	
	void SetAll(uint8_t val);
	void SetRange(size_t idx, size_t len, uint8_t val);
	
	uint32_t GetDword(size_t idx) const;
	void SetDword(size_t idx, uint32_t val);
	
	float GetFloat(size_t idx) const;
	void SetFloat(size_t idx, float val);
	
	void CopyFrom(const ByteBuf& that);
	void CopyFrom(const uint8_t *arr);
	
	void Dump() const;
	
	size_t GetSize() const { return this->m_iSize; }
	
	const uint8_t *CPtr() const { return this->m_Buf; }
	      uint8_t *MPtr() const { return this->m_Buf; } // unsafe!
	
private:
	const size_t m_iSize;
	uint8_t *m_Buf = nullptr;
};


inline ByteBuf::ByteBuf(size_t size) :
	m_iSize(size)
{
	this->m_Buf = new uint8_t[size];
	this->SetAll(0x00);
}

inline ByteBuf::~ByteBuf()
{
	delete[] this->m_Buf;
}


inline uint8_t& ByteBuf::operator[](size_t idx)
{
	assert(idx >= 0 && idx < this->m_iSize);
	return this->m_Buf[idx];
}

inline const uint8_t& ByteBuf::operator[](size_t idx) const
{
	assert(idx >= 0 && idx < this->m_iSize);
	return this->m_Buf[idx];
}


inline void ByteBuf::SetAll(uint8_t val)
{
	std::fill_n(this->m_Buf, this->m_iSize, val);
}

inline void ByteBuf::SetRange(size_t idx, size_t len, uint8_t val)
{
	assert(idx >= 0 && idx + len <= this->m_iSize);
	std::fill_n(this->m_Buf + idx, len, val);
}


inline uint32_t ByteBuf::GetDword(size_t idx) const
{
	assert(idx >= 0 && idx + sizeof(uint32_t) <= this->m_iSize);
	return *reinterpret_cast<uint32_t *>(this->m_Buf + idx);
}

inline void ByteBuf::SetDword(size_t idx, uint32_t val)
{
	assert(idx >= 0 && idx + sizeof(uint32_t) <= this->m_iSize);
	*reinterpret_cast<uint32_t *>(this->m_Buf + idx) = val;
}


inline float ByteBuf::GetFloat(size_t idx) const
{
	assert(idx >= 0 && idx + sizeof(float) <= this->m_iSize);
	return *reinterpret_cast<float *>(this->m_Buf + idx);
}

inline void ByteBuf::SetFloat(size_t idx, float val)
{
	assert(idx >= 0 && idx + sizeof(float) <= this->m_iSize);
	*reinterpret_cast<float *>(this->m_Buf + idx) = val;
}


inline void ByteBuf::CopyFrom(const ByteBuf& that)
{
	assert(this->m_iSize == that.m_iSize);
	std::copy_n(that.m_Buf, this->m_iSize, this->m_Buf);
}

inline void ByteBuf::CopyFrom(const uint8_t *arr)
{
	std::copy_n(arr, this->m_iSize, this->m_Buf);
}


inline void ByteBuf::Dump() const
{
	constexpr size_t bytes_per_line  = 16;
	constexpr size_t bytes_per_group =  4;
	
	int addr_digits = [](size_t size){
		if (size <= (1 <<  8)) return 2;
		if (size <= (1 << 16)) return 4;
		if (size <= (1 << 24)) return 6;
		return 8;
	}(this->GetSize());
	
	DevMsg("   %*s__00_01_02_03__04_05_06_07__08_09_0A_0B__0C_0D_0E_0F__\n", addr_digits, "");
	for (size_t i = 0; i < this->GetSize(); i += bytes_per_line) {
		std::string line = CFmtStrN<16>("+0x%0*X", addr_digits, i).Get();
		
		for (size_t j = i; j < this->GetSize() && j < i + bytes_per_line; ++j) {
			line += CFmtStrN<16>("%*s%02X", ((j % bytes_per_group == 0) ? 2 : 1), "", this->m_Buf[j]).Get();
		}
		
		DevMsg("%s\n", line.c_str());
	}
}


#endif
