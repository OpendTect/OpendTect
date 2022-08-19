#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
