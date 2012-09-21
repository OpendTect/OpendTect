/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "menubutcommands.h"
#include "cmddriverbasics.h"
#include "cmdrecorder.h"

#include "timefun.h"

#include "uibuttongroup.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uimdiarea.h"
#include "uimenu.h"
#include "uitabbar.h"
#include "uitoolbutton.h"


namespace CmdDrive
{


#define mGetMenuBar( mbar ) \
\
    const uiMenuBar* mbar = const_cast<uiMainWin*>(curWin())->menuBar(); \
    if ( !mbar ) \
    { \
	mWinErrStrm << "Window has no menu bar" << std::endl; \
	return false; \
    } \
    if ( !mbar->isSensitive() && (greyOutsSkipped() || goingToChangeUiObj()) ) \
    { \
	mWinErrStrm << "Menu bar is disabled" << std::endl; \
	return false; \
    }

bool MenuCmd::act( const char* parstr )
{
    mParPathStrInit( "menu", parstr, parnext, menupath );
    mParOnOffInit( parnext, partail, onoff );
    mParTail( partail );

    mGetMenuBar( mnubar );
    mFindMenuItem( menupath, *mnubar, mnuitm );

    if ( applWin() )
    {
	mFindMenuItem( "Survey`Exit", *applWin()->menuBar(), surveyexit );
	if ( mnuitm == surveyexit )
	    return CloseCmd(drv_).act("All");
    }

    mParOnOffPre( "menu item",onoff,mnuitm->isChecked(),mnuitm->isCheckable() );

    mActivate( Menu, Activator(*mnuitm) );

    mParOnOffPost( "menu item", onoff, mnuitm->isChecked() );
    return true;
}


MenuActivator::MenuActivator( const uiMenuItem& mnuitm )
    : actmnuitm_( const_cast<uiMenuItem&>(mnuitm) )
{}


bool NrMenuItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParPathStrInit( "menu", parnext, partail, menupath );
    mParTail( partail );

    mGetMenuBar( mnubar );
    mGetMenuInfo( menupath, true, *mnubar, menuinfo );
    mParIdentPost( identname, menuinfo.nrchildren_, parnext );
    return true;
}


bool IsMenuItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParPathStrInit( "menu", parnext, partail, menupath );
    mParTail( partail );

    mGetMenuBar( mnubar );
    mGetMenuInfo( menupath, false, *mnubar, menuinfo );
    mParIdentPost( identname, menuinfo.ison_, parnext );
    return true;
}


bool GetMenuItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParPathStrInit( "menu", parnext, parnexxt, menupath );
    mParFormInit( parnexxt, partail, form );
    mParTail( partail );

    mGetMenuBar( mnubar );
    mGetMenuInfo( menupath, false, *mnubar, menuinfo );
    mParForm( answer, form, menuinfo.text_, menuinfo.siblingnr_ );
    mParIdentPost( identname, answer, parnext );
    return true;
}


void MenuActivator::actCB( CallBacker* cb )
{ actmnuitm_.activated.trigger(); }


#define mGetButtonMenuButton( uibut, toolbut ) \
\
    mDynamicCastGet( const uiToolButton*, toolbut, uibut ); \
    if ( !toolbut || !toolbut->menu() ) \
    { \
	mWinErrStrm << "This button has no menu" << std::endl; \
	return false; \
    } \
    if ( !toolbut->menu()->isEnabled() && \
	 (greyOutsSkipped() || goingToChangeUiObj()) ) \
    { \
	mWinErrStrm << "Button menu is disabled" << std::endl; \
	return false; \
    }

bool ButtonMenuCmd::act( const char* parstr )
{
    mParKeyStrInit( "button", parstr, parnext, keys, selnr );
    mParPathStrInit( "menu", parnext, parnexxt, menupath );
    mParOnOffInit( parnexxt, partail, onoff );
    mParTail( partail );

    mFindObjects( objsfound, uiButton, keys, nrgrey );
    mParKeyStrPre( "button", objsfound, nrgrey, keys, selnr );

    mGetButtonMenuButton( objsfound[0], toolbut );
    mFindMenuItem( menupath, *toolbut->menu(), mnuitm );

    mParOnOffPre( "menu item",onoff,mnuitm->isChecked(),mnuitm->isCheckable() );

    mActivate( Menu, Activator(*mnuitm) );

    mParOnOffPost( "menu item", onoff, mnuitm->isChecked() );
    return true;
}


