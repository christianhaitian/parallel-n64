/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assemble.c                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2007 Richard Goedeken (Richard42)                       *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __ASSEMBLE_H__
#define __ASSEMBLE_H__

#include "r4300/recomph.h"
#include "api/callbacks.h"
#include "osal/preproc.h"

#include <stdlib.h>
#include <stdint.h>

#ifdef __x86_64__
extern int64_t reg[32];
typedef uint64_t native_type;

#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7
#else
typedef uint32_t native_type;
extern long long int reg[32];
#endif

#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7

#define AX 0
#define CX 1
#define DX 2
#define BX 3
#define SP 4
#define BP 5
#define SI 6
#define DI 7

#define AL 0
#define CL 1
#define DL 2
#define BL 3
#define AH 4
#define CH 5
#define DH 6
#define BH 7

extern int branch_taken;

extern const uint16_t trunc_mode, round_mode, ceil_mode, floor_mode;

void jump_start_rel8(void);
void jump_end_rel8(void);
void jump_start_rel32(void);
void jump_end_rel32(void);
void add_jump(unsigned int pc_addr, unsigned int mi_addr, unsigned int absolute64);

static INLINE void put8(unsigned char octet)
{
   (*inst_pointer)[code_length] = octet;
   code_length++;
   if (code_length == max_code_length)
   {
      *inst_pointer = (unsigned char*)realloc_exec(*inst_pointer, max_code_length, max_code_length+8192);
      max_code_length += 8192;
   }
}

static INLINE void put32(unsigned int dword)
{
   if ((code_length + 4) >= max_code_length)
   {
      *inst_pointer = (unsigned char*)realloc_exec(*inst_pointer, max_code_length, max_code_length+8192);
      max_code_length += 8192;
   }
   *((unsigned int *) (*inst_pointer + code_length)) = dword;
   code_length += 4;
}

static INLINE void put64(unsigned long long qword)
{
   if ((code_length + 8) >= max_code_length)
   {
      *inst_pointer = realloc_exec(*inst_pointer, max_code_length, max_code_length+8192);
      max_code_length += 8192;
   }
   *((unsigned long long *) (*inst_pointer + code_length)) = qword;
   code_length += 8;
}

static INLINE int rel_r15_offset(void *dest, const char *op_name)
{
   /* calculate the destination pointer's offset from the base of the r4300 registers */
   long long rel_offset = (long long) ((unsigned char *) dest - (unsigned char *) reg);

   if (llabs(rel_offset) > 0x7fffffff)
   {
      DebugMessage(M64MSG_ERROR, "Error: destination %p more than 2GB away from r15 base %p in %s()", dest, reg, op_name);
      OSAL_BREAKPOINT_INTERRUPT;
   }

   return (int) rel_offset;
}

#ifndef __x86_64__
static INLINE void mov_m8_reg8(unsigned char *m8, int reg8)
{
   put8(0x88);
   put8((reg8 << 3) | 5);
   put32((unsigned int)(m8));
}

static INLINE void mov_reg16_m16(int reg16, unsigned short *m16)
{
   put8(0x66);
   put8(0x8B);
   put8((reg16 << 3) | 5);
   put32((unsigned int)(m16));
}

static INLINE void mov_m16_reg16(unsigned short *m16, int reg16)
{
   put8(0x66);
   put8(0x89);
   put8((reg16 << 3) | 5);
   put32((unsigned int)(m16));
}

static INLINE void cmp_reg32_m32(int reg32, unsigned int *m32)
{
   put8(0x3B);
   put8((reg32 << 3) | 5);
   put32((unsigned int)(m32));
}
#endif

static INLINE void mov_memoffs32_eax(unsigned int *memoffs32)
{
   put8(0xA3);
#ifdef __x86_64__
   put64((unsigned long long) memoffs32);
#else
   put32((unsigned int)(memoffs32));
#endif
}

#ifdef __x86_64__
static INLINE void mov_rax_memoffs64(unsigned long long *memoffs64)
{
   put8(0x48);
   put8(0xA1);
   put64((unsigned long long) memoffs64);
}

static INLINE void mov_memoffs64_rax(unsigned long long *memoffs64)
{
   put8(0x48);
   put8(0xA3);
   put64((unsigned long long) memoffs64);
}

static INLINE void mov_m8rel_xreg8(unsigned char *m8, int xreg8)
{
   int offset = rel_r15_offset(m8, "mov_m8rel_xreg8");

   put8(0x41 | ((xreg8 & 8) >> 1));
   put8(0x88);
   put8(0x87 | ((xreg8 & 7) << 3));
   put32(offset);
}

static INLINE void mov_xreg16_m16rel(int xreg16, unsigned short *m16)
{
   int offset = rel_r15_offset(m16, "mov_xreg16_m16rel");

   put8(0x66);
   put8(0x41 | ((xreg16 & 8) >> 1));
   put8(0x8B);
   put8(0x87 | ((xreg16 & 7) << 3));
   put32(offset);
}

static INLINE void mov_m16rel_xreg16(unsigned short *m16, int xreg16)
{
   int offset = rel_r15_offset(m16, "mov_m16rel_xreg16");

   put8(0x66);
   put8(0x41 | ((xreg16 & 8) >> 1));
   put8(0x89);
   put8(0x87 | ((xreg16 & 7) << 3));
   put32(offset);
}

