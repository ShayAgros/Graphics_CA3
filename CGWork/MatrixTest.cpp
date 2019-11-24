/* Testing the Matrix class */

#include "Matrix.h"
#include <iostream>

using namespace std;

int main()
{
    Matrix mat1;
    Matrix mat2;
    Matrix mat3;

    double array[4][4] = {
        {1, 2, 3, 4},
        {1, 4, 2, 1},
        {0, 2, 1, -2},
        {1, 1, -2, 0}};

    // Check basic construction
    cout << "Basic Construction - " << endl
         << endl;

    mat1 = Matrix();
    mat1.Print();

    mat1 = Matrix(3);
    mat1.Print();

    mat1 = Matrix::Identity();
    mat1.Print();

    mat2 = mat1;
    mat2.Print();

    mat1 = Matrix(array);
    mat1.Print();

    cout << endl;

    // Check arythmatics
    cout << "Checking arythmatics - " << endl
         << endl;

    mat1 = mat1 * 4;
    mat1.Print();

    mat1 *= 0.25;
    mat1.Print();

    cout << endl;

    // Check Transpose
    cout << "Checking transpose - " << endl
         << endl;

    mat1.Print();
    mat2 = mat1.Transpose();
    mat2.Print();

    cout << endl;

    // Check Adjoint
    cout << "Checking adjoint - " << endl
         << endl;

    mat1.Print();
    mat2 = mat1.Adjoint();
    mat2.Print();

    cout << endl;

    // Check determinant
    cout << "Checking determinant - " << endl
         << endl;

    mat1 = Matrix::Identity();
    mat1.Print();
    cout << mat1.Determinant() << endl;
    mat1 *= 3;
    mat1.Print();
    cout << mat1.Determinant() << endl;
    mat1 = Matrix(array);
    mat1.Print();
    cout << mat1.Determinant() << endl;

    cout << endl;

    // Check Inverse
    cout << "Checking inverse - " << endl
         << endl;

    mat1 = Matrix::Identity() * 3;
    mat1.Print();
    mat2 = mat1.Inverse();
    mat2.Print();
    mat3 = mat1 * mat2;
    mat3.Print();

    mat1 = Matrix(array);
    mat1.Print();
    mat2 = mat1.Inverse();
    mat2.Print();
    mat3 = mat1 * mat2;
    mat3.Print();

    cout << endl;
}