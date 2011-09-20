/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjectitemviewwin.cc,v 1.15 2011-09-20 10:30:15 cvsbruno Exp $";

#include "uiobjectitemviewwin.h"

#include "scaler.h"
#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uibutton.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uirgbarraycanvas.h"
#include "uiprogressbar.h"
#include "uislider.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"


#define mSldUnits 250
#define mZoomFac 2

uiObjectItemViewWin::uiObjectItemViewWin(uiParent* p, const Setup& su)
    : uiMainWin(p,su.wintitle_)
    , startwidth_(su.startwidth_)
    , startheight_(su.startheight_)
    , infoheight_(su.infoheight_) 
    , hslval_(1)
    , vslval_(1)
{
    setPrefWidth( startwidth_ + su.infoheight_ );
    setPrefHeight( startheight_+ su.infoheight_ );

    mainviewer_ = new uiObjectItemView( this );
    mainviewer_->setSceneLayoutPos( su.layoutpos_ );
    mainviewer_->setPrefWidth( startwidth_ );
    mainviewer_->setPrefHeight( startheight_ );
    mainviewer_->enableScrollBars( true );
    mainviewer_->disableScrollZoom();
    mainviewer_->scrollBarUsed.notify(mCB(this,uiObjectItemViewWin,scrollBarCB));
    infobar_ = new uiObjectItemViewInfoBar( this );
    infobar_->setPrefWidth( startwidth_ );
    infobar_->setPrefHeight( su.infoheight_ );
    infobar_->setSceneLayoutPos( su.layoutpos_ );
    infobar_->setStretch( 2, 0 );
    infobar_->disableScrollZoom();

    mainviewer_->attach( ensureBelow, infobar_, 0 );

    makeSliders();
}


void uiObjectItemViewWin::addObject( uiObject* obj, uiObject* infoobj )
{
    uiObjectItem* itm = new uiObjectItem( obj );
    itm->setObjectSize( obj->width(), obj->height() );
    uiObjectItem* infoitm = infoobj ? new uiObjectItem( infoobj ) : 0;
    if ( infoitm )
       infoitm->setObjectSize(obj->width(),infoheight_);
    addItem( itm, infoitm );
}


void uiObjectItemViewWin::addGroup( uiGroup* obj, uiGroup* infoobj )
{
    uiObjectItem* itm = new uiObjectItem( obj );
    itm->setObjectSize(obj->mainObject()->width(),obj->mainObject()->height());
    uiObjectItem* infoitm = infoobj ? new uiObjectItem( infoobj ) : 0;
    if ( infoitm )
       infoitm->setObjectSize(obj->mainObject()->width(),infoheight_);
    addItem( itm, infoitm );
}


void uiObjectItemViewWin::addItem( uiObjectItem* itm, uiObjectItem* infoitm )
{
    mainviewer_->addItem( itm );
    infobar_->addItem( infoitm, itm );
}


void uiObjectItemViewWin::insertItem( int idx, 
				uiObjectItem* itm, uiObjectItem* infoitm )
{
    mainviewer_->insertItem( itm, idx );
    infobar_->insertItem( infoitm, itm, idx );
}