static INLINE void cmp_xreg32_m32rel(int xreg32, unsigned int *m32)
{
   int offset = rel_r15_offset(m32, "cmp_xreg32_m32rel");

   put8(0x41 | ((xreg32 & 8) >> 1));
   put8(0x3B);
   put8(0x87 | ((xreg32 & 7) << 3));
   put32(offset);
}

static INLINE void cmp_xreg64_m64rel(int xreg64, unsigned long long *m64)
{
   int offset = rel_r15_offset(m64, "cmp_xreg64_m64rel");

   put8(0x49 | ((xreg64 & 8) >> 1));
   put8(0x3B);
   put8(0x87 | ((xreg64 & 7) << 3));
   put32(offset);
}
#endif

static INLINE void cmp_reg32_reg32(int reg1, int reg2)
{
   put8(0x39);
   put8((reg2 << 3) | reg1 | 0xC0);
}

#ifdef __x86_64__
static INLINE void cmp_reg64_reg64(int reg1, int reg2)
{
   put8(0x48);
   put8(0x39);
   put8((reg2 << 3) | reg1 | 0xC0);
}
#endif

static INLINE void cmp_reg32_imm8(int reg32, unsigned char imm8)
{
   put8(0x83);
   put8(0xF8 + reg32);
   put8(imm8);
}

#ifdef __x86_64__
static INLINE void cmp_reg64_imm8(int reg64, unsigned char imm8)
{
   put8(0x48);
   put8(0x83);
   put8(0xF8 + reg64);
   put8(imm8);
}
#else
static INLINE void add_m32_reg32(unsigned int *m32, int reg32)
{
   put8(0x01);
   put8((reg32 << 3) | 5);
   put32((unsigned int)(m32));
}

static INLINE void sub_reg32_m32(int reg32, unsigned int *m32)
{
   put8(0x2B);
   put8((reg32 << 3) | 5);
   put32((unsigned int)(m32));
}

static INLINE void mov_eax_memoffs32(unsigned int *memoffs32)
{
   put8(0xA1);
   put32((unsigned int)(memoffs32));
}

static INLINE void cmp_preg32pimm32_imm8(int reg32, unsigned int imm32, unsigned char imm8)
{
   put8(0x80);
   put8(0xB8 + reg32);
   put32(imm32);
   put8(imm8);
}

static INLINE void test_reg32_imm32(int reg32, unsigned int imm32)
{
   put8(0xF7);
   put8(0xC0 + reg32);
   put32(imm32);
}

static INLINE void test_m32_imm32(unsigned int *m32, unsigned int imm32)
{
   put8(0xF7);
   put8(0x05);
   put32((unsigned int)m32);
   put32(imm32);
}

static INLINE void sbb_reg32_reg32(int reg1, int reg2)
{
   put8(0x19);
   put8((reg2 << 3) | reg1 | 0xC0);
}

static INLINE void sub_reg32_imm32(int reg32, unsigned int imm32)
{
   put8(0x81);
   put8(0xE8 + reg32);
   put32(imm32);
}
#endif

static INLINE void cmp_reg32_imm32(int reg32, unsigned int imm32)
{
   put8(0x81);
   put8(0xF8 + reg32);
   put32(imm32);
}

#ifdef __x86_64__
static INLINE void cmp_reg64_imm32(int reg64, unsigned int imm32)
{
   put8(0x48);
   put8(0x81);
   put8(0xF8 + reg64);
   put32(imm32);
}

static INLINE void cmp_preg64preg64_imm8(int reg1, int reg2, unsigned char imm8)
{
   put8(0x80);
   put8(0x3C);
   put8((reg1 << 3) | reg2);
   put8(imm8);
}

static INLINE void sete_m8rel(unsigned char *m8)
{
   int offset = rel_r15_offset(m8, "sete_m8rel");

   put8(0x41);
   put8(0x0F);
   put8(0x94);
   put8(0x87);
   put32(offset);
}

static INLINE void setne_m8rel(unsigned char *m8)
{
   int offset = rel_r15_offset(m8, "setne_m8rel");

   put8(0x41);
   put8(0x0F);
   put8(0x95);
   put8(0x87);
   put32(offset);
}

static INLINE void setl_m8rel(unsigned char *m8)
{
   int offset = rel_r15_offset(m8, "setl_m8rel");

   put8(0x41);
   put8(0x0F);
   put8(0x9C);
   put8(0x87);
   put32(offset);
}

static INLINE void setle_m8rel(unsigned char *m8)
{
   int offset = rel_r15_offset(m8, "setle_m8rel");

   put8(0x41);
   put8(0x0F);
   put8(0x9E);
   put8(0x87);
   put32(offset);
}

static INLINE void setg_m8rel(unsigned char *m8)
{
   int offset = rel_r15_offset(m8, "setg_m8rel");

   put8(0x41);
   put8(0x0F);
   put8(0x9F);
   put8(0x87);
   put32(offset);
}
#endif

static INLINE void setge_m8rel(unsigned char *m8)
{
   int offset = rel_r15_offset(m8, "setge_m8rel");

   put8(0x41);
   put8(0x0F);
   put8(0x9D);
   put8(0x87);
   put32(offset);
}

static INLINE void setl_reg8(unsigned int reg8)
{
   put8(0x40);  /* we need an REX prefix to use the uniform byte registers */
   put8(0x0F);
   put8(0x9C);
   put8(0xC0 | reg8);
}

