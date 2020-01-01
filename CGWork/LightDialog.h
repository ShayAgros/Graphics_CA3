#pragma once

#include "Light.h"
// CLightDialog dialog



class CLightDialog : public CDialog
{
	DECLARE_DYNAMIC(CLightDialog)

public:
	CLightDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLightDialog();

	//dialog interface
	void SetDialogData(LightID id,const LightParams& light);
	LightParams GetDialogData(LightID id);

// Dialog Data
	enum { IDD = IDD_LIGHTS_DLG };

protected:
	LightParams m_lights[MAX_LIGHT];
	LightParams m_ambiant;
	int m_currentLightIdx;
	int GetCurrentLightIndex();

	double ka, kd, ks;
	int cosn;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedRadioLight();
//    afx_msg void On();
    virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeAmblColorR2();

	void set_ka(double new_ka) {
		ka = new_ka;
	}

	void set_kd(double new_kd) {
		kd = new_kd;
	}

	void set_ks(double new_ks) {
		ks = new_ks;
	}

	void set_cosn(int new_cosn) {
		cosn = new_cosn;
	}

	double get_ka() {
		return ka;
	}

	double get_kd() {
		return kd;
	}

	double get_ks() {
		return ks;
	}

	int get_cosn() {
		return cosn;
	}
};
