/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlaymoddisp.cc,v 1.15 2010-12-21 13:19:26 cvsbert Exp $";

#include "uistratlaymoddisp.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uispinbox.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uigraphicsview.h"
#include "uiaxishandler.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "property.h"


uiStratLayerModelDisp::uiStratLayerModelDisp( uiParent* p,
						const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel display")
    , lm_(lm)
    , emptyitm_(0)
    , zoomboxitm_(0)
    , zoomwr_(0,0,0,0)
    , dispprop_(1)
    , dispeach_(1)
    , fillmdls_(true)
    , zrg_(0,1)
    , vrg_(0,1)
    , logblckitms_(*new uiGraphicsItemSet)
    , lvlitms_(*new uiGraphicsItemSet)
    , dispEachChg(this)
    , levelChg(this)
{
    const CallBack redrawcb( mCB(this,uiStratLayerModelDisp,reDraw) );
    gv_ = new uiGraphicsView( this, "LayerModel display" );
    gv_->setPrefWidth( 500 ); gv_->setPrefHeight( 250 );
    gv_->reSize.notify( redrawcb );
    gv_->reDrawNeeded.notify( redrawcb );

    gv_->getMouseEventHandler().buttonReleased.notify(
	    			mCB(this,uiStratLayerModelDisp,usrClickCB) );

    const uiBorder border( 10 );
    uiAxisHandler::Setup xahsu( uiRect::Top );
    xahsu.border( border ).nogridline( true );
    xax_ = new uiAxisHandler( &scene(), xahsu );
    uiAxisHandler::Setup yahsu( uiRect::Left );
    yahsu.border( border ).name( "Depth" );
    yax_ = new uiAxisHandler( &scene(), yahsu );
    yax_->setEnd( xax_ );
    xax_->setBegin( yax_ );

    qtyfld_ = new uiGenInput( this, "Display", StringListInpSpec() );
    qtyfld_->attach( ensureBelow, gv_ );
    qtyfld_->valuechanged.notify( redrawcb );
    uiLabel* eachlbl = new uiLabel( this, "each" );
    eachlbl->attach( rightOf, qtyfld_ );
    eachfld_ = new uiSpinBox( this, 0, "DispEach" );
    eachfld_->setInterval( 1, 1000 );
    eachfld_->attach( rightOf, eachlbl );
    eachfld_->valueChanging.notify(
	    		mCB(this,uiStratLayerModelDisp,dispEachChgd) );
    lvlfld_ = new uiComboBox( this, "Level" );
    lvlfld_->attach( rightOf, eachfld_ );
    lvlfld_->selectionChanged.notify( mCB(this,uiStratLayerModelDisp,lvlChgd) );
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
    eraseAll();
    delete &lvlitms_;
    delete &logblckitms_;
}


int uiStratLayerModelDisp::getEachDisp() const
{
    return eachfld_->getValue();
}


void uiStratLayerModelDisp::getDispProperties( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<lm_.propertyRefs().size(); idx++ )
	nms.add( lm_.propertyRefs()[idx]->name() );
}


void uiStratLayerModelDisp::eraseAll()
{
    logblckitms_.erase();
    lvlitms_.erase();
    lvldpths_.erase();
    delete emptyitm_; emptyitm_ = 0;
}


void uiStratLayerModelDisp::dispEachChgd( CallBacker* cb )
{
    dispeach_ = getEachDisp();
    reDraw( cb );
    dispEachChg.trigger();
    if ( zoomboxitm_ )
	zoomboxitm_->setVisible( false );
}


void uiStratLayerModelDisp::lvlChgd( CallBacker* cb )
{
    reDraw( cb );
    levelChg.trigger();
}


void uiStratLayerModelDisp::usrClickCB( CallBacker* cb )
{
}


uiGraphicsScene& uiStratLayerModelDisp::scene()
{
    return gv_->scene();
}


