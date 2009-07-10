/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/


static const char* rcsID = "$Id: uiwelltiecontrolview.cc,v 1.16 2009-07-10 16:11:17 cvsbruno Exp $";

#include "uiwelltiecontrolview.h"

#include "flatviewzoommgr.h"
#include "mouseevent.h"
#include "pixmap.h"
#include "welltiepickset.h"

#include "uibutton.h"
#include "uiflatviewer.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uirgbarraycanvas.h"
#include "uitoolbar.h"
#include "uiworld2ui.h"

#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( toolbar_, 0, ioPixmap(fnm), \
			    mCB(this,uiWellTieControlView,cbnm) ); \
    but->setToolTip( tt ); \
    toolbar_->addObject( but );

uiWellTieControlView::uiWellTieControlView( uiParent* p, uiToolBar* toolbar,
       					    uiFlatViewer* vwr)
    : uiFlatViewStdControl(*vwr, uiFlatViewStdControl::Setup()
	   						.withstates(false)
	    						.withcoltabed(false))
    , toolbar_(toolbar)
    , manip_(true)
{
    mDynamicCastGet(uiMainWin*,mw,p)
    if ( mw )
	mw->removeToolBar( tb_ );
    else
	tb_->display(false);

    toolbar_->addSeparator();
    toolbar_->addObject( vwr_.rgbCanvas().getSaveImageButton() );
    mDefBut(parsbut_,"2ddisppars.png",parsCB,"Set display parameters");
    mDefBut(zoominbut_,"zoomforward.png",altZoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward.png",altZoomCB,"Zoom out");
    mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode");
    toolbar_->addSeparator();

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


void uiWellTieControlView::rubBandCB( CallBacker* cb )
{
    setSelView();
}


void uiWellTieControlView::altZoomCB( CallBacker* but )
{
    const bool zoomin = but == zoominbut_;
    const uiWorldRect bbox = vwr_.boundingBox();
    if ( but == zoominbut_ )
    {
	zoommgr_.forward();
	Geom::Size2D<double> size = zoommgr_.current();
	size.setWidth( bbox.left()- bbox.right() );
	vwr_.setView( getZoomOrPanRect( vwr_.curView().centre(),size, bbox ) );
    }
    else
    {
	if ( zoommgr_.atStart() )
	return;
	zoommgr_.back();
	Geom::Size2D<double> size = zoommgr_.current();
	size.setWidth( bbox.left()- bbox.right() );
	vwr_.setView( getZoomOrPanRect( vwr_.curView().centre(),size, bbox ) );
    }
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
    MouseCursor cursor;
    if ( manip_ )
	cursor.shape_ = MouseCursor::Arrow;
    else
    {
	cursor.shape_ = MouseCursor::Bitmap;
	cursor.filename_ = "zoomforward.png";
	cursor.hotx_ = 8;
	cursor.hoty_ = 6;
    }

    vwr_.setCursor( cursor );
}


void uiWellTieControlView::setSelView( bool isnewsel )
{
    const uiRect viewarea = isnewsel ? 
	*vwr_.rgbCanvas().getSelectedArea() : getViewRect( &vwr_ );
    if ( viewarea.topLeft() == viewarea.bottomRight() ) return;

    uiWorld2Ui w2u; vwr_.getWorld2Ui( w2u );
    uiWorldRect wr = w2u.transform( viewarea );

    const uiWorldRect bbox = vwr_.boundingBox();

    Interval<double> zrg( wr.top() , wr.bottom() );
    wr.setTopBottom( zrg );
    
    Interval<double> xrg( bbox.left(), bbox.right());
    wr.setLeftRight( xrg );

    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();

    vwr_.handleChange( FlatView::Viewer::All );
    setNewView( centre, newsz );
}






