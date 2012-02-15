/*********************************************************
 * Copyright (C) 1998 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 *********************************************************/

/*
 * vm_atomic.h --
 *
 *	Atomic power
 *
 * Note: Only partially tested on ARM processors: Works for View Open
 *       Client, which shouldn't have threads.
 *
 *       In ARM, GCC intrinsics (__sync*) compile but might not
 *       work, while MS intrinsics (_Interlocked*) do not compile,
 *       and ARM has no equivalent to the "lock" instruction prior to
 *       ARMv6; the current ARM target is ARMv5.  According to glibc
 *       documentation, ARMv5 cannot have atomic code in user space.
 *       Instead a Linux system call to kernel code referenced in
 *       entry-armv.S is used to achieve atomic functions.  See bug
 *       478054 for details.
 */

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

//#define FAKE_ATOMIC /* defined if true atomic not needed */

#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMMEXT
#define INCLUDE_ALLOW_MODULE
#define INCLUDE_ALLOW_VMMON
#define INCLUDE_ALLOW_VMNIXMOD
#define INCLUDE_ALLOW_VMKDRIVERS
#define INCLUDE_ALLOW_VMK_MODULE
#define INCLUDE_ALLOW_VMKERNEL
#define INCLUDE_ALLOW_DISTRIBUTE
#define INCLUDE_ALLOW_VMCORE
#define INCLUDE_ALLOW_VMIROM
#include "includeCheck.h"

#include "vm_basic_types.h"


/* Basic atomic type: 32 bits */
typedef struct Atomic_uint32 {
   volatile uint32 value;
} Atomic_uint32;


/* Basic atomic type: 64 bits */
typedef struct  Atomic_uint64 {
   volatile uint64 value;
} Atomic_uint64 ALIGNED(8);

#ifdef __arm__
#ifndef NOT_IMPLEMENTED
#error NOT_IMPLEMENTED undefined
#endif
#ifdef __GNUC__
EXTERN Atomic_uint32 atomicLocked64bit;
#ifndef FAKE_ATOMIC
   /*
    * Definitions for kernel function call which attempts an
    * atomic exchange, returning 0 only upon success.
    * The code actually called is put in memory by the kernel,
    * and is in fact what the kernel uses for this atomic
    * instruction.  This does not work for Linux versions
    * before 2.6 or (obviously) for non-Linux implementations.
    * For other implementations on ARMv6 and up, use
    * LDREX/SUBS/STREXEQ/LDRNE/ADDS/BNE spin-lock; for pre-ARMv6,
    * use SWP-based spin-lock.
    */
#if !defined(__linux__)
#define __kernel_cmpxchg(x, y, z) NOT_IMPLEMENTED()
#else
   typedef int (__kernel_cmpxchg_t)(uint32 oldVal,
                                    uint32 newVal,
                                    volatile uint32 *mem);
#define __kernel_cmpxchg (*(__kernel_cmpxchg_t *)0xffff0fc0)
#endif
#endif // FAKE_ATOMIC
#endif // __GNUC__
#endif // __arm__

/*
 * Prototypes for msft atomics.  These are defined & inlined by the
 * compiler so no function definition is needed.  The prototypes are
 * needed for c++.  Since amd64 compiler doesn't support inline asm we
 * have to use these.  Unfortunately, we still have to use some inline asm
 * for the 32 bit code since the and/or/xor implementations didn't show up
 * untill xp or 2k3.
 * 
 * The declarations for the intrinsic functions were taken from ntddk.h
 * in the DDK. The declarations must match otherwise the 64-bit c++
 * compiler will complain about second linkage of the intrinsic functions.
 * We define the intrinsic using the basic types corresponding to the 
 * Windows typedefs. This avoids having to include windows header files
 * to get to the windows types.
 */
#if defined(_MSC_VER) && _MSC_VER >= 1310
#ifdef __cplusplus
extern "C" {
#endif
long  _InterlockedExchange(long volatile*, long);
long  _InterlockedCompareExchange(long volatile*, long, long);
long  _InterlockedExchangeAdd(long volatile*, long);
long  _InterlockedDecrement(long volatile*);
long  _InterlockedIncrement(long volatile*);
#pragma intrinsic(_InterlockedExchange, _InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangeAdd, _InterlockedDecrement)
#pragma intrinsic(_InterlockedIncrement)

#if defined(VM_X86_64)
long     _InterlockedAnd(long volatile*, long);
__int64  _InterlockedAnd64(__int64 volatile*, __int64);
long     _InterlockedOr(long volatile*, long);
__int64  _InterlockedOr64(__int64 volatile*, __int64);
long     _InterlockedXor(long volatile*, long);
__int64  _InterlockedXor64(__int64 volatile*, __int64);
__int64  _InterlockedExchangeAdd64(__int64 volatile*, __int64);
__int64  _InterlockedIncrement64(__int64 volatile*);
__int64  _InterlockedDecrement64(__int64 volatile*);
__int64  _InterlockedExchange64(__int64 volatile*, __int64);
__int64  _InterlockedCompareExchange64(__int64 volatile*, __int64, __int64);
#if !defined(_WIN64)
#pragma intrinsic(_InterlockedAnd, _InterlockedAnd64)
#pragma intrinsic(_InterlockedOr, _InterlockedOr64)
#pragma intrinsic(_InterlockedXor, _InterlockedXor64)
#pragma intrinsic(_InterlockedExchangeAdd64, _InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64, _InterlockedExchange64)
#pragma intrinsic(_InterlockedCompareExchange64)
#endif /* !_WIN64 */
#endif /* __x86_64__ */

#ifdef __cplusplus
}
#endif
#endif /* _MSC_VER */


