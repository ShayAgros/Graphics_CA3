#include "IritObjects.h"
#include "aux_functions.h"
#include "PngWrapper.h"
#include "iritSkel.h"

Matrix createTranslationMatrix(double x, double y, double z = 0);
Matrix createTranslationMatrix(Vector &v);

bool isSilhouette(State state, Matrix &vertex_transform, IritPoint *first, IritPoint *second);
void lineDraw(State state, double alpha, int width, int height, RGBQUAD color, Vector first, Vector second);

#define BOX_NUM_OF_VERTICES 8
#define RELEVANT_NORMAL(x) ((state.use_calc_normals) ? x->normal_calc : x->normal_irit)

VertexList *object_vertex_list;
PolygonList *object_polygon_list;

IritPolygon::IritPolygon() : m_point_nr(0), m_points(nullptr), center_of_mass(Vector(0, 0, 0, 1)),
			normal_irit(Vector(0, 0, 0, 1)), normal_calc(Vector(0,0,0,1)), m_next_polygon(nullptr), max_U(0), max_V(0) {
}

IritPolygon::~IritPolygon() {
	struct IritPoint *next_point;
	while (m_points) {
		next_point = m_points->next_point;
		delete m_points;
		m_points = next_point;
	}
}

bool IritPolygon::addPoint(struct IritPoint &point) {

	IritPoint *new_point = new IritPoint(point);
	if (!new_point)
		return false;
	new_point->next_point = nullptr;

	m_point_nr++;

	if (!m_points) {
		// first point
		m_points = new_point;
		return true;
	}

	IritPoint *last_point = m_points;
	while (last_point->next_point)
		last_point = last_point->next_point;
	last_point->next_point = new_point;

	return true;
}

bool IritPolygon::addPoint(double &x, double &y, double &z, double &normal_irit_x, double &normal_irit_y,
	double &normal_irit_z, double &normal_calc_x, double &normal_calc_y,
	double &normal_calc_z) {
	IritPoint new_point;
	new_point.vertex = Vector(x, y, z, 1);
	new_point.normal_irit = Vector(normal_irit_x, normal_irit_y, normal_irit_z, 1);
	new_point.normal_calc = Vector(normal_calc_x, normal_calc_y, normal_calc_z, 1);

	return addPoint(new_point);
}

bool IritPolygon::addPoint(IPVertexStruct *vertex, Vector normal_calc) {
	IritPoint new_point;
	float *UV;

	for (int i = 0; i < 3; i++) {
		new_point.vertex[i] = vertex->Coord[i];
		new_point.normal_irit[i] = vertex->Normal[i];
		new_point.normal_calc[i] = normal_calc[i];
	}

	// Homogeneous component
	new_point.vertex[3] = 1;
	new_point.normal_irit[3] = 1;
	new_point.normal_calc[3] = 1;

	/* Get vertex UV information */
	if ((UV = AttrGetUVAttrib(vertex->Attr, "uvvals")) != NULL) {
		new_point.U = UV[0];
		new_point.V = UV[1];

		if (!this->m_point_nr) {
			this->max_U = this->min_U = UV[0];
			this->max_V = this->min_V = UV[1];
		}
		else {
			this->max_U = max(this->max_U, UV[0]);
			this->max_V = max(this->max_V, UV[1]);
			this->min_U = min(this->min_U, UV[0]);
			this->min_V = min(this->min_V, UV[1]);
		}
	}

	return addPoint(new_point);
}

IritPolygon *IritPolygon::getNextPolygon() {
	return m_next_polygon;
}

void IritPolygon::setNextPolygon(IritPolygon *polygon) {
	m_next_polygon = polygon;
}

bool isOutsideClippingBoundries(Vector &vertex)
{	
	return vertex[Z_AXIS] > 1 ||vertex[Z_AXIS] < -1;
	// Dont get too close, but you can be as far as you want, so we can see fog effects and whatnot
	return vertex[X_AXIS] > 1 || vertex[X_AXIS] < -1 || vertex[Y_AXIS] > 1 || vertex[Y_AXIS] < -1 ||
		   vertex[Z_AXIS] > 1 /* ||vertex[Z_AXIS] < -1 */;
}

void set_line_normals(threed_line &line, IritPoint *current_point, IritPoint *next_point, Vector &current_vertex,
	Vector &next_vertex, Matrix &vertex_transform, int &sign, struct State &state)
{
	line.p1.normal_irit = vertex_transform * (RELEVANT_NORMAL(current_point) * sign + current_point->vertex);
	line.p1.normal_irit.Homogenize();
	line.p1.normal_irit = line.p1.normal_irit - current_vertex;
	line.p2.normal_irit = vertex_transform * (RELEVANT_NORMAL(next_point) * sign + next_point->vertex);
	line.p2.normal_irit.Homogenize();
	line.p2.normal_irit = line.p2.normal_irit - next_vertex;
}

Vector IritPolygon::transformPolygonNormal(Matrix &transformation, struct State &state, int &sign)
{
	Vector vertex;
	Vector normal;

	vertex = transformation * center_of_mass;
	normal = transformation * (center_of_mass + RELEVANT_NORMAL(this) * sign * 0.3);

	if (state.is_perspective_view) {
		vertex.Homogenize();
		normal.Homogenize();
	}

	return normal - vertex;
}

