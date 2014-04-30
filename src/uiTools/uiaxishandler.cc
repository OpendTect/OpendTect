/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uifont.h"
#include "draw.h"
#include "linear.h"
#include "axislayout.h"

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
    : mDefInitList
    , scene_(scene)
    , setup_(su)
    , height_(su.height_)
    , width_(su.width_)
    , ticsz_(su.ticsz_)
    , beghndlr_(0)
    , endhndlr_(0)
    , annotstart_(0)
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


void uiAxisHandler::setCaption( const uiString& caption )
{
    setup_.caption_ = caption;
    reCalc();
}

bool uiAxisHandler::doPlotExtreme( float plottedxtrmval, bool isstart ) const
{
    const float actxtrmval = isstart ? rg_.start : rg_.stop;
    if (setup_.annotinint_ )
	return false;
    const uiFont& uifont = uiFontList::getInst().get(setup_.fontdata_.family());
    const int reqnrchars = getNrAnnotCharsForDisp();
    const int actxtrmvalpospix = getPix( actxtrmval );
    const int plottedxtrmvalpospix = getPix( plottedxtrmval );
    const float halftxtwidth =
	isHor() ? ( (float)reqnrchars/2) * uifont.avgWidth()
		: uifont.height()/2 ;
    const bool ismin = (isstart && !rgisrev_) || (rgisrev_ && !isstart);
    const int fac = ismin ? 1 : -1;
    const int actxtrmvalendpix = actxtrmvalpospix + fac*mNINT32( halftxtwidth );
    const int plottedxtrmvalendpix =
	plottedxtrmvalpospix - fac*mNINT32( halftxtwidth );
    const bool doesannotcoincide =
	ismin ? actxtrmvalendpix > plottedxtrmvalendpix
	      : actxtrmvalendpix < plottedxtrmvalendpix;
    return !doesannotcoincide;
}

void uiAxisHandler::setRange( const StepInterval<float>& rg, float* astart )
{
    rg_ = rg;
    annotstart_ = astart ? *astart : rg_.start;

    float fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps < 0 )
	rg_.step = -rg_.step;
    if ( mIsZero(fsteps,1e-6) )
	{ rg_.start -= rg_.step * 1.5f; rg_.stop += rg_.step * 1.5f; }
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
    AxisLayout<float> al( rg, setup_.annotinint_ );
    if ( (!isrev && (al.sd_.start < rg.start))
      || ( isrev && (al.sd_.start > rg.start)) )
	al.sd_.start += al.sd_.step;
    setRange( StepInterval<float>(rg.start,rg.stop,al.sd_.step),&al.sd_.start);
}

#define sDefNrDecimalPlaces 3

