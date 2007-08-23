/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: uiflatviewstdcontrol.cc,v 1.5 2007-08-23 15:27:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewstdcontrol.h"
#include "flatviewzoommgr.h"
#include "uiflatviewer.h"
#include "uiflatviewthumbnail.h"
#include "uibutton.h"
#include "uimenuhandler.h"
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
    : uiFlatViewControl(vwr,setup.parent_,true)
    , manipbut_(0)
    , menu_(*new uiMenuHandler(&vwr,-1))	//TODO multiple menus ?
    , propertiesmnuitem_("Properties...",100)
{
    tb_ = new uiToolBar( mainwin(), "Flat Viewer Tools" );
    if ( setup.withstates_ )
    {
	mDefBut(manipbut_,"view.png",stateCB,"View mode (zoom)");
	manipbut_->setToggleButton( true ); manipbut_->setOn( true );
	mDefBut(drawbut_,"pick.png",stateCB,"Interact mode");
	drawbut_->setToggleButton( true ); drawbut_->setOn( false );
	tb_->addSeparator();
    }

    mDefBut(zoominbut_,"zoomforward.png",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward.png",zoomCB,"Zoom out");
    mDefBut(panupbut_,"uparrow.png",panCB,"Pan up");
    mDefBut(panleftbut_,"leftarrow.png",panCB,"Pan left");
    mDefBut(panrightbut_,"rightarrow.png",panCB,"Pan right");
    mDefBut(pandownbut_,"downarrow.png",panCB,"Pan down");
    uiToolButton* 
    mDefBut(fliplrbut,"flip_lr.png",flipCB,"Flip left/right");

    tb_->addSeparator();
    mDefBut(parsbut_,"2ddisppars.png",parsCB,"Set display parameters");

    vwr.viewChanged.notify( mCB(this,uiFlatViewStdControl,vwChgCB) );

    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiFlatViewStdControl,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiFlatViewStdControl,handleMenuCB));

    uiFlatViewThumbnail* tn = new uiFlatViewThumbnail( this, vwr );
    tn->setColors( Color(0,0,200), Color::White );
}


uiFlatViewStdControl::~uiFlatViewStdControl()
{
    menu_.unRef();
}


void uiFlatViewStdControl::finalPrepare()
{
    updatePosButtonStates();
}


void uiFlatViewStdControl::updatePosButtonStates()
{
    zoomoutbut_->setSensitive( !zoommgr_.atStart() );

    const uiWorldRect bb( getBoundingBox() );
    const uiWorldRect cv( vwrs_[0]->curView() );
    const bool isrevx = cv.left() > cv.right();
    const bool isrevy = cv.bottom() > cv.top();
    const Geom::Size2D<double> bbsz( bb.size() );

    double bbeps = bb.width() * 1e-5;
    if ( cv.left() > cv.right() )
    {
	panleftbut_->setSensitive( cv.left() < bb.right() - bbeps );
	panrightbut_->setSensitive( cv.right() > bb.left() + bbeps );
    }
    else
    {
	panleftbut_->setSensitive( cv.left() > bb.left() + bbeps );
	panrightbut_->setSensitive( cv.right() < bb.right() - bbeps );
    }
    bbeps = bb.height() * 1e-5;
    if ( cv.bottom() > cv.top() )
    {
	pandownbut_->setSensitive( cv.bottom() < bb.top() - bbeps );
	panupbut_->setSensitive( cv.top() > bb.bottom() + bbeps );
    }
    else
    {
	pandownbut_->setSensitive( cv.bottom() > bb.bottom() + bbeps );
	panupbut_->setSensitive( cv.top() < bb.top() - bbeps );
    }
}


void uiFlatViewStdControl::vwChgCB( CallBacker* but )
{
    updatePosButtonStates();
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
    const bool isleft = but == panleftbut_;
    const bool isright = but == panrightbut_;
    const bool isup = but == panupbut_;
    const bool isdown = but == pandownbut_;
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
    if ( !manipbut_ ) return;

    const bool ismanip = (but == manipbut_ && manipbut_->isOn())
		      || (but == drawbut_ && !drawbut_->isOn());

    if ( but == manipbut_ )
	drawbut_->setOn( !ismanip );
    else
	manipbut_->setOn( ismanip );
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
