/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Duntao Wei
 Date:          Mar. 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uigraphicssceneaxismgr.h"

#include "draw.h"
#include "axislayout.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uiworld2ui.h"

#define mRemoveGraphicsItem( item ) \
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
    , annotinint_( false )
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

    reDraw();
}


void uiGraphicsSceneAxis::enableMask( bool yn )
{
    if ( yn==(bool) mask_ )
	return;

    if ( yn )
    {
	mask_ = new uiRectItem;
	itmgrp_->add( mask_ );
	mask_->setFillColor( Color::White() );
	OD::LineStyle lst; lst.type_ = OD::LineStyle::None;

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
    reDraw();
}


void uiGraphicsSceneAxis::setWorldCoords( const Interval<double>& rg )
{
    rg_ = rg;
    reDraw();
}


void uiGraphicsSceneAxis::setFontData( const FontData& fnt )
{
    fontdata_ = fnt;
    reDraw();
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

#define mDefNrDecimalPlaces 3

int uiGraphicsSceneAxis::getNrAnnotChars() const
{
    const int widthlogval = mIsZero(rg_.width(),mDefEps)
				? 0 : mNINT32( Math::Log10(fabs(rg_.width())) );
    const int startlogval = mIsZero(rg_.start,mDefEps)
				? 0 : mNINT32( Math::Log10(fabs(rg_.start)) );
    const int stoplogval = mIsZero(rg_.stop,mDefEps)
				? 0 : mNINT32( Math::Log10(fabs(rg_.stop)) );
    int nrofpredecimalchars = mMAX(stoplogval,startlogval) + 1;
    if ( nrofpredecimalchars < 1 )
	nrofpredecimalchars = 1;
    int nrofpostdecimalchars = mDefNrDecimalPlaces - widthlogval;
    if ( annotinint_ || nrofpostdecimalchars < 0 )
	nrofpostdecimalchars = 0;
    return nrofpredecimalchars + nrofpostdecimalchars + 1;
}


void uiGraphicsSceneAxis::drawAtPos( float worldpos, bool drawgrid,
				     int& curtextitm, int& curlineitm )
{
    Interval<int> axisrg( isx_ ? viewrect_.left() : viewrect_.top(),
			  isx_ ? viewrect_.right() : viewrect_.bottom() );
    Interval<int> datarg( isx_ ? viewrect_.top() : viewrect_.left(),
			  isx_ ? viewrect_.bottom() : viewrect_.right() );
    const int ticklen = fontdata_.pointSize();
    const int baseline = istop_ ? datarg.start : datarg.stop;
    const int bias = inside_==istop_ ? ticklen : -ticklen;
    const int ticklinestart = baseline + bias;
    const int ticklinestop = baseline;

    uiString txt = mToUiStringTodo( toStringLim(worldpos * txtfactor_,
							   getNrAnnotChars()) );
    const double worldrelpos = fabs(rg_.getfIndex( worldpos, rg_.width() ));
    float axispos = (float) ( axisrg.start + worldrelpos*axisrg.width() );

    mGetItem( uiLineItem, line, tickline );

    Geom::Point2D<float> tickstart( axispos, mCast(float,ticklinestart) );
    Geom::Point2D<float> tickstop( axispos, mCast(float,ticklinestop) );

    if ( !isx_ )
    {
	tickstart.swapXY();
	tickstop.swapXY();
    }

    tickline->setLine( tickstart, tickstop );
    tickline->setPenStyle( ls_ );

    if ( drawgridlines_ && drawgrid )
    {
	mGetItem( uiLineItem, line, gridline );
	Geom::Point2D<float> gridstart(axispos, mCast(float,datarg.start));
	Geom::Point2D<float> gridstop( axispos, mCast(float,datarg.stop) );

	if ( !isx_ )
	{
	    gridstart.swapXY();
	    gridstop.swapXY();
	}

	gridline->setLine( gridstart, gridstop );
	gridline->setPenStyle( gridls_ );
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

    mGetItem( uiTextItem, text, label );
    label->setAlignment( al );
    label->setText( txt );
    label->setFontData( fontdata_ );
    label->setPos( tickstart );
    label->setTextColor( ls_.color_ );
}


void uiGraphicsSceneAxis::reDraw()
{
    AxisLayout<double> al( Interval<double>(rg_.start ,rg_.stop), annotinint_ );
    SamplingData<double> axis = al.sd_;
    Interval<int> axisrg( isx_ ? viewrect_.left() : viewrect_.top(),
				isx_ ? viewrect_.right() : viewrect_.bottom() );
    Interval<int> datarg( isx_ ? viewrect_.top() : viewrect_.left(),
			       isx_ ? viewrect_.bottom() : viewrect_.right() );
    const int baseline = istop_ ? datarg.start : datarg.stop;
    int curtextitm = 0;
    int curlineitm = 0;
    if ( drawaxisline_ )
    {
	mGetItem( uiLineItem, line, line );

	uiPoint start( axisrg.start, baseline );
	uiPoint stop( axisrg.stop, baseline );
	if ( !isx_ )
	{
	    start.swapXY();
	    stop.swapXY();
	}

	line->setLine( start, stop );
	line->setPenStyle( ls_ );
    }

    const float fnrsteps = (float) ( rg_.width(false)/axis.step );
    const int nrsteps = mNINT32( fnrsteps )+2;
    if ( !mIsEqual(rg_.start,axis.start,axis.step/100.f) &&
	 (!annotinint_ || mIsEqual(rg_.start,mNINT32(rg_.start),1e-4)) )
	drawAtPos( mCast(float,rg_.start), false, curtextitm, curlineitm );
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	const double worldpos = axis.atIndex(idx);
	if ( !rg_.includes(worldpos,true) )
	    continue;
	drawAtPos( mCast(float,worldpos), true, curtextitm, curlineitm );
    }

    if ( !mIsEqual(rg_.stop,axis.atIndex(nrsteps-1),axis.step/100.f) &&
	 (!annotinint_ || mIsEqual(rg_.stop,mNINT32(rg_.stop),1e-4)) )
	drawAtPos( mCast(float,rg_.stop), false, curtextitm, curlineitm );
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
    , xaxis_( new uiAxisHandler(&view.scene(),
				uiAxisHandler::Setup(uiRect::Top)
				.fixedborder(true).nogridline(true).ticsz(5)) )
    , yaxis_( new uiAxisHandler(&view.scene(),
				uiAxisHandler::Setup(uiRect::Left)
				.fixedborder(true).ticsz(5)) )
    , uifont_( uiFontList::getInst().get(FontData::key(FontData::Graphics2D)) )
{
    xaxis_->setBegin( yaxis_ );
    yaxis_->setEnd( xaxis_ );

    updateFontSizeCB( 0 );
    OD::LineStyle lst; lst.type_ = OD::LineStyle::None;

    mAddMask( topmask_ );
    mAddMask( bottommask_ );
    mAddMask( leftmask_ );
    mAddMask( rightmask_ );
    mAttachCB( uifont_.changed, uiGraphicsSceneAxisMgr::updateFontSizeCB );
}


uiGraphicsSceneAxisMgr::~uiGraphicsSceneAxisMgr()
{
    detachAllNotifiers();
    delete xaxis_;
    delete yaxis_;
}


void uiGraphicsSceneAxisMgr::setZValue( int z )
{
    xaxis_->setup().zval( z+1 );
    yaxis_->setup().zval( z+1 );

    topmask_->setZValue( z );
    bottommask_->setZValue( z );
    leftmask_->setZValue( z );
    rightmask_->setZValue( z );
}


void uiGraphicsSceneAxisMgr::setViewRect( const uiRect& viewrect )
{
    xaxis_->updateDevSize();
    yaxis_->updateDevSize();

    const uiRect fullrect = view_.getViewArea();

    topmask_->setRect( fullrect.left(), fullrect.top(),
		      fullrect.width(), viewrect.top()-fullrect.top() );
    bottommask_->setRect( fullrect.left(), viewrect.bottom(),
			 fullrect.width(),
			 fullrect.bottom()-viewrect.bottom()+1);
    leftmask_->setRect( fullrect.left(), fullrect.top(),
		       viewrect.left()-fullrect.left(), fullrect.height() );
    rightmask_->setRect( viewrect.right(), fullrect.top(),
			fullrect.right()-viewrect.right(), fullrect.height());
}


void uiGraphicsSceneAxisMgr::setBorder( const uiBorder& border )
{
    xaxis_->setBorder( border );
    yaxis_->setBorder( border );
}


void uiGraphicsSceneAxisMgr::setWorldCoords( const uiWorldRect& wr )
{
    xaxis_->setBounds( Interval<float>(mCast(float,wr.left()),
				       mCast(float,wr.right())) );
    yaxis_->setBounds( Interval<float>( mCast(float,wr.bottom()),
					mCast(float,wr.top()) ) );
    updateScene();
}


void uiGraphicsSceneAxisMgr::setXLineStyle( const OD::LineStyle& xls )
{
    xaxis_->setup().style_ = xls;
}


void uiGraphicsSceneAxisMgr::setYLineStyle( const OD::LineStyle& yls )
{
    yaxis_->setup().style_ = yls;
}


int uiGraphicsSceneAxisMgr::getZValue() const
{
    return topmask_->getZValue();
}


int uiGraphicsSceneAxisMgr::getNeededWidth() const
{
    const int nrchars = yaxis_->getNrAnnotCharsForDisp() + 5;
    return nrchars*uifont_.avgWidth();
}


int uiGraphicsSceneAxisMgr::getNeededHeight() const
{
    return uifont_.height()+2;
}


NotifierAccess& uiGraphicsSceneAxisMgr::layoutChanged()
{ return uifont_.changed; }


void uiGraphicsSceneAxisMgr::updateFontSizeCB( CallBacker* )
{
    const FontData& fontdata = uifont_.fontData();
    xaxis_->setup().fontdata_ = fontdata;
    yaxis_->setup().fontdata_ = fontdata;
}


void uiGraphicsSceneAxisMgr::setGridLineStyle( const OD::LineStyle& gls )
{
    xaxis_->setup().gridlinestyle_ = gls;
    yaxis_->setup().gridlinestyle_ = gls;
}


void uiGraphicsSceneAxisMgr::setAnnotInside( bool yn )
{
    xaxis_->setup().annotinside_ = yn;
    yaxis_->setup().annotinside_ = yn;
}


void uiGraphicsSceneAxisMgr::enableAxisLine( bool yn )
{
    xaxis_->setup().noaxisline( !yn );
    yaxis_->setup().noaxisline( !yn );
}


void uiGraphicsSceneAxisMgr::setAuxAnnotPositions(
	const TypeSet<PlotAnnotation>& auxannot, bool forx )
{
    if ( forx )
	xaxis_->setAuxAnnot( auxannot );
    else
	yaxis_->setAuxAnnot( auxannot );
}


void uiGraphicsSceneAxisMgr::setAuxLineStyle( const OD::LineStyle& ls,
					      bool forx, bool forhl )
{
    uiAxisHandler* axis = forx ? xaxis_ : yaxis_;
    if ( forhl )
	axis->setup().auxhllinestyle_ = ls;
    else
	axis->setup().auxlinestyle_ = ls;
}


void uiGraphicsSceneAxisMgr::showAuxPositions( bool forx, bool yn )
{
    if ( forx )
	xaxis_->setup().showauxannot( yn );
    else
	yaxis_->setup().showauxannot( yn );
}
