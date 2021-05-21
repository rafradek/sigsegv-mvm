#ifndef _INCLUDE_SIGSEGV_DISASM_DISASM_H_
#define _INCLUDE_SIGSEGV_DISASM_DISASM_H_


#if !defined _MSC_VER
#define __in
#define __out
#define __inout
#define __forceinline //[[gnu::always_inline]]
#define __checkReturn [[gnu::warn_unused_result]]
#endif

#include <X86Disasm.hh>


class Disassembler : private CX86Disasm86
{
private:
	using Base = CX86Disasm86;
	
public:
	template<typename... ARGS> Disassembler(ARGS&&... args) : Base(std::forward<ARGS>(args)...)
	{
		this->SetSyntax  (cs_opt_value::CS_OPT_SYNTAX_INTEL);
		this->SetDetail  (cs_opt_value::CS_OPT_ON);
		this->SetSkipData(cs_opt_value::CS_OPT_OFF);
	}
	
	using Base::GetError;
	using Base::Version;
	using Base::ErrToStr;
	using Base::SetSyntax;
	using Base::SetDetail;
	using Base::SetMode;
	using Base::SetMemMgrFunc;
	using Base::SetSkipData;
	using Base::SetSkipDataCallback;
	
	bool HasError()        { return (this->GetError() != CS_ERR_OK); }
	const char *ErrorStr() { return Base::ErrToStr(this->GetError()); }
	
	template<typename... ARGS> std::unique_ptr<ins_holder_t> Disasm(ARGS&&... args)
	{
		return std::unique_ptr<ins_holder_t>(Base::Disasm(std::forward<ARGS>(args)...));
	}
};


#endif
