#ifndef __VECTOR_H__
#define __VECTOR_H__

/* Header file for the vector class */

class Vector
{
public:
    double coordinates[4];

	// default vector. all zeros
	Vector();

    // Ctor an init value
    Vector(double value);

    // Copy Ctor
    Vector(const Vector &vec);

    // Ctor for values for all indices
    Vector(double x, double y, double z, double w = 1);

    // Dtor
    ~Vector();

    // Multiply each part of the vector with a constant
    Vector operator*(double param) const;

    void operator*=(double param);

    // Dot multiplication of two vectors
    double operator*(const Vector &vec) const;

    // Cross multiplication of two vectors
    Vector operator^(const Vector &vec) const;

    // Vector addition
    Vector operator+(const Vector &vec) const;

    // Vector addition
    void operator+=(const Vector &vec);

    // Vector subtraction
    Vector operator-(const Vector &vec) const;

    // Vector subtraction
    void operator-=(const Vector &vec);

	// Vector array index
	double &operator[](const int index);

	double operator[](int index) const;

    // Normalize the vector
    void Normalize();

    // Homogenize the vector
    void Homogenize();

    // Print the vector - for debug purposes
    void Print() const;
};

#endif // __VECTOR_H__