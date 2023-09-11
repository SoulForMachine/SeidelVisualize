
/* ----------------------------------------
	File: mathutil.h
	Purpose: 3d math library helpers
	Author: Milan D.
	Date: 01.05.2005
   ---------------------------------------- */

#ifndef _MATH3D_MATHUTIL_H_
#define _MATH3D_MATHUTIL_H_

#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <limits>
#include <type_traits>


namespace math3d
{
// some constants
constexpr float PI = float(3.141592653589793);
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = 0.5f * PI;

template <class _ST>
constexpr auto abs(_ST x)
	-> std::enable_if_t<std::is_floating_point_v<_ST>, _ST>
{
	return (x >= 0) ? x : ((x < 0) ? -x : x);
}
template <class _ST>
constexpr auto abs(_ST x)
	-> std::enable_if_t<std::is_integral_v<_ST>, _ST>
{
	return (x >= 0) ? x : -x;
}

/*
	functions for comparing floating types
*/
template <class _ST>
constexpr auto fcmp_eq(_ST t1, _ST t2, _ST err = std::numeric_limits<_ST>::epsilon())
	-> std::enable_if_t<std::is_floating_point_v<_ST>, bool>
{
	return abs(t1 - t2) <= err;
}
template <class _ST>
constexpr auto fcmp_ne(_ST t1, _ST t2, _ST err = std::numeric_limits<_ST>::epsilon())
	-> std::enable_if_t<std::is_floating_point_v<_ST>, bool>
{
	return abs(t1 - t2) > err;
}
template <class _ST>
constexpr auto fcmp_lt(_ST t1, _ST t2, _ST err = std::numeric_limits<_ST>::epsilon())
	-> std::enable_if_t<std::is_floating_point_v<_ST>, bool>
{
	return (t1 - t2) < err;
}
template <class _ST>
constexpr auto fcmp_le(_ST t1, _ST t2, _ST err = std::numeric_limits<_ST>::epsilon())
	-> std::enable_if_t<std::is_floating_point_v<_ST>, bool>
{
	return (t1 - t2) <= err;
}
template <class _ST>
constexpr auto fcmp_gt(_ST t1, _ST t2, _ST err = std::numeric_limits<_ST>::epsilon())
	-> std::enable_if_t<std::is_floating_point_v<_ST>, bool>
{
	return (t1 - t2) > err;
}
template <class _ST>
constexpr auto fcmp_ge(_ST t1, _ST t2, _ST err = std::numeric_limits<_ST>::epsilon())
	-> std::enable_if_t<std::is_floating_point_v<_ST>, bool>
{
	return (t1 - t2) >= err;
}

/*
	convert an angle from radians to degrees
	and vice versa
*/
template <class _ST>
constexpr _ST rad2deg(_ST rad)
{
	return rad * _ST(180 / PI);
}
template <class _ST>
constexpr _ST deg2rad(_ST deg)
{
	return deg * _ST(PI / 180);
}

/*
	trigonometric functions that work with
	angles in degrees rather than radians
*/
template <class _ST>
inline _ST deg_sin(_ST angle)
{
	return std::sin(deg2rad(angle));
}
template <class _ST>
inline _ST deg_cos(_ST angle)
{
	return std::cos(deg2rad(angle));
}
template <class _ST>
inline _ST deg_tan(_ST angle)
{
	return std::tan(deg2rad(angle));
}
template <class _ST>
inline _ST deg_asin(_ST t)
{
	return rad2deg(std::asin(t));
}
template <class _ST>
inline _ST deg_acos(_ST t)
{
	return rad2deg(std::acos(t));
}
template <class _ST>
inline _ST deg_atan(_ST t)
{
	return rad2deg(std::atan(t));
}
template <class _ST>
inline _ST deg_atan2(_ST y, _ST x)
{
	return rad2deg(std::atan2(y, x));
}

/*
	set t to be in range low <= t <= high;
	assumes that low < high 
*/
template <class _ST>
constexpr _ST clamp(_ST t, _ST low, _ST high)
{
	if (t < low)
		return low;
	else if (t > high)
		return high;
	else
		return t;
}

template <class _ST>
inline void wrap(_ST& t, _ST low, _ST high)
{
	if (t < low)
		t = high - std::fmod(low - t, high - low);
	else if (t > high)
		t = low + std::fmod(t, high - low);
}

template <class _ST>
inline _ST frand()
{
	return _ST(std::rand()) / _ST(RAND_MAX);
}

template <class _T, class _ST>
constexpr _T lerp(const _T& start, const _T& end, _ST t)
{
	return (1.0f - t) * start + t * end;
}

/*
	Return fractional part of val
*/
template <class _ST>
constexpr auto frac(_ST val)
	-> std::enable_if_t<std::is_floating_point_v<_ST>, _ST>
{
	return val - (float)(int)val;
}

// Returns -1 if val is negative and 1 if it is positive.
template <class _ST>
constexpr _ST sign(_ST val)
{
	return (val < _ST(0)) ? _ST(-1) : _ST(1);
}

/*
	Round to nearest integer
*/
template <class _ST>
constexpr auto round(_ST val)
	-> std::enable_if_t<std::is_floating_point_v<_ST>, _ST>
{
	return (_ST)(int)(val + sign(val) * _ST(0.5));
}

// Round val to the greater number which is a multiple of mult.
template <class _ST>
constexpr auto next_greater_multiple(_ST val, _ST mult)
	-> std::enable_if_t<std::is_floating_point_v<_ST>, _ST>
{
	if (mult == 0.0f)
		return val;

	_ST n = val / mult;
	int i = static_cast<int>(n);
	_ST f = frac(n);
	if (val < 0)
		return _ST(i + ((f == _ST(0)) ? 1 : 0)) * mult;
	else
		return _ST(i + 1) * mult;
}

// Round val to the greater number which is a multiple of mult,
// or just return val if it is a multiple of mult.
template <class _ST>
constexpr auto next_greater_eq_multiple(_ST val, _ST mult)
	-> std::enable_if_t<std::is_floating_point_v<_ST>, _ST>
{
	if (mult == 0.0f)
		return val;

	_ST n = val / mult;
	int i = static_cast<int>(n);
	_ST f = frac(n);
	if (val < 0)
		return _ST(i) * mult;
	else
		return _ST(i + ((f != _ST(0)) ? 1 : 0)) * mult;
}

// Round val to the nearest number which is a multiple of mult.
template <class _ST>
constexpr auto nearest_multiple(_ST val, _ST mult)
	-> std::enable_if_t<std::is_floating_point_v<_ST>, _ST>
{
	if (math3d::fcmp_eq(mult, 0.0f))
		return val;

	_ST n = val / mult;
	int i = static_cast<int>(n);
	_ST f = frac(n);

	return _ST(i) * mult + round(f) * mult;
}

// Round val to the nearest half integer.
// E.g.:
// -1.0 -> -0.5
// -0.9 -> -0.5
// -0.1 -> -0.5
// 0.0 -> 0.5
// 0.1 -> 0.5
// 0.9 -> 0.5
// 1.0f -> 1.5
template <class _ST>
constexpr auto nearest_half(_ST val)
	-> std::enable_if_t<std::is_floating_point_v<_ST>, _ST>
{
	int ival = (int)val;
	return ((val < ival) ? ival - 1 : ival) + 0.5f;
}

}  // namespace math3d

#endif  // _MATH3D_MATHUTIL_H_
