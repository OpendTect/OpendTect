/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewwin.cc,v 1.2 2007-02-23 14:26:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewmainwin.h"
#include "uiflatviewdockwin.h"
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
    for ( int idx=0; idx<tonull_.size(); idx++ )
	*tonull_[idx] = 0;
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


void uiFlatViewMainWin::handleNewViewer( uiFlatViewer* vwr )
{
    //TODO connect each viewer's mouseover to status bar
}


uiFlatViewDockWin::uiFlatViewDockWin( uiParent* p,
				      const uiFlatViewDockWin::Setup& setup )
    : uiDockWin(p,setup.name_)
{
    setCloseMode( setup.closemode_ );
    setDockName( setup.name_ );
    setResizeEnabled( true );
    createViewers( setup.nrviewers_ );
}


void uiFlatViewDockWin::handleNewViewer( uiFlatViewer* vwr )
{
}