/* This function uses an algorithm to find intersection between
 * two lines. For more info see
 * https://www.geeksforgeeks.org/program-for-point-of-intersection-of-two-lines/
*/
void IritPolygon::paintPolygon(int width, int height, RGBQUAD color, State &state,
							   Vector &polygon_normal, Vector p_center_of_mass, double alpha) {

	int y_max = (int)(state.screen_mat * Vector(0, 1, 0, 1))[Y_AXIS];
	double A1, B1, C1; // We represent our lines by A1*X + B1*Y = C1
	double A2, B2, C2;
	double determinant;
	RGBQUAD new_color;

	struct IntersectionPoint *intersecting_x;
	int intersecting_x_nr;
	int min_x, max_x;
	int min_y, max_y;

	if (!lines_nr)
		return;

	mergeSort(lines, lines_nr);

	intersecting_x = new struct IntersectionPoint[lines_nr * 2];
	min_x = min(lines[0].x1, lines[0].x2);
	max_x = min(lines[0].x1, lines[0].x2);

	min_y = lines[0].ymin();
	max_y = lines[0].ymax();
	for (int line_ix = 0; line_ix < lines_nr; line_ix++)
		max_y = max(max_y, lines[line_ix].ymax());

	for (int y = min_y; y < max_y; y++) {
		intersecting_x_nr = 0;
		for (int line_ix = 0; line_ix < lines_nr; line_ix++) {
			struct threed_line &current_line = lines[line_ix];
			// Does this line start before our y value
			if (y < current_line.ymin())
				break;
			// Does this line end before our y value
			if (current_line.ymax() < y)
				continue;

			// Set coefficients of the first line
			A1 = current_line.y2 - current_line.y1;
			B1 = current_line.x1 - current_line.x2;
			C1 = A1 * current_line.x1 + B1 * current_line.y1;

			// Set coefficients of the second line, f(x) = y
			A2 = 0;
			B2 = 1;
			C2 = y;

			determinant = A1 * B2 - A2 * B1;
			if (determinant == 0) {

				// Lines are parallel, we add X boundries as intersection point
				intersecting_x[intersecting_x_nr].x = current_line.x1;
				intersecting_x[intersecting_x_nr].y = y;
				intersecting_x[intersecting_x_nr].point_pos = NULL;
				intersecting_x[intersecting_x_nr].point_normal = NULL;
				intersecting_x[intersecting_x_nr].containing_line = &current_line;
				intersecting_x[intersecting_x_nr].z = current_line.z1;
				calculate_normals_and_shading(intersecting_x[intersecting_x_nr], state, polygon_normal, p_center_of_mass);
				this->calculate_2d_intersection_texture(intersecting_x[intersecting_x_nr], state);
				intersecting_x_nr++;
				intersecting_x[intersecting_x_nr].x = current_line.x2;
				intersecting_x[intersecting_x_nr].y = y;
				intersecting_x[intersecting_x_nr].point_pos = NULL;
				intersecting_x[intersecting_x_nr].point_normal = NULL;
				intersecting_x[intersecting_x_nr].containing_line = &current_line;
				intersecting_x[intersecting_x_nr].z = current_line.z2;
				calculate_normals_and_shading(intersecting_x[intersecting_x_nr], state, polygon_normal, p_center_of_mass);
				this->calculate_2d_intersection_texture(intersecting_x[intersecting_x_nr], state);
				intersecting_x_nr++;

				min_x = min(min_x, min(current_line.x1, current_line.x2));
				max_x = max(max_x, max(current_line.x1, current_line.x2));
			}
			else {
				double t;
				double extrapolated_z;
				intersecting_x[intersecting_x_nr].x = (int)((B2 * C1 - B1 * C2) / determinant);
				intersecting_x[intersecting_x_nr].y = y;
				intersecting_x[intersecting_x_nr].point_pos = NULL;
				intersecting_x[intersecting_x_nr].point_normal = NULL;
				intersecting_x[intersecting_x_nr].containing_line = &current_line;
				calculate_normals_and_shading(intersecting_x[intersecting_x_nr], state, polygon_normal, p_center_of_mass);
				this->calculate_2d_intersection_texture(intersecting_x[intersecting_x_nr], state);

				if (current_line.x2 == current_line.x1) {
					t = (double)(intersecting_x[intersecting_x_nr].y - current_line.y1) / (double)(current_line.y2 - current_line.y1);
				}
				else {
					t = (double)(intersecting_x[intersecting_x_nr].x - current_line.x1) / (double)(current_line.x2 - current_line.x1);
				}
				extrapolated_z = (1.0 - t) * current_line.z1 + t * current_line.z2;

				intersecting_x[intersecting_x_nr].z = extrapolated_z;

				min_x = min(min_x, intersecting_x[intersecting_x_nr].x);
				max_x = max(max_x, intersecting_x[intersecting_x_nr].x);

				intersecting_x_nr++;
			}
		}

		// nothing to draw for this y value
		if (!intersecting_x_nr)
			continue;

		// sort pixels in increasing order
		bucketSortAndUnique(intersecting_x, intersecting_x_nr, min_x, max_x);

		// Now we finally draw the pixels for each two adjacent x values
		for (int i = 0; i < intersecting_x_nr - 1; i += 2) {

			double t = 0;
			double t_step = 1.0 / (intersecting_x[i + 1].x - intersecting_x[i].x + 1);

			// paint all the pixels between the two adjact x values (inclusive)
			for (int x = intersecting_x[i].x; x <= intersecting_x[i + 1].x; x++) {
				PixelNode *new_node;
				PixelNode *current_node, *next_node;
				double extrapolated_z;
				extrapolated_z = (1 - t) * intersecting_x[i].z + t * intersecting_x[i + 1].z;
				t += t_step;

				Vector light_color = calculateLight(intersecting_x[i], intersecting_x[i + 1], t, state);

				// UV 2-D mapping
				/* Does polygon have UV representation ? */
				if (this->max_U != 0 || this->max_V != 0) {
					Vector p_texture = this->get2DPixelTexture(intersecting_x[i], intersecting_x[i + 1], t, state);

					// RGBQUAD format is  <B G R A>
					light_color += p_texture;
				}

				unsigned int new_red_c = min(light_color[0], 255);
				unsigned int new_green_c = min(light_color[1], 255);
				unsigned int new_blue_c = min(light_color[2], 255);

				if (state.fog) {
					// The further we are from the camera, the color should be more fog	
					double fog_t = CLAMP((extrapolated_z - FOG_END) / (FOG_START - FOG_END));

					new_red_c = fog_t * new_red_c + (1 - fog_t) * RED(*((int*)&state.fog_color));
					new_green_c = fog_t * new_green_c + (1 - fog_t) * GREEN(*((int*)&state.fog_color));
					new_blue_c = fog_t * new_blue_c + (1 - fog_t) * BLUE(*((int*)&state.fog_color));
				}

				// RGBQUAD format is  <B G R A>
				new_color = { (BYTE)new_blue_c, (BYTE)new_green_c, (BYTE)new_red_c, (BYTE)light_color[3] };

				if (state.transparency) {
					current_node = state.z_buffer[y * width + x];
					while (current_node != nullptr) { // Test to see if were painting close to an exisiting node
						if (abs(current_node->depth - extrapolated_z) < (10 * EPSILON))
							break;
						current_node = current_node->next;
					}
					if (current_node != nullptr && abs(current_node->depth - extrapolated_z) < (10 * EPSILON))
						continue; // Skip this pixel

					new_node = new PixelNode;
					new_node->depth = extrapolated_z;
					new_node->alpha = alpha;
					new_node->color = *((int*)&new_color);
					new_node->next = nullptr;

					// Check if should be entered first
					if (state.z_buffer[y * width + x]->depth < new_node->depth) {
						new_node->next = state.z_buffer[y * width + x];
						state.z_buffer[y * width + x] = new_node;
					} else {
						current_node = state.z_buffer[y * width + x];
						next_node = current_node->next;

						while (next_node != nullptr && next_node->depth >= new_node->depth) {
							current_node = current_node->next;
							next_node = next_node->next;
						}
						// End of list
						if (next_node == nullptr) {
							current_node->next = new_node;
						} else {
							new_node->next = next_node;
							current_node->next = new_node;
						}
					}
				} else {
					bool closer = extrapolated_z > state.z_buffer[y * width + x]->depth;

					if (closer) {
						state.z_buffer[y * width + x]->color = *((int*)&new_color);
						//bitmap[y * width + x] = *((int*)&color);
						state.z_buffer[y * width + x]->depth = extrapolated_z;
					}
				}
			}
		}
	}
	delete[] intersecting_x;
}

