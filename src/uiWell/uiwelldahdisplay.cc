/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldahdisplay.cc,v 1.1 2010-09-17 12:26:07 cvsbruno Exp $";

#include "uiwelldahdisplay.h"


uiWellDahDisplay::uiWellDahDisplay( uiParent* p, const char* nm )
    : uiGraphicsView(p,nm)
{
    setStretch( 2, 2 );

    reSize.notify( mCB(this,uiWellDahDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    finaliseDone.notify( mCB(this,uiWellDahDisplay,init) );
}


void uiWellDahDisplay::init( CallBacker* )
{
    dataChanged();
    show();
}


void uiWellDahDisplay::dataChanged()
{
    gatherInfo(); draw();
}


void uiWellDahDisplay::reSized( CallBacker* )
{
    draw();
}
