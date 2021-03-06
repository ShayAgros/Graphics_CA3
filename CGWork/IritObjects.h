#pragma once
#include <afxwin.h>
#include <assert.h>
#include <iritprsr.h>
#include "Vector.h"
#include "Matrix.h"
#include "IritLight.h"
#include "PngWrapper.h"
#include "Light.h"

// The color scheme here is			 <B G R *reserved*>
#define BG_DEFAULT_COLOR			{0, 0, 0, 0}       // Black
#define WIRE_DEFAULT_COLOR			{128, 128, 128, 0} // Grey
#define FRAME_DEFAULT_COLOR			{0, 0, 255, 0}     // Red
#define NORMAL_DEFAULT_COLOR		{255, 255, 255, 0} // White
#define SILHOUETTE_DEFAULT_COLOR	{255, 255, 255, 0} // White
#define FOG_DEFAULT_COLOR			{200, 200, 200, 0} // Light Grey

#define DEFAULT_PROJECTION_PLANE_DISTANCE 20
// We start the fog at the start of the camera, please change both together.
#define DEAULT_VIEW_PARAMETERS 0, 0, 10
#define FOG_START					 10.0
#define FOG_END						-5.0

#define DEFAULT_FINENESS 20.0

#define DEFAULT_DEPTH -2

#define DEFAULT_MOTION_BLUR 0.25

// Various color masking
#define COLORREF_TO_RGBQUAD(x) {(BYTE)((x & 0xff0000) >> 16), (BYTE)((x & 0xff00) >> 8), (BYTE)(x & 0xff), 0}
#define ARGB_TO_RGBA(x) (((x & 0xff000000) >> 24) + ((x & 0x00ff0000) << 8) + ((x & 0x0000ff00) << 8) + ((x & 0x000000ff) << 8))
#define RGBA_TO_ARGB(x) (((x & 0xff000000) >> 8) + ((x & 0x00ff0000) >> 8) + ((x & 0x0000ff00) >> 8) + ((x & 0x000000ff) << 24))
// FOR RGBQUAD
#define RED(x)		((BYTE)((x & 0x00ff0000) >> 16))
#define GREEN(x)	((BYTE)((x & 0x0000ff00) >> 8))
#define BLUE(x)		((BYTE)((x & 0x000000ff) >> 0))
#define CLAMP(x)    ((x > 1) ? 1 : (x < 0) ? 0 : x)

const RGBQUAD max_rgb = { 255, 255, 255, 0 };
//const unsigned int max_rgb_uint = ((unsigned int *)&max_rgb)

// Declarations
/* Creates a view matrix which simulates changing the position of the camera
 * @x,y,z - camera coordinates in *world* view system
 * returns a matrix which maps every object to view space
*/
Matrix createViewMatrix(double x, double y, double z);
Matrix createTranslationMatrix(Vector &v);
Matrix createTranslationMatrix(double x, double y, double z);

unsigned int recursiveGetColor(struct PixelNode *node);

// this enum prob isnt needed, beacuse of built in axis info - m_nAxis
enum Axis {
	X_AXIS,
	Y_AXIS,
	Z_AXIS,
	NUM_OF_AXES
};

enum ShadingMode {
	SHADING_M_FLAT,
	SHADING_M_GOURAUD,
	SHADING_M_PHONG,
};

class IritPolygon;

struct IritPoint {
	Vector vertex;

	Vector normal_irit;
	Vector normal_calc;

	/* Parametric values (map polygons vertices to [-1, 1]^2 space */
	float U, V;

	struct IritPoint *next_point;
};

struct State {
	bool show_vertex_normal;
	bool show_polygon_normal;
	bool object_frame;
	bool is_perspective_view;
	bool object_transform;
	bool is_default_color;
	bool use_calc_normals;
	bool invert_normals;
	bool backface_culling;
	bool only_mesh;
	bool save_to_png;
	bool png_stretch;
	bool background_png;
	bool show_silhouette;
	bool fog;
	bool transparency;
	bool motion_blur;

	double projection_plane_distance;
	double sensitivity;
	double fineness;
	double motion_blur_drag;

	bool is_axis_active[3];

	Matrix ratio_mat;
	Matrix coord_mat;
	Matrix center_mat;
	Matrix world_mat;
	Matrix object_mat;
	Matrix ortho_mat;
	Matrix view_mat;

	Matrix orthogonal_to_perspective;
	Matrix perspective_mat;
	Matrix screen_mat;

	RGBQUAD wire_color;
	RGBQUAD frame_color;
	RGBQUAD bg_color;
	RGBQUAD normal_color;
	RGBQUAD fog_color;

	// For hidden face removal and transparency
	struct PixelNode **z_buffer;

	enum ShadingMode shading_mode;

