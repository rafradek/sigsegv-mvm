#ifndef _INCLUDE_SIGSEGV_MEM_OPCODE_H_
#define _INCLUDE_SIGSEGV_MEM_OPCODE_H_


enum OpCode : uint8_t
{
	OPCODE_PUSH_IMM32        = 0x68,
	OPCODE_NOP               = 0x90,
	OPCODE_MOV_REG32_IMM32   = 0xb8, // b8~bf [b8+reg]
	OPCODE_INT3              = 0xcc,
	OPCODE_CALL_REL_IMM32    = 0xe8,
	OPCODE_JMP_REL_IMM32     = 0xe9,
	OPCODE_FF                = 0xff, // opcode ext in modrm.reg
};
static_assert(sizeof(OpCode) == 1);


enum Mod : uint8_t
{
	MOD_INDIRECT        = 0b00,
	MOD_INDIRECT_DISP8  = 0b01,
	MOD_INDIRECT_DISP32 = 0b10,
	MOD_DIRECT          = 0b11,
	
	MOD_MASK            = 0b11,
};
static_assert(sizeof(Mod) <= 1);

enum Reg : uint8_t
{
	REG_AX   = 0b000,
	REG_CX   = 0b001,
	REG_DX   = 0b010,
	REG_BX   = 0b011,
	REG_SP   = 0b100,
	REG_BP   = 0b101,
	REG_SI   = 0b110,
	REG_DI   = 0b111,
	
	OP_FF_INC_RM32   = 0b000,
	OP_FF_DEC_RM32   = 0b001,
	OP_FF_CALL_RM32  = 0b010,
	OP_FF_CALL_FAR   = 0b011,
	OP_FF_JMP_RM32   = 0b100,
	OP_FF_JMP_FAR    = 0b101,
	OP_FF_PUSH_RM32  = 0b110,
	
	REG_MASK = 0b111,
};
static_assert(sizeof(Reg) <= 1);

enum RM : uint8_t
{
	RM_AX     = 0b000,
	RM_CX     = 0b001,
	RM_DX     = 0b010,
	RM_BX     = 0b011,
	RM_SIB    = 0b100,
	RM_DISP32 = 0b101,
	RM_SI     = 0b110,
	RM_DI     = 0b111,
	
	RM_MASK   = 0b111,
};
static_assert(sizeof(RM) <= 1);


enum Scale : uint8_t
{
	SCALE_1    = 0b00,
	SCALE_2    = 0b01,
	SCALE_4    = 0b10,
	SCALE_8    = 0b11,
	
	SCALE_MASK = 0b11,
};
static_assert(sizeof(Scale) <= 1);


// TODO: turn these struct members back into enum types once the GCC devs get their heads out of their asses
// ("warning: ‘ModRM::mod’ is too small to hold all values of ‘enum Mod’" etc bullshit)
struct ModRM
{
	uint8_t mod : 2;
	uint8_t reg : 3;
	uint8_t rm  : 3;
	
	operator uint8_t() const { return *reinterpret_cast<const uint8_t *>(this); }
};
static_assert(sizeof(ModRM) == 1);

// TODO: turn these back into enum types, see above
struct SIB
{
	uint8_t scale : 2;
	uint8_t index : 3;
	uint8_t base  : 3;
	
	operator uint8_t() const { return *reinterpret_cast<const uint8_t *>(this); }
};
static_assert(sizeof(SIB) == 1);


class X86Instr
{
public:
	/* is this atomic? no
	 * do we care?  ...no */
	void Write() const
	{
		std::copy_n(this->m_Buf, this->m_nSize, this->m_pWhere);
	}
	
	/* pad out extra space with NOP's to avoid confusing the disassembler */
	void WritePadded(size_t len, OpCode pad = OPCODE_NOP) const
	{
		assert(len >= this->m_nSize);
		std::copy_n(this->m_Buf, this->m_nSize, this->m_pWhere);
		std::fill_n(this->m_pWhere + this->m_nSize, len - this->m_nSize, pad);
	}
	
protected:
	X86Instr(size_t size, uint8_t *where) : m_nSize(size), m_pWhere(where) {}
	
	size_t m_nSize;
	uint8_t *m_pWhere;
	uint8_t m_Buf[15];
};


template<size_t SIZE>
class X86InstrSized : public X86Instr
{
public:
	X86InstrSized(uint8_t *where) : X86Instr(Size(), where) {}
	
	static constexpr size_t Size() { return SIZE; }
};


class PushImm32 : public X86InstrSized<5>
{
public:
	PushImm32(uint8_t *where, uint32_t value) : X86InstrSized(where)
	{
		this->m_Buf[0] = OPCODE_PUSH_IMM32;
		*reinterpret_cast<uint32_t *>(this->m_Buf + 1) = value;
	}
};

class MovRegImm32 : public X86InstrSized<5>
{
public:
	MovRegImm32(uint8_t *where, Reg dst, uint32_t value) : X86InstrSized(where)
	{
		assert(dst == (dst & REG_MASK));
		this->m_Buf[0] = (OPCODE_MOV_REG32_IMM32 + (dst & REG_MASK));
	}
};

class CallRelImm32 : public X86InstrSized<5>
{
public:
	/* rel: basis for the relative offset; usually equal to where the instruction will go, but not always */
	CallRelImm32(uint8_t *where, uint32_t target, const uint8_t *rel) : X86InstrSized(where)
	{
		this->m_Buf[0] = OPCODE_CALL_REL_IMM32;
		*reinterpret_cast<uint32_t *>(this->m_Buf + 1) = (target - ((uintptr_t)rel + Size()));
	}
	/* rel = where */
	CallRelImm32(uint8_t *where, uint32_t target) : CallRelImm32(where, target, where) {}
};

class JmpRelImm32 : public X86InstrSized<5>
{
public:
	/* rel: basis for the relative offset; usually equal to where the instruction will go, but not always */
	JmpRelImm32(uint8_t *where, uint32_t target, const uint8_t *rel) : X86InstrSized(where)
	{
		this->m_Buf[0] = OPCODE_JMP_REL_IMM32;
		*reinterpret_cast<uint32_t *>(this->m_Buf + 1) = (target - ((uintptr_t)rel + Size()));
	}
	/* rel = where */
	JmpRelImm32(uint8_t *where, uint32_t target) : JmpRelImm32(where, target, where) {}
};

class CallIndirectMem32 : public X86InstrSized<6>
{
public:
	CallIndirectMem32(uint8_t *where, uint32_t target) : X86InstrSized(where)
	{
		this->m_Buf[0] = OPCODE_FF;
		this->m_Buf[1] = ModRM{ MOD_INDIRECT, OP_FF_CALL_RM32, RM_DISP32 };
		*reinterpret_cast<uint32_t *>(this->m_Buf + 2) = target;
	}
};


#endif