void uiStratLayerModelDisp::reDraw( CallBacker* )
{
    eraseAll(); lm_.prepareUse();
    if ( lm_.isEmpty() )
    {
	emptyitm_ = scene().addItem( new uiTextItem( "<---empty--->",
				     mAlignment(HCenter,VCenter) ) );
	emptyitm_->setPenColor( Color::Black() );
	emptyitm_->setPos( uiPoint( gv_->width()/2, gv_->height() / 2 ) );
	return;
    }

    doDraw();
}


void uiStratLayerModelDisp::setZoomBox( const uiWorldRect& wr )
{
    if ( !zoomboxitm_ )
    {
	zoomboxitm_ = scene().addItem( new uiRectItem );
	zoomboxitm_->setPenStyle( LineStyle(LineStyle::Dot,3,Color::Black()) );
	zoomboxitm_->setZValue( 100 );
    }

    zoomwr_ = wr; // provided rect is always in system [0.5,N+0.5]
    zoomwr_.setLeft( (wr.left()-0.5) * dispeach_ + 1 );
    zoomwr_.setRight( (wr.right()-0.5) * dispeach_ + 1 );
    updZoomBox();
}


void uiStratLayerModelDisp::updZoomBox()
{
    if ( zoomwr_.width() < 0.001 || !xax_ )
	{ if ( zoomboxitm_ ) zoomboxitm_->setVisible( false ); return; }

    const int xpix = xax_->getPix( zoomwr_.left() );
    const int ypix = yax_->getPix( zoomwr_.top() );
    const int wdth = xax_->getPix( zoomwr_.right() ) - xpix;
    const int hght = yax_->getPix( zoomwr_.bottom() ) - ypix;
    zoomboxitm_->setRect( xpix, ypix, wdth, hght );
    zoomboxitm_->setVisible( true );
}


void uiStratLayerModelDisp::modelChanged()
{
    BufferString selnm( qtyfld_->text() );
    BufferStringSet nms;
    for ( int idx=1; idx<lm_.propertyRefs().size(); idx++ )
	nms.add( lm_.propertyRefs()[idx]->name() );
    qtyfld_->newSpec( StringListInpSpec(nms), 0 );
    int idxof = nms.isEmpty() || selnm.isEmpty() ? -1 : nms.indexOf( selnm );
    if ( idxof >= 0 )
	qtyfld_->setValue( idxof );

    selnm = lvlfld_->text();
    lvlfld_->setEmpty();
    if ( !lm_.isEmpty() )
    {
	const Strat::LevelSet& lvls = Strat::LVLS();
	lvlfld_->addItem( "---" );
	for ( int idx=0; idx<lvls.size(); idx++ )
	    lvlfld_->addItem( lvls.levels()[idx]->name() );
	if ( lvlfld_->size() > 1 )
	{
	    idxof = selnm.isEmpty() ? -1 : lvlfld_->indexOf( selnm );
	    if ( idxof < 0 ) idxof = 1;
	    lvlfld_->setCurrentItem( idxof );
	}
    }

    zoomwr_ = uiWorldRect(0,0,0,0);
    reDraw( 0 );
}


#define mStartLayLoop(op) \
    const int nrseqs = lm_.size(); \
    for ( int iseq=0; iseq<nrseqs; iseq++ ) \
    { \
	if ( iseq % dispeach_ ) continue; \
	op; \
	float prevval = mUdf(float); \
	const Strat::LayerSequence& seq = lm_.sequence( iseq ); \
	const int nrlays = seq.size(); \
	for ( int ilay=0; ilay<nrlays; ilay++ ) \
	{ \
	    const Strat::Layer& lay = *seq.layers()[ilay]; \
	    const float z0 = lay.zTop(); \
	    const float z1 = lay.zBot(); \
	    const float val = lay.value( dispprop_ );

#define mEndLayLoop(op) \
	    prevval = val; \
	} \
	op; \
    }