void IritPolygon::draw(int width, int height, RGBQUAD color, struct State &state,
					   Matrix &vertex_transform, double alpha) {
	struct IritPoint *current_point = m_points;
	Vector current_vertex = current_point->vertex;
	Vector next_vertex;
	Vector backface[2];
	Vector polygon_normal[2];
	RGBQUAD current_color = (state.is_default_color) ? color : state.wire_color;
	struct threed_line line;
	struct threed_line *prev_line = NULL;
	bool is_silhouette;

	// Used to invert normals if needed
	int sign = (state.invert_normals) ? -1 : 1;

	// Check backface culling
	backface[0] = vertex_transform * ((RELEVANT_NORMAL(this) + this->center_of_mass) * sign);
	backface[1] = (vertex_transform * this->center_of_mass);
	if (state.is_perspective_view) {
		backface[0].Homogenize();
		backface[1].Homogenize();
	}
	backface[0] = backface[0] - backface[1];
	if (state.backface_culling && backface[0][Z_AXIS] <= 0) {
		return;
	}

	// For vertex normal drawing
	Vector normal;

	// For figure painting

	// We assume that number of lines is at most the number of points
	lines = new struct threed_line[m_point_nr];
	// we count the exact number of lines when drawing the wireframe
	lines_nr = 0;

	/* Draw shape's wireframe and find lines */
	while (current_point->next_point != nullptr) {
		current_vertex = current_point->vertex;
		next_vertex = current_point->next_point->vertex;
		current_vertex = vertex_transform * current_vertex;
		next_vertex = vertex_transform * next_vertex;

		if (state.is_perspective_view) {
			current_vertex.Homogenize();
			next_vertex.Homogenize();
		}

		// Clipping. Don't draw points that are outside of our view volume
		if (isOutsideClippingBoundries(current_vertex) || isOutsideClippingBoundries(next_vertex)) {
			goto pass_this_point;
		}

		set_line_normals(line, current_point, current_point->next_point, current_vertex, next_vertex,
						 vertex_transform, sign, state);

		line.p1.vertex = current_vertex;
		line.p2.vertex = next_vertex;

		line.p1.U = current_point->U;
		line.p1.V = current_point->V;
		line.p2.U = current_point->next_point->U;
		line.p2.V = current_point->next_point->V;

		current_vertex = state.screen_mat * current_vertex;
		next_vertex = state.screen_mat * next_vertex;

		/* Don't draw mesh lines if we're painting the object as well */
		if (state.only_mesh)
			lineDraw(state, alpha, width, height, current_color, current_vertex, next_vertex);

		if (state.show_silhouette) {
			is_silhouette = isSilhouette(state, vertex_transform, current_point, current_point->next_point);

			if (is_silhouette) {
				Vector first_silhouette = current_vertex,
					second_silhouette = next_vertex;

				first_silhouette[Z_AXIS] = first_silhouette[Z_AXIS] + EPSILON;
				second_silhouette[Z_AXIS] = second_silhouette[Z_AXIS] + EPSILON;

				lineDraw(state, alpha, width, height, SILHOUETTE_DEFAULT_COLOR,
					first_silhouette, second_silhouette);
			}
		}

		// Add the drawn line to the list of lines on the screen
		line.x1 = (int)current_vertex.coordinates[X_AXIS];
		line.y1 = (int)current_vertex.coordinates[Y_AXIS];
		line.x2 = (int)next_vertex.coordinates[X_AXIS];
		line.y2 = (int)next_vertex.coordinates[Y_AXIS];

		line.z1 = (double)current_vertex.coordinates[Z_AXIS];
		line.z2 = (double)next_vertex.coordinates[Z_AXIS];

		if (state.show_vertex_normal) {

			// Shrink normal to reduce cluttering, and invert if needed
			normal = RELEVANT_NORMAL(current_point) * 0.3 * sign;

			// Place normal in space
			normal += current_point->vertex;
			normal = vertex_transform * normal;

			if (state.is_perspective_view)
				normal.Homogenize();
			normal = state.screen_mat * normal;
			lineDraw(state, alpha, width, height, NORMAL_DEFAULT_COLOR, current_vertex, normal);
		}

		if (line.x1 != line.x2 || line.y1 != line.y2) {
			// we ignore lines that only change in their z value since
			// its won't be a line in 2d space
			lines[lines_nr] = line;
			prev_line = &lines[lines_nr];
			lines_nr++;
		}
pass_this_point:
		current_point = current_point->next_point;
	}

	if (state.show_polygon_normal) {
		polygon_normal[0] = vertex_transform * center_of_mass;
		polygon_normal[1] = vertex_transform * (center_of_mass + RELEVANT_NORMAL(this) * sign * 0.3);

		if (state.is_perspective_view) {
			polygon_normal[0].Homogenize();
			polygon_normal[1].Homogenize();
		}

		polygon_normal[0] = state.screen_mat * polygon_normal[0];
		polygon_normal[1] = state.screen_mat * polygon_normal[1];
		
		lineDraw(state, alpha, width, height, NORMAL_DEFAULT_COLOR, polygon_normal[0], polygon_normal[1]);
	}

	// close the figure by drawing line from last point to the first
	current_vertex = current_point->vertex;
	next_vertex = m_points->vertex;
	current_vertex = vertex_transform * current_vertex;
	next_vertex = vertex_transform * next_vertex;

	if (state.is_perspective_view) {
		current_vertex.Homogenize();
		next_vertex.Homogenize();
	}

	// Clipping. Don't draw points that are outside of our view volume
	if (isOutsideClippingBoundries(current_vertex) || isOutsideClippingBoundries(next_vertex)) {
		goto paint_polygon;
	}

	line.p1.vertex = current_vertex;
	line.p2.vertex = next_vertex;

	line.p1.U = current_point->U;
	line.p1.V = current_point->V;
	line.p2.U = m_points->U;
	line.p2.V = m_points->V;

	set_line_normals(line, current_point, m_points, current_vertex, next_vertex,
					 vertex_transform, sign, state);

	current_vertex = state.screen_mat * current_vertex;
	next_vertex = state.screen_mat * next_vertex;

	/* Don't draw mesh lines if we're painting the object as well */
	if (state.only_mesh)
		lineDraw(state, alpha, width, height, current_color, current_vertex, next_vertex);

	if (state.show_silhouette) {
		is_silhouette = isSilhouette(state, vertex_transform, current_point, m_points);

		if (is_silhouette) {
			Vector first_silhouette = current_vertex,
				second_silhouette = next_vertex;

			first_silhouette[Z_AXIS] = first_silhouette[Z_AXIS] + EPSILON;
			second_silhouette[Z_AXIS] = second_silhouette[Z_AXIS] + EPSILON;

			lineDraw(state, alpha, width, height, SILHOUETTE_DEFAULT_COLOR,
				first_silhouette, second_silhouette);
		}
	}

	// Add the drawn line to the list of lines on the screen
	line.x1 = (int)current_vertex.coordinates[X_AXIS];
	line.y1 = (int)(int)current_vertex.coordinates[Y_AXIS];
	line.x2 = (int)next_vertex.coordinates[X_AXIS];
	line.y2 = (int)next_vertex.coordinates[Y_AXIS];

	line.z1 = current_vertex.coordinates[Z_AXIS];
	line.z2 = (double)next_vertex.coordinates[Z_AXIS];

	normal = RELEVANT_NORMAL(current_point) * 0.3;
	normal += current_point->vertex;
	normal = vertex_transform * normal;
	if (state.is_perspective_view)
		normal.Homogenize();

	if (line.x1 != line.x2 || line.y1 != line.y2) {
		// we ignore lines that only change in their z value since
		// its won't be a line in 2d space
		lines[lines_nr++] = line;
	}
paint_polygon:
	// paint the polygon
	if (!state.only_mesh) {
		polygon_normal[0] = transformPolygonNormal(vertex_transform, state, sign);
		paintPolygon(width, height, current_color, state, polygon_normal[0],
					 vertex_transform * center_of_mass, alpha);
	}
	delete[] lines;
}