static INLINE void setb_reg8(unsigned int reg8)
{
   put8(0x40);  /* we need an REX prefix to use the uniform byte registers */
   put8(0x0F);
   put8(0x92);
   put8(0xC0 | reg8);
}

static INLINE void test_m32rel_imm32(unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(m32, "test_m32rel_imm32");

   put8(0x41);
   put8(0xF7);
   put8(0x87);
   put32(offset);
   put32(imm32);
}

static INLINE void add_m32rel_xreg32(unsigned int *m32, int xreg32)
{
   int offset = rel_r15_offset(m32, "add_m32rel_xreg32");

   put8(0x41 | ((xreg32 & 8) >> 1));
   put8(0x01);
   put8(0x87 | ((xreg32 & 7) << 3));
   put32(offset);
}

static INLINE void sub_xreg32_m32rel(int xreg32, unsigned int *m32)
{
   int offset = rel_r15_offset(m32, "sub_xreg32_m32rel");

   put8(0x41 | ((xreg32 & 8) >> 1));
   put8(0x2B);
   put8(0x87 | ((xreg32 & 7) << 3));
   put32(offset);
}

static INLINE void sub_reg32_reg32(int reg1, int reg2)
{
   put8(0x29);
   put8((reg2 << 3) | reg1 | 0xC0);
}

static INLINE void sub_reg64_reg64(int reg1, int reg2)
{
   put8(0x48);
   put8(0x29);
   put8((reg2 << 3) | reg1 | 0xC0);
}

static INLINE void sub_reg64_imm32(int reg64, unsigned int imm32)
{
   put8(0x48);
   put8(0x81);
   put8(0xE8 + reg64);
   put32(imm32);
}

static INLINE void sub_eax_imm32(unsigned int imm32)
{
   put8(0x2D);
   put32(imm32);
}

static INLINE void jne_rj(unsigned char saut)
{
   put8(0x75);
   put8(saut);
}

static INLINE void je_rj(unsigned char saut)
{
   put8(0x74);
   put8(saut);
}

static INLINE void jb_rj(unsigned char saut)
{
   put8(0x72);
   put8(saut);
}

static INLINE void jbe_rj(unsigned char saut)
{
   put8(0x76);
   put8(saut);
}

static INLINE void ja_rj(unsigned char saut)
{
   put8(0x77);
   put8(saut);
}

static INLINE void jae_rj(unsigned char saut)
{
   put8(0x73);
   put8(saut);
}

static INLINE void jp_rj(unsigned char saut)
{
   put8(0x7A);
   put8(saut);
}

static INLINE void je_near_rj(unsigned int saut)
{
   put8(0x0F);
   put8(0x84);
   put32(saut);
}

static INLINE void mov_reg32_imm32(int reg32, unsigned int imm32)
{
   put8(0xB8+reg32);
   put32(imm32);
}

static INLINE void mov_reg64_imm64(int reg64, unsigned long long imm64)
{
   put8(0x48);
   put8(0xB8+reg64);
   put64(imm64);
}

static INLINE void jmp_imm_short(char saut)
{
   put8(0xEB);
   put8(saut);
}

#ifndef __x86_64__
static INLINE void or_m32_imm32(unsigned int *m32, unsigned int imm32)
{
   put8(0x81);
   put8(0x0D);
   put32((unsigned int)(m32));
   put32(imm32);
}
#endif

static INLINE void or_m32rel_imm32(unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(m32, "or_m32rel_imm32");

   put8(0x41);
   put8(0x81);
   put8(0x8F);
   put32(offset);
   put32(imm32);
}

