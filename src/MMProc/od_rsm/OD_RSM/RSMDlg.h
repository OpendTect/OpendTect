// RSMDlg.h : header file
//

#pragma once
#include "afxwin.h"

class TrayIcon;
// CRSMDlg dialog
class CRSMDlg : public CDialog
{
// Construction
public:
	CRSMDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_RSM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
protected:
    TrayIcon* trayicon_;
public:
    void startUP();
    void LogMessage( CString msg );
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnTrayNotify( WPARAM wParam, LPARAM lParam );
	afx_msg void OnShowdialog();
	afx_msg void OnMenuExit();
	afx_msg void OnMenuStart();
	afx_msg void OnMenuStop();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnClose();
    afx_msg void OnBnClickedButtonStart();
    afx_msg void OnBnClickedButtonStop();
protected:
    CListBox loglist_;
};
