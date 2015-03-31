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
#include "uigraphicsscene.h"
#include "uimainwin.h"
#include "uimenuhandler.h"
#include "uirgbarraycanvas.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uiworld2ui.h"

#include "flatviewzoommgr.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "texttranslator.h"

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(tb_,fnm,tt,mCB(this,uiFlatViewStdControl,cbnm) ); \
    tb_->addButton( but );

uiFlatViewStdControl::uiFlatViewStdControl( uiFlatViewer& vwr,
					    const Setup& setup )
    : uiFlatViewControl(vwr,setup.parent_,setup.withrubber_,setup.withhanddrag_)
    , vwr_(vwr)
    , ctabed_(0)
    , mousepressed_(false)
    , menu_(*new uiMenuHandler(0,-1))
    , propertiesmnuitem_(tr("Properties..."),100)
    , editbut_(0)
    , rubbandzoombut_(0)
    , zoominbut_(0)
    , zoomoutbut_(0)
    , vertzoominbut_(0)
    , vertzoomoutbut_(0)
    , cancelzoombut_(0)
{
    uiToolBar::ToolBarArea tba( setup.withcoltabed_ ? uiToolBar::Left
						    : uiToolBar::Top );
    if ( setup.tba_ > 0 )
	tba = (uiToolBar::ToolBarArea)setup.tba_;
    tb_ = new uiToolBar( mainwin(), tr("Flat Viewer Tools"), tba );
    vwr_.rgbCanvas().setDragMode( uiGraphicsView::ScrollHandDrag );

    if ( setup.withedit_ )
    {
	mDefBut(editbut_,"seedpickmode",dragModeCB,tr("Edit mode"));
	editbut_->setToggleButton( true );
    }

    if ( setup.withzoombut_ || setup.withvertzoombut_ )
    {
	mDefBut(rubbandzoombut_,"rubbandzoom",dragModeCB,tr("Rubberband zoom"));
	rubbandzoombut_->setToggleButton( true );
    }

    if ( setup.withzoombut_ )
    {
	mDefBut(zoominbut_,"zoomforward",zoomCB,tr("Zoom in"));
	mDefBut(zoomoutbut_,"zoombackward",zoomCB,tr("Zoom out"));
    }

    if ( setup.withvertzoombut_ )
    {
	mDefBut(vertzoominbut_,"vertzoomin",zoomCB,tr("Vertical zoom in"));
	mDefBut(vertzoomoutbut_,"vertzoomout",zoomCB,tr("Vertical zoom out"));
    }

    if ( setup.withzoombut_ || setup.withvertzoombut_ )
    {
	mDefBut(cancelzoombut_,"cancelzoom",cancelZoomCB,tr("Cancel zoom"));
    }

    if ( setup.withflip_ )
    {
	uiToolButton* mDefBut(fliplrbut,"flip_lr",flipCB,tr("Flip left/right"));
    }

    if ( setup.withsnapshot_ )
    {
	vwr_.rgbCanvas().enableImageSave();
	tb_->addObject( vwr_.rgbCanvas().getSaveImageButton(tb_) );
	tb_->addObject( vwr_.rgbCanvas().getPrintImageButton(tb_) );
    }

    tb_->addSeparator();
    mDefBut(parsbut_,"2ddisppars",parsCB,tr("Set display parameters"));

    if ( setup.withcoltabed_ )
    {
	uiColorTableToolBar* coltabtb = new uiColorTableToolBar( mainwin() );
	ctabed_ = new uiFlatViewColTabEd( *coltabtb );
	coltabtb->display( vwr.rgbCanvas().prefHNrPics()>=400 );
	if ( setup.managescoltab_ )
	{
	    mAttachCB( ctabed_->colTabChgd, uiFlatViewStdControl::coltabChg );
	    mAttachCB( vwr.dispParsChanged, uiFlatViewStdControl::dispChgCB );
	}
    }

    if ( !setup.helpkey_.isEmpty() )
    {
	uiToolButton* mDefBut(helpbut,"contexthelp",helpCB,uiStrings::sHelp());
	helpkey_ = setup.helpkey_;
    }

    menu_.ref();
    mAttachCB( menu_.createnotifier, uiFlatViewStdControl::createMenuCB );
    mAttachCB( menu_.handlenotifier, uiFlatViewStdControl::handleMenuCB );
    mAttachCB( zoomChanged, uiFlatViewStdControl::zoomChgCB );
    mAttachCB( rubberBandUsed, uiFlatViewStdControl::rubBandUsedCB );
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
	MouseEventHandler& mevh =
	    vwrs_[idx]->rgbCanvas().getNavigationMouseEventHandler();
	mAttachCBIfNotAttached(
		mevh.wheelMove, uiFlatViewStdControl::wheelMoveCB );
	GestureEventHandler& pinchhandler =
	    vwrs_[idx]->rgbCanvas().gestureEventHandler();
	mAttachCBIfNotAttached( pinchhandler.pinchnotifier,
				uiFlatViewStdControl::pinchZoomCB );

	if ( withhanddrag_ )
	{
	    mAttachCBIfNotAttached(
		    mevh.buttonPressed, uiFlatViewStdControl::handDragStarted );
	    mAttachCBIfNotAttached(
		    mevh.buttonReleased, uiFlatViewStdControl::handDragged );
	    mAttachCBIfNotAttached(
		    mevh.movement, uiFlatViewStdControl::handDragging );
	}
    }
}


