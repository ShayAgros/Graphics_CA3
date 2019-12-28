/* This file implements all light related functionality,
 * including methods from other claases which are related
 * to lightning
*/

#include "IritObjects.h"
#include "IritLight.h"


void IritWorld::addLightSource(Vector &pos, int intensity)
{
	struct light new_light = { pos, intensity };
	struct light *lights = new struct light[state.lights_nr + 1];

	for (int i = 0; i < state.lights_nr; i++)
		lights[i] = state.lights[i];
	lights[state.lights_nr] = new_light;
	state.lights_nr++;

	if (state.lights_nr > 0) {
		delete[] state.lights;
	}
	state.lights = lights;
}

RGBQUAD vector_to_rgbq(Vector &vector)
{
	BYTE red = (BYTE)vector[0];
	BYTE green = (BYTE)vector[1];
	BYTE blue = (BYTE)vector[2];

	return { red, green, blue, 0 };
}

Vector extrapolate_normal(struct IntersectionPoint &intersecting_p)
{
	double t;
	double intersecting_2d_x = intersecting_p.x;
	double intersecting_2d_y = intersecting_p.y;
	struct threed_line *containing_line = intersecting_p.containing_line;
	double left_2d_x = containing_line->x1;
	double right_2d_x = containing_line->x2;
	double upper_2d_x = containing_line->y2;
	double lower_2d_x = containing_line->y1;
	Vector &left_x_normal = containing_line->p1.normal;
	Vector &right_x_normal = containing_line->p2.normal;

	if (left_2d_x == right_2d_x && upper_2d_x == lower_2d_x)
		printf("Error\n");

	// We find which axis has more difference between its two boundries, to
	// get a more fine-grained normal change
	if (right_2d_x - left_2d_x > upper_2d_x - lower_2d_x)
		t = (intersecting_2d_x - left_2d_x) / (right_2d_x - left_2d_x);
	else
		t = (intersecting_2d_y - lower_2d_x) / (upper_2d_x - lower_2d_x);

	return (left_x_normal * (1 - t)) + (right_x_normal * t);
}

Vector calculatePhongLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
						   int t, struct State &state)
{
	int ambient_intensity = 60;
	Vector ka = Vector(0.75);

	Vector left_side_normal = extrapolate_normal(intersecting_x1);
	Vector right_side_normal = extrapolate_normal(intersecting_x2);

	return ka * ambient_intensity;
}

RGBQUAD calculateLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
					   int t, struct State &state)
{
	RGBQUAD rgbq = { 0, 0, 0, 0 };

	//return color_black, if there are no lights
	if (!state.lights_nr)
		return rgbq;

	if (state.shading_mode == SHADING_M_PHONG)
		rgbq = vector_to_rgbq(calculatePhongLight(intersecting_x1, intersecting_x2, t, state));

	return rgbq;
}