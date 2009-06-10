/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogdisplay.cc,v 1.1 2009-06-10 13:21:39 cvsbert Exp $";

#include "uiwelllogdisplay.h"
#include "welllog.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "mouseevent.h"
#include "dataclipper.h"
#include "survinfo.h"
#include <iostream>


uiWellLogDisplay::LogData::LogData( uiGraphicsScene& scn, bool isfirst )
    : wl_(0)
    , xax_(&scn,uiAxisHandler::Setup(isfirst?uiRect::Top:uiRect::Bottom))
    , yax_(&scn,uiAxisHandler::Setup(isfirst?uiRect::Left:uiRect::Right))
    , zrg_(mUdf(float),0)
    , valrg_(mUdf(float),0)
    , logarithmic_(false)
    , linestyle_(LineStyle::Solid)
    , clipratio_(0.001)
{
    if ( isfirst )
	xax_.setBegin( &yax_ );
    else
	xax_.setEnd( &yax_ );

    if ( !isfirst )
	yax_.setup().nogridline(true);

    const uiBorder b( 10 );
    xax_.setup().border( b );
    yax_.setup().border( b );
}


void uiWellLogDisplay::LogData::setSecond( uiWellLogDisplay::LogData& ld )
{
    xax_.setEnd( &ld.yax_ );
    yax_.setBegin( &ld.xax_ );
}


uiWellLogDisplay::uiWellLogDisplay( uiParent* p )
    : uiGraphicsView(p,"Well Log display viewer")
    , ld1_(scene(),true)
    , ld2_(scene(),false)
    , zrg_(mUdf(float),0)
    , dispzinft_(SI().zInFeet())
{
    setStretch( 2, 2 );
    getMouseEventHandler().buttonReleased.notify(
			    mCB(this,uiWellLogDisplay,mouseRelease) );

    reSize.notify( mCB(this,uiWellLogDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    finaliseDone.notify( mCB(this,uiWellLogDisplay,init) );
}


uiWellLogDisplay::~uiWellLogDisplay()
{
}


void uiWellLogDisplay::reSized( CallBacker* )
{
    draw();
}


void uiWellLogDisplay::init( CallBacker* )
{
    dataChanged();
}


void uiWellLogDisplay::dataChanged()
{
    gatherInfo(); draw();
}


void uiWellLogDisplay::gatherInfo()
{
    gatherInfo( true );
    gatherInfo( false );
    ld1_.setSecond( ld2_ );

    if ( mIsUdf(zrg_.start) && ld1_.wl_ )
    {
	zrg_ = ld1_.zrg_;
	if ( ld2_.wl_ )
	    zrg_.include( ld2_.zrg_ );
    }

    ld1_.yax_.setup().islog( ld1_.logarithmic_ );
    ld2_.yax_.setup().islog( ld2_.logarithmic_ );

    if ( ld1_.wl_ ) ld1_.xax_.setup().name( ld1_.wl_->name() );
    if ( ld2_.wl_ ) ld2_.xax_.setup().name( ld1_.wl_->name() );
    BufferString znm( "MD ", dispzinft_ ? "(ft)" : "(m)" );
    ld1_.yax_.setup().name( znm ); ld2_.yax_.setup().name( znm );
}


void uiWellLogDisplay::gatherInfo( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;

    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;

    DataClipSampler dcs( sz );
    dcs.add( ld.wl_->valArr(), sz );
    ld.valrg_ = dcs.getRange( ld.clipratio_ );
    ld.zrg_.start = ld.wl_->dah( 0 );
    ld.zrg_.stop = ld.wl_->dah( sz-1 );
    ld.xax_.setRange( ld.valrg_ );
    Interval<float> dispzrg( zrg_ );
    if ( dispzinft_ )
	dispzrg.scale( mToFeetFactor );
    ld.yax_.setRange( dispzrg );
}


void uiWellLogDisplay::draw()
{
    if ( mIsUdf(zrg_.start) ) return;

    ld1_.xax_.plotAxis(); ld1_.yax_.plotAxis();
    ld2_.xax_.plotAxis(); ld2_.yax_.plotAxis();

    drawCurve( true );
    drawCurve( false );
}


void uiWellLogDisplay::drawCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = first ? ld1_ : ld2_;
    delete ld.curveitm_; ld.curveitm_ = 0;
    delete ld.curvenmitm_; ld.curvenmitm_ = 0;
    const int sz = ld.wl_ ? ld.wl_->size() : 0;
    if ( sz < 2 ) return;

    TypeSet<uiPoint> pts;
    for ( int idx=0; idx<sz; idx++ )
    {
	float dah = ld.wl_->dah( idx );
	if ( !zrg_.includes(dah) )
	    continue;

	if ( dispzinft_ ) dah *= mToFeetFactor;
	const float val = ld.wl_->value( idx );
	pts += uiPoint( ld.xax_.getPix(val), ld.yax_.getPix(dah) );
    }
    if ( pts.isEmpty() ) return;

    ld.curveitm_ = scene().addItem( new uiPolyLineItem(pts) );
    ld.curveitm_->setPenColor( ld.color_ );
    Alignment al( Alignment::HCenter,
	    	  first ? Alignment::Top : Alignment::Bottom );
    ld.curvenmitm_ = scene().addItem( new uiTextItem(ld.wl_->name(),al) );
    ld.curvenmitm_->setTextColor( ld.color_ );
    ld.curvenmitm_->setPos( first ? pts[0] : pts[pts.size()-1] );
}


void uiWellLogDisplay::mouseRelease( CallBacker* )
{
}