	int m_nLightShading;			// shading: Flat, Gouraud.

	double m_lMaterialAmbient;		// The Ambient in the scene
	double m_lMaterialDiffuse;		// The Diffuse in the scene
	double m_lMaterialSpecular;		// The Specular in the scene
	int m_nMaterialCosineFactor;		// The cosine factor for the specular

	LightParams m_lights[MAX_LIGHT];	//configurable lights array
	LightParams m_ambientLight;		//ambient light (only RGB is used)

	// 2D texture
	PngWrapper *texture_png;
	double max_u, max_v, min_u, min_v;
};

struct PolygonList {
	IPPolygonStruct *skel_polygon;
	IritPolygon *polygon;
	PolygonList *next;
};

struct VertexList {
	IPVertexStruct *vertex;
	PolygonList *polygon_list;
	VertexList *next;
};

struct PixelNode {
	unsigned int color; // Technically RGBQUAD
	double alpha;
	double depth;
	PixelNode* next;
};

struct IntersectionPoint {
	int x;
	int y;
	double z; 

	// will need to contain extrapolated normal as well
	struct threed_line *containing_line;

	// These hold the intersection point's 3D normal and position
	Vector point_normal;
	Vector point_pos;

	/* Light related variables */
	Vector polygon_shade;
	Vector point_shade;

	/* 2D Texture related variables */
	double point_U;
	double point_V;
};

/* Represents a 3 dimensional line
*/
struct threed_line {
	int x1, y1;
	double z1;
	int x2, y2;
	double z2;

	IritPoint p1;
	IritPoint p2;
	
	Vector polygon_normal;

	bool operator<(threed_line &l1) {
		return this->ymin() < l1.ymin();
	}

	int ymin() {
		return min(this->y1, this->y2);
	}

	int ymax() {
		return max(this->y1, this->y2);
	}
};

class IritPolygon {
	int m_point_nr;
	struct IritPoint *m_points;

	IritPolygon *m_next_polygon;

	struct threed_line *lines;
	int lines_nr;

	/* This function uses the lines array to do a
	   scan conversion painting of the figure */
	void paintPolygon(int width, int height, RGBQUAD color, State &state,
					  Vector &polygon_normal, Vector p_center_of_mass, double alpha);
	/* Transform polygon's normal according to a given transformation */
	Vector transformPolygonNormal(Matrix &transformation, struct State &state, int &sign);

	void calculate_2d_intersection_texture(struct IntersectionPoint &point, struct State state);

	/* Return the appropriate 2D intensity of a given pixel during scan conversion */
	Vector get2DPixelTexture(struct IntersectionPoint &intersecting_x1, struct IntersectionPoint &intersecting_x2,
							 double t, struct State &state);

public:
	Vector center_of_mass;

	Vector normal_irit;
	Vector normal_calc;

	/* 2D parametric representation range */
	float max_U, max_V;
	float min_U, min_V;

	IritPolygon();

	~IritPolygon();
	
	bool addPoint(struct IritPoint &point);

	/* Creates an homogenious with a normal */
	bool addPoint(double &x, double &y, double &z, double &normal_irit_x, double &normal_irit_y,
		double &normal_irit_z, double &normal_calc_x, double &normal_calc_y,
		double &normal_calc_z);

	bool addPoint(IPVertexStruct *vertex, Vector normal_calc);

	IritPolygon *getNextPolygon();

	void setNextPolygon(IritPolygon *polygon);

	/* Draws an polygon (draw lines between each of its points).
	 * Each of the points is multiplied by a transformation matrix.
	 * @pDCToUse - a pointer to the the DC with which the
	 *				polygon is drawn
	 * @state - world state (current coordinate system, scaling function
	 *					 etc.)
	 * @normal_transform - a transformation matrix for the normal vectors
	 * @vertex_transform - a transformation matrix for the the vertices (each
	 *						vertex is multiplied by this matrix before being drawn
	*/
	void draw(int width, int height, RGBQUAD color, struct State &state,
			  Matrix &vertex_transform, double alpha);

	// Operators overriding
	IritPolygon &operator++();

	int getPointsNr();
};

/* This class represents an object in the IRIT world. An object is formed from
 * a list of polygons (each represented by its vertices)
 */
class IritObject {
	int m_polygons_nr;
	IritPolygon *m_polygons;
	IritPolygon *m_iterator;

public:
	RGBQUAD object_color;

	VertexList *vertex_connection;
	PolygonList *polygon_connection;

	double alpha;

	IritObject();
	
	~IritObject();

	/* Received a pointer to a new polygon, and adds this polygon to the list
	 * of polygons in the object. The polygon is always added as the last
	 * polygon.
	 * @polygon - a pointer to the polygon to add
	 */
	void addPolygonP(IritPolygon *polygon);

