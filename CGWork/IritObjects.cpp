#include "IritObjects.h"

Matrix createTranslationMatrix(double &x, double &y, double z = 0);
Matrix createTranslationMatrix(Vector &v);
void lineDraw(int *bits, int width, int height, RGBQUAD color, Vector first, Vector second);

#define BOX_NUM_OF_VERTICES 8

IritPolygon::IritPolygon() : m_point_nr(0), m_points(nullptr), normal_start(Vector(0, 0, 0, 1)),
			normal_end(Vector(0, 0, 0, 1)), is_irit_normal(false), m_next_polygon(nullptr) {
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

bool IritPolygon::addPoint(double &x, double &y, double &z, double &normal_x, double &normal_y,
	double &normal_z) {
	IritPoint new_point;
	new_point.vertex = Vector(x, y, z, 1);
	new_point.normal = Vector(normal_x, normal_y, normal_z, 1);

	return addPoint(new_point);
}

bool IritPolygon::addPoint(IPVertexStruct *vertex, bool is_irit_normal, Vector normal) {
	IritPoint new_point;
	new_point.is_irit_normal = is_irit_normal;

	for (int i = 0; i < 3; i++) {
		new_point.vertex[i] = vertex->Coord[i];

		if (is_irit_normal) {
			new_point.normal[i] = vertex->Normal[i];
		} else {
			new_point.normal[i] = normal[i];
		}
	}

	// Homogeneous component
	new_point.vertex[3] = 1;
	new_point.normal[3] = 1;

	return addPoint(new_point);
}

IritPolygon *IritPolygon::getNextPolygon() {
	return m_next_polygon;
}

void IritPolygon::setNextPolygon(IritPolygon *polygon) {
	m_next_polygon = polygon;
}


void IritPolygon::draw(int *bitmap, int width, int height, struct State state, Matrix &normal_transform,
					   Matrix &vertex_transform) {
	struct IritPoint *current_point = m_points;
	Vector current_vertex = current_point->vertex;
	Vector next_vertex;
	Vector polygon_normal[2];

	// For vertex normal drawing
	Vector normal;

	/* Draw shape's lines */
	while (current_point->next_point != nullptr) {
		current_vertex = current_point->vertex;
		next_vertex = current_point->next_point->vertex;
		current_vertex = vertex_transform * current_vertex;
		next_vertex = vertex_transform * next_vertex;

		if (state.is_perspective_view) {
			current_vertex.Homogenize();
			next_vertex.Homogenize();
		}
		current_vertex = state.screen_mat * current_vertex;
		next_vertex = state.screen_mat * next_vertex;

		lineDraw(bitmap, width, height, state.wire_color, current_vertex, next_vertex);

		if (state.show_vertex_normal) {
			normal = current_point->normal * 0.3;
			normal += current_point->vertex;
			normal = vertex_transform * normal;
			if (state.is_perspective_view)
				normal.Homogenize();
			normal = state.screen_mat * normal;

			lineDraw(bitmap, width, height, state.wire_color, current_vertex, normal);
		}

		current_point = current_point->next_point;
	}

	if (state.show_polygon_normal) {
		polygon_normal[0] = vertex_transform * normal_start;
		polygon_normal[1] = vertex_transform * normal_end;

		if (state.is_perspective_view) {
			polygon_normal[0].Homogenize();
			polygon_normal[1].Homogenize();
		}

		polygon_normal[0] = state.screen_mat * polygon_normal[0];
		polygon_normal[1] = state.screen_mat * polygon_normal[1];
		
		lineDraw(bitmap, width, height, state.wire_color, polygon_normal[0], polygon_normal[1]);
	}
}

IritPolygon &IritPolygon::operator++() {
	return *m_next_polygon;
}

IritObject::IritObject() : m_polygons_nr(0), m_polygons(nullptr), m_iterator(nullptr) {
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

void IritObject::draw(int *bitmap, int width, int height, struct State state, Matrix &normal_transform,
					  Matrix &vertex_transform) {
	m_iterator = m_polygons;
	while (m_iterator) {
		m_iterator->draw(bitmap, width, height, state, normal_transform, vertex_transform);
		m_iterator = m_iterator->getNextPolygon();
	}
}

IritWorld::IritWorld() : m_objects_nr(0), m_objects_arr(nullptr) {
	state.show_vertex_normal = false;
	state.show_polygon_normal = false;
	state.object_frame = false;
	state.is_perspective_view = false;
	state.object_transform = true;

	max_bound_coord = Vector();
	min_bound_coord = Vector();

	state.coord_mat = Matrix::Identity();
	state.center_mat = Matrix::Identity();
	state.ratio_mat = Matrix::Identity();
	state.world_mat = Matrix::Identity();
	state.object_mat = Matrix::Identity();
	state.ortho_mat = Matrix::Identity();

	state.view_mat = createViewMatrix(DEAULT_VIEW_PARAMETERS);
	state.projection_plane_distance = DEFAULT_PROJECTION_PLANE_DISTANCE;

	state.bg_color = BG_DEFAULT_COLOR;
	state.wire_color = WIRE_DEFAULT_COLOR;
	state.frame_color = FRAME_DEFAULT_COLOR;
}

IritWorld::IritWorld(Vector axes[NUM_OF_AXES], Vector &axes_origin) : m_objects_nr(0), m_objects_arr(nullptr) {
	state.show_vertex_normal = false;
	state.show_polygon_normal = false;
	state.object_frame = false;
	state.is_perspective_view = false;
	state.object_transform = true;

	max_bound_coord = Vector();
	min_bound_coord = Vector();

	state.coord_mat = Matrix::Identity();
	state.center_mat = Matrix::Identity();
	state.ratio_mat = Matrix::Identity();
	state.world_mat = Matrix::Identity();
	state.object_mat = Matrix::Identity();
	state.ortho_mat = Matrix::Identity();

	state.view_mat = createViewMatrix(DEAULT_VIEW_PARAMETERS);
	state.projection_plane_distance = DEFAULT_PROJECTION_PLANE_DISTANCE;

	state.bg_color = BG_DEFAULT_COLOR;
	state.wire_color = WIRE_DEFAULT_COLOR;
	state.frame_color = FRAME_DEFAULT_COLOR;
}

IritWorld::~IritWorld() {
	delete[] m_objects_arr;
}

void IritWorld::setScreenMat(Vector axes[NUM_OF_AXES], Vector &axes_origin, int screen_width, int screen_height) {
	Matrix coor_mat;
	Matrix center_mat;
	Matrix ratio_mat;

	// Expand to ratio

	// Ratio should be about a fifth of the screen.
	ratio_mat = Matrix::Identity();
	ratio_mat.array[0][0] = screen_width / 5.0;
	ratio_mat.array[1][1] = screen_height / 5.0;
	state.ratio_mat = ratio_mat;
	
	// Set world coordinate system
	coor_mat = Matrix(axes[0], axes[1], axes[2]);
	state.coord_mat = coor_mat;

	// Center to screen
	center_mat = createTranslationMatrix(axes_origin);
	state.center_mat = center_mat;
}

void IritWorld::setOrthoMat()
{
	double max_x = max_bound_coord[0],
	   	   min_x = min_bound_coord[0],
		   max_y = max_bound_coord[1],
		   min_y = min_bound_coord[1],
		   max_z = max_bound_coord[2],
		   min_z = min_bound_coord[2];

	state.ortho_mat.array[X_AXIS][0] = 2 / (max_x - min_x);
	state.ortho_mat.array[Y_AXIS][1] = 2 / (max_y - min_y);
	state.ortho_mat.array[Z_AXIS][2] = 2 / (max_z - min_z);

	state.ortho_mat.array[X_AXIS][3] = -(max_x + min_x) / (max_x - min_x);
	state.ortho_mat.array[Y_AXIS][3] = -(max_y + min_y) / (max_y - min_y);
	state.ortho_mat.array[Z_AXIS][3] =  (max_z + min_z) / (max_z - min_z);
}

IritObject *IritWorld::createObject() {
	IritObject *new_object = new IritObject();
	if (!new_object)
		return nullptr;

	if (!addObjectP(new_object))
		return nullptr;

	return new_object;
}

bool IritWorld::addObjectP(IritObject *p_object) {

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

bool IritWorld::isEmpty() {
	return m_objects_nr == 0;
};

Matrix createProjectionMatrix(const float &angleOfView, const float &near_z, const float &far_z)
{
	Matrix projection_mat(Matrix::Identity());
	// set the basic projection matrix
	double scale = 1 / tan(angleOfView * 0.5 * M_PI / 180);
	projection_mat.array[0][0] = -scale; // scale the x coordinates of the projected point
	projection_mat.array[1][1] = -scale; // scale the y coordinates of the projected point
	projection_mat.array[2][2] = -far_z / (far_z - near_z); // used to remap z to [0,1]
	projection_mat.array[3][2] = -far_z * near_z / (far_z - near_z); // used to remap z [0,1]
	projection_mat.array[2][3] = -1; // set w = -z
	projection_mat.array[3][3] = 0;

	return projection_mat;
}

void IritWorld::draw(int *bitmap, int width, int height) {
		Matrix projection_mat;
		Matrix shrink = Matrix::Identity() * (1.0 / 10.0);

		Matrix model_mat = state.world_mat * state.coord_mat;

		if (state.is_perspective_view) {
			Matrix perspective_matrix = Matrix::Identity();
			perspective_matrix.array[3][2] = 1 / state.projection_plane_distance;
			perspective_matrix.array[3][3] = 0;

			/* Use frustum perpective view */
			//projection_mat = createProjectionMatrix(45, 0.5, 30) * state.view_mat;

			/* Use Gershon's perpective matrix */
			projection_mat = perspective_matrix * state.view_mat * state.ortho_mat;
		} else {
			projection_mat = state.view_mat * state.ortho_mat;
		}

		this->state.screen_mat = state.center_mat * state.ratio_mat;

		Matrix vertex_transform = projection_mat * model_mat * state.object_mat;
		// Normal ended being a BIT too big. Lets divide them by 10
		Matrix normal_transform = shrink * vertex_transform;

		// Draw all objects
		for (int i = 0; i < m_objects_nr; i++)
			m_objects_arr[i]->draw(bitmap, width, height, state, normal_transform, vertex_transform);

		// Draw a frame around all objects
		if (state.object_frame)
			drawFrame(bitmap, width, height, state, vertex_transform);
}

void IritWorld::drawFrame(int *bitmap, int width, int height, struct State state, Matrix &transform) {
	//Matrix transform = state.center_mat * state.coord_mat * state.ratio_mat * state.ortho_mat * state.world_mat * state.object_mat;
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

	// Draw "front side"

	lineDraw(bitmap, width, height, state.frame_color, coords[0], coords[1]);
	lineDraw(bitmap, width, height, state.frame_color, coords[1], coords[3]);
	lineDraw(bitmap, width, height, state.frame_color, coords[3], coords[2]);
	lineDraw(bitmap, width, height, state.frame_color, coords[2], coords[0]);

	// Draw "back side"

	lineDraw(bitmap, width, height, state.frame_color, coords[4], coords[5]);
	lineDraw(bitmap, width, height, state.frame_color, coords[5], coords[7]);
	lineDraw(bitmap, width, height, state.frame_color, coords[7], coords[6]);
	lineDraw(bitmap, width, height, state.frame_color, coords[6], coords[4]);

	// Draw "sides"

	// Top right
	lineDraw(bitmap, width, height, state.frame_color, coords[0], coords[4]);
	// Bottom right
	lineDraw(bitmap, width, height, state.frame_color, coords[1], coords[5]);
	// Top left
	lineDraw(bitmap, width, height, state.frame_color, coords[2], coords[6]);
	// Bottom left
	lineDraw(bitmap, width, height, state.frame_color, coords[3], coords[7]);
}

Matrix createTranslationMatrix(double &x, double &y, double z) {
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
	Matrix camera_translation = createTranslationMatrix(x, y, z);

	return camera_translation.Inverse();
}

void lineDrawOct0(int *bits, int width, int height, RGBQUAD color, Vector first, Vector second) {
	int dx = (int)(second[0] - first[0]),
		dy = (int)(second[1] - first[1]),
		error = (2 * dy) - dx,
		x = (int)first[0],
		y = (int)first[1],
		end_x = (int)second[0],
		end_y = (int)second[1];

	while (x != end_x) {
		if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
			bits[y * width + x] = *((int*)&color);
		}			
		if (error > 0) {
			y++;
			x++;
			error += 2 * (dy -  dx); // North-East
		} else {
			x++;
			error += 2 * dy;         // East
		}
	}
}

void lineDrawOct1(int *bits, int width, int height, RGBQUAD color, Vector first, Vector second) {
	int dx = (int)(second[0] - first[0]),
		dy = (int)(second[1] - first[1]),
		error = dy - (2 * dx),
		x = (int)first[0],
		y = (int)first[1],
		end_x = (int)second[0],
		end_y = (int)second[1];

	while (y != end_y) {
		if ((x >= 0) && (x < width) && (y >= 0) && (y < height))
			bits[y * width + x] = *((int*)&color);
		if (error > 0) {
			y++;
			error += -2 * dx;       // North
		} else {
			y++;
			x++;
			error += 2 * (dy - dx); // North-East
		}
	}
}

void lineDrawOct6(int *bits, int width, int height, RGBQUAD color, Vector first, Vector second) {
	int dx = (int)(second[0] - first[0]),
		dy = (int)(second[1] - first[1]),
		error = dy + (2 * dx),
		x = (int)first[0],
		y = (int)first[1],
		end_x = (int)second[0],
		end_y = (int)second[1];

	while (y != end_y) {
		if ((x >= 0) && (x < width) && (y >= 0) && (y < height))
			bits[y * width + x] = *((int*)&color);
		if (error > 0) {
			y--;
			x++;
			error += 2 * (dy + dx); // South-East
		} else {
			y--;
			error += 2 * dx;        // South
		}
	}
}

void lineDrawOct7(int *bits, int width, int height, RGBQUAD color, Vector first, Vector second) {
	int dx = (int)(second[0] - first[0]),
		dy = (int)(second[1] - first[1]),
		error = (2 * dy) + dx,
		x = (int)first[0],
		y = (int)first[1],
		end_x = (int)second[0],
		end_y = (int)second[1];

	while (x != end_x) {
		if ((x >= 0) && (x < width) && (y >= 0) && (y < height))
			bits[y * width + x] = *((int*)&color);
		if (error > 0) {
			x++;
			error += 2 * dy; // East
		} else {
			x++;
			y--;
			error += 2 * (dx + dy); // South-East
		}
	}
}

void lineDraw(int *bits, int width, int height, RGBQUAD color, Vector first, Vector second) {
	double delta_x, delta_y, ratio;

	// Handle case where they are vertical
	if (first[0] == second[0]) {
		if (first[1] < second[1]) {
			lineDrawOct1(bits, width, height, color, first, second);
		} else {
			if (first[1] > second[2]) {
				lineDrawOct6(bits, width, height, color, first, second);
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
		lineDrawOct1(bits, width, height, color, first, second);
	} else if ((ratio >= 0.0) && (ratio < 1.0)) {  // Octant 0
		lineDrawOct0(bits, width, height, color, first, second);
	} else if ((ratio >= -1.0) && (ratio < 0.0)) { // Octant 7
		lineDrawOct7(bits, width, height, color, first, second);
	} else {									   // Octant 6
		lineDrawOct6(bits, width, height, color, first, second);
	}
}
