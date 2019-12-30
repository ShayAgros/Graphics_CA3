/* This file implements all light related functionality,
 * including methods from other claases which are related
 * to lightning
*/

#include "IritObjects.h"
#include "IritLight.h"
#include "Light.h"

// ============================================ General functions ================================================================

RGBQUAD vector_to_rgbq(Vector &vector)
{
	BYTE red = (BYTE)vector[0];
	BYTE green = (BYTE)vector[1];
	BYTE blue = (BYTE)vector[2];

	return { red, green, blue, 0 };
}

/* Calculate ambient, diffusive, and specular light of a point */
Vector calculate_point_shading(Vector &point_pos, Vector &point_normal, struct State &state)
{
	/* General */
	Vector camera_position = Vector(0, 0, 2);

	/* Ambient */
	Vector ambient_intensity(state.m_ambientLight.colorR, state.m_ambientLight.colorG, state.m_ambientLight.colorB);
	double ka = state.m_lMaterialAmbient;

	/* Diffusive */
	double kd = state.m_lMaterialDiffuse;

	/* Specular */
	double ks = state.m_lMaterialSpecular;
	int cosine_factor = state.m_nMaterialCosineFactor;

	Vector overall_lighting = ambient_intensity * ka;

	/* Positional vectors */
	Vector point_to_light;
	Vector light_to_point;
	Vector point_to_eye;
	Vector reflected_vector;


	point_normal.Normalize();

	point_to_eye = camera_position - point_pos;
	point_to_eye.Normalize();

	for (int light_id = LIGHT_ID_1; light_id < MAX_LIGHT; light_id++) {

		LightParams &light = state.m_lights[light_id];

		if (!light.enabled)
			continue;

		Vector light_source_pos(light.posX, light.posY, light.posZ);
		Vector point_source_intensity(light.colorR, light.colorG, light.colorB);

		point_to_light = light_source_pos - point_pos;
		light_to_point = point_to_light * (-1);

		reflected_vector = light_to_point - (point_normal * (point_normal * light_to_point) * 2);

		point_to_light.Normalize();
		reflected_vector.Normalize();
		light_to_point.Normalize();

		double cos_theta = point_normal * point_to_light;
		if (cos_theta > 0) {

			Vector diffusive_light = point_source_intensity * kd * cos_theta;
			overall_lighting += diffusive_light;

			double alpha = reflected_vector * point_to_eye;
			if (alpha > 0) {
				Vector specular_light = point_source_intensity * ks * (pow(alpha, cosine_factor));
				overall_lighting += specular_light;
			}
		}
	}
	return overall_lighting;
}

// ============================================ PHONG Shading ================================================================

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
	Vector &left_x_normal = containing_line->p1.normal;
	Vector &right_x_normal = containing_line->p2.normal;
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

void get_point_normal_and_position(struct IntersectionPoint &intersecting_p, Vector &extrapolated_normal, Vector &extrapolated_pos) {
	// extrapolate the normals and position of the containing lines boundries
	if (!intersecting_p.point_normal || !intersecting_p.point_pos) {
		extrapolate_normal_and_vertex(intersecting_p, extrapolated_normal, extrapolated_pos);
		intersecting_p.point_normal = new Vector(extrapolated_normal);
		intersecting_p.point_pos = new Vector(extrapolated_pos);
	}
	else {
		// We already calculated them in the past
		extrapolated_normal = *intersecting_p.point_normal;
		extrapolated_pos = *intersecting_p.point_pos;
	}
}

/* See http://www.ogldev.org/www/tutorial19/tutorial19.html for reflection calculations */
Vector calculatePhongLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
						   double t, struct State &state)
{

	/* Positional vectors */
	Vector left_side_normal;
	Vector left_side_pos;

	Vector right_side_normal;
	Vector right_side_pos;

	Vector point_normal;
	Vector point_pos;

	get_point_normal_and_position(intersecting_x1, left_side_normal, left_side_pos);

	get_point_normal_and_position(intersecting_x2, right_side_normal, right_side_pos);

	// The 3D-normal and the 3D-position of our intersection point
	point_normal = (left_side_normal * (1 - t)) + (right_side_normal * t);
	point_pos = (left_side_pos * (1 - t)) + (right_side_pos * t);

	return calculate_point_shading(point_pos, point_normal, state);

}

// ============================================ GOURAUD Shading ================================================================

void get_intersection_point_shading(struct IntersectionPoint &intersecting_p, Vector &shading, struct State &state)
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
	Vector &left_x_vertex = containing_line->p1.vertex;
	Vector &right_x_vertex = containing_line->p2.vertex;

	Vector left_shading = calculate_point_shading(left_x_vertex, left_x_normal, state);
	Vector right_shading = calculate_point_shading(right_x_vertex, right_x_normal, state);

	// We find which axis has more difference between its two boundries, to
	// get a more fine-grained normal change
	if (abs(right_2d_x - left_2d_x) > abs(lower_2d_x - upper_2d_x))
		t = (intersecting_2d_x - left_2d_x) / (right_2d_x - left_2d_x);
	else
		t = (intersecting_2d_y - lower_2d_x) / (upper_2d_x - lower_2d_x);

	if (t < 0 || t > 1)
		printf("STOP\n");

	shading = (left_shading * (1 - t)) + (right_shading	 * t);
}

Vector calculateGouraudLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
							 double t, struct State &state)
{
	/* Positional vectors */
	Vector left_side_shading;
	Vector right_side_shading;

	get_intersection_point_shading(intersecting_x1, left_side_shading, state);
	get_intersection_point_shading(intersecting_x2, right_side_shading, state);


	return (left_side_shading * (1 - t)) + (right_side_shading * t);
}

Vector calculateLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
					  double t, struct State &state)
{
	Vector returned_color(0);

	if (state.shading_mode == SHADING_M_PHONG)
		returned_color = calculatePhongLight(intersecting_x1, intersecting_x2, t, state);
	else if(state.shading_mode == SHADING_M_GOURAUD) {
		returned_color = calculateGouraudLight(intersecting_x1, intersecting_x2, t, state);
	}

	if (returned_color[0] > 128 || returned_color[1] > 128 || returned_color[2] > 128)
		printf("Here starts trouble\n");

	return returned_color;
}