#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
________________________________________________________________________

-*/

#include "stdafx.h"
#include "resource.h"


mClass(uiMFC) uiMFCDialog : public CDialog
{
    DECLARE_DYNAMIC(uiMFCDialog)
public:
			uiMFCDialog(CWnd* pParent = NULL); 
    virtual		~uiMFCDialog();

    enum { IDD = IDD_DIALOG1 };

protected:
    virtual void	DoDataExchange(CDataExchange* pDX);  
    DECLARE_MESSAGE_MAP()

public:
    afx_msg void	OnBnClickedOk();
protected:
    CString mesgtext;
};


bool initMFCDialog( HWND hwnd );

