#ifndef RECT_H
#define RECT_H

#include "Vector2.h"

/// A rectangle.
template <typename T>
class RectT
{
public:
	typedef RectT<T> RectType;
	typedef float BaseType;

	T x; ///< Rectangle x coordinate.
	T y; ///< Rectangle y coordinate.
	T width; ///< Rectangle width.
	T height; ///< Rectangle height.

	//DECLARE_SERIALIZE_OPTIMIZE_TRANSFER (Rectf)
	inline static const char* GetTypeString ();
	inline static bool MightContainPPtr () { return false; }
	inline static bool AllowTransferOptimization ()	{ return true; }

	/// Create a empty rectangle.
	RectT ()
	{
		Reset ();
	}

	/// Create a new rectangle.
	RectT (T inX, T inY, T iWidth, T iHeight)
	{
		x = inX; width = iWidth;
		y = inY; height = iHeight;
	}
	
	// Top/Bottom assume (0,0) is top left like in Direct3D.
	// In OpenGL the y coordinates are inverted, so Top/Bottom is flipped.
	T GetRight() const { return x + width; }
	T GetBottom() const { return y + height; }
	void SetLeft(T l) { T oldXMax = GetXMax(); x = l; width = oldXMax - x; }
	void SetTop(T t) { T oldYMax = GetYMax(); y = t; height = oldYMax - y; }
	void SetRight(T r) { width = r - x; }
	void SetBottom(T b) { height = b - y; }


	T GetXMax() const { return x + width; }
	T GetYMax() const { return y + height; }

	/// Return true if rectangle is empty.
	inline bool IsEmpty () const { return width <= 0 || height <= 0; }
	
	inline void		SetPosition(const Vector2f& position) { x = position.x; y = position.y; }
	inline Vector2f GetPosition() const { return Vector2f(x, y); }

	inline void		SetSize(const Vector2f& size) { width = size.x; height = size.y; }
	inline Vector2f GetSize() const { return Vector2f(width, height); }
	/// Resets the rectangle
	inline void Reset() { x = y = width = height = 0; }

	/// Sets the rectangle
	inline void Set(T inX, T inY, T iWidth, T iHeight)
	{
		x = inX; width = iWidth;
		y = inY; height = iHeight;
	}

	inline void Scale (T dx, T dy)		{ x *= dx; width *= dx; y *= dy; height *= dy;}

	/// Set Center position of rectangle (size stays the same)
	void SetCenterPos (T cx, T cy)		{ x = cx - width / 2; y = cy - height / 2; }
	Vector2f GetCenterPos() const		{ return Vector2f(x + (BaseType)width / 2, y + (BaseType)height / 2); }

	/// Ensure this is inside the rect r.
	void Clamp (const RectType &r)
	{
		T x2 = x + width;
		T y2 = y + height;
		T rx2 = r.x + r.width;
		T ry2 = r.y + r.height;

		if (x < r.x) x = r.x;
		if (x2 > rx2) x2 = rx2;
		if (y < r.y) y = r.y;
		if (y2 > ry2) y2 = ry2;

		width = x2 - x;
		if (width < 0) width = 0;

		height = y2 - y;
		if (height < 0) height = 0;
	}

	/// Move rectangle by deltaX, deltaY.
	inline void Move (T dX, T dY)		{ x += dX; y += dY; }

	/// Return the width of rectangle.
	inline T Width () const					{ return width; }

	/// Return the height of rectangle.
	inline T Height () const					{ return height; }

	/// Return true if a point lies within rectangle bounds.
	inline bool Contains (T px, T py) const		{ return (px >= x) && (px < x + width) && (py >= y) && (py < y + height); }
	inline bool Contains (const Vector2f& p) const		{ return Contains(p.x, p.y); }
	/// Return true if a relative point lies within rectangle bounds.
	inline bool ContainsRel (T x, T y) const
	{ return (x >= 0) && (x < Width ()) && (y >= 0) && (y < Height ()); }

	inline bool Intersects(const RectType& r) const
	{
		// Rects are disjoint if there's at least one separating axis
		bool disjoint = x + width < r.x;
		disjoint |= r.x + r.width < x;
		disjoint |= y + height < r.y;
		disjoint |= r.y + r.height < y;
		return !disjoint;
	}

	/// Normalize a rectangle such that xmin <= xmax and ymin <= ymax.
	inline void Normalize ()
	{
		width = std::max<T>(width, 0);
		height = std::max<T>(height, 0);
	}
	
	bool operator == (const RectType& r)const		{ return x == r.x && y == r.y && width == r.width && height == r.height; }
	bool operator != (const RectType& r)const		{ return x != r.x || y != r.y || width != r.width || height != r.height; }

	// not meaningful; only for use with associative containers
	bool operator < (const RectType& r) const
	{
		if (x != r.x)
			return x < r.x;
		else if (y != r.y)
			return y < r.y;
		else if (width != r.width)
			return width < r.width;
		else
			return height < r.height;
	}
};

typedef RectT<float> Rectf;
typedef RectT<int> RectInt;

inline bool CompareApproximately (const Rectf& lhs, const Rectf& rhs)
{
	return CompareApproximately (lhs.x, rhs.x) && CompareApproximately (lhs.y, rhs.y) &&
	         CompareApproximately (lhs.width, rhs.width) && CompareApproximately (lhs.height, rhs.height);
}

/// Make a rect with width & height
template<typename T>
inline RectT<T> MinMaxRect (T minx, T miny, T maxx, T maxy) { return RectT<T> (minx, miny, maxx - minx, maxy - miny); }

// RectT<float> specialization
template<>
inline bool Rectf::IsEmpty () const { return width <= 0.00001F || height <= 0.00001F; }

#endif
