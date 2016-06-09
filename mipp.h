/*
Copyright (c) 1998, Regents of the University of California
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of the University of California, Berkeley nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * mipp.h
 *
 *  Created on: 5 oct. 2014
 *      Author: Adrien Cassagne
 */

#ifndef MY_INTRINSICS_PLUS_PLUS_H_
#define MY_INTRINSICS_PLUS_PLUS_H_

#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#include <arm_neon.h>
#elif defined(__SSE__) || defined(__AVX__) || defined(__MIC__) || defined(__KNCNI__) || defined(__AVX512__) || defined(__AVX512F__)
// header for special functions: log, exp, sin, cos
#if !defined(__INTEL_COMPILER) && !defined(__ICL) && !defined(__ICC)
#if defined(__AVX__)
#include "math/avx_mathfun.h"
#elif defined(__SSE__)
#include "math/sse_mathfun.h"
#endif
#endif
#include <immintrin.h>
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __SSE3__
#include <pmmintrin.h>
#endif
#ifdef __SSSE3__
#include <tmmintrin.h>
#endif
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif
#endif

#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cassert>
#include <string>
#include <vector>
#include <map>

namespace mipp // My Intrinsics Plus Plus => mipp
{
// ------------------------------------------------------------------------------------------ myIntrinsics vector sizes
// --------------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------- ARM NEON-128
#if defined(__ARM_NEON__) || defined(__ARM_NEON)
	#define REQUIRED_ALIGNMENT 16
	constexpr int RequiredAlignment = REQUIRED_ALIGNMENT;
	constexpr int RegisterSizeBit = 128;

	using reg = float32x4_t;

	const std::string IntructionsType = "ARM NEONv1-128";

// -------------------------------------------------------------------------------------------------------- X86 AVX-512
#elif defined(__MIC__) || defined(__KNCNI__) || defined(__AVX512__) || defined(__AVX512F__)
	#define REQUIRED_ALIGNMENT 64
	constexpr int RequiredAlignment = REQUIRED_ALIGNMENT;
	constexpr int RegisterSizeBit = 512;

	using reg = __m512;

	const std::string IntructionsType = "x86 AVX-512";

// -------------------------------------------------------------------------------------------------------- X86 AVX-256
#elif defined(__AVX__)
	#define REQUIRED_ALIGNMENT 32
	constexpr int RequiredAlignment = REQUIRED_ALIGNMENT;
	constexpr int RegisterSizeBit = 256;

	using reg = __m256;

#ifdef __AVX2__
	const std::string IntructionsType = "x86 AVX2-256";
#else
	const std::string IntructionsType = "x86 AVX1-256";
#endif

// -------------------------------------------------------------------------------------------------------- X86 SSE-128
#elif defined(__SSE__)
	#define REQUIRED_ALIGNMENT 16
	constexpr int RequiredAlignment = REQUIRED_ALIGNMENT;
	constexpr int RegisterSizeBit = 128;

	using reg = __m128;

#ifdef __SSE4_2__
	const std::string IntructionsType = "x86 SSE4.2-128";
#elif defined(__SSE4_1__)
	const std::string IntructionsType = "x86 SSE4.1-128";
#elif defined(__SSSE3__)
	const std::string IntructionsType = "x86 SSSE3-128";
#elif defined(__SSE3__)
	const std::string IntructionsType = "x86 SSE3-128";
#elif defined(__SSE2__)
	const std::string IntructionsType = "x86 SSE2-128";
#else
	const std::string IntructionsType = "x86 SSE1-128";
#endif

// ---------------------------------------------------------------------------------------------------------- UNDEFINED
#else
	#define UNKNOWN_INSTRUCTION_TYPE
	#define REQUIRED_ALIGNMENT 1
	constexpr int RequiredAlignment = REQUIRED_ALIGNMENT;
	constexpr int RegisterSizeBit = 0;

	using reg = int;