int IritPolygon::getPointsNr() {
	return m_point_nr;
}

IritPolygon &IritPolygon::operator++() {
	return *m_next_polygon;
}

IritObject::IritObject() : m_polygons_nr(0), m_polygons(nullptr), m_iterator(nullptr) {
	object_color = WIRE_DEFAULT_COLOR;
	alpha = 1; // Completely Opaque.
}

IritObject::~IritObject() {
	IritPolygon *next_polygon;
	while (m_polygons) {
		next_polygon = m_polygons->getNextPolygon();
		delete m_polygons;
		m_polygons = next_polygon;
	}
}

void IritObject::addPolygonP(IritPolygon *polygon) {

	if (!m_polygons) {
		m_polygons = polygon;
	} else {
		if (!m_iterator)
			m_iterator = m_polygons;

		while (m_iterator->getNextPolygon())
			m_iterator = m_iterator->getNextPolygon();
		m_iterator->setNextPolygon(polygon);
	}
	m_polygons_nr++;
}

IritPolygon *IritObject::createPolygon() {
	IritPolygon *new_polygon = new IritPolygon();
	if (!new_polygon)
		return nullptr;
	addPolygonP(new_polygon);
	return new_polygon;
}

void IritObject::draw(int width, int height, State &state,
					  Matrix &vertex_transform, double figure_alpha, bool global_alpha) {
	double current_alpha = (global_alpha) ? figure_alpha : this->alpha;
	// set the current connectivity lists
	object_vertex_list = this->vertex_connection; 
	object_polygon_list = this->polygon_connection;

	int points_nr = 0;
	m_iterator = m_polygons;

	m_iterator = m_polygons;
	while (m_iterator) {
		m_iterator->draw(width, height, object_color, state, vertex_transform, current_alpha);
		m_iterator = m_iterator->getNextPolygon();
	}
}

void IritObject::getMaxMinUV(double &max_u, double &max_v, double &min_u, double &min_v)
{
	m_iterator = m_polygons;
	while (m_iterator) {
		IritPolygon *polygon = m_iterator;

		if (max_u == max_v && max_u == min_u && max_u == min_v && max_u == 0) {
			max_u = polygon->max_U;
			max_v = polygon->max_V;
			min_u = polygon->min_U;
			min_v = polygon->min_V;
		}
		else {
			max_u = max(polygon->max_U, max_u);
			max_v = max(polygon->max_V, max_v);
			min_u = min(polygon->min_U, min_u);
			min_v = min(polygon->min_V, min_v);
		}

		m_iterator = m_iterator->getNextPolygon();
	}
}


IritObject *IritFigure::createObject() {
	IritObject *new_object = new IritObject();
	if (!new_object)
		return nullptr;

	if (!addObjectP(new_object))
		return nullptr;

	return new_object;
}

bool IritFigure::addObjectP(IritObject *p_object) {

	IritObject **new_objects_array = new IritObject*[m_objects_nr + 1];
	if (!new_objects_array)
		return false;

	for (int i = 0; i < m_objects_nr; i++)
		new_objects_array[i] = m_objects_arr[i];
	new_objects_array[m_objects_nr] = p_object;

	delete m_objects_arr;
	m_objects_arr = new_objects_array;
	m_objects_nr++;

	return true;
}

