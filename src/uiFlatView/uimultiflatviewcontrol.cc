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
    vwr.viewChanged.notify( mCB(this,uiMultiFlatViewControl,setZoomBoxesCB) );
    vwr.viewChanged.notify( mCB(this,uiMultiFlatViewControl,setZoomAreasCB) );
    parsbuts_ += parsbut_;
    toolbars_ += tb_;
    zoommgrs_ += &zoommgr_;

    finalPrepare();
}


uiMultiFlatViewControl::~uiMultiFlatViewControl()
{
    for ( int idx=zoommgrs_.size()-1; idx>=1; idx-- )
	delete zoommgrs_.removeSingle( idx );
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
    if ( !activevwr_ ) 
	return;

    uiWorldRect br = activevwr_->boundingBox();
    br.checkCorners();
    const uiWorldRect wr = getNewWorldRect(centre,sz,activevwr_->curView(),br); 
    const bool havezoom = haveZoom( activevwr_->curView().size(), sz );
    if ( (activevwr_ != &vwr_) && havezoom )
	 zoommgrs_[vwrs_.indexOf(activevwr_)]->add( sz );

    activevwr_->setView( wr );

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
    mevh.wheelMove.notify( mCB(this,uiMultiFlatViewControl,wheelMoveCB) );

    toolbars_ += new uiToolBar(mainwin(),"Flat Viewer Tools",tb_->prefArea());
    zoommgrs_ += new FlatView::ZoomMgr;

    parsbuts_ += new uiToolButton( toolbars_[ivwr],"2ddisppars",
	    "Set display parameters", mCB(this,uiMultiFlatViewControl,parsCB) );

    toolbars_[ivwr]->addButton( parsbuts_[ivwr] );

    vwr.setRubberBandingOn( !manip_ );
    vwr.viewChanged.notify( mCB(this,uiMultiFlatViewControl,vwChgCB) );
    //vwr.dispParsChanged.notify( mCB(this,uiMultiFlatViewControl,dispChgCB) );
    vwr.appearance().annot_.editable_ = false;
    vwr.viewChanged.notify( mCB(this,uiMultiFlatViewControl,setZoomAreasCB) );
    vwr.viewChanged.notify( mCB(this,uiMultiFlatViewControl,setZoomBoxesCB) );

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


void uiMultiFlatViewControl::dataChangeCB( CallBacker* cb )
{}


void uiMultiFlatViewControl::reInitZooms()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->setView( vwrs_[idx]->boundingBox() );
	zoommgrs_[idx]->reInit( vwrs_[idx]->boundingBox() );
	zoommgrs_[idx]->toStart();
    }
}


void uiMultiFlatViewControl::wheelMoveCB( CallBacker* cb )
{
    activevwr_ = vwrs_[0];
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if (vwrs_[idx]->rgbCanvas().getNavigationMouseEventHandler().hasEvent())
	    { activevwr_ = vwrs_[idx]; break; }
    }
    if ( !activevwr_ ) return;

    const MouseEvent& ev = 
	activevwr_->rgbCanvas().getNavigationMouseEventHandler().event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    zoomCB( ev.angle() < 0 ? zoominbut_ : zoomoutbut_ );
}


void uiMultiFlatViewControl::zoomCB( CallBacker* but )
{
    if ( !activevwr_ ) 
	activevwr_ = vwrs_[0];

    const bool zoomin = but == zoominbut_;
    doZoom( zoomin, *activevwr_, *zoommgrs_[vwrs_.indexOf( activevwr_ )] );
}


void uiMultiFlatViewControl::setZoomAreasCB( CallBacker* cb )
{
    if ( !iszoomcoupled_ || !activeVwr() ) return;

    const uiWorldRect& masterbbox = activeVwr()->boundingBox();
    const uiWorldRect& wr = activeVwr()->curView();

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( vwrs_[idx] == activeVwr() )
	    continue;

	const uiWorldRect& bbox = vwrs_[idx]->boundingBox();
	const uiWorldRect& oldwr = vwrs_[idx]->curView();
	LinScaler sclr( masterbbox.left(), bbox.left(),
		        masterbbox.right(), bbox.right() );
	LinScaler sctb( masterbbox.top(), bbox.top(),
		        masterbbox.bottom(), bbox.bottom() );
	uiWorldRect newwr( sclr.scale(wr.left()), sctb.scale(wr.top()),
			   sclr.scale(wr.right()), sctb.scale(wr.bottom()) );
	NotifyStopper ns( vwrs_[idx]->viewChanged );
	vwrs_[idx]->setView( newwr );

	const bool havezoom = haveZoom( oldwr.size(), newwr.size() );
	if ( havezoom )
	    zoommgr_.add( newwr.size() );
    }
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

