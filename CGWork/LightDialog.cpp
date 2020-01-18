// LightDialog.cpp : implementation file
//

#include "stdafx.h"
#include "CGWork.h"
#include "LightDialog.h"


// CLightDialog dialog

IMPLEMENT_DYNAMIC(CLightDialog, CDialog)

CLightDialog::CLightDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CLightDialog::IDD, pParent)
{
    m_currentLightIdx = 0;
}

CLightDialog::~CLightDialog()
{
}

void CLightDialog::updatePositionDirectionOptions()
{
	LightType type = (LightType)((CComboBox*)this->GetDlgItem(IDC_LIGHT_TYPE))->GetCurSel();
	//int type = m_lights[m_currentLightIdx].type;
	//DDX_CBIndex(pDX, IDC_LIGHT_TYPE, type);

	if (type == LIGHT_TYPE_DIRECTIONAL) {
		// disable position control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_X)->EnableWindow(false);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_Y)->EnableWindow(false);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_Z)->EnableWindow(false);

		// enable direction control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_X)->EnableWindow(true);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_Y)->EnableWindow(true);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_Z)->EnableWindow(true);

		// disable spotlight angle control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_SPOTLIGHT_ANGLE)->EnableWindow(false);
	}
	else if (type == LIGHT_TYPE_POINT)
	{
		// disable Direction control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_X)->EnableWindow(false);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_Y)->EnableWindow(false);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_Z)->EnableWindow(false);

		// enable position control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_X)->EnableWindow(true);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_Y)->EnableWindow(true);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_Z)->EnableWindow(true);

		// disable spotlight angle control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_SPOTLIGHT_ANGLE)->EnableWindow(false);
	}
	else // type == LIGHT_TYPE_SPOT)
	{
		// disable Direction control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_X)->EnableWindow(true);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_Y)->EnableWindow(true);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_DIR_Z)->EnableWindow(true);

		// enable position control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_X)->EnableWindow(true);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_Y)->EnableWindow(true);
		(CEdit*)this->GetDlgItem(IDC_LIGHT_POS_Z)->EnableWindow(true);

		// enable spotlight angle control
		(CEdit*)this->GetDlgItem(IDC_LIGHT_SPOTLIGHT_ANGLE)->EnableWindow(true);
	}
}

void CLightDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//ambient light
	DDX_Text(pDX, IDC_AMBL_COLOR_R, m_ambiant.colorR);
	DDX_Text(pDX, IDC_AMBL_COLOR_G, m_ambiant.colorG);
	DDX_Text(pDX, IDC_AMBL_COLOR_B, m_ambiant.colorB);

	//update light parameters for the currently selected light
	DDX_Text(pDX, IDC_LIGHT_COLOR_R, m_lights[m_currentLightIdx].colorR);
	DDX_Text(pDX, IDC_LIGHT_COLOR_G, m_lights[m_currentLightIdx].colorG);
	DDX_Text(pDX, IDC_LIGHT_COLOR_B, m_lights[m_currentLightIdx].colorB);

	DDX_Text(pDX, IDC_LIGHT_POS_X, m_lights[m_currentLightIdx].posX);
	DDX_Text(pDX, IDC_LIGHT_POS_Y, m_lights[m_currentLightIdx].posY);
	DDX_Text(pDX, IDC_LIGHT_POS_Z, m_lights[m_currentLightIdx].posZ);

	DDX_Text(pDX, IDC_LIGHT_DIR_X, m_lights[m_currentLightIdx].dirX);
	DDX_Text(pDX, IDC_LIGHT_DIR_Y, m_lights[m_currentLightIdx].dirY);
	DDX_Text(pDX, IDC_LIGHT_DIR_Z, m_lights[m_currentLightIdx].dirZ);

	DDX_Text(pDX, IDC_LIGHT_SPOTLIGHT_ANGLE, m_lights[m_currentLightIdx].m_spotlight_angle_degrees);


	DDX_Text(pDX, IDC_KA, ka);
	DDX_Text(pDX, IDC_KD, kd);
	DDX_Text(pDX, IDC_KS, ks);
	DDX_Text(pDX, IDC_COSN, cosn);

	//NOTE:Add more dialog controls which are associated with the structure below this line		
	//...

	//the following class members can't be updated directly through DDX
	//using a helper variable for type-casting to solve the compilation error

	int helper=m_lights[m_currentLightIdx].enabled;
	DDX_Check(pDX,IDC_LIGHT_ENABLED,helper);
	m_lights[m_currentLightIdx].enabled = (bool)helper;

	helper = m_lights[m_currentLightIdx].type;
	DDX_CBIndex(pDX,IDC_LIGHT_TYPE,helper);
	m_lights[m_currentLightIdx].type = (LightType)helper;

	// If light is position, no need to enable direction twinking, and vice versa
	updatePositionDirectionOptions();

	helper = m_lights[m_currentLightIdx].space;
	DDX_CBIndex(pDX,IDC_LIGHT_SPACE,helper);
	m_lights[m_currentLightIdx].space = (LightSpace)helper;

}


BEGIN_MESSAGE_MAP(CLightDialog, CDialog)
    ON_BN_CLICKED(IDC_RADIO_LIGHT1, &CLightDialog::OnBnClickedRadioLight)
    ON_BN_CLICKED(IDC_RADIO_LIGHT2, &CLightDialog::OnBnClickedRadioLight)
    ON_BN_CLICKED(IDC_RADIO_LIGHT3, &CLightDialog::OnBnClickedRadioLight)
    ON_BN_CLICKED(IDC_RADIO_LIGHT4, &CLightDialog::OnBnClickedRadioLight)
    ON_BN_CLICKED(IDC_RADIO_LIGHT5, &CLightDialog::OnBnClickedRadioLight)
    ON_BN_CLICKED(IDC_RADIO_LIGHT6, &CLightDialog::OnBnClickedRadioLight)
    ON_BN_CLICKED(IDC_RADIO_LIGHT7, &CLightDialog::OnBnClickedRadioLight)
    ON_BN_CLICKED(IDC_RADIO_LIGHT8, &CLightDialog::OnBnClickedRadioLight)
	ON_CBN_SELCHANGE(IDC_LIGHT_TYPE, &CLightDialog::OnCbnSelchangeLightType)
END_MESSAGE_MAP()

void CLightDialog::SetDialogData( LightID id,const LightParams& light )
{    
    if (id<=LIGHT_ID_AMBIENT)
	m_ambiant = light;
    else
	m_lights[id]=light;
}

LightParams CLightDialog::GetDialogData( LightID id )
{
    if (id == LIGHT_ID_AMBIENT)
	return m_ambiant;
    else
	return m_lights[id];
}
// CLightDialog message handlers

//this callback function is called when each of the radio buttons on the dialog is clicked
void CLightDialog::OnBnClickedRadioLight()
{
    //save the dialog state into the data variables
    UpdateData(TRUE);
    //get the newly selected light index from the radio buttons
    m_currentLightIdx=GetCheckedRadioButton(IDC_RADIO_LIGHT1,IDC_RADIO_LIGHT8)-IDC_RADIO_LIGHT1;
    //Update all dialog fields according to the new light index
    UpdateData(FALSE);
    Invalidate();
}

BOOL CLightDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    //Set the radio button of the current light to be selected
    CheckRadioButton(IDC_RADIO_LIGHT1,IDC_RADIO_LIGHT8,m_currentLightIdx+IDC_RADIO_LIGHT1);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


void CLightDialog::OnCbnSelchangeLightType()
{
	updatePositionDirectionOptions();
}
