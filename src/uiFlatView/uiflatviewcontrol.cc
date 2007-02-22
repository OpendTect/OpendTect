/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewcontrol.cc,v 1.2 2007-02-22 15:55:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "flatdispzoommgr.h"
#include "pixmap.h"

#define mDefBut(butnm,grp,fnm,cbnm,tt) \
    butnm = new uiToolButton( grp, 0, ioPixmap(fnm), \
			      mCB(this,uiFlatViewControl,cbnm) ); \
    butnm->setToolTip( tt )

uiFlatViewControl::uiFlatViewControl( uiFlatViewer& vwr, const Setup& s )
    : uiGroup(vwr.attachObj()->parent(),"Flat viewer control")
    , setup_(s)
    , stategrp_(0)
    , zoommgr_(*new FlatDisp::ZoomMgr)
{
    setBorder( 0 );

    if ( setup_.withstates_ )
    {
	stategrp_ = new uiButtonGroup( this, "", setup_.vertical_ );
	mDefBut(manipbut_,stategrp_,"view.png",stateCB,"View mode (zoom)");
	manipbut_->setToggleButton( true ); manipbut_->setOn( true );
	mDefBut(drawbut_,stategrp_,"pick.png",stateCB,"Interact mode");
	drawbut_->setToggleButton( true ); drawbut_->setOn( true );
    }

    posgrp_ = new uiButtonGroup( this, "", setup_.vertical_ );
    mDefBut(zoominbut_,posgrp_,"zoomforward.png",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,posgrp_,"zoombackward.png",zoomCB,"Zoom out");
    mDefBut(panupbut_,posgrp_,"uparrow.png",panCB,"Pan up");
    mDefBut(panleftbut_,posgrp_,"leftarrow.png",panCB,"Pan left");
    mDefBut(panrightbut_,posgrp_,"rightarrow.png",panCB,"Pan right");
    mDefBut(pandownbut_,posgrp_,"downarrow.png",panCB,"Pan down");

    parsgrp_ = new uiButtonGroup( this, "", setup_.vertical_ );
    mDefBut(parsbut_,parsgrp_,"2ddisppars.png",parsCB,"Set display parameters");

    if ( stategrp_ )
	posgrp_->attach( setup_.vertical_?ensureBelow:ensureRightOf, stategrp_);
    parsgrp_->attach( setup_.vertical_ ? ensureBelow : ensureRightOf, posgrp_ );

    if ( setup_.vertical_ )
	vwr.attach( rightOf, this );
    else
	attach( alignedBelow, &vwr );

    addViewer( vwr );
    mainwin()->finaliseDone.notify( mCB(this,uiFlatViewControl,initStates) );
}


uiFlatViewControl::~uiFlatViewControl()
{
    delete &zoommgr_;
}


void uiFlatViewControl::addViewer( uiFlatViewer& vwr )
{
    vwrs_ += &vwr;
}


void uiFlatViewControl::updatePosButtonStates()
{
    zoomoutbut_->setSensitive( !zoommgr_.atStart() );

    const uiWorldRect bb( vwrs_[0]->boundingBox() );
    const uiWorldRect cv( vwrs_[0]->curView() );
    const Geom::Size2D<double> bbsz( bb.size() );
    const Geom::Size2D<double> bbszeps( bb.width() * 1e-6, bb.height() * 1e-6 );

    panleftbut_->setSensitive( cv.left() > bb.left() + bbszeps.width() );
    panrightbut_->setSensitive( cv.right() < bb.right() - bbszeps.width() );
    pandownbut_->setSensitive( cv.top() < bb.top() - bbszeps.height() );
    panupbut_->setSensitive( cv.bottom() > bb.bottom() + bbszeps.height() );
}


void uiFlatViewControl::initStates( CallBacker* but )
{
    const uiWorldRect bb( vwrs_[0]->boundingBox() );
    zoommgr_.init( bb );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setView( bb );

    updatePosButtonStates();
}


void uiFlatViewControl::zoomCB( CallBacker* but )
{
    const bool zoomin = but == zoominbut_;
    if ( but == zoominbut_ )
	zoommgr_.forward();
    else
	zoommgr_.back();

    const Geom::Size2D<double> newsz = zoommgr_.current();
    Geom::Point2D<double> centre( vwrs_[0]->curView().centre() );
    if ( zoommgr_.atStart() )
	centre = zoommgr_.initialCenter();

    setNewView( vwrs_[0]->curView().centre(),
	        Geom::Size2D<double>(newsz.width()*.5,newsz.height()*.5) );
}


void uiFlatViewControl::panCB( CallBacker* but )
{
    const bool isleft = but == panleftbut_;
    const bool isright = but == panrightbut_;
    const bool isup = but == panupbut_;
    const bool isdown = but == pandownbut_;
    const bool ishor = isleft || isright;

    uiWorldRect cv( vwrs_[0]->curView() );
    const bool isrev = ishor ? cv.left() > cv.right() : cv.bottom() > cv.top();

    const bool isxdown = (!isrev && isleft) || (isrev && isright);
    const bool isxup = (isrev && isleft) || (!isrev && isright);
    const bool isydown = (isrev && isup) || (!isrev && isdown);
    const bool isyup = (!isrev && isup) || (isrev && isdown);

    const double fac = 1 - zoommgr_.fwdFac();
    const double pandist = fac * (ishor ? cv.width() : cv.height());

    const uiWorldRect bb( vwrs_[0]->boundingBox() );
    const double bbsz = ishor ? bb.width() : bb.height();
    const double bbszeps = bbsz * 1e-5;
    const double halfcvsz = 0.5 * (ishor ? cv.width() : cv.height());
    Geom::Point2D<double> centre = cv.centre();

    if ( isxdown )
    {
	if ( centre.x - pandist - halfcvsz < bb.left() + bbszeps )
	    centre.x = bb.left() + halfcvsz;
	else
	    centre.x -= pandist;
    }
    if ( isxup )
    {
	if ( centre.x + pandist + halfcvsz > bb.right() - bbszeps )
	    centre.x = bb.right() - halfcvsz;
	else
	    centre.x += pandist;
    }
    if ( isydown )
    {
	if ( centre.y - pandist - halfcvsz < bb.bottom()+ bbszeps )
	    centre.y = bb.bottom() + halfcvsz;
	else
	    centre.y -= pandist;
    }
    if ( isyup )
    {
	if ( centre.y + pandist + halfcvsz > bb.top() - bbszeps )
	    centre.y = bb.top() - halfcvsz;
	else
	    centre.y += pandist;
    }

    setNewView( centre, cv.size() );
}


void uiFlatViewControl::setNewView( Geom::Point2D<double> centre,
				    Geom::Size2D<double> radius )
{
    const uiWorldRect cv( vwrs_[0]->curView() );
    if ( cv.left() > cv.right() ) radius.setWidth( -radius.width() );
    if ( cv.bottom() > cv.top() ) radius.setHeight( -radius.height() );

    uiWorldRect wr( centre.x - radius.width(), centre.y - radius.height(),
		    centre.x + radius.width(), centre.y + radius.height() );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setView( wr );

    updatePosButtonStates();
}


void uiFlatViewControl::stateCB( CallBacker* but )
{
    if ( !stategrp_ ) return;

    const bool ismanip = (but == manipbut_ && manipbut_->isOn())
		      || (but == drawbut_ && !drawbut_->isOn());

    if ( but == manipbut_ )
	drawbut_->setOn( !ismanip );
    else
	manipbut_->setOn( ismanip );
}


void uiFlatViewControl::parsCB( CallBacker* )
{
}
