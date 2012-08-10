/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistratlaymoddisp.cc,v 1.31 2012-08-10 03:50:06 cvsaneesh Exp $";

#include "uistratsimplelaymoddisp.h"
#include "uistratlaymodtools.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiaxishandler.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratreftree.h"
#include "property.h"


uiStratLayerModelDisp::uiStratLayerModelDisp( uiStratLayModEditTools& t,
					      const Strat::LayerModel& lm )
    : uiGroup(t.parent(),"LayerModel display")
    , tools_(t)
    , lm_(lm)
    , zoomwr_(mUdf(double),0,0,0)
    , selseqidx_(-1)
    , zrg_(0,1)
    , sequenceSelected(this)
    , genNewModelNeeded(this)
    , rangeChanged(this)   
{
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
}


void uiStratLayerModelDisp::selectSequence( int selidx )
{
    selseqidx_ = selidx;
    drawSelectedSequence();
}


bool uiStratLayerModelDisp::haveAnyZoom() const
{
    const int nrseqs = lm_.size();
    uiWorldRect wr( 1, zrg_.start, nrseqs + 1, zrg_.stop );
    return zoomwr_.isInside( wr, 1e-5 );
}


uiStratSimpleLayerModelDisp::uiStratSimpleLayerModelDisp(
		uiStratLayModEditTools& t, const Strat::LayerModel& lm )
    : uiStratLayerModelDisp(t,lm)
    , emptyitm_(0)
    , zoomboxitm_(0)
    , dispprop_(1)
    , dispeach_(1)
    , fillmdls_(true)
    , uselithcols_(true)
    , showzoomed_(false)
    , vrg_(0,1)
    , logblckitms_(*new uiGraphicsItemSet)
    , lvlitms_(*new uiGraphicsItemSet)
    , contitms_(*new uiGraphicsItemSet)
    , selseqitm_(0)
    , selseqidx_(-1)
    , selectedlevel_(-1)
    , selectedcontent_(0)
{
    gv_ = new uiGraphicsView( this, "LayerModel display" );
    gv_->setPrefWidth( 500 ); gv_->setPrefHeight( 250 );
    gv_->getMouseEventHandler().buttonReleased.notify(
			    mCB(this,uiStratSimpleLayerModelDisp,usrClicked) );

    const uiBorder border( 10 );
    uiAxisHandler::Setup xahsu( uiRect::Top );
    xahsu.border( border ).nogridline( true );
    xax_ = new uiAxisHandler( &scene(), xahsu );
    uiAxisHandler::Setup yahsu( uiRect::Left );
    yahsu.border( border ).name( "Depth" );
    yax_ = new uiAxisHandler( &scene(), yahsu );
    yax_->setEnd( xax_ );
    xax_->setBegin( yax_ );

    const CallBack redrawcb( mCB(this,uiStratSimpleLayerModelDisp,reDrawCB) );
    gv_->reSize.notify( redrawcb );
    gv_->reDrawNeeded.notify( redrawcb );
    tools_.selPropChg.notify( redrawcb );
    tools_.selLevelChg.notify( redrawcb );
    tools_.selContentChg.notify( redrawcb );
    tools_.dispEachChg.notify( redrawcb );
    tools_.dispZoomedChg.notify( redrawcb );
    tools_.dispLithChg.notify( redrawcb );
}


uiStratSimpleLayerModelDisp::~uiStratSimpleLayerModelDisp()
{
    eraseAll();
    delete &lvlitms_;
    delete &logblckitms_;
}


void uiStratSimpleLayerModelDisp::eraseAll()
{
    logblckitms_.erase();
    lvlitms_.erase();
    lvldpths_.erase();
    delete selseqitm_; selseqitm_ = 0;
    delete emptyitm_; emptyitm_ = 0;
}


uiGraphicsScene& uiStratSimpleLayerModelDisp::scene()
{
    return gv_->scene();
}


void uiStratSimpleLayerModelDisp::usrClicked( CallBacker* cb )
{
    MouseEventHandler& mevh = gv_->getMouseEventHandler();
    if ( lm_.isEmpty() || !mevh.hasEvent() || mevh.isHandled() )
	return;

    const MouseEvent& mev = mevh.event();
    if ( !OD::rightMouseButton( mev.buttonState() ) )
	return;

    float fselidx = xax_->getVal( mev.pos().x );
    int selidx = mNINT32( fselidx ) - 1;
    if ( selidx < 0 || selidx >= lm_.size() )
	return;

    mevh.setHandled( true );

    selectSequence( selidx );
    sequenceSelected.trigger();
}