bool NrButtonMenuItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "button", parnext, parnexxt, keys, selnr );
    mParPathStrInit( "menu", parnexxt, partail, menupath );
    mParTail( partail );

    mFindObjects( objsfound, uiButton, keys, nrgrey );
    mParKeyStrPre( "button", objsfound, nrgrey, keys, selnr );

    mGetButtonMenuButton( objsfound[0], toolbut );
    mGetMenuInfo( menupath, true, *toolbut->menu(), menuinfo );
    mParIdentPost( identname, menuinfo.nrchildren_, parnext );
    return true;
}


bool IsButtonMenuItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "button", parnext, parnexxt, keys, selnr );
    mParPathStrInit( "menu", parnexxt, partail, menupath );
    mParTail( partail );

    mFindObjects( objsfound, uiButton, keys, nrgrey );
    mParKeyStrPre( "button", objsfound, nrgrey, keys, selnr );

    mGetButtonMenuButton( objsfound[0], toolbut );
    mGetMenuInfo( menupath, false, *toolbut->menu(), menuinfo );
    mParIdentPost( identname, menuinfo.ison_, parnext );
    return true;
}


bool GetButtonMenuItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "button", parnext, parnexxt, keys, selnr );
    mParPathStrInit( "menu", parnexxt, parnexxxt, menupath );
    mParFormInit( parnexxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiButton, keys, nrgrey );
    mParKeyStrPre( "button", objsfound, nrgrey, keys, selnr );

    mGetButtonMenuButton( objsfound[0], toolbut );
    mGetMenuInfo( menupath, false, *toolbut->menu(), menuinfo );
    mParForm( answer, form, menuinfo.text_, menuinfo.siblingnr_ );
    mParIdentPost( identname, answer, parnext );
    return true;
}

#define mTitleBarButWarn( butnm, noaccess, cmdkey ) \
{ \
    BufferString buf("Use \""); buf += cmdkey; buf += "\"-command to access"; \
    if ( noaccess ) \
	buf = "This window offers no access to"; \
\
    mWinWarnStrm << buf << " \"" << butnm \
		 << "\"-button in title bar if intended" << std::endl; \
}

#define mTitleBarButtonCheck( objsfound, keys, nrgrey ) \
{ \
    SearchKey key = mSearchKey( keys[0] ); \
    int nrmatches = key.isMatching("Menu"); \
    nrmatches += key.isMatching("Help"); \
    nrmatches += key.isMatching("Close"); \
    nrmatches += key.isMatching("Minimize"); \
    nrmatches += key.isMatching("Maximize"); \
    nrmatches += key.isMatching("Restore Down") || key.isMatching("Restore"); \
\
    if ( objsfound.isEmpty() && !nrgrey && nrmatches==true ) \
    { \
	if ( key.isMatching("Menu") ) \
	    mTitleBarButWarn( "Menu", true, 0 ); \
	if ( key.isMatching("Help") ) \
	    mTitleBarButWarn( "Help", true, 0 ); \
	if ( key.isMatching("Close") ) \
	    mTitleBarButWarn( "Close", false, CloseCmd::keyWord() ); \
	if ( key.isMatching("Minimize") ) \
	    mTitleBarButWarn( "Minimize", openQDlg(), ShowCmd::keyWord() ); \
	if ( key.isMatching("Maximize") ) \
	    mTitleBarButWarn( "Maximize", openQDlg(), ShowCmd::keyWord() ); \
	if ( key.isMatching("Restore Down") || key.isMatching("Restore") ) \
	    mTitleBarButWarn("Restore (Down)",openQDlg(),ShowCmd::keyWord()); \
    } \
}

bool ButtonCmd::act( const char* parstr )
{
    if ( openQDlg() )
	return actQDlgButton( parstr );

    mParKeyStrInit( "button", parstr, parnext, keys, selnr );
    mParOnOffInit( parnext, partail, onoff );
    mParTail( partail );
   
    mFindObjects( objsfound, uiButton, keys, nrgrey );
    mTitleBarButtonCheck( objsfound, keys, nrgrey );
    mParKeyStrPre( "button", objsfound, nrgrey, keys, selnr );

    mDynamicCastGet( const uiButton*, uibut, objsfound[0] );
    mDynamicCastGet( const uiButtonGroup*, butgrp, uibut->parent() );
    if ( butgrp && butgrp->isExclusive() && onoff<0 )
    {
	mWinErrStrm << "An auto-exclusive button cannot be switched off"
		    << std::endl;
	return false;
    }

    mDynamicCastGet( const uiPushButton*, pushbut, uibut );
    mDynamicCastGet( const uiRadioButton*, radiobut, uibut );
    mDynamicCastGet( const uiCheckBox*, checkbox, uibut );
    mDynamicCastGet( const uiToolButton*, toolbut, uibut );

    if ( pushbut )
	mParOnOffPre( "push-button", onoff, false, false );
    if ( radiobut )
	mParOnOffPre( "radio-button", onoff, radiobut->isChecked(), true );
    if ( checkbox )
	mParOnOffPre( "check-box", onoff, checkbox->isChecked(), true );
    if ( toolbut )
	mParOnOffPre( "tool-button", onoff, toolbut->isOn(), 
		      toolbut->isToggleButton() );
    
    mActivate( Button, Activator(*uibut) );

    if ( radiobut )
	mParOnOffPost( "radio-button", onoff, radiobut->isChecked() );
    if ( checkbox )
	mParOnOffPost( "check-box", onoff, checkbox->isChecked() );
    if ( toolbut )
	mParOnOffPost( "tool-button", onoff, toolbut->isOn() );

    return true;
}


