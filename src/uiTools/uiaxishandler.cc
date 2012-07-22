/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiaxishandler.cc,v 1.68 2012-07-22 05:06:04 cvskris Exp $";

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uifont.h"
#include "draw.h"
#include "linear.h"
#include "axislayout.h"

#include <math.h>
#include <stdio.h>

static const float logof2 = logf(2);

#define mDefInitList \
      gridlineitmgrp_(0) \
    , axislineitm_(0) \
    , endannottextitm_(0) \
    , annottxtitmgrp_(0) \
    , nameitm_(0) \
    , annotlineitmgrp_(0)

uiAxisHandler::uiAxisHandler( uiGraphicsScene* scene,
			      const uiAxisHandler::Setup& su )
    : NamedObject(su.name_)
    , mDefInitList  
    , scene_(scene)
    , setup_(su)
    , height_(su.height_)
    , width_(su.width_)
    , ticsz_(su.ticsz_)
    , beghndlr_(0)
    , endhndlr_(0)
    , ynmtxtvertical_(false)
{
    setRange( StepInterval<float>(0,1,1) );
}


uiAxisHandler::~uiAxisHandler()
{
    if ( nameitm_ ) delete scene_->removeItem( nameitm_ );
    if ( axislineitm_ ) delete scene_->removeItem( axislineitm_ );
    if ( endannottextitm_ ) delete scene_->removeItem( endannottextitm_ );
    if ( annottxtitmgrp_ ) delete scene_->removeItem( annottxtitmgrp_ );
    if ( annotlineitmgrp_ ) delete scene_->removeItem( annotlineitmgrp_ );
    if ( gridlineitmgrp_ ) delete scene_->removeItem( gridlineitmgrp_ );
}


void uiAxisHandler::setName( const char* nm )
{
    NamedObject::setName( nm );
    reCalc();
}


void uiAxisHandler::setRange( const StepInterval<float>& rg, float* astart )
{
    rg_ = rg;
    annotstart_ = astart ? *astart : rg_.start;

    float fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps < 0 )
	rg_.step = -rg_.step;
    if ( mIsZero(fsteps,1e-6) )
	{ rg_.start -= rg_.step * 1.5; rg_.stop += rg_.step * 1.5; }
    fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps > 50 )
    	rg_.step /= (fsteps / 50);

    rgisrev_ = rg_.start > rg_.stop;
    rgwidth_ = rg_.width();

    reCalc();
}


void uiAxisHandler::setBounds( Interval<float> rg )
{
    const bool isrev = rg.start > rg.stop;
    AxisLayout<float> al( rg );
    if ( (!isrev && (al.sd_.start < rg.start))
      || ( isrev && (al.sd_.start > rg.start)) )
	al.sd_.start += al.sd_.step;
    setRange( StepInterval<float>(rg.start,rg.stop,al.sd_.step), &al.sd_.start );
}


void uiAxisHandler::reCalc()
{
    pos_.erase(); strs_.erase();
    calcwdth_ = 0;

    StepInterval<float> annotrg( rg_ );
    annotrg.start = annotstart_;

    const uiFont& font = FontList().get();
    const bool allocspaceforname = !setup_.noaxisannot_
				&& !setup_.annotinside_
				&& !name().isEmpty();
    if ( allocspaceforname )
	calcwdth_ += font.height();

    int rgwdth = 0;
    const int nrsteps = annotrg.nrSteps();
    for ( int idx=0; idx<=nrsteps; idx++ )
    {
	float pos = annotrg.start + idx * rg_.step;
	if ( mIsZero( pos, setup_.epsaroundzero_ ) ) 
	    pos = 0;
	BufferString str;
	if ( setup_.maxnumberdigitsprecision_ )
	    sprintf(str.buf(),"%.*g",setup_.maxnumberdigitsprecision_,pos);
	else
	    str = pos; 
	strs_.add( str );
	float relpos = pos - rg_.start;
	if ( rgisrev_ ) relpos = -relpos;
	relpos /= rgwidth_;
	if ( setup_.islog_ )
	    relpos = log( 1 + relpos );
	pos_ += relpos;
	const int wdth = font.width( str );
	    if ( idx == 0 )			rgwdth = font.width( str );
	    else if ( rgwdth < wdth )		rgwdth = wdth;
    }
    if ( !setup_.noaxisannot_ )
	calcwdth_ += isHor() ? font.height() : rgwdth;

    endpos_ = setup_.islog_ ? logof2 : 1;
    newDevSize();
}


