#pragma once

#include "stdafx.h"
#include "IritObjects.h"

/*****  Header for all the Dialog classes *****/

class CMotionBlurDialog : public CDialog
{
	DECLARE_DYNAMIC(CMotionBlurDialog)

public:
	double m_motion_drag;

	CMotionBlurDialog(CWnd* pParent = NULL);   // standard constructor
	CMotionBlurDialog(double drag);


	virtual ~CMotionBlurDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};

class CTransDialog : public CDialog
{
	DECLARE_DYNAMIC(CTransDialog)

public:
	double m_alpha;

	CTransDialog(CWnd* pParent = NULL);   // standard constructor
	CTransDialog(double alpha);


	virtual ~CTransDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};

class CPngDialog : public CDialog
{
	DECLARE_DYNAMIC(CPngDialog)

public:
	// static colors
	int m_height,
		m_width;

	CPngDialog(CWnd* pParent = NULL);   // standard constructor
	CPngDialog(int height, int width);


	virtual ~CPngDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};

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
