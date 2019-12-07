/***** Defines the class behaviors for the dialog. *****/

#include "stdafx.h"
#include "afxdialogex.h"
#include "Resource.h"

#include "CGDialog.h"

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


BEGIN_MESSAGE_MAP(CEx2Dialog, CDialog)
END_MESSAGE_MAP()