void uiAxisHandler::newDevSize()
{
    devsz_ = isHor() ? width_ : height_;
    axsz_ = devsz_ - pixBefore() - pixAfter();
}


void uiAxisHandler::updateDevSize()
{
    setNewDevSize( (int)(isHor() ? scene_->width() : scene_->height()),
	    	   (int)(isHor() ? scene_->height() : scene_->width() ));
}


void uiAxisHandler::setNewDevSize( int devsz, int otherdim )
{
    devsz_ = devsz;
    axsz_ = devsz_ - pixBefore() - pixAfter();
    ( isHor() ? width_ : height_ ) = devsz_;
    ( isHor() ? height_ : width_ ) = otherdim ;
}


float uiAxisHandler::getVal( int pix ) const
{
    float relpix;
    if ( isHor() )
	{ pix -= pixBefore(); relpix = pix; }
    else
	{ pix -= pixAfter(); relpix = axsz_-pix; }
    relpix /= axsz_;

    if ( setup_.islog_ )
	relpix = expf( relpix * logof2 );

    return rg_.start + (rgisrev_?-1:1) * rgwidth_ * relpix;
}


float uiAxisHandler::getRelPos( float v ) const
{
    float relv = rgisrev_ ? rg_.start - v : v - rg_.start;
    if ( !setup_.islog_ )
	return relv / rgwidth_;

    if ( relv < -0.9 ) relv = -0.9;
    return log( relv + 1 ) / logof2;
}


int uiAxisHandler::getRelPosPix( float relpos ) const
{
    return isHor() ? (int)(pixBefore() + axsz_ * relpos + .5)
		   : (int)(pixAfter() + axsz_ * (1 - relpos) + .5);
}


int uiAxisHandler::getPix( float pos ) const
{
    return getRelPosPix( getRelPos(pos) );
}


int uiAxisHandler::pixToEdge( bool withborder ) const
{
    int ret = withborder ? setup_.border_.get(setup_.side_) : 0;
    if ( setup_.noaxisannot_ || setup_.annotinside_ ) return ret;

    ret += ticSz() + calcwdth_;
    return ret;
}


int uiAxisHandler::pixBefore() const
{
    if ( beghndlr_ ) return beghndlr_->pixToEdge();
    return setup_.border_.get( isHor() ? uiRect::Left : uiRect::Bottom );
}


int uiAxisHandler::pixAfter() const
{
    if ( endhndlr_ ) return endhndlr_->pixToEdge();
    return setup_.border_.get( isHor() ? uiRect::Right : uiRect::Top );
}


Interval<int> uiAxisHandler::pixRange() const
{
    if ( isHor() )
	return Interval<int>( pixBefore(), devsz_ - pixAfter() );
    else
	return Interval<int>( pixAfter(), devsz_ - pixBefore() );
}


void uiAxisHandler::createAnnotItems()
{
    if ( !annottxtitmgrp_ && setup_.noaxisannot_ ) return;
    
    if ( !annottxtitmgrp_ && !setup_.noaxisannot_ )
    {
	annottxtitmgrp_ = new uiGraphicsItemGroup();
	scene_->addItemGrp( annottxtitmgrp_ );
	annottxtitmgrp_->setZValue( setup_.zval_ );
    }
    else if ( annottxtitmgrp_ )
	annottxtitmgrp_->removeAll( true );
    if ( !annotlineitmgrp_ && !setup_.noaxisannot_ )
    {
	annotlineitmgrp_ = new uiGraphicsItemGroup();
	scene_->addItemGrp( annotlineitmgrp_ );
	annotlineitmgrp_->setZValue( setup_.zval_ );
    }
    else if ( annotlineitmgrp_ )
	annotlineitmgrp_->removeAll( true );
    
    if ( setup_.noaxisannot_ ) return;
    for ( int idx=0; idx<pos_.size(); idx++ )
    {
	const float relpos = pos_[idx] / endpos_;
	annotPos( getRelPosPix(relpos), strs_.get(idx), setup_.style_ );
    }
}


