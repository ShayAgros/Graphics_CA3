/* This file implements all light related functionality,
 * including methods from other claases which are related
 * to lightning
*/

#include "IritObjects.h"
#include "IritLight.h"
#include "Light.h"

void extrapolate_normal_and_vertex(struct IntersectionPoint &intersecting_p, Vector &extrapolated_normal, Vector &extrapolated_pos);
void get_intersection_point_shading(struct IntersectionPoint &intersecting_p, Vector &shading, struct State &state);

// ============================================ General functions ================================================================

/* Calculate ambient, diffusive, and specular light of a point */
Vector calculate_point_shading(Vector &point_pos, Vector &point_normal, struct State &state)
{
	/* General */
	Vector camera_position = Vector(0, 0, 10);

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

	double light_point_angle_cos;
	double spotlight_difference_angle;

	point_normal.Normalize();

	point_to_eye = camera_position - point_pos;
	point_to_eye.Normalize();

	for (int light_id = LIGHT_ID_1; light_id < MAX_LIGHT; light_id++) {

		LightParams &light = state.m_lights[light_id];

		if (!light.enabled)
			continue;

		Vector light_source_pos(light.posX, light.posY, light.posZ);
		Vector light_source_direction(light.dirX, light.dirY, light.dirZ);
		Vector point_source_intensity(light.colorR, light.colorG, light.colorB);

		bool is_light_directional = light.type == LIGHT_TYPE_DIRECTIONAL;

		point_to_light = light_source_pos - point_pos;
		light_to_point = point_pos - light_source_pos;

		point_to_light.Normalize();
		light_to_point.Normalize();
		light_source_direction.Normalize();

		reflected_vector = light_to_point - (point_normal * (point_normal * light_to_point) * 2);

		reflected_vector.Normalize();

		double cos_theta;
		if (!is_light_directional) /* point source or spotlight */
			cos_theta = point_normal * point_to_light;
		else
			cos_theta = point_normal * (light_source_direction * (-1));

		if (cos_theta > 0) {
			/* If Spotlight is enabled this value defines the difuse of the light */
			spotlight_difference_angle = 1;

			if (light.type == LIGHT_TYPE_SPOT) {
				/* calculate the angle between the point-light_source vecotr
				   and the light source direction */
				light_point_angle_cos = light_to_point * light_source_direction;

				/* If the point is outside the "Cone" created by the spotlight, don't
				   light it */
				if (light_point_angle_cos < light.m_cos_spotlight_angle)
					continue;

				spotlight_difference_angle = (light_point_angle_cos - light.m_cos_spotlight_angle) / (1 - light.m_cos_spotlight_angle);
			}

			Vector diffusive_light = point_source_intensity * kd * cos_theta;
			overall_lighting += diffusive_light * spotlight_difference_angle;

			double alpha = reflected_vector * point_to_eye;
			if (alpha > 0 && !is_light_directional) {
				Vector specular_light = point_source_intensity * ks * (pow(alpha, cosine_factor));
				overall_lighting += specular_light * spotlight_difference_angle;
			}

		}
	}
	return overall_lighting;
}

void calculate_normals_and_shading(struct IntersectionPoint &intersection_p, struct State &state,
								   Vector &polygon_normal, Vector &center_of_mass) {
	if (state.shading_mode == SHADING_M_FLAT) {
		intersection_p.polygon_shade = calculate_point_shading(center_of_mass, polygon_normal, state);
	}
	else if (state.shading_mode == SHADING_M_PHONG) {
		extrapolate_normal_and_vertex(intersection_p, intersection_p.point_normal, intersection_p.point_pos);
	}
	else if (state.shading_mode == SHADING_M_GOURAUD) {
		get_intersection_point_shading(intersection_p, intersection_p.point_shade, state);
	}
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
	Vector &left_x_normal = containing_line->p1.normal_irit;
	Vector &right_x_normal = containing_line->p2.normal_irit;
	Vector &left_x_vertex = containing_line->p1.vertex;
	Vector &right_x_vertex = containing_line->p2.vertex;

	// We find which axis has more difference between its two boundries, to
	// get a more fine-grained normal change
	if (abs(right_2d_x - left_2d_x) > abs(lower_2d_x - upper_2d_x))
		t = (intersecting_2d_x - left_2d_x) / (right_2d_x - left_2d_x);
	else
		t = (intersecting_2d_y - lower_2d_x) / (upper_2d_x - lower_2d_x);

	extrapolated_normal = (left_x_normal * (1 - t)) + (right_x_normal * t);
	extrapolated_pos = (left_x_vertex * (1 - t)) + (right_x_vertex * t);
}

/* See http://www.ogldev.org/www/tutorial19/tutorial19.html for reflection calculations */
Vector calculatePhongLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
						   double t, struct State &state)
{
	/* Positional vectors */
	Vector &left_side_normal = intersecting_x1.point_normal;
	Vector &left_side_pos = intersecting_x1.point_pos;

	Vector &right_side_normal = intersecting_x2.point_normal;
	Vector &right_side_pos = intersecting_x2.point_pos;

	Vector point_normal;
	Vector point_pos;

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
	Vector &left_x_normal = containing_line->p1.normal_irit;
	Vector &right_x_normal = containing_line->p2.normal_irit;
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

	shading = (left_shading * (1 - t)) + (right_shading	 * t);
}

Vector calculateGouraudLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
							 double t, struct State &state)
{
	/* Positional vectors */
	Vector left_side_shading;
	Vector right_side_shading;

	return (intersecting_x1.point_shade * (1 - t)) + (intersecting_x2.point_shade * t);
}

// ============================================ Flat Shading ================================================================

Vector calculateFlatLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
						  double t, struct State &state)
{
	return (intersecting_x1.polygon_shade * (1 - t)) + (intersecting_x2.polygon_shade * t);
}


Vector calculateLight(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
					  double t, struct State &state)
{
	Vector returned_color(0);

	if (state.shading_mode == SHADING_M_PHONG)
		returned_color = calculatePhongLight(intersecting_x1, intersecting_x2, t, state);
	else if(state.shading_mode == SHADING_M_GOURAUD) {
		returned_color = calculateGouraudLight(intersecting_x1, intersecting_x2, t, state);
	} else if (state.shading_mode == SHADING_M_FLAT) {
		returned_color = calculateFlatLight(intersecting_x1, intersecting_x2, t, state);
	}

	return returned_color;
}