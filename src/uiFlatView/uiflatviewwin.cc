/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewwin.cc,v 1.12 2008-08-04 10:27:11 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiflatviewmainwin.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"
#include "uistatusbar.h"


void uiFlatViewWin::createViewers( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	uiFlatViewer* vwr = new uiFlatViewer( viewerParent() );
	vwrs_ += vwr;
	handleNewViewer( vwr );
    }
}


void uiFlatViewWin::cleanUp()
{
    deepErase( vwrs_ );
}


void uiFlatViewWin::setDarkBG( bool yn )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->appearance().setDarkBG( yn );
    }

    viewerParent()->setBackgroundColor( yn ? Color::Black : Color::White );
}


uiFlatViewMainWin::uiFlatViewMainWin( uiParent* p,
				      const uiFlatViewMainWin::Setup& setup )
    : uiMainWin(p,setup.wintitle_,setup.nrstatusfields_,setup.menubar_)
{
    createViewers( setup.nrviewers_ );
}


void uiFlatViewMainWin::addControl( uiFlatViewControl* fvc )
{
    if ( !fvc ) return;

    fvc->infoChanged.notify(mCB(this,uiFlatViewMainWin,displayInfo) );
}


void uiFlatViewMainWin::displayInfo( CallBacker* cb )
{
    mCBCapsuleUnpack(IOPar,pars,cb);
    BufferString str;
    BufferString tooltip;
    if ( pars.indexOf( "X-coordinate" ) > 0 )
    {
	str += "X: ";
	str += pars.find( "X-coordinate" );
	str += "; ";
    }
    if ( pars.indexOf( "Y-coordinate" ) > 0 )
    {
	str += "Y: ";
	str += pars.find( "Y-coordinate" );
	str += "; ";
    }
    if ( pars.indexOf( "Z" ) > 0 )
    {
	str += "Z: ";
	str += pars.find( "Z" );
	str += "; ";
    }

    const char* vdstr = pars.find( "Variable density data" );
    const int vdstridx = pars.indexOf( "Variable density data" );
    const char* wvastr = vdstr ? vdstr : pars.find( "Wiggle/VA data" );
    const int wvastridx = pars.indexOf( "Wiggle/VA data" );
    if ( pars.size()>2 )
    {
	if ( pars.find("VD Value") )
	{
	    tooltip += "VD: ";
	    tooltip += pars.getValue( vdstridx );
	    tooltip += "; ";
	    const int index = pars.indexOf( "VD Value" );
	    str += "VD Value: ";
	    str += pars.getValue( index );
	    str += "; ";
	}
	if ( pars.find("WVA Value") && wvastr )
	{
	    tooltip += "WVA: ";
	    tooltip += pars.getValue( wvastridx );
	    tooltip += "; ";
	    const int index = pars.indexOf( "WVA Value" );
	    str += "WVA Value: ";
	    str += pars.getValue( index );
	    str += "; ";
	}
    }
    statusBar()->setToolTip( 0, tooltip.buf() );
    statusBar()->message( str.buf() );
}


uiFlatViewDockWin::uiFlatViewDockWin( uiParent* p,
				      const uiFlatViewDockWin::Setup& setup )
    : uiDockWin(p,setup.name_)
{
    setDockName( setup.name_ );
    createViewers( setup.nrviewers_ );
}


uiFlatViewDockWin::~uiFlatViewDockWin()
{
    cleanUp();
}
