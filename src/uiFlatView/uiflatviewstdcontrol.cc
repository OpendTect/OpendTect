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
#include "uimain.h"
#include "uimenuhandler.h"
#include "uirgbarraycanvas.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "keyboardevent.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "texttranslator.h"

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(tb_,fnm,tt,mCB(this,uiFlatViewStdControl,cbnm) ); \
    tb_->addButton( but );

uiFlatViewStdControl::uiFlatViewStdControl( uiFlatViewer& vwr,
					    const Setup& setup )
    : uiFlatViewControl(vwr,setup.parent_,setup.withrubber_)
    , vwr_(vwr)
    , setup_(setup)
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
    , sethomezoombut_(0)
    , gotohomezoombut_(0)
    , setHomeZoomPushed(this)
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

    if ( setup.withzoombut_ || setup.isvertical_ )
    {
	mDefBut(rubbandzoombut_,"rubbandzoom",dragModeCB,tr("Rubberband zoom"));
	rubbandzoombut_->setToggleButton( true );
    }

    if ( setup.withzoombut_ )
    {
	mDefBut(zoominbut_,"zoomforward",zoomCB,tr("Zoom in"));
	mDefBut(zoomoutbut_,"zoombackward",zoomCB,tr("Zoom out"));
    }

    if ( setup.isvertical_ )
    {
	mDefBut(vertzoominbut_,"vertzoomin",zoomCB,tr("Vertical zoom in"));
	mDefBut(vertzoomoutbut_,"vertzoomout",zoomCB,tr("Vertical zoom out"));
    }

    if ( setup.withzoombut_ || setup.isvertical_ )
    {
	mDefBut(cancelzoombut_,"cancelzoom",cancelZoomCB,tr("Cancel zoom"));
    }

    if ( setup.withhomebutton_ )
    {
	mDefBut(sethomezoombut_,"set_homezoom",setHomeZoomCB,
		tr("Set home zoom"));
	mDefBut(gotohomezoombut_,"homezoom",gotoHomeZoomCB,
		tr("Go to home zoom"));
	gotohomezoombut_->setSensitive( !mIsUdf(setup_.x1pospercm_) &&
					!mIsUdf(setup_.x2pospercm_) );
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
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	uiGraphicsView& view = vwrs_[idx]->rgbCanvas();
	if ( setup_.withfixedaspectratio_ )
	{
	    vwrs_[idx]->updateBitmapsOnResize( false );
	    mAttachCBIfNotAttached(
		    view.reSize, uiFlatViewStdControl::aspectRatioCB );
	}

	MouseEventHandler& mevh = view.getNavigationMouseEventHandler();
	mAttachCBIfNotAttached(
		mevh.wheelMove, uiFlatViewStdControl::wheelMoveCB );

	if ( setup_.withhanddrag_ )
	{
	    mAttachCBIfNotAttached(
		    mevh.buttonPressed, uiFlatViewStdControl::handDragStarted );
	    mAttachCBIfNotAttached(
		    mevh.buttonReleased, uiFlatViewStdControl::handDragged );
	    mAttachCBIfNotAttached(
		    mevh.movement, uiFlatViewStdControl::handDragging );
	}

	mAttachCBIfNotAttached( view.gestureEventHandler().pinchnotifier,
				uiFlatViewStdControl::pinchZoomCB );
    }

    updatePosButtonStates();
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


void uiFlatViewStdControl::aspectRatioCB( CallBacker* cb )
{
    mCBCapsuleGet(uiSize,caps,cb);
    mDynamicCastGet(uiGraphicsView*,view,caps->caller);
    int vwridx = -1;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	if ( &vwrs_[idx]->rgbCanvas() == view )
	    { vwridx = idx; break; }
    if ( vwridx == -1 ) return;

    uiFlatViewer& vwr = *vwrs_[vwridx];
    if ( !view->mainwin()->poppedUp() )
	{ vwr.setView( vwr.curView() ); return; }

    const uiWorldRect bb = vwr.boundingBox();
    const uiWorld2Ui& w2ui = vwr.getWorld2Ui();
    uiWorldRect wr = w2ui.transform( vwr.getViewRect(false) );
    wr = getZoomOrPanRect( wr.centre(), wr.size(), wr, bb );
    vwr.setExtraBorders( w2ui.transform(bb) );
    vwr.setView( wr );
    if ( !zoommgr_.atStart() )
	updateZoomManager();
}


