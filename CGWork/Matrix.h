#ifndef __MATRIX_H__
#define __MATRIX_H__

/* Header file for the matrix class */

#include <exception>
#include "Vector.h"

using namespace std;

class Matrix
{
public:
    double array[4][4];

    // Ctor with an init value
    Matrix(double value = 0);

    // Copy Ctor
    Matrix(const Matrix &matrix);

    // Ctor for values for all indices
    Matrix(double matrix[4][4]);

	/* Create a matrix out of four vector.
	 * The fourh vector is (0 0 0 1)
	 * @vi - the vector in row i
	 */
	Matrix(Vector &v1, Vector &v2, Vector &v3, Vector &v4);

	/* Create a matrix out of three vector.
	 * The fourh vector is (0 0 0 1)
	 * @vi - the vector in row i
	 */
	Matrix(Vector &v1, Vector &v2, Vector &v3);

    // Dtor
    ~Matrix();

    // Multiply each part of the vector with a constant
    Matrix operator*(double param) const;

    void operator*=(double param);

    // Multiply 2 matrices
    Matrix operator*(const Matrix &matrix) const;

    // Multiply a Matrix and a vector
    Vector operator*(const Vector &vector) const;

    // Calculate the determinant of the Matrix
    double Determinant() const;

    // Return the transpose the matrix
    Matrix Transpose() const;

    // Return the adjoint of the matrix
    Matrix Adjoint() const;

    // Return the inverse of the matrix
    Matrix Inverse() const;

    // Return an identity matrix
    static Matrix Identity();

    // Print the vector - for debug purposes
    void Print() const;

    class MatrixNotReversible : public exception{};
};

#endif // __MATRIX_H__