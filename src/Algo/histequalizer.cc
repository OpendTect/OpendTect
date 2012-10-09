/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          June 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "histequalizer.h"
#include "dataclipper.h"
#include "typeset.h"
#include "statrand.h"

HistEqualizer::HistEqualizer( const int nrseg )
    : datapts_(*new TypeSet<float>() )
    , histeqdatarg_(0)
    , nrseg_(nrseg)
{
}


void HistEqualizer::setData( const TypeSet<float>& datapts ) 
{
    datapts_ = datapts;
    update();
}


void HistEqualizer::setRawData( const TypeSet<float>& datapts )
{
    DataClipper clipper;
    clipper.putData( datapts.arr(), datapts.size() );
    clipper.fullSort();
    setData( clipper.statPts() );
}


void HistEqualizer::update()
{
    const int datasz = datapts_.size();
    if ( histeqdatarg_ )
	delete histeqdatarg_;
    histeqdatarg_ = new TypeSet< Interval<float> >();
    int index = 0;
    TypeSet<int> segszs;
    getSegmentSizes( segszs );
    for ( int idx = 0; idx < segszs.size(); idx++ )
    {
	int startidx = index;
	int stopindex = startidx + segszs[idx] > datasz - 1 ? 
			datasz - 1 : startidx + segszs[idx];
	*histeqdatarg_ += Interval<float> ( datapts_[startidx], 
					    datapts_[stopindex]);
	index = stopindex;
    }
    TypeSet< Interval<float> > testprint( *histeqdatarg_ );
}


float HistEqualizer::position( float val ) const
{
    int start = 0;
    int end = nrseg_ -1;
    int midval = ( nrseg_ -1 )/2;
    float ret = 0;
    Interval<float> startrg = (*histeqdatarg_)[start];
    Interval<float> stoprg = (*histeqdatarg_)[end];
    if (  val < startrg.stop )
	return 0;
    if (  val > stoprg.start )
	return 1;
    while ( end-start > 1 )
    {
	Interval<float> midvalrg = (*histeqdatarg_)[midval];
	if ( midvalrg.start <= val && midvalrg.stop >= val )
	{
	    ret = (float)midval/(float)nrseg_;
	    if ( ret > 1 ) ret = 1;
	    else if ( ret < 0 ) ret = 0;
	    return ret;
	}
	if ( midvalrg.stop < val )
	{
	    start = midval;
	    midval = start + ( end - start ) / 2;
	}
	else if ( midvalrg.start > val )
	{
	    end = midval;
	    midval = end - ( end - start ) / 2;
	}
    }
    return ret;
}


void HistEqualizer::getSegmentSizes( TypeSet<int>& segszs )
{
    const int datasz = datapts_.size();
    const int aindexlength = (int)(datasz/nrseg_);
    const int bindexlength = aindexlength+1;
    const int numberofa = bindexlength*nrseg_ - datasz;
    const int numberofb = nrseg_ - numberofa;

    segszs.setSize( nrseg_, aindexlength );
    int count = 0;
    while ( true )
    {
	if ( numberofb == 0 )
	    break;
	int idx = Stats::RandGen::getIndex( nrseg_ );
	if ( segszs[idx] == bindexlength )
	    continue;

	segszs[idx] = bindexlength;
	count++;
	if ( count == numberofb )
	    break;
    }
}