void uiFlatViewStdControl::wheelMoveCB( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !meh->hasEvent() ) return;

    const MouseEvent& ev = meh->event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    uiToolButton* but = ev.angle()<0 ?
	(ev.shiftStatus() ? vertzoominbut_ : zoominbut_) :
	(ev.shiftStatus() ? vertzoomoutbut_ : zoomoutbut_);

    zoomCB( but );
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
    Geom::Point2D<double> pos = vwr.getWorld2Ui().transform( gevent->pos() );

    const uiWorldRect wr = getZoomOrPanRect( pos, newsz, vwr.curView(),
	    				     vwr.boundingBox() );
    vwr.setView( wr );

    if ( gevent->getState() == GestureEvent::Finished )
	updateZoomManager();
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

    Geom::Point2D<double> mousepos = hasmouseevent ?
	vwr.getWorld2Ui().transform(meh.event().pos()) : vwr.curView().centre();
    Geom::Size2D<double> newsz = zoommgr_.current( vwridx );

    setNewView( mousepos, newsz );
}


void uiFlatViewStdControl::cancelZoomCB( CallBacker* )
{
    reInitZooms();
}

#define sInchToCMFac 2.54f

void uiFlatViewStdControl::setHomeZoomCB( CallBacker* )
{
    const uiFlatViewer* curvwr = vwrs_[0];
    uiRect pixrect = curvwr->getViewRect();
    const int pixwidth = pixrect.right() - pixrect.left();
    const int screendpi = uiMain::getDPI();
    const float cmwidth = ((float)pixwidth/(float)screendpi) * sInchToCMFac;
    const int pixheight = pixrect.bottom() - pixrect.left();
    const float cmheight = ((float)pixheight/(float)screendpi) * sInchToCMFac;
    Setup& su = const_cast<Setup&>(setup_);
    su.x1pospercm_ = (curvwr->posRangeInView(true).nrSteps()+1) / cmwidth;
    su.x2pospercm_ = (curvwr->posRangeInView(false).nrSteps()+1) / cmheight;
    setHomeZoomPushed.trigger();
    gotohomezoombut_->setSensitive( true );
}


void uiFlatViewStdControl::gotoHomeZoomCB( CallBacker* )
{
    setHomeZoomViews();
}


void uiFlatViewStdControl::setHomeZoomViews()
{
    if ( mIsUdf(setup_.x1pospercm_) || mIsUdf(setup_.x2pospercm_) )
	return;
    TypeSet< uiWorldPoint > viewcenterpos;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	viewcenterpos += vwrs_[idx]->curView().centre();
    reInitZooms();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	setHomeZoomView( *vwrs_[idx], viewcenterpos[idx] );
}


void uiFlatViewStdControl::setHomeZoomView( uiFlatViewer& vwr,
					    const uiWorldPoint& centerpos )
{
    const uiRect pixrect = vwr.getViewRect();
    const int screendpi = uiMain::getDPI();
    const float cmwidth =
	((float)pixrect.width()/(float)screendpi) * sInchToCMFac;
    const double x1postofit = cmwidth * setup_.x1pospercm_;
    const float cmheight =
	((float)pixrect.height()/(float)screendpi) * sInchToCMFac;
    const double x2postofit = cmheight * setup_.x2pospercm_;

    StepInterval<double> viewx1rg( vwr.posRange(true) );
    viewx1rg.stop = viewx1rg.atIndex( mNINT32(x1postofit) );

    StepInterval<double> viewx2rg( vwr.posRange(false) );
    viewx2rg.stop = viewx2rg.atIndex( mNINT32(x2postofit) );

    Geom::Size2D<double> homesz( viewx1rg.width(), viewx2rg.width() );
    setNewView( centerpos, homesz );
}


void uiFlatViewStdControl::handDragStarted( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || meh->event().rightButton() ) return;

    const int vwridx = getViewerIdx( meh, false );
    if ( vwridx<0 ) return;
    const uiFlatViewer* vwr = vwrs_[vwridx];
    mousedownpt_ = meh->event().pos();
    mousedownwr_ = vwr->curView();
    mousepressed_ = true;
}


void uiFlatViewStdControl::handDragging( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !mousepressed_ ) return;

    const int vwridx = getViewerIdx( meh, false );
    if ( vwridx<0 ) return;
    uiFlatViewer* vwr = vwrs_[vwridx];
    if ( vwr->rgbCanvas().dragMode() != uiGraphicsViewBase::ScrollHandDrag )
	return;

    const uiWorld2Ui w2ui( mousedownwr_, vwr->getViewRect().size() );
    const uiWorldPoint startwpt = w2ui.transform( mousedownpt_ );
    const uiWorldPoint curwpt = w2ui.transform( meh->event().pos() );

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

    uiGraphicsViewBase::ODDragMode mode( uiGraphicsViewBase::ScrollHandDrag );
    MouseCursor cursor( MouseCursor::OpenHand );
    if ( iszoommode || iseditmode )
    {
	mode = iszoommode ? uiGraphicsViewBase::RubberBandDrag
			  : uiGraphicsViewBase::NoDrag;
	cursor = iszoommode ? MouseCursor::Arrow : MouseCursor::Cross;
    }

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->rgbCanvas().setDragMode( mode );
	vwrs_[idx]->setCursor( cursor );
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