ButtonActivator::ButtonActivator( const uiButton& uibut )
    : actbut_( const_cast<uiButton&>(uibut) )
{}


void ButtonActivator::actCB( CallBacker* cb )
{ actbut_.click(); }


#define mFindQDlgButtons( keys, butsfound, buttexts ) \
\
    BufferStringSet buttexts; \
    TypeSet<int> butsfound; \
{ \
    if ( keys.size() > 1 ) \
    { \
	mWinErrStrm << "QDialog does not accept multiple search keys" \
		    << std::endl; \
        return false; \
    } \
    int butidx = 0; \
    while ( true ) \
    { \
	mGetAmpFilteredStr( buttxt, curWin()->activeModalQDlgButTxt(butidx) ); \
	if ( !buttxt || !*buttxt ) \
	    break; \
\
	buttexts.add( buttxt ); \
	if ( mSearchKey(keys[0]).isMatching(buttxt) ) \
	    butsfound += butidx; \
\
	butidx++; \
    } \
} 

#define mExitQDlg( retval ) \
{ \
    mActivate( CloseQDlg, Activator(retval) ); \
    return true; \
}


bool ButtonCmd::actQDlgButton( const char* parstr )
{
    mParKeyStrInit( "button", parstr, parnext, keys, selnr );
    mParOnOffInit( parnext, partail, onoff );
    mParTail( partail );

    mFindQDlgButtons( keys, butsfound, buttexts );
    mTitleBarButtonCheck( butsfound, keys, 0 );
    mParStrPre( "QDialog exit-button",butsfound,0,keys[0],selnr,"key",true );
    mParOnOffPre( "push-button", onoff, false, false );
    wildcardMan().check( mSearchKey(keys[0]), buttexts.get(butsfound[0]) );

    mExitQDlg( curWin()->activeModalQDlgRetVal(butsfound[0]) );
}


bool IsButtonOnCmd::act( const char* parstr )
{
    if ( openQDlg() )
	return actQDlgButton( parstr );

    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "button", parnext, partail, keys, selnr );
    mParTail( partail );
   
    mFindObjects( objsfound, uiButton, keys, nrgrey );
    mParKeyStrPre( "button", objsfound, nrgrey, keys, selnr );

    mDynamicCastGet( const uiRadioButton*, radiobut, objsfound[0] );
    mDynamicCastGet( const uiCheckBox*, checkbox, objsfound[0] );
    mDynamicCastGet( const uiToolButton*, toolbut, objsfound[0] );

    int ison = -1;
    if ( radiobut )
	ison = radiobut->isChecked() ? 1 : 0;
    if ( checkbox )
	ison = checkbox->isChecked() ? 1 : 0;
    if ( toolbut && toolbut->isToggleButton() )
	ison = toolbut->isOn() ? 1 : 0;

    mParIdentPost( identname, ison, parnext );
    return true;
}


bool IsButtonOnCmd::actQDlgButton( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "button", parnext, partail, keys, selnr );
    mParTail( partail );

    mFindQDlgButtons( keys, butsfound, buttexts );
    mParStrPre( "QDialog exit-button",butsfound,0,keys[0],selnr,"key",true );
    wildcardMan().check( mSearchKey(keys[0]), buttexts.get(butsfound[0]) );
    mParIdentPost( identname, -1, parnext );
    return true;
}


bool GetButtonCmd::act( const char* parstr )
{
    if ( openQDlg() )
	return actQDlgButton( parstr );

    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "button", parnext, parnexxt, keys, selnr );
    mParExtraFormInit( parnexxt, partail, form, "Color" )
    mParTail( partail );
   
    mFindObjects( objsfound, uiButton, keys, nrgrey );
    mParKeyStrPre( "button", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiButton*, uibut, objsfound[0] );
    mDynamicCastGet( const uiColorInput*, uicolinp, objsfound[0]->parent() );

    mGetAmpFilteredStr( text, const_cast<uiButton*>(uibut)->text() );
    mGetColorString( uicolinp->color(), uicolinp, colorstr );
    mParForm( answer, form, text, colorstr );
    mParEscIdentPost( identname, answer, parnext, form!=Colour );
    return true;
}


