/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Duntao Wei
 Date:          Mar. 2005
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: drawaxis2d.cc,v 1.40 2012-07-13 09:39:42 cvsbruno Exp $";

#include "drawaxis2d.h"

#include "draw.h"
#include "axislayout.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uiworld2ui.h"

# define mRemoveGraphicsItem( item ) \
if ( item ) \
{ scene_.removeItem( item ); delete item; item = 0; }

#define mMaskZ	0
#define mAnnotZ	100


uiGraphicsSceneAxis::uiGraphicsSceneAxis( uiGraphicsScene& scene)
    : scene_( scene )
    , itmgrp_( 0 )
    , txtfactor_( 1 )
    , drawaxisline_( true )
    , drawgridlines_( true )
    , mask_( 0 )
{
    itmgrp_ = new uiGraphicsItemGroup;
    scene_.addItem( itmgrp_ );
}


uiGraphicsSceneAxis::~uiGraphicsSceneAxis()
{
    mRemoveGraphicsItem( itmgrp_ );
}


void uiGraphicsSceneAxis::setZValue( int zvalue )
{
    itmgrp_->setZValue( zvalue );
}


void uiGraphicsSceneAxis::setPosition(bool isx,bool istoporleft,bool isinside)
{
    inside_ = isinside;
    isx_ = isx;
    istop_ = istoporleft;
    
    update();
}


void uiGraphicsSceneAxis::drawMask( bool yn )
{
    if ( yn==(bool) mask_ )
	return;
    
    if ( yn )
    {
	mask_ = new uiRectItem;
	itmgrp_->add( mask_ );
	mask_->setFillColor( Color::White() );
	LineStyle lst; lst.type_ = LineStyle::None;
	
	mask_->setPenStyle( lst );
	mask_->setZValue( mMaskZ );
    }
    else
    {
	itmgrp_->remove( mask_, true );
	mask_ = 0;
    }
}


void uiGraphicsSceneAxis::setViewRect( const uiRect& uir )
{
    viewrect_ = uir;
    update();
}


void uiGraphicsSceneAxis::setWorldCoords( const Interval<double>& rg )
{
    rg_ = rg;
    update();
}


void uiGraphicsSceneAxis::setFontData( const FontData& fnt )
{
    fontdata_ = fnt;
    update();
}


void uiGraphicsSceneAxis::turnOn( bool yn )
{
    if ( itmgrp_ ) itmgrp_->setVisible( yn );
}

    
#define mGetItem( type, nm, var ) \
type* var; \
if ( nm##s_.validIdx( cur##nm##itm ) ) \
    var = nm##s_[cur##nm##itm]; \
else \
{ \
    var = new type; \
    itmgrp_->add( var ); \
    nm##s_ += var; \
} \
cur##nm##itm++


void uiGraphicsSceneAxis::update()
{
    SamplingData<double> axis =
    	AxisLayout<double>( Interval<double>(rg_.start ,rg_.stop) ).sd_;
    
    
    const Interval<int> axisrg( isx_ ? viewrect_.left() : viewrect_.top(),
			 	isx_ ? viewrect_.right() : viewrect_.bottom() );
    const Interval<int> datarg( isx_ ? viewrect_.top() : viewrect_.left(),
			       isx_ ? viewrect_.bottom() : viewrect_.right() );
    
    
    const int ticklen = fontdata_.pointSize();
    
    const int baseline = istop_ ? datarg.start : datarg.stop;
    const int bias = inside_==istop_ ? ticklen : -ticklen;
    
    const int ticklinestart = baseline + bias;
    const int ticklinestop = baseline;
    
    int curtextitm = 0;
    int curlineitm = 0;
    
     
    if ( drawaxisline_ )
    {
	mGetItem( uiLineItem, line, line );
		
	Geom::Point2D<float> start( axisrg.start, baseline );
	Geom::Point2D<float> stop( axisrg.stop, baseline );
	
	if ( !isx_ )
	{
	    start.swapXY();
	    stop.swapXY();
	}
	
	line->setLine( start, stop, false );
	line->setPenStyle( ls_ );
	
    }
    
    Alignment al;
    if ( isx_ )
    {
	al.set( Alignment::HCenter );
	al.set( bias<0 ? Alignment::Bottom : Alignment::Top );
    }
    else
    {
	al.set( Alignment::VCenter );
	al.set( bias<0 ? Alignment::Right : Alignment::Left );
    }
    
    BufferString txt;
    const float fnrsteps = rg_.width(false)/axis.step;
    const int nrsteps = mNINT32( fnrsteps )+1;
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	const double worldpos = axis.atIndex(idx);
	if ( !rg_.includes(worldpos,true) )
	    continue;
	
	txt = worldpos * txtfactor_;
	const double worldrelpos = rg_.getfIndex( worldpos, rg_.width() );
	float axispos = axisrg.start + worldrelpos*axisrg.width();
	
	mGetItem( uiLineItem, line, tickline );
	
	Geom::Point2D<float> tickstart( axispos, ticklinestart );
	Geom::Point2D<float> tickstop( axispos, ticklinestop );
	
	if ( !isx_ )
	{
	    tickstart.swapXY();
	    tickstop.swapXY();
	}
	
	tickline->setLine( tickstart, tickstop, false );
	tickline->setPenStyle( ls_ );
	
	if ( drawgridlines_ )
	{
	    mGetItem( uiLineItem, line, gridline );
	    Geom::Point2D<float> gridstart( axispos, datarg.start );
	    Geom::Point2D<float> gridstop( axispos, datarg.stop );
	    
	    if ( !isx_ )
	    {
		gridstart.swapXY();
		gridstop.swapXY();
	    }
	    
	    gridline->setLine( gridstart, gridstop );
	    gridline->setPenStyle( gridls_ );
	}
	

	mGetItem( uiTextItem, text, label );
	label->setAlignment( al );
	label->setText( txt );
	label->setFontData( fontdata_ );
	label->setPos( tickstart );
	label->setTextColor( ls_.color_ );
    }
    
    while ( curlineitm<lines_.size() )
	itmgrp_->remove( lines_.pop(), true );
    
    while ( curtextitm<texts_.size() )
	itmgrp_->remove( texts_.pop(), true );
}


