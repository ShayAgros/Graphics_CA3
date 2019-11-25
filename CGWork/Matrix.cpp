/* Implementation of the Matrix class */

#include "Matrix.h"
#include <iostream>
#include <math.h>
#include "Vector.h"

using std::cout;
using std::endl;

double SubDeterminant(double array[3][3]);

Matrix::Matrix(double value)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            array[i][j] = value;
        }
    }
}

// Copy Ctor
Matrix::Matrix(const Matrix &matrix)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            this->array[i][j] = matrix.array[i][j];
        }
    }
}

// Ctor for values for all indices
Matrix::Matrix(double matrix[4][4])
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            this->array[i][j] = matrix[i][j];
        }
    }
}

Matrix::Matrix(Vector &v1, Vector &v2, Vector &v3, Vector &v4)
{
	double coordinates[4][4] = {
			{v1.coordinates[0], v1.coordinates[1], v1.coordinates[2], v1.coordinates[3]},
			{v2.coordinates[0], v2.coordinates[1], v2.coordinates[2], v2.coordinates[3]},
			{v3.coordinates[0], v3.coordinates[1], v3.coordinates[2], v3.coordinates[3]},
			{v4.coordinates[0], v4.coordinates[1], v4.coordinates[2], v4.coordinates[3]}
	};
	Matrix::Matrix(coordinates);
}

Matrix::Matrix(Vector &v1, Vector &v2, Vector &v3) {
	Vector v4(0, 0, 0, 1);

	Matrix(v1, v2, v3, v4);
}

// Dtor
Matrix::~Matrix()
{
}

// Multiply each part of the vector with a constant
Matrix Matrix::operator*(double param) const
{
    Matrix matrix;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            matrix.array[i][j] = this->array[i][j] * param;
        }
    }

    return matrix;
}

// Multiply each part of the vector with a constant
void Matrix::operator*=(double param)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            this->array[i][j] *= param;
        }
    }
}

// Multiply 2 matrices
Matrix Matrix::operator*(const Matrix &matrix) const
{
    Matrix new_matrix = Matrix();

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                new_matrix.array[i][j] += this->array[i][k] * matrix.array[k][j];
            }
        }
    }

    return new_matrix;
}

// Multiply a Matrix and a vector
Vector Matrix::operator*(Vector &vector) const
{
    Vector new_vec;

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
			new_vec[i] += vector[j] * array[i][j];
        }
    }
	return new_vec;
}

// Calculate the determinant of the Matrix
double Matrix::Determinant() const
{
    double determinant = 0;
    Matrix adj = this->Adjoint();

    determinant = 0;

    for (int i = 0; i < 4; i++)
    {
        determinant += adj.array[i][0] * this->array[0][i];
    }

    return determinant;
}

// Return the transpose the matrix
Matrix Matrix::Transpose() const
{
    Matrix matrix = Matrix();

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            matrix.array[i][j] = this->array[j][i];
        }
    }

    return matrix;
}

// Return the inverse of the matrix
Matrix Matrix::Inverse() const
{
    MatrixNotReversible exception;
    Matrix matrix;
    double determinant = this->Determinant();

    if (determinant != 0)
    {
        matrix = this->Adjoint();
        matrix *= (1.0 / determinant);
    }
    else
    {
        throw exception;
    }

    return matrix;
}

Matrix Matrix::Adjoint() const
{
    Matrix matrix = Matrix();
    double temp_arr[3][3];
    int index, sign;

    // Calculate Cofactor for each index
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            index = 0;
            // Create sub array
            for (int k = 0; k < 4; k++)
            {
                for (int l = 0; l < 4; l++)
                {
                    if (k == i || l == j)
                    {
                        continue;
                    }
                    *((double *)temp_arr + index++) = this->array[k][l];
                }
            }
            // if (i+j) is even, then sign is 1, else -1
            sign = (int)pow(-1, i + j);
            matrix.array[i][j] = sign * SubDeterminant(temp_arr);
        }
    }

    matrix = matrix.Transpose();
    return matrix;
}

double SubDeterminant(double array[3][3])
{
    double determinant,
        sub_value_a,
        sub_value_b,
        sub_value_c;

    sub_value_a = array[0][0] * ((array[1][1] * array[2][2]) - (array[1][2] * array[2][1]));
    sub_value_b = array[0][1] * ((array[1][0] * array[2][2]) - (array[1][2] * array[2][0]));
    sub_value_c = array[0][2] * ((array[1][0] * array[2][1]) - (array[1][1] * array[2][0]));

    determinant = sub_value_a - sub_value_b + sub_value_c;

    return determinant;
}

// Return an identity matrix
Matrix Matrix::Identity()
{
    Matrix matrix = Matrix();

    for (int i = 0; i < 4; i++)
    {
        matrix.array[i][i] = 1;
    }

    return matrix;
}

// Print the vector - for debug purposes
void Matrix::Print() const
{
    for (int i = 0; i < 4; i++)
    {
        cout << "<";
        for (int j = 0; j < 4; j++)
        {
            cout << this->array[i][j] << " ";
        }
        cout << ">" << endl;
    }
}
