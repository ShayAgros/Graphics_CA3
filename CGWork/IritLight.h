/* This file (and its corresponding .c file) declare and implement
 * all light related functions, includin shading.
*/

#pragma once

#include "Vector.h"

/* A light source
 * @pos - represent the light position in the *clipping space*
 * @ai	- ambient light intensity
*/
struct light {
	Vector pos;
	int ai;

};

/* Calculates the light value that multiplies a single value. Takes ambient, diffusive
 * and specular light into account. Uses the state value to know which model is in use
 * @intersecting_x{1,2} - the two points between which we paint the form. This struct also
 *						  contains all the information regarding the lines that contain these
 *						  x values, and their normals.
 * @t					- offset value between the boundries of the painted line. Used for extrapolation
 *						  of the light for points between the intersecting lines
 * @state				- contains all light sources in our world
*/
RGBQUAD calculateLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
					   double t, struct State &state);