/* Convert a volatile int to Atomic_uint32. */
static INLINE Atomic_uint32 *
Atomic_VolatileToAtomic(volatile uint32 *var)
{
   return (Atomic_uint32 *)var;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Init, Atomic_SetFence, AtomicUseFence --
 *
 *      Determine whether an lfence intruction is executed after
 *	every locked instruction.
 *
 *	Certain AMD processes have a bug (see bug 107024) that
 *	requires an lfence after every locked instruction.
 *
 *	The global variable AtomicUseFence controls whether lfence
 *	is used (see AtomicEpilogue).
 *
 *	Atomic_SetFence sets AtomicUseFence to the given value.
 *
 *	Atomic_Init computes and sets AtomicUseFence.
 *	It does not take into account the number of processors.
 *
 *	The rationale for all this complexity is that Atomic_Init
 *	is the easy-to-use interface.  It can be called a number
 *	of times cheaply, and does not depend on other libraries.
 *	However, because the number of CPUs is difficult to compute,
 *	it does without it and always assumes there are more than one.
 *
 *	For programs that care or have special requirements,
 *	Atomic_SetFence can be called directly, in addition to Atomic_Init.
 *	It overrides the effect of Atomic_Init, and can be called
 *	before, after, or between calls to Atomic_Init.
 *
 *-----------------------------------------------------------------------------
 */

// The freebsd assembler doesn't know the lfence instruction
#if defined(__GNUC__) &&                                                \
     __GNUC__ >= 3 &&                                                   \
    (defined(__VMKERNEL__) || !defined(__FreeBSD__)) &&                 \
    (!defined(MODULE) || defined(__VMKERNEL_MODULE__)) &&               \
    !defined(__APPLE__) &&                                              \
    (defined(__i386__) || defined(__x86_64__)) /* PR136775 */
#define ATOMIC_USE_FENCE
#endif

#if defined(VMATOMIC_IMPORT_DLLDATA)
VMX86_EXTERN_DATA Bool AtomicUseFence;
#else
EXTERN Bool AtomicUseFence;
#endif

EXTERN Bool atomicFenceInitialized;

void AtomicInitFence(void);

static INLINE void
Atomic_Init(void)
{
#ifdef ATOMIC_USE_FENCE
   if (!atomicFenceInitialized) {
      AtomicInitFence();
   }
#endif
}

static INLINE void
Atomic_SetFence(Bool fenceAfterLock) /* IN: TRUE to enable lfence */
                                     /*     FALSE to disable. */
{
   AtomicUseFence = fenceAfterLock;
#if defined(__VMKERNEL__)
   extern void Atomic_SetFenceVMKAPI(Bool fenceAfterLock);
   Atomic_SetFenceVMKAPI(fenceAfterLock);
#endif
   atomicFenceInitialized = TRUE;
}


/* Conditionally execute fence after interlocked instruction. */
static INLINE void
AtomicEpilogue(void)
{
#ifdef ATOMIC_USE_FENCE
#ifdef VMM
      /* The monitor conditionally patches out the lfence when not needed.*/
      /* Construct a MonitorPatchTextEntry in the .patchtext section. */
   asm volatile ("1:\n\t"
                 "lfence\n\t"
                 "2:\n\t"
                 ".pushsection .patchtext\n\t"
#ifdef VMM32
                 ".long 1b\n\t"
                 ".long 0\n\t"
                 ".long 2b\n\t"
                 ".long 0\n\t"
#else
                 ".quad 1b\n\t"
                 ".quad 2b\n\t"
#endif
                 ".popsection\n\t" ::: "memory");
#else
   if (UNLIKELY(AtomicUseFence)) {
      asm volatile ("lfence" ::: "memory");
   }
#endif
#endif
}


/*
 * All the assembly code is tricky and written conservatively.
 * For example, to make sure gcc won't introduce copies,
 * we force the addressing mode like this:
 *
 *    "xchgl %0, (%1)"
 *    : "=r" (val)
 *    : "r" (&var->value),
 *      "0" (val)
 *    : "memory"
 *
 * - edward
 *
 * Actually - turns out that gcc never generates memory aliases (it
 * still does generate register aliases though), so we can be a bit
 * more agressive with the memory constraints. The code above can be
 * modified like this:
 *
 *    "xchgl %0, %1"
 *    : "=r" (val),
 *      "=m" (var->value),
 *    : "0" (val),
 *      "1" (var->value)
 *
 * The advantages are that gcc can use whatever addressing mode it
 * likes to access the memory value, and that we dont have to use a
 * way-too-generic "memory" clobber as there is now an explicit
 * declaration that var->value is modified.
 *
 * see also /usr/include/asm/atomic.h to convince yourself this is a
 * valid optimization.
 *
 * - walken
 */


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Read --
 *
 *      Read
 *
 * Results:
 *      The value of the atomic variable.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_Read(Atomic_uint32 const *var) // IN
{
   return var->value;
}
#define Atomic_Read32 Atomic_Read


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Write --
 *
 *      Write
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Write(Atomic_uint32 *var, // IN
             uint32 val)         // IN
{
   var->value = val;
}
#define Atomic_Write32 Atomic_Write


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_ReadWrite --
 *
 *      Read followed by write
 *
 * Results:
 *      The value of the atomic variable before the write.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_ReadWrite(Atomic_uint32 *var, // IN
                 uint32 val)         // IN
{
#ifdef FAKE_ATOMIC
   uint32 retval = var->value;
   var->value = val;
   return retval;
#elif defined(__GNUC__)
#ifdef __arm__
   register uint32 retval;
   register volatile uint32 *mem = &(var->value);
   /* XXX - ARMv5 only: for ARMv6, use LDREX/STREX/CMP/BEQ spin-lock */
   __asm__ __volatile__("swp %0, %1, [%2]"
			: "=&r,&r" (retval)
			: "r,0" (val), "r,r" (mem) : "memory");
   return retval;
#else // __arm__ (assume x86*)
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "xchgl %0, %1"
#   if VM_ASM_PLUS
      : "=r" (val),
	"+m" (var->value)
      : "0" (val)
#   else
      : "=r" (val),
	"=m" (var->value)
      : "0" (val),
	"1" (var->value)
#   endif
   );
   AtomicEpilogue();
   return val;
#endif // __arm__
#elif defined _MSC_VER
#if _MSC_VER >= 1310
   return _InterlockedExchange((long *)&var->value, (long)val);
#else
#pragma warning(push)
#pragma warning(disable : 4035)         // disable no-return warning
   {
      __asm mov eax, val
      __asm mov ebx, var
      __asm xchg [ebx]Atomic_uint32.value, eax
      // eax is the return value, this is documented to work - edward
   }
#pragma warning(pop)
#endif
#else
#error No compiler defined for Atomic_ReadWrite
#endif
}
#define Atomic_ReadWrite32 Atomic_ReadWrite


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_ReadIfEqualWrite --
 *
 *      Compare exchange: Read variable, if equal to oldVal, write newVal
 *
 * Results:
 *      The value of the atomic variable before the write.
 *
 * Side effects:
 *      The variable may be modified.
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_ReadIfEqualWrite(Atomic_uint32 *var, // IN
                        uint32 oldVal,      // IN
                        uint32 newVal)      // IN
{
#ifdef FAKE_ATOMIC
   uint32 readVal = var->value;

   if (oldVal == readVal) {
     var->value = newVal;
   }
   return oldVal;
#elif defined(__GNUC__)
#ifdef __arm__
   uint32 readVal;
   register volatile uint32 *mem = &(var->value);

   // loop until var not oldVal or var successfully replaced when var oldVal
   do {
      readVal = Atomic_Read(var);
      if (oldVal != readVal) {
         return readVal;
      }
   } while (__kernel_cmpxchg(oldVal, newVal, mem) != 0);
   return oldVal; // success
#else // __arm__ (assume x86*)
   uint32 val;

   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "lock; cmpxchgl %2, %1"
#   if VM_ASM_PLUS
      : "=a" (val),
	"+m" (var->value)
      : "r" (newVal),
	"0" (oldVal)
#   else
      : "=a" (val),
	"=m" (var->value)
      : "r" (newVal),
	"0" (oldVal)
     /*
      * "1" (var->value): results in inconsistent constraints on gcc 2.7.2.3
      * when compiling enterprise-2.2.17-14-RH7.0-update.
      * The constraint has been commented out for now. We may consider doing
      * this systematically, but we need to be sure it is the right thing to
      * do. However, it is also possible that the offending use of this asm
      * function will be removed in the near future in which case we may
      * decide to reintroduce the constraint instead. hpreg & agesen.
      */
#   endif
      : "cc"
   );
   AtomicEpilogue();
   return val;
