/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
________________________________________________________________________

-*/

#include "uimultiflatviewcontrol.h"

#include "uiflatviewer.h"
#include "uimainwin.h"
#include "uigraphicsscene.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uirgbarraycanvas.h"

#include "scaler.h"
#include "survinfo.h"


MFVCViewManager::~MFVCViewManager()
{
    deepErase( d2tmodels_ );
}


void MFVCViewManager::setD2TModels(const ObjectSet<const TimeDepthModel>& d2tms)
{
    deepErase( d2tmodels_ );
    deepCopy( d2tmodels_, d2tms );
}


void MFVCViewManager::setViewerType( const uiFlatViewer* vwr, bool isintime )
{
    const int vwridx = vwrs_.indexOf( vwr );
    if ( vwridx<0 )
    {
	vwrs_ += vwr;
	zintimeflags_ += BoolTypeSetType( isintime );
	return;
    }
    zintimeflags_[vwridx] = isintime;
}


bool MFVCViewManager::getViewRect( const uiFlatViewer* activevwr,
				   const uiFlatViewer* curvwr,
				   uiWorldRect& viewwr ) const
{
    const int activevwridx = vwrs_.indexOf( activevwr );
    const int curvwridx = vwrs_.indexOf( curvwr );
    if ( !vwrs_.validIdx(curvwridx) || !vwrs_.validIdx(activevwridx) ||
	 activevwridx<0 )
	return false;
    const uiWorldRect& wr = activevwr->curView();
    float srdval = SI().seismicReferenceDatum();
    if ( SI().depthsInFeet() )
	srdval *= mToFeetFactorF;

    if ( d2tmodels_.isEmpty() )
    {
	const uiWorldRect& mainbbox = activevwr->boundingBox();
	const uiWorldRect bbox = vwrs_[curvwridx]->boundingBox();
	LinScaler sclr( mainbbox.left(), bbox.left(),
			mainbbox.right(), bbox.right() );
	LinScaler sctb( mainbbox.top(), bbox.top(),
			mainbbox.bottom(), bbox.bottom() );
	viewwr = uiWorldRect( sclr.scale(wr.left()), sctb.scale(wr.top()),
			      sclr.scale(wr.right()), sctb.scale(wr.bottom()) );
    }
    else
    {
	if ( !zintimeflags_.validIdx(curvwridx) ||
	     !zintimeflags_.validIdx(activevwridx) )
	    return false;
	const bool isactiveintime = zintimeflags_[activevwridx];
	const bool iscurintime = zintimeflags_[curvwridx];
	if ( isactiveintime == iscurintime )
	{
	    viewwr = wr;
	    return true;
	}
	viewwr.setLeft( wr.left() );
	viewwr.setRight( wr.right() );

	if ( isactiveintime )
	{
	    Interval<float> timerg( mCast(float,wr.top()),
				    mCast(float,wr.bottom()) );
	    Interval<double> depthrg( d2tmodels_[0]->getDepth(timerg.start),
				      d2tmodels_[0]->getDepth(timerg.stop) );
	    if ( !depthrg.isUdf() && SI().depthsInFeet() )
		depthrg.scale( mToFeetFactorF );
	    for ( int idx=1; idx<d2tmodels_.size(); idx++ )
	    {
		const TimeDepthModel& d2t = *d2tmodels_[idx];
		Interval<double> curdepthrg( d2t.getDepth(timerg.start),
					     d2t.getDepth(timerg.stop) );
		if ( !curdepthrg.isUdf() )
		{
		    if ( SI().depthsInFeet() )
			curdepthrg.scale( mToFeetFactorF );
		    depthrg.include( curdepthrg );
		}
	    }

	    if ( isFlattened() && !depthrg.isUdf() )
		depthrg.shift( srdval );
	    viewwr.setTop( depthrg.start );
	    viewwr.setBottom( depthrg.stop );
	}
	else
	{
	    Interval<float> depthrg( mCast(float,wr.top()),
				     mCast(float,wr.bottom()) );
	    if ( isFlattened() )
		depthrg.shift( -srdval );

	    if ( SI().depthsInFeet() )
		depthrg.scale( mFromFeetFactorF );

	    Interval<double> timerg( d2tmodels_[0]->getTime(depthrg.start),
				      d2tmodels_[0]->getTime(depthrg.stop) );
	    for ( int idx=1; idx<d2tmodels_.size(); idx++ )
	    {
		const TimeDepthModel& d2t = *d2tmodels_[idx];
		Interval<double> curtimerg( d2t.getTime(depthrg.start),
					     d2t.getTime(depthrg.stop) );
		if ( !curtimerg.isUdf() )
		    timerg.include( curtimerg );
	    }

	    viewwr.setTop( timerg.start );
	    viewwr.setBottom( timerg.stop );
	}
    }

    return true;
}


