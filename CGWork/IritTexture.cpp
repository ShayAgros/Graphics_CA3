#include "IritObjects.h"

// ============================================= 2D texture ======================================================

static double normalize(double val, double max, double min)
{
	return (val - min) / (max - min);
}

// calculate U,V values for the intersection point
void IritPolygon::calculate_2d_intersection_texture(struct IntersectionPoint &point, struct State state)
{
	double t;
	double intersecting_2d_x = point.x;
	double intersecting_2d_y = point.y;
	struct threed_line *containing_line = point.containing_line;
	double left_2d_x = containing_line->x1;
	double right_2d_x = containing_line->x2;
	double upper_2d_y = containing_line->y2;
	double lower_2d_y = containing_line->y1;
	double left_U = containing_line->p1.U;
	double right_U = containing_line->p2.U;
	double left_V = containing_line->p1.V;
	double right_V = containing_line->p2.V;

	/* Does polygon have UV representation ? */
	if (this->max_U != 0 || this->min_U != 0) {

		left_U = normalize(left_U, state.max_u, state.min_u);
		right_U = normalize(right_U, state.max_u, state.min_u);
		left_V = normalize(left_V, state.max_v, state.min_v);
		right_V = normalize(right_V, state.max_v, state.min_v);

		/* We find which axis has more difference between its two boundries, to
		   get a more fine-grained normal change */
		if (abs(right_2d_x - left_2d_x) > abs(lower_2d_y - upper_2d_y))
			t = (intersecting_2d_x - left_2d_x) / (right_2d_x - left_2d_x);
		else
			t = (intersecting_2d_y - lower_2d_y) / (upper_2d_y - lower_2d_y);


		point.point_U = (left_U * (1 - t)) + (right_U * t);

		point.point_V = (left_V * (1 - t)) + (right_V * t);

	}
}

Vector IritPolygon::get2DPixelTexture(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
									  double t, struct State &state)
{
	Vector pixel_intensity = { 0, 0, 0, 0 };
	PngWrapper *p = state.texture_png;
	double x, y;
	int i_color;
	BYTE *pixel_c;

	if (p) {
		int png_width = p->GetWidth(),
			png_height = p->GetHeight();
		double left_u = intersecting_x1.point_U;
		double left_v = intersecting_x1.point_V;
		double right_u = intersecting_x2.point_U;
		double right_v = intersecting_x2.point_V;

		x = (left_u * (1 - t)) + (right_u * t);
		y = (left_v * (1 - t)) + (right_v * t);

		x = (int)(x * png_width);
		y = (int)(y * png_height);

		/* Screen is inverted compared to PNG */
		y = png_height - y - 1;

		i_color = RGBA_TO_ARGB(p->GetValue(x, y));
		pixel_c = (BYTE*)&i_color;

		// Output format is <B G R A>
		pixel_intensity[0] = pixel_c[2];
		pixel_intensity[1] = pixel_c[1];
		pixel_intensity[2] = pixel_c[0];
		pixel_intensity[3] = pixel_c[3];

	}

	return pixel_intensity;
}