// CGWorkView.cpp : implementation of the CCGWorkView class
//
#include "stdafx.h"
#include "CGWork.h"

#include "CGWorkDoc.h"
#include "CGWorkView.h"

#include <math.h>

#include <iostream>
using std::cout;
using std::endl;
#include "MaterialDlg.h"
#include "LightDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PngWrapper.h"
#include "iritSkel.h"

#include "IritObjects.h"


// For Status Bar access
#include "MainFrm.h"

// Use this macro to display text messages in the status bar.
#define STATUS_BAR_TEXT(str) (((CMainFrame*)GetParentFrame())->getStatusBar().SetWindowText(str))

IritWorld world;

static CPoint mouse_location;

static bool is_mouse_down;

void resetWorld(void);

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView

IMPLEMENT_DYNCREATE(CCGWorkView, CView)

BEGIN_MESSAGE_MAP(CCGWorkView, CView)
	//{{AFX_MSG_MAP(CCGWorkView)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_LOAD, OnFileLoad)
	ON_COMMAND(ID_VIEW_ORTHOGRAPHIC, OnViewOrthographic)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ORTHOGRAPHIC, OnUpdateViewOrthographic)
	ON_COMMAND(ID_VIEW_PERSPECTIVE, OnViewPerspective)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PERSPECTIVE, OnUpdateViewPerspective)
	ON_COMMAND(ID_ACTION_ROTATE, OnActionRotate)
	ON_UPDATE_COMMAND_UI(ID_ACTION_ROTATE, OnUpdateActionRotate)
	ON_COMMAND(ID_ACTION_SCALE, OnActionScale)
	ON_UPDATE_COMMAND_UI(ID_ACTION_SCALE, OnUpdateActionScale)
	ON_COMMAND(ID_ACTION_TRANSLATE, OnActionTranslate)
	ON_UPDATE_COMMAND_UI(ID_ACTION_TRANSLATE, OnUpdateActionTranslate)
	ON_COMMAND(ID_AXIS_X, OnAxisX)
	ON_UPDATE_COMMAND_UI(ID_AXIS_X, OnUpdateAxisX)
	ON_COMMAND(ID_AXIS_Y, OnAxisY)
	ON_UPDATE_COMMAND_UI(ID_AXIS_Y, OnUpdateAxisY)
	ON_COMMAND(ID_AXIS_Z, OnAxisZ)
	ON_UPDATE_COMMAND_UI(ID_AXIS_Z, OnUpdateAxisZ)
	ON_COMMAND(ID_POLYGON_NORMAL, OnPolygonNormals)
	ON_UPDATE_COMMAND_UI(ID_POLYGON_NORMAL, OnUpdatePolygonNormals)
	ON_COMMAND(ID_VERTEX_NORMAL, OnVertexNormals)
	ON_UPDATE_COMMAND_UI(ID_VERTEX_NORMAL, OnUpdateVertexNormals)
	ON_COMMAND(ID_OBJECT_FRAME, OnObjectFrame)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_FRAME, OnUpdateObjectFrame)
	ON_COMMAND(ID_OBJECT_COLOR, OnObjectColor)
	ON_COMMAND(ID_BG_COLOR, OnBGColor)
	ON_COMMAND(ID_WORLD_TRANSFORM, OnWorldTransform)
	ON_UPDATE_COMMAND_UI(ID_WORLD_TRANSFORM, OnUpdateWorldTransform)
	ON_COMMAND(ID_OBJECT_TRANSFORM, OnObjectTransform)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_TRANSFORM, OnUpdateObjectTransform)
	ON_COMMAND(ID_LIGHT_SHADING_FLAT, OnLightShadingFlat)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_SHADING_FLAT, OnUpdateLightShadingFlat)
	ON_COMMAND(ID_LIGHT_SHADING_GOURAUD, OnLightShadingGouraud)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_SHADING_GOURAUD, OnUpdateLightShadingGouraud)
	ON_COMMAND(ID_LIGHT_CONSTANTS, OnLightConstants)
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


// A patch to fix GLaux disappearance from VS2005 to VS2008
void auxSolidCone(GLdouble radius, GLdouble height) {
        GLUquadric *quad = gluNewQuadric();
        gluQuadricDrawStyle(quad, GLU_FILL);
        gluCylinder(quad, radius, 0.0, height, 20, 20);
        gluDeleteQuadric(quad);
}

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView construction/destruction

CCGWorkView::CCGWorkView()
{
	// Set default values
	m_nAxis = ID_AXIS_X;
	m_nAction = ID_ACTION_ROTATE;

	// Init the state machine
	resetWorld();

	m_nLightShading = ID_LIGHT_SHADING_FLAT;

	m_lMaterialAmbient = 0.2;
	m_lMaterialDiffuse = 0.8;
	m_lMaterialSpecular = 1.0;
	m_nMaterialCosineFactor = 32;

	//init the first light to be enabled
	m_lights[LIGHT_ID_1].enabled=true;
	m_pDbBitMap = NULL;
	m_pDbDC = NULL;
}

