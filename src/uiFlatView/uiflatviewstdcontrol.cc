/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: uiflatviewstdcontrol.cc,v 1.1 2007-03-01 19:35:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewstdcontrol.h"
#include "flatviewzoommgr.h"
#include "uiflatviewer.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "pixmap.h"

#define mDefBut(butnm,grp,fnm,cbnm,tt) \
    butnm = new uiToolButton( grp, 0, ioPixmap(fnm), \
			      mCB(this,uiFlatViewStdControl,cbnm) ); \
    butnm->setToolTip( tt )

uiFlatViewStdControl::uiFlatViewStdControl( uiFlatViewer& vwr,
					    const Setup& setup )
    : uiFlatViewControl(vwr,setup.parent_,true)
    , stategrp_(0)
{
    if ( setup.withstates_ )
    {
	stategrp_ = new uiButtonGroup( this, "", setup.vertical_ );
	mDefBut(manipbut_,stategrp_,"view.png",stateCB,"View mode (zoom)");
	manipbut_->setToggleButton( true ); manipbut_->setOn( true );
	mDefBut(drawbut_,stategrp_,"pick.png",stateCB,"Interact mode");
	drawbut_->setToggleButton( true ); drawbut_->setOn( true );
    }

    posgrp_ = new uiButtonGroup( this, "", setup.vertical_ );
    mDefBut(zoominbut_,posgrp_,"zoomforward.png",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,posgrp_,"zoombackward.png",zoomCB,"Zoom out");
    mDefBut(panupbut_,posgrp_,"uparrow.png",panCB,"Pan up");
    mDefBut(panleftbut_,posgrp_,"leftarrow.png",panCB,"Pan left");
    mDefBut(panrightbut_,posgrp_,"rightarrow.png",panCB,"Pan right");
    mDefBut(pandownbut_,posgrp_,"downarrow.png",panCB,"Pan down");
    uiToolButton* 
    mDefBut(fliplrbut,posgrp_,"flip_lr.png",flipCB,"Flip left/right");

    parsgrp_ = new uiButtonGroup( this, "", setup.vertical_ );
    mDefBut(parsbut_,parsgrp_,"2ddisppars.png",parsCB,"Set display parameters");

    if ( stategrp_ )
	posgrp_->attach( setup.vertical_?ensureBelow:ensureRightOf, stategrp_);
    parsgrp_->attach( setup.vertical_ ? ensureBelow : ensureRightOf, posgrp_ );

    if ( setup.vertical_ )
	vwr.attach( rightOf, this );
    else
	attach( alignedBelow, &vwr );

    vwr.viewChanged.notify( mCB(this,uiFlatViewStdControl,vwChgCB) );
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
    if ( !stategrp_ ) return;

    const bool ismanip = (but == manipbut_ && manipbut_->isOn())
		      || (but == drawbut_ && !drawbut_->isOn());

    if ( but == manipbut_ )
	drawbut_->setOn( !ismanip );
    else
	manipbut_->setOn( ismanip );
}