#endif // __arm__
#elif defined _MSC_VER
#if _MSC_VER >= 1310
   return _InterlockedCompareExchange((long *)&var->value,
				      (long)newVal,
				      (long)oldVal);
#else
#pragma warning(push)
#pragma warning(disable : 4035)         // disable no-return warning
   {
      __asm mov eax, oldVal
      __asm mov ebx, var
      __asm mov ecx, newVal
      __asm lock cmpxchg [ebx]Atomic_uint32.value, ecx
      // eax is the return value, this is documented to work - edward
   }
#pragma warning(pop)
#endif
#else
#error No compiler defined for Atomic_ReadIfEqualWrite
#endif
}
#define Atomic_ReadIfEqualWrite32 Atomic_ReadIfEqualWrite


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_ReadIfEqualWrite64 --
 *
 *      Compare exchange: Read variable, if equal to oldVal, write newVal
 *
 * Results:
 *      The value of the atomic variable before the write.
 *
 * Side effects:
 *      The variable may be modified.
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint64
Atomic_ReadIfEqualWrite64(Atomic_uint64 *var, // IN
                          uint64 oldVal,      // IN
                          uint64 newVal)      // IN
{
#if defined(__GNUC__)
   uint64 val;

   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; cmpxchgq %2, %1"
      : "=a" (val),
	"+m" (var->value)
      : "r" (newVal),
	"0" (oldVal)
      : "cc"
   );
   AtomicEpilogue();
   return val;
#elif defined _MSC_VER
   return _InterlockedCompareExchange64((__int64 *)&var->value,
					(__int64)newVal,
					(__int64)oldVal);
#else
#error No compiler defined for Atomic_ReadIfEqualWrite64
#endif
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_And --
 *
 *      Atomic read, bitwise AND with a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_And(Atomic_uint32 *var, // IN
           uint32 val)         // IN
{
#ifdef FAKE_ATOMIC
   var->value &= val;
#elif defined(__GNUC__)
#ifdef __arm__
   /* same as Atomic_FetchAndAnd without return value */
   uint32 res;
   register volatile uint32 *mem = &(var->value);

   do {
     res = Atomic_Read(var);
   } while (__kernel_cmpxchg(res, res & val, mem) != 0);
#else // __arm__
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "lock; andl %1, %0"
#   if VM_ASM_PLUS
      : "+m" (var->value)
      : "ri" (val)
#   else
      : "=m" (var->value)
      : "ri" (val),
        "0" (var->value)
#   endif
      : "cc"
   );
   AtomicEpilogue();
#endif // __arm__
#elif defined _MSC_VER
#if defined(__x86_64__)
   _InterlockedAnd((long *)&var->value, (long)val);
#else
   __asm mov eax, val
   __asm mov ebx, var
   __asm lock and [ebx]Atomic_uint32.value, eax
#endif
#else
#error No compiler defined for Atomic_And
#endif
}
#define Atomic_And32 Atomic_And


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_And64 --
 *
 *      Atomic read, bitwise AND with a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_And64(Atomic_uint64 *var, // IN
             uint64 val)         // IN
{
#if defined(__GNUC__)
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; andq %1, %0"
      : "+m" (var->value)
      : "ri" (val)
      : "cc"
   );
   AtomicEpilogue();
#elif defined _MSC_VER
   _InterlockedAnd64((__int64 *)&var->value, (__int64)val);
#else
#error No compiler defined for Atomic_And64
#endif
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Or --
 *
 *      Atomic read, bitwise OR with a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Or(Atomic_uint32 *var, // IN
          uint32 val)         // IN
{
#ifdef FAKE_ATOMIC
   var->value |= val;
#elif defined(__GNUC__)
#ifdef __arm__
   /* same as Atomic_FetchAndOr without return value */
   uint32 res;
   register volatile uint32 *mem = &(var->value);

   do {
     res = Atomic_Read(var);
   } while (__kernel_cmpxchg(res, res | val, mem) != 0);
#else // __arm__
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "lock; orl %1, %0"
#   if VM_ASM_PLUS
      : "+m" (var->value)
      : "ri" (val)
#   else
      : "=m" (var->value)
      : "ri" (val),
        "0" (var->value)
#   endif
      : "cc"
   );
   AtomicEpilogue();
#endif // __arm__
#elif defined _MSC_VER
#if defined(__x86_64__)
   _InterlockedOr((long *)&var->value, (long)val);
#else
   __asm mov eax, val
   __asm mov ebx, var
   __asm lock or [ebx]Atomic_uint32.value, eax
#endif
#else
#error No compiler defined for Atomic_Or
#endif
}
#define Atomic_Or32 Atomic_Or


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Or64 --
 *
 *      Atomic read, bitwise OR with a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Or64(Atomic_uint64 *var, // IN
            uint64 val)         // IN
{
#if defined(__GNUC__)
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; orq %1, %0"
      : "+m" (var->value)
      : "ri" (val)
      : "cc"
   );
   AtomicEpilogue();
#elif defined _MSC_VER
   _InterlockedOr64((__int64 *)&var->value, (__int64)val);
#else
#error No compiler defined for Atomic_Or64
#endif
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Xor --
 *
 *      Atomic read, bitwise XOR with a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Xor(Atomic_uint32 *var, // IN
           uint32 val)         // IN
{
#ifdef FAKE_ATOMIC
   var->value ^= val;
#elif defined(__GNUC__)
#ifdef __arm__
   uint32 res;
   register volatile uint32 *mem = &(var->value);

   do {
     res = Atomic_Read(var);
   } while (__kernel_cmpxchg(res, res ^ val, mem) != 0);
#else // __arm__
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "lock; xorl %1, %0"
#   if VM_ASM_PLUS
      : "+m" (var->value)
      : "ri" (val)
#   else
      : "=m" (var->value)
      : "ri" (val),
        "0" (var->value)
#   endif
      : "cc"
   );
   AtomicEpilogue();
#endif // __arm__
#elif defined _MSC_VER
#if defined(__x86_64__)
   _InterlockedXor((long *)&var->value, (long)val);