void IritFigure::draw(int width, int height, Matrix transform, State &state) {
	double ratio = (max_bound_coord[Y_AXIS] - min_bound_coord[Y_AXIS]) / (max_bound_coord[X_AXIS] - min_bound_coord[X_AXIS]);
	Matrix fix_ortho_ratio = Matrix::createScaleMatrix(1, ratio, 1);
	Matrix shrink = Matrix::createScaleMatrix((double)(1) / 2, (double)(1) / 2, (double)(1) / 2);

	Matrix vertex_transform = transform * shrink * normalization_mat * world_mat * object_mat * fix_ortho_ratio;

	state.max_u = state.max_v = state.min_u = state.min_v = 0;

	for (int i = 0; i < m_objects_nr; i++) {
		IritObject * object = m_objects_arr[i];
		
		object->getMaxMinUV(state.max_u, state.max_v, state.min_u, state.min_v);
	}

	// Draw all objects
	for (int i = 0; i < m_objects_nr; i++)
		m_objects_arr[i]->draw(width, height, state, vertex_transform, alpha, has_global_alpha);

	// Draw a frame around all objects
	if (state.object_frame)
		drawFrame(width, height, state, vertex_transform);
}

void IritFigure::drawFrame(int width, int height, struct State state, Matrix &transform) {
	double frame_max_x = max_bound_coord[0],
		frame_max_y = max_bound_coord[1],
		frame_max_z = max_bound_coord[2],
		frame_min_x = min_bound_coord[0],
		frame_min_y = min_bound_coord[1],
		frame_min_z = min_bound_coord[2];

	Vector coords[BOX_NUM_OF_VERTICES] = {
		Vector(frame_max_x, frame_max_y, frame_max_z, 1), // Front top right
		Vector(frame_max_x, frame_min_y, frame_max_z, 1), // Front bottom right
		Vector(frame_min_x, frame_max_y, frame_max_z, 1), // Front top left
		Vector(frame_min_x, frame_min_y, frame_max_z, 1), // Front bottom left
		Vector(frame_max_x, frame_max_y, frame_min_z, 1), // Back top right
		Vector(frame_max_x, frame_min_y, frame_min_z, 1), // Back bottom right
		Vector(frame_min_x, frame_max_y, frame_min_z, 1), // Back top left
		Vector(frame_min_x, frame_min_y, frame_min_z, 1)  // Back bottom left
	};

	// Update box to current transformation
	for (int i = 0; i < BOX_NUM_OF_VERTICES; i++) {
		coords[i] = transform * coords[i];

		if (state.is_perspective_view)
			coords[i].Homogenize();

		coords[i] = state.screen_mat * coords[i];
	}
	// Frame alpha should be 1, so it's always visible.

	// Draw "front side"

	lineDraw(state, 1.0, width, height, state.frame_color, coords[0], coords[1]);
	lineDraw(state, 1.0, width, height, state.frame_color, coords[1], coords[3]);
	lineDraw(state, 1.0, width, height, state.frame_color, coords[3], coords[2]);
	lineDraw(state, 1.0, width, height, state.frame_color, coords[2], coords[0]);

	// Draw "back side"

	lineDraw(state, 1.0, width, height, state.frame_color, coords[4], coords[5]);
	lineDraw(state, 1.0, width, height, state.frame_color, coords[5], coords[7]);
	lineDraw(state, 1.0, width, height, state.frame_color, coords[7], coords[6]);
	lineDraw(state, 1.0, width, height, state.frame_color, coords[6], coords[4]);

	// Draw "sides"

	// Top right
	lineDraw(state, 1.0, width, height, state.frame_color, coords[0], coords[4]);
	// Bottom right
	lineDraw(state, 1.0, width, height, state.frame_color, coords[1], coords[5]);
	// Top left
	lineDraw(state, 1.0, width, height, state.frame_color, coords[2], coords[6]);
	// Bottom left
	lineDraw(state, 1.0, width, height, state.frame_color, coords[3], coords[7]);
}

bool IritFigure::isEmpty() {
	return m_objects_nr == 0;
}

void IritFigure::backup_transformation(State &state) {
	if (state.object_transform) {
		backup_transformation_matrix = object_mat;
	} else {
		backup_transformation_matrix = world_mat;
	}
}

IritFigure::IritFigure() : m_objects_nr(0), m_objects_arr(nullptr) {

	alpha = 1.0;
	has_global_alpha = false;

	max_bound_coord = Vector();
	max_bound_coord[3] = 1;
	min_bound_coord = Vector();
	min_bound_coord[3] = 1;

	object_mat = Matrix::Identity();

	// We draw an object in a 1/2 of its size. No need to fill the whole screen
	world_mat = Matrix::Identity();
}

IritFigure::~IritFigure() {
	delete[] m_objects_arr;
}

IritWorld::IritWorld() : m_figures_nr(0), m_figures_arr(nullptr) {
	state.show_vertex_normal = false;
	state.show_polygon_normal = false;
	state.object_frame = false;
	state.is_perspective_view = false;
	state.object_transform = true;
	state.is_default_color = true;
	state.use_calc_normals = false;
	state.invert_normals = false;
	state.backface_culling = false;
	state.only_mesh = false;
	state.save_to_png = false;
	state.show_silhouette = false;
	state.fog = false;
	state.transparency = false;
	state.motion_blur = false;

	background = new PngWrapper();

	for (int i = 0; i < 3; i++)
		state.is_axis_active[i] = false;

	max_bound_coord = Vector();
	max_bound_coord[3] = 1;
	min_bound_coord = Vector();
	min_bound_coord[3] = 1;

	state.coord_mat = Matrix::Identity();
	state.center_mat = Matrix::Identity();
	state.ratio_mat = Matrix::Identity();
	state.world_mat = Matrix::Identity();
	state.object_mat = Matrix::Identity();
	state.ortho_mat = Matrix::Identity();

	state.view_mat = createViewMatrix(DEAULT_VIEW_PARAMETERS);
	state.projection_plane_distance = DEFAULT_PROJECTION_PLANE_DISTANCE;
	state.sensitivity = 1.0;
	state.fineness = DEFAULT_FINENESS;
	state.motion_blur_drag = DEFAULT_MOTION_BLUR;

	state.bg_color = BG_DEFAULT_COLOR;
	state.wire_color = WIRE_DEFAULT_COLOR;
	state.frame_color = FRAME_DEFAULT_COLOR;
	state.normal_color = NORMAL_DEFAULT_COLOR;
	state.fog_color = FOG_DEFAULT_COLOR;

	state.z_buffer = nullptr;
	motion_blur_bitmap = nullptr;

}