void uiObjectItemViewWin::makeSliders()
{
    uiLabel* dummylbl = new uiLabel( this, "" );
    dummylbl->attach( rightOf, mainviewer_ );
    dummylbl->setStretch( 0, 2 );
    dummylbl->attach( ensureRightOf, infobar_ );

    uiSliderExtra::Setup su;
    su.sldrsize_ = 250;
    su.withedit_ = false;
    StepInterval<float> sintv( 1, mSldUnits, 1 );
    su.isvertical_ = true;
    versliderfld_ = new uiSliderExtra( this, su, "Vertical Scale" );
    versliderfld_->sldr()->setInterval( sintv );
    versliderfld_->sldr()->setInverted( true );
    versliderfld_->sldr()->sliderReleased.notify(
	    			mCB(this,uiObjectItemViewWin,reSizeSld) );
    versliderfld_->attach( centeredBelow, dummylbl );
    versliderfld_->setStretch( 0, 0 );

    fittoscreenbut_ = new uiToolButton( this, "exttofullsurv.png",
		    "Fit to screen", mCB(this,uiObjectItemViewWin,fitToScreen));
    fittoscreenbut_->attach( centeredBelow, versliderfld_ );

    su.isvertical_ = false;
    horsliderfld_ = new uiSliderExtra( this, su, "Horizontal Scale" );
    horsliderfld_->sldr()->setInterval( sintv );
    horsliderfld_->sldr()->sliderReleased.notify(
				    mCB(this,uiObjectItemViewWin,reSizeSld));
    horsliderfld_->setStretch( 0, 0 );
    horsliderfld_->attach( leftOf, fittoscreenbut_ );
    horsliderfld_->attach( ensureBelow, mainviewer_ );

    zoomratiofld_ = new uiCheckBox( this, "Keep zoom ratio" );
    zoomratiofld_->attach( leftOf, horsliderfld_ );
    zoomratiofld_->setChecked( true );
}


void uiObjectItemViewWin::reSizeSld( CallBacker* cb )
{
    uiSlider* hsldr = horsliderfld_->sldr();
    uiSlider* vsldr = versliderfld_->sldr();
    hslval_ = hsldr->getValue();
    vslval_ = vsldr->getValue();

    mDynamicCastGet(uiSlider*,sld,cb)
    if ( sld && zoomratiofld_->isChecked() ) 
    {
	bool ishor = sld == hsldr;
	uiSlider* revsld = ishor ? vsldr : hsldr;
	if ( ishor )
	    vslval_ = hslval_;
	else
	    hslval_ = vslval_;

	NotifyStopper ns( revsld->sliderReleased );
	revsld->setValue( hslval_ );
    }
    reSizeItems();
}


void uiObjectItemViewWin::reSizeItems()
{
    const float nritems = mainviewer_->nrItems();
    if ( !nritems ) return;

    LinScaler scaler(1,1,mSldUnits,nritems*mZoomFac);
    hslval_ = scaler.scale( hslval_ );
    scaler.set(1,1,mSldUnits,mZoomFac);
    vslval_ = scaler.scale( vslval_ );

    const int w = mNINT( hslval_*startwidth_/(float)nritems );
    const int h = mNINT( vslval_*startheight_ );
    for ( int idx=0; idx<nritems; idx++ )
	mainviewer_->reSizeItem( idx, uiSize( w, h ) );

    mainviewer_->resetViewArea(0);
    infobar_->reSizeItems(); 
}


void uiObjectItemViewWin::removeAllItems()
{
    mainviewer_->removeAllItems();
    infobar_->removeAllItems();
}


void uiObjectItemViewWin::removeGroup( uiGroup* grp )
{
    for ( int idx=0; idx<mainviewer_->nrItems(); idx++ )
    {
	uiObjectItem* itm = mainviewer_->getItem( idx );
	if ( itm->getGroup() == grp )
	{
	    infobar_->removeItem( itm );
	    mainviewer_->removeItem( itm );
	    break;
	}
    }
}


void uiObjectItemViewWin::removeObject( uiObject* obj )
{
    for ( int idx=0; idx<mainviewer_->nrItems(); idx++ )
    {
	uiObjectItem* itm = mainviewer_->getItem( idx );
	if ( itm->getObject() == obj )
	{
	    infobar_->removeItem( itm );
	    mainviewer_->removeItem( itm );
	    break;
	}
    }
}


void uiObjectItemViewWin::insertGroup( int idx, uiGroup* grp, uiGroup* infogrp )
{
    uiObjectItem* itm = new uiObjectItem( grp );
    uiObjectItem* infoitm = infogrp ? new uiObjectItem( infogrp ) : 0;
    insertItem( idx, itm, infoitm );
}


void uiObjectItemViewWin::insertObject(int idx, uiObject* obj,uiObject* infoobj)
{
    uiObjectItem* itm = new uiObjectItem( obj );
    uiObjectItem* infoitm = infoobj ? new uiObjectItem( infoobj ) : 0;
    insertItem( idx, itm, infoitm );
}


