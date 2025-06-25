#include "mem/alloc.h"
#include "mem/protect.h"
#include "mem/opcode.h"
#include "mem/wrapper.h"
#include "mem/func_copy.h"

#include <udis86.h>

/* get number of instruction operands */
static unsigned int UD86_num_operands(struct ud *ud)
{
	for (unsigned int i = 0; i < 4; ++i) {
		if (ud_insn_opr(ud, i) == nullptr) return i;
	}
	return 4;
}

static void UD86_init_buffer(struct ud *ud, const uint8_t *input, size_t size)
{
	ud_init(ud);
#ifdef PLATFORM_64BITS
	ud_set_mode(ud, 64);
#else
	ud_set_mode(ud, 32);
#endif
	ud_set_pc(ud, (uint64_t)input);
	ud_set_input_buffer(ud, input, size);
}

/* fix [rip + disp32] operands*/
static bool UD86_insn_fix_disp32(struct ud *ud, const uint8_t *func = nullptr, const uint8_t *dest = nullptr, uint8_t *buffer = nullptr)
{
	auto mnemonic = ud_insn_mnemonic(ud);
	if (ud_insn_opr(ud, 0) == nullptr) return false;
	
	//Msg("Instr %s %d | length %d offset %d ptr %p opr count %d\n", ud_insn_asm(ud), ud_insn_mnemonic(ud), ud_insn_len(ud), ud_insn_off(ud), ud_insn_ptr(ud), UD86_num_operands(ud));
	//if (ud_insn_len(ud) != 5)       return false;
	//if (UD86_num_operands(ud) != 1) return false;

	int32_t writeOffset = -1;
	uint64_t dispValue = -1ULL;
	int memArgs = 0;
	int immArgSize = 0;
	
	for (unsigned int i = 0; i < UD86_num_operands(ud); i++) {
		auto op = ud_insn_opr(ud, i);
		if (op->type == UD_OP_MEM && op->base == UD_R_RIP) {
			dispValue = op->lval.uqword;
			memArgs++;
		}
		if (op->type == UD_OP_IMM) {
			immArgSize = op->size / 8;
		}
	}
	writeOffset = ud_insn_len(ud) - 4 - immArgSize;
	
	if (memArgs != 1 || writeOffset == -1 || dispValue == -1ULL || (dispValue & 0xFFFFFFFF) != dispValue) return false;


	if (dest == nullptr || func == nullptr || buffer == nullptr) return true;

	int32_t diff = (intptr_t)(dest) - (intptr_t)func;

	memcpy(buffer, (uint8_t *)ud_insn_off(ud), ud_insn_len(ud));

	*(int32_t *)(buffer + writeOffset) -= diff;
	
	return true;
}

/* fix instruction: '(jmp|call) <rel_imm32>' */
static bool UD86_insn_fix_jmpcall_rel_imm32(struct ud *ud, size_t &newLength, const uint8_t *func = nullptr, const uint8_t *dest = nullptr, uint8_t *buffer = nullptr, int32_t shortJumpMax = 0, int32_t shortJumpMin = 0)
{
	auto mnemonic = ud_insn_mnemonic(ud);
	// if (mnemonic != UD_Ijmp && mnemonic != UD_Icall ) return false;
	
	// if (ud_insn_len(ud) != 5)       return false;
	if (UD86_num_operands(ud) != 1) return false;
	
	newLength = ud_insn_len(ud);

	const auto *op0 = ud_insn_opr(ud, 0);
	if (op0->type != UD_OP_JIMM) return false;

	int32_t jumpTarget = op0->size == 8 ? op0->lval.sbyte : op0->lval.sdword;
    if (jumpTarget >= shortJumpMin && jumpTarget <= shortJumpMax) {
        return false;
    }

    if (buffer != nullptr)
	    memcpy(buffer, (uint8_t *)ud_insn_off(ud), ud_insn_len(ud));

	if (op0->size != 32) {
        // Convert jumps from 8 bit to 32 bit address
        if (mnemonic == UD_Ijmp) {
            if (buffer != nullptr) {
                buffer[0] = OPCODE_JMP_REL_IMM32;
                buffer[2] = 0;
                buffer[3] = 0;
                buffer[4] = 0;
            }

            newLength = 5;
        }
        else {
            if (buffer != nullptr) {
                buffer[5] = 0;
                buffer[4] = 0;
                buffer[3] = 0;
                buffer[2] = buffer[1];
                buffer[1] = (buffer[0] & 0x0F) | 0x80;
                buffer[0] = OPCODE_JCC_REL_IMM32;
            }
            newLength = 6;
        }
	}
	int32_t writeOffset = newLength - 4;

	if (func == nullptr || dest == nullptr || buffer == nullptr) return true;

	int32_t diff = (intptr_t)(dest + (newLength - ud_insn_len(ud))) - (intptr_t)func;

	*(int32_t *)(buffer + writeOffset) -= diff;

	return true;
}

/* detect instruction: 'call <rel_imm32>' */
static bool UD86_insn_is_call_rel_imm32(struct ud *ud, const uint8_t **call_target = nullptr)
{
	auto mnemonic = ud_insn_mnemonic(ud);
	if (mnemonic != UD_Icall) return false;
	
	if (ud_insn_len(ud) != 5)       return false;
	if (UD86_num_operands(ud) != 1) return false;
	
	const auto *op0 = ud_insn_opr(ud, 0);
	if (op0->type != UD_OP_JIMM) return false;
	if (op0->size != 32)         return false;
	
	/* optional parameter: write out the call destination address */
	if (call_target != nullptr) {
		*call_target = (const uint8_t *)(ud_insn_off(ud) + ud_insn_len(ud) + op0->lval.sdword);
	}
	
	return true;
}

