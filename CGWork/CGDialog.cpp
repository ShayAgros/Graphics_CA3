/***** Defines the class behaviors for the dialog. *****/

#include "stdafx.h"
#include "afxdialogex.h"
#include "Resource.h"

#include "CGDialog.h"

// Motion Blur Dialog

IMPLEMENT_DYNAMIC(CMotionBlurDialog, CDialog)

CMotionBlurDialog::CMotionBlurDialog(CWnd* pParent /* = NULL */)
	: CDialog(IDD_SET_MOTION_BLUR, pParent),
	m_motion_drag(DEFAULT_MOTION_BLUR)
{
}

CMotionBlurDialog::CMotionBlurDialog(double drag)
{
	m_motion_drag = drag;
}

CMotionBlurDialog::~CMotionBlurDialog()
{
}

BOOL CMotionBlurDialog::OnInitDialog()
{
	CString string;
	BOOL result = CDialog::OnInitDialog();

	string.Format(_T("%f"), m_motion_drag);
	GetDlgItem(IDC_MOTION_BLUR)->SetWindowText(string);

	return result;
}

void CMotionBlurDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MOTION_BLUR, m_motion_drag);
	DDV_MinMaxDouble(pDX, m_motion_drag, 0, 1);
}

// Transparency Dialog

IMPLEMENT_DYNAMIC(CTransDialog, CDialog)

CTransDialog::CTransDialog(CWnd* pParent /* = NULL */)
	: CDialog(IDD_SET_TRANSPARENCY, pParent),
	m_alpha(1)
{
}

CTransDialog::CTransDialog(double alpha)
{
	m_alpha = alpha;
}

CTransDialog::~CTransDialog()
{
}

BOOL CTransDialog::OnInitDialog()
{
	CString string;
	BOOL result = CDialog::OnInitDialog();

	string.Format(_T("%f"), m_alpha);
	GetDlgItem(IDC_TRANSPARENCY)->SetWindowText(string);

	return result;
}

void CTransDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_TRANSPARENCY, m_alpha);
	DDV_MinMaxDouble(pDX, m_alpha, 0, 1);
}

// PNG DIALOG

IMPLEMENT_DYNAMIC(CPngDialog, CDialog)

CPngDialog::CPngDialog(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SAVE_TO_PNG, pParent),
	m_height(0),
	m_width(0)
{
}

CPngDialog::CPngDialog(int height, int width)
{
	m_height = height;
	m_width = width;
}

CPngDialog::~CPngDialog()
{
}

BOOL CPngDialog::OnInitDialog()
{
	CString string;
	BOOL result = CDialog::OnInitDialog();

	string.Format(_T("%d"), m_height);
	GetDlgItem(IDC_PNG_HEIGHT)->SetWindowText(string);
	string.Format(_T("%d"), m_width);
	GetDlgItem(IDC_PNG_WIDTH)->SetWindowText(string);

	return result;
}

void CPngDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PNG_HEIGHT, m_height);
	DDV_MinMaxInt(pDX, m_height, 1, INT_MAX);
	DDX_Text(pDX, IDC_PNG_WIDTH, m_width);
	DDV_MinMaxInt(pDX, m_width, 1, INT_MAX);
}

// SENS/DISTANCE/FINENESS Dialog

IMPLEMENT_DYNAMIC(CEx2Dialog, CDialog)

CEx2Dialog::CEx2Dialog(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SENS_DISTANCE, pParent),
	m_sensitivity(1.0),
	m_distance(1.0),
	m_fineness(20.0)
{
}

CEx2Dialog::CEx2Dialog(double sensitivity, double distance, double fineness, CWnd* pParent)
	: CDialog(IDD_SENS_DISTANCE, pParent)
{
	m_sensitivity = sensitivity;
	m_distance = distance;
	m_fineness = fineness;
}

CEx2Dialog::~CEx2Dialog()
{
}

BOOL CEx2Dialog::OnInitDialog()
{
	CString string;
	BOOL result = CDialog::OnInitDialog();

	string.Format(_T("%0.3f"), m_sensitivity);
	GetDlgItem(IDC_SENS)->SetWindowText(string);
	string.Format(_T("%0.3f"), m_distance);
	GetDlgItem(IDC_DISTANCE)->SetWindowText(string);
	string.Format(_T("%0.3f"), m_distance);
	GetDlgItem(IDC_FINENESS)->SetWindowText(string);

	return result;
}

void CEx2Dialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SENS, m_sensitivity);
	DDV_MinMaxDouble(pDX, m_sensitivity, 0.1, INT_MAX);
	DDX_Text(pDX, IDC_DISTANCE, m_distance);
	DDV_MinMaxDouble(pDX, m_distance, 0.1, INT_MAX);
	DDX_Text(pDX, IDC_FINENESS, m_fineness);
	DDV_MinMaxDouble(pDX, m_fineness, 2.0, INT_MAX);
}

BEGIN_MESSAGE_MAP(CMotionBlurDialog, CDialog)
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(CTransDialog, CDialog)
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(CPngDialog, CDialog)
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(CEx2Dialog, CDialog)
END_MESSAGE_MAP()