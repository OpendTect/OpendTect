/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: canvascommands.cc,v 1.1 2012-09-17 12:37:40 cvsjaap Exp $";

#include "canvascommands.h"
#include "cmddriverbasics.h"
#include "cmdrecorder.h"

#include "mouseevent.h"
#include "uigraphicsviewbase.h"
#include "uimenu.h"

namespace CmdDrive
{


bool CanvasMenuCmd::act( const char* parstr )
{
    mParKeyStrInit( "canvas", parstr, parnext, keys, selnr );
    mParPathStrInit( "menu", parnext, parnexxt, menupath );
    mParOnOffInit( parnexxt, partail, onoff );
    mParTail( partail );

    mFindObjects( objsfound, uiGraphicsViewBase, keys, nrgrey );
    mParKeyStrPre( "canvas", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiGraphicsViewBase*, uigraph, objsfound[0] );

    prepareIntercept( menupath, onoff );

    mActivate( GraphicsViewMenu, Activator(*uigraph) );

    return didInterceptSucceed( "Canvas" );
}


void GraphicsViewMenuActivator::actCB( CallBacker* cb )
{
    const MouseEvent right( OD::RightButton );
    actobj_.getMouseEventHandler().triggerButtonPressed( right );
}


#define mInterceptCanvasMenu( menupath, allowroot, uigraph ) \
\
    CmdDriver::InterceptMode mode = \
	allowroot ? CmdDriver::NodeInfo : CmdDriver::ItemInfo; \
    prepareIntercept( menupath, 0, mode ); \
    mActivate( GraphicsViewMenu, Activator(*uigraph) ); \
\
    if ( !didInterceptSucceed("Canvas") ) \
	return false;

bool NrCanvasMenuItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "canvas", parnext, parnexxt, keys, selnr );
    mParPathStrInit( "menu", parnexxt, partail, menupath );
    mParTail( partail );

    mFindObjects( objsfound, uiGraphicsViewBase, keys, nrgrey );
    mParKeyStrPre( "canvas", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiGraphicsViewBase*, uigraph, objsfound[0] );
    mInterceptCanvasMenu( menupath, true, uigraph );

    mParIdentPost( identname, interceptedMenuInfo().nrchildren_, parnext );
    return true;
}


bool IsCanvasMenuItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "canvas", parnext, parnexxt, keys, selnr );
    mParPathStrInit( "menu", parnexxt, partail, menupath );
    mParTail( partail );

    mFindObjects( objsfound, uiGraphicsViewBase, keys, nrgrey );
    mParKeyStrPre( "canvas", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiGraphicsViewBase*, uigraph, objsfound[0] );
    mInterceptCanvasMenu( menupath, false, uigraph );

    mParIdentPost( identname, interceptedMenuInfo().ison_, parnext );
    return true;
}


bool GetCanvasMenuItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    mParKeyStrInit( "canvas", parnext, parnexxt, keys, selnr );
    mParPathStrInit( "menu", parnexxt, parnexxxt, menupath );
    mParFormInit( parnexxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiGraphicsViewBase, keys, nrgrey );
    mParKeyStrPre( "canvas", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiGraphicsViewBase*, uigraph, objsfound[0] );
    mInterceptCanvasMenu( menupath, false, uigraph );

    const MenuInfo menuinfo = interceptedMenuInfo();
    mParForm( answer, form, menuinfo.text_, menuinfo.siblingnr_ );
    mParIdentPost( identname, answer, parnext );
    return true;
}


//====== CmdComposers =========================================================


bool CanvasMenuCmdComposer::accept( const CmdRecEvent& ev )
{
    const bool accepted = CmdComposer::accept( ev );

    if ( ignoreflag_ || quitflag_ || !ev.begin_ )
	return accepted;

    if ( ev.dynamicpopup_ )
    {
	const char* onoff = !ev.mnuitm_->isCheckable() ? "" :
			    ( ev.mnuitm_->isChecked() ? " On" : " Off" );

	insertWindowCaseExec( ev, eventlist_[0]->casedep_ );
	mRecOutStrm << "CanvasMenu \"" << eventlist_[0]->keystr_ << "\" \""
		    << ev.menupath_ << "\"" << onoff << std::endl;
    }
    else if ( !mMatchCI(ev.msg_, "rightButtonPressed" ) )
	mRefuseAndQuit();

    return accepted;
}


}; // namespace CmdDrive