CCGWorkView::~CCGWorkView()
{
}


/////////////////////////////////////////////////////////////////////////////
// CCGWorkView diagnostics

#ifdef _DEBUG
void CCGWorkView::AssertValid() const
{
	CView::AssertValid();
}

void CCGWorkView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CCGWorkDoc* CCGWorkView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCGWorkDoc)));
	return (CCGWorkDoc*)m_pDocument;
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CCGWorkView Window Creation - Linkage of windows to CGWork

BOOL CCGWorkView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	// An CGWork window must be created with the following
	// flags and must NOT include CS_PARENTDC for the
	// class style.

	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	return CView::PreCreateWindow(cs);
}



int CCGWorkView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	InitializeCGWork();

	return 0;
}


// This method initialized the CGWork system.
BOOL CCGWorkView::InitializeCGWork()
{
	m_pDC = new CClientDC(this);
	
	if ( NULL == m_pDC ) { // failure to get DC
		::AfxMessageBox(CString("Couldn't get a valid DC."));
		return FALSE;
	}

	CRect r;
	GetClientRect(&r);
	m_pDbDC = new CDC();
	m_pDbDC->CreateCompatibleDC(m_pDC);
	SetTimer(1, 1000, NULL);
	m_pDbBitMap = CreateCompatibleBitmap(m_pDC->m_hDC, r.right, r.bottom);	
	m_pDbDC->SelectObject(m_pDbBitMap);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CCGWorkView message handlers


void CCGWorkView::OnSize(UINT nType, int cx, int cy) 
{
	Vector axes[NUM_OF_AXES];
	Vector origin;

	CView::OnSize(nType, cx, cy);

	if ( 0 >= cx || 0 >= cy ) {
		return;
	}

	// save the width and height of the current window
	m_WindowWidth = cx;
	m_WindowHeight = cy;

	// compute the aspect ratio
	// this will keep all dimension scales equal
	m_AspectRatio = (GLdouble)m_WindowWidth/(GLdouble)m_WindowHeight;

	CRect r;
	GetClientRect(&r);
	DeleteObject(m_pDbBitMap);
	m_pDbBitMap = CreateCompatibleBitmap(m_pDC->m_hDC, r.right, r.bottom);	
	m_pDbDC->SelectObject(m_pDbBitMap);

	// Initialize world object (scene) with window properties
	origin = Vector(floor(r.right / 2), floor(r.bottom / 2), 0, 1); // x, y, z, w
	axes[X_AXIS] = Vector(1, 0, 0, 0);
	axes[Y_AXIS] = Vector(0, -1, 0, 0);
	axes[Z_AXIS] = Vector(0, 0, 1, 0);

	world.setScreenMat(axes, origin, cx, cy);
}


BOOL CCGWorkView::SetupViewingFrustum(void)
{
    return TRUE;
}


// This viewing projection gives us a constant aspect ration. This is done by
// increasing the corresponding size of the ortho cube.
BOOL CCGWorkView::SetupViewingOrthoConstAspect(void)
{
	return TRUE;
}





BOOL CCGWorkView::OnEraseBkgnd(CDC* pDC) 
{
	// Windows will clear the window with the background color every time your window 
	// is redrawn, and then CGWork will clear the viewport with its own background color.

	
	return true;
}



/////////////////////////////////////////////////////////////////////////////
// CCGWorkView drawing
/////////////////////////////////////////////////////////////////////////////

void CCGWorkView::OnDraw(CDC* pDC)
{
	CCGWorkDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
	    return;
	CRect r;

	GetClientRect(&r);
	CDC *pDCToUse = /*m_pDC*/m_pDbDC;
	
	pDCToUse->FillSolidRect(&r, world.bg_color);
	
	if (!world.isEmpty())
		world.draw(pDCToUse);


	if (pDCToUse != m_pDC) 
	{
		m_pDC->BitBlt(r.left, r.top, r.Width(), r.Height(), pDCToUse, r.left, r.top, SRCCOPY);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CCGWorkView CGWork Finishing and clearing...

void CCGWorkView::OnDestroy() 
{
	CView::OnDestroy();

	// delete the DC
	if ( m_pDC ) {
		delete m_pDC;
	}

	if (m_pDbDC) {
		delete m_pDbDC;
	}
}



/////////////////////////////////////////////////////////////////////////////
// User Defined Functions

void CCGWorkView::RenderScene() {
	// do nothing. This is supposed to be overriden...

	return;
}


void CCGWorkView::OnFileLoad() 
{
	TCHAR szFilters[] = _T ("IRIT Data Files (*.itd)|*.itd|All Files (*.*)|*.*||");

	CFileDialog dlg(TRUE, _T("itd"), _T("*.itd"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY ,szFilters);

	if (dlg.DoModal () == IDOK) {
		m_strItdFileName = dlg.GetPathName();		// Full path and filename
		PngWrapper p;

		// Reset the world before loading a new file
		CGSkelProcessIritDataFiles(m_strItdFileName, 1);
		// Open the file and read it.
		// Your code here...

		Invalidate();	// force a WM_PAINT for drawing.
	} 

}

// VIEW HANDLERS ///////////////////////////////////////////

// Note: that all the following Message Handlers act in a similar way.
// Each control or command has two functions associated with it.

void CCGWorkView::OnViewOrthographic() 
{
	m_nView = ID_VIEW_ORTHOGRAPHIC;
	world.state.perspective = false;
	Invalidate();		// redraw using the new view.
}

void CCGWorkView::OnUpdateViewOrthographic(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nView == ID_VIEW_ORTHOGRAPHIC);
}

void CCGWorkView::OnViewPerspective() 
{
	m_nView = ID_VIEW_PERSPECTIVE;
	world.state.perspective = true;
	Invalidate();
}

void CCGWorkView::OnUpdateViewPerspective(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nView == ID_VIEW_PERSPECTIVE);
}




// ACTION HANDLERS ///////////////////////////////////////////

void CCGWorkView::OnActionRotate() 
{
	m_nAction = ID_ACTION_ROTATE;
}

void CCGWorkView::OnUpdateActionRotate(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nAction == ID_ACTION_ROTATE);
}

void CCGWorkView::OnActionTranslate() 
{
	m_nAction = ID_ACTION_TRANSLATE;
}

void CCGWorkView::OnUpdateActionTranslate(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nAction == ID_ACTION_TRANSLATE);
}

