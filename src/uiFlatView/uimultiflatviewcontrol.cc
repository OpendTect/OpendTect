/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimultiflatviewcontrol.h"

#include "uiflatviewer.h"
#include "uimainwin.h"
#include "uigraphicsscene.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uirgbarraycanvas.h"
#include "uiworld2ui.h"

#include "flatviewzoommgr.h"
#include "scaler.h"
#include "mouseevent.h"
#include "pixmap.h"

uiMultiFlatViewControl::uiMultiFlatViewControl( uiFlatViewer& vwr,
				    const uiFlatViewStdControl::Setup& setup )
    : uiFlatViewStdControl(vwr,setup)
    , iszoomcoupled_(true)
    , drawzoomboxes_(false)			  
    , activevwr_(0)  
{
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomBoxesCB );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomAreasCB );
    parsbuts_ += parsbut_;
    toolbars_ += tb_;

    finalPrepare();
}


uiMultiFlatViewControl::~uiMultiFlatViewControl()
{
    detachAllNotifiers();
}


bool uiMultiFlatViewControl::setActiveVwr( int vwridx )
{
    if ( !vwrs_.validIdx(vwridx) )
	return false;

    activevwr_ = vwrs_[vwridx];
    return true;
}


void uiMultiFlatViewControl::setNewView(Geom::Point2D<double>& centre,
					Geom::Size2D<double>& sz)
{
    if ( !activevwr_ ) return;

    uiWorldRect br = activevwr_->boundingBox();
    br.sortCorners();
    const uiWorldRect wr = getNewWorldRect(centre,sz,activevwr_->curView(),br); 
    activevwr_->setView( wr );
    zoommgr_.add( sz, vwrs_.indexOf(activevwr_) );

    zoomChanged.trigger();
}


#define mAddBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(tb_,fnm,tt,mCB(this,uiFlatViewStdControl,cbnm) ); \
    tb_->addButton( but );

void uiMultiFlatViewControl::vwrAdded( CallBacker* )
{
    const int ivwr = vwrs_.size()-1;
    uiFlatViewer& vwr = *vwrs_[ivwr];
    MouseEventHandler& mevh = vwr.rgbCanvas().getNavigationMouseEventHandler();
    mAttachCB( mevh.wheelMove, uiMultiFlatViewControl::wheelMoveCB );

    toolbars_ += new uiToolBar(mainwin(),"Flat Viewer Tools",tb_->prefArea());

    parsbuts_ += new uiToolButton( toolbars_[ivwr],"2ddisppars",
	    "Set display parameters", mCB(this,uiMultiFlatViewControl,parsCB) );

    toolbars_[ivwr]->addButton( parsbuts_[ivwr] );

    vwr.setRubberBandingOn( !manip_ );
    vwr.appearance().annot_.editable_ = false;
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::vwChgCB );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomAreasCB );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomBoxesCB );

    reInitZooms();
}


void uiMultiFlatViewControl::rubBandCB( CallBacker* cb )
{
    mDynamicCastGet( uiGraphicsView*,cnv, cb );

    activevwr_ = 0;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( &vwrs_[idx]->rgbCanvas()  == cnv )
	    { activevwr_ = vwrs_[idx]; break; }
    }
    if ( !activevwr_ ) return;

    const uiRect* selarea = activevwr_->rgbCanvas().getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight()) ||
	(selarea->width()<5 && selarea->height()<5) )
	return;

    uiWorld2Ui w2u;
    activevwr_->getWorld2Ui(w2u);
    uiWorldRect wr = w2u.transform(*selarea);
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();

    setNewView( centre, newsz );
}


void uiMultiFlatViewControl::reInitZooms()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setViewToBoundingBox();
    zoommgr_.reInit( getBoundingBoxes() );
    zoommgr_.toStart();
}


void uiMultiFlatViewControl::wheelMoveCB( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh ) return;

    const int vwridx = getViewerIdx(meh);
    if ( vwridx<0 ) return;
    activevwr_ = vwrs_[vwridx];

    const MouseEvent& ev = 
	activevwr_->rgbCanvas().getNavigationMouseEventHandler().event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    zoomCB( ev.angle() < 0 ? zoominbut_ : zoomoutbut_ );
}