void uiFlatViewStdControl::clearToolBar()
{
    delete mainwin()->removeToolBar( tb_ );
    tb_ = 0;
    zoominbut_ = zoomoutbut_ = rubbandzoombut_ = parsbut_ = editbut_ = 0;
    vertzoominbut_ = vertzoomoutbut_ = cancelzoombut_ = 0;
}


void uiFlatViewStdControl::updatePosButtonStates()
{
    const bool yn = !zoommgr_.atStart();
    if ( zoomoutbut_ ) zoomoutbut_->setSensitive( yn );
    if ( vertzoomoutbut_ ) vertzoomoutbut_->setSensitive( yn );
    if ( cancelzoombut_ ) cancelzoombut_->setSensitive( yn );
}


void uiFlatViewStdControl::dispChgCB( CallBacker* )
{
    if ( ctabed_ ) ctabed_->setColTab( vwr_.appearance().ddpars_.vd_ );
}


void uiFlatViewStdControl::zoomChgCB( CallBacker* )
{
    updatePosButtonStates();
}


void uiFlatViewStdControl::rubBandUsedCB( CallBacker* )
{
    if ( rubbandzoombut_ && rubbandzoombut_->isOn() )
    {
	rubbandzoombut_->setOn( false );
	dragModeCB( rubbandzoombut_ );
    }
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


void uiFlatViewStdControl::pinchZoomCB( CallBacker* cb )
{
    mDynamicCastGet(const GestureEventHandler*,evh,cb);
    if ( !evh || evh->isHandled() || vwrs_.isEmpty() )
	return;

    const GestureEvent* gevent = evh->getPinchEventInfo();
    if ( !gevent )
	return;
   
    uiFlatViewer& vwr = *vwrs_[0];
    const Geom::Size2D<double> cursz = vwr.curView().size();
    const float scalefac = gevent->scale();
    Geom::Size2D<double> newsz( cursz.width() * (1/scalefac), 
				cursz.height() * (1/scalefac) );

    uiWorld2Ui w2ui;
    vwr.getWorld2Ui( w2ui );
    Geom::Point2D<double> pos = w2ui.transform( gevent->pos() );

    const uiWorldRect wr = getZoomOrPanRect( pos, newsz, vwr.curView(),
	    				     vwr.boundingBox() );
    vwr.setView( wr );

    if ( gevent->getState() == GestureEvent::Finished )
    	zoommgr_.add( newsz );
 
    zoomChanged.trigger();
}


void uiFlatViewStdControl::zoomCB( CallBacker* but )
{
    if ( !but ) return;
    const bool zoomin = but==zoominbut_ || but==vertzoominbut_;
    const bool onlyvertzoom = but==vertzoominbut_ || but==vertzoomoutbut_;
    doZoom( zoomin, onlyvertzoom, *vwrs_[0] );
}


void uiFlatViewStdControl::doZoom( bool zoomin, bool onlyvertzoom,
				   uiFlatViewer& vwr )
{
    const int vwridx = vwrs_.indexOf( &vwr );
    if ( vwridx < 0 ) return;

    const MouseEventHandler& meh =
	vwr.rgbCanvas().getNavigationMouseEventHandler();
    const bool hasmouseevent = meh.hasEvent();

    if ( zoomin )
	zoommgr_.forward( vwridx, onlyvertzoom, true );
    else
    {
	if ( zoommgr_.atStart(vwridx) )
	    return;
	zoommgr_.back( vwridx, onlyvertzoom, hasmouseevent );
    }

    uiWorld2Ui w2ui;
    vwr.getWorld2Ui( w2ui );

    Geom::Point2D<double> mousepos = hasmouseevent ?
	w2ui.transform( meh.event().pos() ) : vwr.curView().centre();
    Geom::Size2D<double> newsz = zoommgr_.current( vwridx );

    setNewView( mousepos, newsz );
}


void uiFlatViewStdControl::cancelZoomCB( CallBacker* )
{
    reInitZooms();
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
    if ( !meh || !mousepressed_ || !withhanddrag_ ) return;

    const int vwridx = getViewerIdx( meh, false );
    if ( vwridx<0 ) return;
    uiFlatViewer* vwr = vwrs_[vwridx];
    if ( vwr->rgbCanvas().dragMode() != uiGraphicsViewBase::ScrollHandDrag )
	return;

    const uiPoint curpt = meh->event().pos();
    const uiWorld2Ui w2ui( mousedownwr_, vwr->getViewRect().size() );
    const uiWorldPoint startwpt = w2ui.transform( mousedownpt_ );
    const uiWorldPoint curwpt = w2ui.transform( curpt );

    uiWorldRect newwr( mousedownwr_ );
    newwr.translate( startwpt-curwpt );

    newwr = getZoomOrPanRect( newwr.centre(), newwr.size(), newwr,
	    		      vwr->boundingBox() );
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
    dragModeCB( editbut_ );
}


void uiFlatViewStdControl::dragModeCB( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb);
    const bool iseditbut = but==editbut_;
    const bool iseditmode = editbut_ && editbut_->isOn();
    const bool iszoommode = rubbandzoombut_ && rubbandzoombut_->isOn();

    uiGraphicsViewBase::ODDragMode mode;
    if ( iseditmode && !iszoommode )
	mode = uiGraphicsViewBase::NoDrag;
    else
	mode = iszoommode ? uiGraphicsViewBase::RubberBandDrag
			  : uiGraphicsViewBase::ScrollHandDrag;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->rgbCanvas().setDragMode( mode );
	vwrs_[idx]->rgbCanvas().scene().setMouseEventActive( true );
	vwrs_[idx]->appearance().annot_.editable_ = iseditmode &&
	   				!vwrs_[idx]->hasZAxisTransform();
	// TODO: Change while enabling tracking in Z-transformed 2D Viewers.
    }

    if ( iseditbut && iseditmode && rubbandzoombut_ )
	rubbandzoombut_->setOn( false );
}


void uiFlatViewStdControl::helpCB( CallBacker* )
{
    HelpProvider::provideHelp( helpkey_ );
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
    vwr_.appearance().ddpars_.vd_ = ctabed_->getDisplayPars();
    vwr_.handleChange( FlatView::Viewer::DisplayPars );
}


void uiFlatViewStdControl::keyPressCB( CallBacker* cb )
{
    mDynamicCastGet( const KeyboardEventHandler*, keh, cb );
    if ( !keh || !keh->hasEvent() ) return;

    if ( keh->event().key_==OD::Escape && rubbandzoombut_ )
    {
	rubbandzoombut_->setOn( !rubbandzoombut_->isOn() );
	dragModeCB( rubbandzoombut_ );
    }
}


NotifierAccess* uiFlatViewStdControl::editPushed()
{ return editbut_ ? &editbut_->activated : 0; }