/* detect instruction: 'mov e[acdb]x,[esp]' */
static bool UD86_insn_is_mov_r32_rtnval(struct ud *ud, Reg *dest_reg = nullptr)
{
	auto mnemonic = ud_insn_mnemonic(ud);
	if (mnemonic != UD_Imov) return false;
	
	if (ud_insn_len(ud) != 3)       return false;
	if (UD86_num_operands(ud) != 2) return false;
	
	const auto *op0 = ud_insn_opr(ud, 0);
	if (op0->type != UD_OP_REG) return false;
	if (op0->size != 32)        return false;
	
	Reg reg;
	switch (op0->base) {
	case UD_R_EAX: reg = REG_AX; break;
	case UD_R_ECX: reg = REG_CX; break;
	case UD_R_EDX: reg = REG_DX; break;
	case UD_R_EBX: reg = REG_BX; break;
	default: return false;
	}
	
	const auto *op1 = ud_insn_opr(ud, 1);
	if (op1->type   != UD_OP_MEM) return false;
	if (op1->size   != 32)        return false;
	if (op1->base   != UD_R_ESP)  return false;
	if (op1->index  != UD_NONE)   return false;
	if (op1->scale  != UD_NONE)   return false;
	if (op1->offset != 0)         return false;
	
	/* optional parameter: write out the first operand base register */
	if (dest_reg != nullptr) {
		*dest_reg = reg;
	}
	
	return true;
}

/* detect instruction: 'ret' */
static bool UD86_insn_is_ret(struct ud *ud)
{
	auto mnemonic = ud_insn_mnemonic(ud);
	if (mnemonic != UD_Iret) return false;
	
	if (ud_insn_len(ud) != 1)       return false;
	if (UD86_num_operands(ud) != 0) return false;
	
	return true;
}


/* detect whether an instruction is a call to __i686.get_pc_thunk.(ax|cx|dx|bx) */
static bool UD86_insn_is_call_to_get_pc_thunk(struct ud *ud, Reg *dest_reg = nullptr)
{
	const uint8_t *call_target;
	if (!UD86_insn_is_call_rel_imm32(ud, &call_target)) return false;
	
	ud_t ux;
    UD86_init_buffer(&ux, call_target, 0x100);
	
	if (ud_decode(&ux) == 0) return false;
	if (!UD86_insn_is_mov_r32_rtnval(&ux, dest_reg)) return false;
	
	if (ud_decode(&ux) == 0) return false;
	if (!UD86_insn_is_ret(&ux)) return false;
	
	return true;
}

size_t CopyAndFixUpFuncBytes(size_t len_min, size_t len_max, const uint8_t *source, const uint8_t *destination_address, uint8_t *buffer, bool stop_at_nop)
{
	auto dest = destination_address;
	
	ud_t ud;
    UD86_init_buffer(&ud, source, len_max);
	
    size_t len_calc = 0;
    size_t len_min_calc = len_min;
	while (len_calc < len_min_calc) {
		size_t len_decoded = ud_decode(&ud);
		size_t orig_len_decoded = len_decoded;

        if (len_decoded == 0) break;

		// They typically determine end of function
		if (stop_at_nop && (ud_insn_mnemonic(&ud) == UD_Inop || ud_insn_mnemonic(&ud) == UD_Iint3)) break;
		
        UD86_insn_fix_jmpcall_rel_imm32(&ud, len_decoded, source + len_calc, dest, nullptr, len_min_calc - len_calc - len_decoded, -len_calc - len_decoded);
		// In case a fixup increased instruction size
		len_min_calc += len_decoded - orig_len_decoded;
		len_calc += len_decoded;
		dest += len_decoded;
	}

    if (len_calc > len_max) return 0;
    
    if (buffer == nullptr) return len_calc;

    UD86_init_buffer(&ud, source, len_max);
    
    dest = destination_address;
	size_t len_actual = 0;
	while (len_actual < len_min) {
		size_t len_decoded = ud_decode(&ud);
		size_t orig_len_decoded = len_decoded;

        if (len_decoded == 0) break;

		// They typically determine end of function
		if (stop_at_nop && (ud_insn_mnemonic(&ud) == UD_Inop || ud_insn_mnemonic(&ud) == UD_Iint3)) break;
		
        // fixup jmp and call relative offsets
        if (UD86_insn_fix_jmpcall_rel_imm32(&ud, len_decoded, source + len_actual, dest, buffer, len_min - len_actual - len_decoded, -len_actual - len_decoded)) {
        }
        // fixup [rip+disp32] relative offsets
        else if (UD86_insn_fix_disp32(&ud, source + len_actual, dest, buffer)){
        }
        else {
            memcpy(buffer, (uint8_t *)ud_insn_off(&ud), len_decoded);
        }
		len_min += len_decoded - orig_len_decoded;
		len_actual += len_decoded;
		dest += len_decoded;
        buffer += len_decoded;
	}
    assert(len_calc == len_actual);
	return len_actual;
}