void uiStratSimpleLayerModelDisp::reDrawCB( CallBacker* )
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


void uiStratSimpleLayerModelDisp::setZoomBox( const uiWorldRect& wr )
{
    if ( !zoomboxitm_ )
    {
	zoomboxitm_ = scene().addItem( new uiRectItem );
	zoomboxitm_->setPenStyle( LineStyle(LineStyle::Dot,3,Color::Black()) );
	zoomboxitm_->setZValue( 100 );
    }

    // provided rect is always in system [0.5,N+0.5]
    zoomwr_.setLeft( wr.left() + .5 );
    zoomwr_.setRight( wr.right() + .5 );
    zoomwr_.setTop( wr.bottom() );
    zoomwr_.setBottom( wr.top() );
    updZoomBox();
    if ( showzoomed_ )
	reDrawCB( 0 );
}


void uiStratSimpleLayerModelDisp::updZoomBox()
{
    if ( zoomwr_.width() < 0.001 || !zoomboxitm_ || !xax_ )
	{ if ( zoomboxitm_ ) zoomboxitm_->setVisible( false ); return; }

    const int xpix = xax_->getPix( (float) zoomwr_.left() );
    const int ypix = yax_->getPix( (float) zoomwr_.top() );
    const int wdth = xax_->getPix( (float) zoomwr_.right() ) - xpix;
    const int hght = yax_->getPix( (float) zoomwr_.bottom() ) - ypix;
    zoomboxitm_->setRect( xpix, ypix, wdth, hght );
    zoomboxitm_->setVisible( haveAnyZoom() && !showzoomed_ );
}


void uiStratSimpleLayerModelDisp::modelChanged()
{
    zoomwr_ = uiWorldRect(mUdf(double),0,0,0);
    reDrawCB( 0 );
}


#define mStartLayLoop(chckdisp) \
    const int nrseqs = lm_.size(); \
    for ( int iseq=0; iseq<nrseqs; iseq++ ) \
    { \
	if ( chckdisp && !isDisplayedModel(iseq) ) continue; \
	float prevval mUnusedVar = mUdf(float); \
	const Strat::LayerSequence& seq = lm_.sequence( iseq ); \
	const int nrlays = seq.size(); \
	for ( int ilay=0; ilay<nrlays; ilay++ ) \
	{ \
	    const Strat::Layer& lay = *seq.layers()[ilay]; \
	    const float z0 = lay.zTop(); \
	    const float z1 = lay.zBot(); \
	    const float val = dispprop_ < lay.nrValues() \
	    		? lay.value( dispprop_ ) : mUdf(float);

#define mEndLayLoop() \
	    prevval = val; \
	} \
    }


void uiStratSimpleLayerModelDisp::getBounds()
{
    Interval<float> zrg(mUdf(float),mUdf(float)), vrg(mUdf(float),mUdf(float));
    mStartLayLoop( false )
#	define mChckBnds(var,op,bnd) \
	if ( (mIsUdf(var) || var op bnd) && !mIsUdf(bnd) ) \
	    var = bnd
	mChckBnds(zrg.start,>,z0);
	mChckBnds(zrg.stop,<,z1);
	mChckBnds(vrg.start,>,val);
	mChckBnds(vrg.stop,<,val);
    mEndLayLoop()

    if ( mIsUdf(zrg.start) )
	zrg_ = Interval<float>(0,1);
    else
	zrg_ = zrg;
    if ( mIsUdf(vrg.start) )
	vrg_ = Interval<float>(0,1);
    else
	vrg_ = vrg;

    if ( mIsUdf(zoomwr_.left()) )
    {
	zoomwr_.setLeft( 1 );
	zoomwr_.setRight( nrseqs+1 );
	zoomwr_.setTop( zrg_.stop );
	zoomwr_.setBottom( zrg_.start );
    }
}


int uiStratSimpleLayerModelDisp::getXPix( int iseq, float relx ) const
{
    static const float margin = 0.05;
    relx = (1-margin) * relx + margin * .5f; // get relx between 0.025 and 0.975
    relx *= dispeach_;
    return xax_->getPix( iseq + 1 + relx );
}


