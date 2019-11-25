#include "IritObjects.h"

/* This function creates a matrix which traslates a point by x, y, z
 * @x - x offset
 * @y - y offset
 * @z - z offset
 */
Matrix createTranslationMatrix(double &x, double &y, double z = 0) {
	Matrix translation = Matrix::Identity();

	translation.array[0][3] = x;
	translation.array[1][3] = y;
	translation.array[2][3] = z;

	return translation;
}

/* This function creates a matrix which traslates a point by a vector
 * @v - the vector by which to translate
 */
Matrix createTranslationMatrix(Vector &v) {

	return createTranslationMatrix(v.coordinates[0],
		v.coordinates[1],
		v.coordinates[2]);
}

void IritWorld::draw(CDC *pDCToUse) {
	Matrix transformation = createTranslationMatrix(m_axes_origin);
	for (int i = 0; i < m_objects_nr; i++)
		m_objects_arr[i]->draw(pDCToUse, transformation);
}