//developed by Mahmoud Khairy, Purdue Univ
//abdallm@purdue.edu

#ifndef TRACE_OPCODE_H
#define TRACE_OPCODE_H

#include "../abstract_hardware_model.h"
#include <unordered_map>
#include <string>


enum TraceInstrOpcode {
	OP_FADD = 1, OP_FADD32I, OP_FCHK, OP_FFMA32I, OP_FFMA, OP_FMNMX, OP_FMUL, OP_FMUL32I, OP_FSEL, OP_FSET, OP_FSETP,
	OP_FSWZADD, OP_MUFU, OP_HADD2, OP_HADD2_32I, OP_HFMA2, OP_HFMA2_32I, OP_HMUL2, OP_HMUL2_32I, OP_HSET2, OP_HSETP2,
	OP_HMMA, OP_DADD, OP_DFMA, OP_DMUL, OP_DSETP,
	OP_BMSK, OP_BREV, OP_FLO, OP_IABS, OP_IADD, OP_IADD3, OP_IADD32I, OP_IDP, OP_IDP4A, OP_IMAD, OP_IMMA, OP_IMNMX,
	OP_IMUL, OP_IMUL32I, OP_ISCADD, OP_ISCADD32I, OP_ISETP, OP_LEA, OP_LOP, OP_LOP3, OP_LOP32I, OP_POPC, OP_SHF, OP_SHR,
	OP_VABSDIFF, OP_VABSDIFF4,
	OP_F2F, OP_F2I, OP_I2F, OP_I2I, OP_I2IP, OP_FRND, OP_MOV, OP_MOV32I, OP_PRMT, OP_SEL, OP_SGXT, OP_SHFL, OP_PLOP3,
	OP_PSETP, OP_P2R, OP_R2P, OP_LD, OP_LDC, OP_LDG, OP_LDL, OP_LDS, OP_ST, OP_STG, OP_STL, OP_STS, OP_MATCH, OP_QSPC,
	OP_ATOM, OP_ATOMS, OP_ATOMG, OP_RED, OP_CCTL, OP_CCTLL, OP_ERRBAR, OP_MEMBAR, OP_CCTLT,
	OP_TEX, OP_TLD, OP_TLD4,
	OP_TMML, OP_TXD, OP_TXQ, OP_BMOV, OP_BPT, OP_BRA, OP_BREAK, OP_BRX, OP_BSSY, OP_BSYNC, OP_CALL, OP_EXIT, OP_JMP, OP_JMX,
	OP_KILL, OP_NANOSLEEP, OP_RET, OP_RPCMOV, OP_RTT, OP_WARPSYNC, OP_YIELD, OP_B2R, OP_BAR, OP_CS2R, OP_CSMTEST, OP_DEPBAR,
	OP_GETLMEMBASE, OP_LEPC, OP_NOP, OP_PMTRIG, OP_R2B, OP_S2R, OP_SETCTAID,  OP_SETLMEMBASE, OP_VOTE, OP_VOTE_VTG,
	SASS_NUM_OPCODES /* The total number of opcodes. */
};
typedef enum TraceInstrOpcode sass_op_type;

/*
enum uarch_op_t {
   NO_OP=-1,
   ALU_OP=1,
   SFU_OP,
   TENSOR_CORE_OP,
   DP_OP,
   SP_OP,
   INTP_OP,
   ALU_SFU_OP,
   LOAD_OP,
   TENSOR_CORE_LOAD_OP,
   TENSOR_CORE_STORE_OP,
   STORE_OP,
   BRANCH_OP,
   BARRIER_OP,
   MEMORY_BARRIER_OP,
   CALL_OPS,
   RET_OPS
};
typedef enum uarch_op_t op_type;
 */

struct OpcodeChar
{
	OpcodeChar(unsigned m_opcode, unsigned m_opcode_category) {
		opcode = m_opcode;
		opcode_category = m_opcode_category;
	}
	unsigned opcode;
	unsigned opcode_category;
};


#endif