bool GetButtonCmd::actQDlgButton( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "button", parnext, parnexxt, keys, selnr );
    mParExtraFormInit( parnexxt, partail, form, "Color" )
    mParTail( partail );

    mFindQDlgButtons( keys, butsfound, buttexts );
    mParStrPre( "QDialog exit-button",butsfound,0,keys[0],selnr,"key",true );
    const char* text = buttexts.get( butsfound[0] );
    wildcardMan().check( mSearchKey(keys[0]), text );
    mParForm( answer, form, text, "255`255`255`255" );
    mParEscIdentPost( identname, answer, parnext, form!=Colour );
    return true;
}


static int getOkCancelRetVal( bool ok )
{ 
    if ( uiMainWin::activeModalType() != uiMainWin::Message )
	return ok ? 1 : 0;

    if ( ok || !*uiMainWin::activeModalQDlgButTxt(1) )
	return 0;

    return *uiMainWin::activeModalQDlgButTxt(2) ? 2 : 1;
}


bool OkCancelCmd::act( const char* parstr )
{
    const bool ok = mMatchCI( name(), OkCmd::keyWord() );

    mParTail( parstr );

    if ( openQDlg() )
	mExitQDlg( getOkCancelRetVal(ok) );

    mDynamicCastGet( const uiDialog*, dlg, curWin() );
    if ( dlg )
    {
	uiDialog::Button buttyp = ok ? uiDialog::OK : uiDialog::CANCEL;
    	const uiButton* but = const_cast<uiDialog*>(dlg)->button( buttyp );

	if ( but && !but->sensitive() )
	{
	    mWinErrStrm << "The " << name() << "-button is currently disabled"
			<< std::endl;
	    return false;
	}

	if ( but )
	{
	    mActivate( Button, Activator(*but) );
	    return true;
	}
    }

    mWinWarnStrm << "Close-button used for lack of " << name() << "-button"
		 << std::endl;
	
    return CloseCmd(drv_).act( "" );
}


bool CloseCmd::actCloseCurWin( const char* parstr )
{
    bool closeall = false;
    BufferString closetag;
    const char* partail = getNextWord( parstr, closetag.buf() );

    if ( mMatchCI(closetag, "All") )
	closeall = true;
    else
	partail = parstr;

    mParTail( partail );

    if ( closeall )
    {
	while ( openQDlg() )
	{
	    const int retval = getOkCancelRetVal( false );
	    mActivate( CloseQDlg, Activator(retval) );
	}
	ObjectSet<uiMainWin> windowlist;
	uiMainWin::getTopLevelWindows( windowlist );

	for ( int idx=windowlist.size()-1; idx>=0; idx-- )
	{
	    if ( windowlist[idx] != applWin() )
		mActivate( Close, Activator(*windowlist[idx]) );
	}
	if ( applWin() )
	    mActivate( Close, Activator(*applWin()) );

	return true;
    }

    if ( openQDlg() )
	mExitQDlg( getOkCancelRetVal(false) );

    if ( curWin() == applWin() )
    {
	mWinErrStrm << "Using \"Close All\" is required to kill main window"
		    << std::endl;
	return false;
    }

    mActivate( Close, Activator(*curWin()) );
    return true;
}


#define mFindMdiAreaSubWin( subwinnames, mdiobj, winstr, selnr ) \
\
    BufferStringSet subwinnames; \
    mdiobj->getWindowNames( subwinnames ); \
    for ( int idx=subwinnames.size()-1; idx>=0; idx-- ) \
    { \
	if ( !mSearchKey(winstr).isMatching(*subwinnames[idx]) ) \
	    subwinnames.remove(idx); \
    } \
    mParStrPre( "subwindow", subwinnames, 0, winstr, selnr, "string", true ); \
    wildcardMan().check( mSearchKey(winstr), subwinnames.get(0) );

