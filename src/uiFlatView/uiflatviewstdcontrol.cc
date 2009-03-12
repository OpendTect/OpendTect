/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatviewstdcontrol.cc,v 1.17 2009-03-12 14:59:32 cvsbert Exp $";

#include "uiflatviewstdcontrol.h"

#include "uiflatviewcoltabed.h"
#include "flatviewzoommgr.h"
#include "uiflatviewer.h"
#include "uiflatviewthumbnail.h"
#include "uibutton.h"
#include "uimainwin.h"
#include "uimenuhandler.h"
#include "uirgbarraycanvas.h"
#include "uitoolbar.h"
#include "mouseevent.h"
#include "pixmap.h"

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, 0, ioPixmap(fnm), \
			    mCB(this,uiFlatViewStdControl,cbnm) ); \
    but->setToolTip( tt ); \
    tb_->addObject( but );

uiFlatViewStdControl::uiFlatViewStdControl( uiFlatViewer& vwr,
					    const Setup& setup )
    : uiFlatViewControl(vwr,setup.parent_,true,setup.withwva_)
    , vwr_(vwr)
    , ctabed_(0)
    , manip_(false)
    , menu_(*new uiMenuHandler(&vwr,-1))	//TODO multiple menus ?
    , propertiesmnuitem_("Properties...",100)
    , manipdrawbut_(0)
{
    tb_ = new uiToolBar( mainwin(), "Flat Viewer Tools" );
    if ( setup.withstates_ )
	{ mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode"); }
    vwr_.setRubberBandingOn( !manip_ );

    mDefBut(zoominbut_,"zoomforward.png",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward.png",zoomCB,"Zoom out");
    uiToolButton* mDefBut(fliplrbut,"flip_lr.png",flipCB,"Flip left/right");

    tb_->addSeparator();
    mDefBut(parsbut_,"2ddisppars.png",parsCB,"Set display parameters");

    if ( setup.withcoltabed_ )
    {
	uiToolBar* coltabtb = new uiToolBar( mainwin(), "Color Table" );
	ctabed_ = new uiFlatViewColTabEd( this, vwr );
	ctabed_->colTabChgd.notify( mCB(this,uiFlatViewStdControl,coltabChg) );
	coltabtb->addObject( ctabed_->colTabGrp()->attachObj() );
	coltabtb->display( vwr.rgbCanvas().prefHNrPics()>400 );
    }

    if ( !setup.helpid_.isEmpty() )
    {
	uiToolButton* mDefBut(helpbut,"contexthelp.png",helpCB,"Help");
	helpid_ = setup.helpid_;
    }

    vwr.viewChanged.notify( mCB(this,uiFlatViewStdControl,vwChgCB) );
    vwr.dispParsChanged.notify( mCB(this,uiFlatViewStdControl,dispChgCB) );

    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiFlatViewStdControl,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiFlatViewStdControl,handleMenuCB));

    new uiFlatViewThumbnail( this, vwr );
    viewerAdded.notify( mCB(this,uiFlatViewStdControl,vwrAdded) );

    //TODO attach keyboard events to panCB
}


uiFlatViewStdControl::~uiFlatViewStdControl()
{
    menu_.unRef();
}


void uiFlatViewStdControl::finalPrepare()
{
    updatePosButtonStates();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	MouseEventHandler& mevh = mouseEventHandler( idx );
	mevh.wheelMove.notify( mCB(this,uiFlatViewStdControl,wheelMoveCB) );
    }
}


void uiFlatViewStdControl::updatePosButtonStates()
{
    zoomoutbut_->setSensitive( !zoommgr_.atStart() );
}


void uiFlatViewStdControl::dispChgCB( CallBacker* )
{
    if ( ctabed_ )
	ctabed_->setColTab( vwr_ );
}


void uiFlatViewStdControl::vwrAdded( CallBacker* )
{
    MouseEventHandler& mevh = mouseEventHandler( vwrs_.size()-1 );
    mevh.wheelMove.notify( mCB(this,uiFlatViewStdControl,wheelMoveCB) );
}


void uiFlatViewStdControl::vwChgCB( CallBacker* )
{
    updatePosButtonStates();
}


void uiFlatViewStdControl::wheelMoveCB( CallBacker* )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( !mouseEventHandler( idx ).hasEvent() )
	    continue;

	const MouseEvent& ev = mouseEventHandler(idx).event();
	if ( mIsZero(ev.angle(),0.01) )
	    continue;

	zoomCB( ev.angle() < 0 ? zoominbut_ : zoomoutbut_ );
    }
}


void uiFlatViewStdControl::zoomCB( CallBacker* but )
{
    const bool zoomin = but == zoominbut_;
    if ( but == zoominbut_ )
	zoommgr_.forward();
    else
	zoommgr_.back();

    Geom::Size2D<double> newsz = zoommgr_.current();
    Geom::Point2D<double> centre( vwrs_[0]->curView().centre() );
    if ( zoommgr_.atStart() )
	centre = zoommgr_.initialCenter();

    setNewView( centre, newsz );
}


void uiFlatViewStdControl::panCB( CallBacker* but )
{
    const bool isleft = true;
    const bool isright = false;
    const bool isup = false;
    const bool isdown = false;
    const bool ishor = isleft || isright;

    const uiWorldRect cv( vwrs_[0]->curView() );
    const bool isrev = ishor ? cv.left() > cv.right() : cv.bottom() > cv.top();
    const double fac = 1 - zoommgr_.fwdFac();
    const double pandist = fac * (ishor ? cv.width() : cv.height());

    Geom::Point2D<double> centre = cv.centre();
    Geom::Size2D<double> sz = cv.size();
    if ( (!isrev && isleft) || (isrev && isright) )
	centre.x -= pandist;
    else if ( (isrev && isleft) || (!isrev && isright) )
	centre.x += pandist;
    else if ( (isrev && isup) || (!isrev && isdown) )
	centre.y -= pandist;
    else if ( (!isrev && isup) || (isrev && isdown) )
	centre.y += pandist;

    setNewView( centre, sz );
}


void uiFlatViewStdControl::flipCB( CallBacker* but )
{
    flip( true );
}


void uiFlatViewStdControl::parsCB( CallBacker* but )
{
    doPropertiesDialog();
}


void uiFlatViewStdControl::stateCB( CallBacker* but )
{
    if ( !manipdrawbut_ ) return;
    manip_ = !manip_;

    manipdrawbut_->setPixmap( manip_ ? "altview.png" : "altpick.png" );
    vwr_.setRubberBandingOn( !manip_ );
}


void uiFlatViewStdControl::helpCB( CallBacker* )
{
    uiMainWin::provideHelp( helpid_ );
}


bool uiFlatViewStdControl::handleUserClick()
{
    //TODO and what about multiple viewers?
    const MouseEvent& ev = mouseEventHandler(0).event();
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	  !ev.altStatus() )
    {
	menu_.executeMenu(0);
	return true;
    }
    return false;
}


void uiFlatViewStdControl::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    mAddMenuItem( menu, &propertiesmnuitem_, true, false );
}


void uiFlatViewStdControl::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    bool ishandled = true;
    if ( mnuid==propertiesmnuitem_.id )
	doPropertiesDialog();
    else
	ishandled = false;

    menu->setIsHandled( ishandled );
}


void uiFlatViewStdControl::coltabChg( CallBacker* )
{
    vwr_.handleChange( FlatView::Viewer::VDPars, false );
    vwr_.handleChange( FlatView::Viewer::VDData );
}