int uiAxisHandler::getNrAnnotCharsForDisp() const
{
    if ( setup_.maxnrchars_ ) return setup_.maxnrchars_;
    const int widthlogval = mIsZero(rg_.width(),mDefEps)
	? 0 : mNINT32( Math::Log10(fabs(rg_.width())) );
    const int startlogval = mIsZero(rg_.start,mDefEps)
	? 0 : mNINT32( Math::Log10(fabs(rg_.start)) );
    const int stoplogval = mIsZero(rg_.stop,mDefEps)
	? 0 : mNINT32( Math::Log10(fabs(rg_.stop)) );
    int nrofpredecimalchars = mMAX(stoplogval,startlogval) + 1;
    // number of chars needed for pre decimal part for maximum value
    if ( nrofpredecimalchars < 1 )
    nrofpredecimalchars = 1;
    int nrofpostdecimalchars = sDefNrDecimalPlaces - widthlogval;
    // number of chars needed for decimal places on the basis of range
    if ( setup_.annotinint_ || nrofpostdecimalchars < 0 )
	nrofpostdecimalchars = 0;
    else
	nrofpostdecimalchars += 1; // +1 for the decimal itself
    const int nrannotchars = nrofpredecimalchars + nrofpostdecimalchars;
    return setup_.maxnrchars_ && nrannotchars>setup_.maxnrchars_
		? setup_.maxnrchars_ : nrannotchars;
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
				&& !setup_.fixedborder_
				&& !setup_.caption_.isEmpty();
    if ( allocspaceforname )
	calcwdth_ += font.height();

    int rgwdth = 0;
    int nrsteps = annotrg.nrSteps();
    if ( !rg_.includes(annotrg.atIndex(nrsteps),false) )
	nrsteps--;
    const int reqnrchars = getNrAnnotCharsForDisp();
    if ( doPlotExtreme(annotstart_,true) )
    {
	pos_ += 0.0f;
	strs_.add( toStringLim( rg_.start,
				rg_.start<0 ? reqnrchars+1 : reqnrchars) );
    }

    for ( int idx=0; idx<=nrsteps; idx++ )
    {
	float pos = annotrg.atIndex( idx );
	if ( !rg_.includes(pos,rgisrev_) )
	    continue;
	if ( mIsZero( pos, setup_.epsaroundzero_ ) )
	    pos = 0;
	const BufferString posstr( toStringLim(pos,pos<0 ? reqnrchars+1
							 : reqnrchars) );
	strs_.add( posstr );
	float relpos = pos - rg_.start;
	if ( rgisrev_ ) relpos = -relpos;
	relpos /= rgwidth_;
	if ( setup_.islog_ )
	    relpos = log( 1 + relpos );
	pos_ += relpos;
	const int wdth = font.width( posstr );
	if ( idx == 0 || rgwdth < wdth )
	    rgwdth = wdth;
    }

    if ( doPlotExtreme(annotrg.atIndex(nrsteps),false) )
    {
	pos_ += 1.0f;
	strs_.add(
		toStringLim(rg_.stop,rg_.stop<0 ? reqnrchars+1 : reqnrchars) );
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
	{ pix -= pixBefore(); relpix = mCast( float, pix ); }
    else
	{ pix -= pixAfter(); relpix = mCast( float, axsz_-pix ); }
    relpix /= axsz_;

    if ( setup_.islog_ )
	relpix = expf( relpix * logof2 );

    return rg_.start + (rgisrev_?-1:1) * rgwidth_ * relpix;
}


float uiAxisHandler::getRelPos( float v ) const
{
    float relv = ( rgisrev_ ? rg_.start - v : v - rg_.start ) / rgwidth_;
    if ( !setup_.islog_ )
	return relv;

    if ( relv < -0.9 ) relv = -0.9;
    return log( relv + 1 ) / logof2;
}


int uiAxisHandler::getRelPosPix( float relpos ) const
{
    return isHor() ? (int)( (rgisrev_ ? pixAfter() : pixBefore()) +
			     axsz_ * relpos + .5)
		   : (int)( (rgisrev_ ? pixBefore() : pixAfter()) +
			     axsz_ * (1 - relpos) + .5);
}


int uiAxisHandler::getPix( float pos ) const
{
    return getRelPosPix( getRelPos(pos) );
}


int uiAxisHandler::getPix( double pos ) const
{
    return getRelPosPix( getRelPos( (float) pos) );
}


int uiAxisHandler::getPix( int pos ) const
{
    return getRelPosPix( getRelPos( (float) pos) );
}



int uiAxisHandler::pixToEdge( bool withborder ) const
{
    int ret = withborder ? setup_.border_.get(setup_.side_) : 0;
    if ( setup_.noaxisannot_ || setup_.annotinside_ || setup_.fixedborder_ )
	return ret;

    ret += ticSz() + calcwdth_;
    return ret;
}


int uiAxisHandler::pixBefore() const
{
    int pixbefore  = 0;
    if ( beghndlr_ )
	pixbefore = beghndlr_->pixToEdge();
    else
    {
	uiRect::Side beforeside;
	if ( endhndlr_ )
	    beforeside = uiRect::across( endhndlr_->setup_.side_ );
	else
	    beforeside = isHor() ? uiRect::Left : uiRect::Bottom;
	pixbefore = setup_.border_.get( beforeside );
    }

    return pixbefore;
}


int uiAxisHandler::pixAfter() const
{
    int pixafter = 0;
    if ( endhndlr_ )
	pixafter =  endhndlr_->pixToEdge();
    else
    {
	uiRect::Side afterside = isHor() ? uiRect::Right : uiRect::Top;
	if ( beghndlr_ )
	    afterside = uiRect::across( beghndlr_->setup_.side_ );
	pixafter = setup_.border_.get( afterside );
    }

    return pixafter;
}


Interval<int> uiAxisHandler::pixRange() const
{
    if ( isHor() )
	return Interval<int>( pixBefore(), devsz_ - pixAfter() );
    else
	return Interval<int>( pixAfter(), devsz_ - pixBefore() );
}


void uiAxisHandler::updateAnnotations()
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
	annotPos( getRelPosPix(relpos), strs_[idx], setup_.style_ );
    }
}


void uiAxisHandler::setVisible( bool visible )
{
    if ( gridlineitmgrp_ ) gridlineitmgrp_->setVisible( visible );
    if ( annottxtitmgrp_ )annottxtitmgrp_->setVisible( visible );
    if ( annotlineitmgrp_ )annotlineitmgrp_->setVisible( visible );
    if ( axislineitm_ ) axislineitm_->setVisible( visible );
    if ( endannottextitm_ ) endannottextitm_->setVisible( visible );
    if ( nameitm_ ) nameitm_->setVisible( visible );
}


void uiAxisHandler::updateGridLines()
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

	if ( setup_.nogridline_ ) return;

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