bool CloseCmd::act( const char* parstr )
{
    const int nrdquoted = StringProcessor(parstr).consecutiveDQuoted();
    if ( !nrdquoted )
	return actCloseCurWin( parstr );

    const char* extraparstr = nrdquoted<2 ? "\"\a\"" : parstr;
    mParKeyStrInit( "workspace", extraparstr, extraparnext, keys, keyselnr );
    const char* parnext = extraparstr==parstr ? extraparnext : parstr;
    mParWinStrInit( "subwindow", parnext, partail, winstr, winselnr, false );
    mParTail( partail );
    
    if ( openQDlg() )
    {
	mWinErrStrm << "Unable to close subwindows of open QDialog"
		    << std::endl;
	return false; 
    }

    mFindObjects( objsfound, uiMdiArea, keys, nrgrey );
    mParKeyStrPre( "workspace", objsfound, nrgrey, keys, keyselnr );
    mDynamicCastGet( const uiMdiArea*, mdiarea, objsfound[0] );
    mFindMdiAreaSubWin( subwinnames, mdiarea, winstr, winselnr );
    
    mActivate( MdiAreaClose, Activator(*mdiarea,subwinnames.get(0)) );
    return true;
}


MdiAreaCloseActivator::MdiAreaCloseActivator( const uiMdiArea& mdiarea,
					      const char* winname )
    : actwindow_(const_cast<uiMdiAreaWindow*>(mdiarea.getWindow(winname)))
{}


void MdiAreaCloseActivator::actCB( CallBacker* cb )
{ if ( actwindow_ ) actwindow_->close(); }


#define mParShowTagInit( parstr, parnext, minnormmax ) \
\
    int minnormmax = 1; \
    BufferString newsize; \
    const char* parnext = getNextWord( parstr, newsize.buf() ); \
\
    if ( mMatchCI(newsize,"Maximized") ) \
	minnormmax = 2; \
    else if ( mMatchCI(newsize,"Minimized") ) \
	minnormmax = 0; \
    else if ( !mMatchCI(newsize,"Normal") ) \
    { \
	mParseErrStrm << "Size argument not in {Minimized, Normal, Maximized}" \
		      << std::endl; \
    }

#define mParShowTagPre( objnm, isminimized, ismaximized, minnormmax ) \
\
    const int oldmnm = isminimized ? 0 : ( ismaximized ? 2 : 1 ); \
    if ( oldmnm == minnormmax  ) \
    { \
	mWinWarnStrm << "This " << objnm << " is already shown " \
	    << (oldmnm<1 ? "minimized" : (oldmnm>1 ? "maximized" : "normal")) \
	    << std::endl; \
    }

bool ShowCmd::actShowCurWin( const char* parstr )
{
    mParShowTagInit( parstr, partail, minnormmax );
    mParTail( partail );
    mParShowTagPre( "window", curWin()->isMinimized(), curWin()->isMaximized(),
		    minnormmax );

    mActivate( Show, Activator(*curWin(), minnormmax) );
    return true;
}


ShowActivator::ShowActivator( const uiMainWin& uimw, int minnormax )
    : actmainwin_( const_cast<uiMainWin&>(uimw) )
    , actminnormax_( minnormax )
{}


void ShowActivator::actCB( CallBacker* cb )
{ 
    if ( actminnormax_ > 1 )
	actmainwin_.showMaximized();
    else if ( actminnormax_ < 1 )
	actmainwin_.showMinimized();
    else
	actmainwin_.showNormal();
}


bool ShowCmd::act( const char* parstr )
{
    const int nrdquoted = StringProcessor(parstr).consecutiveDQuoted();
    if ( !nrdquoted )
	return actShowCurWin( parstr );

    const char* extraparstr = nrdquoted<2 ? "\"\a\"" : parstr;
    mParKeyStrInit( "workspace", extraparstr, extraparnext, keys, keyselnr );
    const char* parnext = extraparstr==parstr ? extraparnext : parstr;
    mParWinStrInit( "subwindow", parnext, parnexxt, winstr, winselnr, false );
    mParShowTagInit( parnexxt, partail, minnormmax );
    mParTail( partail );
    
    mFindObjects( objsfound, uiMdiArea, keys, nrgrey );
    mParKeyStrPre( "workspace", objsfound, nrgrey, keys, keyselnr );
    mDynamicCastGet( const uiMdiArea*, mdiarea, objsfound[0] );
    mFindMdiAreaSubWin( subwinnames, mdiarea, winstr, winselnr );

    const uiMdiAreaWindow* mdiwin = mdiarea->getWindow( subwinnames.get(0) );
    mParShowTagPre( "subwindow", mdiwin ? mdiwin->isMinimized() : false,
		    mdiwin ? mdiwin->isMaximized() : false, minnormmax );
    
    mActivate( MdiAreaShow, Activator(*mdiarea,*subwinnames[0],minnormmax) );

    return true;
}


