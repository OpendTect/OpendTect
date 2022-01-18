/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2008
________________________________________________________________________

-*/

#include "uifuncdispbase.h"
#include "od_iostream.h"
#include "uiaxishandlerbase.h"


uiFuncDispBase::~uiFuncDispBase()
{
}


void uiFuncDispBase::setVals( const float* xvals, const float* yvals, int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz > 0 )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( mIsUdf(xvals[idx]) || mIsUdf(yvals[idx]) )
		continue;
	    xvals_ += xvals[idx];
	    yvals_ += yvals[idx];
	}
    }

    gatherInfo( false ); draw();
}


void uiFuncDispBase::setVals( const Interval<float>& xrg, const float* yv,
				 int sz )
{
    if ( sz<0 )
	return;

    StepInterval<float> xint( xrg );
    xint.step = (xrg.stop-xrg.start) / (sz-1);
    float* xv = new float[sz];
    for ( int idx=0; idx<sz; idx++ )
	xv[idx] = xint.atIndex( idx );

    setVals( xv, yv, sz );
    delete [] xv;
}


void uiFuncDispBase::setY2Vals( const float* xv, const float* yv, int sz )
{
    y2xvals_.erase(); y2yvals_.erase();
    if ( sz > 0 )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    y2xvals_ += xv[idx];
	    y2yvals_ += yv[idx];
	}
    }

    gatherInfo( true ); draw();
}


void uiFuncDispBase::setY2Vals( const Interval<float>& xrg,
				   const float* yv, int sz )
{
    if ( sz<0 )
	return;

    StepInterval<float> xint( xrg );
    xint.step = (xrg.stop-xrg.start) / (sz-1);
    float* xv = new float[sz];
    for ( int idx=0; idx<sz; idx++ )
	xv[idx] = xint.atIndex( idx );

    setY2Vals( xv, yv, sz );
    delete [] xv;
}


void uiFuncDispBase::getAxisRanges( const TypeSet<float>& vals,
				       const Interval<float>& setuprg,
				       Interval<float>& rg ) const
{
    rg.set( mUdf(float), -mUdf(float) );
    for ( int idx=0; idx<vals.size(); idx++ )
    {
	if ( mIsUdf(vals[idx]) )
	    continue;

	rg.include( vals[idx], false );
    }

    if ( !setup_.fixdrawrg_ )
	return;

    if ( !mIsUdf(setuprg.start) ) rg.start = setuprg.start;
    if ( !mIsUdf(setuprg.stop) ) rg.stop = setuprg.stop;
}


void uiFuncDispBase::setMarkValue( float val, bool is_x )
{
    (is_x ? xmarklineval_ : ymarklineval_) = val;
    drawMarkLines();
}


void uiFuncDispBase::setMark2Value( float val, bool is_x )
{
    (is_x ? xmarkline2val_ : ymarkline2val_) = val;
    drawMarkLines();
}



void uiFuncDispBase::setEmpty()
{
    xmarklineval_ = ymarklineval_ = xmarkline2val_ = ymarkline2val_ =
								    mUdf(float);
    setVals( 0, 0, 0 );
    setY2Vals( 0, 0, 0 );
    cleanUp();
}


void uiFuncDispBase::dump( od_ostream& strm, bool y2 ) const
{
    const TypeSet<float>& xvals = y2 ? y2xvals_ : xvals_;
    const TypeSet<float>& yvals = y2 ? y2yvals_ : yvals_;

    strm.stdStream() << std::fixed;
    for ( int idx=0; idx<xvals.size(); idx++ )
	strm << xvals[idx] << od_tab << yvals[idx] << od_endl;
}


void uiFuncDispBase::gatherInfo( bool fory2 )
{
    const bool usey2 = fory2 && !setup_.useyscalefory2_;
    if ( !xax_ || ( !usey2 && !yax_ ) || ( usey2 && !y2ax_ ) )
	return;

    Interval<float> xrg, yrg;
    if ( xvals_.isEmpty() )
    {
	xrg.start = mUdf(float); xrg.stop = -mUdf(float);
	yrg = xrg;
    }
    else
    {
	getAxisRanges( xvals_, setup_.xrg_, xrg );
	getAxisRanges( usey2 ? y2yvals_ : yvals_,
		       usey2 ? setup_.y2rg_ : setup_.yrg_, yrg );
    }

    uiAxisHandlerBase* xaxis = xAxis();
    uiAxisHandlerBase* yaxis = yAxis( usey2 );
    if ( !xaxis || !yaxis )
	return;

    xaxis->setBounds( xrg );
    yaxis->setBounds( yrg );
}
