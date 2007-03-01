/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:		Feb 2007
 RCS:           $Id: uiflatviewcontrol.cc,v 1.10 2007-03-01 19:35:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"
#include "flatviewzoommgr.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uirgbarraycanvas.h"
#include "uiworld2ui.h"

#define mDefBut(butnm,grp,fnm,cbnm,tt) \
    butnm = new uiToolButton( grp, 0, ioPixmap(fnm), \
			      mCB(this,uiFlatViewControl,cbnm) ); \
    butnm->setToolTip( tt )

uiFlatViewControl::uiFlatViewControl( uiFlatViewer& vwr, uiParent* p,
				      bool wrubb )
    : uiGroup(p ? p : vwr.attachObj()->parent(),"Flat viewer control")
    , zoommgr_(*new FlatView::ZoomMgr)
    , haverubber_(wrubb)
    , propdlg_(0)
{
    setBorder( 0 );
    addViewer( vwr );
    if ( mainwin() ) //TODO: I need a finalise callback
	mainwin()->finaliseDone.notify( mCB(this,uiFlatViewControl,onFinalise));
}


uiFlatViewControl::~uiFlatViewControl()
{
    delete &zoommgr_;
}


void uiFlatViewControl::addViewer( uiFlatViewer& vwr )
{
    vwrs_ += &vwr;
    if ( haverubber_ )
    {
	vwr.rgbCanvas().setRubberBandingOn( OD::LeftButton );
	vwr.rgbCanvas().rubberBandUsed.notify(
		    mCB(this,uiFlatViewControl,rubBandCB));
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


void uiFlatViewControl::onFinalise( CallBacker* )
{
    const uiWorldRect bb( getBoundingBox() );
    zoommgr_.init( bb );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setView( bb );

    finalPrepare();
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
    centre = wr.centre(); sz = wr.size();
    if ( havezoom )
	zoommgr_.add( sz );

    if ( cv.left() > cv.right() ) wr.swapHor();
    if ( cv.bottom() > cv.top() ) wr.swapVer();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setView( wr );
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


void uiFlatViewControl::doPropertiesDialog()
{
    //TODO what if more than one viewer? Also functions below
    uiFlatViewer& vwr = *vwrs_[0];

    if ( propdlg_ ) delete propdlg_;
    propdlg_ = new uiFlatViewPropDlg( vwr.attachObj()->parent(), vwr,
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

    uiFlatViewer& vwr = *vwrs_[0];
    vwr.handleChange( FlatView::Viewer::All );
}


void uiFlatViewControl::saveProperties()
{
}
