#include "IritObjects.h"

Matrix createTranslationMatrix(double &x, double &y, double z = 0);
Matrix createTranslationMatrix(Vector &v);

IritPolygon::IritPolygon() : m_point_nr(0), m_points(nullptr), m_next_polygon(nullptr) {
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

bool IritPolygon::addPoint(IPVertexStruct *vertex) {
	IritPoint new_point;

	for (int i = 0; i < 3; i++) {
		new_point.vertex[i] = vertex->Coord[i];
		new_point.normal[i] = vertex->Coord[i];
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


void IritPolygon::draw(CDC *pDCToUse, Matrix &transformation) {
	struct IritPoint *current_point = m_points;
	Vector vertex = current_point->vertex * 30;

	/* "Draw" first point */
	vertex = transformation * vertex;
	pDCToUse->MoveTo((int)floor(vertex[0]), (int)floor(vertex[1]));
	current_point = current_point->next_point;

	/* Draw shape's lines */
	while (current_point) {
		vertex = current_point->vertex * 30;
		vertex = transformation * vertex;

		pDCToUse->LineTo((int)floor(vertex[0]), (int)floor(vertex[1]));
		current_point = current_point->next_point;
	}

	/* Draw last line from last vertex to first one */
	vertex = m_points->vertex * 30;
	vertex = transformation * vertex;

	pDCToUse->LineTo((int)floor(vertex[0]), (int)floor(vertex[1]));

	// TODO: draw normals
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

void IritObject::draw(CDC *pDCToUse, Matrix &transformation) {
	m_iterator = m_polygons;
	while (m_iterator) {
		m_iterator->draw(pDCToUse, transformation);
		m_iterator = m_iterator->getNextPolygon();
	}
}

IritWorld::IritWorld() : m_objects_nr(0), m_objects_arr(nullptr) {
	world_matrix = Matrix::Identity();
	object_matrix = Matrix::Identity();
}

IritWorld::IritWorld(Vector axes[NUM_OF_AXES], Vector &axes_origin) : m_objects_nr(0), m_objects_arr(nullptr) {
	for (int i = 0; i < 3; i++)
		m_axes[i] = axes[i];
	m_axes_origin = axes_origin;
	world_matrix = Matrix::Identity();
	object_matrix = Matrix::Identity();
}

IritWorld::~IritWorld() {
	delete[] m_objects_arr;
}

void IritWorld::setSceneCoordinateSystem(Vector axes[NUM_OF_AXES], Vector &axes_origin) {
	for (int i = 0; i < 3; i++)
		m_axes[i] = axes[i];
	m_axes_origin = axes_origin;
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
	Matrix center_screen = createTranslationMatrix(m_axes_origin);
	Matrix transformation = center_screen * world_matrix * object_matrix;
	for (int i = 0; i < m_objects_nr; i++)
		m_objects_arr[i]->draw(pDCToUse, transformation);
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