/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewcontrol.cc,v 1.9 2007-03-01 15:07:15 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"

#include "flatviewzoommgr.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uirgbarraycanvas.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiworld2ui.h"

#include "pixmap.h"

#define mDefBut(butnm,grp,fnm,cbnm,tt) \
    butnm = new uiToolButton( grp, 0, ioPixmap(fnm), \
			      mCB(this,uiFlatViewControl,cbnm) ); \
    butnm->setToolTip( tt )

uiFlatViewControl::uiFlatViewControl( uiFlatViewer& vwr, const Setup& s )
    : uiGroup(vwr.attachObj()->parent(),"Flat viewer control")
    , setup_(s)
    , stategrp_(0)
    , propdlg_(0)
    , zoommgr_(*new FlatView::ZoomMgr)
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
    uiToolButton* 
    mDefBut(fliplrbut,posgrp_,"flip_lr.png",flipCB,"Flip left/right");

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
    if ( mainwin() ) //TODO: I need a finalise callback
	mainwin()->finaliseDone.notify( mCB(this,uiFlatViewControl,initStates));

    vwr.rgbCanvas().setRubberBandingOn( OD::LeftButton );
    vwr.rgbCanvas().rubberBandUsed.notify( mCB(this,uiFlatViewControl,
					       rubBandCB));
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


uiWorldRect uiFlatViewControl::getBoundingBox() const
{
    uiWorldRect bb( vwrs_[0]->boundingBox() );

    for ( int idx=1; idx<vwrs_.size(); idx++ )
    {
	const uiWorldRect bb2( vwrs_[idx]->boundingBox() );
	if ( bb2.left() < bb.left() ) bb.setLeft( bb2.left() );
	if ( bb2.right() > bb.right() ) bb.setRight( bb2.right() );
	if ( bb2.bottom() < bb.bottom() ) bb.setBottom( bb2.bottom() );
	if ( bb2.top() > bb.top() ) bb.setTop( bb2.top() );
    }

    return bb;
}


void uiFlatViewControl::initStates( CallBacker* but )
{
    const uiWorldRect bb( getBoundingBox() );
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

    Geom::Size2D<double> newsz = zoommgr_.current();
    Geom::Point2D<double> centre( vwrs_[0]->curView().centre() );
    if ( zoommgr_.atStart() )
	centre = zoommgr_.initialCenter();

    setNewView( centre, newsz );
}


void uiFlatViewControl::panCB( CallBacker* but )
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


bool uiFlatViewControl::havePan(
	Geom::Point2D<double> oldcentre, Geom::Point2D<double> newcentre,
	Geom::Size2D<double> sz )
{
    const Geom::Point2D<double> eps( sz.width()*1e-6, sz.height()*1e-6 );
    return !mIsZero(oldcentre.x-newcentre.x,eps.x)
	|| !mIsZero(oldcentre.y-newcentre.y,eps.y);
}


bool uiFlatViewControl::haveZoom( Geom::Size2D<double> oldsz,
				  Geom::Size2D<double> newsz )
{
    const Geom::Point2D<double> eps( oldsz.width()*1e-6, oldsz.height()*1e-6 );
    return !mIsZero(oldsz.width()-newsz.width(),eps.x)
	|| !mIsZero(oldsz.height()-newsz.height(),eps.y);
}


void uiFlatViewControl::setNewView( Geom::Point2D<double>& centre,
				    Geom::Size2D<double>& sz )
{
    const uiWorldRect cv( vwrs_[0]->curView() );
    const bool havepan = havePan( cv.centre(), centre, cv.size() );
    const bool havezoom = haveZoom( cv.size(), sz );
    if ( !havepan && !havezoom ) return;

    uiWorldRect wr( havepan && havezoom ? getZoomAndPanRect(centre,sz)
	    				: getZoomOrPanRect(centre,sz) );
    if ( cv.left() > cv.right() ) wr.swapHor();
    if ( cv.bottom() > cv.top() ) wr.swapVer();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setView( wr );

    centre = wr.centre(); sz = wr.size();
    if ( havezoom )
	zoommgr_.add( sz );
    updatePosButtonStates();
}


uiWorldRect uiFlatViewControl::getZoomAndPanRect( Geom::Point2D<double> centre,
					      Geom::Size2D<double> sz ) const
{
    //TODO have different policy
    return getZoomOrPanRect( centre, sz );
}