IritWorld::~IritWorld() {
	delete[] m_figures_arr; 
}

IritFigure *IritWorld::createFigure() {
	IritFigure *new_figure = new IritFigure();
	if (!new_figure)
		return nullptr;

	if (!addFigureP(new_figure))
		return nullptr;

	return new_figure;
}

bool IritWorld::addFigureP(IritFigure *p_figure) {

	IritFigure **new_figures_array = new IritFigure*[m_figures_nr + 1];
	if (!new_figures_array)
		return false;

	for (int i = 0; i < m_figures_nr; i++)
		new_figures_array[i] = m_figures_arr[i];
	new_figures_array[m_figures_nr] = p_figure;

	delete m_figures_arr;
	m_figures_arr = new_figures_array;
	m_figures_nr++;

	return true;
}

bool IritWorld::isEmpty() {
	return m_figures_nr == 0;
};

Vector IritWorld::projectPoint_in_screen_axes(Vector &td_point, Matrix &transformation) {
	Vector transformed_point = transformation * td_point;
	if (state.is_perspective_view)
		transformed_point.Homogenize();
	return state.screen_mat * Matrix::createScaleMatrix(1, -1, 1) * transformed_point;
}

void IritWorld::draw(int *bitmap, int width, int height) {
		static bool first_frame = true;
		BYTE red, blue, green;
		RGBQUAD motion_blur_color;

		Matrix partial_projection_mat = createProjectionMatrix();
		Matrix projection_mat = partial_projection_mat * state.ortho_mat;
		Matrix transformation = projection_mat * state.view_mat;

		this->state.orthogonal_to_perspective = partial_projection_mat;
		this->state.screen_mat = state.center_mat * state.ratio_mat;

		// Draw all objects
		for (int i = 0; i < m_figures_nr; i++)
			m_figures_arr[i]->draw(width, height, transformation, state);

		// After all drawing is done, calculate final color of each pixel based on transparency
		for (int i = 0; i < width * height; i++) {
			PixelNode* current_node = state.z_buffer[i];
			unsigned int color = recursiveGetColor(current_node);
			bitmap[i] = color;
		}

		// POST EFFECTS SECTION... YAY!

		// Create motion blur
		if (state.motion_blur) {
			double t = state.motion_blur_drag;

			if (first_frame) {
				// First frame, copy the image
				motion_blur_bitmap = new unsigned int[width * height];
				for (int i = 0; i < width* height; i++) {
					motion_blur_bitmap[i] = bitmap[i];
				}
				first_frame = false;
			} else {
				// Any other frame, extrapolate with existing image.
				for (int i = 0; i < width* height; i++) {
					red = t * RED(motion_blur_bitmap[i]) + (1 - t) * RED(bitmap[i]);
					green = t * GREEN(motion_blur_bitmap[i]) + (1 - t) * GREEN(bitmap[i]);
					blue = t * BLUE(motion_blur_bitmap[i]) + (1 - t) * BLUE(bitmap[i]);
					
					// RGBQUAD is <B G R A>
					motion_blur_color = { blue, green, red, 0 };

					// Remember the new frame
					motion_blur_bitmap[i] = *((int*)&motion_blur_color);
					// Update to the new frame
					bitmap[i] = *((int*)&motion_blur_color);
				}
			}
		} else {
			delete(motion_blur_bitmap);
			motion_blur_bitmap = nullptr;
			first_frame = true;
		}
}

IritFigure *IritWorld::getFigureInPoint(CPoint &point) {
	IritFigure *figure;
	Matrix shrink = Matrix::createScaleMatrix((double)(1) / 2, (double)(1) / 2, (double)(1) / 2);
	Matrix projection_mat = createProjectionMatrix() * state.ortho_mat;
	Matrix world_transformation = projection_mat * state.view_mat * shrink;
	Matrix figure_transformation;
	Matrix transformation_mat;

	Vector min_2d_point, max_2d_point;

	int max_x, min_x, max_y, min_y;

	for (int i = 0; i < m_figures_nr; i++) {
		figure = m_figures_arr[i];
		/* The received point assumes that the point is given in a coordinate system in which 
		 * the y value grows down (left-upper corner is (0, 0) ). To match our coordinate system
		   to the point's, we rotate the object by 'coord_mat' */
		figure_transformation = figure->normalization_mat * figure->world_mat * figure->object_mat;
		transformation_mat = world_transformation * figure_transformation;

		min_2d_point = projectPoint_in_screen_axes(figure->min_bound_coord, transformation_mat);
		max_2d_point = projectPoint_in_screen_axes(figure->max_bound_coord, transformation_mat);

		max_x = (int)max(min_2d_point[X_AXIS], max_2d_point[X_AXIS]);
		min_x = (int)min(min_2d_point[X_AXIS], max_2d_point[X_AXIS]);
		max_y = (int)max(min_2d_point[Y_AXIS], max_2d_point[Y_AXIS]);
		min_y = (int)min(min_2d_point[Y_AXIS], max_2d_point[Y_AXIS]);

		if (min_x <= point.x && point.x <= max_x &&
			min_y <= point.y && point.y <= max_y) {
			return figure;
		}
	}
	return NULL;
}

IritFigure &IritWorld::getLastFigure() {
	assert(m_figures_nr > 0);

	return *m_figures_arr[m_figures_nr - 1];
}