	/* Creates an empty polygon and returns a pointer to it.
	 * the polygon is added to the list of polygons of the object
	 * as the last polygon.
	 * returns null pointer on memory failure
	 */
	IritPolygon *createPolygon();

	/* Draws an object (each of its polygons at a time). Each
	 * of the points of the object are multiplied by a transformation
	 * matrix.
	 * @pDCToUse - a pointer to the the DC with which the
	 *				object is drawn
	 * @state - world state (current coordinate system, scaling function
	 *					 etc.)
	 * @normal_transform - a transformation matrix for the normal vectors
	 * @vertex_transform - a transformation matrix for the the vertices (each
	 *						vertex is multiplied by this matrix before being drawn
	*/
	void draw(int width, int height, State &state,
			  Matrix &vertex_transform, double figure_alpha, bool global_alpha);

	void getMaxMinUV(double &max_u, double &max_v, double &min_u, double &min_v);
};

/* This class represents an Irit Figure which is build from many objects, which have many
 * polygons, which have many dots. It can be described like this:
 *	Figure -> Object
 *			  Object
 *			  Object ->
 *					Polygon
 *					Polygon
 *					Polygon ->
 *						Point
 *						Point
*/
class IritFigure {
	int m_objects_nr;
	IritObject **m_objects_arr;

	void drawFrame(int width, int height, struct State state, Matrix &transform);

public:

	Matrix world_mat;
	Matrix object_mat;
	Matrix normalization_mat;

	Matrix backup_transformation_matrix;

	double alpha;
	bool has_global_alpha;

	// Bounding frame params
	Vector max_bound_coord,
		min_bound_coord;

	IritFigure();

	~IritFigure();

	/* Saves the current object or world transformation matrix
	 * (depending on what is being modified at the moment).
	 * This is to save the original location when changing the figure's
	 * position using the mouse
	 */
	void backup_transformation(State &state);

	/* Receieves a pointer to an object and adds the object
	 * the objects' list.
	 * @p_object - a pointer to the object that needs to be added
	 * returns false on allocation error and true otherwise
	*/
	bool addObjectP(IritObject *p_object);

	/* Creates an empty object and returns a pointer to it.
	 * the object is added to the list of objects in the IritWorld.
	 * It is added as the last object
	 * return NULL on memory allocation error
	*/
	IritObject *createObject();

	void draw(int width, int height, Matrix transform, State &state);

	/* Adjust figures points so that it is bounded in a [-1,1]^3 cube */
	void normalizeFigure();

	bool isEmpty();
};

class IritWorld {

	int m_figures_nr;
	IritFigure **m_figures_arr;

	/* Returns the perspective matrix */
	Matrix getPerspectiveMatrix(const double &angleOfView, const double &near_z, const double &far_z);

	/* Creates a projection matrix */
	Matrix createProjectionMatrix();


	/* Project a point to screen space using the given transformation and
	 * the world's screen matrix.
	 * This transformation is done in *screen axes*, which means that the (0,0) point
	 * is at the upper-left side of the screen and that the y values grow down
	 */
	Vector projectPoint_in_screen_axes(Vector &td_point, Matrix &transformation);

	// A private bitmap to create motion blur
	unsigned int *motion_blur_bitmap;

public:
	// World state
	struct State state;

	PngWrapper *background;
	
	// PNG Size
	int png_height;
	int png_width;

	// Bounding frame params
	Vector max_bound_coord,
		   min_bound_coord;

	IritWorld();

	~IritWorld();

	/* Sets a coordinate system (with axes and origin point)
	 * @axes[3] - three orthonormal vectors in world space
	 * @axes_origin - origin point in world space
	*/
	void setScreenMat(Vector axes[NUM_OF_AXES], Vector &axes_origin, int screen_widht, int screen_height);

	void setOrthoMat(void);


	/* Creates an empty figure and returns a pointer to it.
	 * the figure is added to the list of figures in the IritWorld.
	 * It is added as the last object
	 * return NULL on memory allocation error
	*/
	IritFigure *createFigure();

	/* Receieves a pointer to a figure and adds the figure
	 * the figures' list.
	 * @p_figure - a pointer to the figure that needs to be added
	 * returns false on allocation error and true otherwise
	*/
	bool addFigureP(IritFigure *p_figure);

	/* Returns a reference to the last figure in the figures list */
	IritFigure &getLastFigure();

	/* Checks whether the point is inside the bounding box of one of the
	 * figures. If so, choose the first figure that matches.
	 * If the point isn't inside a figure's bounding box, return NULL
	 */
	IritFigure *getFigureInPoint(CPoint &point);

	bool isEmpty();

	void draw(int *bitmap, int width, int height);
};