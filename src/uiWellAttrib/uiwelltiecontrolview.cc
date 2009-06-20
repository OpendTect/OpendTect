/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/


static const char* rcsID = "$Id: uiwelltiecontrolview.cc,v 1.12 2009-06-20 11:57:45 cvsbruno Exp $";

#include "uiwelltiecontrolview.h"

#include "mouseevent.h"
#include "welltiepickset.h"
#include "pixmap.h"

#include "uiflatviewer.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uitoolbar.h"
#include "uiworld2ui.h"

#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, 0, ioPixmap(fnm), \
			    mCB(this,uiWellTieControlView,cbnm) ); \
    but->setToolTip( tt ); \
    tb_->addObject( but );

uiWellTieControlView::uiWellTieControlView( uiParent* p, uiToolBar* toolbar,
       					    uiFlatViewer* vwr)
    : uiFlatViewStdControl(*vwr, uiFlatViewStdControl::Setup()
	   						.withstates(false)
	    						.withcoltabed(false))
    , toolbar_(toolbar)
    , manip_(true)
{
     mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode");
     vwr_.setRubberBandingOn( !manip_ );
}


bool uiWellTieControlView::handleUserClick()
{
    const MouseEvent& ev = mouseEventHandler(0).event();
    uiWorld2Ui w2u;
    vwr_.getWorld2Ui(w2u);
    const uiWorldPoint wp = w2u.transform( ev.pos() );
    vwr_.getAuxInfo( wp, infopars_ );
    if ( ev.leftButton() && !ev.ctrlStatus() && !ev.shiftStatus() 
	    && !ev.altStatus() && checkIfInside(wp.x,wp.y)  )
    {
	Interval<float> xvwrsize; 
	xvwrsize.set( (float)(vwr_.boundingBox().left()),
		      (float)(vwr_.boundingBox().right()) );
	picksetmgr_->addPick( xvwrsize.start, xvwrsize.stop, wp.x, wp.y );

	return true;
    }
    return false;
}


bool uiWellTieControlView::checkIfInside( double xpos, double zpos )
{
    const double sizxleft  = vwr_.boundingBox().left();
    const double sizxright = vwr_.boundingBox().right();
    const double sizztop   = vwr_.boundingBox().top();
    const double sizzbot   = vwr_.boundingBox().bottom();
    if ( xpos < sizxleft || xpos > sizxright 
	    || zpos > sizztop || zpos < sizzbot )
	mErrRet("Please select your pick inside the work area");
    return true;
}


void uiWellTieControlView::stateCB( CallBacker* )
{
    if ( !manipdrawbut_ ) return;
    if ( manip_ ) 
	manip_ = 0;
    else
	manip_ = 1;
    manipdrawbut_->setPixmap( manip_ ? "altpick.png" :"altview.png" );
    vwr_.setRubberBandingOn( !manip_ );
}