void lineDrawOct0(State state, double alpha, int width, int height, RGBQUAD color, Vector first, Vector second) {
	PixelNode *new_node;
	PixelNode *current_node, *next_node;
	int dx = (int)(second[0] - first[0]),
		dy = (int)(second[1] - first[1]),
		error = (2 * dy) - dx,
		x = (int)first[0],
		y = (int)first[1],
		end_x = (int)second[0],
		end_y = (int)second[1];
	double start_z = first[2],
		   end_z = second[2],
		   current_z = first[2],
		   z_step = 1.0 / (end_x - x),
		   t = 0.0;

	while (x != end_x) {
		bool within_bounds = (x >= 0) && (x < width) && (y >= 0) && (y < height);

		if (within_bounds) {
			current_z = (1.0 - t) * start_z + t * end_z;

			if (state.transparency) {
				new_node = new PixelNode;
				new_node->depth = current_z;
				new_node->alpha = alpha;
				new_node->color = *((int*)&color);
				new_node->next = nullptr;

				// Check if should be entered first
				if (state.z_buffer[y * width + x]->depth < new_node->depth) {
					new_node->next = state.z_buffer[y * width + x];
					state.z_buffer[y * width + x] = new_node;
				} else {
					current_node = state.z_buffer[y * width + x];
					next_node = current_node->next;

					while (next_node != nullptr && next_node->depth >= new_node->depth) {
						current_node = current_node->next;
						next_node = next_node->next;
					}
					// End of list
					if (next_node == nullptr) {
						current_node->next = new_node;
					} else {
						new_node->next = next_node;
						current_node->next = new_node;
					}
				}
			} else {
				bool closer = current_z > state.z_buffer[y * width + x]->depth;

				if (closer) {
					state.z_buffer[y * width + x]->color = *((int*)&color);
					state.z_buffer[y * width + x]->depth = current_z;
				}
			}
		}
		if (error > 0) {
			y++;
			x++;
			error += 2 * (dy -  dx); // North-East
		} else {
			x++;
			error += 2 * dy;         // East
		}
		t += z_step;
	}
}

void lineDrawOct1(State state, double alpha, int width, int height, RGBQUAD color, Vector first, Vector second) {
	PixelNode *new_node;
	PixelNode *current_node, *next_node;
	int dx = (int)(second[0] - first[0]),
		dy = (int)(second[1] - first[1]),
		error = dy - (2 * dx),
		x = (int)first[0],
		y = (int)first[1],
		end_x = (int)second[0],
		end_y = (int)second[1];
	double start_z = first[2],
		end_z = second[2],
		current_z = first[2],
		z_step = 1.0 / (end_y - y),
		t = 0.0;

	while (y != end_y) {
		bool within_bounds = (x >= 0) && (x < width) && (y >= 0) && (y < height);

		if (within_bounds) {
			current_z = (1.0 - t) * start_z + t * end_z;

			if (state.transparency) {
				new_node = new PixelNode;
				new_node->depth = current_z;
				new_node->alpha = alpha;
				new_node->color = *((int*)&color);
				new_node->next = nullptr;

				// Check if should be entered first
				if (state.z_buffer[y * width + x]->depth < new_node->depth) {
					new_node->next = state.z_buffer[y * width + x];
					state.z_buffer[y * width + x] = new_node;
				} else {
					current_node = state.z_buffer[y * width + x];
					next_node = current_node->next;

					while (next_node != nullptr && next_node->depth >= new_node->depth) {
						current_node = current_node->next;
						next_node = next_node->next;
					}
					// End of list
					if (next_node == nullptr) {
						current_node->next = new_node;
					} else {
						new_node->next = next_node;
						current_node->next = new_node;
					}
				}
			} else {
				bool closer = current_z > state.z_buffer[y * width + x]->depth;

				if (closer) {
					state.z_buffer[y * width + x]->color = *((int*)&color);
					state.z_buffer[y * width + x]->depth = current_z;
				}
			}
		}
		if (error > 0) {
			y++;
			error += -2 * dx;       // North
		} else {
			y++;
			x++;
			error += 2 * (dy - dx); // North-East
		}
		t += z_step;
	}
}

void lineDrawOct6(State state, double alpha, int width, int height, RGBQUAD color, Vector first, Vector second) {
	PixelNode *new_node;
	PixelNode *current_node, *next_node;
	int dx = (int)(second[0] - first[0]),
		dy = (int)(second[1] - first[1]),
		error = dy + (2 * dx),
		x = (int)first[0],
		y = (int)first[1],
		end_x = (int)second[0],
		end_y = (int)second[1];
	double start_z = first[2],
		end_z = second[2],
		current_z = first[2],
		z_step = 1.0 / (end_y - y),
		t = 0.0;

	while (y != end_y) {
		bool within_bounds = (x >= 0) && (x < width) && (y >= 0) && (y < height);

		if (within_bounds) {
			current_z = (1.0 - t) * start_z + t * end_z;

			if (state.transparency) {
				new_node = new PixelNode;
				new_node->depth = current_z;
				new_node->alpha = alpha;
				new_node->color = *((int*)&color);
				new_node->next = nullptr;

				// Check if should be entered first
				if (state.z_buffer[y * width + x]->depth < new_node->depth) {
					new_node->next = state.z_buffer[y * width + x];
					state.z_buffer[y * width + x] = new_node;
				} else {
					current_node = state.z_buffer[y * width + x];
					next_node = current_node->next;

					while (next_node != nullptr && next_node->depth >= new_node->depth) {
						current_node = current_node->next;
						next_node = next_node->next;
					}
					// End of list
					if (next_node == nullptr) {
						current_node->next = new_node;
					} else {
						new_node->next = next_node;
						current_node->next = new_node;
					}
				}
			} else {
				bool closer = current_z > state.z_buffer[y * width + x]->depth;

				if (closer) {
					state.z_buffer[y * width + x]->color = *((int*)&color);
					state.z_buffer[y * width + x]->depth = current_z;
				}
			}
		}
		if (error > 0) {
			y--;
			x++;
			error += 2 * (dy + dx); // South-East
		} else {
			y--;
			error += 2 * dx;        // South
		}
		t += z_step;
	}
}

void lineDrawOct7(State state, double alpha, int width, int height, RGBQUAD color, Vector first, Vector second) {
	PixelNode *new_node;
	PixelNode *current_node, *next_node;
	int dx = (int)(second[0] - first[0]),
		dy = (int)(second[1] - first[1]),
		error = (2 * dy) + dx,
		x = (int)first[0],
		y = (int)first[1],
		end_x = (int)second[0],
		end_y = (int)second[1];
	double start_z = first[2],
		end_z = second[2],
		current_z = first[2],
		z_step = 1.0 / (end_x - x),
		t = 0.0;

	while (x != end_x) {
		bool within_bounds = (x >= 0) && (x < width) && (y >= 0) && (y < height);

		if (within_bounds) {
			current_z = (1.0 - t) * start_z + t * end_z;

			if (state.transparency) {
				new_node = new PixelNode;
				new_node->depth = current_z;
				new_node->alpha = alpha;
				new_node->color = *((int*)&color);
				new_node->next = nullptr;

				// Check if should be entered first
				if (state.z_buffer[y * width + x]->depth < new_node->depth) {
					new_node->next = state.z_buffer[y * width + x];
					state.z_buffer[y * width + x] = new_node;
				} else {
					current_node = state.z_buffer[y * width + x];
					next_node = current_node->next;

					while (next_node != nullptr && next_node->depth >= new_node->depth) {
						current_node = current_node->next;
						next_node = next_node->next;
					}
					// End of list
					if (next_node == nullptr) {
						current_node->next = new_node;
					} else {
						new_node->next = next_node;
						current_node->next = new_node;
					}
				}
			} else {
				bool closer = current_z > state.z_buffer[y * width + x]->depth;

				if (closer) {
					state.z_buffer[y * width + x]->color = *((int*)&color);
					state.z_buffer[y * width + x]->depth = current_z;
				}
			}
		}
		if (error > 0) {
			x++;
			error += 2 * dy; // East
		} else {
			x++;
			y--;
			error += 2 * (dx + dy); // South-East
		}
		t += z_step;
	}
}

