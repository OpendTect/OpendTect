/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/

#include "uiobjectitemviewwin.h"

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uibutton.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uilabel.h"
#include "uirgbarraycanvas.h"
#include "uiprogressbar.h"
#include "uislider.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uistrings.h"
#include "mousecursor.h"

#define mSldUnits 250
#define mMaxObjectSize 30 //18 x object size
#define mScrollBarSize mainviewer_->scrollBarSize(false).width()-2

uiObjectItemViewWin::uiObjectItemViewWin( uiParent* p, const Setup& su )
    : uiMainWin(p,su.wintitle_)
    , startwidth_(su.startwidth_)
    , startheight_(su.startheight_)
    , infoheight_(su.infoheight_)
    , screensz_(uiSize(su.startwidth_,su.startheight_))
    , fittoscreen_(true)
    , hslval_(1.0f)
    , vslval_(1.0f)
{
    setPrefWidth( startwidth_ + su.infoheight_ );
    setPrefHeight( startheight_+ su.infoheight_ );

    mainviewer_ = new uiObjectItemView( this );
    mainviewer_->setSceneLayoutPos( su.layoutpos_ );
    mainviewer_->setPrefWidth( startwidth_ );
    mainviewer_->setPrefHeight( startheight_ );
    mainviewer_->enableScrollBars( true );
    mainviewer_->disableScrollZoom();
    mainviewer_->setMidMouseButtonForDrag( true );
    mainviewer_->reSize.notify( mCB(this,uiObjectItemViewWin,reSizeCB) );
    mainviewer_->rubberBandUsed.notify(mCB(this,uiObjectItemViewWin,rubBandCB));
    mainviewer_->scrollBarUsed.notify(
	    mCB(this,uiObjectItemViewWin,scrollBarCB) );
    infobar_ = new uiObjectItemViewInfoBar( this );
    infobar_->setPrefWidth( startwidth_ - mScrollBarSize );
    infobar_->setPrefHeight( su.infoheight_ );
    infobar_->setSceneLayoutPos( su.layoutpos_ );
    infobar_->setStretch( 2, 0 );
    infobar_->disableScrollZoom();
    infobar_->viewareareset.notify( mCB(this,uiObjectItemViewWin,scrollBarCB) );

    uiGraphicsView* dummyview = new uiGraphicsView( this, "Dummy view" );
    dummyview->setNoBackGround();
    dummyview->setPrefWidth( mScrollBarSize );
    dummyview->setPrefHeight( su.infoheight_ );
    dummyview->attach( rightOf, infobar_, 0 );
    dummyview->setStretch( 0, 0 );

    mainviewer_->attach( ensureBelow, infobar_, 0 );
    mainviewer_->attach( ensureBelow, dummyview, 0 );

    mainviewer_->setSceneBorder(0);
    infobar_->setSceneBorder(0);

    makeSliders();
    versliderfld_->attach( ensureRightOf, dummyview );
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
    uiLabel* dummylbl = new uiLabel( this, uiString::empty() );
    dummylbl->attach( rightOf, mainviewer_ );
    dummylbl->setStretch( 0, 2 );
    dummylbl->attach( ensureRightOf, infobar_ );

    uiSlider::Setup slsu;
    slsu.isvertical( true ).sldrsize( 250 ).isinverted( true );
    StepInterval<float> sintv( 1, mSldUnits, 1 );
    versliderfld_ = new uiSlider( this, slsu, "Vertical Scale" );
    versliderfld_->setInterval( sintv );
    versliderfld_->sliderReleased.notify(
				mCB(this,uiObjectItemViewWin,reSizeSld) );
    versliderfld_->attach( alignedBelow, dummylbl );
    versliderfld_->slider()->setVSzPol( uiObject::WideVar );
    versliderfld_->slider()->setPrefWidth( uiObject::toolButtonSize() );
    versliderfld_->setStretch( 0, 1 );

    fittoscreenbut_ = new uiToolButton( this, "exttofullsurv",
					tr("Fit to screen"),
                                        mCB(this,uiObjectItemViewWin,
                                        fitToScreen));
    fittoscreenbut_->attach( alignedBelow, versliderfld_ );

    slsu.isvertical( false ).isinverted( false );
    horsliderfld_ = new uiSlider( this, slsu, "Horizontal Scale" );
    horsliderfld_->setInterval( sintv );
    horsliderfld_->sliderReleased.notify(
				    mCB(this,uiObjectItemViewWin,reSizeSld));
    horsliderfld_->setStretch( 1, 0 );
    horsliderfld_->attach( leftOf, fittoscreenbut_ );
    horsliderfld_->attach( ensureBelow, mainviewer_ );

    zoomratiofld_ = new uiCheckBox( this, tr("Keep zoom ratio") );
    zoomratiofld_->attach( leftOf, horsliderfld_ );
    zoomratiofld_->setChecked( true );
}


