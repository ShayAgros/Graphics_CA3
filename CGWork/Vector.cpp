/* Implementation of the Vector class*/

#include "Vector.h"
#include "math.h"
#include <iostream>

using std::cout;
using std::endl;

// Ctor an init value
Vector::Vector(double value)
{
    for (int i = 0; i < 3; i++)
    {
        this->coordinates[i] = value;
    }

    // Keep vector homogenized
    this->coordinates[3] = 1;
}

// Ctor with another vector as parameter
Vector::Vector(const Vector &vec)
{
    for (int i = 0; i < 3; i++)
    {
        this->coordinates[i] = vec.coordinates[i];
    }
}

// Ctor for values for all indices
Vector::Vector(double x, double y, double z, double w)
{
    this->coordinates[0] = x;
    this->coordinates[1] = y;
    this->coordinates[2] = z;
    this->coordinates[3] = w;
}

// Dtor
Vector::~Vector()
{
}

// Multiply each part of the vector with a constant
Vector Vector::operator*(double param) const
{
    Vector vec = Vector();

    vec.coordinates[0] = this->coordinates[0] * param;
    vec.coordinates[1] = this->coordinates[1] * param;
    vec.coordinates[2] = this->coordinates[2] * param;

    // W shouldnt be affected by constant multiplication.

    return vec;
}

void Vector::operator*=(double param)
{
    this->coordinates[0] = this->coordinates[0] * param;
    this->coordinates[1] = this->coordinates[1] * param;
    this->coordinates[2] = this->coordinates[2] * param;

    // W shouldnt be affected by constant multiplication.
}

// Dot multiplication of two vectors
double Vector::operator*(const Vector &vec) const
{
    double sum = 0;

    for (int i = 0; i < 3; i++)
    {
        sum += this->coordinates[i] * vec.coordinates[i];
    }

    // W shouldnt affect dot product

    return sum;
}

// Cross multiplication of two vectors
Vector Vector::operator^(const Vector &vec) const
{
    Vector cross = Vector();

    cross.coordinates[0] = (this->coordinates[1] * vec.coordinates[2]) - (this->coordinates[2] * vec.coordinates[1]);
    cross.coordinates[1] = (this->coordinates[2] * vec.coordinates[0]) - (this->coordinates[0] * vec.coordinates[2]);
    cross.coordinates[2] = (this->coordinates[0] * vec.coordinates[1]) - (this->coordinates[1] * vec.coordinates[0]);

    // W is 1 by default, and doesn't affect cross product

    return cross;
}

// Vector addition
Vector Vector::operator+(const Vector &vec) const
{
    Vector new_vec = Vector();

    for (int i = 0; i < 3; i++)
    {
        new_vec.coordinates[i] = this->coordinates[i] + vec.coordinates[i];
    }

    // W isn't affected;
}

// Vector addition
void Vector::operator+=(const Vector &vec)
{
    for (int i = 0; i < 3; i++)
    {
        this->coordinates[i] += vec.coordinates[i];
    }

    // W isn't affected;
}

// Vector subtraction
Vector Vector::operator-(const Vector &vec) const
{
    Vector new_vec = Vector();

    for (int i = 0; i < 3; i++)
    {
        new_vec.coordinates[i] = this->coordinates[i] - vec.coordinates[i];
    }

    // W isn't affected;
}

// Vector subtraction
void Vector::operator-=(const Vector &vec)
{
    for (int i = 0; i < 3; i++)
    {
        this->coordinates[i] -= vec.coordinates[i];
    }

    // W isn't affected;
}

// Vector array index
double &Vector::operator[](const int index) {
	return coordinates[index];
}

// Normalize the vector
void Vector::Normalize()
{
    double norm = sqrt(pow(coordinates[0], 2) + pow(coordinates[1], 2) + pow(coordinates[2], 2));

    if (norm != 0)
    {
        coordinates[0] /= norm;
        coordinates[1] /= norm;
        coordinates[2] /= norm;
    }

    // Doesn't affect W
}

// Homogenize the vector
void Vector::Homogenize()
{
    if (coordinates[3] != 0)
    {
        coordinates[0] /= coordinates[3];
        coordinates[1] /= coordinates[3];
        coordinates[2] /= coordinates[3];

        coordinates[3] = 1;
    }
}

// Print the vector - for debug purposes
void Vector::Print() const
{
    cout << "<";
    for (int i = 0; i < 4; i++)
    {
        cout << coordinates[i] << ", ";
    }
    cout << ">" << endl;
}
