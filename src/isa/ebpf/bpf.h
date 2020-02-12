/* copy from linux/bpf.h, linux/bpf_common.h 
 * commit: 9e6c535c64adf6155e4a11fe8d63b384fe3452f8
 */

#pragma once

/* Instruction classes */
#define BPF_CLASS(code) ((code) & 0x07)
#define   BPF_LD    0x00
#define   BPF_LDX   0x01
#define   BPF_ST    0x02
#define   BPF_STX   0x03
#define   BPF_ALU   0x04
#define   BPF_JMP   0x05
#define   BPF_RET   0x06
#define   BPF_MISC  0x07

/* ld/ldx fields */
#define BPF_SIZE(code)  ((code) & 0x18)
#define   BPF_W   0x00 /* 32-bit */
#define   BPF_H   0x08 /* 16-bit */
#define   BPF_B   0x10 /*  8-bit */
/* eBPF   BPF_DW    0x18  64-bit */
#define BPF_MODE(code)  ((code) & 0xe0)
#define   BPF_IMM   0x00
#define   BPF_ABS   0x20
#define   BPF_IND   0x40
#define   BPF_MEM   0x60
#define   BPF_LEN   0x80
#define   BPF_MSH   0xa0

/* alu/jmp fields */
#define BPF_OP(code)    ((code) & 0xf0)
#define   BPF_ADD   0x00
#define   BPF_SUB   0x10
#define   BPF_MUL   0x20
#define   BPF_DIV   0x30
#define   BPF_OR    0x40
#define   BPF_AND   0x50
#define   BPF_LSH   0x60
#define   BPF_RSH   0x70
#define   BPF_NEG   0x80
#define   BPF_MOD   0x90
#define   BPF_XOR   0xa0

#define   BPF_JA    0x00
#define   BPF_JEQ   0x10
#define   BPF_JGT   0x20
#define   BPF_JGE   0x30
#define   BPF_JSET  0x40
#define BPF_SRC(code)   ((code) & 0x08)
#define   BPF_K     0x00
#define   BPF_X     0x08

/* instruction classes */
#define BPF_JMP32 0x06  /* jmp mode in word width */
#define BPF_ALU64 0x07  /* alu mode in double word width */

/* ld/ldx fields */
#define BPF_DW    0x18  /* double word (64-bit) */
#define BPF_XADD  0xc0  /* exclusive add */

/* alu/jmp fields */
#define BPF_MOV   0xb0  /* mov reg to reg */
#define BPF_ARSH  0xc0  /* sign extending arithmetic shift right */

/* change endianness of a register */
#define BPF_END   0xd0  /* flags for endianness conversion: */
#define BPF_TO_LE 0x00  /* convert to little-endian */
#define BPF_TO_BE 0x08  /* convert to big-endian */
#define BPF_FROM_LE BPF_TO_LE
#define BPF_FROM_BE BPF_TO_BE

/* jmp encodings */
#define BPF_JNE   0x50  /* jump != */
#define BPF_JLT   0xa0  /* LT is unsigned, '<' */
#define BPF_JLE   0xb0  /* LE is unsigned, '<=' */
#define BPF_JSGT  0x60  /* SGT is signed '>', GT in x86 */
#define BPF_JSGE  0x70  /* SGE is signed '>=', GE in x86 */
#define BPF_JSLT  0xc0  /* SLT is signed, '<' */
#define BPF_JSLE  0xd0  /* SLE is signed, '<=' */
#define BPF_CALL  0x80  /* function call */
#define BPF_EXIT  0x90  /* function return */

/* Register numbers */
enum {
  BPF_REG_0 = 0,
  BPF_REG_1,
  BPF_REG_2,
  BPF_REG_3,
  BPF_REG_4,
  BPF_REG_5,
  BPF_REG_6,
  BPF_REG_7,
  BPF_REG_8,
  BPF_REG_9,
  BPF_REG_10,
  __MAX_BPF_REG,
};