static INLINE void or_reg64_reg64(unsigned int reg1, unsigned int reg2)
{
   put8(0x48);
   put8(0x09);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void and_reg64_reg64(unsigned int reg1, unsigned int reg2)
{
   put8(0x48);
   put8(0x21);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void and_m32rel_imm32(unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(m32, "and_m32rel_imm32");

   put8(0x41);
   put8(0x81);
   put8(0xA7);
   put32(offset);
   put32(imm32);
}

static INLINE void xor_reg32_reg32(unsigned int reg1, unsigned int reg2)
{
   put8(0x31);
   put8(0xC0 | (reg2 << 3) | reg1);
}

#ifndef __x86_64__
static INLINE void and_m32_imm32(unsigned int *m32, unsigned int imm32)
{
   put8(0x81);
   put8(0x25);
   put32((unsigned int)(m32));
   put32(imm32);
}

static INLINE void sub_m32_imm32(unsigned int *m32, unsigned int imm32)
{
   put8(0x81);
   put8(0x2D);
   put32((unsigned int)(m32));
   put32(imm32);
}
#endif

static INLINE void and_reg32_reg32(unsigned int reg1, unsigned int reg2)
{
   put8(0x21);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void xor_reg64_reg64(unsigned int reg1, unsigned int reg2)
{
   put8(0x48);
   put8(0x31);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void add_reg64_imm32(unsigned int reg64, unsigned int imm32)
{
   put8(0x48);
   put8(0x81);
   put8(0xC0+reg64);
   put32(imm32);
}

static INLINE void add_reg32_imm32(unsigned int reg32, unsigned int imm32)
{
   put8(0x81);
   put8(0xC0+reg32);
   put32(imm32);
}

#ifndef __x86_64__
static INLINE void cmp_m32_imm32(unsigned int *m32, unsigned int imm32)
{
   put8(0x81);
   put8(0x3D);
   put32((unsigned int)(m32));
   put32(imm32);
}

static INLINE void inc_m32(unsigned int *m32)
{
   put8(0xFF);
   put8(0x05);
   put32((unsigned int)(m32));
}
#endif

static INLINE void inc_m32rel(unsigned int *m32)
{
   int offset = rel_r15_offset(m32, "inc_m32rel");

   put8(0x41);
   put8(0xFF);
   put8(0x87);
   put32(offset);
}

static INLINE void cmp_m32rel_imm32(unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(m32, "cmp_m32rel_imm32");

   put8(0x41);
   put8(0x81);
   put8(0xBF);
   put32(offset);
   put32(imm32);
}

#ifndef __x86_64__
static INLINE void mov_m32_imm32(unsigned int *m32, unsigned int imm32)
{
   put8(0xC7);
   put8(0x05);
   put32((unsigned int)(m32));
   put32(imm32);
}
#endif

static INLINE void cmp_eax_imm32(unsigned int imm32)
{
   put8(0x3D);
   put32(imm32);
}

static INLINE void mov_m32rel_imm32(unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(m32, "mov_m32rel_imm32");

   put8(0x41);
   put8(0xC7);
   put8(0x87);
   put32(offset);
   put32(imm32);
}

static INLINE void shr_reg32_imm8(unsigned int reg32, unsigned char imm8)
{
   put8(0xC1);
   put8(0xE8+reg32);
   put8(imm8);
}

static INLINE void call_reg32(unsigned int reg32)
{
   put8(0xFF);
   put8(0xD0+reg32);
}

static INLINE void jmp(unsigned int mi_addr)
{
#ifdef __x86_64__
   put8(0xFF);
   put8(0x25);
   put32(0);
   put64(0);
   add_jump(code_length-8, mi_addr, 1);
#else
   put8(0xE9);
   put32(0);
   add_jump(code_length-4, mi_addr, 0);
#endif
}

static INLINE void cdq(void)
{
   put8(0x99);
}

static INLINE void call_reg64(unsigned int reg64)
{
   put8(0xFF);
   put8(0xD0+reg64);
}

static INLINE void shr_reg64_imm8(unsigned int reg64, unsigned char imm8)
{
   put8(0x48);
   put8(0xC1);
   put8(0xE8+reg64);
   put8(imm8);
}

static INLINE void shr_reg32_cl(unsigned int reg32)
{
   put8(0xD3);
   put8(0xE8+reg32);
}

static INLINE void shr_reg64_cl(unsigned int reg64)
{
   put8(0x48);
   put8(0xD3);
   put8(0xE8+reg64);
}

static INLINE void sar_reg32_cl(unsigned int reg32)
{
   put8(0xD3);
   put8(0xF8+reg32);
}

static INLINE void sar_reg64_cl(unsigned int reg64)
{
   put8(0x48);
   put8(0xD3);
   put8(0xF8+reg64);
}

static INLINE void shl_reg32_cl(unsigned int reg32)
{
   put8(0xD3);
   put8(0xE0+reg32);
}

static INLINE void shld_reg32_reg32_cl(unsigned int reg1, unsigned int reg2)
{
   put8(0x0F);
   put8(0xA5);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void shld_reg32_reg32_imm8(unsigned int reg1, unsigned int reg2, unsigned char imm8)
{
   put8(0x0F);
   put8(0xA4);
   put8(0xC0 | (reg2 << 3) | reg1);
   put8(imm8);
}

static INLINE void shl_reg64_cl(unsigned int reg64)
{
   put8(0x48);
   put8(0xD3);
   put8(0xE0+reg64);
}

static INLINE void sar_reg32_imm8(unsigned int reg32, unsigned char imm8)
{
   put8(0xC1);
   put8(0xF8+reg32);
   put8(imm8);
}

static INLINE void shrd_reg32_reg32_imm8(unsigned int reg1, unsigned int reg2, unsigned char imm8)
{
   put8(0x0F);
   put8(0xAC);
   put8(0xC0 | (reg2 << 3) | reg1);
   put8(imm8);
}

static INLINE void sar_reg64_imm8(unsigned int reg64, unsigned char imm8)
{
   put8(0x48);
   put8(0xC1);
   put8(0xF8+reg64);
   put8(imm8);
}

#ifndef __x86_64__
static INLINE void mul_m32(unsigned int *m32)
{
   put8(0xF7);
   put8(0x25);
   put32((unsigned int)(m32));
}
#endif

static INLINE void mul_m32rel(unsigned int *m32)
{
   int offset = rel_r15_offset(m32, "mul_m32rel");

   put8(0x41);
   put8(0xF7);
   put8(0xA7);
   put32(offset);
}

static INLINE void imul_reg32(unsigned int reg32)
{
   put8(0xF7);
   put8(0xE8+reg32);
}

static INLINE void mul_reg64(unsigned int reg64)
{
   put8(0x48);
   put8(0xF7);
   put8(0xE0+reg64);
}

static INLINE void mul_reg32(unsigned int reg32)
{
   put8(0xF7);
   put8(0xE0+reg32);
}

static INLINE void idiv_reg32(unsigned int reg32)
{
   put8(0xF7);
   put8(0xF8+reg32);
}

static INLINE void div_reg32(unsigned int reg32)
{
   put8(0xF7);
   put8(0xF0+reg32);
}

static INLINE void add_reg32_reg32(unsigned int reg1, unsigned int reg2)
{
   put8(0x01);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void add_reg64_reg64(unsigned int reg1, unsigned int reg2)
{
   put8(0x48);
   put8(0x01);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void jmp_reg32(unsigned int reg32)
{
   put8(0xFF);
   put8(0xE0 + reg32);
}

static INLINE void jmp_reg64(unsigned int reg64)
{
   put8(0xFF);
   put8(0xE0 + reg64);
}

static INLINE void mov_reg32_preg64(unsigned int reg1, unsigned int reg2)
{
   put8(0x8B);
   put8((reg1 << 3) | reg2);
}

static INLINE void mov_preg64_reg32(int reg1, int reg2)
{
   put8(0x89);
   put8((reg2 << 3) | reg1);
}

static INLINE void mov_reg64_preg64(int reg1, int reg2)
{
   put8(0x48);
   put8(0x8B);
   put8((reg1 << 3) | reg2);
}

static INLINE void mov_reg32_preg32preg32pimm32(int reg1, int reg2, int reg3, unsigned int imm32)
{
   put8(0x8B);
   put8((reg1 << 3) | 0x84);
   put8(reg2 | (reg3 << 3));
   put32(imm32);
}

static INLINE void mov_preg32pimm32_imm8(int reg32, unsigned int imm32, unsigned char imm8)
{
   put8(0xC6);
   put8(0x80 + reg32);
   put32(imm32);
   put8(imm8);
}

static INLINE void mov_preg32_reg32(int reg1, int reg2)
{
   put8(0x89);
   put8((reg2 << 3) | reg1);
}

static INLINE void mov_preg32pimm32_reg16(int reg32, unsigned int imm32, int reg16)
{
   put8(0x66);
   put8(0x89);
   put8(0x80 | reg32 | (reg16 << 3));
   put32(imm32);
}

static INLINE void mov_preg32pimm32_reg32(int reg1, unsigned int imm32, int reg2)
{
   put8(0x89);
   put8(0x80 | reg1 | (reg2 << 3));
   put32(imm32);
}

static INLINE void mov_preg32pimm32_reg8(int reg32, unsigned int imm32, int reg8)
{
   put8(0x88);
   put8(0x80 | reg32 | (reg8 << 3));
   put32(imm32);
}

static INLINE void mov_reg32_preg32pimm32(int reg1, int reg2, unsigned int imm32)
{
   put8(0x8B);
   put8(0x80 | (reg1 << 3) | reg2);
   put32(imm32);
}

static INLINE void mov_reg32_preg32x4pimm32(int reg1, int reg2, unsigned int imm32)
{
   put8(0x8B);
   put8((reg1 << 3) | 4);
   put8(0x80 | (reg2 << 3) | 5);
   put32(imm32);
}

static INLINE void mov_reg32_preg32(unsigned int reg1, unsigned int reg2)
{
   put8(0x8B);
   put8((reg1 << 3) | reg2);
}

static INLINE void mov_reg32_preg64preg64pimm32(int reg1, int reg2, int reg3, unsigned int imm32)
{
   put8(0x8B);
   put8((reg1 << 3) | 0x84);
   put8(reg2 | (reg3 << 3));
   put32(imm32);
}

static INLINE void mov_preg64preg64pimm32_reg32(int reg1, int reg2, unsigned int imm32, int reg3)
{
   put8(0x89);
   put8((reg3 << 3) | 0x84);
   put8(reg1 | (reg2 << 3));
   put32(imm32);
}

static INLINE void mov_reg64_preg64preg64pimm32(int reg1, int reg2, int reg3, unsigned int imm32)
{
   put8(0x48);
   put8(0x8B);
   put8((reg1 << 3) | 0x84);
   put8(reg2 | (reg3 << 3));
   put32(imm32);
}

static INLINE void mov_reg32_preg64preg64(int reg1, int reg2, int reg3)
{
   put8(0x8B);
   put8((reg1 << 3) | 0x04);
   put8((reg2 << 3) | reg3);
}

static INLINE void mov_reg64_preg64preg64(int reg1, int reg2, int reg3)
{
   put8(0x48);
   put8(0x8B);
   put8((reg1 << 3) | 0x04);
   put8(reg2 | (reg3 << 3));
}

static INLINE void mov_reg32_preg64pimm32(int reg1, int reg2, unsigned int imm32)
{
   put8(0x8B);
   put8(0x80 | (reg1 << 3) | reg2);
   put32(imm32);
}

static INLINE void mov_reg64_preg64pimm32(int reg1, int reg2, unsigned int imm32)
{
   put8(0x48);
   put8(0x8B);
   put8(0x80 | (reg1 << 3) | reg2);
   put32(imm32);
}

static INLINE void mov_reg64_preg64pimm8(int reg1, int reg2, unsigned int imm8)
{
   put8(0x48);
   put8(0x8B);
   put8(0x40 | (reg1 << 3) | reg2);
   put8(imm8);
}

static INLINE void mov_reg64_preg64x8preg64(int reg1, int reg2, int reg3)
{
   put8(0x48);
   put8(0x8B);
   put8((reg1 << 3) | 4);
   put8(0xC0 | (reg2 << 3) | reg3);
}

static INLINE void mov_preg64preg64_reg8(int reg1, int reg2, int reg8)
{
   put8(0x88);
   put8(0x04 | (reg8 << 3));
   put8((reg1 << 3) | reg2);
}

static INLINE void mov_preg64preg64_imm8(int reg1, int reg2, unsigned char imm8)
{
   put8(0xC6);
   put8(0x04);
   put8((reg1 << 3) | reg2);
   put8(imm8);
}

static INLINE void mov_preg64preg64_reg16(int reg1, int reg2, int reg16)
{
   put8(0x66);
   put8(0x89);
   put8(0x04 | (reg16 << 3));
   put8((reg1 << 3) | reg2);
}

static INLINE void mov_preg64preg64_reg32(int reg1, int reg2, int reg32)
{
   put8(0x89);
   put8(0x04 | (reg32 << 3));
   put8((reg1 << 3) | reg2);
}

static INLINE void mov_preg64pimm32_reg32(int reg1, unsigned int imm32, int reg2)
{
   put8(0x89);
   put8(0x80 | reg1 | (reg2 << 3));
   put32(imm32);
}

static INLINE void mov_preg64pimm8_reg64(int reg1, unsigned int imm8, int reg2)
{
   put8(0x48);
   put8(0x89);
   put8(0x40 | (reg2 << 3) | reg1);
   put8(imm8);
}

static INLINE void add_eax_imm32(unsigned int imm32)
{
   put8(0x05);
   put32(imm32);
}

static INLINE void shl_reg32_imm8(unsigned int reg32, unsigned char imm8)
{
   put8(0xC1);
   put8(0xE0 + reg32);
   put8(imm8);
}

static INLINE void shl_reg64_imm8(unsigned int reg64, unsigned char imm8)
{
   put8(0x48);
   put8(0xC1);
   put8(0xE0 + reg64);
   put8(imm8);
}

#ifndef __x86_64__
static INLINE void movsx_reg32_m8(int reg32, unsigned char *m8)
{
   put8(0x0F);
   put8(0xBE);
   put8((reg32 << 3) | 5);
   put32((unsigned int)(m8));
}
#endif

static INLINE void movsx_reg32_8preg32pimm32(int reg1, int reg2, unsigned int imm32)
{
   put8(0x0F);
   put8(0xBE);
   put8((reg1 << 3) | reg2 | 0x80);
   put32(imm32);
}

static INLINE void movsx_reg32_16preg32pimm32(int reg1, int reg2, unsigned int imm32)
{
   put8(0x0F);
   put8(0xBF);
   put8((reg1 << 3) | reg2 | 0x80);
   put32(imm32);
}

#ifndef __x86_64__
static INLINE void movsx_reg32_m16(int reg32, unsigned short *m16)
{
   put8(0x0F);
   put8(0xBF);
   put8((reg32 << 3) | 5);
   put32((unsigned int)(m16));
}
#endif

static INLINE void not_reg32(unsigned int reg32)
{
   put8(0xF7);
   put8(0xD0 + reg32);
}

static INLINE void mov_reg32_reg32(unsigned int reg1, unsigned int reg2)
{
   if (reg1 == reg2) return;
   put8(0x89);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void mov_reg64_reg64(unsigned int reg1, unsigned int reg2)
{
   if (reg1 == reg2) return;
   put8(0x48);
   put8(0x89);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void mov_xreg32_m32rel(unsigned int xreg32, unsigned int *m32)
{
   int offset = rel_r15_offset(m32, "mov_xreg32_m32rel");

   put8(0x41 | ((xreg32 & 8) >> 1));
   put8(0x8B);
   put8(0x87 | ((xreg32 & 7) << 3));
   put32(offset);
}

static INLINE void mov_m32rel_xreg32(unsigned int *m32, unsigned int xreg32)
{
   int offset = rel_r15_offset(m32, "mov_m32rel_xreg32");

   put8(0x41 | ((xreg32 & 8) >> 1));
   put8(0x89);
   put8(0x87 | ((xreg32 & 7) << 3));
   put32(offset);
}

static INLINE void mov_xreg64_m64rel(unsigned int xreg64, unsigned long long* m64)
{
   int offset = rel_r15_offset(m64, "mov_xreg64_m64rel");

   put8(0x49 | ((xreg64 & 8) >> 1));
   put8(0x8B);
   put8(0x87 | ((xreg64 & 7) << 3));
   put32(offset);
}

static INLINE void mov_m64rel_xreg64(unsigned long long *m64, unsigned int xreg64)
{
   int offset = rel_r15_offset(m64, "mov_m64rel_xreg64");

   put8(0x49 | ((xreg64 & 8) >> 1));
   put8(0x89);
   put8(0x87 | ((xreg64 & 7) << 3));
   put32(offset);
}

static INLINE void mov_xreg8_m8rel(int xreg8, unsigned char *m8)
{
   int offset = rel_r15_offset(m8, "mov_xreg8_m8rel");

   put8(0x41 | ((xreg8 & 8) >> 1));
   put8(0x8A);
   put8(0x87 | ((xreg8 & 7) << 3));
   put32(offset);
}

#ifndef __x86_64__
static INLINE void mov_reg32_m32(unsigned int reg32, unsigned int* m32)
{
   put8(0x8B);
   put8((reg32 << 3) | 5);
   put32((unsigned int)(m32));
}
#endif

static INLINE void adc_reg32_reg32(unsigned int reg1, unsigned int reg2)
{
   put8(0x11);
   put8(0xC0 | (reg2 << 3) | reg1);
}

static INLINE void adc_reg32_imm32(unsigned int reg32, unsigned int imm32)
{
   put8(0x81);
   put8(0xD0 + reg32);
   put32(imm32);
}

#ifndef __x86_64__
static INLINE void add_reg32_m32(unsigned int reg32, unsigned int *m32)
{
   put8(0x03);
   put8((reg32 << 3) | 5);
   put32((unsigned int)(m32));
}
#endif

static INLINE void and_eax_imm32(unsigned int imm32)
{
   put8(0x25);
   put32(imm32);
}

static INLINE void or_reg64_imm32(int reg64, unsigned int imm32)
{
   put8(0x48);
   put8(0x81);
   put8(0xC8 + reg64);
   put32(imm32);
}

#ifndef __x86_64__
static INLINE void mov_reg8_m8(int reg8, unsigned char *m8)
{
   put8(0x8A);
   put8((reg8 << 3) | 5);
   put32((unsigned int)(m8));
}
#endif

static INLINE void and_reg32_imm32(int reg32, unsigned int imm32)
{
   put8(0x81);
   put8(0xE0 + reg32);
   put32(imm32);
}

static INLINE void and_reg64_imm32(int reg64, unsigned int imm32)
{
   put8(0x48);
   put8(0x81);
   put8(0xE0 + reg64);
   put32(imm32);
}

static INLINE void and_reg64_imm8(int reg64, unsigned char imm8)
{
   put8(0x48);
   put8(0x83);
   put8(0xE0 + reg64);
   put8(imm8);
}

static INLINE void or_reg32_imm32(int reg32, unsigned int imm32)
{
   put8(0x81);
   put8(0xC8 + reg32);
   put32(imm32);
}

static INLINE void xor_reg32_imm32(int reg32, unsigned int imm32)
{
   put8(0x81);
   put8(0xF0 + reg32);
   put32(imm32);
}

static INLINE void xor_reg8_imm8(int reg8, unsigned char imm8)
{
#ifdef __x86_64__
   put8(0x40);  /* we need an REX prefix to use the uniform byte registers */
#endif
   put8(0x80);
   put8(0xF0 + reg8);
   put8(imm8);
}

static INLINE void xor_reg64_imm32(int reg64, unsigned int imm32)
{
   put8(0x48);
   put8(0x81);
   put8(0xF0 + reg64);
   put32(imm32);
}

static INLINE void not_reg64(unsigned int reg64)
{
   put8(0x48);
   put8(0xF7);
   put8(0xD0 + reg64);
}

static INLINE void neg_reg32(unsigned int reg32)
{
   put8(0xF7);
   put8(0xD8 + reg32);
}

static INLINE void neg_reg64(unsigned int reg64)
{
   put8(0x48);
   put8(0xF7);
   put8(0xD8 + reg64);
}

static INLINE void movsx_xreg32_m8rel(int xreg32, unsigned char *m8)
{
   int offset = rel_r15_offset(m8, "movsx_xreg32_m8rel");

   put8(0x41 | ((xreg32 & 8) >> 1));
   put8(0x0F);
   put8(0xBE);
   put8(0x87 | ((xreg32 & 7) << 3));
   put32(offset);
}

static INLINE void movsx_reg32_8preg64preg64(int reg1, int reg2, int reg3)
{
   put8(0x0F);
   put8(0xBE);
   put8((reg1 << 3) | 0x04);
   put8((reg2 << 3) | reg3);
}

static INLINE void movsx_reg32_16preg64preg64(int reg1, int reg2, int reg3)
{
   put8(0x0F);
   put8(0xBF);
   put8((reg1 << 3) | 0x04);
   put8((reg2 << 3) | reg3);
}

static INLINE void movsx_xreg32_m16rel(int xreg32, unsigned short *m16)
{
   int offset = rel_r15_offset(m16, "movsx_xreg32_m16rel");

   put8(0x41 | ((xreg32 & 8) >> 1));
   put8(0x0F);
   put8(0xBF);
   put8(0x87 | ((xreg32 & 7) << 3));
   put32(offset);
}

static INLINE void movsxd_reg64_reg32(int reg64, int reg32)
{
   put8(0x48);
   put8(0x63);
   put8((reg64 << 3) | reg32 | 0xC0);
}

#ifndef __x86_64__
static INLINE void fldcw_m16(unsigned short *m16)
{
   put8(0xD9);
   put8(0x2D);
   put32((unsigned int)(m16));
}
#endif

static INLINE void fldcw_m16rel(unsigned short *m16)
{
   int offset = rel_r15_offset(m16, "fldcw_m16rel");

   put8(0x41);
   put8(0xD9);
   put8(0xAF);
   put32(offset);
}

#ifdef __x86_64__
static INLINE void fld_preg64_dword(int reg64)
{
   put8(0xD9);
   put8(reg64);
}

static INLINE void fdiv_preg64_dword(int reg64)
{
   put8(0xD8);
   put8(0x30 + reg64);
}

static INLINE void fstp_preg64_dword(int reg64)
{
   put8(0xD9);
   put8(0x18 + reg64);
}
#endif

static INLINE void fchs(void)
{
   put8(0xD9);
   put8(0xE0);
}

#ifdef __x86_64__
static INLINE void fstp_preg64_qword(int reg64)
{
   put8(0xDD);
   put8(0x18 + reg64);
}

static INLINE void fadd_preg64_dword(int reg64)
{
   put8(0xD8);
   put8(reg64);
}

static INLINE void fsub_preg64_dword(int reg64)
{
   put8(0xD8);
   put8(0x20 + reg64);
}

static INLINE void fmul_preg64_dword(int reg64)
{
   put8(0xD8);
   put8(0x08 + reg64);
}

static INLINE void fistp_preg64_dword(int reg64)
{
   put8(0xDB);
   put8(0x18 + reg64);
}

static INLINE void fistp_preg64_qword(int reg64)
{
   put8(0xDF);
   put8(0x38 + reg64);
}

static INLINE void fld_preg64_qword(int reg64)
{
   put8(0xDD);
   put8(reg64);
}

static INLINE void fild_preg64_qword(int reg64)
{
   put8(0xDF);
   put8(0x28+reg64);
}

static INLINE void fild_preg64_dword(int reg64)
{
   put8(0xDB);
   put8(reg64);
}

static INLINE void fadd_preg64_qword(int reg64)
{
   put8(0xDC);
   put8(reg64);
}

static INLINE void fdiv_preg64_qword(int reg64)
{
   put8(0xDC);
   put8(0x30 + reg64);
}

static INLINE void fsub_preg64_qword(int reg64)
{
   put8(0xDC);
   put8(0x20 + reg64);
}

static INLINE void fmul_preg64_qword(int reg64)
{
   put8(0xDC);
   put8(0x08 + reg64);
}

#else

static INLINE void mov_m32_reg32(unsigned int *m32, unsigned int reg32)
{
   put8(0x89);
   put8((reg32 << 3) | 5);
   put32((unsigned int)(m32));
}

static INLINE void fdiv_preg32_dword(int reg32)
{
   put8(0xD8);
   put8(0x30 + reg32);
}

static INLINE void fstp_preg32_dword(int reg32)
{
   put8(0xD9);
   put8(0x18 + reg32);
}

static INLINE void fstp_preg32_qword(int reg32)
{
   put8(0xDD);
   put8(0x18 + reg32);
}

static INLINE void fadd_preg32_dword(int reg32)
{
   put8(0xD8);
   put8(reg32);
}

static INLINE void fsub_preg32_dword(int reg32)
{
   put8(0xD8);
   put8(0x20 + reg32);
}

static INLINE void fmul_preg32_dword(int reg32)
{
   put8(0xD8);
   put8(0x08 + reg32);
}

static INLINE void fistp_preg32_dword(int reg32)
{
   put8(0xDB);
   put8(0x18 + reg32);
}

static INLINE void fistp_preg32_qword(int reg32)
{
   put8(0xDF);
   put8(0x38 + reg32);
}

static INLINE void fld_preg32_qword(int reg32)
{
   put8(0xDD);
   put8(reg32);
}

static INLINE void fild_preg32_qword(int reg32)
{
   put8(0xDF);
   put8(0x28+reg32);
}

static INLINE void fild_preg32_dword(int reg32)
{
   put8(0xDB);
   put8(reg32);
}

static INLINE void fadd_preg32_qword(int reg32)
{
   put8(0xDC);
   put8(reg32);
}

static INLINE void fdiv_preg32_qword(int reg32)
{
   put8(0xDC);
   put8(0x30 + reg32);
}

static INLINE void fsub_preg32_qword(int reg32)
{
   put8(0xDC);
   put8(0x20 + reg32);
}

static INLINE void fmul_preg32_qword(int reg32)
{
   put8(0xDC);
   put8(0x08 + reg32);
}
#endif

static INLINE void fsqrt(void)
{
   put8(0xD9);
   put8(0xFA);
}

static INLINE void fabs_(void)
{
   put8(0xD9);
   put8(0xE1);
}

static INLINE void fcomip_fpreg(int fpreg)
{
   put8(0xDF);
   put8(0xF0 + fpreg);
}

static INLINE void fucomip_fpreg(int fpreg)
{
   put8(0xDF);
   put8(0xE8 + fpreg);
}

static INLINE void ffree_fpreg(int fpreg)
{
   put8(0xDD);
   put8(0xC0 + fpreg);
}

static INLINE void jle_rj(unsigned char saut)
{
   put8(0x7E);
   put8(saut);
}

static INLINE void jge_rj(unsigned char saut)
{
   put8(0x7D);
   put8(saut);
}

static INLINE void jg_rj(unsigned char saut)
{
   put8(0x7F);
   put8(saut);
}

static INLINE void jl_rj(unsigned char saut)
{
   put8(0x7C);
   put8(saut);
}

static INLINE void fld_preg32_dword(int reg32)
{
   put8(0xD9);
   put8(reg32);
}

static INLINE void shrd_reg32_reg32_cl(unsigned int reg1, unsigned int reg2)
{
   put8(0x0F);
   put8(0xAD);
   put8(0xC0 | (reg2 << 3) | reg1);
}

#endif /* __ASSEMBLE_H__ */