void uiAxisHandler::createGridLines()
{
    if ( setup_.style_.isVisible() )
    {
	if ( !gridlineitmgrp_ && !setup_.nogridline_ )
	{
	    gridlineitmgrp_ = new uiGraphicsItemGroup();
	    scene_->addItemGrp( gridlineitmgrp_ );
	    gridlineitmgrp_->setZValue( setup_.zval_ );
	}
	else if ( gridlineitmgrp_ )
	    gridlineitmgrp_->removeAll( true );

	if ( setup_.nogridline_ && setup_.noaxisannot_ ) return;
	
	Interval<int> toplot( 0, pos_.size()-1 );
	for ( int idx=0; idx<pos_.size(); idx++ )
	{
	    const float relpos = pos_[idx] / endpos_;
	    if ( relpos>0.01 && relpos<1.01 && (!endhndlr_ || relpos<0.99) )
		drawGridLine( getRelPosPix(relpos) );
	}
    }
    else if ( gridlineitmgrp_ )
    {
	gridlineitmgrp_->removeAll( true );
    }
}


void uiAxisHandler::plotAxis()
{
    drawAxisLine();

    if ( setup_.nogridline_ ) 
    {
	if ( gridlineitmgrp_ )
	    gridlineitmgrp_->removeAll( true );
    }
    
    createAnnotItems();  

    if ( !setup_.noaxisannot_ && !name().isEmpty() )
	drawName();

    if ( setup_.noaxisannot_ ) return;
    createGridLines();
}


void uiAxisHandler::drawAxisLine()
{
    if ( setup_.noaxisline_ )
    {
	if( axislineitm_ )
	    axislineitm_->setVisible(false );
	return;
    }
    LineStyle ls( setup_.style_ );
    ls.type_ = LineStyle::Solid;

    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	const int startpix = pixBefore();
	const int endpix = devsz_-pixAfter();
	const int pixpos = setup_.side_ == uiRect::Top
	    		 ? edgepix : height_ - edgepix;
	if ( !axislineitm_ )
	    axislineitm_ = scene_->addItem(
		    new uiLineItem(startpix,pixpos,endpix,pixpos,true) );
	else
	    axislineitm_->setLine( startpix, pixpos, endpix, pixpos, true );
	axislineitm_->setPenStyle( ls );
	axislineitm_->setZValue( setup_.zval_ );
    }
    else
    {
	const int startpix = pixAfter();
	const int endpix = devsz_ - pixBefore();
	const int pixpos = setup_.side_ == uiRect::Left
	    		 ? edgepix : width_ - edgepix;

	if ( !axislineitm_ )
	    axislineitm_ = scene_->addItem(
		    new uiLineItem(pixpos,startpix,pixpos,endpix,true) );
	else
	    axislineitm_->setLine( pixpos, startpix, pixpos, endpix, true );
	axislineitm_->setPenStyle( ls );
	axislineitm_->setZValue( setup_.zval_ );
    }
}


