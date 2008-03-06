/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uiaxishandler.cc,v 1.2 2008-03-06 18:25:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "iodrawtool.h"
#include "uifont.h"
#include "draw.h"


uiAxisHandler::uiAxisHandler( ioDrawTool& dt, const uiAxisHandler::Setup& su )
    : dt_(dt)
    , setup_(su)
    , ticsz_(2)
    , beghndlr_(0)
    , endhndlr_(0)
{
    setRange( StepInterval<float>(0,1,1) );
}


void uiAxisHandler::reCalc()
{
    pos_.erase(); strs_.deepErase();

    float fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps < 0 )
	rg_.step = -rg_.step;
    if ( mIsZero(fsteps,1e-6) )
	{ rg_.start -= rg_.step * 1.5; rg_.stop += rg_.step * 1.5; }
    fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps > 100 )
    	rg_.step /= (fsteps / 100);

    BufferString str; str = rg_.start;
    pos_ += 0;
    const uiFont& font = uiFontList::get();
    wdthx_ = font.width( str );
    wdthy_ = font.height();

    const int nrsteps = rg_.nrSteps();
    const float rgwdth = rg_.width();
    const float logrgwdth = log( rgwdth + 1 );
    for ( int idx=1; idx<nrsteps; idx++ )
    {
	float pos = rg_.start + idx*rg_.step;
	str = pos; strs_.add( str );
	if ( setup_.islog_ )	pos = log( pos - rg_.start + 1 );
	else			pos /= rgwdth;
	pos_ += pos;
	const int wdth = font.width( str );
	if ( wdthx_ < wdth ) wdthx_ = wdth;
	strs_.add( str );
    }

    str = rg_.stop; strs_.add( str );
    pos_ += setup_.islog_ ? log( rgwdth + 1 ) : 1;
    const int wdth = font.width( str );
    if ( wdthx_ < wdth ) wdthx_ = wdth;

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

    if ( !setup_.islog_ )
	return rg_.start + rg_.width() * relpix;

    relpix = exp( relpix );
    return rg_.start + rg_.width() * (relpix / pos_[pos_.size()-1]);
}


float uiAxisHandler::getRelPos( float v ) const
{
    float relv = v - rg_.start;
    if ( !setup_.islog_ )
	return relv / (rg_.stop - rg_.start);

    if ( relv < -0.9 ) relv = -0.9;
    return log( relv + 1 ) / pos_[pos_.size()-1];
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
	for ( int idx=0; idx<pos_.size(); idx++ )
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
}


void uiAxisHandler::drawAxisLine() const
{
    LineStyle ls( setup_.style_ );
    ls.type_ = LineStyle::Solid;
    dt_.setLineStyle( ls );

    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	if ( setup_.side_ == uiRect::Top )
	    dt_.drawLine( pixBefore(), edgepix, devsz_-pixAfter(), edgepix );
	else
	{
	    const int devhght = dt_.getDevHeight();
	    dt_.drawLine( pixBefore(), devhght-edgepix,
		         devsz_-pixAfter(), devhght-edgepix );
	}
    }
    else
    {
	if ( setup_.side_ == uiRect::Left )
	    dt_.drawLine( edgepix, pixAfter(), edgepix, devsz_-pixBefore() );
	else
	{
	    const int devwdth = dt_.getDevWidth();
	    dt_.drawLine( devwdth-edgepix, pixAfter(),
		         devwdth-edgepix, devsz_-pixBefore() );
	}
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
    const int edgepix = pixToEdge();
    const uiAxisHandler* hndlr = beghndlr_ ? beghndlr_ : endhndlr_;
    const int ppix0 = hndlr ? hndlr->pixBefore() : edgepix;
    const int ppix1 = hndlr ? hndlr->pixAfter()
			    : setup_.border_.get(uiRect::across(setup_.side_));
    if ( isHor() )
	dt_.drawLine( pix, ppix0, pix, dt_.getDevHeight() - ppix1 );
    else
	dt_.drawLine( edgepix, ppix0, dt_.getDevWidth() - ppix1, pix );
}
