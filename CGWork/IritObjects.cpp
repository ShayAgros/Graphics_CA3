#include "IritObjects.h"
#include "aux_functions.h"

Matrix createTranslationMatrix(double x, double y, double z = 0);
Matrix createTranslationMatrix(Vector &v);
void lineDraw(int *bitmap, State state, int width, int height, RGBQUAD color, Vector first, Vector second);

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

/* This function uses an algorithm to find intersection between
 * two lines. For more info see
 * https://www.geeksforgeeks.org/program-for-point-of-intersection-of-two-lines/
*/
void IritPolygon::paintObject(int *bitmap, int width, int height, RGBQUAD color, State &state) {

	int y_max = (int)(state.screen_mat * Vector(0, 1, 0, 1))[Y_AXIS];
	double A1, B1, C1; // We represent our lines by A1*X + B1*Y = C1
	double A2, B2, C2;
	double determinant;

	IntersectionPoint *intersecting_x;
	int intersecting_x_nr;
	int min_x, max_x;
	int min_y, max_y;

	if (!lines_nr)
		return;

	mergeSort(lines, lines_nr);

	intersecting_x = new IntersectionPoint[lines_nr * 2];
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
				intersecting_x[intersecting_x_nr++].z = current_line.z1;
				intersecting_x[intersecting_x_nr].x = current_line.x2;
				intersecting_x[intersecting_x_nr++].z = current_line.z2;

				min_x = min(min_x, min(current_line.x1, current_line.x2));
				max_x = max(max_x, max(current_line.x1, current_line.x2));
			}
			else {
				double t;
				double extrapolated_z;
				intersecting_x[intersecting_x_nr].x = (int)((B2 * C1 - B1 * C2) / determinant);

				t = (double)(intersecting_x[intersecting_x_nr].x - current_line.x1) / (double)(current_line.x2 - current_line.x1);
				extrapolated_z = (1 - t) * current_line.z1 + t * current_line.z2;

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
			double t_step = 1.0 / (intersecting_x[i + 1].x - intersecting_x[i].x);

			// paint all the pixels between the two adjact x values (inclusive)
			for (int x = intersecting_x[i].x; x <= intersecting_x[i + 1].x; x++) {
				if (bitmap[y * width + x])
					continue;
				bitmap[y * width + x] = *((int*)&color); 
				
				// This might be a better way of doing this, since line draw already check all these stuff
				// anyway, this can probably be removed when we are comfortable with the current
				// way to draw
				/*
				Vector* first = new Vector((double)intersecting_x[i].x, (double)y, intersecting_x[i].z);
				Vector* second = new Vector((double)intersecting_x[i+1].x, (double)y, intersecting_x[i+1].z);
				lineDraw(bitmap, state, width, height, color, *first, *second);
				delete(first);
				delete(second);
				*/
				

				double extrapolated_z;
				extrapolated_z = (1 - t) * intersecting_x[i].z + t * intersecting_x[i + 1].z;
				t += t_step;

				
				bool not_drawn,
					closer,
					within_bounds = ((x >= 0) && (x < width) && (y >= 0) && (y < height));

				if (within_bounds) {
					not_drawn = !state.is_drawn_buffer[y * width + x];
				    closer = extrapolated_z < state.z_buffer[y * width + x];
					if (closer || not_drawn) {
						bitmap[y * width + x] = *((int*)&color);
						state.z_buffer[y * width + x] = extrapolated_z;
						state.is_drawn_buffer[y * width + x] = true;
					}
				}
			}
		}
	}

}