void drawLine( uiLineItem& lineitm, const LinePars& lp,
	       const uiAxisHandler& xah, const uiAxisHandler& yah,
	       const Interval<float>* extxvalrg )
{
    if ( !&xah || !&yah || !&lineitm || !extxvalrg )
	return;
    const Interval<int> ypixrg( yah.pixRange() );
    const Interval<float> yvalrg( yah.getVal(ypixrg.start),
	    			  yah.getVal(ypixrg.stop) );
    Interval<int> xpixrg( xah.pixRange() );
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    if ( extxvalrg )
    {
	xvalrg = *extxvalrg;
	xpixrg.start = xah.getPix( xvalrg.start );
	xpixrg.stop = xah.getPix( xvalrg.stop );
	xpixrg.sort();
	xvalrg.start = xah.getVal(xpixrg.start);
	xvalrg.stop = xah.getVal(xpixrg.stop);
    }

    uiPoint from(xpixrg.start,ypixrg.start), to(xpixrg.stop,ypixrg.stop);
    if ( lp.ax == 0 )
    {
	const int ypix = yah.getPix( lp.a0 );
	if ( !ypixrg.includes( ypix,true ) ) return;
	from.x = xpixrg.start; to.x = xpixrg.stop;
	from.y = to.y = ypix;
    }
    else
    {
	const float xx0 = xvalrg.start; const float yx0 = lp.getValue( xx0 );
 	const float xx1 = xvalrg.stop; const float yx1 = lp.getValue( xx1 );
	const float yy0 = yvalrg.start; const float xy0 = lp.getXValue( yy0 );
 	const float yy1 = yvalrg.stop; const float xy1 = lp.getXValue( yy1 );
	const bool yx0ok = yvalrg.includes( yx0,true );
	const bool yx1ok = yvalrg.includes( yx1,true );
	const bool xy0ok = xvalrg.includes( xy0,true );
	const bool xy1ok = xvalrg.includes( xy1,true );

	if ( !yx0ok && !yx1ok && !xy0ok && !xy1ok )
	    return;

	if ( yx0ok )
	{
	    from.x = xah.getPix( xx0 ); from.y = yah.getPix( yx0 );
	    if ( yx1ok )
		{ to.x = xah.getPix( xx1 ); to.y = yah.getPix( yx1 ); }
	    else if ( xy0ok )
		{ to.x = xah.getPix( xy0 ); to.y = yah.getPix( yy0 ); }
	    else if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
	else if ( yx1ok )
	{
	    from.x = xah.getPix( xx1 ); from.y = yah.getPix( yx1 );
	    if ( xy0ok )
		{ to.x = xah.getPix( xy0 ); to.y = yah.getPix( yy0 ); }
	    else if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
	else if ( xy0ok )
	{
	    from.x = xah.getPix( xy0 ); from.y = yah.getPix( yy0 );
	    if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
    }

    lineitm.setLine( from, to, true );
}


void uiAxisHandler::annotAtEnd( const char* txt )
{
    const int edgepix = pixToEdge();
    int xpix, ypix; Alignment al;
    if ( isHor() )
    {
	xpix = devsz_ - pixAfter() - 2;
	ypix = setup_.side_ == uiRect::Top ? edgepix  : height_ - edgepix - 2;
	al.set( Alignment::Left,
		setup_.side_==uiRect::Top ? Alignment::Bottom : Alignment::Top);
    }
    else
    {
	xpix = setup_.side_ == uiRect::Left  ? edgepix + 5 
	    				     : width_ - edgepix - 5;
	ypix = pixBefore() + 5;
	al.set( setup_.side_==uiRect::Left ? Alignment::Left : Alignment::Right,
		Alignment::VCenter );
    }


    if ( !endannottextitm_ )
	endannottextitm_ = scene_->addItem( new uiTextItem(txt,al) );
    else
	endannottextitm_->setText( txt );
    endannottextitm_->setPos( uiPoint(xpix,ypix) );
    endannottextitm_->setZValue( setup_.zval_ );
}


void uiAxisHandler::annotPos( int pix, const char* txt, const LineStyle& ls )
{
    if ( setup_.noaxisannot_ || setup_.noannotpos_ ) return;
    const int edgepix = pixToEdge();
    const bool inside = setup_.annotinside_;
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int y0 = istop ? edgepix : height_ - edgepix; 
	const int y1 = istop ? ( inside ? y0+ticSz()+calcwdth_ : y0-ticSz() ) 
			     : ( inside ? y0-ticSz()-calcwdth_ : y0+ticSz() ); 

	uiLineItem* annotposlineitm = new uiLineItem();
	annotposlineitm->setLine( pix, y0, pix, y1 );
	annotposlineitm->setPenColor( ls.color_ );
	annotlineitmgrp_->add( annotposlineitm );
	Alignment al( Alignment::HCenter,
		      istop ? Alignment::Bottom : Alignment::Top );
	uiTextItem* annotpostxtitem =
	    new uiTextItem( uiPoint(pix,y1), txt, al );
	annotpostxtitem->setTextColor( ls.color_ );
	annottxtitmgrp_->add( annotpostxtitem );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	const int x0 = isleft ? edgepix : width_ - edgepix;
	const int x1 = isleft ? ( inside ? x0+ticSz()+calcwdth_ : x0-ticSz() )
			      : ( inside ? x0-ticSz()-calcwdth_ : x0+ticSz() );
	uiLineItem* annotposlineitm = new uiLineItem();
	annotposlineitm->setLine( x0, pix, x1, pix );
	annotposlineitm->setPenColor( ls.color_ );
	annotlineitmgrp_->add( annotposlineitm );
	Alignment al( isleft ? Alignment::Right : Alignment::Left,
		      Alignment::VCenter );
	uiTextItem* annotpostxtitem =
	    new uiTextItem( uiPoint(x1,pix), txt, al );
	annotpostxtitem->setTextColor( ls.color_ );
	annottxtitmgrp_->add( annotpostxtitem );
    }
}


void uiAxisHandler::drawGridLine( int pix )
{
    if ( setup_.nogridline_ ) return;
    uiLineItem* lineitem = getFullLine( pix );
    lineitem->setPenStyle( setup_.style_ );
    gridlineitmgrp_->add( lineitem );
    gridlineitmgrp_->setVisible( setup_.style_.isVisible() );
}


uiLineItem* uiAxisHandler::getFullLine( int pix )
{
    const uiAxisHandler* hndlr = beghndlr_ ? beghndlr_ : endhndlr_;
    int endpix = setup_.border_.get( uiRect::across(setup_.side_) );
    if ( hndlr )
	endpix = setup_.side_ == uiRect::Left || setup_.side_ == uiRect::Bottom
	    	? hndlr->pixAfter() : hndlr->pixBefore();
    const int startpix = pixToEdge();

    uiLineItem* lineitem = new uiLineItem();
    switch ( setup_.side_ )
    {
    case uiRect::Top:
	lineitem->setLine( pix, startpix, pix, height_ - endpix );
	break;
    case uiRect::Bottom:
	lineitem->setLine( pix, endpix, pix, height_ - startpix );
	break;
    case uiRect::Left:
	lineitem->setLine( startpix, pix, width_ - endpix, pix );
	break;
    case uiRect::Right:
	lineitem->setLine( endpix, pix, width_ - startpix, pix );
	break;
    }

    return lineitem;
}


void uiAxisHandler::drawName() 
{
    if ( !nameitm_ )
	nameitm_ = scene_->addItem( new uiTextItem(name()) );
    else
	nameitm_->setText( name() );

    Alignment al( Alignment::HCenter, Alignment::VCenter );
    float namepos = pixToEdge() - ticSz() - calcwdth_;
    uiPoint pt;
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	pt.x = pixBefore() + axsz_ / 2;
	pt.y = (int) (istop ? namepos : height_ - namepos);
	al.set( istop ? Alignment::Top : Alignment::Bottom );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	namepos -= FontList().get().height()/2; //shift due to rotation
	pt.x = (int) (isleft ? namepos : width_ - namepos);
	pt.y = ( height_+nameitm_->getTextSize().width() ) / 2;
	al.set( isleft ? Alignment::Left : Alignment::Left );

	if ( !ynmtxtvertical_ )
	    nameitm_->setRotation( isleft ? -90 : 90 );
	ynmtxtvertical_ = true;
    }
    nameitm_->setPos( pt );
    nameitm_->setAlignment( al );
    nameitm_->setZValue( setup_.zval_ );
    Color col = setup_.nmcolor_ == Color::NoColor() ? setup_.style_.color_ 
						    : setup_.nmcolor_;
    nameitm_->setTextColor( col );
}


int uiAxisHandler::ticSz() const
{ return setup_.noaxisannot_ ? 0 : ticsz_; }