#define mAddMask( var ) \
var = new uiRectItem(); \
view_.scene().addItem( var ); \
var->setFillColor( Color::White() ); \
var->setPenStyle( lst )



uiGraphicsSceneAxisMgr::uiGraphicsSceneAxisMgr( uiGraphicsView& view )
    : view_( view )
    , xaxis_( new uiGraphicsSceneAxis( view.scene() ) )
    , yaxis_( new uiGraphicsSceneAxis( view.scene() ) )
{
    xaxis_->setPosition( true, true, false );
    yaxis_->setPosition( false, true, false );
    
    LineStyle lst; lst.type_ = LineStyle::None;

    mAddMask( topmask_ );
    mAddMask( bottommask_ );
    mAddMask( leftmask_ );
    mAddMask( rightmask_ );
}


uiGraphicsSceneAxisMgr::~uiGraphicsSceneAxisMgr()
{
    delete xaxis_; delete yaxis_;
}


void uiGraphicsSceneAxisMgr::setZvalue( int z )
{
    xaxis_->setZValue( z+1 );
    yaxis_->setZValue( z+1 );
    
    topmask_->setZValue( z );
    bottommask_->setZValue( z );
    leftmask_->setZValue( z );
    rightmask_->setZValue( z );
}


void uiGraphicsSceneAxisMgr::setViewRect( const uiRect& viewrect )
{
    xaxis_->setViewRect( viewrect );
    yaxis_->setViewRect( viewrect );
    
    const uiRect fullrect = view_.getViewArea();
    
    topmask_->setRect( fullrect.left(), fullrect.top(),
		      fullrect.width(), viewrect.top()-fullrect.top() );
    bottommask_->setRect( fullrect.left(), viewrect.bottom(),
			 fullrect.width(), fullrect.bottom()-viewrect.bottom()+1);
    leftmask_->setRect( fullrect.left(), fullrect.top(),
		       viewrect.left()-fullrect.left(), fullrect.width() );
    rightmask_->setRect( viewrect.right(), fullrect.top(),
			fullrect.right()-viewrect.right(), fullrect.height());
}


void uiGraphicsSceneAxisMgr::setXFactor( int xf )
{
    xaxis_->setTextFactor( xf );
}


void uiGraphicsSceneAxisMgr::setYFactor( int yf )
{
    yaxis_->setTextFactor( yf );
}


void uiGraphicsSceneAxisMgr::setWorldCoords( const uiWorldRect& wr )
{
    xaxis_->setWorldCoords( Interval<double>(wr.left(),wr.right()) );
    yaxis_->setWorldCoords( Interval<double>( wr.top(), wr.bottom() ) );
}


void uiGraphicsSceneAxisMgr::setXLineStyle( const LineStyle& xls )
{
    xaxis_->setLineStyle( xls );
}


void uiGraphicsSceneAxisMgr::setYLineStyle( const LineStyle& yls )
{
    yaxis_->setLineStyle( yls );
}

void uiGraphicsSceneAxisMgr::setGridLineStyle( const LineStyle& gls )
{
    xaxis_->setGridLineStyle( gls );
    yaxis_->setGridLineStyle( gls );
}