void uiAxisHandler::updateScene()
{
    updateAxisLine();

    if ( setup_.nogridline_ )
    {
	if ( gridlineitmgrp_ )
	    gridlineitmgrp_->removeAll( true );
    }

    updateAnnotations();

    if ( !setup_.noaxisannot_ && !setup_.caption_.isEmpty() )
	updateName();

    updateGridLines();
}


void uiAxisHandler::updateAxisLine()
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
		    new uiLineItem(startpix,pixpos,endpix,pixpos) );
	else
	    axislineitm_->setLine( startpix, pixpos, endpix, pixpos );
	axislineitm_->setPenStyle( ls );
	axislineitm_->setZValue( setup_.zval_ );
    }
    else
    {
	const int startpix = rgisrev_ ? pixBefore() : pixAfter();
	const int endpix = devsz_ - ( rgisrev_ ? pixAfter() : pixBefore() );
	const int pixpos = setup_.side_ == uiRect::Left
			 ? edgepix : width_ - edgepix;

	if ( !axislineitm_ )
	    axislineitm_ = scene_->addItem(
		    new uiLineItem(pixpos,startpix,pixpos,endpix) );
	else
	    axislineitm_->setLine( pixpos, startpix, pixpos, endpix );
	axislineitm_->setPenStyle( ls );
	axislineitm_->setZValue( setup_.zval_ );
    }
}


void setLine( uiLineItem& lineitm, const LinePars& lp,
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

    lineitm.setLine( from, to );
}


void uiAxisHandler::annotAtEnd( const uiString& txt )
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
    endannottextitm_->setFontData( setup_.fontdata_ );
}


void uiAxisHandler::annotPos( int pix, const uiString& txt, const LineStyle& ls)
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
		      istop ? inside ? Alignment::Top : Alignment::Bottom
			    : inside ? Alignment::Bottom : Alignment::Top );
	uiTextItem* annotpostxtitem =
	    new uiTextItem( uiPoint(pix,y1), txt, al );
	annotpostxtitem->setTextColor( ls.color_ );
	annotpostxtitem->setFontData( setup_.fontdata_ );
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
	annotpostxtitem->setFontData( setup_.fontdata_ );
	annottxtitmgrp_->add( annotpostxtitem );
    }
}


void uiAxisHandler::drawGridLine( int pix )
{
    if ( setup_.nogridline_ ) return;
    uiLineItem* lineitem = getFullLine( pix );
    lineitem->setPenStyle( setup_.gridlinestyle_ );
    gridlineitmgrp_->add( lineitem );
    gridlineitmgrp_->setVisible( setup_.style_.isVisible() );
}


uiLineItem* uiAxisHandler::getFullLine( int pix )
{
    const uiAxisHandler* hndlr = beghndlr_ ? beghndlr_ : endhndlr_;
    int endpix = setup_.border_.get( uiRect::across(setup_.side_) );
    if ( hndlr )
	endpix = beghndlr_ ? hndlr->pixAfter() : hndlr->pixBefore();
    const int startpix = pixToEdge();

    uiLineItem* lineitem = new uiLineItem();
    const int length = isHor() ? height_ : width_ ;
    switch ( setup_.side_ )
    {
    case uiRect::Top:
	lineitem->setLine( pix, startpix, pix, length - endpix );
	break;
    case uiRect::Bottom:
	lineitem->setLine( pix, endpix, pix, length - startpix );
	break;
    case uiRect::Left:
	lineitem->setLine( startpix, pix, length - endpix, pix );
	break;
    case uiRect::Right:
	lineitem->setLine( endpix, pix, length - startpix, pix );
	break;
    }

    return lineitem;
}


void uiAxisHandler::updateName()
{
    if ( !nameitm_ )
	nameitm_ = scene_->addItem( new uiTextItem( setup_.caption_ ) );
    else
	nameitm_->setText( setup_.caption_ );

    Alignment al( Alignment::HCenter, Alignment::VCenter );
    uiPoint pt;
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int namepos = pixToEdge() - ticSz() - calcwdth_;
	pt.x = pixBefore() + axsz_/2;
	pt.y = istop ? namepos : height_-namepos;
	al.set( istop ? Alignment::Top : Alignment::Bottom );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	pt.x = isleft ? pixAfter() : width_-pixAfter();
	pt.y = height_/2 - pixAfter();
	al.set( Alignment::HCenter );

	if ( !ynmtxtvertical_ )
	    nameitm_->setRotation( mCast( float, isleft ? -90 : 90 ) );
	ynmtxtvertical_ = true;
    }
    nameitm_->setPos( pt );
    nameitm_->setAlignment( al );
    nameitm_->setZValue( setup_.zval_ );
    Color col = setup_.nmcolor_ == Color::NoColor() ? setup_.style_.color_
						    : setup_.nmcolor_;
    nameitm_->setTextColor( col );
    nameitm_->setFontData( setup_.fontdata_ );
}


int uiAxisHandler::ticSz() const
{ return setup_.noaxisannot_ ? 0 : ticsz_; }