bool uiStratSimpleLayerModelDisp::isDisplayedModel( int iseq ) const
{
    if ( iseq % dispeach_ )
	return false;

    if ( showzoomed_ )
    {
	const int xpix0 = getXPix( iseq, 0 );
	const int xpix1 = getXPix( iseq, 1 );
	if ( xax_->getVal(xpix1) > zoomwr_.right()
	  || xax_->getVal(xpix0) < zoomwr_.left() )
	    return false;
    }
    return true;
}


void uiStratSimpleLayerModelDisp::doDraw()
{
    dispprop_ = tools_.selPropIdx();
    selectedlevel_ = tools_.selLevelIdx();
    dispeach_ = tools_.dispEach();
    showzoomed_ = tools_.dispZoomed();
    uselithcols_ = tools_.dispLith();
    selectedcontent_ = lm_.refTree().contents().getByName(tools_.selContent());
    getBounds();

    xax_->updateDevSize(); yax_->updateDevSize();
    if ( !showzoomed_ )
    {
	xax_->setBounds( Interval<float>(1,lm_.size()+1) );
	yax_->setBounds( Interval<float>(zrg_.stop,zrg_.start) );
    }
    else
    {
	xax_->setBounds( Interval<float>((float) zoomwr_.left(),
						(float) zoomwr_.right()) );
	yax_->setBounds( Interval<float>((float) zoomwr_.top(),
						(float) zoomwr_.bottom()) );
    }
    yax_->plotAxis(); xax_->plotAxis();
    const float vwdth = vrg_.width();

    mStartLayLoop( true )

	if ( showzoomed_ )
	{
	    if ( z0 > zoomwr_.top() || z1 < zoomwr_.bottom() )
		continue;
	    if ( z1 > zoomwr_.top() )
		const_cast<float&>(z1) = (float) zoomwr_.top();
	    if ( z0 < zoomwr_.bottom() )
		const_cast<float&>(z0) = (float) zoomwr_.bottom();
	}

	const int ypix0 = yax_->getPix( z0 );
	const int ypix1 = yax_->getPix( z1 );
	if ( ypix0 != ypix1 && !mIsUdf(val) )
	{
	    const float relx = (val-vrg_.start) / vwdth;
	    const int xpix0 = getXPix( iseq, 0 );
	    const int xpix1 = getXPix( iseq, relx );
	    uiRectItem* it = scene().addRect( xpix0, ypix0,
		    			xpix1-xpix0+1, ypix1-ypix0+1 );
	    const Color laycol = lay.dispColor( uselithcols_ );
	    const bool isannotcont = selectedcontent_
				  && lay.content() == *selectedcontent_;
	    const Color pencol = isannotcont ? laycol.complementaryColor()
					     : laycol;
	    it->setPenColor( pencol );
	    if ( pencol != laycol )
		it->setPenStyle( LineStyle(LineStyle::Solid,2,pencol) );
	    if ( fillmdls_ )
		it->setFillColor( laycol );
	    logblckitms_ += it;
	}

    mEndLayLoop()

    drawLevels();
    drawSelectedSequence();
    updZoomBox();
}


void uiStratSimpleLayerModelDisp::drawLevels()
{
    lvldpths_.erase();
    const Strat::Level* lvl = tools_.selStratLevel();
    if ( !lvl ) return;
    lvlcol_ = tools_.selLevelColor();
    const int nrseqs = lm_.size();
    if ( nrseqs < 1 ) return;

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
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


void uiStratSimpleLayerModelDisp::drawSelectedSequence()
{
    delete selseqitm_; selseqitm_ = 0;
    const int nrseqs = lm_.size();
    if ( nrseqs < 1 || selseqidx_ > nrseqs || selseqidx_ < 0 ) return;

    const int ypix1 = yax_->getPix( yax_->range().start );
    const int ypix2 = yax_->getPix( yax_->range().stop );
    const float xpix1 = (float)getXPix( selseqidx_, 0 );
    const float xpix2 = (float)getXPix( selseqidx_, 1 );
    const int midpix = (int)( xpix1 + ( xpix2 - xpix1 ) /2 );

    uiLineItem* it = scene().addItem(
		    new uiLineItem( midpix, ypix1, midpix, ypix2, true ) );
    it->setPenStyle( LineStyle(LineStyle::Dot,2,Color::Black()) );
    it->setZValue( 2 );
    selseqitm_ = it;
}
