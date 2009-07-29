/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/


static const char* rcsID = "$Id: uiwelltiecontrolview.cc,v 1.20 2009-07-29 10:05:49 cvsbruno Exp $";

#include "uiwelltiecontrolview.h"

#include "flatviewzoommgr.h"
#include "keyboardevent.h"
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
    mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode (Esc)");
    mDefBut(editbut_,"seedpickmode.png",editCB,"Pick mode (P)");
    editbut_->setToggleButton( true );

    vwr_.rgbCanvas().getKeyboardEventHandler().keyPressed.notify(
	                    mCB(this,uiWellTieControlView,keyPressCB) );
    toolbar_->addSeparator();
}


void uiWellTieControlView::finalPrepare()
{
    updatePosButtonStates();
    MouseEventHandler& mevh =
	vwr_.rgbCanvas().getNavigationMouseEventHandler();
    mevh.wheelMove.notify( mCB(this,uiWellTieControlView,wheelMoveCB) );
    mevh.buttonPressed.notify(
	mCB(this,uiWellTieControlView,handDragStarted));
    mevh.buttonReleased.notify(
	mCB(this,uiWellTieControlView,handDragged));
    mevh.movement.notify( mCB(this,uiWellTieControlView,handDragging));
}


bool uiWellTieControlView::handleUserClick()
{
    const MouseEvent& ev = mouseEventHandler(0).event();
    uiWorld2Ui w2u; vwr_.getWorld2Ui(w2u);
    const uiWorldPoint wp = w2u.transform( ev.pos() );
    if ( ev.leftButton() && !ev.ctrlStatus() && !ev.shiftStatus() 
	&& !ev.altStatus() && checkIfInside(wp.x,wp.y) && editbut_->isOn()  )
    {
	vwr_.getAuxInfo( wp, infopars_ );
	const uiWorldRect& bbox = vwr_.boundingBox();
	picksetmgr_->addPick( bbox.left(), bbox.right(), wp.x, wp.y );
	return true;
    }
    return false;
}


bool uiWellTieControlView::checkIfInside( double xpos, double zpos )
{
    const uiWorldRect& bbox = vwr_.boundingBox();
    const Interval<double> xrg( bbox.left(), bbox.right() ),
			   zrg( bbox.bottom(), bbox.top() );
    if ( !xrg.includes( xpos, false ) || !zrg.includes( zpos, false ) ) 
	mErrRet("Please select your pick inside the work area");
    return true;
}


void uiWellTieControlView::rubBandCB( CallBacker* cb )
{
    setSelView();
}


void uiWellTieControlView::altZoomCB( CallBacker* but )
{
    const uiWorldRect& bbox = vwr_.boundingBox();
    const Interval<double> xrg( bbox.left(), bbox.right());
    zoomCB( but );
    uiWorldRect wr = vwr_.curView();
    wr.setLeftRight( xrg );
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> size = wr.size();
    setNewView( centre, size );
}


void uiWellTieControlView::wheelMoveCB( CallBacker* )
{
    if ( !vwr_.rgbCanvas().
	getNavigationMouseEventHandler().hasEvent() )
	return;

    const MouseEvent& ev =
	vwr_.rgbCanvas().getNavigationMouseEventHandler().event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    altZoomCB( ev.angle() < 0 ? zoominbut_ : zoomoutbut_ );
}


void uiWellTieControlView::keyPressCB( CallBacker* )
{
    const KeyboardEvent& ev =
	vwr_.rgbCanvas().getKeyboardEventHandler().event();
    if ( ev.key_ == OD::P )
    {
	editbut_->setOn( !editbut_->isOn() );
	editCB( 0 );
    }
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


void uiWellTieControlView::setEditOn( bool yn )
{
    editbut_->setOn( yn );
    editCB( 0 );
}

/*
void uiWellTieControlView::stateCB( CallBacker* )
{
    if ( !manipdrawbut_ ) return;
    if ( manip_ ) 
	manip_ = 0;
    else
	manip_ = 1;
    manipdrawbut_->setPixmap( manip_ ? "altview.png" : "altpick.png" );
    vwr_.setRubberBandingOn( !manip_ );
    vwr_.rgbCanvas().setDragMode( !manip_ ? uiGraphicsViewBase::RubberBandDrag
					  : uiGraphicsViewBase::ScrollHandDrag);
    if ( editbut_ )
	editbut_->setOn( false );
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


void uiFlatViewStdControl::editCB( CallBacker* )
{
    uiGraphicsViewBase::ODDragMode mode;
    if ( editbut_->isOn() )
	mode = uiGraphicsViewBase::NoDrag;
    else
	mode = manip_ ? uiGraphicsViewBase::ScrollHandDrag
					  : uiGraphicsViewBase::RubberBandDrag;

    vwr_.rgbCanvas().setDragMode( mode );
}
*/


