#include "stdafx.h"
#include "iritSkel.h"
#include "IritObjects.h"

/*****************************************************************************
* Skeleton for an interface to a parser to read IRIT data files.			 *
******************************************************************************
* (C) Gershon Elber, Technion, Israel Institute of Technology                *
******************************************************************************
* Written by:  Gershon Elber				Ver 1.0, Feb 2002				 *
* Minimal changes made by Amit Mano			November 2008					 *
******************************************************************************/

#define MIN(x, y) ((x) < (y)) ? (x) : (y)
#define MAX(x, y) ((x) > (y)) ? (x) : (y)

#define EPSILON 0.005

bool areVerticesEqual(IPVertexStruct *first, IPVertexStruct *second);

void updateBoundingFrameLimits(IPVertexStruct *vertex);

IPFreeformConvStateStruct CGSkelFFCState = {
	FALSE,          /* Talkative */
	FALSE,          /* DumpObjsAsPolylines */
	TRUE,           /* DrawFFGeom */
	FALSE,          /* DrawFFMesh */
	{ 10, 10, 10 }, /* 10 isocurves peru/v/w direction. */
	100,            /* 100 point samples along a curve. */
	SYMB_CRV_APPROX_UNIFORM,  /* CrvApproxMethod */
	FALSE,   /* ShowIntrnal */
	FALSE,   /* CubicCrvsAprox */
	20,      /* Polygonal FineNess */
	FALSE,   /* ComputeUV */
	TRUE,    /* ComputeNrml */
	FALSE,   /* FourPerFlat */
	0,       /* OptimalPolygons */
	FALSE,   /* BBoxGrid */
	TRUE,    /* LinearOnePolyFlag */
	FALSE
};

//CGSkelProcessIritDataFiles(argv + 1, argc - 1);

extern IritWorld world;

bool is_first_polygon;
bool is_first_figure;

VertexList *connectivity;

PolygonList *all_polygons;

/*****************************************************************************
* DESCRIPTION:                                                               *
* Main module of skeleton - Read command line and do what is needed...	     *
*                                                                            *
* PARAMETERS:                                                                *
*   FileNames:  Files to open and read, as a vector of strings.              *
*   NumFiles:   Length of the FileNames vector.								 *
*                                                                            *
* RETURN VALUE:                                                              *
*   bool:		false - fail, true - success.                                *
*****************************************************************************/
bool CGSkelProcessIritDataFiles(CString &FileNames, int NumFiles)
{
	IPObjectStruct *PObjects;
	IrtHmgnMatType CrntViewMat;

	/* Get the data files: */
	IPSetFlattenObjects(FALSE);
	CStringA CStr(FileNames);
	if ((PObjects = IPGetDataFiles((const char* const *)&CStr, 1/*NumFiles*/, TRUE, FALSE)) == NULL)
		return false;
	PObjects = IPResolveInstances(PObjects);

	if (IPWasPrspMat)
		MatMultTwo4by4(CrntViewMat, IPViewMat, IPPrspMat);
	else
		IRIT_GEN_COPY(CrntViewMat, IPViewMat, sizeof(IrtHmgnMatType));

	/* Here some useful parameters to play with in tesselating freeforms: */
	CGSkelFFCState.FineNess = 20;   /* Res. of tesselation, larger is finer. */
	CGSkelFFCState.ComputeUV = TRUE;   /* Wants UV coordinates for textures. */
	CGSkelFFCState.FourPerFlat = TRUE;/* 4 poly per ~flat patch, 2 otherwise.*/
	CGSkelFFCState.LinearOnePolyFlag = TRUE;    /* Linear srf gen. one poly. */

	// Need to be initialized before any object is proccesed;
	is_first_polygon = true;

	is_first_figure = true;

	/* Traverse ALL the parsed data, recursively. */
	IPTraverseObjListHierarchy(PObjects, CrntViewMat,
        CGSkelDumpOneTraversedObject);

	world.setOrthoMat();

	return true;
}

