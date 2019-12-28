/* This file implements all methods that create of modify
 * one of the transformation matrices.
*/
#include "IritObjects.h"
#include "Matrix.h"
#include "Vector.h"
#include "iritSkel.h"



void IritWorld::setScreenMat(Vector axes[NUM_OF_AXES], Vector &axes_origin, int screen_width, int screen_height) {
	Matrix coor_mat;
	Matrix center_mat;
	Matrix ratio_mat;
	int min_size = min(screen_width, screen_height);

	// Expand to ratio

	// Ratio should be about a fifth of the screen.
	ratio_mat = Matrix::Identity();
	ratio_mat.array[0][0] = min_size / 2;
	ratio_mat.array[1][1] = min_size / 2;
	//	ratio_mat.array[0][0] = screen_width / 2;
	//	ratio_mat.array[1][1] = screen_height / 2;
	state.ratio_mat = ratio_mat;

	// Set world coordinate system
	coor_mat = Matrix(axes[0], axes[1], axes[2]);
	state.coord_mat = coor_mat;

	// Center to screen
	center_mat = createTranslationMatrix(axes_origin);
	state.center_mat = center_mat;
}

/* This function creates the Orthogonal matrix. The purpose of this matrix is to
 * transform all objects into a cube whose center at the origin, at all the edges
 * are of length 2.
*/
void IritWorld::setOrthoMat()
{

	double max_x = max_bound_coord[X_AXIS],
		min_x = min_bound_coord[X_AXIS],
		max_y = max_bound_coord[Y_AXIS],
		min_y = min_bound_coord[Y_AXIS],
		max_z = max_bound_coord[Z_AXIS],
		min_z = min_bound_coord[Z_AXIS];

	state.ortho_mat = Matrix::Identity();

	max_z = (state.view_mat * Vector(0, 0, max_z, 1))[Z_AXIS]; // *1.75;
	min_z = (state.view_mat * Vector(0, 0, min_z, 1))[Z_AXIS]; // *0.25;

	state.ortho_mat.array[X_AXIS][0] = 2 / (max_x - min_x);
	state.ortho_mat.array[Y_AXIS][1] = 2 / (max_y - min_y);
	state.ortho_mat.array[Z_AXIS][2] = 2 / (max_z - min_z);

	state.ortho_mat.array[X_AXIS][3] = -(max_x + min_x) / (max_x - min_x);
	state.ortho_mat.array[Y_AXIS][3] = -(max_y + min_y) / (max_y - min_y);
	state.ortho_mat.array[Z_AXIS][3] = -(max_z + min_z) / (max_z - min_z);
}

Matrix IritWorld::getPerspectiveMatrix(const double &angleOfView, const double &near_z, const double &far_z)
{
	Matrix projection_mat(Matrix::Identity());
	// set the basic projection matrix
	double scale = 1 / tan(angleOfView * 0.5 * M_PI / 180);

	projection_mat.array[0][0] = scale;
	projection_mat.array[1][1] = scale;
	projection_mat.array[2][2] = ((double)near_z + far_z) / ((double)far_z - near_z);
	projection_mat.array[2][3] = (2 * (double)near_z * (double)far_z) / ((double)far_z - near_z);
	projection_mat.array[3][2] = -1; // set w = -z
	projection_mat.array[3][3] = 0;
	return projection_mat;
}

/* The ortho_mat transforms the figure into a cubic with side
 * of length 1 cantered at the origin. Since changing into perspective
 * view involves dividing by z, we cannot allow the figure to stay
 * to pass the z=0 point. To switch to perspective view, we first move
 * the figure in some offset of z.
*/
Matrix IritWorld::createProjectionMatrix() {
	if (state.is_perspective_view) {
		Matrix translation = createTranslationMatrix(0, 0, -8);
		Matrix scale_back = Matrix::createScaleMatrix(8, 8, 1);
		Matrix perspective_matrix = getPerspectiveMatrix(90, 7, 9);

		return scale_back * perspective_matrix * translation;
	}

	// we're in orthogonal view
	return Matrix::Identity();
}

Matrix createTranslationMatrix(double x, double y, double z) {
	Matrix translation = Matrix::Identity();

	translation.array[0][3] = x;
	translation.array[1][3] = y;
	translation.array[2][3] = z;

	return translation;
}

/* This function creates a matrix which translates a point by a vector
 * @v - the vector by which to translate
 */
Matrix createTranslationMatrix(Vector &v) {

	return createTranslationMatrix(v.coordinates[0],
		v.coordinates[1],
		v.coordinates[2]);
}

Matrix createViewMatrix(double x, double y, double z)
{
	Matrix camera_translation = createTranslationMatrix(-x, -y, -z);
	Matrix camera_rotation;

	// TODO: This actually needs to be implemented with vectors and cross
	// see 'transformation' tutorial, slide 16
	// Make it look at the (0, 0, -1) direction
	camera_rotation.array[0][0] = 1;
	camera_rotation.array[1][1] = 1;
	camera_rotation.array[2][2] = 1;
	camera_rotation.array[3][3] = 1;

	return camera_rotation * camera_translation;
}