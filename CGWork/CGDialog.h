#pragma once

#include "stdafx.h"

/*****  Header for the Ex1Dialog class *****/

class CEx2Dialog : public CDialog
{
	DECLARE_DYNAMIC(CEx2Dialog)

public:
	// static colors
	double m_sensitivity,
		m_distance,
		m_fineness;

	CEx2Dialog(CWnd* pParent = NULL);   // standard constructor
	CEx2Dialog(double sensitivity, double distance, double fineness, CWnd* pParent = nullptr);


	virtual ~CEx2Dialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
