#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>


class Vec3 {
public:
    Vec3(double x_, double y_, double z_)
	{
        x = x_; y = y_; z = z_;
    }

    // default initialisation
    Vec3() { x = y = z = 0; }

    // norm and norm-squared of v
    double norm() const { return sqrt(x*x + y*y + z*z); }
    double norm2() const { return x*x + y*y + z*z; }

    void normalise()
	{
        double d = norm();
        x /= d; y /= d; z /= d;
    }

	// +=
	Vec3 &operator+=(const Vec3 &rhs) {
		x+=rhs.x; y+=rhs.y; z+=rhs.z;
		return *this;
	}

	// -=
	Vec3 &operator-=(const Vec3 &rhs) {
		x-=rhs.x; y-=rhs.y; z-=rhs.z;
		return *this;
	}

	// Componentwise *=
	Vec3 &operator*=(const Vec3 &rhs) {
		x*=rhs.x; y*=rhs.y; z*=rhs.z;
		return *this;
	}

	// Scalar *=
	Vec3 &operator*=(double rhs) {
		x*=rhs; y*=rhs; z*=rhs;
		return *this;
	}

	// Scalar /=
	Vec3 &operator/=(double rhs) {
		x/=rhs; y/=rhs; z/=rhs;
		return *this;
	}

	double x, y, z;
};


// VECTOR FUNCTIONS
// Binary +
inline Vec3 operator+(const Vec3 &lhs, const Vec3 &rhs) {
    return Vec3(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z);
}

// Binary -
inline Vec3 operator-(const Vec3 &lhs, const Vec3 &rhs) {
    return Vec3(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z);
}


// Unary -
inline Vec3 operator-(const Vec3 &rhs) {
    return Vec3(-rhs.x, -rhs.y, -rhs.z);
}

// Scalar * vector
inline Vec3 operator*(const double d, const Vec3 &rhs) {
    return Vec3(d*rhs.x, d*rhs.y, d*rhs.z);
}

// Vector * scalar
inline Vec3 operator*(const Vec3 &rhs, const double d) {
    return Vec3(d*rhs.x, d*rhs.y, d*rhs.z);
}

// Componentwise vector product
inline Vec3 operator*(const Vec3 &lhs, const Vec3 &rhs) {
    return Vec3(lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z);
}

// Vector / scalar
inline Vec3 operator/(const Vec3 &rhs, const double d) {
    return Vec3(rhs.x/d, rhs.y/d, rhs.z/d);
}

// Dot product
inline double dot(const Vec3 &a, const Vec3 &b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

// Cross product
inline Vec3 cross(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.y*b.z - a.z*b.y,
                a.z*b.x - a.x*b.z,
                a.x*b.y - a.y*b.x);
}

// ostream output
inline std::ostream &operator<<(std::ostream &os, const Vec3 &v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

#endif
