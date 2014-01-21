/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiflatviewstdcontrol.h"

#include "uicolortable.h"
#include "uiflatviewcoltabed.h"
#include "uiflatviewer.h"
#include "uiflatviewthumbnail.h"
#include "uigraphicsscene.h"
#include "uimainwin.h"
#include "uimenuhandler.h"
#include "uirgbarraycanvas.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uiworld2ui.h"

#include "flatviewzoommgr.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "pixmap.h"
#include "texttranslator.h"

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(tb_,fnm,tt,mCB(this,uiFlatViewStdControl,cbnm) ); \
    tb_->addButton( but );

uiFlatViewStdControl::uiFlatViewStdControl( uiFlatViewer& vwr,
					    const Setup& setup )
    : uiFlatViewControl(vwr,setup.parent_,setup.withrubber_,setup.withhanddrag_)
    , vwr_(vwr)
    , ctabed_(0)
    , manip_(false)
    , mousepressed_(false)
    , menu_(*new uiMenuHandler(0,-1))
    , propertiesmnuitem_("Properties...",100)
    , manipdrawbut_(0)
    , editbut_(0)
    , zoominbut_(0)
    , zoomoutbut_(0)
    , thumbnail_(0)
{
    uiToolBar::ToolBarArea tba( setup.withcoltabed_ ? uiToolBar::Left
	    					    : uiToolBar::Top );
    if ( setup.tba_ > 0 )
	tba = (uiToolBar::ToolBarArea)setup.tba_;
    tb_ = new uiToolBar( mainwin(), "Flat Viewer Tools", tba );
    if ( setup.withstates_ )
	mDefBut(manipdrawbut_,"altpick",stateCB,"Switch view mode");

    vwr_.setRubberBandingOn( !manip_ );

    if ( setup.withedit_ )
    {
	mDefBut(editbut_,"seedpickmode",editCB,"Edit mode");
	editbut_->setToggleButton( true );
    }

    mDefBut(zoominbut_,"zoomforward",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward",zoomCB,"Zoom out");
    if ( setup.withflip_ )
    {
	uiToolButton* mDefBut(fliplrbut,"flip_lr",flipCB,"Flip left/right");
    }

    if ( setup.withsnapshot_ )
    {
	vwr_.rgbCanvas().enableImageSave();
	tb_->addObject( vwr_.rgbCanvas().getSaveImageButton(tb_) );
    }

    tb_->addSeparator();
    mDefBut(parsbut_,"2ddisppars",parsCB,"Set display parameters");

    if ( setup.withcoltabed_ )
    {
	uiColorTableToolBar* coltabtb = new uiColorTableToolBar( mainwin() );
	ctabed_ = new uiFlatViewColTabEd( *coltabtb, vwr );
	mAttachCB( ctabed_->colTabChgd, uiFlatViewStdControl::coltabChg );
	coltabtb->display( vwr.rgbCanvas().prefHNrPics()>=400 );
    }

    if ( !setup.helpid_.isEmpty() )
    {
	uiToolButton* mDefBut(helpbut,"contexthelp",helpCB,"Help");
	helpid_ = setup.helpid_;
    }

    if ( TrMgr().tr() && TrMgr().tr()->enabled() )
    {
	uiToolButton* mDefBut(trlbut,"google",translateCB,"Translate");
    }

    mAttachCB( zoomChanged, uiFlatViewStdControl::zoomChgCB );
    mAttachCB( vwr.dispParsChanged, uiFlatViewStdControl::dispChgCB );

    menu_.ref();
    mAttachCB( menu_.createnotifier, uiFlatViewStdControl::createMenuCB );
    mAttachCB( menu_.handlenotifier, uiFlatViewStdControl::handleMenuCB );

    if ( setup.withthumbnail_ )
	thumbnail_ = new uiFlatViewThumbnail( this, vwr );
}


uiFlatViewStdControl::~uiFlatViewStdControl()
{
    detachAllNotifiers();
    menu_.unRef();
    delete ctabed_; ctabed_ = 0;
}


void uiFlatViewStdControl::finalPrepare()
{
    updatePosButtonStates();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	MouseEventHandler& mevh = mouseEventHandler( idx, false );
	mAttachCB( mevh.wheelMove, uiFlatViewStdControl::wheelMoveCB );
	if ( withhanddrag_ )
	{
	    mAttachCB(mevh.buttonPressed,uiFlatViewStdControl::handDragStarted);
	    mAttachCB( mevh.buttonReleased, uiFlatViewStdControl::handDragged );
	    mAttachCB( mevh.movement, uiFlatViewStdControl::handDragging );
	}
    }
}


void uiFlatViewStdControl::clearToolBar()
{
    delete mainwin()->removeToolBar( tb_ );
    tb_ = 0;
    zoominbut_ = zoomoutbut_ = manipdrawbut_ = parsbut_ = editbut_ = 0;
}


void uiFlatViewStdControl::updatePosButtonStates()
{
    if ( zoomoutbut_ ) zoomoutbut_->setSensitive( !zoommgr_.atStart() );
}


void uiFlatViewStdControl::dispChgCB( CallBacker* )
{
    if ( ctabed_ ) ctabed_->setColTab( vwr_ );
}


void uiFlatViewStdControl::zoomChgCB( CallBacker* )
{
    updatePosButtonStates();
}