void uiObjectItemViewWin::reSizeSld( CallBacker* cb )
{
    const float prevhslval = hslval_;
    const float prevvslval = vslval_;

    uiSlider* hsldr = horsliderfld_;
    uiSlider* vsldr = versliderfld_;
    hslval_ = hsldr->getFValue();
    vslval_ = vsldr->getFValue();

    mDynamicCastGet(uiSlider*,sld,cb);
    fittoscreen_ = !sld;

    if ( sld && zoomratiofld_->isChecked() )
    {
	bool ishor = sld == hsldr;
	uiSlider* revsld = ishor ? vsldr : hsldr;
	ishor ? vslval_ = hslval_ : hslval_ = vslval_;

	NotifyStopper nsl( revsld->sliderReleased );
	revsld->setValue( hslval_ );
    }
    const uiRect& mainrect = mainviewer_->getViewArea();
    const int x = mainrect.left();
    const int y = mainrect.top();
    const int width = mainrect.width();
    const int height = mainrect.height();
    float xcenter = x + width/(float)2;
    float ycenter = y + height/(float)2;

    reSizeItems();

    const float hfac = hslval_/prevhslval;
    const float vfac = vslval_/prevvslval;
    xcenter *= hfac;
    ycenter *= vfac;

    mainviewer_->centreOn( uiPoint( mNINT32(xcenter), mNINT32(ycenter) ) );
}


void uiObjectItemViewWin::scaleVal( float& val, bool hor, bool yn )
{
    scaler_.set( 1, 1, mSldUnits, mMaxObjectSize );
    val = (float) ( yn ? scaler_.scale( val ) : scaler_.unScale( val ) );
}


void uiObjectItemViewWin::reSizeItems()
{
    const int nritems = mainviewer_->nrItems();
    if ( !nritems ) return;

    scaleVal( hslval_, true, true );
    scaleVal( vslval_, false, true );

    const int w = mNINT32( hslval_*startwidth_/(float)nritems );
    const int h = mNINT32( vslval_*startheight_ );
    for ( int idx=0; idx<nritems; idx++ )
	mainviewer_->reSizeItem( idx, uiSize( w, h ) );

    infobar_->reSizeItems();
    mainviewer_->resetViewArea(0);
    infobar_->resetViewArea(0);
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
    const int y = inforect.top();
    const int w = mainrect.width() - mScrollBarSize;
    infobar_->centreOn( uiPoint( x + (int)(w/(float)2), y  ) );
}


void uiObjectItemViewWin::reSizeCB( CallBacker* )
{
    if ( !fittoscreen_ ) return;
    const uiSize newscreensz( mainviewer_->viewWidth(),
			mainviewer_->height() );
    if ( newscreensz != screensz_ ) fitToScreen(0);
}


void uiObjectItemViewWin::fitToScreen( CallBacker* )
{
    mDynamicCastGet(uiGraphicsObjectScene*,sc,&mainviewer_->scene())
    const uiSize screensz( mainviewer_->viewWidth(),
			mainviewer_->viewHeight() );
    if ( screensz.width()<=0 || screensz.height()<=0 )
	return;

    const uiSize layoutsz(sc->layoutSize().width(),sc->layoutSize().height());
    if ( layoutsz.width()<=0 || layoutsz.height()<=0 ) return;
    float xratio = screensz.width()/(float)layoutsz.width();
    float yratio = screensz.height()/(float)layoutsz.height();
    float newhslval = hslval_*xratio;
    float newvslval = vslval_*yratio;
    scaleVal( newhslval, true, false );
    scaleVal( newvslval, false, false );
    if ( ( newhslval == hslval_ ) && ( newvslval == vslval_ ) )
	return;

    horsliderfld_->setValue( newhslval );
    versliderfld_->setValue( newvslval );

    zoomratiofld_->setChecked(false);
    screensz_ = screensz;
    reSizeSld(0);
}


void uiObjectItemViewWin::fillPar( IOPar& iop ) const
{
    if ( !versliderfld_ || !horsliderfld_ ) return;

    iop.set( sKeyVZoomVal(), versliderfld_->getFValue() );
    iop.set( sKeyHZoomVal(), horsliderfld_->getFValue() );
}


void uiObjectItemViewWin::usePar( const IOPar& iop )
{
    if ( !versliderfld_ || !horsliderfld_ ) return;

    float hval, vval;
    iop.get( sKeyHZoomVal(), hval );
    iop.get( sKeyVZoomVal(), vval );
    horsliderfld_->setValue( hval );
    versliderfld_->setValue( vval );
    reSizeSld(0);
}


#define mMinSelWidth 10
#define mMinSelHeight 10

