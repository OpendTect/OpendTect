/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjectitemviewwin.cc,v 1.3 2011-05-06 13:45:45 cvsbruno Exp $";

#include "uiobjectitemviewwin.h"

#include "uigraphicsitemimpl.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"
#include "uislider.h"
#include "uiprogressbar.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"


uiObjectItemViewWin::uiObjectItemViewWin(uiParent* p,const char* title)
    : uiMainWin(p,title)
    , startwidth_(400)
    , startheight_(600) 
{
    infobar_ = new uiObjectItemViewInfoBar( this );
    infobar_->setPrefWidth( startwidth_ );
    infobar_->setPrefHeight( 50 );
    infobar_->setStretch( 2, 0 );

    mainviewer_ = new uiObjectItemView( this );
    mainviewer_->setPrefWidth( startwidth_ );
    mainviewer_->setPrefHeight( startheight_ );
    mainviewer_->enableScrollBars( true );
    mainviewer_->attach( ensureBelow, infobar_, 0 );
    mainviewer_->scrollBarUsed.notify(mCB(this,uiObjectItemViewWin,scrollBarCB));
    mainviewer_->disableScrollZoom();
    infobar_->disableScrollZoom();

    makeSliders();

    setPrefWidth( startwidth_ + 50 );
    setPrefHeight( startheight_+ 50 );
}


void uiObjectItemViewWin::addObject( uiObject* obj, uiObject* infoobj )
{
    uiObjectItem* itm = new uiObjectItem( obj );
    uiObjectItem* infoitm = infoobj ? new uiObjectItem( infoobj ) : 0;
    addItem( itm, infoitm );
}


void uiObjectItemViewWin::addGroup( uiGroup* obj, uiGroup* infoobj )
{
    uiObjectItem* itm = new uiObjectItem( obj );
    uiObjectItem* infoitm = infoobj ? new uiObjectItem( infoobj ) : 0;
    addItem( itm, infoitm );
}


void uiObjectItemViewWin::addItem( uiObjectItem* itm, uiObjectItem* infoitm )
{
    mainviewer_->addItem( itm, 1 );
    infobar_->addItem( infoitm, itm );
}


#define mSldNrUnits 30

void uiObjectItemViewWin::makeSliders()
{
    uiLabel* dummylbl = new uiLabel( this, "" );
    dummylbl->attach( rightOf, mainviewer_ );
    dummylbl->setStretch( 0, 2 );
    dummylbl->attach( ensureRightOf, infobar_ );

    uiSliderExtra::Setup su;
    su.sldrsize_ = 200;
    su.withedit_ = false;
    StepInterval<float> sintv( 1, mSldNrUnits, 1 );
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
    const int nritems = mainviewer_->nrItems();
    if ( !nritems ) return;

    const int width = (int)(( hslval_*startwidth_ )+1);
    const int height = (int)(( vslval_*startheight_ )+1);
    const uiSize sz( width, height );

    const uiSize objsz = uiSize( sz.width() / nritems , sz.height() );
    for ( int idx=0; idx<nritems; idx++ )
	mainviewer_->reSizeItem( idx, objsz );

    mainviewer_->resetViewArea(0);
    infobar_->reSizeItems(); 
}


void uiObjectItemViewWin::removeAllItems()
{
    mainviewer_->removeAllItems();
    infobar_->removeAllItems();
}


void uiObjectItemViewWin::scrollBarCB( CallBacker* )
{
    const uiRect& mainrect = mainviewer_->getViewArea();
    const uiRect& inforect = infobar_->getViewArea();
    infobar_->setViewArea( mainrect.left(), inforect.top(),  
			   mainrect.right(), inforect.bottom());
    infobar_->updateItemsPos();
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



uiObjectItemViewInfoBar::uiObjectItemViewInfoBar( uiParent* p )
        : uiObjectItemView(p)
{
    enableScrollBars(false);
}


void uiObjectItemViewInfoBar::addItem( uiObjectItem* infoitm,
					uiObjectItem* cpleditm )
{
    uiObjectItemView::addItem( infoitm );
    coupleditems_ += cpleditm;
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


void uiObjectItemViewInfoBar::removeItem( uiObjectItem* itm )
{
    const int idx = objectitems_.indexOf( itm );
    if ( idx >= 0 ) coupleditems_.remove( idx );
    uiObjectItemView::removeItem( itm );
}



#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(toolbar_,fnm,tt,mCB(this,uiObjectItemViewControl,cbnm) ); \
    toolbar_->addButton( but );

uiObjectItemViewControl::uiObjectItemViewControl( uiObjectItemView& mw )
    : uiGroup(mw.parent(),"ObjectItemView control")
    , mainviewer_(mw)
    , manip_(false)		     
    , toolbar_(0)
{
    uiToolBar::ToolBarArea tba( uiToolBar::Top );
    toolbar_ = new uiToolBar( mw.parent(), "ObjectItemView tools", tba );
    mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode (Esc)");

    mainviewer_.disableScrollZoom();
    mainviewer_.setScrollBarPolicy( true, uiGraphicsView::ScrollBarAsNeeded );
    mainviewer_.setScrollBarPolicy( false, uiGraphicsView::ScrollBarAsNeeded );
    mainviewer_.scene().setMouseEventActive( true );
} 


void uiObjectItemViewControl::stateCB( CallBacker* )
{
    if ( !manipdrawbut_ ) return;
    manip_ = !manip_;

    uiGraphicsViewBase::ODDragMode mode = !manip_ ? 
	uiGraphicsViewBase::NoDrag : uiGraphicsViewBase::ScrollHandDrag;

    manipdrawbut_->setPixmap( manip_ ? "altview.png" : "altpick.png" );
    mainviewer_.setDragMode( mode );
    mainviewer_.scene().setMouseEventActive( true );
    if ( mode == uiGraphicsViewBase::ScrollHandDrag )
	cursor_.shape_ = MouseCursor::OpenHand;
    else
	cursor_.shape_ = MouseCursor::Arrow;

    mainviewer_.setCursor( cursor_ );
}