MdiAreaShowActivator::MdiAreaShowActivator( const uiMdiArea& mdiarea,
					    const char* winname, int minnormax )
    : actwindow_(const_cast<uiMdiAreaWindow*>(mdiarea.getWindow(winname)))
    , actminnormax_( minnormax )
{}


void MdiAreaShowActivator::actCB( CallBacker* cb )
{
    if ( !actwindow_ ) return;

    if ( actminnormax_ > 1 )
	actwindow_->showMaximized();
    else if ( actminnormax_ < 1 )
	actwindow_->showMinimized();
    else
	actwindow_->show();
}


#define mGetShowStatus( answer, win, minnormmax  ) \
\
    BufferString answer; \
    if ( win && minnormmax<1 ) \
	answer = win->isMinimized() ? 1 : 0; \
    if ( win && minnormmax>1 ) \
	answer =win->isMaximized() ? 1 : 0; \
    if ( win && minnormmax==1 ) \
	answer = win->isMinimized() || win->isMaximized() ? 0 : 1;

bool IsShownCmd::actIsCurWinShown( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParShowTagInit( parnext, partail, minnormmax );
    mParTail( partail );

    mGetShowStatus( answer, curWin(), minnormmax ); 
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool IsShownCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );

    const int nrdquoted = StringProcessor(parnext).consecutiveDQuoted();
    if ( !nrdquoted )
	return actIsCurWinShown( parstr );

    const char* extraparstr = nrdquoted<2 ? "\"\a\"" : parnext;
    mParKeyStrInit( "workspace", extraparstr, extraparnext, keys, keyselnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParWinStrInit( "subwindow", parnexxt, parnexxxt, winstr, winselnr, false );
    mParShowTagInit( parnexxxt, partail, minnormmax );
    mParTail( partail );
    
    mFindObjects( objsfound, uiMdiArea, keys, nrgrey );
    mParKeyStrPre( "workspace", objsfound, nrgrey, keys, keyselnr );
    mDynamicCastGet( const uiMdiArea*, mdiarea, objsfound[0] );
    mFindMdiAreaSubWin( subwinnames, mdiarea, winstr, winselnr );

    mGetShowStatus(answer, mdiarea->getWindow(subwinnames.get(0)), minnormmax);
    mParIdentPost( identname, answer, parnext );
    return true;
}


#define mParTabSelPre( uitabbar, tabstr, tabnr, tabidxs ) \
\
    BufferStringSet tabtexts; \
    TypeSet<int> tabidxs; \
    int nrgreytabs = 0; \
    for ( int idx=0; idx<uitabbar->size(); idx++ ) \
    { \
	mGetAmpFilteredStr( tabtxt, uitabbar->textOfTab(idx) ); \
	tabtexts.add( tabtxt ); \
	if ( !mSearchKey(tabstr).isMatching(tabtxt) ) \
	    continue; \
	if ( !greyOutsSkipped() || uitabbar->isTabEnabled(idx) ) \
	    tabidxs += idx; \
	if ( !uitabbar->isTabEnabled(idx) ) \
	    nrgreytabs++; \
    } \
    mParStrPre( "tab", tabidxs, nrgreytabs, tabstr, tabnr, "name", true ); \
    mDisabilityCheck( "tab", 1, !uitabbar->isTabEnabled(tabidxs[0]) ); \
    wildcardMan().check( mSearchKey(tabstr), tabtexts.get(tabidxs[0]) ); 

bool TabCmd::act( const char* parstr )
{
    const int nrdquoted = StringProcessor(parstr).consecutiveDQuoted();
    const char* extraparstr = nrdquoted<2 ? "\"\a\"" : parstr;
    mParKeyStrInit( "tab bar", extraparstr, extraparnext, keys, selnr );
    const char* parnext = extraparstr==parstr ? extraparnext : parstr;
    mParDQuoted( "tab name", parnext, partail, tabstr, false, false );
    mParDisambiguator( "tab name", tabstr, tabnr );
    mParTail( partail );

    mFindObjects( objsfound, uiTabBar, keys, nrgreybars );
    mParKeyStrPre( "tab bar", objsfound, nrgreybars, keys, selnr );
    mDynamicCastGet( const uiTabBar*, uitabbar, objsfound[0] );
    mParTabSelPre( uitabbar, tabstr, tabnr, tabidxs );

    mActivate( Tab, Activator(*uitabbar, tabidxs[0]) );
    return true;
}


TabActivator::TabActivator( const uiTabBar& uitabbar, int tabidx )
    : acttabbar_( const_cast<uiTabBar&>(uitabbar) )
    , acttabidx_( tabidx )
{}


