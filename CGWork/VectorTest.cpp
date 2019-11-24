/** Testing the vector class **/

#include <iostream>
#include "Vector.h"

using std::cout;
using std::endl;

int main()
{
    Vector vec1, vec2, vec3;

    // Test basic construction
    cout << "Basic Construction Testing - " << endl
         << endl;

    vec1 = Vector();
    vec1.Print();

    vec1 = Vector(5);
    vec1.Print();

    vec1 = Vector(1, 2, 3, 4);
    vec1.Print();

    vec1 = Vector(2, 3, 4);
    vec1.Print();

    vec2 = Vector(vec1);
    vec2.Print();

    cout << endl;

    // Multiply by a consant
    cout << "Multiply by a consant" << endl
         << endl;

    vec1 = vec1 * 1.5;
    vec1.Print();

    vec1 *= 2;
    vec1.Print();

    cout << endl;

    // Dot product
    cout << "Dot product" << endl
         << endl;

    vec1 = Vector(1, 2, 3, 2);
    vec2 = Vector(2, 3, 4, 4);
    vec3 = vec1 * vec2;
    vec3.Print();
    vec3 = vec2 * vec1;
    vec3.Print();

    cout << endl;

    // Cross product
    cout << "Cross product" << endl
         << endl;

    vec1 = Vector(1, 0, 0, 2);
    vec2 = Vector(0, 1, 0, 3);
    vec3 = vec1 ^ vec2;
    vec3.Print();
    vec3 = vec2 ^ vec1;
    vec3.Print();

    vec1 = Vector(1, 2, 3);
    vec2 = Vector(4, 1, 2);
    vec3 = vec1 ^ vec2;
    vec3.Print();
    vec3 = vec2 ^ vec1;
    vec3.Print();

    cout << endl;

    // Normalizing a vector
    cout << "Normalizing a vector" << endl
         << endl;

    vec1 = Vector(1, 1, 0, 5);
    vec1.Normalize();
    vec1.Print();

    vec1 = Vector(3, 3, 3, 2);
    vec1.Normalize();
    vec1.Print();

    vec1 = Vector(0, 0, 0, 2);
    vec1.Normalize();
    vec1.Print();

    cout << endl;

    // Normalizing a vector
    cout << "Homogenizing a vector" << endl
         << endl;

    vec1 = Vector(1, 1, 0, 5);
    vec1.Homogenize();
    vec1.Print();

    vec1 = Vector(3, 3, 3, 0);
    vec1.Homogenize();
    vec1.Print();

    vec1 = Vector(1, 2, 3, 2);
    vec1.Homogenize();
    vec1.Print();

    cout << endl;

    return 0;
}
