/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiflatviewstdcontrol.cc,v 1.49 2012-07-12 15:04:44 cvsbruno Exp $";

#include "uiflatviewstdcontrol.h"

#include "uiflatviewcoltabed.h"
#include "flatviewzoommgr.h"
#include "uiflatviewer.h"
#include "uiflatviewthumbnail.h"
#include "uigraphicsscene.h"
#include "uitoolbutton.h"
#include "uimainwin.h"
#include "uimenuhandler.h"
#include "uirgbarraycanvas.h"
#include "uitoolbar.h"
#include "uiworld2ui.h"

#include "keyboardevent.h"
#include "mouseevent.h"
#include "pixmap.h"
#include "texttranslator.h"

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(tb_,fnm,tt,mCB(this,uiFlatViewStdControl,cbnm) ); \
    tb_->addButton( but );

uiFlatViewStdControl::uiFlatViewStdControl( uiFlatViewer& vwr,
					    const Setup& setup )
    : uiFlatViewControl(vwr,setup.parent_,true,setup.withhanddrag_)
    , vwr_(vwr)
    , ctabed_(0)
    , manip_(false)
    , mousepressed_(false)
    , viewdragged_(false)
    , menu_(*new uiMenuHandler(&vwr,-1))
    , propertiesmnuitem_("Properties...",100)
    , manipdrawbut_(0)
    , editbut_(0)
{
    uiToolBar::ToolBarArea tba( setup.withcoltabed_ ? uiToolBar::Left
	    					    : uiToolBar::Top );
    if ( setup.tba_ > 0 )
	tba = (uiToolBar::ToolBarArea)setup.tba_;
    tb_ = new uiToolBar( mainwin(), "Flat Viewer Tools", tba );
    if ( setup.withstates_ )
    {
	mDefBut(manipdrawbut_,"altpick",stateCB,"Switch view mode");
	vwr_.rgbCanvas().getKeyboardEventHandler().keyPressed.notify(
		mCB(this,uiFlatViewStdControl,keyPressCB) );
    }

    vwr_.setRubberBandingOn( !manip_ );

    if ( setup.withedit_ )
    {
	mDefBut(editbut_,"seedpickmode",editCB,"Edit mode");
	editbut_->setToggleButton( true );
    }

    mDefBut(zoominbut_,"zoomforward",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward",zoomCB,"Zoom out");
    uiToolButton* mDefBut(fliplrbut,"flip_lr",flipCB,"Flip left/right");
    tb_->addObject( vwr_.rgbCanvas().getSaveImageButton(tb_) );

    tb_->addSeparator();
    mDefBut(parsbut_,"2ddisppars",parsCB,"Set display parameters");

    if ( setup.withcoltabed_ )
    {
	uiToolBar* coltabtb = new uiToolBar( mainwin(), "Color Table" );
	ctabed_ = new uiFlatViewColTabEd( coltabtb, vwr );
	ctabed_->colTabChgd.notify( mCB(this,uiFlatViewStdControl,coltabChg) );
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

    //zoomChanged.notify( mCB(this,uiFlatViewStdControl,vwChgCB) );

    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiFlatViewStdControl,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiFlatViewStdControl,handleMenuCB));

    if ( setup.withthumbnail_ )
	new uiFlatViewThumbnail( this, vwr );
}


uiFlatViewStdControl::~uiFlatViewStdControl()
{
    menu_.unRef();

    if ( ctabed_ ) { delete ctabed_; ctabed_ = 0; }
}


void uiFlatViewStdControl::finalPrepare()
{
    updatePosButtonStates();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	MouseEventHandler& mevh =
	    vwrs_[vwrs_.size()-1]->rgbCanvas().getNavigationMouseEventHandler();
	mevh.wheelMove.notify( mCB(this,uiFlatViewStdControl,wheelMoveCB) );
	if ( withhanddrag_ )
	{
	    mevh.buttonPressed.notify(
		    mCB(this,uiFlatViewStdControl,handDragStarted));
	    mevh.buttonReleased.notify(
		    mCB(this,uiFlatViewStdControl,handDragged));
	    mevh.movement.notify( mCB(this,uiFlatViewStdControl,handDragging));
	}
    }
}


void uiFlatViewStdControl::updatePosButtonStates()
{
    zoomoutbut_->setSensitive( !zoommgr_.atStart() );
}


void uiFlatViewStdControl::dispChgCB( CallBacker* )
{
    if ( ctabed_ )
	ctabed_->setColTab( vwr_ );
}


void uiFlatViewStdControl::vwChgCB( CallBacker* )
{
    updatePosButtonStates();
}


