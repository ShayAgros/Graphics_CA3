#include "IritObjects.h"

Matrix createTranslationMatrix(double &x, double &y, double z = 0);
Matrix createTranslationMatrix(Vector &v);

#define BOX_NUM_OF_VERTICES 8

IritPolygon::IritPolygon() : m_point_nr(0), m_points(nullptr),
			normal(Vector(0, 0, 0, 1)), has_normal(false), m_next_polygon(nullptr) {
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

bool IritPolygon::addPoint(IPVertexStruct *vertex, bool has_normal) {
	IritPoint new_point;
	new_point.has_normal = false;

	for (int i = 0; i < 3; i++) {
		new_point.vertex[i] = vertex->Coord[i];

		if (has_normal)
			new_point.normal[i] = vertex->Normal[i];
	}

	// Homogeneous component
	new_point.vertex[3] = 1;
	if (has_normal) {
		new_point.normal[3] = 1;
		new_point.has_normal = true;
	}

	return addPoint(new_point);
}

IritPolygon *IritPolygon::getNextPolygon() {
	return m_next_polygon;
}

void IritPolygon::setNextPolygon(IritPolygon *polygon) {
	m_next_polygon = polygon;
}


void IritPolygon::draw(CDC *pDCToUse, struct State state, Matrix &normal_transform,
					   Matrix &vertex_transform) {
	struct IritPoint *current_point = m_points;
	Vector vertex = current_point->vertex;

	// For vertex normal drawing
	Vector normal;
	double normal_end_x, normal_end_y;

	// For polygon normal drawing
	double x_sum = 0, y_sum = 0, num_of_vertices = 0;

	/* "Draw" first point */
	vertex = vertex_transform * vertex;
	x_sum += vertex[0];
	y_sum += vertex[1];
	++num_of_vertices;

	pDCToUse->MoveTo((int)floor(vertex[0]), (int)floor(vertex[1]));

	if (state.show_vertex_normal && current_point->has_normal) {
		normal = normal_transform * current_point->normal;
		normal_end_x = vertex[0] + normal[0];
		normal_end_y = vertex[1] + normal[1];
		pDCToUse->LineTo((int)floor(normal_end_x), (int)floor(normal_end_y));
		pDCToUse->MoveTo((int)floor(vertex[0]), (int)floor(vertex[1]));
	}

	current_point = current_point->next_point;

	/* Draw shape's lines */
	while (current_point) {
		vertex = current_point->vertex;
		vertex = vertex_transform * vertex;
		x_sum += vertex[0];
		y_sum += vertex[1];
		++num_of_vertices;

		pDCToUse->LineTo((int)floor(vertex[0]), (int)floor(vertex[1]));

		if (state.show_vertex_normal && current_point->has_normal) {
			normal = normal_transform * current_point->normal;
			normal_end_x = vertex[0] + normal[0];
			normal_end_y = vertex[1] + normal[1];
			pDCToUse->LineTo((int)floor(normal_end_x), (int)floor(normal_end_y));
			pDCToUse->MoveTo((int)floor(vertex[0]), (int)floor(vertex[1]));
		}

		current_point = current_point->next_point;
	}

	/* Draw last line from last vertex to first one */
	vertex = m_points->vertex;
	vertex = vertex_transform * vertex;

	pDCToUse->LineTo((int)floor(vertex[0]), (int)floor(vertex[1]));

	if (state.show_polygon_normal && this->has_normal) {
		// TODO: add z value to normals
		Vector polygon_center = Vector(x_sum / num_of_vertices, y_sum / num_of_vertices, 0, 1);
		normal = normal_transform * this->normal;
		pDCToUse->MoveTo((int)floor(polygon_center[0]), (int)floor(polygon_center[1]));
		pDCToUse->LineTo((int)floor(polygon_center[0] + normal[0]), (int)floor(polygon_center[1] + normal[1]));
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

void IritObject::draw(CDC *pDCToUse, struct State state, Matrix &normal_transform,
					  Matrix &vertex_transform) {
	m_iterator = m_polygons;
	while (m_iterator) {
		m_iterator->draw(pDCToUse, state, normal_transform, vertex_transform);
		m_iterator = m_iterator->getNextPolygon();
	}
}

IritWorld::IritWorld() : m_objects_nr(0), m_objects_arr(nullptr) {
	state.show_vertex_normal = false;
	state.show_polygon_normal = false;
	state.object_frame = false;
	state.perspective = false;
	state.object_transform = true;

	bg_color = BG_DEFAULT_COLOR;
	wire_color = WIRE_DEFAULT_COLOR;
	frame_color = FRAME_DEFULAT_COLOR;

	max_bound_coord = Vector();
	min_bound_coord = Vector();

	state.coord_mat = Matrix::Identity();
	state.center_mat = Matrix::Identity();
	state.ratio_mat = Matrix::Identity();
	state.world_mat = Matrix::Identity();
	state.object_mat = Matrix::Identity();
}

IritWorld::IritWorld(Vector axes[NUM_OF_AXES], Vector &axes_origin) : m_objects_nr(0), m_objects_arr(nullptr) {
	state.show_vertex_normal = false;
	state.show_polygon_normal = false;
	state.object_frame = false;
	state.perspective = false;
	state.object_transform = true;

	bg_color = BG_DEFAULT_COLOR;
	wire_color = WIRE_DEFAULT_COLOR;
	frame_color = FRAME_DEFULAT_COLOR;

	max_bound_coord = Vector();
	min_bound_coord = Vector();

	state.coord_mat = Matrix::Identity();
	state.center_mat = Matrix::Identity();
	state.ratio_mat = Matrix::Identity();
	state.world_mat = Matrix::Identity();
	state.object_mat = Matrix::Identity();
}

IritWorld::~IritWorld() {
	delete[] m_objects_arr;
}

void IritWorld::setScreenMat(Vector axes[NUM_OF_AXES], Vector &axes_origin, int screen_width, int screen_height) {
	Matrix coor_mat;
	Matrix center_mat;
	Matrix ratio_mat;

	// Expand to ratio

	// TODO: the ratio here shouldn't be 15, but used with ortho normalization
	ratio_mat = Matrix::Identity();
	ratio_mat.array[0][0] = screen_width / 15;
	ratio_mat.array[1][1] = screen_height / 15;
	state.ratio_mat = ratio_mat;
	
	// Set to correct coordinate system
	coor_mat = Matrix(axes[0], axes[1], axes[2]);
	state.coord_mat = coor_mat;

	// Center to screen
	center_mat = createTranslationMatrix(axes_origin);
	state.center_mat = center_mat;
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

void IritWorld::draw(CDC *pDCToUse) {
	CPen *object_pen = new CPen(PS_SOLID, 0, wire_color),
		 *frame_pen = new CPen(PS_SOLID, FRAME_WIDTH, frame_color);

	// Normal doesnt need to be centered to screen
	Matrix normal_transform = state.coord_mat * state.ratio_mat * state.world_mat * state.object_mat;
	Matrix vertex_transform = state.center_mat * normal_transform;

	// Normal ended being a BIT too big. Lets divide them by 3
	Matrix shrink = Matrix::Identity() * (1.0 / 3.0);
	normal_transform = shrink * normal_transform;

	// Draw all objects
	pDCToUse->SelectObject(object_pen);
	for (int i = 0; i < m_objects_nr; i++)
		m_objects_arr[i]->draw(pDCToUse, state, normal_transform, vertex_transform);

	// Draw a frame around all objects
	pDCToUse->SelectObject(frame_pen);
	if (state.object_frame)
		drawFrame(pDCToUse);

	delete object_pen;
	delete frame_pen;
}

void IritWorld::drawFrame(CDC *pDCToUse) {
	Matrix transform = state.center_mat * state.coord_mat * state.ratio_mat * state.world_mat * state.object_mat;

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
	}

	// Draw "front side"

	pDCToUse->MoveTo((int)floor((coords[0])[0]), (int)floor((coords[0])[1]));
	pDCToUse->LineTo((int)floor((coords[1])[0]), (int)floor((coords[1])[1]));
	pDCToUse->LineTo((int)floor((coords[3])[0]), (int)floor((coords[3])[1]));
	pDCToUse->LineTo((int)floor((coords[2])[0]), (int)floor((coords[2])[1]));
	pDCToUse->LineTo((int)floor((coords[0])[0]), (int)floor((coords[0])[1]));

	// Draw "back side"

	pDCToUse->MoveTo((int)floor((coords[4])[0]), (int)floor((coords[4])[1]));
	pDCToUse->LineTo((int)floor((coords[5])[0]), (int)floor((coords[5])[1]));
	pDCToUse->LineTo((int)floor((coords[7])[0]), (int)floor((coords[7])[1]));
	pDCToUse->LineTo((int)floor((coords[6])[0]), (int)floor((coords[6])[1]));
	pDCToUse->LineTo((int)floor((coords[4])[0]), (int)floor((coords[4])[1]));

	// Draw "sides"

	// Top right
	pDCToUse->MoveTo((int)floor((coords[0])[0]), (int)floor((coords[0])[1]));
	pDCToUse->LineTo((int)floor((coords[4])[0]), (int)floor((coords[4])[1]));
	// Bottom right
	pDCToUse->MoveTo((int)floor((coords[1])[0]), (int)floor((coords[1])[1]));
	pDCToUse->LineTo((int)floor((coords[5])[0]), (int)floor((coords[5])[1]));
	// Top left
	pDCToUse->MoveTo((int)floor((coords[2])[0]), (int)floor((coords[2])[1]));
	pDCToUse->LineTo((int)floor((coords[6])[0]), (int)floor((coords[6])[1]));
	// Bottom left
	pDCToUse->MoveTo((int)floor((coords[3])[0]), (int)floor((coords[3])[1]));
	pDCToUse->LineTo((int)floor((coords[7])[0]), (int)floor((coords[7])[1]));
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