uiWorldRect uiFlatViewControl::getZoomOrPanRect( Geom::Point2D<double> centre,
						 Geom::Size2D<double> sz ) const
{
    const uiWorldRect bb( getBoundingBox() );
    if ( sz.width() > bb.width() )
	sz.setWidth( bb.width() );
    if ( sz.height() > bb.height() )
	sz.setHeight( bb.height() );
    const double hwdth = sz.width() * .5;
    const double hhght = sz.height() * .5;

    if ( centre.x - hwdth < bb.left() )		centre.x = bb.left() + hwdth;
    if ( centre.y - hhght < bb.bottom() )	centre.y = bb.bottom() + hhght;
    if ( centre.x + hwdth > bb.right() )	centre.x = bb.right() - hwdth;
    if ( centre.y + hhght > bb.top() )		centre.y = bb.top() - hhght;

    return uiWorldRect( centre.x - hwdth, centre.y + hhght,
			centre.x + hwdth, centre.y - hhght );
}


void uiFlatViewControl::flipCB( CallBacker* but )
{
    flip( true );
}


void uiFlatViewControl::flip( bool hor )
{
    const uiWorldRect cv( vwrs_[0]->curView() );
    const uiWorldRect newview(	hor ? cv.right()  : cv.left(),
				hor ? cv.top()    : cv.bottom(),
				hor ? cv.left()   : cv.right(),
				hor ? cv.bottom() : cv.top() );

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	FlatView::Annotation::AxisData& ad = hor
					   ? vwrs_[idx]->context().annot_.x1_
					   : vwrs_[idx]->context().annot_.x2_;
	ad.reversed_ = !ad.reversed_;
	vwrs_[idx]->setView( newview );
    }

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


void uiFlatViewControl::rubBandCB( CallBacker* cb )
{
    //TODO handle when zoom is disabled
    mCBCapsuleUnpack(uiRect,r,cb);
    uiSize sz = r.getPixelSize();
    if ( sz.hNrPics() == 0 && sz.vNrPics() == 0 )
	return;

    uiWorld2Ui w2u;
    vwrs_[0]->getWorld2Ui(w2u);
    uiWorldRect wr = w2u.transform(r);
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();

    const uiWorldRect oldview( vwrs_[0]->curView() );
    setNewView( centre, newsz );
}


void uiFlatViewControl::parsCB( CallBacker* )
{
    if ( propdlg_ ) delete propdlg_;
    propdlg_ = new uiFlatViewPropDlg( vwrs_[0]->attachObj()->parent(),
	    			vwrs_[0]->context().ddpars_,
				vwrs_[0]->context().annot_.x1_,
				vwrs_[0]->context().annot_.x2_,
			    	mCB(this,uiFlatViewControl,applyProperties) );
    propdlg_->windowClosed.notify(mCB(this,uiFlatViewControl,propDlgClosed));
    propdlg_->go();
}


void uiFlatViewControl::propDlgClosed( CallBacker* )
{
    if ( propdlg_->uiResult() == 1 )
    {
	applyProperties(0);
	if ( propdlg_->saveButtonChecked() )
	    saveProperties();
    }
}


void uiFlatViewControl::applyProperties( CallBacker* cb )
{
    if ( !propdlg_ ) return;

    uiWVAFVPropTab* wvatab = propdlg_->wvaproptab_;
    uiVDFVPropTab* vdtab = propdlg_->vdproptab_;
    if ( vdtab )
    {
	if ( cb )
	    vdtab->fillDispPars();

	for ( int idx=0; idx<vwrs_.size(); idx++ )
	{
	    vwrs_[idx]->context().ddpars_.vd_ = vdtab->vdpars_;
	    vwrs_[idx]->context().ddpars_.dispvd_ = vdtab->dispvd_;
	    vwrs_[idx]->handleChange(FlatView::Viewer::VDPars);
	}
    }
    if ( wvatab )
    {
	if ( cb )
	    wvatab->fillDispPars();

	for ( int idx=0; idx<vwrs_.size(); idx++ )
	{
	    vwrs_[idx]->context().ddpars_.wva_ = wvatab->wvapars_;
	    vwrs_[idx]->context().ddpars_.dispwva_ = wvatab->dispwva_;
	    vwrs_[idx]->handleChange(FlatView::Viewer::WVAPars);
	}
    }
}


void uiFlatViewControl::saveProperties()
{
    /*
    Settings& setts = Settings::fetch( "flatdisp" );
    context().fillPar( setts );
    setts.write();
    */
}