void uiObjectItemViewWin::scrollBarCB( CallBacker* )
{
    const uiRect& mainrect = mainviewer_->getViewArea();
    const uiRect& inforect = infobar_->getViewArea();
    const int x = mainrect.left();
    const int y = mainrect.right();
    const int top = inforect.top();
    const int bot = inforect.bottom();
    const int w = abs( y - x ); 
    const int h = abs( y - x ); 
    infobar_->setViewArea( x, y, w, h );
    infobar_->updateItemsPos();
    mainviewer_->resetViewArea(0);
}


void uiObjectItemViewWin::fitToScreen( CallBacker* )
{
    mDynamicCastGet(uiGraphicsObjectScene*,sc,&mainviewer_->scene())
    const uiSize screensz( mainviewer_->parent()->mainObject()->width(),
    mainviewer_->parent()->mainObject()->height() );
    if ( screensz.width()<=0 || screensz.height()<=0 ) return;
    const uiSize layoutsz( sc->layoutSize().width() + (int)sc->layoutPos().x,
			   sc->layoutSize().height() + (int)sc->layoutPos().y );
    float xratio = ( screensz.width()/(float)layoutsz.width() );
    float yratio = ( screensz.height()/(float)layoutsz.height() );
    uiSlider* hsldr = horsliderfld_->sldr();
    uiSlider* vsldr = versliderfld_->sldr();
    float hslval = hsldr->getValue();
    float vslval = vsldr->getValue();
    int hscaledfac = (int)(hslval*xratio);
    int vscaledfac = (int)(vslval*yratio);
    hsldr->setValue( hscaledfac );
    vsldr->setValue( vscaledfac );
}


void uiObjectItemViewWin::fillPar( IOPar& iop ) const
{
    if ( !versliderfld_ || !horsliderfld_ ) return;
    iop.set( sKeyVZoomVal(), versliderfld_->sldr()->getValue() );
    iop.set( sKeyHZoomVal(), horsliderfld_->sldr()->getValue() );
}


void uiObjectItemViewWin::usePar( const IOPar& iop )
{
    if ( !versliderfld_ || !horsliderfld_ ) return;
    float hval, vval;
    iop.get( sKeyHZoomVal(), hval );
    iop.get( sKeyVZoomVal(), vval );
    horsliderfld_->sldr()->setValue( hval );
    versliderfld_->sldr()->setValue( vval );
    reSizeSld(0);
}




uiObjectItemViewInfoBar::uiObjectItemViewInfoBar( uiParent* p )
        : uiObjectItemView(p)
{
    enableScrollBars(false);
}


void uiObjectItemViewInfoBar::addItem( uiObjectItem* infoitm,
					uiObjectItem* cpleditm )
{
    addItem( infoitm );
    coupleditems_ += cpleditm;
    updateItemsPos();
}


void uiObjectItemViewInfoBar::removeItem( uiObjectItem* itm )
{
    const int idx = objectitems_.indexOf( itm );
    if ( idx >= 0 ) coupleditems_.remove( idx );
    uiObjectItemView::removeItem( itm );
    updateItemsPos();
}


void uiObjectItemViewInfoBar::removeItemByCouple( uiObjectItem* coupleditem )
{
    const int idx = coupleditems_.indexOf( coupleditem );
    if ( objectitems_.validIdx( idx ) )
	removeItem( objectitems_[idx] );
}


void uiObjectItemViewInfoBar::insertItem( uiObjectItem* itm, 
					uiObjectItem* cpleditm, int pos )
{
    insertItem( itm, pos );
    coupleditems_.insertAt( cpleditm, pos );
    updateItemsPos();
}


void uiObjectItemViewInfoBar::reSizeItems()
{
    for( int idx=0; idx<objectitems_.size(); idx++ )
    {
	uiObjectItem* itm = objectitems_[idx];
	uiObjectItem* cpleditm = coupleditems_[idx];
	const int w = cpleditm->objectSize().width();
	const int h = height();
	itm->setObjectSize( w, h );
    }
    updateItemsPos();
}


