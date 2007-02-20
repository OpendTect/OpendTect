/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewwin.cc,v 1.1 2007-02-20 18:15:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewwin.h"
#include "uiflatviewer.h"


uiFlatViewWin::uiFlatViewWin( uiParent* p, const uiFlatViewWin::Setup& s )
    : uiMainWin(p,s.wintitle_,s.nrstatusfields_,s.menubar_)
{
    for ( int idx=0; idx<s.nrviewers_; idx++ )
	addViewer();
}


void uiFlatViewWin::addViewer()
{
    //TODO connect each viewer's mouseover to status bar
    vwrs_ += new uiFlatViewer( this );
}


bool uiFlatViewWin::closeOK()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->setPack( true, 0 );
	vwrs_[idx]->setPack( false, 0 );
    }
    for ( int idx=0; idx<tonull_.size(); idx++ )
	*tonull_[idx] = 0;

    return true;
}
