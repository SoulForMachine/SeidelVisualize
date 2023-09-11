
#ifndef _MATH3D_GEOMETRY_H_
#define _MATH3D_GEOMETRY_H_

#include "vec4.h"
#include "mathutil.h"


namespace math3d
{

template<class _ST>
constexpr vec4<_ST> plane_from_triangle(const vec3<_ST> triangle[3])
{
	vec4<_ST> plane;
	plane.rvec3() = cross(triangle[1] - triangle[0], triangle[2] - triangle[0]);
	plane.rvec3().normalize();
	plane.w = - dot(plane.rvec3(), triangle[0]);
	return plane;
}

template<class _ST>
constexpr vec3<_ST> triangle_normal(const vec3<_ST> triangle[3])
{
	vec3<_ST> normal;
	normal = cross(triangle[1] - triangle[0], triangle[2] - triangle[0]);
	normal.normalize();
	return normal;
}

template<class _ST>
constexpr bool point_in_triangle(const vec3<_ST>& point, const vec3<_ST> triangle[3])
{
	vec3<_ST> u = triangle[1] - triangle[0];
	vec3<_ST> v = triangle[2] - triangle[0];
	vec3<_ST> w = point - triangle[0];

	_ST uv = dot(u, v);
	_ST wv = dot(w, v);
	_ST vv = dot(v, v);
	_ST wu = dot(w, u);
	_ST uu = dot(u, u);

	_ST denom = uv * uv - uu * vv;
	_ST s = (uv * wv - vv * wu) / denom;
	_ST t = (uv * wu - uu * wv) / denom;
		
	return (s >= _ST(0) && t >= _ST(0) && s + t <= _ST(1));
}

template<class _ST>
constexpr bool point_in_triangle_2d(const vec2<_ST>& point, const vec2<_ST> triangle[3])
{
	vec2<_ST> u = triangle[1] - triangle[0];
	vec2<_ST> v = triangle[2] - triangle[0];
	vec2<_ST> w = point - triangle[0];

	_ST uv = dot(u, v);
	_ST wv = dot(w, v);
	_ST vv = dot(v, v);
	_ST wu = dot(w, u);
	_ST uu = dot(u, u);

	_ST denom = uv * uv - uu * vv;
	_ST s = (uv * wv - vv * wu) / denom;
	_ST t = (uv * wu - uu * wv) / denom;
		
	return (s >= _ST(0) && t >= _ST(0) && s + t <= _ST(1));
}

template<class _ST>
constexpr bool intersect_line_plane(vec3<_ST>& result, const vec3<_ST>& line_pt, const vec3<_ST>& line_dir, const vec4<_ST>& plane)
{
	_ST b = dot(plane.rvec3(), line_dir);
	if(fcmp_eq(b, _ST(0)))
		return false; // line paralel to plane
	_ST a = - (dot(plane.rvec3(), line_pt) + plane.w);
	result = line_pt + (a / b) * line_dir;
	return true;
}

template<class _ST>
constexpr bool intersect_ray_plane(vec3<_ST>& result, const vec3<_ST>& ray_pt, const vec3<_ST>& ray_dir, const vec4<_ST>& plane)
{
	_ST b = dot(plane.rvec3(), ray_dir);
	if(fcmp_eq(b, _ST(0)))
		return false; // ray paralel to plane
	_ST a = - (dot(plane.rvec3(), ray_pt) + plane.w);
	_ST t = a / b;
	if(t < _ST(0))
		return false; // plane behind ray
	result = ray_pt + t * ray_dir;
	return true;
}

template<class _ST>
constexpr bool intersect_line_triangle(vec3<_ST>& result, const vec3<_ST>& line_pt, const vec3<_ST>& line_dir, const vec3<_ST> triangle[3])
{
	// find intersection of line and plane containing the triangle
	vec4<_ST> plane;
	vec3<_ST> u = triangle[1] - triangle[0];
	vec3<_ST> v = triangle[2] - triangle[0];
	plane.rvec3() = cross(u, v);
	_ST len_sq = plane.rvec3().length_sq();
	if(fcmp_eq(len_sq, _ST(0)))
		return false; // degenerate triangle
	plane.rvec3() *= _ST(1) / std::sqrt(len_sq);
	plane.w = - dot(plane.rvec3(), triangle[0]);
	vec3<_ST> pt;
	if(!intersect_line_plane(pt, line_pt, line_dir, plane))
		return false;

	// check if the intersection point is within the triangle
	vec3<_ST> w = pt - triangle[0];

	_ST uv = dot(u, v);
	_ST wv = dot(w, v);
	_ST vv = dot(v, v);
	_ST wu = dot(w, u);
	_ST uu = dot(u, u);

	_ST denom = uv * uv - uu * vv;
	_ST s = (uv * wv - vv * wu) / denom;
	_ST t = (uv * wu - uu * wv) / denom;
	if(s >= _ST(0) && t >= _ST(0) && s + t <= _ST(1))
	{
		result = pt;
		return true;
	}
	else
	{
		return false;
	}
}

template<class _ST>
constexpr bool intersect_ray_triangle(vec3<_ST>& result, const vec3<_ST>& ray_pt, const vec3<_ST>& ray_dir, const vec3<_ST> triangle[3])
{
	// find intersection of ray and plane containing the triangle
	vec4<_ST> plane;
	vec3<_ST> u = triangle[1] - triangle[0];
	vec3<_ST> v = triangle[2] - triangle[0];
	plane.rvec3() = cross(u, v);
	_ST len_sq = plane.rvec3().length_sq();
	if(fcmp_eq(len_sq, _ST(0)))
		return false; // degenerate triangle
	plane.rvec3() *= _ST(1) / std::sqrt(len_sq);
	plane.w = - dot(plane.rvec3(), triangle[0]);
	vec3<_ST> pt;
	if(!intersect_ray_plane(pt, ray_pt, ray_dir, plane))
		return false;

	// check if the intersection point is within the triangle
	vec3<_ST> w = pt - triangle[0];

	_ST uv = dot(u, v);
	_ST wv = dot(w, v);
	_ST vv = dot(v, v);
	_ST wu = dot(w, u);
	_ST uu = dot(u, u);

	_ST denom = uv * uv - uu * vv;
	_ST s = (uv * wv - vv * wu) / denom;
	_ST t = (uv * wu - uu * wv) / denom;
	if(s >= _ST(0) && t >= _ST(0) && s + t <= _ST(1))
	{
		result = pt;
		return true;
	}
	else
	{
		return false;
	}
}

template <class _ST>
constexpr vec2<_ST> rotate_90_ccw_2d(const vec2<_ST>& vector)
{
	return vec2<_ST>(-vector.y, vector.x);
}

template <class _ST>
constexpr vec2<_ST> rotate_90_cw_2d(const vec2<_ST>& vector)
{
	return vec2<_ST>(vector.y, -vector.x);
}

/*
	Line is represented in the form:
		ax + by + c = 0
	[a, b] is line normal, c is signed distance from origin
*/
template<class _ST>
constexpr bool intersect_lines_2d(vec2<_ST>& result, const vec3<_ST>& line1, const vec3<_ST>& line2)
{
	_ST denom = line1.y * line2.x - line2.y * line1.x;
	if(fcmp_eq(denom, _ST(0)))
		return false;
	result.x = (line1.z * line2.y - line2.z * line1.y) / denom;
	result.y = (line1.z * line2.x - line2.z * line1.x) / -denom;
	return true;
}

/*
	Evaluates the point relative to a plane (returns signed distance).
	If less than 0, the point is behind the plane,
	if greater than 0, the point is in front of plane,
	and if 0, the point is on the plane.
*/
template <class _ST>
constexpr _ST point_to_plane_sgn_dist(const vec3<_ST>& point, const vec4<_ST>& plane)
{
	return dot(point, plane.rvec3()) + plane.w;
}

// returns absolute distance of a point from a plane
template <class _ST>
constexpr _ST point_to_plane_dist(const vec3<_ST>& point, const vec4<_ST>& plane)
{
	return abs(point_to_plane_sgn_dist(point, plane));
}

template <class _ST>
constexpr vec3<_ST> nearest_point_on_plane(const vec3<_ST>& point, const vec4<_ST>& plane)
{
	_ST d = point_to_plane_sgn_dist(point, plane);
	return point - d * plane.rvec3();
}

template <class _ST>
constexpr vec3<_ST> line_from_points_2d(const vec2<_ST>& point1, const vec2<_ST>& point2)
{
	vec3<_ST> line;
	line.rvec2() = rotate_90_ccw_2d(point2 - point1);
	line.rvec2().normalize();
	line.z = -dot(line.rvec2(), point1);
	return line;
}

template <class _ST>
constexpr vec3<_ST> line_from_point_and_vec_2d(const vec2<_ST>& point, const vec2<_ST>& vec)
{
	vec3<_ST> line;
	line.rvec2() = rotate_90_ccw_2d(vec);
	line.rvec2().normalize();
	line.z = -dot(line.rvec2(), point);
	return line;
}

template <class _ST>
constexpr _ST point_to_line_sgn_dist_2d(const vec2<_ST>& point, const vec3<_ST>& line)
{
	return dot(point, line.rvec2()) + line.z;
}

template <class _ST>
constexpr _ST point_to_line_dist_2d(const vec2<_ST>& point, const vec3<_ST>& line)
{
	return abs(point_to_line_sgn_dist_2d(point, line));
}

template <class _ST>
constexpr vec2<_ST> nearest_point_on_line_2d(const vec2<_ST>& point, const vec3<_ST>& line)
{
	_ST d = point_to_line_sgn_dist_2d(point, line);
	return point - d * line.rvec2();
}

template <class _ST>
constexpr bool point_in_rectangle_2d(const vec2<_ST>& point, const vec4<_ST>& rect)
{
	return (point.x >= rect.x && point.x <= rect.z &&
			point.y >= rect.y && point.y <= rect.w);
}

template <class _ST>
constexpr bool point_in_circle_2d(const vec2<_ST>& point, const vec2<_ST>& center, _ST radius)
{
	return (point - center).length_sq() <= (radius * radius);
}

template <class _ST>
constexpr bool point_in_polygon_2d(const vec2<_ST>& point, const vec2<_ST>* polyVerts, size_t vertexCount)
{
	bool inside = false;

	for (int i = 0, j = vertexCount - 1; i < vertexCount; j = i++)
	{
		if ((((polyVerts[i].y <= point.y) && (point.y < polyVerts[j].y)) || ((polyVerts[j].y <= point.y) && (point.y < polyVerts[i].y))) &&
			(point.x < (polyVerts[j].x - polyVerts[i].x) * (point.y - polyVerts[i].y) / (polyVerts[j].y - polyVerts[i].y) + polyVerts[i].x))
		{
			inside = !inside;
		}
	}

	return inside;
}

template <class _ST>
constexpr bool overlap_rectangle_circle_2d(const vec4<_ST>& rect, const vec2<_ST>& center, _ST radius)
{
	// Find rectangle's point closest to circle center.
	vec2<_ST> closestPt {
		clamp(center.x, rect.x, rect.z),
		clamp(center.y, rect.y, rect.w)
	};

	return (center - closestPt).length_sq() <= (radius * radius);
}

template <class _ST>
constexpr bool overlap_rectangles_2d(const vec4<_ST>& rect1, const vec4<_ST>& rect2)
{
	_ST x1 = std::max(rect1.x, rect2.x);
	_ST x2 = std::min(rect1.z, rect2.z);
	_ST y1 = std::max(rect1.y, rect2.y);
	_ST y2 = std::min(rect1.w, rect2.w);

	return (x1 <= x2 && y1 <= y2);
}

//
// Returns an angle swept between two unit vectors in counter-clockwise direction.
//
template <class _ST>
constexpr _ST angle_between_vectors_2d(const vec2<_ST>& unitVec1, const vec2<_ST>& unitVec2)
{
	_ST angle;
	_ST dotP = dot(unitVec1, unitVec2);
	if (cross(vec3<_ST> { unitVec1, _ST(0) }, vec3<_ST> { unitVec2, _ST(0) }).z > _ST(0))
		angle = std::acos(dotP);
	else
		angle = TWO_PI - std::acos(dotP);

	return angle;
}

//
// Returns unit vector given an angle sweapt starting from positive x-axis.
//
template <class _ST>
constexpr vec2<_ST> unit_vector_from_angle(_ST angle)
{
	return { std::cos(angle), std::sin(angle) };
}

enum class PointOrientation
{
	CW,
	CCW,
	COLINEAR
};

template <class _ST>
constexpr PointOrientation orientation_2d(const vec2<_ST>& pt1, const vec2<_ST>& pt2, const vec2<_ST>& pt3)
{
	_ST val = (pt2.y - pt1.y) * (pt3.x - pt2.x) - (pt2.x - pt1.x) * (pt3.y - pt2.y);

	if (val == _ST(0))
		return PointOrientation::COLINEAR;
	else if (val > _ST(0))
		return PointOrientation::CW;
	else
		return PointOrientation::CCW;
}

template <class _ST>
constexpr bool is_within_bounds(const vec2<_ST>& pt, const vec2<_ST>& bound_pt1, const vec2<_ST>& bound_pt2)
{
	auto minX = std::min(bound_pt1.x, bound_pt2.x);
	auto maxX = std::max(bound_pt1.x, bound_pt2.x);
	auto minY = std::min(bound_pt1.y, bound_pt2.y);
	auto maxY = std::max(bound_pt1.y, bound_pt2.y);

	return (
		pt.x >= minX && pt.x <= maxX &&
		pt.y >= minY && pt.y <= maxY
		);
};

template <class _ST>
constexpr bool is_point_on_segment(const vec2<_ST>& pt, const vec2<_ST>& seg_start, const vec2<_ST>& seg_end)
{
	return (
		orientation_2d(seg_start, seg_end, pt) == PointOrientation::COLINEAR &&
		is_within_bounds(pt, seg_start, seg_end)
		);
}

template <class _ST>
constexpr bool do_line_segments_intersect_2d(const vec2<_ST>& seg1_start, const vec2<_ST>& seg1_end, const vec2<_ST>& seg2_start, const vec2<_ST>& seg2_end)
{
	auto or1 = orientation_2d(seg1_start, seg1_end, seg2_start);
	auto or2 = orientation_2d(seg1_start, seg1_end, seg2_end);
	auto or3 = orientation_2d(seg2_start, seg2_end, seg1_start);
	auto or4 = orientation_2d(seg2_start, seg2_end, seg1_end);

	if (or1 != or2 && or3 != or4)
		return true;

	if (or1 == PointOrientation::COLINEAR && is_within_bounds(seg2_start, seg1_start, seg1_end))
		return true;

	if (or2 == PointOrientation::COLINEAR && is_within_bounds(seg2_end, seg1_start, seg1_end))
		return true;

	if (or3 == PointOrientation::COLINEAR && is_within_bounds(seg1_start, seg2_start, seg2_end))
		return true;

	if (or4 == PointOrientation::COLINEAR && is_within_bounds(seg1_end, seg2_start, seg2_end))
		return true;

	return false;
}

template <class _ST>
constexpr bool do_line_segments_intersect_exclude_endpoints_2d(const vec2<_ST>& seg1_start, const vec2<_ST>& seg1_end, const vec2<_ST>& seg2_start, const vec2<_ST>& seg2_end)
{
	auto or1 = orientation_2d(seg1_start, seg1_end, seg2_start);
	auto or2 = orientation_2d(seg1_start, seg1_end, seg2_end);
	auto or3 = orientation_2d(seg2_start, seg2_end, seg1_start);
	auto or4 = orientation_2d(seg2_start, seg2_end, seg1_end);

	if (or1 != or2 && or3 != or4)
	{
		if (or1 == PointOrientation::COLINEAR)
		{
			if (seg2_start == seg1_start || seg2_start == seg1_end)
				return false;
		}
		else if (or2 == PointOrientation::COLINEAR)
		{
			if (seg2_end == seg1_start || seg2_end == seg1_end)
				return false;
		}

		if (or3 == PointOrientation::COLINEAR)
		{
			if (seg1_start == seg2_start || seg1_start == seg2_end)
				return false;
		}
		else if (or4 == PointOrientation::COLINEAR)
		{
			if (seg1_end == seg2_start || seg1_end == seg2_end)
				return false;
		}

		return true;
	}

	if (or1 == PointOrientation::COLINEAR && is_within_bounds(seg2_start, seg1_start, seg1_end))
	{
		if (or2 == PointOrientation::COLINEAR)
		{
			if (seg2_start == seg1_start)
			{
				if (dot(seg1_end - seg1_start, seg2_end - seg1_start) > 0.0f)
					return true;
			}

			if (seg2_start == seg1_end)
			{
				if (dot(seg1_start - seg1_end, seg2_end - seg1_end) > 0.0f)
					return true;
			}
		}
		else
		{
			if ((seg2_start != seg1_start) && (seg2_start != seg1_end))
				return true;
		}
	}

	if (or2 == PointOrientation::COLINEAR && is_within_bounds(seg2_end, seg1_start, seg1_end))
	{
		if (or1 == PointOrientation::COLINEAR)
		{
			if (seg2_end == seg1_start)
			{
				if (dot(seg1_end - seg1_start, seg2_start - seg1_start) > 0.0f)
					return true;
			}

			if (seg2_end == seg1_end)
			{
				if (dot(seg1_start - seg1_end, seg2_start - seg1_end) > 0.0f)
					return true;
			}
		}
		else
		{
			if ((seg2_end != seg1_start) && (seg2_end != seg1_end))
				return true;
		}
	}

	if (or3 == PointOrientation::COLINEAR && is_within_bounds(seg1_start, seg2_start, seg2_end))
	{
		if (or4 == PointOrientation::COLINEAR)
		{
			if (seg1_start == seg2_start)
			{
				if (dot(seg2_end - seg2_start, seg1_end - seg2_start) > 0.0f)
					return true;
			}

			if (seg1_start == seg2_end)
			{
				if (dot(seg2_start - seg2_end, seg1_end - seg2_end) > 0.0f)
					return true;
			}
		}
		else
		{
			if ((seg1_start != seg2_start) && (seg1_start != seg2_end))
				return true;
		}
	}

	if (or4 == PointOrientation::COLINEAR && is_within_bounds(seg1_end, seg2_start, seg2_end))
	{
		if (or3 == PointOrientation::COLINEAR)
		{
			if (seg1_end == seg2_start)
			{
				if (dot(seg2_end - seg2_start, seg1_start - seg2_start) > 0.0f)
					return true;
			}

			if (seg1_end == seg2_end)
			{
				if (dot(seg2_start - seg2_end, seg1_start - seg2_end) > 0.0f)
					return true;
			}
		}
		else
		{
			if ((seg1_end != seg2_start) && (seg1_end != seg2_end))
				return true;
		}
	}

	return false;
}

template <typename _ST>
constexpr bool do_line_segment_rectangle_intersect_2d(const vec2<_ST>& pt1, const vec2<_ST>& pt2, const vec4<_ST>& rect)
{
	if (point_in_rectangle_2d(pt1, rect))
		return true;

	if (point_in_rectangle_2d(pt2, rect))
		return true;

	if (do_line_segments_intersect_2d(pt1, pt2, { rect.x, rect.y }, { rect.z, rect.y }))
		return true;

	if (do_line_segments_intersect_2d(pt1, pt2, { rect.z, rect.y }, { rect.z, rect.w }))
		return true;

	if (do_line_segments_intersect_2d(pt1, pt2, { rect.z, rect.w }, { rect.x, rect.w }))
		return true;

	if (do_line_segments_intersect_2d(pt1, pt2, { rect.x, rect.w }, { rect.x, rect.y }))
		return true;

	return false;
}

template <typename _ST>
constexpr vec2<_ST> centroid(const vec2<_ST> points[], size_t pointCount)
{
	vec2<_ST> result(_ST(0), _ST(0));

	for (size_t i = 0; i < pointCount; ++i)
		result += points[i] / _ST(pointCount);

	return result;
}

} // namespace math3d

#endif // _MATH3D_GEOMETRY_H_