void CCGWorkView::OnActionScale() 
{
	m_nAction = ID_ACTION_SCALE;
}

void CCGWorkView::OnUpdateActionScale(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nAction == ID_ACTION_SCALE);
}




// AXIS HANDLERS ///////////////////////////////////////////


// Gets calles when the X button is pressed or when the Axis->X menu is selected.
// The only thing we do here is set the ChildView member variable m_nAxis to the 
// selected axis.
void CCGWorkView::OnAxisX() 
{
	m_nAxis = ID_AXIS_X;
}

// Gets called when windows has to repaint either the X button or the Axis pop up menu.
// The control is responsible for its redrawing.
// It sets itself disabled when the action is a Scale action.
// It sets itself Checked if the current axis is the X axis.
void CCGWorkView::OnUpdateAxisX(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nAxis == ID_AXIS_X);
}


void CCGWorkView::OnAxisY() 
{
	m_nAxis = ID_AXIS_Y;
}

void CCGWorkView::OnUpdateAxisY(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nAxis == ID_AXIS_Y);
}


void CCGWorkView::OnAxisZ() 
{
	m_nAxis = ID_AXIS_Z;
}

void CCGWorkView::OnUpdateAxisZ(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nAxis == ID_AXIS_Z);
}





// OPTIONS HANDLERS ///////////////////////////////////////////




// LIGHT SHADING HANDLERS ///////////////////////////////////////////

void CCGWorkView::OnLightShadingFlat() 
{
	m_nLightShading = ID_LIGHT_SHADING_FLAT;
}

void CCGWorkView::OnUpdateLightShadingFlat(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nLightShading == ID_LIGHT_SHADING_FLAT);
}


void CCGWorkView::OnLightShadingGouraud() 
{
	m_nLightShading = ID_LIGHT_SHADING_GOURAUD;
}

void CCGWorkView::OnUpdateLightShadingGouraud(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nLightShading == ID_LIGHT_SHADING_GOURAUD);
}

// LIGHT SETUP HANDLER ///////////////////////////////////////////

void CCGWorkView::OnLightConstants() 
{
	CLightDialog dlg;

	for (int id=LIGHT_ID_1;id<MAX_LIGHT;id++)
	{	    
	    dlg.SetDialogData((LightID)id,m_lights[id]);
	}
	dlg.SetDialogData(LIGHT_ID_AMBIENT,m_ambientLight);

	if (dlg.DoModal() == IDOK) 
	{
	    for (int id=LIGHT_ID_1;id<MAX_LIGHT;id++)
	    {
		m_lights[id] = dlg.GetDialogData((LightID)id);
	    }
	    m_ambientLight = dlg.GetDialogData(LIGHT_ID_AMBIENT);
	}	
	Invalidate();
}