uiMultiFlatViewControl::uiMultiFlatViewControl( uiFlatViewer& vwr,
				    const uiFlatViewStdControl::Setup& setup )
    : uiFlatViewStdControl(vwr,setup)
    , iszoomcoupled_(true)
    , drawzoomboxes_(false)
    , activevwr_(&vwr)
{
    setViewerType( &vwr, true );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomBoxesCB );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomAreasCB );
    parsbuts_ += parsbut_;
    toolbars_ += tb_;
}


uiMultiFlatViewControl::~uiMultiFlatViewControl()
{
    detachAllNotifiers();

    for ( int idx=0; idx<zoomboxes_.size(); idx++ )
	vwrs_[idx]->removeAuxData( zoomboxes_[idx] );

    deepErase( zoomboxes_ );
}


bool uiMultiFlatViewControl::setActiveVwr( int vwridx )
{
    if ( !vwrs_.validIdx(vwridx) )
	return false;

    activevwr_ = vwrs_[vwridx];
    return true;
}


#define mAddBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(tb_,fnm,tt,mCB(this,uiFlatViewStdControl,cbnm) ); \
    tb_->addObject( but );

void uiMultiFlatViewControl::vwrAdded( CallBacker* )
{
    const int ivwr = vwrs_.size()-1;
    uiFlatViewer& vwr = *vwrs_[ivwr];
    MouseEventHandler& mevh = vwr.rgbCanvas().getNavigationMouseEventHandler();
    mAttachCB( mevh.wheelMove, uiMultiFlatViewControl::wheelMoveCB );

    toolbars_ += new uiToolBar(mainwin(),tr("Flat Viewer Tools"),
			       tb_->prefArea());

    parsbuts_ += new uiToolButton( toolbars_[ivwr],"2ddisppars",
				   tr("Set display parameters"),
				   mCB(this,uiMultiFlatViewControl,parsCB) );
    toolbars_[ivwr]->add( parsbuts_[ivwr] );

    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomAreasCB );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomBoxesCB );
    setViewerType( &vwr, true );
}


void uiMultiFlatViewControl::rubBandCB( CallBacker* cb )
{
    mDynamicCastGet( uiGraphicsView*,cnv, cb );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	if ( &vwrs_[idx]->rgbCanvas()  == cnv )
	    { activevwr_ = vwrs_[idx]; break; }

    const uiRect* selarea = activevwr_->rgbCanvas().getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight()) ||
	(selarea->width()<5 && selarea->height()<5) )
	return;

    const uiWorldRect wr = activevwr_->getWorld2Ui().transform( *selarea );
    setNewView( wr.centre(), wr.size(), *activevwr_ );
    rubberBandUsed.trigger();
}


void uiMultiFlatViewControl::updateZoomManager()
{
    if ( !iszoomcoupled_ )
    {
	zoommgr_.add( activevwr_->curView().size(), vwrs_.indexOf(activevwr_) );
	zoomChanged.trigger();
	return;
    }

    uiFlatViewControl::updateZoomManager();
}


void uiMultiFlatViewControl::wheelMoveCB( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !meh->hasEvent() ) return;

    const int vwridx = getViewerIdx( meh, false );
    if ( vwridx<0 ) return;
    activevwr_ = vwrs_[vwridx];

    uiFlatViewStdControl::wheelMoveCB( cb );
}