void uiFlatViewStdControl::wheelMoveCB( CallBacker* )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( !vwrs_[idx]->rgbCanvas().
		getNavigationMouseEventHandler().hasEvent() )
	    continue;

	const MouseEvent& ev =
	    vwrs_[idx]->rgbCanvas().getNavigationMouseEventHandler().event();
	if ( mIsZero(ev.angle(),0.01) )
	    continue;

	zoomCB( ev.angle() < 0 ? zoominbut_ : zoomoutbut_ );
    }
}


void uiFlatViewStdControl::zoomCB( CallBacker* but )
{
    const bool zoomin = but == zoominbut_;
    doZoom( zoomin, *vwrs_[0], zoommgr_ );
}


void uiFlatViewStdControl::doZoom( bool zoomin, uiFlatViewer& vwr, 
					FlatView::ZoomMgr& zoommgr )
{
    uiRect viewrect = vwr.getViewRect();
    uiSize newrectsz = viewrect.size();
    if ( zoomin )
    {
	zoommgr.forward();
	newrectsz.setWidth( mNINT32(newrectsz.width()*zoommgr.fwdFac()) );
	newrectsz.setHeight( mNINT32(newrectsz.height()*zoommgr.fwdFac()));
    }
    else
    {
	if ( zoommgr.atStart() )
	    return;
	zoommgr.back();
    }

    Geom::Point2D<double> centre;
    Geom::Size2D<double> newsz;
    if (!vwr.rgbCanvas().getNavigationMouseEventHandler().hasEvent() || !zoomin)
    {
	newsz = zoommgr.current();
	centre = vwr.curView().centre();
    }
    else
    {
	uiWorld2Ui w2ui;
	vwr.getWorld2Ui( w2ui );
	const Geom::Point2D<int> viewevpos =
	    vwr.rgbCanvas().getCursorPos();
	uiRect selarea( viewevpos.x-(newrectsz.width()/2),
			viewevpos.y-(newrectsz.height()/2),
			viewevpos.x+(newrectsz.width()/2),
			viewevpos.y+(newrectsz.height()/2) );
	int hoffs = 0;
	int voffs = 0;
	if ( viewrect.left() > selarea.left() )
	    hoffs = viewrect.left() - selarea.left();
	if ( viewrect.right() < selarea.right() )
	    hoffs = viewrect.right() - selarea.right();
	if ( viewrect.top() > selarea.top() )
	    voffs = viewrect.top() - selarea.top();
	if ( viewrect.bottom() < selarea.bottom() )
	    voffs = viewrect.bottom() - selarea.bottom();

	selarea += uiPoint( hoffs, voffs );
	uiWorldRect wr = w2ui.transform( selarea );
	centre = wr.centre();
	newsz = wr.size();
    }

    if ( zoommgr.atStart() )
	centre = zoommgr.initialCenter();

    setNewView( centre, newsz );
}


void uiFlatViewStdControl::handDragStarted( CallBacker* cb )
{
    mousepressed_ = true;
    
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( meh )
    {
	mousedownpt_ = meh->event().pos();
	mousedownwr_ = vwrs_[0]->curView();
    }
}


void uiFlatViewStdControl::handDragging( CallBacker* cb )
{
    const uiGraphicsView& canvas = vwrs_[0]->rgbCanvas();
    if ( (canvas.dragMode() != uiGraphicsViewBase::ScrollHandDrag) ||
	 !mousepressed_ || !withhanddrag_ )
	return;
    
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh )
	return;
    
    const uiPoint curpt = meh->event().pos();
    const uiWorld2Ui w2ui( mousedownwr_, vwrs_[0]->getViewRect().size() );
    const uiWorldPoint startwpt = w2ui.transform( mousedownpt_ );
    const uiWorldPoint curwpt = w2ui.transform( curpt );
    
    uiWorldRect newwr( mousedownwr_ );
    newwr.translate( startwpt-curwpt);
    
    vwrs_[0]->setView( newwr );    
}


void uiFlatViewStdControl::handDragged( CallBacker* cb )
{
    handDragging( cb );
    mousepressed_ = false;
    
    //TODO: Should we set the zoom-manager ?
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


bool uiFlatViewStdControl::handleUserClick()
{
    //TODO and what about multiple viewers?
    const MouseEvent& ev = mouseEventHandler(0).event();
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


void uiFlatViewStdControl::keyPressCB( CallBacker* )
{
    const KeyboardEvent& ev = 
	vwr_.rgbCanvas().getKeyboardEventHandler().event();
    if ( ev.key_ == OD::Escape )
	stateCB( 0 );
}