void CCGWorkView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	CView::OnTimer(nIDEvent);
	if (nIDEvent == 1)
		Invalidate();
}

void resetWorld() {
	world.state.show_vertex_normal = false;
	world.state.show_polygon_normal = false;
	world.state.object_frame = false;
	world.state.perspective = false;
	world.state.object_transform = true;

	world.bg_color = BG_DEFAULT_COLOR;
	world.wire_color = WIRE_DEFAULT_COLOR;

	world.state.world_mat = Matrix::Identity();
	world.state.object_mat = Matrix::Identity();
	world.state.ortho_mat = Matrix::Identity();
}

// TODO: tweak sensitivity
Matrix createRotateMatrix(int axis, int shift) {
	Matrix transform = Matrix::Identity();
	double sin_val = sin(shift / (2 * M_PI));
	double cos_val = cos(shift / (2 * M_PI));

	switch (axis) {
	case X_AXIS: // X axis
		transform.array[1][1] = cos_val;
		transform.array[2][2] = cos_val;
		transform.array[1][2] = -sin_val;
		transform.array[2][1] = sin_val;
		break;
	case Y_AXIS: // Y axis
		transform.array[0][0] = cos_val;
		transform.array[2][2] = cos_val;
		transform.array[0][2] = sin_val;
		transform.array[2][0] = -sin_val;
		break;
	case Z_AXIS: // Z axis
		transform.array[0][0] = cos_val;
		transform.array[1][1] = cos_val;
		transform.array[0][1] = -sin_val;
		transform.array[1][0] = sin_val;
		break;
	}

	return transform;
}

// TODO: find a better solution to this, cause this one sucks.
Matrix createScaleMatrix(int axis, int shift) {
	Matrix transform = Matrix::Identity();

	if (shift != 0)
		transform.array[axis][axis] = (1 + 1/shift);

	return transform;
}

Matrix createTranslateMatrix(int axis, int shift) {
	Matrix transform = Matrix::Identity();

	transform.array[axis][3] = shift;
	
	return transform;
}

void CCGWorkView::OnLButtonDown(UINT nFlags, CPoint point)
{
	is_mouse_down = true;
	mouse_location = point;

	CView::OnLButtonDown(nFlags, point);
}

void CCGWorkView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (is_mouse_down) {
		int shift = mouse_location.x - point.x;
		int axis = m_nAxis - ID_AXIS_X;
		Matrix* mat_to_transform;

		if (world.state.object_transform) {
			mat_to_transform = &world.state.object_mat;
		} else {
			mat_to_transform = &world.state.world_mat;
		}

		Matrix transform = Matrix::Identity();
		switch (m_nAction) {
		case ID_ACTION_ROTATE :
			transform = createRotateMatrix(axis, shift);
			break;
		case ID_ACTION_SCALE :
			transform = createScaleMatrix(axis, shift);
			break;
		case ID_ACTION_TRANSLATE :
			transform = createTranslateMatrix(axis, shift);
		default:
			break;
	}		
		*mat_to_transform = transform * *mat_to_transform;

		mouse_location = point;
		Invalidate();
	}

	CView::OnMouseMove(nFlags, point);
}

void CCGWorkView::OnLButtonUp(UINT nFlags, CPoint point)
{
	is_mouse_down = false;

	CView::OnLButtonUp(nFlags, point);
}

void CCGWorkView::OnPolygonNormals() {
	world.state.show_polygon_normal = !world.state.show_polygon_normal;
	Invalidate();
}

void CCGWorkView::OnUpdatePolygonNormals(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(world.state.show_polygon_normal);
}

void CCGWorkView::OnVertexNormals() {
	world.state.show_vertex_normal = !world.state.show_vertex_normal;
	Invalidate();
}

void CCGWorkView::OnUpdateVertexNormals(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(world.state.show_vertex_normal);
}

void CCGWorkView::OnObjectFrame()
{
	world.state.object_frame = !world.state.object_frame;
	Invalidate();
}

void CCGWorkView::OnUpdateObjectFrame(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(world.state.object_frame);
}

void CCGWorkView::OnObjectTransform()
{
	world.state.object_transform = true;
}

void CCGWorkView::OnUpdateObjectTransform(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(world.state.object_transform);
}

void CCGWorkView::OnWorldTransform()
{
	world.state.object_transform = false;
}

void CCGWorkView::OnUpdateWorldTransform(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!world.state.object_transform);
}

void CCGWorkView::OnObjectColor()
{
	CColorDialog diag;

	if (diag.DoModal() == IDOK) {
		world.wire_color = diag.GetColor();
		Invalidate();
	}
}

void CCGWorkView::OnBGColor()
{
	CColorDialog diag;

	if (diag.DoModal() == IDOK) {
		world.bg_color = diag.GetColor();
		Invalidate();
	}
}