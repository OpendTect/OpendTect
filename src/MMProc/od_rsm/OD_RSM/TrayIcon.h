// SimpleTray.h: interface for the TrayIconclass.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SIMPLETRAY_H__23CC3125_1378_4441_87C9_B5D445D9CCDF__INCLUDED_)
#define AFX_SIMPLETRAY_H__23CC3125_1378_4441_87C9_B5D445D9CCDF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <afxtempl.h>

// custom window message
#define WM_TRAYNOTIFY WM_APP+1

class TrayIcon 
{
public:
	TrayIcon();
	virtual ~TrayIcon();
	
	void Show();
	void Hide();

	void SetIcon( HICON hIcon );
	void SetTooltip( LPCTSTR lpTooltip );
			
private:
	BOOL m_bEnabled;
	HICON m_hIcon;
	//NOTIFYICONDATA m_nid;

	NOTIFYICONDATA  m_nid;

	CString m_strTooltip;
};

#endif // !defined(AFX_SIMPLETRAY_H__23CC3125_1378_4441_87C9_B5D445D9CCDF__INCLUDED_)