void lineDraw(State state, double alpha, int width, int height, RGBQUAD color, Vector first, Vector second) {
	double delta_x, delta_y, ratio;

	// Handle case where they are vertical
	if (first[0] == second[0]) {
		if (first[1] < second[1]) {
			lineDrawOct1(state, alpha, width, height, color, first, second);
		} else {
			if (first[1] > second[2]) {
				lineDrawOct6(state, alpha, width, height, color, first, second);
			} else {
				return; // They are the same point
			}
		}
	}

	if (first[0] > second[0]) { // Octants 2 3 4 5
		// Just draw the lines the opposite direction, this way we need only 4 drawing functions
		Vector temp = first;
		first = second;
		second = temp;
	}

	// Now we should be in octants 0 1 6 7

	delta_x = second[0] - first[0];
	delta_y = second[1] - first[1];
	ratio = delta_y / delta_x;

	if (ratio >= 1.0) {							   // Octant 1
		lineDrawOct1(state, alpha, width, height, color, first, second);
	} else if ((ratio >= 0.0) && (ratio < 1.0)) {  // Octant 0
		lineDrawOct0(state, alpha, width, height, color, first, second);
	} else if ((ratio >= -1.0) && (ratio < 0.0)) { // Octant 7
		lineDrawOct7(state, alpha, width, height, color, first, second);
	} else {									   // Octant 6
		lineDrawOct6(state, alpha, width, height, color, first, second);
	}
}

bool arePointsEqual(VertexList *vertex, IritPoint *point) {
	for (int i = 0; i < 3; i++) {
		if (vertex->vertex->Coord[i] != point->vertex[i])
			return false;
	}
	return true;
}

bool arePolygonsEqual(PolygonList *first, PolygonList *second) {
	// While technically center_of_mass should be enough,
	// we compare normals just to be on the safe side
	for (int i = 0; i < 4; i++) {
		if (first->polygon->center_of_mass[i] != second->polygon->center_of_mass[i])
			return false;
		if (first->polygon->normal_irit[i] != second->polygon->normal_irit[i])
			return false;
	}
	return true;
}

// Technically, we can keep a list of shared edges to save complexity.
// No time for that though, so i'll skip it for now
bool isSilhouette(State state, Matrix &vertex_transform, IritPoint *first, IritPoint *second) {
	VertexList *first_vertex,
		*second_vertex,
		*current_vertex;
	int num_of_shared_polygons = 0;
	PolygonList *first_list, *second_list, *iterator;
	IritPolygon *first_polygon, *second_polygon;
	Vector first_normal, second_normal, first_center, second_center;

	// Find the vertices in the connectivity list
	current_vertex = object_vertex_list;
	while (!arePointsEqual(current_vertex, first) && current_vertex->next != nullptr) {
		current_vertex = current_vertex->next;
	}
	first_vertex = current_vertex;

	current_vertex = object_vertex_list;
	while (!arePointsEqual(current_vertex, second) && current_vertex->next != nullptr) {
		current_vertex = current_vertex->next;
	}
	second_vertex = current_vertex;

	first_list = first_vertex->polygon_list;
	second_list = second_vertex->polygon_list;

	// 0 Shared polygons is boring
	// 1 Shared polygons means were are a border.
	// 2 Shared polygons means we might be a silhouette
	do {
		iterator = second_list;
		do {
			if (arePolygonsEqual(first_list, iterator)) {
				num_of_shared_polygons++;
				if (num_of_shared_polygons == 1) {
					first_polygon = first_list->polygon;
				} else {
					second_polygon = first_list->polygon;
				}
			}
			iterator = iterator->next;
		} while (iterator != nullptr);
		first_list = first_list->next;
	} while (first_list != nullptr);

	if (num_of_shared_polygons != 2)
		return false;

	// Now we transform the polygons normals
	// no need for sign, because both normals will be inverted, so the result is the same
	first_center = first_polygon->center_of_mass;
	first_normal = first_center + RELEVANT_NORMAL(first_polygon);
	first_center = vertex_transform * first_center;
	first_normal = vertex_transform * first_normal;

	second_center = second_polygon->center_of_mass;
	second_normal = second_center + RELEVANT_NORMAL(second_polygon);
	second_center = vertex_transform * second_center;
	second_normal = vertex_transform * second_normal;

	if (state.is_perspective_view) {
		first_center.Homogenize();
		first_normal.Homogenize();
		second_center.Homogenize();
		second_normal.Homogenize();
	}

	// Return to only normals
	first_normal = first_normal - first_center;
	second_normal = second_normal - second_center;

	// One should be positive and the other negative.
	if (first_normal[Z_AXIS] * second_normal[Z_AXIS] <= 0)
		return true;

	return false;
}

unsigned int recursiveGetColor(PixelNode *node) {
	// If we reached the end, return black.
	if (node == nullptr) {
		return 0;
	}
	// If pixel is comletely opaque, no point in going on.
	if (node->alpha == 1) {
		return node->color;
	}
	unsigned int recursive_color = recursiveGetColor(node->next);
	BYTE red, green, blue;
	RGBQUAD final_color;

	red = RED(node->color) * node->alpha + RED(recursive_color) * (1 - node->alpha);
	green = GREEN(node->color) * node->alpha + GREEN(recursive_color) * (1 - node->alpha);
	blue = BLUE(node->color) * node->alpha + BLUE(recursive_color) * (1 - node->alpha);

	//RGBQUAD is <B G R A>
	final_color = { blue, green, red, 0 };
	return *((int*)&final_color);
}