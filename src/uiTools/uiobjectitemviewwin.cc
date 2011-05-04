/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjectitemviewwin.cc,v 1.1 2011-05-04 15:20:02 cvsbruno Exp $";

#include "uiobjectitemviewwin.h"

#include "uigraphicsitemimpl.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uirgbarraycanvas.h"
#include "uislider.h"
#include "uiprogressbar.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"


uiObjectItemViewWin::uiObjectItemViewWin(uiParent* p,const char* title)
    : uiMainWin(p,title)
    , startwidth_(80)
    , startheight_(100) 
{
    infobar_ = new uiObjectItemViewInfoBar( this );
    infobar_->setPrefWidth( startwidth_ );
    infobar_->setPrefHeight( 50 );
    infobar_->setStretch( 2, 0 );

    mainviewer_ = new uiObjectItemView( this );
    mainviewer_->setPrefWidth( startwidth_ );
    mainviewer_->setPrefHeight( startheight_ );
    mainviewer_->enableScrollBars( false );
    mainviewer_->attach( alignedBelow, infobar_, 0 );

    setPrefWidth( startwidth_ );
    setPrefHeight( startheight_ + 20 );

    makeSliders();
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


#define mSldNrUnits 50

void uiObjectItemViewWin::makeSliders()
{
    uiLabel* dummylbl = new uiLabel( this, "" );
    dummylbl->attach( rightOf, mainviewer_ );
    dummylbl->setStretch( 0, 2 );
    if ( infobar_ )
	infobar_->attach( ensureLeftOf, dummylbl );

    uiSliderExtra::Setup su;
    su.sldrsize_ = 150;
    su.withedit_ = false;
    StepInterval<float> sintv( 1, mSldNrUnits, 1 );
    su.isvertical_ = true;
    versliderfld_ = new uiSliderExtra( this, su, "Vertical Scale" );
    versliderfld_->sldr()->setInterval( sintv );
    versliderfld_->sldr()->setValue( 5 );
    versliderfld_->sldr()->setInverted( true );
    versliderfld_->sldr()->sliderReleased.notify(
	    			mCB(this,uiObjectItemViewWin,reSizeSld) );
    versliderfld_->attach( centeredBelow, dummylbl );
    versliderfld_->setStretch( 0, 0 );

    su.isvertical_ = false;
    horsliderfld_ = new uiSliderExtra( this, su, "Horizontal Scale" );
    horsliderfld_->sldr()->setInterval( sintv );
    horsliderfld_->sldr()->sliderReleased.notify(
				    mCB(this,uiObjectItemViewWin,reSizeSld));
    horsliderfld_->setStretch( 0, 0 );
    horsliderfld_->sldr()->setValue( 5 );
    horsliderfld_->attach( rightBorder, 20 );
    horsliderfld_->attach( ensureLeftOf, versliderfld_ );
    horsliderfld_->attach( ensureBelow, versliderfld_ );
    horsliderfld_->attach( ensureBelow, mainviewer_ );

    zoomratiofld_ = new uiCheckBox( this, "Keep zoom ratio" );
    zoomratiofld_->attach( leftOf, horsliderfld_ );
    zoomratiofld_->setChecked( true );
}


void uiObjectItemViewWin::reSizeSld( CallBacker* cb )
{
    uiSlider* hsldr = horsliderfld_->sldr();
    uiSlider* vsldr = versliderfld_->sldr();
    float hslval = hsldr->getValue();
    float vslval = vsldr->getValue();

    mDynamicCastGet(uiSlider*,sld,cb)
    if ( sld && zoomratiofld_->isChecked() ) 
    {
	bool ishor = sld == hsldr;
	uiSlider* revsld = ishor ? vsldr : hsldr;
	if ( ishor )
	    vslval = hslval;
	else
	    hslval = vslval;

	NotifyStopper ns( revsld->sliderReleased );
	revsld->setValue( hslval );
    }

    const int width = (int)( hslval*startwidth_ );
    const int height = (int)( vslval*startheight_ );
    reSizeItems( uiSize( width, height ) );
}


void uiObjectItemViewWin::reSizeItems( const uiSize& sz )
{
    const int nritems = mainviewer_->nrItems();
    if ( !nritems ) return;

    const uiSize objsz = uiSize( sz.width() / nritems , sz.height() );
    for ( int idx=0; idx<nritems; idx++ )
	mainviewer_->reSizeItem( idx, objsz );

    infobar_->reSizeItems(); 
}


void uiObjectItemViewWin::removeAllItems()
{
    mainviewer_->removeAllItems();
    infobar_->removeAllItems();
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

