/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimultiflatviewcontrol.cc,v 1.1 2012-02-15 15:52:29 cvsbruno Exp $";

#include "uimultiflatviewcontrol.h"

#include "uiflatviewer.h"
#include "uigraphicsscene.h"
#include "uitoolbutton.h"
#include "uirgbarraycanvas.h"
#include "uiworld2ui.h"

#include "flatviewzoommgr.h"
#include "mouseevent.h"
#include "pixmap.h"

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(tb_,fnm,tt,mCB(this,uiMultiFlatViewControl,cbnm) ); \
    tb_->addButton( but );

uiMultiFlatViewControl::uiMultiFlatViewControl( uiFlatViewer& vwr,
				    const uiFlatViewStdControl::Setup& setup )
    : uiFlatViewStdControl(vwr,setup)
{
    zoommgrs_ += &zoommgr_;
    finalPrepare();
}


uiMultiFlatViewControl::~uiMultiFlatViewControl()
{
    for ( int idx=zoommgrs_.size()-1; idx>=1; idx++ )
	delete zoommgrs_.remove( idx );
}


void uiMultiFlatViewControl::setNewView(Geom::Point2D<double>& centre,
	                                           Geom::Size2D<double>& sz)
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	uiFlatViewer& vwr = *vwrs_[idx];
	if ( !vwr.rgbCanvas().getNavigationMouseEventHandler().hasEvent() )
	    continue;

	uiWorldRect br = vwrs_[idx]->boundingBox();
	br.checkCorners();
	const uiWorldRect wr = getNewWorldRect( centre, sz, vwr.curView(),br ); 

	vwr.setView( wr );
    }	
}


void uiMultiFlatViewControl::vwrAdded( CallBacker* )
{
    MouseEventHandler& mevh =
	    vwrs_[vwrs_.size()-1]->rgbCanvas().getNavigationMouseEventHandler();
    mevh.wheelMove.notify( mCB(this,uiMultiFlatViewControl,wheelMoveCB) );
    zoommgrs_ += new FlatView::ZoomMgr;
    reInitZooms();
}


void uiMultiFlatViewControl::rubBandCB( CallBacker* cb )
{
    mDynamicCastGet( uiFlatViewer*, vwr, cb );
    if ( !vwr ) return;

    const uiRect* selarea = vwr->rgbCanvas().getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight()) ||
	(selarea->width()<5 && selarea->height()<5) )
	return;

    uiWorld2Ui w2u;
    vwr->getWorld2Ui(w2u);
    uiWorldRect wr = w2u.transform(*selarea);
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();

    const uiWorldRect oldview( vwr->curView() );
    setNewView( centre, newsz );
}


void uiMultiFlatViewControl::editCB( CallBacker* )
{
    uiGraphicsViewBase::ODDragMode mode;
    if ( editbut_->isOn() )
	mode = uiGraphicsViewBase::NoDrag;
    else
	mode = manip_ ? uiGraphicsViewBase::ScrollHandDrag
	      : uiGraphicsViewBase::RubberBandDrag;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->rgbCanvas().setDragMode( mode );
	vwrs_[idx]->rgbCanvas().scene().setMouseEventActive( true );
	vwrs_[idx]->appearance().annot_.editable_ = editbut_->isOn();
    }
}


void uiMultiFlatViewControl::dataChangeCB( CallBacker* cb )
{
    reInitZooms();
}


void uiMultiFlatViewControl::reInitZooms()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->setView( vwrs_[idx]->boundingBox() );
	zoommgrs_[idx]->init( vwrs_[idx]->boundingBox() );
    }
}


void uiMultiFlatViewControl::zoomCB( CallBacker* but )
{
    uiFlatViewer* vwr =  0; FlatView::ZoomMgr* zoommgr =0;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwr = vwrs_[idx]; zoommgr = zoommgrs_[idx];
	if ( vwr->rgbCanvas().getNavigationMouseEventHandler().hasEvent() )
	    break;
    }
    if ( !vwr ) return;

    const bool zoomin = but == zoominbut_;
    doZoom( zoomin, *vwr, *zoommgr );
}