void uiObjectItemViewWin::rubBandCB( CallBacker* )
{
    const uiRect* selrect = mainviewer_->getSelectedArea();
    if ( !selrect ) return;

    const int selwidth = selrect->width();
    const int selheight = selrect->height();
    if ( selwidth<mMinSelWidth || selheight<mMinSelHeight )
	return;

    const uiRect viewrect = mainviewer_->getViewArea();
    const int viewwidth = viewrect.width();
    const int viewheight = viewrect.height();

    const float xfac = float(viewwidth)/selwidth;
    const float yfac = float(viewheight)/selheight;

    zoomratiofld_->setChecked(false);

    float newhorfac = xfac*hslval_;
    scaleVal( newhorfac, true, false );
    float newverfac = yfac*vslval_;
    scaleVal( newverfac, false, false );

    uiSlider* hsldr = horsliderfld_;
    uiSlider* vsldr = versliderfld_;
    NotifyStopper nsh( hsldr->sliderReleased );
    NotifyStopper nsv( vsldr->sliderReleased );
    hsldr->setValue( newhorfac );
    vsldr->setValue( newverfac );
    reSizeSld(0); fittoscreen_ = false;

    mainviewer_->setViewArea( selrect->left()*xfac, selrect->top()*yfac,
			      selrect->width()*xfac, selrect->height()*yfac );
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
    resetViewArea(0);
}


void uiObjectItemViewInfoBar::removeItem( uiObjectItem* itm )
{
    const int idx = objectitems_.indexOf( itm );
    if ( idx >= 0 ) coupleditems_.removeSingle( idx );
    uiObjectItemView::removeItem( itm );
    resetViewArea(0);
}


void uiObjectItemViewInfoBar::removeItemByCouple( uiObjectItem* coupleditem )
{
    const int idx = coupleditems_.indexOf( coupleditem );
    if ( objectitems_.validIdx( idx ) )
	removeItem( objectitems_[idx] );
    resetViewArea(0);
}


void uiObjectItemViewInfoBar::insertItem( uiObjectItem* itm,
					uiObjectItem* cpleditm, int pos )
{
    insertItem( itm, pos );
    coupleditems_.insertAt( cpleditm, pos );
    resetViewArea(0);
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
    resetViewArea(0);
}




#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(toolbar_,fnm,tt, \
    mCB(this,uiObjectItemViewControl,cbnm) ); \
    toolbar_->add( but );

uiObjectItemViewControl::uiObjectItemViewControl( uiObjectItemView& mw )
    : uiGroup(mw.parent(),"ObjectItemView control")
    , mainviewer_(mw)
    , manipdrawbut_(0)
    , toolbar_(0)
    , cursor_(*new MouseCursor)
{
    uiToolBar::ToolBarArea tba( uiToolBar::Top );
    toolbar_ = new uiToolBar( mw.parent(), tr("ObjectItemView Tools"), tba );
    setToolButtons();

    mainviewer_.disableScrollZoom();
    mainviewer_.getKeyboardEventHandler().keyPressed.notify(
	    mCB(this,uiObjectItemViewControl,keyPressedCB) );
    mainviewer_.setScrollBarPolicy( true, uiGraphicsView::ScrollBarAsNeeded );
    mainviewer_.setScrollBarPolicy( false, uiGraphicsView::ScrollBarAsNeeded );
    mainviewer_.setDragMode( uiGraphicsViewBase::NoDrag );
}


uiObjectItemViewControl::~uiObjectItemViewControl()
{
    delete &cursor_;
}


void uiObjectItemViewControl::setToolButtons()
{
    mDefBut(manipdrawbut_,"rubbandzoom",stateCB,tr("Switch view mode (Esc)"));
    manipdrawbut_->setToggleButton( true );
}


void uiObjectItemViewControl::keyPressedCB( CallBacker* )
{
    if ( mainviewer_.getKeyboardEventHandler().event().key_ == OD::KB_Escape )
	setRubberBandingOn( !manipdrawbut_->isOn() );
}


void uiObjectItemViewControl::stateCB( CallBacker* )
{
    changeStatus();
}


void uiObjectItemViewControl::setRubberBandingOn( bool yn )
{
    manipdrawbut_->setOn( yn );
    changeStatus();
}


void uiObjectItemViewControl::changeStatus()
{
    const bool isrubband = manipdrawbut_->isOn();
    uiGraphicsViewBase::ODDragMode mode = isrubband ?
	uiGraphicsViewBase::RubberBandDrag : uiGraphicsViewBase::NoDrag;
    mainviewer_.setDragMode( mode );
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
    const int widthshift = scene_->layoutPos().x_ - zax_->pixToEdge(false);
    const int heightshift = scene_->layoutPos().y_;
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
	zax_->updateScene();
}