/*****************************************************************************
* DESCRIPTION:                                                               *
*   Call back function of IPTraverseObjListHierarchy. Called on every non    *
* list object found in hierarchy.                                            *
*                                                                            *
* PARAMETERS:                                                                *
*   PObj:       Non list object to handle.                                   *
*   Mat:        Transformation matrix to apply to this object.               *
*   Data:       Additional data.                                             *
*                                                                            *
* RETURN VALUE:                                                              *
*   void									                                 *
*****************************************************************************/
void CGSkelDumpOneTraversedObject(IPObjectStruct *PObj,
                                  IrtHmgnMatType Mat,
                                  void *Data)
{
	IPObjectStruct *PObjs;

	if (IP_IS_FFGEOM_OBJ(PObj))
		PObjs = IPConvertFreeForm(PObj, &CGSkelFFCState);
	else
		PObjs = PObj;

	if (is_first_figure) {
		// We don't create a new figure if it has no objects
		world.createFigure();
		is_first_figure = false;
	}

	for (PObj = PObjs; PObj != NULL; PObj = PObj -> Pnext)
		if (!CGSkelStoreData(PObj)) 
			exit(1);
}

/*****************************************************************************
* DESCRIPTION:                                                               *
*   Prints the data from given geometry object.								 *
*                                                                            *
* PARAMETERS:                                                                *
*   PObj:       Object to print.                                             *
*   Indent:     Column of indentation.                                       *
*                                                                            *
* RETURN VALUE:                                                              *
*   bool:		false - fail, true - success.                                *
*****************************************************************************/
bool CGSkelStoreData(IPObjectStruct *PObj)
{
	int i, num_of_vertices;
	const char *Str;
	double RGB[3], Transp,
		center_mass_x = 0, center_mass_y = 0, center_mass_z = 0;
	IPPolygonStruct *PPolygon;
	IPVertexStruct *PVertex;

	connectivity = new VertexList();
	all_polygons = new PolygonList();

	PolygonList *current_polygon;
	VertexList *current_vertex;

	const IPAttributeStruct *Attrs =
        AttrTraceAttributes(PObj -> Attr, PObj -> Attr);
	IritFigure &figure = world.getLastFigure();
	IritObject *irit_object = figure.createObject();

	Vector first, second, third, vertex;

	assert(irit_object);

	if (PObj->ObjType != IP_OBJ_POLY) {
		AfxMessageBox(_T("Non polygonal object detected and ignored"));
		return true;
	}

	/* You can use IP_IS_POLYGON_OBJ(PObj) and IP_IS_POINTLIST_OBJ(PObj)
	   to identify the type of the object*/

	if (CGSkelGetObjectColor(PObj, RGB))
	{
		/* color code */
	}
	if (CGSkelGetObjectTransp(PObj, &Transp))
	{
		/* transparency code */
	}
	if ((Str = CGSkelGetObjectTexture(PObj)) != NULL)
	{
		/* volumetric texture code */
	}
	if ((Str = CGSkelGetObjectPTexture(PObj)) != NULL)
	{
		/* parametric texture code */
	}
	if (Attrs != NULL)
	{
		printf("[OBJECT\n");
		while (Attrs) {
			/* attributes code */
			Attrs = AttrTraceAttributes(Attrs, NULL);
		}
	}

	//  First pass - build linked lists
	for (PPolygon = PObj->U.Pl; PPolygon != NULL; PPolygon = PPolygon->Pnext) {

		IritPolygon *new_polygon = new IritPolygon();

		// List of all polygons
		current_polygon = all_polygons;
		if (current_polygon->polygon == nullptr) { // Populate the first node
			current_polygon->skel_polygon = PPolygon;
			current_polygon->polygon = new_polygon;
		} else {
			while (current_polygon->next != nullptr) {
				current_polygon = current_polygon->next;
			}
			PolygonList *new_node = new PolygonList();
			current_polygon->next = new_node;
			current_polygon->next->polygon = new_polygon;
			current_polygon->next->skel_polygon = PPolygon;
		}

		// List of all vertices, with connectiviy information
		if (PPolygon->PVertex == NULL) {
			AfxMessageBox(_T("Dump: Attemp to dump empty polygon"));
			return false; // might cause memory leakage
		}

		PVertex = PPolygon->PVertex;
		do {
			current_vertex = connectivity;
			if (current_vertex->vertex == nullptr) { // Populate the first node
				current_vertex->vertex = PVertex;
				current_vertex->polygon_list = new PolygonList();
				current_vertex->polygon_list->skel_polygon = PPolygon;
				current_vertex->polygon_list->polygon = new_polygon;
			} else {
				while (!areVerticesEqual(current_vertex->vertex, PVertex) && current_vertex->next != nullptr) {
					current_vertex = current_vertex->next;
				}
				if (areVerticesEqual(current_vertex->vertex, PVertex)) { // We found the vertex
					current_polygon = current_vertex->polygon_list;
					while (current_polygon->skel_polygon != PPolygon && current_polygon->next != nullptr) {
						current_polygon = current_polygon->next;
					}
					if (current_polygon->next == nullptr) {
						current_polygon->next = new PolygonList();
						current_polygon->next->skel_polygon = PPolygon;
						current_polygon->next->polygon = new_polygon;
						// If we found the polygon, it means the vertex was the one 
						// who closes the loop for the polygon, so no need for the else clause
					}
				} else { // Vertex is not on the list
					current_vertex->next = new VertexList();
					current_vertex->next->vertex = PVertex;
					current_vertex->next->polygon_list = new PolygonList();
					current_vertex->next->polygon_list->skel_polygon = PPolygon;
					current_vertex->next->polygon_list->polygon = new_polygon;
				}
			}
			PVertex = PVertex->Pnext;
		} while (PVertex != nullptr && PVertex != PPolygon->PVertex);
	}

	// Second pass - calculate polygon normals
	current_polygon = all_polygons;
	while (current_polygon != nullptr) {
		center_mass_x = 0;
		center_mass_y = 0;
		center_mass_z = 0;
		IritPolygon *irit_polygon = current_polygon->polygon;
		PPolygon = current_polygon->skel_polygon;
		if (IP_HAS_PLANE_POLY(current_polygon->skel_polygon)) {
			irit_polygon->is_irit_normal = true;
			for (int j = 0; j < 3; j++) {
				irit_polygon->normal_end[j] = current_polygon->skel_polygon->Plane[j];
			}
		} else {
			PVertex = PPolygon->PVertex;
			first = Vector(PVertex->Coord[0], PVertex->Coord[1], PVertex->Coord[2], 1);
			PVertex = PVertex->Pnext;
			second = Vector(PVertex->Coord[0], PVertex->Coord[1], PVertex->Coord[2], 1);
			PVertex = PVertex->Pnext;
			third = Vector(PVertex->Coord[0], PVertex->Coord[1], PVertex->Coord[2], 1);

			irit_polygon->normal_end = (second - first) ^ (third - second);
			irit_polygon->normal_end.Normalize();
			irit_polygon->normal_end[3] = 1;
		}

		// FOR SMALLER POLYGON NORMALS
		irit_polygon->normal_end = irit_polygon->normal_end * 0.3;

		// Find center of mass
		PVertex = PPolygon->PVertex;
		num_of_vertices = 0;
		do {
			center_mass_x += PVertex->Coord[0];
			center_mass_y += PVertex->Coord[1];
			center_mass_z += PVertex->Coord[2];
			num_of_vertices++;
			PVertex = PVertex->Pnext;
		} while (PVertex != nullptr && PVertex != PPolygon->PVertex);
		center_mass_x /= num_of_vertices;
		center_mass_y /= num_of_vertices;
		center_mass_z /= num_of_vertices;

		irit_polygon->normal_start = Vector(center_mass_x, center_mass_y, center_mass_z, 1);
		irit_polygon->normal_end = irit_polygon->normal_end + irit_polygon->normal_start;
		irit_polygon->normal_end[3] = 1;

		// calculate next polygon
		current_polygon = current_polygon->next;
	}

	// Third pass - populate the world
	current_polygon = all_polygons;
	do {
		int polygon_count = 0;
		bool is_irit_normal;
		PolygonList *iterator;
		Vector vertex_normal;

		irit_object->addPolygonP(current_polygon->polygon);

		PVertex = current_polygon->skel_polygon->PVertex;
		do { // Assume at least one vertex in the polygon
			vertex_normal = Vector(0, 0, 0, 1);
			is_irit_normal = false;
			if (IP_HAS_NORMAL_VRTX(PVertex)) {
				is_irit_normal = true;
			} else {
				// Find vertex in vertices list
				current_vertex = connectivity;
				while (current_vertex->vertex != PVertex) { // It should find it
					current_vertex = current_vertex->next;
				}
				iterator = current_vertex->polygon_list;
				while (iterator != nullptr) {
					vertex_normal += current_polygon->polygon->normal_end - current_polygon->polygon->normal_start;
					polygon_count++;
					iterator = iterator->next;
				}
				vertex_normal = vertex_normal * (1.0 / polygon_count);
				vertex_normal.Normalize();
			}
			current_polygon->polygon->addPoint(PVertex, is_irit_normal, vertex_normal);

			if (is_first_polygon) {
				is_first_polygon = false;
				world.max_bound_coord[0] = PVertex->Coord[0];
				world.min_bound_coord[0] = PVertex->Coord[0];
				world.max_bound_coord[1] = PVertex->Coord[1];
				world.min_bound_coord[1] = PVertex->Coord[1];
				world.max_bound_coord[2] = PVertex->Coord[2];
				world.min_bound_coord[2] = PVertex->Coord[2];
			} else {
				updateBoundingFrameLimits(PVertex);
			}

			PVertex = PVertex->Pnext;
		} while (PVertex != current_polygon->skel_polygon->PVertex && PVertex != NULL);		

		current_polygon = current_polygon->next;
	} while (current_polygon != nullptr);

	/* Close the object. */
	return true;
}