#else
   __asm mov eax, val
   __asm mov ebx, var
   __asm lock xor [ebx]Atomic_uint32.value, eax
#endif
#else
#error No compiler defined for Atomic_Xor
#endif
}
#define Atomic_Xor32 Atomic_Xor


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Xor64 --
 *
 *      Atomic read, bitwise XOR with a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Xor64(Atomic_uint64 *var, // IN
             uint64 val)         // IN
{
#if defined(__GNUC__)
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; xorq %1, %0"
      : "+m" (var->value)
      : "ri" (val)
      : "cc"
   );
   AtomicEpilogue();
#elif defined _MSC_VER
   _InterlockedXor64((__int64 *)&var->value, (__int64)val);
#else
#error No compiler defined for Atomic_Xor64
#endif
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Add --
 *
 *      Atomic read, add a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Add(Atomic_uint32 *var, // IN
           uint32 val)         // IN
{
#ifdef FAKE_ATOMIC
   var->value += val;
#elif defined(__GNUC__)
#ifdef __arm__
   /* same as Atomic_FetchAndAddUnfenced without return value */
   uint32 res;
   register volatile uint32 *mem = &(var->value);

   do {
     res = Atomic_Read(var);
   } while (__kernel_cmpxchg(res, res + val, mem) != 0);
#else // __arm__
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "lock; addl %1, %0"
#   if VM_ASM_PLUS
      : "+m" (var->value)
      : "ri" (val)
#   else
      : "=m" (var->value)
      : "ri" (val),
        "0" (var->value)
#   endif
      : "cc"
   );
   AtomicEpilogue();
#endif // __arm__
#elif defined _MSC_VER
#if _MSC_VER >= 1310
   _InterlockedExchangeAdd((long *)&var->value, (long)val);
#else
   __asm mov eax, val
   __asm mov ebx, var
   __asm lock add [ebx]Atomic_uint32.value, eax
#endif
#else
#error No compiler defined for Atomic_Add
#endif
}
#define Atomic_Add32 Atomic_Add


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Add64 --
 *
 *      Atomic read, add a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Add64(Atomic_uint64 *var, // IN
             uint64 val)         // IN
{
#if defined(__GNUC__)
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; addq %1, %0"
      : "+m" (var->value)
      : "ri" (val)
      : "cc"
   );
   AtomicEpilogue();
#elif defined _MSC_VER
   _InterlockedExchangeAdd64((__int64 *)&var->value, (__int64)val);
#else
#error No compiler defined for Atomic_Add64
#endif
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Sub --
 *
 *      Atomic read, subtract a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Sub(Atomic_uint32 *var, // IN
           uint32 val)         // IN
{
#ifdef FAKE_ATOMIC
   var->value -= val;
#elif defined(__GNUC__)
#ifdef __arm__
   uint32 res;
   register volatile uint32 *mem = &(var->value);

   do {
     res = Atomic_Read(var);
   } while (__kernel_cmpxchg(res, res - val, mem) != 0);
#else // __arm__
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "lock; subl %1, %0"
#   if VM_ASM_PLUS
      : "+m" (var->value)
      : "ri" (val)
#   else
      : "=m" (var->value)
      : "ri" (val),
        "0" (var->value)
#   endif
      : "cc"
   );
   AtomicEpilogue();
#endif // __arm__
#elif defined _MSC_VER
#if _MSC_VER >= 1310
   _InterlockedExchangeAdd((long *)&var->value, (long)-val);
#else
   __asm mov eax, val
   __asm mov ebx, var
   __asm lock sub [ebx]Atomic_uint32.value, eax
#endif
#else
#error No compiler defined for Atomic_Sub
#endif
}
#define Atomic_Sub32 Atomic_Sub


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Sub64 --
 *
 *      Atomic read, subtract a value, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Sub64(Atomic_uint64 *var, // IN
             uint64 val)         // IN
{
#ifdef __GNUC__
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; subq %1, %0"
      : "+m" (var->value)
      : "ri" (val)
      : "cc"
   );
   AtomicEpilogue();
#elif defined _MSC_VER
   _InterlockedExchangeAdd64((__int64 *)&var->value, (__int64)-val);
#else
#error No compiler defined for Atomic_Sub64
#endif
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Inc --
 *
 *      Atomic read, increment, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Inc(Atomic_uint32 *var) // IN
{
#ifdef __GNUC__
#ifdef __arm__
   /* just use Atomic_Add */
   Atomic_Add(var, 1);
#else // __arm__
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "lock; incl %0"
#   if VM_ASM_PLUS
      : "+m" (var->value)
      :
#   else
      : "=m" (var->value)
      : "0" (var->value)
#   endif
      : "cc"
   );
   AtomicEpilogue();
#endif // __arm__
#elif defined _MSC_VER
#if _MSC_VER >= 1310
   _InterlockedIncrement((long *)&var->value);
#else
   __asm mov ebx, var
   __asm lock inc [ebx]Atomic_uint32.value
#endif
#else
#error No compiler defined for Atomic_Inc
#endif
}
#define Atomic_Inc32 Atomic_Inc


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Dec --
 *
 *      Atomic read, decrement, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Dec(Atomic_uint32 *var) // IN
{
#ifdef __GNUC__
#ifdef __arm__
   /* just use Atomic_Sub */
   Atomic_Sub(var, 1);
#else // __arm__
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
      "lock; decl %0"
#   if VM_ASM_PLUS
      : "+m" (var->value)
      :
#   else
      : "=m" (var->value)
      : "0" (var->value)
#   endif
      : "cc"
   );
   AtomicEpilogue();
#endif // __arm__
#elif defined _MSC_VER
#if _MSC_VER >= 1310
   _InterlockedDecrement((long *)&var->value);
#else
   __asm mov ebx, var
   __asm lock dec [ebx]Atomic_uint32.value
#endif
#else
#error No compiler defined for Atomic_Dec
#endif
}
#define Atomic_Dec32 Atomic_Dec


