/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlaymoddisp.cc,v 1.3 2010-10-27 15:18:18 cvsbert Exp $";

#include "uistratlaymoddisp.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uiaxishandler.h"
#include "stratlayermodel.h"
#include "property.h"


uiStratLayerModelDisp::uiStratLayerModelDisp( uiParent* p,
						const Strat::LayerModel& lm )
    : uiGraphicsView(p,"LayerModel display")
    , lm_(lm)
    , emptyitm_(0)
    , dispprop_(1)
    , zrg_(0,1)
    , vrg_(0,1)
    , logblckitms_(*new uiGraphicsItemSet)
{
    setPrefWidth( 500 );
    setPrefHeight( 250 );
    reSize.notify( mCB(this,uiStratLayerModelDisp,reDraw) );
    reDrawNeeded.notify( mCB(this,uiStratLayerModelDisp,reDraw) );

    uiBorder border( 5 );
    uiAxisHandler::Setup xahsu( uiRect::Top );
    xahsu.border( border ).name( "Model number" ).nogridline( true )
			    .maxnumberdigitsprecision( 0 );
    xax_ = new uiAxisHandler( &scene(), xahsu );

    uiAxisHandler::Setup yahsu( uiRect::Left );
    yahsu.border( border ).name( "Depth" );
    yax_ = new uiAxisHandler( &scene(), yahsu );
    yax_->setBegin( xax_ );
    xax_->setBegin( yax_ );

    getMouseEventHandler().buttonReleased.notify(
	    			mCB(this,uiStratLayerModelDisp,usrClickCB) );
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
    eraseAll();
    delete &logblckitms_;
}


void uiStratLayerModelDisp::getDispProperties( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<lm_.propertyRefs().size(); idx++ )
	nms.add( lm_.propertyRefs()[idx]->name() );
}


void uiStratLayerModelDisp::eraseAll()
{
    logblckitms_.erase();
    delete emptyitm_; emptyitm_ = 0;
}


void uiStratLayerModelDisp::usrClickCB( CallBacker* cb )
{
}


void uiStratLayerModelDisp::reDraw( CallBacker* )
{
    eraseAll(); lm_.prepareUse();

    if ( lm_.isEmpty() )
    {
	emptyitm_ = scene().addItem( new uiTextItem( "<---empty--->",
				     mAlignment(HCenter,VCenter) ) );
	emptyitm_->setPenColor( Color::Black() );
	emptyitm_->setPos( uiPoint( width()/2, height() / 2 ) );
    }
    else
    {
	delete emptyitm_; emptyitm_ = 0;
	doDraw();
    }
}

#define mStartLayLoop \
    const int nrmods = lm_.size(); \
    float prevval = mUdf(float); \
    for ( int imod=0; imod<nrmods; imod++ ) \
    { \
	const Strat::LayerSequence& seq = lm_.sequence( imod ); \
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
    mStartLayLoop
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


void uiStratLayerModelDisp::doDraw()
{
    getBounds();
    xax_->setNewDevSize( width(), height() );
    yax_->setNewDevSize( width(), height() );
    xax_->setBounds(Interval<float>(0,lm_.size()));
    yax_->setBounds(Interval<float>(zrg_.stop,zrg_.start));
    xax_->plotAxis(); yax_->plotAxis();
    const Color vcol( lm_.propertyRefs()[dispprop_]->disp_.color_ );
    const float vwdth = vrg_.width();

	    // z0 z1 val imod ilay
    mStartLayLoop

	if ( mIsUdf(val) ) continue;
	const int ypix0 = yax_->getPix( z0 );
	const int ypix1 = yax_->getPix( z1 );
	const float relx = (val-vrg_.start) / vwdth;
	const int xpix = xax_->getPix( imod + relx );
	if ( !mIsUdf(prevval) )
	{
	    const float prevrelx = (prevval-vrg_.start) / vwdth;
	    const int prevxpix = xax_->getPix( imod + relx );
	    uiLineItem* it = new uiLineItem( prevxpix, ypix0, xpix, ypix0 );
	    it->setPenColor( vcol );
	    logblckitms_ += scene().addItem( it );
	}
	uiLineItem* it = new uiLineItem( xpix, ypix0, xpix, ypix1 );
	it->setPenColor( vcol );
	logblckitms_ += scene().addItem( it );

    mEndLayLoop(;)
}