void IritPolygon::draw(int *bitmap, int width, int height, RGBQUAD color, struct State state,
					   Matrix &vertex_transform) {
	struct IritPoint *current_point = m_points;
	Vector current_vertex = current_point->vertex;
	Vector next_vertex;
	Vector polygon_normal[2];
	RGBQUAD current_color = (state.is_default_color) ? color : state.wire_color;
	RGBQUAD normal_color;

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
		if (current_vertex[X_AXIS] > 1 || current_vertex[X_AXIS] < -1 || current_vertex[Y_AXIS] > 1 || current_vertex[Y_AXIS] < -1 ||
			current_vertex[2] > 1 || current_vertex[2] < -1) {
			printf("Way out of line!\n");
			goto pass_this_point;
		}

		current_vertex = state.screen_mat * current_vertex;
		next_vertex = state.screen_mat * next_vertex;

		lineDraw(bitmap, state, width, height, current_color, current_vertex, next_vertex);

		// Add the drawn line to the list of lines on the screen
		lines[lines_nr++] = { (int)current_vertex.coordinates[X_AXIS],
							  (int)current_vertex.coordinates[Y_AXIS],
							  (double)current_vertex.coordinates[Z_AXIS],
							  (int)next_vertex.coordinates[X_AXIS],
							  (int)next_vertex.coordinates[Y_AXIS],
							  (double)next_vertex.coordinates[Z_AXIS]};

		if (state.show_vertex_normal) {
			normal = current_point->normal * 0.3;
			normal += current_point->vertex;
			normal = vertex_transform * normal;
			if (state.is_perspective_view)
				normal.Homogenize();
			normal = state.screen_mat * normal;

			normal_color = state.normal_color;
			if (state.tell_normals_apart) {
				if (current_point->is_irit_normal)
					normal_color = IRIT_NORMAL_COLOR;
				else
					normal_color = CALC_NORMAL_COLOR;
			}

			lineDraw(bitmap, state, width, height, normal_color, current_vertex, normal);
		}
pass_this_point:
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
		
		normal_color = state.normal_color;
		if (state.tell_normals_apart) {
			if (this->is_irit_normal)
				normal_color = IRIT_NORMAL_COLOR;
			else
				normal_color = CALC_NORMAL_COLOR;
		}
		lineDraw(bitmap, state, width, height, normal_color, polygon_normal[0], polygon_normal[1]);
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
	if (current_vertex[X_AXIS] > 1 || current_vertex[X_AXIS] < -1 || current_vertex[Y_AXIS] > 1 || current_vertex[Y_AXIS] < -1 ||
		current_vertex[2] > 1 || current_vertex[2] < -1) {
		printf("Way out of line!\n");
		goto paint_object;
	}

	current_vertex = state.screen_mat * current_vertex;
	next_vertex = state.screen_mat * next_vertex;


	lineDraw(bitmap, state, width, height, current_color, current_vertex, next_vertex);
	// Add the drawn line to the list of lines on the screen
	lines[lines_nr++] = { (int)current_vertex.coordinates[X_AXIS],
						  (int)current_vertex.coordinates[Y_AXIS],
						  (double)current_vertex.coordinates[Z_AXIS],
						  (int)next_vertex.coordinates[X_AXIS],
						  (int)next_vertex.coordinates[Y_AXIS],
						  (double)next_vertex.coordinates[Z_AXIS] };


paint_object:
	// paint the polygon
	paintObject(bitmap, width, height, current_color, state);
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

void IritObject::draw(int *bitmap, int width, int height, State &state,
					  Matrix &vertex_transform) {
	int points_nr = 0;
	m_iterator = m_polygons;

	m_iterator = m_polygons;
	while (m_iterator) {
		m_iterator->draw(bitmap, width, height, object_color, state, vertex_transform);
		m_iterator = m_iterator->getNextPolygon();
	}
}

IritFigure::IritFigure() : m_objects_nr(0), m_objects_arr(nullptr) {

	max_bound_coord = Vector();
	max_bound_coord[3] = 1;
	min_bound_coord = Vector();
	min_bound_coord[3] = 1;

	object_mat = Matrix::Identity();

	// We draw an object in a 1/2 of its size. No need to fill the whole screen
	world_mat = Matrix::createScaleMatrix((double)(1) /2, (double)(1) /2, 1) * Matrix::Identity();
}

IritFigure::~IritFigure() {
	delete[] m_objects_arr;
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

void IritFigure::draw(int *bitmap, int width, int height, Matrix transform, State &state) {
	Matrix shrink = Matrix::Identity() * (1.0 / 10.0);

	Matrix vertex_transform = transform * world_mat * object_mat;

	// Draw all objects
	for (int i = 0; i < m_objects_nr; i++)
		m_objects_arr[i]->draw(bitmap, width, height, state, vertex_transform);

	// Draw a frame around all objects
	if (state.object_frame)
		drawFrame(bitmap, width, height, state, vertex_transform);
}

void IritFigure::drawFrame(int *bitmap, int width, int height, struct State state, Matrix &transform) {
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

	lineDraw(bitmap, state, width, height, state.frame_color, coords[0], coords[1]);
	lineDraw(bitmap, state, width, height, state.frame_color, coords[1], coords[3]);
	lineDraw(bitmap, state, width, height, state.frame_color, coords[3], coords[2]);
	lineDraw(bitmap, state, width, height, state.frame_color, coords[2], coords[0]);

	// Draw "back side"

	lineDraw(bitmap, state, width, height, state.frame_color, coords[4], coords[5]);
	lineDraw(bitmap, state, width, height, state.frame_color, coords[5], coords[7]);
	lineDraw(bitmap, state, width, height, state.frame_color, coords[7], coords[6]);
	lineDraw(bitmap, state, width, height, state.frame_color, coords[6], coords[4]);

	// Draw "sides"

	// Top right
	lineDraw(bitmap, state, width, height, state.frame_color, coords[0], coords[4]);
	// Bottom right
	lineDraw(bitmap, state, width, height, state.frame_color, coords[1], coords[5]);
	// Top left
	lineDraw(bitmap, state, width, height, state.frame_color, coords[2], coords[6]);
	// Bottom left
	lineDraw(bitmap, state, width, height, state.frame_color, coords[3], coords[7]);
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

IritWorld::IritWorld() : m_figures_nr(0), m_figures_arr(nullptr) {
	state.show_vertex_normal = false;
	state.show_polygon_normal = false;
	state.object_frame = false;
	state.is_perspective_view = false;
	state.object_transform = true;
	state.is_default_color = true;
	state.tell_normals_apart = false;

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

	state.bg_color = BG_DEFAULT_COLOR;
	state.wire_color = WIRE_DEFAULT_COLOR;
	state.frame_color = FRAME_DEFAULT_COLOR;
	state.normal_color = NORMAL_DEFAULT_COLOR;

	state.is_drawn_buffer = nullptr;
	state.z_buffer = nullptr;
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


Vector IritWorld::projectPoint(Vector &td_point, Matrix &transformation) {
	Vector transformed_point = transformation * td_point;
	if (state.is_perspective_view) 
		transformed_point.Homogenize();
	return state.screen_mat * transformed_point;
}

void IritWorld::draw(int *bitmap, int width, int height) {
		Matrix projection_mat = createProjectionMatrix();
		Matrix transformation = projection_mat * state.view_mat;

		this->state.screen_mat = state.center_mat * state.ratio_mat;

		// Draw all objects
		for (int i = 0; i < m_figures_nr; i++)
			m_figures_arr[i]->draw(bitmap, width, height, transformation, state);
}

IritFigure *IritWorld::getFigureInPoint(CPoint &point) {
	IritFigure *figure;
	Matrix projection_mat = createProjectionMatrix();
	Matrix transformation_mat;

	Vector min_2d_point, max_2d_point;

	int max_x, min_x, max_y, min_y;
	
	for (int i = 0; i < m_figures_nr; i++) {
		figure = m_figures_arr[i];
		/* The received point assumes that the point is given in a coordinate system in which 
		 * the y value grows down (left-upper corner is (0, 0) ). To match our coordinate system
		   to the point's, we rotate the object by 'coord_mat' */
		transformation_mat = projection_mat * state.coord_mat * figure->world_mat * figure->object_mat;

		min_2d_point = projectPoint(figure->min_bound_coord, transformation_mat);
		max_2d_point = projectPoint(figure->max_bound_coord, transformation_mat);

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


void lineDrawOct0(int *bitmap, State state, int width, int height, RGBQUAD color, Vector first, Vector second) {
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
		   extrapolate_z = 0.0;

	while (x != end_x) {
		current_z = (1.0 - extrapolate_z) * start_z + extrapolate_z * end_z;
		if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
			if (!state.is_drawn_buffer[y * width + x]) {
				bitmap[y * width + x] = *((int*)&color);
				state.z_buffer[y * width + x] = current_z;
				state.is_drawn_buffer[y * width + x] = true;
			} else {
				if (state.z_buffer[y * width + x] < current_z) {
					bitmap[y * width + x] = *((int*)&color);
					state.z_buffer[y * width + x] = current_z;
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
		extrapolate_z = z_step;
	}
}

void lineDrawOct1(int *bitmap, State state, int width, int height, RGBQUAD color, Vector first, Vector second) {
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
		extrapolate_z = 0.0;

	while (y != end_y) {
		current_z = (1.0 - extrapolate_z) * start_z + extrapolate_z * end_z;
		if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
			current_z = (1.0 - extrapolate_z) * start_z + extrapolate_z * end_z;
			if (!state.is_drawn_buffer[y * width + x]) {
				bitmap[y * width + x] = *((int*)&color);
				state.z_buffer[y * width + x] = current_z;
				state.is_drawn_buffer[y * width + x] = true;
			} else {
				if (state.z_buffer[y * width + x] < current_z) {
					bitmap[y * width + x] = *((int*)&color);
					state.z_buffer[y * width + x] = current_z;
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
		extrapolate_z += z_step;
	}
}

void lineDrawOct6(int *bitmap, State state, int width, int height, RGBQUAD color, Vector first, Vector second) {
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
		extrapolate_z = 0.0;

	while (y != end_y) {
		if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
			current_z = (1.0 - extrapolate_z) * start_z + extrapolate_z * end_z;
			if (!state.is_drawn_buffer[y * width + x]) {
				bitmap[y * width + x] = *((int*)&color);
				state.z_buffer[y * width + x] = current_z;
				state.is_drawn_buffer[y * width + x] = true;
			} else {
				if (state.z_buffer[y * width + x] < current_z) {
					bitmap[y * width + x] = *((int*)&color);
					state.z_buffer[y * width + x] = current_z;
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
		extrapolate_z += z_step;
	}
}

void lineDrawOct7(int *bitmap, State state, int width, int height, RGBQUAD color, Vector first, Vector second) {
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
		extrapolate_z = 0.0;

	while (x != end_x) {
		if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
			current_z = (1.0 - extrapolate_z) * start_z + extrapolate_z * end_z;
			if (!state.is_drawn_buffer[y * width + x]) {
				bitmap[y * width + x] = *((int*)&color);
				state.z_buffer[y * width + x] = current_z;
				state.is_drawn_buffer[y * width + x] = true;
			} else {
				if (state.z_buffer[y * width + x] < current_z) {
					bitmap[y * width + x] = *((int*)&color);
					state.z_buffer[y * width + x] = current_z;
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
		extrapolate_z += z_step;
	}
}

void lineDraw(int *bitmap, State state, int width, int height, RGBQUAD color, Vector first, Vector second) {
	double delta_x, delta_y, ratio;

	// Handle case where they are vertical
	if (first[0] == second[0]) {
		if (first[1] < second[1]) {
			lineDrawOct1(bitmap, state, width, height, color, first, second);
		} else {
			if (first[1] > second[2]) {
				lineDrawOct6(bitmap, state, width, height, color, first, second);
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
		lineDrawOct1(bitmap, state, width, height, color, first, second);
	} else if ((ratio >= 0.0) && (ratio < 1.0)) {  // Octant 0
		lineDrawOct0(bitmap, state, width, height, color, first, second);
	} else if ((ratio >= -1.0) && (ratio < 0.0)) { // Octant 7
		lineDrawOct7(bitmap, state, width, height, color, first, second);
	} else {									   // Octant 6
		lineDrawOct6(bitmap, state, width, height, color, first, second);
	}
}
