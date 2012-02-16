/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimultiflatviewcontrol.cc,v 1.3 2012-02-16 16:14:57 cvsbruno Exp $";

#include "uimultiflatviewcontrol.h"

#include "uiflatviewer.h"
#include "uigraphicsscene.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uirgbarraycanvas.h"
#include "uiworld2ui.h"

#include "flatviewzoommgr.h"
#include "mouseevent.h"
#include "pixmap.h"

uiMultiFlatViewControl::uiMultiFlatViewControl( uiFlatViewer& vwr,
				    const uiFlatViewStdControl::Setup& setup )
    : uiFlatViewStdControl(vwr,setup)
    , activevwr_(0)  
{
    toolbars_ += tb_;
    zoommgrs_ += &zoommgr_;
    finalPrepare();
}


uiMultiFlatViewControl::~uiMultiFlatViewControl()
{
    for ( int idx=zoommgrs_.size()-1; idx>=1; idx-- )
	delete zoommgrs_.remove( idx );
}


void uiMultiFlatViewControl::setNewView(Geom::Point2D<double>& centre,
	                                           Geom::Size2D<double>& sz)
{
    if ( !activevwr_ ) 
	return;

    uiWorldRect br = activevwr_->boundingBox();
    br.checkCorners();
    const uiWorldRect wr = getNewWorldRect(centre,sz,activevwr_->curView(),br); 

    activevwr_->setView( wr );
}


void uiMultiFlatViewControl::vwrAdded( CallBacker* )
{
    MouseEventHandler& mevh =
	    vwrs_[vwrs_.size()-1]->rgbCanvas().getNavigationMouseEventHandler();
    mevh.wheelMove.notify( mCB(this,uiMultiFlatViewControl,wheelMoveCB) );

    toolbars_ += new uiToolBar(mainwin(),"Flat Viewer Tools",tb_->prefArea());
    zoommgrs_ += new FlatView::ZoomMgr;

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

    const uiWorldRect oldview( activevwr_->curView() );
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
    FlatView::ZoomMgr* zoommgr =0;

    for ( int idx=vwrs_.size()-1; idx>=0; idx-- )
    {
	activevwr_ = vwrs_[idx]; zoommgr = zoommgrs_[idx];
	if (vwrs_[idx]->rgbCanvas().getNavigationMouseEventHandler().hasEvent())
	    break;
    }

    const bool zoomin = but == zoominbut_;
    doZoom( zoomin, *activevwr_, *zoommgr );
}


bool uiMultiFlatViewControl::handleUserClick()
{
    return true;
}
