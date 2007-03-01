/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewwin.cc,v 1.5 2007-03-01 12:03:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewmainwin.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"


void uiFlatViewWin::createViewers( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	uiFlatViewer* vwr = new uiFlatViewer( uiparent() );
	vwrs_ += vwr;
	handleNewViewer( vwr );
    }
}


void uiFlatViewWin::cleanUp()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->setPack( true, 0 );
	vwrs_[idx]->setPack( false, 0 );
    }
}


void uiFlatViewWin::setDarkBG( bool yn )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setDarkBG( yn );
    uiparent()->setBackgroundColor( yn ? Color::Black : Color::White );
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
    // TODO connect mouse handling of control to my status bar
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