void TabActivator::actCB( CallBacker* cb )
{
    if ( acttabidx_>=0 && acttabidx_<acttabbar_.size() )
    {
	acttabbar_.setCurrentTab( acttabidx_ );
	acttabbar_.selected.trigger();
    }
}


#define mCountTabs( uitabbar, tabidx, count ) \
\
    int count = 0; \
    for ( int idx=0; idx<uitabbar->size(); idx++ ) \
    { \
	if ( greyOutsSkipped() && !uitabbar->isTabEnabled(idx) ) continue; \
\
	count++; \
	if ( idx == tabidx ) break; \
    }

bool NrTabsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const int nrdquoted = StringProcessor(parnext).consecutiveDQuoted();
    const char* extraparstr = nrdquoted<1 ? "\"\a\"" : parnext;
    mParKeyStrInit( "tab bar", extraparstr, extraparnext, keys, selnr );
    const char* partail = extraparstr==parnext ? extraparnext : parnext;
    mParTail( partail );

    mFindObjects( objsfound, uiTabBar, keys, nrgreybars );
    mParKeyStrPre( "tab bar", objsfound, nrgreybars, keys, selnr );
    mDynamicCastGet( const uiTabBar*, uitabbar, objsfound[0] );

    mCountTabs( uitabbar, -1, count );
    mParIdentPost( identname, count, parnext );
    return true;
}


bool CurTabCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const int nrdquoted = StringProcessor(parnext).consecutiveDQuoted();
    const char* extraparstr = nrdquoted<1 ? "\"\a\"" : parnext;
    mParKeyStrInit( "tab bar", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParFormInit( parnexxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTabBar, keys, nrgreybars );
    mParKeyStrPre( "tab bar", objsfound, nrgreybars, keys, selnr );
    mDynamicCastGet( const uiTabBar*, uitabbar, objsfound[0] );

    const int curtabidx = uitabbar->currentTabId();
    mCountTabs( uitabbar, curtabidx, count );
    mParForm( answer, form, uitabbar->textOfTab(curtabidx), count );
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool GetTabCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const int nrdquoted = StringProcessor(parnext).consecutiveDQuoted();
    const char* extraparstr = nrdquoted<2 ? "\"\a\"" : parnext;
    mParKeyStrInit( "tab bar", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParDQuoted( "tab name", parnexxt, parnexxxt, tabstr, false, false );
    mParDisambiguator( "tab name", tabstr, tabnr );
    mParFormInit( parnexxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTabBar, keys, nrgreybars );
    mParKeyStrPre( "tab bar", objsfound, nrgreybars, keys, selnr );
    mDynamicCastGet( const uiTabBar*, uitabbar, objsfound[0] );
    mParTabSelPre( uitabbar, tabstr, tabnr, tabidxs );

    mCountTabs( uitabbar, tabidxs[0], count );
    mParForm( answer, form, uitabbar->textOfTab(tabidxs[0]), count );
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool IsTabOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const int nrdquoted = StringProcessor(parnext).consecutiveDQuoted();
    const char* extraparstr = nrdquoted<2 ? "\"\a\"" : parnext;
    mParKeyStrInit( "tab bar", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParDQuoted( "tab name", parnexxt, partail, tabstr, false, false );
    mParDisambiguator( "tab name", tabstr, tabnr );
    mParTail( partail );

    mFindObjects( objsfound, uiTabBar, keys, nrgreybars );
    mParKeyStrPre( "tab bar", objsfound, nrgreybars, keys, selnr );
    mDynamicCastGet( const uiTabBar*, uitabbar, objsfound[0] );
    mParTabSelPre( uitabbar, tabstr, tabnr, tabidxs );

    const bool ison = uitabbar->currentTabId()==tabidxs[0] ? 1 : 0;
    mParIdentPost( identname, ison, parnext );
    return true;
}


//====== CmdComposers =========================================================


bool MenuCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ignoreflag_ || !ev.begin_ || !ev.mnuitm_ )
	return true;

    BufferString onoffstr;
    if ( ev.mnuitm_->isCheckable() )
	onoffstr = ev.mnuitm_->isChecked() ? " On" : " Off";

    mDynamicCastGet( const uiButton*, uibut, ev.object_ );
    insertWindowCaseExec( ev );
    if ( uibut )
    {
	mRecOutStrm << "ButtonMenu \"" << ev.keystr_ << "\" \""
		    << ev.menupath_ << "\"" << onoffstr << std::endl;
    }
    else
	mRecOutStrm << "Menu \""
		    << ev.menupath_ << "\"" << onoffstr << std::endl;
    return true;
}


