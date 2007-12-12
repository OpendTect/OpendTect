/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewwin.cc,v 1.8 2007-12-12 15:44:40 cvsbert Exp $
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
	vwrs_[idx]->setDarkBG( yn );
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
    for ( int idx=3; idx<pars.size(); idx++ ) // 0 is the positioning
					      // 1 is the VD/WVA user reference
					      // 2 is the VD/WVA value
    {
	char* ptrout = str.buf() + str.size();
	const char* ptrin = pars.getKey(idx);
	while ( *ptrin )
	{
	    if ( isupper(*ptrin) )
	    {
		*ptrout = *ptrin;
		*ptrout++;
	    }
	    *ptrin++;
	}
	*ptrout = '\0';
	str += ": ";
	str += pars.getValue(idx);
	str += "; ";
    }
    const char* vdstr = pars.find("Variable density data");
    const char* tooltip = vdstr ? vdstr : pars.find("Wiggle/VA data");
    statusBar()->setToolTip( 0, tooltip );
    if ( pars.size()>2 )
    {
	str += vdstr ? "VD Value: " : "WVA Value: ";
	str += pars.getValue(2);
    }
    statusBar()->message( str.buf() );
}


uiFlatViewDockWin::uiFlatViewDockWin( uiParent* p,
				      const uiFlatViewDockWin::Setup& setup )
    : uiDockWin(p,setup.name_)
{
    setDockName( setup.name_ );
    setResizeEnabled( true );
    createViewers( setup.nrviewers_ );
}


uiFlatViewDockWin::~uiFlatViewDockWin()
{
    cleanUp();
}
