// SimpleTray.cpp: implementation of the TrayIconclass.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrayIcon.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TrayIcon::TrayIcon()
{
	// initialize 
	m_hIcon = NULL;
	m_strTooltip = "";
	m_bEnabled = FALSE;
}

TrayIcon::~TrayIcon()
{
	Hide(); // a good thing to do :-)
}

void TrayIcon::Show()
{
	// check to see if we have a valid icon
	if ( m_hIcon == NULL )
		return;

	// make sure we are not already visible
	if ( m_bEnabled == TRUE )
		return;

	// initialize the TRAYNOTIFYDATA struct
	m_nid.cbSize	= sizeof(m_nid);

	m_nid.hWnd		= AfxGetMainWnd()->m_hWnd;	// the 'owner' window
												// this is the window that
												// receives notifications
	m_nid.uID		= 100;	// ID of our icon, as you can have
							// more than one icon in a single app

	m_nid.uCallbackMessage = WM_TRAYNOTIFY;	// our callback message
	wcscpy(m_nid.szTip, m_strTooltip);	// set the tooltip
	m_nid.hIcon = m_hIcon;	// icon to show

	m_nid.uFlags =	NIF_MESSAGE |	// flags are set to indicate what
					NIF_ICON |		// needs to be updated, in this case
					NIF_TIP ;
			
		// callback message, icon, and tooltip
 	

	Shell_NotifyIcon( NIM_ADD, &m_nid );	// finally add the icon

	m_bEnabled = TRUE;	// set our indicator so that we
						// know the current status
}

void TrayIcon::Hide()
{
	// make sure we are enabled
	if ( m_bEnabled == TRUE )
	{
		// we are, so remove
		m_nid.uFlags = 0;
		Shell_NotifyIcon( NIM_DELETE, &m_nid );
		// and again, we need to know what's going on
		m_bEnabled = FALSE;
	}
}

void TrayIcon::SetIcon(HICON hIcon)
{
	// first make sure we got a valid icon
	if ( hIcon == NULL )
		return;

	// set it as our private
	m_hIcon = hIcon;

	// now check if we are already enabled
	if ( m_bEnabled == TRUE )
	{	// we are, so update
		m_nid.hIcon = m_hIcon;
		m_nid.uFlags = NIF_ICON;	// only icon
		Shell_NotifyIcon( NIM_MODIFY, &m_nid );
	}
}

void TrayIcon::SetTooltip(LPCTSTR lpTooltip)
{
	// tooltip can be empty, we don't care
	m_strTooltip = lpTooltip;

	// if we are enabled
	if ( m_bEnabled == TRUE )
	{	// do an update
		wcscpy(m_nid.szTip, m_strTooltip);
		m_nid.uFlags = NIF_TIP;	// tooltip only
		Shell_NotifyIcon( NIM_MODIFY, &m_nid );
	}
}
