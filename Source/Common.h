#ifndef _COMMON_H_
#define _COMMON_H_

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <utility>

// Signed integer types.
using int_t = std::intmax_t;
using index_t = std::ptrdiff_t;

template <typename _T, size_t _N>
constexpr index_t CountOf(_T(&)[_N])
{
	return _N;
}


// Split the string into tokens using a delimiter.
template <typename _CharT>
std::vector<std::basic_string<_CharT>> SplitString(const std::basic_string<_CharT>& str, _CharT delimiter = _CharT(' '))
{
	std::vector<std::basic_string<_CharT>> tokenArr;
	std::basic_stringstream<_CharT> sstr(str);
	std::string token;
	
	while (std::getline(sstr, token, delimiter))
		tokenArr.push_back(std::move(token));

	return tokenArr;
}

#endif // !_COMMON_H_