void uiStratLayerModelDisp::getBounds()
{
    Interval<float> zrg(mUdf(float),mUdf(float)), vrg(mUdf(float),mUdf(float));
    mStartLayLoop(;)
#	define mChckBnds(var,op,bnd) \
	if ( (mIsUdf(var) || var op bnd) && !mIsUdf(bnd) ) \
	    var = bnd
	mChckBnds(zrg.start,>,z0);
	mChckBnds(zrg.stop,<,z1);
	mChckBnds(vrg.start,>,val);
	mChckBnds(vrg.stop,<,val);
    mEndLayLoop(;)

    if ( mIsUdf(zrg.start) )
	zrg_ = Interval<float>(0,1);
    else
	zrg_ = zrg;
    if ( mIsUdf(vrg.start) )
	vrg_ = Interval<float>(0,1);
    else
	vrg_ = vrg;
}


int uiStratLayerModelDisp::getXPix( int iseq, float relx ) const
{
    static const float margin = 0.05;
    relx = (1-margin) * relx + margin * .5; // get relx between 0.025 and 0.975
    relx *= dispeach_;
    return xax_->getPix( iseq + 1 + relx );
}


void uiStratLayerModelDisp::doDraw()
{
    dispprop_ = qtyfld_->getIntValue() + 1;
    getBounds();

    xax_->updateDevSize(); yax_->updateDevSize();
    xax_->setBounds( Interval<float>(1,lm_.size()+1) );
    yax_->setBounds( Interval<float>(zrg_.stop,zrg_.start) );
    yax_->plotAxis(); xax_->plotAxis();
    const float vwdth = vrg_.width();
    TypeSet<uiPoint> polypts;

    mStartLayLoop(polypts.erase())

	const int ypix0 = yax_->getPix( z0 );
	const int ypix1 = yax_->getPix( z1 );
	if ( ypix0 != ypix1 && !mIsUdf(val) )
	{
	    const float relx = (val-vrg_.start) / vwdth;
	    const int xpix = getXPix( iseq, relx );
	    polypts += uiPoint( xpix, ypix0 );
	    polypts += uiPoint( xpix, ypix1 );
	}

    mEndLayLoop(drawModel(polypts,iseq))

    drawLevels();
    updZoomBox();
}


void uiStratLayerModelDisp::drawModel( TypeSet<uiPoint>& polypts, int iseq )
{
    if ( polypts.isEmpty() )
	return;

    if ( fillmdls_ )
    {
	const int xpix = getXPix( iseq, 0 );
	polypts += uiPoint( xpix, polypts[polypts.size()-1].y );
	polypts += uiPoint( xpix, polypts[0].y );
	polypts += uiPoint( polypts[0] );
    }

    uiPolygonItem* it = scene().addPolygon( polypts, fillmdls_ );
    const Color vcol( lm_.propertyRefs()[dispprop_]->disp_.color_ );
    it->setPenColor( vcol ); it->setFillColor( vcol );
    logblckitms_ += it;
}


const char* uiStratLayerModelDisp::selectedLevel() const
{
    return lvlfld_->currentItem() == 0 ? 0 : lvlfld_->text();
}


void uiStratLayerModelDisp::drawLevels()
{
    lvldpths_.erase(); lvlcol_ = Color::NoColor();
    const Strat::Level* lvl = Strat::LVLS().get( selectedLevel() );
    if ( !lvl ) return;
    lvlcol_ = lvl->color();
    const int nrseqs = lm_.size();
    if ( nrseqs < 1 ) return;

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	if ( iseq % dispeach_ ) continue;
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	const int idxof = seq.indexOf( *lvl );
	if ( idxof < 0 )
	    { lvldpths_ += mUdf(float); continue; }

	const Strat::Layer& lay = *seq.layers()[idxof];
	const float zlvl = lay.zTop();
	lvldpths_ += zlvl;
	const int ypix = yax_->getPix( zlvl );
	const int xpix1 = getXPix( iseq, 0 );
	const int xpix2 = getXPix( iseq, 1 );
	uiLineItem* it = scene().addItem(
			new uiLineItem( xpix1, ypix, xpix2, ypix, true ) );
	it->setPenStyle( LineStyle(LineStyle::Solid,2,lvlcol_) );
	it->setZValue( 1 );
	lvlitms_ += it;
    }
}
