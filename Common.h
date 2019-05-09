
#ifndef _BASE_LIB_INTERFACE_COMMON_H_
#define _BASE_LIB_INTERFACE_COMMON_H_

#include <type_traits>

#define BUILD_DLL	0

#if defined(_MSC_VER) && BUILD_DLL == 1
	#ifdef BASELIB_EXPORTS
		#define BASELIB_API __declspec(dllexport)
	#else
		#define BASELIB_API __declspec(dllimport)
	#endif
#else
	#define BASELIB_API
#endif



namespace BaseLib
{

namespace Literals
{

constexpr size_t operator "" _KB(unsigned long long int value)
{
	return (size_t)value * 1024;
}

constexpr size_t operator "" _MB(unsigned long long int value)
{
	return (size_t)value * 1024 * 1024;
}

} // namespace Literals

// Number of elements in array.
template <class _Type, size_t _N>
constexpr size_t CountOf(_Type (&)[_N])
{
	return _N;
}

// Number of characters in a literal string.
template <typename _CharT>
constexpr size_t StrLen(const _CharT* str)
{
	return *str ? 1 + StrLen(str + 1) : 0;
}

template <class _Type>
inline _Type RoundUp(_Type n, size_t a)
{
	static_assert(std::is_integral<_Type>::value || std::is_pointer<_Type>::value, "Must be integral or pointer type.");
	return _Type( (size_t(n) + (a - 1)) & ~(a - 1) );
}

inline bool IsPow2(size_t num)
{
	return !((num - 1) & num);
}

// Returns next power of two, or the same number if it's already power of two.
template <class _Type>
inline _Type RoundToNextPow2(_Type n)
{
	static_assert(std::is_integral<_Type>::value, "Must be integral type.");

	_Type result = 1;
	while(result < n)
		result <<= 1;
	return result;
}

// Set single bit in a bit array at position nbit. Bits are stored in LSB order.
inline void SetBitLSB(unsigned char* bit_array, int nbit, bool val)
{
	unsigned char& b = bit_array[nbit >> 3];
	unsigned char mask = 1 << (nbit & 7);
	b = val ? (b | mask) : (b & ~mask);
}

// Set single bit in a bit array at position nbit. Bits are stored in MSB order.
inline void SetBitMSB(unsigned char* bit_array, int nbit, bool val)
{
	unsigned char& b = bit_array[nbit >> 3];
	unsigned char mask = 1 << (7 - (nbit & 7));
	b = val ? (b | mask) : (b & ~mask);
}

// Get single bit from a bit array at position nbit. Bits are stored in LSB order.
inline bool GetBitLSB(const unsigned char* bit_array, int nbit)
{
	return (bit_array[nbit >> 3] & (1 << (nbit & 7))) != 0;
}

// Get single bit from a bit array at position nbit. Bits are stored in MSB order.
inline bool GetBitMSB(const unsigned char* bit_array, int nbit)
{
	return (bit_array[nbit >> 3] & (1 << (7 - (nbit & 7)))) != 0;
}

} // namespace BaseLib

#endif // _BASE_LIB_INTERFACE_COMMON_H_