void uiFlatViewStdControl::vwChgCB( CallBacker* )
{
    if ( thumbnail_ ) thumbnail_->draw();
}


void uiFlatViewStdControl::wheelMoveCB( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !meh->hasEvent() ) return;

    const MouseEvent& ev = meh->event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    zoomCB( ev.angle()<0 ? zoominbut_ : zoomoutbut_ );
}


void uiFlatViewStdControl::zoomCB( CallBacker* but )
{
    if ( !but ) return;
    const bool zoomin = but == zoominbut_;
    doZoom( zoomin, *vwrs_[0] );
}


void uiFlatViewStdControl::doZoom( bool zoomin, uiFlatViewer& vwr )
{
    const int vwridx = vwrs_.indexOf( &vwr );
    if ( vwridx < 0 ) return;

    if ( zoomin )
	zoommgr_.forward( vwridx );
    else
    {
	if ( zoommgr_.atStart(vwridx) )
	    return;
	zoommgr_.back( vwridx );
    }

    uiWorld2Ui w2ui;
    vwr.getWorld2Ui( w2ui );

    const MouseEventHandler& meh =
	vwr.rgbCanvas().getNavigationMouseEventHandler();
    Geom::Point2D<double> mousepos = meh.hasEvent() ?
	w2ui.transform( meh.event().pos() ) : vwr.curView().centre();
    Geom::Size2D<double> newsz = zoommgr_.current( vwridx );

    setNewView( mousepos, newsz );
}


void uiFlatViewStdControl::handDragStarted( CallBacker* cb )
{
    mousepressed_ = true;
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh ) return;

    const int vwridx = getViewerIdx( meh, false );
    if ( vwridx<0 ) return;
    const uiFlatViewer* vwr = vwrs_[vwridx];
    mousedownpt_ = meh->event().pos();
    mousedownwr_ = vwr->curView();
}


void uiFlatViewStdControl::handDragging( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh ) return;

    const int vwridx = getViewerIdx( meh, false );
    if ( vwridx<0 ) return;
    uiFlatViewer* vwr = vwrs_[vwridx];
    const uiGraphicsView& canvas = vwr->rgbCanvas();
    if ( (canvas.dragMode() != uiGraphicsViewBase::ScrollHandDrag) ||
	 !mousepressed_ || !withhanddrag_ )
	return;
    
    const uiPoint curpt = meh->event().pos();
    const uiWorld2Ui w2ui( mousedownwr_, vwr->getViewRect().size() );
    const uiWorldPoint startwpt = w2ui.transform( mousedownpt_ );
    const uiWorldPoint curwpt = w2ui.transform( curpt );
    
    uiWorldRect newwr( mousedownwr_ );
    newwr.translate( startwpt-curwpt );

    uiWorldRect bb = vwr->boundingBox();
    Geom::Point2D<double> oldcentre = vwr->curView().centre();
    Geom::Size2D<double> size = newwr.size();
    newwr = getNewWorldRect( oldcentre, size, newwr, bb );
    
    vwr->setView( newwr );    
}


void uiFlatViewStdControl::handDragged( CallBacker* cb )
{
    handDragging( cb );
    mousepressed_ = false;
}


void uiFlatViewStdControl::flipCB( CallBacker* )
{
    flip( true );
}


void uiFlatViewStdControl::parsCB( CallBacker* )
{
    doPropertiesDialog();
}


void uiFlatViewStdControl::setEditMode( bool yn )
{
    if ( editbut_ )
	editbut_->setOn( yn );
    editCB( 0 );
}


void uiFlatViewStdControl::stateCB( CallBacker* )
{
    if ( !manipdrawbut_ ) return;
    manip_ = !manip_;

    manipdrawbut_->setPixmap( manip_ ? "altview" : "altpick" );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->rgbCanvas().setDragMode( 
		!manip_ ? uiGraphicsViewBase::RubberBandDrag
		        : uiGraphicsViewBase::ScrollHandDrag);
	vwrs_[idx]->rgbCanvas().scene().setMouseEventActive( true );
	vwrs_[idx]->appearance().annot_.editable_ = false;
    }
    if ( editbut_ )
	editbut_->setOn( false );
}


void uiFlatViewStdControl::editCB( CallBacker* )
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


void uiFlatViewStdControl::helpCB( CallBacker* )
{
    uiMainWin::provideHelp( helpid_ );
}


void uiFlatViewStdControl::translateCB( CallBacker* )
{
    mainwin()->translate();
}


bool uiFlatViewStdControl::handleUserClick( int vwridx )
{
    const MouseEvent& ev = mouseEventHandler(vwridx,true).event();
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

    const bool ishandled = mnuid==propertiesmnuitem_.id ;
    if ( ishandled )
	doPropertiesDialog();

    menu->setIsHandled( ishandled );
}


void uiFlatViewStdControl::coltabChg( CallBacker* )
{
    vwr_.handleChange( FlatView::Viewer::DisplayPars );
}


void uiFlatViewStdControl::keyPressCB( CallBacker* cb )
{
    mDynamicCastGet( const KeyboardEventHandler*, keh, cb );
    if ( !keh || !keh->hasEvent() ) return;

    if ( keh->event().key_ == OD::Escape )
	stateCB( 0 );
}


NotifierAccess* uiFlatViewStdControl::editPushed()
{ return editbut_ ? &editbut_->activated : 0; }