void uiObjectItemViewInfoBar::updateItemsPos()
{
    for( int idx=0; idx<objectitems_.size(); idx++ )
    {
	uiObjectItem* itm = objectitems_[idx];
	uiObjectItem* cpleditm = coupleditems_[idx];
	itm->setPos( cpleditm->getPos().x, itm->getPos().y );
    }
    resetViewArea(0);
}




#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(toolbar_,fnm,tt,mCB(this,uiObjectItemViewControl,cbnm) ); \
    toolbar_->addButton( but );

uiObjectItemViewControl::uiObjectItemViewControl( uiObjectItemView& mw )
    : uiGroup(mw.parent(),"ObjectItemView control")
    , mainviewer_(mw)
    , manipdrawbut_(0)		     
    , manip_(false)		     
    , toolbar_(0)
{
    uiToolBar::ToolBarArea tba( uiToolBar::Top );
    toolbar_ = new uiToolBar( mw.parent(), "ObjectItemView tools", tba );
    setToolButtons();

    mainviewer_.disableScrollZoom();
    mainviewer_.setScrollBarPolicy( true, uiGraphicsView::ScrollBarAsNeeded );
    mainviewer_.setScrollBarPolicy( false, uiGraphicsView::ScrollBarAsNeeded );
    mainviewer_.scene().setMouseEventActive( true );
}


void uiObjectItemViewControl::setToolButtons()
{
    mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode (Esc)");
}


void uiObjectItemViewControl::stateCB( CallBacker* )
{
    changeStatus();
}

void uiObjectItemViewControl::changeStatus() 
{
    manip_ = !manip_;

    uiGraphicsViewBase::ODDragMode mode = !manip_ ? 
	uiGraphicsViewBase::NoDrag : uiGraphicsViewBase::ScrollHandDrag;

    if ( manipdrawbut_ ) 
	manipdrawbut_->setPixmap( manip_ ? "altview.png" : "altpick.png" );

    mainviewer_.setDragMode( mode );
    mainviewer_.scene().setMouseEventActive( true );
    if ( mode == uiGraphicsViewBase::ScrollHandDrag )
	cursor_.shape_ = MouseCursor::OpenHand;
    else
	cursor_.shape_ = MouseCursor::Arrow;

    mainviewer_.setCursor( cursor_ );
}



uiObjectItemViewAxisPainter::uiObjectItemViewAxisPainter( uiObjectItemView& vw )
    : zax_(0)
    , viewer_(vw)  
    , scene_(0)
{
    mDynamicCastGet(uiGraphicsObjectScene*,sc,&vw.scene())
    if ( !sc ) return;

    scene_ = sc;

    uiAxisHandler::Setup asu( uiRect::Bottom );
    asu.side( uiRect::Left );
    zax_ = new uiAxisHandler( scene_, asu );

    vw.viewareareset.notify( mCB(this,uiObjectItemViewAxisPainter,plotAxis) );
}


void uiObjectItemViewAxisPainter::setZRange( Interval<float> zrg )
{
    if ( zax_  )
    {
	zrg.sort( false );
	zax_->setBounds( zrg );
    }
    plotAxis(0);
}


void uiObjectItemViewAxisPainter::setAxisRelations()
{
    if ( !zax_ || !scene_ ) return;
    const int widthshift = scene_->layoutPos().x -zax_->pixToEdge(false);
    const int heightshift = scene_->layoutPos().y;
    uiBorder b( widthshift, heightshift, widthshift, heightshift );
    b += border_;
    zax_->setBorder( b );
    const uiRect& scenerect = viewer_.getSceneRect();
    zax_->setNewDevSize( scenerect.height(), scenerect.width() );
}


void uiObjectItemViewAxisPainter::plotAxis( CallBacker* )
{
    setAxisRelations();
    if ( zax_ )
	zax_->plotAxis();
}