void updateBoundingFrameLimits(IPVertexStruct *vertex)
{
	IritFigure &figure = world.getLastFigure();

	world.min_bound_coord[0] = MIN(world.min_bound_coord[0], vertex->Coord[0]);
	world.max_bound_coord[0] = MAX(world.max_bound_coord[0], vertex->Coord[0]);
	world.min_bound_coord[1] = MIN(world.min_bound_coord[1], vertex->Coord[1]);
	world.max_bound_coord[1] = MAX(world.max_bound_coord[1], vertex->Coord[1]);
	world.min_bound_coord[2] = MIN(world.min_bound_coord[2], vertex->Coord[2]);
	world.max_bound_coord[2] = MAX(world.max_bound_coord[2], vertex->Coord[2]);

	figure.min_bound_coord[0] = MIN(figure.min_bound_coord[0], vertex->Coord[0]);
	figure.max_bound_coord[0] = MAX(figure.max_bound_coord[0], vertex->Coord[0]);
	figure.min_bound_coord[1] = MIN(figure.min_bound_coord[1], vertex->Coord[1]);
	figure.max_bound_coord[1] = MAX(figure.max_bound_coord[1], vertex->Coord[1]);
	figure.min_bound_coord[2] = MIN(figure.min_bound_coord[2], vertex->Coord[2]);
	figure.max_bound_coord[2] = MAX(figure.max_bound_coord[2], vertex->Coord[2]);
}

