/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uiaxishandler.cc,v 1.4 2008-03-20 14:55:10 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "iodrawtool.h"
#include "uifont.h"
#include "draw.h"

static const float logof2 = log(2);


uiAxisHandler::uiAxisHandler( ioDrawTool& dt, const uiAxisHandler::Setup& su )
    : NamedObject(su.name_)
    , dt_(dt)
    , setup_(su)
    , ticsz_(2)
    , beghndlr_(0)
    , endhndlr_(0)
{
    setRange( StepInterval<float>(0,1,1) );
}


void uiAxisHandler::setRange( const StepInterval<float>& rg )
{
    rg_ = rg;

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


void uiAxisHandler::reCalc()
{
    pos_.erase(); strs_.deepErase();

    BufferString str; str = rg_.start; strs_.add( str );
    pos_ += 0;
    const uiFont& font = uiFontList::get();
    wdthx_ = font.width( str );
    wdthy_ = font.height();

    const int nrsteps = rg_.nrSteps();
    for ( int idx=1; idx<=nrsteps; idx++ )
    {
	float relpos = idx * rg_.step;
	float pos = rg_.start + relpos;
	str = pos; strs_.add( str );
	pos = rgisrev_ ? -relpos : relpos;
	pos /= rgwidth_;
	if ( setup_.islog_ )
	    pos = log( 1 + pos );
	pos_ += pos;
	const int wdth = font.width( str );
	if ( wdthx_ < wdth ) wdthx_ = wdth;
    }

    str = rg_.stop; strs_.add( str );
    pos_ += setup_.islog_ ? logof2 : 1;
    const int wdth = font.width( str );
    if ( wdthx_ < wdth ) wdthx_ = wdth;

    newDevSize();
}


void uiAxisHandler::newDevSize()
{
    devsz_ = isHor() ? dt_.getDevWidth() : dt_.getDevHeight();
    axsz_ = devsz_ - pixBefore() - pixAfter();
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
	relpix = exp( relpix * logof2 );

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


int uiAxisHandler::pixToEdge() const
{
    int ret = ticsz_ + setup_.border_.get(setup_.side_);
    ret += isHor() ? wdthy_ : wdthx_;
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


void uiAxisHandler::plotAxis() const
{
    drawAxisLine();

    if ( setup_.style_.isVisible() )
    {
	dt_.setLineStyle( setup_.style_ );
	Interval<int> toplot( 0, pos_.size()-1 );
	if ( beghndlr_ ) toplot.start++;
	if ( endhndlr_ ) toplot.stop--;
	for ( int idx=toplot.start; idx<=toplot.stop; idx++ )
	{
	    const float relpos = pos_[idx] / pos_[pos_.size()-1];
	    drawGridLine( getRelPosPix(relpos) );
	}
    }

    LineStyle ls( setup_.style_ );
    ls.width_ = 1; ls.type_ = LineStyle::Solid;
    dt_.setLineStyle( ls );
    for ( int idx=0; idx<pos_.size(); idx++ )
    {
	const float relpos = pos_[idx] / pos_[pos_.size()-1];
	annotPos( getRelPosPix(relpos), strs_.get(idx) );
    }

    if ( !name().isEmpty() )
	drawName();
}


void uiAxisHandler::drawAxisLine() const
{
    LineStyle ls( setup_.style_ );
    ls.type_ = LineStyle::Solid;
    dt_.setLineStyle( ls );

    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	const int startpix = pixBefore();
	const int endpix = devsz_-pixAfter();
	const int pixpos = setup_.side_ == uiRect::Top
	    		 ? edgepix : dt_.getDevHeight() - edgepix;
	dt_.drawLine( startpix, pixpos, endpix, pixpos );
    }
    else
    {
	const int startpix = pixAfter();
	const int endpix = devsz_-pixBefore();
	const int pixpos = setup_.side_ == uiRect::Left
	    		 ? edgepix : dt_.getDevWidth() - edgepix;

	dt_.drawLine( pixpos, startpix, pixpos, endpix );
    }
}


void uiAxisHandler::annotPos( int pix, const char* txt ) const
{
    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int y0 = istop ? edgepix : dt_.getDevHeight() - edgepix;
	const int y1 = istop ? y0 - ticsz_ : y0 + ticsz_;
	dt_.drawLine( pix, y0, pix, y1 );
	Alignment al( Alignment::Middle,
		      istop ? Alignment::Stop : Alignment::Start );
	dt_.drawText( pix, y1, txt, al );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	const int x0 = isleft ? edgepix : dt_.getDevWidth() - edgepix;
	const int x1 = isleft ? x0 - ticsz_ : x0 + ticsz_;
	dt_.drawLine( x0, pix, x1, pix );
	Alignment al( isleft ? Alignment::Stop : Alignment::Start,
		      Alignment::Middle );
	dt_.drawText( x1, pix, txt, al );
    }
}


void uiAxisHandler::drawGridLine( int pix ) const
{
    const uiAxisHandler* hndlr = beghndlr_ ? beghndlr_ : endhndlr_;
    int endpix = setup_.border_.get( uiRect::across(setup_.side_) );
    if ( hndlr )
	endpix = setup_.side_ == uiRect::Left || setup_.side_ == uiRect::Bottom
	    	? hndlr->pixAfter() : hndlr->pixBefore();
    const int startpix = pixToEdge();

    switch ( setup_.side_ )
    {
    case uiRect::Top:
	dt_.drawLine( pix, startpix, pix, dt_.getDevHeight() - endpix );
	break;
    case uiRect::Bottom:
	dt_.drawLine( pix, endpix, pix, dt_.getDevHeight() - startpix );
	break;
    case uiRect::Left:
	dt_.drawLine( startpix, pix, dt_.getDevWidth() - endpix, pix );
	break;
    case uiRect::Right:
	dt_.drawLine( endpix, pix, dt_.getDevWidth() - startpix, pix );
	break;
    }
}


void uiAxisHandler::drawName() const
{
    uiPoint pt;
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int x = pixBefore() + axsz_ / 2;
	const int y = istop ? 2 : dt_.getDevHeight()-2;
	const Alignment al( Alignment::Middle,
			    istop ? Alignment::Start : Alignment::Stop );
	dt_.drawText( x, y, name(), al );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	const int y = 2;
	const int x = isleft ? pixToEdge() : dt_.getDevWidth()-pixToEdge()-1;
	const Alignment al( isleft ? Alignment::Start : Alignment::Stop,
	       		    Alignment::Start );
	dt_.drawText( x, y, name(), al );
    }
}