void uiMultiFlatViewControl::zoomCB( CallBacker* but )
{
    const MouseEventHandler& meh =
	activevwr_->rgbCanvas().getNavigationMouseEventHandler();
    const bool zoomin = but==zoominbut_ || but==vertzoominbut_;
    const int vwridx = vwrs_.indexOf(activevwr_);
    if ( !zoomin && !meh.hasEvent() && zoommgr_.atStart(vwridx) )
	for ( int idx=0; idx<vwrs_.size(); idx++ )
	    if ( !zoommgr_.atStart(idx) ) { activevwr_ = vwrs_[idx]; break; }

    const bool onlyvertzoom = but==vertzoominbut_ || but==vertzoomoutbut_;
    doZoom( zoomin, onlyvertzoom, *activevwr_ );
}


void uiMultiFlatViewControl::pinchZoomCB( CallBacker* cb )
{
    mDynamicCastGet(const GestureEventHandler*,evh,cb);
    if ( !evh || evh->isHandled() )
	return;

    const GestureEvent* gevent = evh->getPinchEventInfo();
    if ( !gevent )
	return;

    int vwridx = -1;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( evh == &(vwrs_[idx]->rgbCanvas().gestureEventHandler()) )
	{
	    vwridx = idx;
	    break;
	}
    }

    if ( vwridx<0 ) return;
    activevwr_ = vwrs_[vwridx];
    const Geom::Size2D<double> cursz = activevwr_->curView().size();

    const float scalefac = gevent->scale();
    Geom::Size2D<double> newsz( cursz.width() * (1/scalefac),
				cursz.height() * (1/scalefac) );
    Geom::Point2D<double> pos =
	activevwr_->getWorld2Ui().transform( gevent->pos() );

    const uiWorldRect wr = getZoomOrPanRect( pos, newsz, activevwr_->curView(),
					     activevwr_->boundingBox());
    vwrs_[vwridx]->setView( wr );

    if ( gevent->getState() == GestureEvent::Finished )
	updateZoomManager();
}


void uiMultiFlatViewControl::setZoomAreasCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewer*,vwr,cb);
    if ( !iszoomcoupled_ || !vwrs_.isPresent(vwr) ) return;
    activevwr_ = vwr;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( vwrs_[idx] == activeVwr() )
	    continue;

	uiWorldRect newwr;
	if ( !viewmgr_.getViewRect(activeVwr(),vwrs_[idx],newwr) )
	    continue;

	NotifyStopper ns( vwrs_[idx]->viewChanged );
	vwrs_[idx]->setView( newwr );
    }
}


bool uiMultiFlatViewControl::handleUserClick( int vwridx )
{
    return true;
}


void uiMultiFlatViewControl::parsCB( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb);
    const int idx = parsbuts_.indexOf( but );
    if ( idx >= 0 )
	doPropertiesDialog( idx );
}


void uiMultiFlatViewControl::setZoomBoxesCB( CallBacker* cb )
{
    for ( int idx=0; idx<zoomboxes_.size(); idx++ )
	delete vwrs_[idx]->removeAuxData( zoomboxes_[idx] );

    zoomboxes_.erase();

    if ( iszoomcoupled_ || !drawzoomboxes_ )
	return;

    const uiWorldRect& mainbbox = activeVwr()->boundingBox();
    const uiWorldRect& wr = activeVwr()->curView();
    if ( wr == mainbbox )
	return;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	FlatView::AuxData* ad = vwrs_[idx]->createAuxData( "Zoom box" );
	vwrs_[idx]->addAuxData( ad );
	zoomboxes_ += ad;
	ad->linestyle_ = OD::LineStyle(OD::LineStyle::Dash,3,Color::Black());
	ad->zvalue_ = uiFlatViewer::auxDataZVal();

	if ( vwrs_[idx] == activeVwr() )
	    continue;

	uiWorldRect newwr;
	if ( !viewmgr_.getViewRect(activeVwr(),vwrs_[idx],newwr) )
	    continue;
	ad->poly_ += newwr.topLeft();
	ad->poly_ += newwr.topRight();
	ad->poly_ += newwr.bottomRight();
	ad->poly_ += newwr.bottomLeft();
	ad->poly_ += newwr.topLeft();
	vwrs_[idx]->handleChange( FlatView::Viewer::Auxdata );
    }
}


uiToolButton* uiMultiFlatViewControl::parsButton( const uiFlatViewer* vwr )
{
    const int vwridx = vwrs_.indexOf( vwr );
    return vwridx >=0 && parsbuts_.validIdx(vwridx) ? parsbuts_[ vwridx ] : 0;
}