void uiMultiFlatViewControl::zoomCB( CallBacker* but )
{
    if ( !activevwr_ ) activevwr_ = vwrs_[0];

    const MouseEventHandler& meh =
	activevwr_->rgbCanvas().getNavigationMouseEventHandler();
    const bool wheelmoved= meh.hasEvent() && !mIsZero(meh.event().angle(),0.01);
    const bool zoomin = but == zoominbut_;
    if ( !zoomin && !wheelmoved && zoommgr_.atStart(vwrs_.indexOf(activevwr_)) )
	for ( int idx=0; idx<vwrs_.size(); idx++ )
	    if ( !zoommgr_.atStart(idx) ) { activevwr_ = vwrs_[idx]; break; }

    doZoom( zoomin, *activevwr_ );
}


void uiMultiFlatViewControl::setZoomAreasCB( CallBacker* cb )
{
    if ( !iszoomcoupled_ || !activeVwr() ) return;

    mDynamicCastGet(uiFlatViewer*,vwr,cb);
    if ( !vwrs_.isPresent(vwr) ) return;
    activevwr_ = vwr;

    const uiWorldRect& masterbbox = activeVwr()->boundingBox();
    const uiWorldRect& wr = activeVwr()->curView();
    bool havezoom = false;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( vwrs_[idx] == activeVwr() )
	    continue;

	const uiWorldRect bbox = vwrs_[idx]->boundingBox();
	const uiWorldRect oldwr = vwrs_[idx]->curView();
	LinScaler sclr( masterbbox.left(), bbox.left(),
		        masterbbox.right(), bbox.right() );
	LinScaler sctb( masterbbox.top(), bbox.top(),
		        masterbbox.bottom(), bbox.bottom() );
	uiWorldRect newwr( sclr.scale(wr.left()), sctb.scale(wr.top()),
			   sclr.scale(wr.right()), sctb.scale(wr.bottom()) );
	NotifyStopper ns( vwrs_[idx]->viewChanged );
	vwrs_[idx]->setView( newwr );

	if ( !havezoom )
	    havezoom = haveZoom( oldwr.size(), newwr.size() );
    }

    addSizesToZoomMgr();
    if ( havezoom )
	zoomChanged.trigger();
}


bool uiMultiFlatViewControl::handleUserClick()
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

    if ( iszoomcoupled_ || !activeVwr() || !drawzoomboxes_ ) 
	return;

    const uiWorldRect& masterbbox = activeVwr()->boundingBox();
    const uiWorldRect& wr = activeVwr()->curView();

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	FlatView::AuxData* ad = vwrs_[idx]->createAuxData( "Zoom box" );
	vwrs_[idx]->addAuxData( ad );
	zoomboxes_ += ad;
	ad->linestyle_ = LineStyle( LineStyle::Dash, 3, Color::Black() );
	ad->zvalue_ = 5;

	if ( vwrs_[idx] == activeVwr() || wr == masterbbox )
	    continue;

	const uiWorldRect& bbox = vwrs_[idx]->boundingBox();
	LinScaler sclr( masterbbox.left(), bbox.left(),
		        masterbbox.right(), bbox.right() );
	LinScaler sctb( masterbbox.top(), bbox.top(),
		        masterbbox.bottom(), bbox.bottom() );
	uiWorldRect newwr( sclr.scale(wr.left()), sctb.scale(wr.top()),
			   sclr.scale(wr.right()), sctb.scale(wr.bottom()) );
	ad->poly_ += newwr.topLeft(); 
	ad->poly_ += newwr.topRight();
	ad->poly_ += newwr.bottomRight();
	ad->poly_ += newwr.bottomLeft(); 
	ad->poly_ += newwr.topLeft(); 
	vwrs_[idx]->handleChange( FlatView::Viewer::Auxdata ); 
    }
}

