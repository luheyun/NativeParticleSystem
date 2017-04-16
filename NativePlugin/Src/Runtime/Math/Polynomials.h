#ifndef POLYNOMIALS_H
#define POLYNOMIALS_H

#include "FloatConversion.h"

// Returns the highest root for the cubic x^3 + px^2 + qx + r
inline double CubicPolynomialRoot(const double p, const double q, const double r)
{
	double rcp3 = 1.0/3.0;
	double half = 0.5;
	double po3 = p*rcp3;
	double po3_2 = po3*po3;
	double po3_3 = po3_2*po3;
	double b = po3_3 - po3*q*half + r*half;
	double a = -po3_2 + q*rcp3;
	double a3 = a*a*a;
	double det = a3 + b*b;

	if (det >= 0)
	{
		double r0 = sqrt(det) - b;
		r0 = r0 > 0 ? pow(r0, rcp3) : -pow(-r0, rcp3);

		return - po3 - a/r0 + r0;
	}

	double abs = sqrt(-a3);
	double arg = acos(-b/abs);
	abs = pow(abs, rcp3);
	abs = abs - a/abs;
	arg = -po3 + abs*cos(arg*rcp3);
	return arg;
}

// Calculates all real roots of polynomial ax^2 + bx + c (and returns how many)
inline int QuadraticPolynomialRootsGeneric(const float a, const float b, const float c, float& r0, float& r1)
{
	const float eps = 0.00001f;
	if (Abs(a) < eps)
	{
		if (Abs(b) > eps)
		{
			r0 = -c/b;
			return 1;
		}
		else
			return 0;
	}

	float disc = b*b - 4*a*c;
	if (disc < 0.0f)
		return 0;
	
	const float halfRcpA = 0.5f/a;
	const float sqrtDisc = sqrt(disc);
	r0 = (sqrtDisc-b)*halfRcpA;
	r1 = (-sqrtDisc-b)*halfRcpA;
	return 2;
}

// Calculates all the roots for the cubic ax^3 + bx^2 + cx + d. Max num roots is 3.
inline int CubicPolynomialRootsGeneric(float* roots, const double a, const double b, const double c, const double d)
{
	int numRoots = 0;
	if(Abs(a) >= 0.0001f)
	{
		const double p = b / a;
		const double q = c / a;
		const double r = d / a;
		roots[0] = CubicPolynomialRoot(p, q, r);
		numRoots++;

		double la = a;
		double lb = b + a * roots[0];
		double lc = c + b*roots[0] + a*roots[0]*roots[0];
		numRoots += QuadraticPolynomialRootsGeneric(la, lb, lc, roots[1], roots[2]);
	}
	else
	{
		numRoots += QuadraticPolynomialRootsGeneric(b, c, d, roots[0], roots[1]);
	}

	return numRoots;
}

// Specialized version of QuadraticPolynomialRootsGeneric that returns the largest root
inline float QuadraticPolynomialRoot(const float a, const float b, const float c)
{
	float r0=0.0f, r1=0.0f;
	QuadraticPolynomialRootsGeneric(a, b, c, r0, r1);
	return r0;
}

#endif