/*
 * Note that the technique below can be used to implement ReadX(), where X is
 * an arbitrary mathematical function.
 */


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_FetchAndOr --
 *
 *      Atomic read (returned), bitwise OR with a value, write.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_FetchAndOr(Atomic_uint32 *var, // IN
                  uint32 val)         // IN
{
   uint32 res;

#if defined(__arm__) && !defined(FAKE_ATOMIC)
   register volatile uint32 *mem = &(var->value);
   do {
      res = Atomic_Read(var);
   } while (__kernel_cmpxchg(res, res | val, mem) != 0);
#else
   do {
      res = Atomic_Read(var);
   } while (res != Atomic_ReadIfEqualWrite(var, res, res | val));
#endif
   return res;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_FetchAndAnd --
 *
 *      Atomic read (returned), bitwise And with a value, write.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_FetchAndAnd(Atomic_uint32 *var, // IN
                   uint32 val)         // IN
{
   uint32 res;

#if defined(__arm__) && !defined(FAKE_ATOMIC)
   register volatile uint32 *mem = &(var->value);
   do {
      res = Atomic_Read(var);
   } while (__kernel_cmpxchg(res, res & val, mem) != 0);
#else
   do {
      res = Atomic_Read(var);
   } while (res != Atomic_ReadIfEqualWrite(var, res, res & val));
#endif
   return res;
}
#define Atomic_ReadOr32 Atomic_FetchAndOr


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_ReadOr64 --
 *
 *      Atomic read (returned), bitwise OR with a value, write.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint64
Atomic_ReadOr64(Atomic_uint64 *var, // IN
                uint64 val)         // IN
{
   uint64 res;

   do {
      res = var->value;
   } while (res != Atomic_ReadIfEqualWrite64(var, res, res | val));

   return res;
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_FetchAndAddUnfenced --
 *
 *      Atomic read (returned), add a value, write.
 *
 *      If you have to implement FetchAndAdd() on an architecture other than
 *      x86 or x86-64, you might want to consider doing something similar to
 *      Atomic_FetchAndOr().
 *
 *      The "Unfenced" version of Atomic_FetchAndInc never executes
 *      "lfence" after the interlocked operation.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_FetchAndAddUnfenced(Atomic_uint32 *var, // IN
                           uint32 val)         // IN
{
#ifdef FAKE_ATOMIC
   uint32 res = var->value;
   var->value = res + val;
   return res;
#elif defined(__GNUC__)
#ifdef __arm__
   uint32 res;

   register volatile uint32 *mem = &(var->value);
   do {
      res = Atomic_Read(var);
   } while (__kernel_cmpxchg(res, res + val, mem) != 0);

   return res;
#else // __arm__
   /* Checked against the Intel manual and GCC --walken */
   __asm__ __volatile__(
#   if VM_ASM_PLUS
      "lock; xaddl %0, %1"
      : "=r" (val),
	"+m" (var->value)
      : "0" (val)
      : "cc"
#   else
      "lock; xaddl %0, (%1)"
      : "=r" (val)
      : "r" (&var->value),
	"0" (val)
      : "cc", "memory"
#   endif
   );
   return val;
#endif // __arm__
#elif defined _MSC_VER
#if _MSC_VER >= 1310
   return _InterlockedExchangeAdd((long *)&var->value, (long)val);
#else
#pragma warning(push)
#pragma warning(disable : 4035)         // disable no-return warning
   {
      __asm mov eax, val
      __asm mov ebx, var
      __asm lock xadd [ebx]Atomic_uint32.value, eax
   }
#pragma warning(pop)
#endif
#else
#error No compiler defined for Atomic_FetchAndAdd
#endif
}
#define Atomic_ReadAdd32 Atomic_FetchAndAdd


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_FetchAndAdd --
 *
 *      Atomic read (returned), add a value, write.
 *
 *      If you have to implement FetchAndAdd() on an architecture other than
 *      x86 or x86-64, you might want to consider doing something similar to
 *      Atomic_FetchAndOr().
 *
 *      Unlike "Unfenced" version, this one may execute the "lfence" after
 *      interlocked operation.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_FetchAndAdd(Atomic_uint32 *var, // IN
                   uint32 val)         // IN
{
#if defined(__GNUC__) && !defined(__arm__)
   val = Atomic_FetchAndAddUnfenced(var, val);
   AtomicEpilogue();
   return val;
#else
   return Atomic_FetchAndAddUnfenced(var, val);
#endif
}


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_ReadAdd64 --
 *
 *      Atomic read (returned), add a value, write.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint64
Atomic_ReadAdd64(Atomic_uint64 *var, // IN
                 uint64 val)         // IN
{
#if defined(__GNUC__)
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; xaddq %0, %1"
      : "=r" (val),
	"+m" (var->value)
      : "0" (val)
      : "cc"
   );
   AtomicEpilogue();
   return val;
#elif defined _MSC_VER
   return _InterlockedExchangeAdd64((__int64 *)&var->value, (__int64)val);
#else
#error No compiler defined for Atomic_ReadAdd64
#endif
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_FetchAndInc --
 *
 *      Atomic read (returned), increment, write.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_FetchAndInc(Atomic_uint32 *var) // IN
{
   return Atomic_FetchAndAdd(var, 1);
}
#define Atomic_ReadInc32 Atomic_FetchAndInc


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_ReadInc64 --
 *
 *      Atomic read (returned), increment, write.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint64
Atomic_ReadInc64(Atomic_uint64 *var) // IN
{
   return Atomic_ReadAdd64(var, 1);
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_FetchAndDec --
 *
 *      Atomic read (returned), decrement, write.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Atomic_FetchAndDec(Atomic_uint32 *var) // IN
{
   return Atomic_FetchAndAdd(var, (uint32)-1);
}
#define Atomic_ReadDec32 Atomic_FetchAndDec


#if defined(__x86_64__)
/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_ReadDec64 --
 *
 *      Atomic read (returned), decrement, write.
 *
 * Results:
 *      The value of the variable before the operation.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint64
Atomic_ReadDec64(Atomic_uint64 *var) // IN
{
   return Atomic_ReadAdd64(var, CONST64U(-1));
}
#endif


#ifdef VMKERNEL
/*
 *-----------------------------------------------------------------------------
 *
 * CMPXCHG1B --
 *
 *      Compare and exchange a single byte.
 *
 * Results:
 *      The value read from ptr.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */
static INLINE uint8
CMPXCHG1B(volatile uint8 *ptr, // IN
          uint8 oldVal,        // IN
          uint8 newVal)        // IN
{
   uint8 val;
   __asm__ __volatile__("lock; cmpxchgb %b2, %1"
                        : "=a" (val),
                          "+m" (*ptr)
                        : "r" (newVal),
                          "0" (oldVal)
                        : "cc");
   return val;
}
#endif


/*
 * Usage of this helper struct is strictly reserved to the following
 * function. --hpreg
 */
typedef struct {
   uint32 lowValue;
   uint32 highValue;
} S_uint64;


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_CMPXCHG64 --
 *
 *      Compare exchange: Read variable, if equal to oldVal, write newVal
 *
 *      XXX: Ensure that if this function is to be inlined by gcc, it is
 *      compiled with -fno-strict-aliasing. Otherwise it will break.
 *      Unfortunately we know that gcc 2.95.3 (used to build the FreeBSD 3.2
 *      Tools) does not honor -fno-strict-aliasing. As a workaround, we avoid
 *      inlining the function entirely for versions of gcc under 3.0.
 *
 * Results:
 *      TRUE if equal, FALSE if not equal
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

#ifdef __arm__
#define Atomic_CMPXCHG64(x, y, z) NOT_IMPLEMENTED()
#else // __arm__
#if defined(__GNUC__) && __GNUC__ < 3
static Bool
#else
static INLINE Bool
#endif
Atomic_CMPXCHG64(Atomic_uint64 *var,   // IN/OUT
                 uint64 const *oldVal, // IN
                 uint64 const *newVal) // IN
{
#ifdef FAKE_ATOMIC
   uint64 readVal = var->value;

   if (*oldVal == readVal) {
     var->value = *newVal;
   }
   return (*oldVal == readVal);
#elif defined(__GNUC__)
   Bool equal;

   /* Checked against the Intel manual and GCC --walken */
#ifdef VMM64
   uint64 dummy;
   __asm__ __volatile__(
      "lock; cmpxchgq %3, %0" "\n\t"
      "sete %1"
      : "+m" (*var),
	"=qm" (equal),
	"=a" (dummy)
      : "r" (*newVal),
        "2" (*oldVal)
      : "cc"
   );
#else /* 32-bit version */
   int dummy1, dummy2;
#   if defined __PIC__ && !vm_x86_64
   /*
    * Rules for __asm__ statements in __PIC__ code
    * --------------------------------------------
    *
    * The compiler uses %ebx for __PIC__ code, so an __asm__ statement cannot
    * clobber %ebx. The __asm__ statement can temporarily modify %ebx, but _for
    * each parameter that is used while %ebx is temporarily modified_:
    *
    * 1) The constraint cannot be "m", because the memory location the compiler
    *    chooses could then be relative to %ebx.
    *
    * 2) The constraint cannot be a register class which contains %ebx (such as
    *    "r" or "q"), because the register the compiler chooses could then be
    *    %ebx. (This happens when compiling the Fusion UI with gcc 4.2.1, Apple
    *    build 5577.)
    *
    * For that reason alone, the __asm__ statement should keep the regions
    * where it temporarily modifies %ebx as small as possible.
    */
#      if __GNUC__ < 3 // Part of #188541 - for RHL 6.2 etc.
   __asm__ __volatile__(
      "xchg %%ebx, %6"       "\n\t"
      "mov 4(%%ebx), %%ecx"  "\n\t"
      "mov (%%ebx), %%ebx"   "\n\t"
      "lock; cmpxchg8b (%3)" "\n\t"
      "xchg %%ebx, %6"       "\n\t"
      "sete %0"
      : "=a" (equal),
        "=d" (dummy2),
        "=D" (dummy1)
      : /*
         * See the "Rules for __asm__ statements in __PIC__ code" above: %3
         * must use a register class which does not contain %ebx.
         */
        "S" (var),
        "0" (((S_uint64 const *)oldVal)->lowValue),
        "1" (((S_uint64 const *)oldVal)->highValue),
        "D" (newVal)
      : "ecx", "cc", "memory"
   );
#      else
   __asm__ __volatile__(
      "xchgl %%ebx, %6"      "\n\t"
      "lock; cmpxchg8b (%3)" "\n\t"
      "xchgl %%ebx, %6"      "\n\t"
      "sete %0"
      :	"=qm,qm" (equal),
	"=a,a" (dummy1),
	"=d,d" (dummy2)
      : /*
         * See the "Rules for __asm__ statements in __PIC__ code" above: %3
         * must use a register class which does not contain %ebx.
         * "a"/"c"/"d" are already used, so we are left with either "S" or "D".
         */
        "S,D" (var),
        "1,1" (((S_uint64 const *)oldVal)->lowValue),
        "2,2" (((S_uint64 const *)oldVal)->highValue),
        // %6 cannot use "m": 'newVal' is read-only.
        "r,r" (((S_uint64 const *)newVal)->lowValue),
        "c,c" (((S_uint64 const *)newVal)->highValue)
      : "cc", "memory"
   );
#      endif
#   else
   __asm__ __volatile__(
      "lock; cmpxchg8b %0" "\n\t"
      "sete %1"
#      if VM_ASM_PLUS
      : "+m" (*var),
#      else
      : "=m" (*var),
#      endif
	"=qm" (equal),
	"=a" (dummy1),
	"=d" (dummy2)
      : "2" (((S_uint64 const *)oldVal)->lowValue),
        "3" (((S_uint64 const *)oldVal)->highValue),
        "b" (((S_uint64 const *)newVal)->lowValue),
        "c" (((S_uint64 const *)newVal)->highValue)
      : "cc"
   );
#   endif
#endif /* 32-bit version */
   AtomicEpilogue();
   return equal;
#elif defined _MSC_VER
#if defined(__x86_64__)
   return *oldVal == _InterlockedCompareExchange64((__int64 *)&var->value,
                                                   (__int64)*newVal,
						   (__int64)*oldVal);
#else
#pragma warning(push)
#pragma warning(disable : 4035)		// disable no-return warning
   {
      __asm mov esi, var
      __asm mov edx, oldVal
      __asm mov ecx, newVal
      __asm mov eax, [edx]S_uint64.lowValue
      __asm mov edx, [edx]S_uint64.highValue
      __asm mov ebx, [ecx]S_uint64.lowValue
      __asm mov ecx, [ecx]S_uint64.highValue
      __asm lock cmpxchg8b [esi]
      __asm sete al
      __asm movzx eax, al
      // eax is the return value, this is documented to work - edward
   }
#pragma warning(pop)
#endif
#else
#error No compiler defined for Atomic_CMPXCHG64
#endif
}
#endif // __arm__


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_CMPXCHG32 --
 *
 *      Compare exchange: Read variable, if equal to oldVal, write newVal
 *
 * Results:
 *      TRUE if equal, FALSE if not equal
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE Bool
Atomic_CMPXCHG32(Atomic_uint32 *var,   // IN/OUT
                 uint32 oldVal, // IN
                 uint32 newVal) // IN
{
#ifdef FAKE_ATOMIC
   uint32 readVal = var->value;

   if (oldVal == readVal) {
     var->value = newVal;
   }
   return (oldVal == readVal);
#elif defined(__GNUC__)
#ifdef __arm__
   register volatile uint32 *mem = &(var->value);

   return !__kernel_cmpxchg(oldVal, newVal, mem);
#else // __arm__
   Bool equal;

   uint32 dummy;
   __asm__ __volatile__(
      "lock; cmpxchgl %3, %0" "\n\t"
      "sete %1"
#   if VM_ASM_PLUS
      : "+m" (*var),
	"=qm" (equal),
	"=a" (dummy)
      : "r" (newVal),
        "2" (oldVal)
#   else
      : "=m" (*var),
	"=qm" (equal),
	"=a" (dummy)
      : /*"0" (*var), */
        "r" (newVal),
        "2" (oldVal)
#   endif
      : "cc"
   );
   AtomicEpilogue();
   return equal;
#endif // __arm__
#else // defined(__GNUC__)
   return (Atomic_ReadIfEqualWrite(var, oldVal, newVal) == oldVal);
#endif // defined(__GNUC__)
}


#ifdef __arm__

#define Atomic_Read64(x)          NOT_IMPLEMENTED()
#define Atomic_FetchAndAdd64(x,y) NOT_IMPLEMENTED()
#define Atomic_FetchAndInc64(x)   NOT_IMPLEMENTED()
#define Atomic_FetchAndDec64(x)   NOT_IMPLEMENTED()
#define Atomic_Inc64(x)           NOT_IMPLEMENTED()
#define Atomic_Dec64(x)           NOT_IMPLEMENTED()
#define Atomic_ReadWrite64(x,y)   NOT_IMPLEMENTED()
#define Atomic_Write64(x,y)       NOT_IMPLEMENTED()

#else // __arm__

/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Read64 --
 *
 *      Read and return.
 *
 * Results:
 *      The value of the atomic variable.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint64
Atomic_Read64(Atomic_uint64 const *var) // IN
{
#if defined(FAKE_ATOMIC)
   return var->value;
#elif defined(__x86_64__)
   return var->value;
#elif defined(__GNUC__) && defined(__i386__) /* GCC on x86 */
   uint64 value;
   /*
    * Since cmpxchg8b will replace the contents of EDX:EAX with the
    * value in memory if there is no match, we need only execute the
    * instruction once in order to atomically read 64 bits from
    * memory.  The only constraint is that ECX:EBX must have the same
    * value as EDX:EAX so that if the comparison succeeds.  We
    * intentionally don't tell gcc that we are using ebx and ecx as we
    * don't modify them and do not care what value they store.
    */
   __asm__ __volatile__(
      "mov %%ebx, %%eax"   "\n\t"
      "mov %%ecx, %%edx"   "\n\t"
      "lock; cmpxchg8b %1"
      : "=&A" (value)
      : "m" (*var)
      : "cc"
   );
   AtomicEpilogue();
   return value;
#elif defined _MSC_VER /* MSC (assume on x86 for now) */
#   pragma warning(push)
#   pragma warning(disable : 4035)		// disable no-return warning
   {
      __asm mov ecx, var
      __asm mov edx, ecx
      __asm mov eax, ebx
      __asm lock cmpxchg8b [ecx]
      // edx:eax is the return value; this is documented to work. --mann
   }
#   pragma warning(pop)
#else
#   error No compiler defined for Atomic_Read64
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * Atomic_FetchAndAdd64 --
 *
 *      Atomically adds a 64-bit integer to another
 *
 * Results:
 *      Returns the old value just prior to the addition
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

static INLINE uint64
Atomic_FetchAndAdd64(Atomic_uint64 *var, // IN/OUT
		     uint64 addend)      // IN
{
   uint64 oldVal;
   uint64 newVal;

   do {
      oldVal = var->value;
      newVal = oldVal + addend;
   } while (!Atomic_CMPXCHG64(var, &oldVal, &newVal));

   return oldVal;
}


/*
 *----------------------------------------------------------------------
 *
 * Atomic_FetchAndInc64 --
 *
 *      Atomically increments a 64-bit integer
 *
 * Results:
 *      Returns the old value just prior to incrementing
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

static INLINE uint64
Atomic_FetchAndInc64(Atomic_uint64 *var) // IN/OUT
{
   return Atomic_FetchAndAdd64(var, 1);
}


/*
 *----------------------------------------------------------------------
 *
 * Atomic_FetchAndDec64 --
 *
 *      Atomically decrements a 64-bit integer
 *
 * Results:
 *      Returns the old value just prior to decrementing
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

static INLINE uint64
Atomic_FetchAndDec64(Atomic_uint64 *var) // IN/OUT
{
   uint64 oldVal;
   uint64 newVal;

   do {
      oldVal = var->value;
      newVal = oldVal - 1;
   } while (!Atomic_CMPXCHG64(var, &oldVal, &newVal));

   return oldVal;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Inc64 --
 *
 *      Atomic read, increment, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Inc64(Atomic_uint64 *var) // IN
{
#if !defined(__x86_64__)
   Atomic_FetchAndInc64(var);
#elif defined(__GNUC__)
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; incq %0"
      : "+m" (var->value)
      :
      : "cc"
   );
   AtomicEpilogue();
#elif defined _MSC_VER
   _InterlockedIncrement64((__int64 *)&var->value);
#else
#error No compiler defined for Atomic_Inc64
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Dec64 --
 *
 *      Atomic read, decrement, write.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Dec64(Atomic_uint64 *var) // IN
{
#if !defined(__x86_64__)
   Atomic_FetchAndDec64(var);
#elif defined(__GNUC__)
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "lock; decq %0"
      : "+m" (var->value)
      :
      : "cc"
   );
   AtomicEpilogue();
#elif defined _MSC_VER
   _InterlockedDecrement64((__int64 *)&var->value);
#else
#error No compiler defined for Atomic_Dec64
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_ReadWrite64 --
 *
 *      Read followed by write
 *
 * Results:
 *      The value of the atomic variable before the write.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint64
Atomic_ReadWrite64(Atomic_uint64 *var, // IN
                   uint64 val)         // IN
{
#if defined(__x86_64__)
#if defined(__GNUC__)
   /* Checked against the AMD manual and GCC --hpreg */
   __asm__ __volatile__(
      "xchgq %0, %1"
      : "=r" (val),
	"+m" (var->value)
      : "0" (val)
   );
   AtomicEpilogue();
   return val;
#elif defined _MSC_VER
   return _InterlockedExchange64((__int64 *)&var->value, (__int64)val);
#else
#error No compiler defined for Atomic_ReadWrite64
#endif
#else
   uint64 oldVal;

   do {
      oldVal = var->value;
   } while (!Atomic_CMPXCHG64(var, &oldVal, &val));

   return oldVal;
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_Write64 --
 *
 *      Write
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_Write64(Atomic_uint64 *var, // IN
               uint64 val)         // IN
{
#if defined(__x86_64__)
#if defined(__GNUC__)
   /*
    * There is no move instruction for 64-bit immediate to memory, so unless
    * the immediate value fits in 32-bit (i.e. can be sign-extended), GCC
    * breaks the assignment into two movl instructions.  The code below forces
    * GCC to load the immediate value into a register first.
    */

   __asm__ __volatile__(
      "movq %1, %0"
      : "=m" (var->value)
      : "r" (val)
   );
#elif defined _MSC_VER
   /*
    * Microsoft docs guarantee "Simple reads and writes to properly aligned 
    * 64-bit variables are atomic on 64-bit Windows."
    * http://msdn.microsoft.com/en-us/library/ms684122%28VS.85%29.aspx
    */

   var->value = val;
#else
#error No compiler defined for Atomic_Write64
#endif
#else  /* defined(__x86_64__) */
   (void)Atomic_ReadWrite64(var, val);
#endif
}
#endif // __arm__


/*
 * Template code for the Atomic_<name> type and its operators.
 *
 * The cast argument is an intermedia type cast to make some
 * compilers stop complaining about casting uint32 <-> void *,
 * even though we only do it in the 32-bit case so they are always
 * the same size.  So for val of type uint32, instead of
 * (void *)val, we have (void *)(uintptr_t)val.
 * The specific problem case is the Windows ddk compiler
 * (as used by the SVGA driver).  -- edward
 */

#define MAKE_ATOMIC_TYPE(name, size, in, out, cast)                           \
   typedef Atomic_uint ## size Atomic_ ## name;                               \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   AtomicAssertOnCompile ## name(void)                                        \
   {                                                                          \
      enum { AssertOnCompileMisused =    8 * sizeof (in) == size              \
                                      && 8 * sizeof (out) == size             \
                                      && 8 * sizeof (cast) == size            \
                                         ? 1 : -1 };                          \
      typedef char AssertOnCompileFailed[AssertOnCompileMisused];             \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE out                                                          \
   Atomic_Read ## name(Atomic_ ## name const *var)                            \
   {                                                                          \
      return (out)(cast)Atomic_Read ## size(var);                             \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   Atomic_Write ## name(Atomic_ ## name *var,                                 \
                        in val)                                               \
   {                                                                          \
      Atomic_Write ## size(var, (uint ## size)(cast)val);                     \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE out                                                          \
   Atomic_ReadWrite ## name(Atomic_ ## name *var,                             \
                            in val)                                           \
   {                                                                          \
      return (out)(cast)Atomic_ReadWrite ## size(var,                         \
		(uint ## size)(cast)val);                                     \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE out                                                          \
   Atomic_ReadIfEqualWrite ## name(Atomic_ ## name *var,                      \
                                   in oldVal,                                 \
                                   in newVal)                                 \
   {                                                                          \
      return (out)(cast)Atomic_ReadIfEqualWrite ## size(var,                  \
                (uint ## size)(cast)oldVal, (uint ## size)(cast)newVal);      \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   Atomic_And ## name(Atomic_ ## name *var,                                   \
                      in val)                                                 \
   {                                                                          \
      Atomic_And ## size(var, (uint ## size)(cast)val);                       \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   Atomic_Or ## name(Atomic_ ## name *var,                                    \
                     in val)                                                  \
   {                                                                          \
      Atomic_Or ## size(var, (uint ## size)(cast)val);                        \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   Atomic_Xor ## name(Atomic_ ## name *var,                                   \
                      in val)                                                 \
   {                                                                          \
      Atomic_Xor ## size(var, (uint ## size)(cast)val);                       \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   Atomic_Add ## name(Atomic_ ## name *var,                                   \
                      in val)                                                 \
   {                                                                          \
      Atomic_Add ## size(var, (uint ## size)(cast)val);                       \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   Atomic_Sub ## name(Atomic_ ## name *var,                                   \
                      in val)                                                 \
   {                                                                          \
      Atomic_Sub ## size(var, (uint ## size)(cast)val);                       \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   Atomic_Inc ## name(Atomic_ ## name *var)                                   \
   {                                                                          \
      Atomic_Inc ## size(var);                                                \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE void                                                         \
   Atomic_Dec ## name(Atomic_ ## name *var)                                   \
   {                                                                          \
      Atomic_Dec ## size(var);                                                \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE out                                                          \
   Atomic_ReadOr ## name(Atomic_ ## name *var,                                \
                         in val)                                              \
   {                                                                          \
      return (out)(cast)Atomic_ReadOr ## size(var, (uint ## size)(cast)val);  \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE out                                                          \
   Atomic_ReadAdd ## name(Atomic_ ## name *var,                               \
                          in val)                                             \
   {                                                                          \
      return (out)(cast)Atomic_ReadAdd ## size(var, (uint ## size)(cast)val); \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE out                                                          \
   Atomic_ReadInc ## name(Atomic_ ## name *var)                               \
   {                                                                          \
      return (out)(cast)Atomic_ReadInc ## size(var);                          \
   }                                                                          \
                                                                              \
                                                                              \
   static INLINE out                                                          \
   Atomic_ReadDec ## name(Atomic_ ## name *var)                               \
   {                                                                          \
      return (out)(cast)Atomic_ReadDec ## size(var);                          \
   }


/*
 * Since we use a macro to generate these definitions, it is hard to look for
 * them. So DO NOT REMOVE THIS COMMENT and keep it up-to-date. --hpreg
 *
 * Atomic_Ptr
 * Atomic_ReadPtr --
 * Atomic_WritePtr --
 * Atomic_ReadWritePtr --
 * Atomic_ReadIfEqualWritePtr --
 * Atomic_AndPtr --
 * Atomic_OrPtr --
 * Atomic_XorPtr --
 * Atomic_AddPtr --
 * Atomic_SubPtr --
 * Atomic_IncPtr --
 * Atomic_DecPtr --
 * Atomic_ReadOrPtr --
 * Atomic_ReadAddPtr --
 * Atomic_ReadIncPtr --
 * Atomic_ReadDecPtr --
 *
 * Atomic_Int
 * Atomic_ReadInt --
 * Atomic_WriteInt --
 * Atomic_ReadWriteInt --
 * Atomic_ReadIfEqualWriteInt --
 * Atomic_AndInt --
 * Atomic_OrInt --
 * Atomic_XorInt --
 * Atomic_AddInt --
 * Atomic_SubInt --
 * Atomic_IncInt --
 * Atomic_DecInt --
 * Atomic_ReadOrInt --
 * Atomic_ReadAddInt --
 * Atomic_ReadIncInt --
 * Atomic_ReadDecInt --
 */
#if defined(__x86_64__)
MAKE_ATOMIC_TYPE(Ptr, 64, void const *, void *, uintptr_t)
#else
MAKE_ATOMIC_TYPE(Ptr, 32, void const *, void *, uintptr_t)
#endif
MAKE_ATOMIC_TYPE(Int, 32, int, int, int)


/*
 *-----------------------------------------------------------------------------
 *
 * Atomic_MFence --
 *
 *      Implements mfence in terms of a lock xor. The reason for implementing
 *      our own mfence is that not all of our supported cpus have an assembly
 *      mfence (P3, Athlon). We put it here to avoid duplicating code which is
 *      also why it is prefixed with "Atomic_".
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Cause loads and stores prior to this to be globally
 *      visible.
 *
 *-----------------------------------------------------------------------------
 */

static INLINE void
Atomic_MFence(void)
{
   Atomic_uint32 fence;
   Atomic_Xor(&fence, 0x1);
}

#endif // ifndef _ATOMIC_H_