bool areVerticesEqual(IPVertexStruct *first, IPVertexStruct *second) {
	if ((abs(first->Coord[0] - second->Coord[0]) < EPSILON) &&
		(abs(first->Coord[1] - second->Coord[1]) < EPSILON) &&
		(abs(first->Coord[2] - second->Coord[2]) < EPSILON)) {
		return true;
	}
	return false;
}

/*****************************************************************************
* DESCRIPTION:                                                               *
*   Returns the color of an object.                                          *
*                                                                            *
* PARAMETERS:                                                                *
*   PObj:   Object to get its color.                                         *
*   RGB:    as 3 floats in the domain [0, 1].                                *
*                                                                            *
* RETURN VALUE:                                                              *
*   int:    TRUE if object has color, FALSE otherwise.                       *
*****************************************************************************/
int CGSkelGetObjectColor(IPObjectStruct *PObj, double RGB[3])
{
	static int TransColorTable[][4] = {
		{ /* BLACK	*/   0,    0,   0,   0 },
		{ /* BLUE	*/   1,    0,   0, 255 },
		{ /* GREEN	*/   2,    0, 255,   0 },
		{ /* CYAN	*/   3,    0, 255, 255 },
		{ /* RED	*/   4,  255,   0,   0 },
		{ /* MAGENTA 	*/   5,  255,   0, 255 },
		{ /* BROWN	*/   6,   50,   0,   0 },
		{ /* LIGHTGRAY	*/   7,  127, 127, 127 },
		{ /* DARKGRAY	*/   8,   63,  63,  63 },
		{ /* LIGHTBLUE	*/   9,    0,   0, 255 },
		{ /* LIGHTGREEN	*/   10,   0, 255,   0 },
		{ /* LIGHTCYAN	*/   11,   0, 255, 255 },
		{ /* LIGHTRED	*/   12, 255,   0,   0 },
		{ /* LIGHTMAGENTA */ 13, 255,   0, 255 },
		{ /* YELLOW	*/   14, 255, 255,   0 },
		{ /* WHITE	*/   15, 255, 255, 255 },
		{ /* BROWN	*/   20,  50,   0,   0 },
		{ /* DARKGRAY	*/   56,  63,  63,  63 },
		{ /* LIGHTBLUE	*/   57,   0,   0, 255 },
		{ /* LIGHTGREEN	*/   58,   0, 255,   0 },
		{ /* LIGHTCYAN	*/   59,   0, 255, 255 },
		{ /* LIGHTRED	*/   60, 255,   0,   0 },
		{ /* LIGHTMAGENTA */ 61, 255,   0, 255 },
		{ /* YELLOW	*/   62, 255, 255,   0 },
		{ /* WHITE	*/   63, 255, 255, 255 },
		{		     -1,   0,   0,   0 }
	};
	int i, j, Color, RGBIColor[3];

	if (AttrGetObjectRGBColor(PObj,
		&RGBIColor[0], &RGBIColor[1], &RGBIColor[2])) {
			for (i = 0; i < 3; i++)
				RGB[i] = RGBIColor[i] / 255.0;

			return TRUE;
	}
	else if ((Color = AttrGetObjectColor(PObj)) != IP_ATTR_NO_COLOR) {
		for (i = 0; TransColorTable[i][0] >= 0; i++) {
			if (TransColorTable[i][0] == Color) {
				for (j = 0; j < 3; j++)
					RGB[j] = TransColorTable[i][j+1] / 255.0;
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*****************************************************************************
* DESCRIPTION:                                                               *
*   Returns the volumetric texture of an object, if any.                     *
*                                                                            *
* PARAMETERS:                                                                *
*   PObj:   Object to get its volumetric texture.                            *
*                                                                            *
* RETURN VALUE:                                                              *
*   char *:    Name of volumetric texture map to apply, NULL if none.        *
*****************************************************************************/
const char *CGSkelGetObjectTexture(IPObjectStruct *PObj)
{
	return AttrGetObjectStrAttrib(PObj, "texture");
}

/*****************************************************************************
* DESCRIPTION:                                                               *
*   Returns the parametric texture of an object, if any.                     *
*                                                                            *
* PARAMETERS:                                                                *
*   PObj:   Object to get its parametric texture.                            *
*                                                                            *
* RETURN VALUE:                                                              *
*   char *:    Name of parametric texture map to apply, NULL if none.        *
*****************************************************************************/
const char *CGSkelGetObjectPTexture(IPObjectStruct *PObj)
{
	return AttrGetObjectStrAttrib(PObj, "ptexture");
}

/*****************************************************************************
* DESCRIPTION:                                                               *
*   Returns the transparency level of an object, if any.                     *
*                                                                            *
* PARAMETERS:                                                                *
*   PObj:   Object to get its volumetric texture.                            *
*   Transp: Transparency level between zero and one.                         *
*                                                                            *
* RETURN VALUE:                                                              *
*   int:    TRUE if object has transparency, FALSE otherwise.                *
*****************************************************************************/
int CGSkelGetObjectTransp(IPObjectStruct *PObj, double *Transp)
{
	*Transp = AttrGetObjectRealAttrib(PObj, "transp");

	return !IP_ATTR_IS_BAD_REAL(*Transp);
}

