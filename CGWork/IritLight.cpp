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

void extrapolate_normal_and_vertex(struct IntersectionPoint &intersecting_p, Vector &extrapolated_normal, Vector &extrapolated_pos)
{
	double t;
	double intersecting_2d_x = intersecting_p.x;
	double intersecting_2d_y = intersecting_p.y;
	struct threed_line *containing_line = intersecting_p.containing_line;
	double left_2d_x = containing_line->x1;
	double right_2d_x = containing_line->x2;
	double upper_2d_x = containing_line->y2;
	double lower_2d_x = containing_line->y1;
	Vector &left_x_normal = containing_line->p1.normal_irit;
	Vector &right_x_normal = containing_line->p2.normal_irit;
	Vector &left_x_vertex = containing_line->p1.vertex;
	Vector &right_x_vertex = containing_line->p2.vertex;

	if (left_2d_x == right_2d_x && upper_2d_x == lower_2d_x)
		printf("Error\n");

	// We find which axis has more difference between its two boundries, to
	// get a more fine-grained normal change
	if (abs(right_2d_x - left_2d_x) > abs(lower_2d_x - upper_2d_x))
		t = (intersecting_2d_x - left_2d_x) / (right_2d_x - left_2d_x);
	else
		t = (intersecting_2d_y - lower_2d_x) / (upper_2d_x - lower_2d_x);

	if (t < 0 || t > 1)
		printf("STOP\n");

	extrapolated_normal = (left_x_normal * (1 - t)) + (right_x_normal * t);
	extrapolated_pos = (left_x_vertex * (1 - t)) + (right_x_vertex * t);
}

/* See http://www.ogldev.org/www/tutorial19/tutorial19.html for reflection calculations */
Vector calculatePhongLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
						   double t, struct State &state)
{
	/* General */
	Vector light_source_pos = Vector(0, 0, 0.8);
	Vector camera_position = Vector(0, 0, 2);

	/* Ambient */
	int ambient_intensity = 90;
	Vector ka = Vector(0.4);

	/* Diffusive */
	Vector kd = Vector(0.8);
	int point_source_intensity = 70;

	/* Specular */
	Vector ks = Vector(0.7);

	/* Positional vectors */
	Vector left_side_normal;
	Vector left_side_pos;

	Vector right_side_normal;
	Vector right_side_pos;

	Vector point_normal;
	Vector point_pos;

	Vector point_to_light;
	Vector light_to_point;
	Vector point_to_eye;
	Vector reflected_vector;

	// extrapolate the normals and position of the containing lines boundries
	extrapolate_normal_and_vertex(intersecting_x1, left_side_normal, left_side_pos);
	extrapolate_normal_and_vertex(intersecting_x2, right_side_normal, right_side_pos);

	// The 3D-normal and the 3D-position of our intersection point
	point_normal = (left_side_normal * (1 - t)) + (right_side_normal * t);
	point_pos = (left_side_pos * (1 - t)) + (right_side_pos * t);

	point_normal.Normalize();

	point_to_light = light_source_pos - point_pos;
	light_to_point = point_to_light * (-1);
	point_to_eye = camera_position - point_pos;
	reflected_vector = light_to_point - (point_normal * (point_normal * light_to_point) * 2);

	point_to_light.Normalize();
	reflected_vector.Normalize();
	point_to_eye.Normalize();
	light_to_point.Normalize();

	double cos_theta = point_normal * point_to_light;

	if(cos_theta > 0.95)
		printf("Stop here\n");

	if (cos_theta > 0) {
		Vector diffusive = kd * point_source_intensity * cos_theta;
		double alpha = reflected_vector * point_to_eye;
		if (alpha > 0) {
			Vector specular_light = ks * point_source_intensity * (pow(alpha, 50));
			return /*ka * ambient_intensity +*/ diffusive + specular_light;
		}
		return diffusive;
	}
	return Vector(0, 0, 0);
}

RGBQUAD calculateLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
					   double t, struct State &state)
{
	RGBQUAD rgbq = { 0, 0, 0, 0 };

	//return color_black, if there are no lights
	if (!state.lights_nr)
		return rgbq;

	if (state.shading_mode == SHADING_M_PHONG)
		rgbq = vector_to_rgbq(calculatePhongLight(intersecting_x1, intersecting_x2, t, state));

	if (rgbq.rgbRed > 128 || rgbq.rgbGreen > 128 || rgbq.rgbBlue > 128)
		printf("Here starts trouble\n");

	return rgbq;
}