bool ButtonCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ignoreflag_ || !ev.begin_ )
	return true;

    BufferString onoffstr;
    mDynamicCastGet( const uiRadioButton*, radiobut, ev.object_ );
    if ( radiobut )
	onoffstr = radiobut->isChecked() ? " On" : " Off";
    mDynamicCastGet( const uiCheckBox*, checkbox, ev.object_ );
    if ( checkbox )
	onoffstr = checkbox->isChecked() ? " On" : " Off";
    mDynamicCastGet( const uiToolButton*, toolbut, ev.object_ );
    if ( toolbut && toolbut->isToggleButton() )
	onoffstr = toolbut->isOn() ? " On" : " Off";

    insertWindowCaseExec( ev );
    mRecOutStrm << "Button \"" << ev.keystr_ << "\"" << onoffstr << std::endl;
    return true;
}


bool QMsgBoxButCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ev.begin_ )
	return true;
    
    BufferString qmsgboxbutword;
    const char* msgnext = getNextWord( ev.msg_, qmsgboxbutword.buf() );
    mSkipBlanks ( msgnext );
    mGetAmpFilteredStr( butname, msgnext );
    mDressNameString( butname, sKeyStr );

    insertWindowCaseExec( ev );
    mRecOutStrm << "Button \"" << butname << "\"" << std::endl;
    return true;
}


bool CloseCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ignoreflag_ || !ev.begin_ )
	return true;

    insertWindowCaseExec( ev );
    if ( ev.srcwin_ == applWin() )
    {
	mRecOutStrm << "Close All" << std::endl;
	rec_.stop();
    }
    else
	mRecOutStrm << "Close" << std::endl;

    return true;
}


bool MdiAreaCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ignoreflag_ || !ev.begin_ )
	return true;

    BufferString actword;
    const char* msgnext = getNextWord( ev.msg_, actword.buf() );

    char* msgnexxt;
    const int curwinidx = strtol( msgnext, &msgnexxt, 0 );

    BufferStringSet subwinnames; 
    mDynamicCastGet(const uiMdiArea*,mdiarea,ev.object_);
    mdiarea->getWindowNames( subwinnames ); 

    BufferString curwintitle = subwinnames.get( curwinidx );
    mDressNameString( curwintitle, sWinName );

    bool titlecasedep = false;
    int nrmatches = 0;
    int selnr = 0;

    for ( int idx=0; idx<subwinnames.size(); idx++ )
    {
	const char* wintitle = subwinnames.get( idx );
	if ( SearchKey(curwintitle,false).isMatching(wintitle) )
	{
	    if ( SearchKey(curwintitle,true).isMatching(wintitle) )
	    {
		nrmatches++;
		if ( idx == curwinidx )
		    selnr = nrmatches;
	    }
	    else 
		titlecasedep = true;
	}
    }

    if ( selnr && nrmatches>1 )
    {
	curwintitle += "#"; curwintitle += selnr;
    }

    if ( mMatchCI(actword,"Close") )
    {
	insertWindowCaseExec( ev, titlecasedep );
	mRecOutStrm << "Close \"" << ev.keystr_ << "\" \"" << curwintitle
		    << "\"" << std::endl;
    }

    return true;
}


bool TabCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ignoreflag_ || !ev.begin_ )
	return true;

    mDynamicCastGet( const uiTabBar*, uitabs, ev.object_ );

    mGetAmpFilteredStr( curtabname, uitabs->textOfTab(uitabs->currentTabId()) );
    mDressNameString( curtabname, sItemName );

    bool namecasedep = false;
    int nrmatches = 0;
    int selnr = 0;

    for ( int idx=0; idx<uitabs->size(); idx++ )
    {
	if ( !uitabs->isTabEnabled(idx) )
	    continue;

	mGetAmpFilteredStr( tabtxt, uitabs->textOfTab(idx) ); 
	if ( SearchKey(curtabname,false).isMatching(tabtxt) )
	{
	    if ( SearchKey(curtabname,true).isMatching(tabtxt) )
	    {
		nrmatches++;
		if ( idx == uitabs->currentTabId() )
		    selnr = nrmatches;
	    }
	    else 
		namecasedep = true;
	}
    }

    if ( selnr && nrmatches>1 )
    {
	curtabname += "#"; curtabname += selnr;
    }

    insertWindowCaseExec( ev, namecasedep );
    if ( ev.similarobjs_ )
    {
	mRecOutStrm << "Tab \"" << ev.keystr_ << "\" \"" << curtabname << "\""
		    << std::endl;
    }
    else 
	 mRecOutStrm << "Tab \"" << curtabname << "\"" << std::endl;

    return true;
}


}; // namespace CmdDrive