	const std::string IntructionsType = "UNDEFINED";

#endif

typedef struct regx2 { reg val[2]; } regx2;

template <typename T>
constexpr int nElmtsPerRegister()
{
#ifdef UNKNOWN_INSTRUCTION_TYPE
	return 1;
#else
	return RegisterSizeBit / (8 * sizeof(T));
#endif
}

template <typename T>
constexpr int nElReg() { return mipp::nElmtsPerRegister<T>(); }

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------- memory allocator
template <typename T>
T* malloc(int nData)
{
	T* ptr = nullptr;

#if defined(__SSE2__) || defined(__AVX__) || defined(__MIC__) || defined(__KNCNI__) || defined(__AVX512__) || defined(__AVX512F__)
	ptr = (T*)_mm_malloc(nData * sizeof(T), mipp::RequiredAlignment);
#else
	ptr = new T[nData];
#endif

	return ptr;
}

template <typename T>
void free(T* ptr)
{
#if defined(__SSE2__) || defined(__AVX__) || defined(__MIC__) || defined(__KNCNI__) || defined(__AVX512__) || defined(__AVX512F__)
	_mm_free(ptr);
#else
	delete[] ptr;
#endif
}

template <class T>
struct AlignedAllocator
{
	typedef T value_type;
	AlignedAllocator() { }
	template <class C> AlignedAllocator(const AlignedAllocator<C>& other) { }
	T* allocate(std::size_t n) { return mipp::malloc<T>((int)n); }
	void deallocate(T* p, std::size_t n) { mipp::free<T>(p); }
};

// returns true if and only if storage allocated from ma1 can be deallocated from ma2, and vice versa.
// always returns true for stateless allocators.
template <class C1, class C2>
bool operator==(const AlignedAllocator<C1>& ma1, const AlignedAllocator<C2>& ma2) { return true; }

template <class C1, class C2>
bool operator!=(const AlignedAllocator<C1>& ma1, const AlignedAllocator<C2>& ma2) { return !(ma1 == ma2); }

// override vector type
template<class T> using vector = std::vector<T, AlignedAllocator<T>>;

// --------------------------------------------------------------------------------------------------- memory allocator
// --------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------- myIntrinsics prototypes
// --------------------------------------------------------------------------------------------------------------------

template <typename T>
static void errorMessage(std::string instr)
{
	// define type names
	std::unordered_map<std::type_index,std::string> type_names;
	type_names[typeid(char)]                   = "char";
	type_names[typeid(unsigned char)]          = "unsigned char";
	type_names[typeid(signed char)]            = "signed char";
	type_names[typeid(short)]                  = "short";
	type_names[typeid(short int)]              = "short int";
	type_names[typeid(unsigned short)]         = "unsigned short";
	type_names[typeid(unsigned short int)]     = "unsigned short int";
	type_names[typeid(signed short)]           = "signed short";
	type_names[typeid(signed short int)]       = "signed short int";
	type_names[typeid(int)]                    = "int";
	type_names[typeid(unsigned)]               = "unsigned";
	type_names[typeid(unsigned int)]           = "unsigned int";
	type_names[typeid(signed int)]             = "signed int";
	type_names[typeid(long)]                   = "long";
	type_names[typeid(long int)]               = "long int";
	type_names[typeid(unsigned long)]          = "unsigned long";
	type_names[typeid(unsigned long int)]      = "unsigned long int";
	type_names[typeid(signed long)]            = "signed long";
	type_names[typeid(signed long int)]        = "signed long int";
	type_names[typeid(long long)]              = "long long";
	type_names[typeid(long long int)]          = "long long int";
	type_names[typeid(unsigned long long)]     = "unsigned long long";
	type_names[typeid(unsigned long long int)] = "unsigned long long int";
	type_names[typeid(signed long long)]       = "signed long long";
	type_names[typeid(signed long long int)]   = "signed long long int";
	type_names[typeid(float)]                  = "float";
	type_names[typeid(double)]                 = "double";

	if (RegisterSizeBit == 0)
		std::cerr << "Undefined type of instructions, try to add -mfpu=neon, -msse4.2, -mavx, -march=native... "
		          << "at the compile time." << std::endl;
	std::cerr << "mipp::" << instr << "<" << type_names[typeid(T)] << "> (" << IntructionsType << ") is undefined! "
	          << "Program halting..." << std::endl;
}

template <typename T1, typename T2>
static void errorMessage(std::string instr)
{
	// define type names
	std::unordered_map<std::type_index,std::string> type_names;
	type_names[typeid(char)]                   = "char";
	type_names[typeid(unsigned char)]          = "unsigned char";
	type_names[typeid(signed char)]            = "signed char";
	type_names[typeid(short)]                  = "short";
	type_names[typeid(short int)]              = "short int";
	type_names[typeid(unsigned short)]         = "unsigned short";
	type_names[typeid(unsigned short int)]     = "unsigned short int";
	type_names[typeid(signed short)]           = "signed short";
	type_names[typeid(signed short int)]       = "signed short int";
	type_names[typeid(int)]                    = "int";
	type_names[typeid(unsigned)]               = "unsigned";
	type_names[typeid(unsigned int)]           = "unsigned int";
	type_names[typeid(signed int)]             = "signed int";
	type_names[typeid(long)]                   = "long";
	type_names[typeid(long int)]               = "long int";
	type_names[typeid(unsigned long)]          = "unsigned long";
	type_names[typeid(unsigned long int)]      = "unsigned long int";
	type_names[typeid(signed long)]            = "signed long";
	type_names[typeid(signed long int)]        = "signed long int";
	type_names[typeid(long long)]              = "long long";
	type_names[typeid(long long int)]          = "long long int";
	type_names[typeid(unsigned long long)]     = "unsigned long long";
	type_names[typeid(unsigned long long int)] = "unsigned long long int";
	type_names[typeid(signed long long)]       = "signed long long";
	type_names[typeid(signed long long int)]   = "signed long long int";
	type_names[typeid(float)]                  = "float";
	type_names[typeid(double)]                 = "double";

	if (RegisterSizeBit == 0)
		std::cerr << "Undefined type of instructions, try to add -mfpu=neon, -msse4.2, -mavx, -march=native... "
		          << "at the compile time." << std::endl;
	std::cerr << "mipp::" << instr << "<" << type_names[typeid(T1)] << "," << type_names[typeid(T2)] << "> (" 
	          << IntructionsType << ") is undefined! Program halting..." << std::endl;
}

template <typename T> inline reg   load         (const T*)                        { errorMessage<T>("load");          exit(-1); }
template <typename T> inline reg   loadu        (const T*)                        { errorMessage<T>("loadu");         exit(-1); }
template <typename T> inline void  store        (T*, const reg)                   { errorMessage<T>("store");         exit(-1); }
template <typename T> inline void  storeu       (T*, const reg)                   { errorMessage<T>("storeu");        exit(-1); }
template <typename T> inline reg   set          (const T[nElReg<T>()])            { errorMessage<T>("set");           exit(-1); }
template <typename T> inline reg   set1         (const T)                         { errorMessage<T>("set1");          exit(-1); }
template <typename T> inline reg   set0         ()                                { errorMessage<T>("set0");          exit(-1); }
template <typename T> inline reg   cmask        (const int[nElReg<T>()])          { errorMessage<T>("cmask");         exit(-1); }
template <typename T> inline reg   cmask2       (const int[nElReg<T>()/2])        { errorMessage<T>("cmask2");        exit(-1); }
template <typename T> inline reg   shuff        (const reg, const reg)            { errorMessage<T>("shuff");         exit(-1); }
template <typename T> inline reg   shuff2       (const reg, const reg)            { errorMessage<T>("shuff2");        exit(-1); }
template <typename T> inline reg   interleavelo (const reg, const reg)            { errorMessage<T>("interleavelo");  exit(-1); }
template <typename T> inline reg   interleavehi (const reg, const reg)            { errorMessage<T>("interleavehi");  exit(-1); }
template <typename T> inline reg   interleavelo2(const reg, const reg)            { errorMessage<T>("interleavelo2"); exit(-1); }
template <typename T> inline reg   interleavehi2(const reg, const reg)            { errorMessage<T>("interleavehi2"); exit(-1); }
template <typename T> inline regx2 interleave   (const reg, const reg)            { errorMessage<T>("interleave");    exit(-1); }
template <typename T> inline regx2 interleave2  (const reg, const reg)            { errorMessage<T>("interleave2");   exit(-1); }
template <typename T> inline reg   interleave   (const reg)                       { errorMessage<T>("interleave");    exit(-1); }
template <typename T> inline regx2 interleavex2 (const reg, const reg)            { errorMessage<T>("interleavex2");  exit(-1); }
template <typename T> inline reg   interleavex4 (const reg)                       { errorMessage<T>("interleavex4");  exit(-1); }
template <typename T> inline reg   interleavex16(const reg)                       { errorMessage<T>("interleavex16"); exit(-1); }
template <typename T> inline void  transpose    (      reg[nElReg<T>()])          { errorMessage<T>("transpose");     exit(-1); }
template <typename T> inline void  transpose8x8 (      reg[8])                    { errorMessage<T>("transpose8x8");  exit(-1); }
template <typename T> inline void  transpose2   (      reg[nElReg<T>()/2])        { errorMessage<T>("transpose2");    exit(-1); }
template <typename T> inline void  transpose28x8(      reg[8])                    { errorMessage<T>("transpose28x8"); exit(-1); }
template <typename T> inline reg   andb         (const reg, const reg)            { errorMessage<T>("andb");          exit(-1); }
template <typename T> inline reg   andnb        (const reg, const reg)            { errorMessage<T>("andnb");         exit(-1); }
template <typename T> inline reg   orb          (const reg, const reg)            { errorMessage<T>("orb");           exit(-1); }
template <typename T> inline reg   xorb         (const reg, const reg)            { errorMessage<T>("xorb");          exit(-1); }
template <typename T> inline reg   lshift       (const reg, const int)            { errorMessage<T>("lshift");        exit(-1); }
template <typename T> inline reg   rshift       (const reg, const int)            { errorMessage<T>("rshift");        exit(-1); }
template <typename T> inline reg   cmpeq        (const reg, const reg)            { errorMessage<T>("cmpeq");         exit(-1); }
template <typename T> inline reg   cmpneq       (const reg, const reg)            { errorMessage<T>("cmpneq");        exit(-1); }
template <typename T> inline reg   cmple        (const reg, const reg)            { errorMessage<T>("cmple");         exit(-1); }
template <typename T> inline reg   cmplt        (const reg, const reg)            { errorMessage<T>("cmplt");         exit(-1); }
template <typename T> inline reg   add          (const reg, const reg)            { errorMessage<T>("add");           exit(-1); }
template <typename T> inline reg   sub          (const reg, const reg)            { errorMessage<T>("sub");           exit(-1); }
template <typename T> inline reg   mul          (const reg, const reg)            { errorMessage<T>("mul");           exit(-1); }
template <typename T> inline reg   div          (const reg, const reg)            { errorMessage<T>("div");           exit(-1); }
template <typename T> inline reg   min          (const reg, const reg)            { errorMessage<T>("min");           exit(-1); }
template <typename T> inline reg   max          (const reg, const reg)            { errorMessage<T>("max");           exit(-1); }
template <typename T> inline reg   sign         (const reg)                       { errorMessage<T>("sign");          exit(-1); }
template <typename T> inline reg   sign         (const reg, const reg)            { errorMessage<T>("sign");          exit(-1); }
template <typename T> inline reg   neg          (const reg, const reg)            { errorMessage<T>("neg");           exit(-1); }
template <typename T> inline reg   abs          (const reg)                       { errorMessage<T>("abs");           exit(-1); }
template <typename T> inline reg   sqrt         (const reg)                       { errorMessage<T>("sqrt");          exit(-1); }
template <typename T> inline reg   rsqrt        (const reg)                       { errorMessage<T>("rsqrt");         exit(-1); }
template <typename T> inline reg   exp          (const reg)                       { errorMessage<T>("exp");           exit(-1); }
template <typename T> inline reg   fmadd        (const reg, const reg, const reg) { errorMessage<T>("fmadd");         exit(-1); }
template <typename T> inline reg   fnmadd       (const reg, const reg, const reg) { errorMessage<T>("fnmadd");        exit(-1); }
template <typename T> inline reg   fmsub        (const reg, const reg, const reg) { errorMessage<T>("fmsub");         exit(-1); }
template <typename T> inline reg   rot          (const reg)                       { errorMessage<T>("rot");           exit(-1); }
template <typename T> inline reg   rotr         (const reg)                       { errorMessage<T>("rotr");          exit(-1); }
template <typename T> inline reg   div2         (const reg)                       { errorMessage<T>("div2");          exit(-1); }
template <typename T> inline reg   div4         (const reg)                       { errorMessage<T>("div4");          exit(-1); }
template <typename T> inline reg   sat          (const reg, T, T)                 { errorMessage<T>("sat");           exit(-1); }
template <typename T> inline reg   round        (const reg)                       { errorMessage<T>("round");         exit(-1); }

template <typename T1, typename T2> 
inline reg cvt(const reg) {
	errorMessage<T1,T2>("cvt");  
	exit(-1); 
}

template <typename T1, typename T2>
inline reg pack(const reg, const reg) {
	errorMessage<T1,T2>("pack");  
	exit(-1); 
}

template <typename T>
using red_op = reg (*)(const reg, const reg);

template <typename T>
using ld_op = reg (*)(const T*);

template <typename T, red_op<T> OP>
struct _Reduction
{
	static reg apply(const reg) {
		errorMessage<T>("_Reduction::apply");
		exit(-1); 
	}
};

template <typename T, red_op<T> OP>
struct Reduction
{
	static reg apply(const reg r) 
	{
		return _Reduction<T,OP>::apply(r);
	}

	template <ld_op<T> LD = mipp::load<T>>
	static T apply(const mipp::vector<T> &data) 
	{
		return Reduction<T,OP>::template apply<LD>(data.data(), data.size());
	}

	template <ld_op<T> LD = mipp::loadu<T>>
	static T apply(const std::vector<T> &data) 
	{
		return Reduction<T,OP>::template apply<LD>(data.data(), data.size());
	}

	template <ld_op<T> LD = mipp::loadu<T>>
	static T apply(const T *data, const int dataSize) 
	{
		assert(dataSize > 0);
		assert(dataSize % mipp::nElReg<T>() == 0);

		auto rRed = LD(&data[0]);
		for (auto i = mipp::nElReg<T>(); i < dataSize; i += mipp::nElReg<T>())
			rRed = OP(rRed, LD(&data[i]));
		rRed = Reduction<T,OP>::apply(rRed);

		T tRed[mipp::nElReg<T>()];
		mipp::store<T>(tRed, rRed);

		return tRed[0];
	}
};

// --------------------------------------------------------------------------------------- myIntrinsics implementations
// --------------------------------------------------------------------------------------------------------------------

template <typename T>
void dump(const mipp::reg r, std::ostream &stream = std::cout, const int elmtWidth = 6)
{
	T dumpArray[mipp::nElReg<T>()];
	mipp::storeu<T>(dumpArray, r);

	stream << "[";
	if (typeid(char) == typeid(T) || typeid(signed char) == typeid(T))
		for (auto i = 0; i < mipp::nElReg<T>(); i++)
			stream << std::setw(elmtWidth) << (int)dumpArray[i] << ((i < mipp::nElReg<T>()-1) ? ", " : "");
	else if (typeid(unsigned char) == typeid(T))
		for (auto i = 0; i < mipp::nElReg<T>(); i++)
			stream << std::setw(elmtWidth) << (unsigned)dumpArray[i] << ((i < mipp::nElReg<T>()-1) ? ", " : "");
	else
		for (auto i = 0; i < mipp::nElReg<T>(); i++)
			stream << std::setw(elmtWidth) << dumpArray[i] << ((i < mipp::nElReg<T>()-1) ? ", " : "");

	stream << "]";
}

// ------------------------------------------------------------------------------------------------------- ARM NEON-128
// --------------------------------------------------------------------------------------------------------------------
#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#include "mipp_impl_NEON.hxx"
// -------------------------------------------------------------------------------------------------------- X86 AVX-512
// --------------------------------------------------------------------------------------------------------------------
#elif defined(__MIC__) || defined(__KNCNI__) || defined(__AVX512__) || defined(__AVX512F__)
#include "mipp_impl_AVX512.hxx"
// -------------------------------------------------------------------------------------------------------- X86 AVX-256
// --------------------------------------------------------------------------------------------------------------------
#elif defined(__AVX__)
#include "mipp_impl_AVX.hxx"
// -------------------------------------------------------------------------------------------------------- X86 SSE-128
// --------------------------------------------------------------------------------------------------------------------
#elif defined(__SSE__)
#include "mipp_impl_SSE.hxx"
#endif
}

#endif /* MY_INTRINSICS_PLUS_PLUS_H_ */
