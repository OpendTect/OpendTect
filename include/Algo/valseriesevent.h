#ifndef valseriesevent_h
#define valseriesevent_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		May 2005
 RCS:		$Id: valseriesevent.h,v 1.19 2012-08-03 13:00:06 cvskris Exp $
________________________________________________________________________

*/

#include "algomod.h"
#include "enums.h"
#include "mathfunc.h"
#include "ptrman.h"
#include "ranges.h"
#include "samplingdata.h"
#include "valseries.h"

mClass(Algo) VSEvent
{
public:
    enum Type	{ None, Extr, Max, Min, ZC, ZCNegPos, ZCPosNeg,
		  GateMax, GateMin };
		DeclareEnumUtils(Type);
};


/*!\brief Event in value series

  Template args are: Value Type, Position Type.
  Usually float,float

  */

template <class VT,class PT>
class ValueSeriesEvent : public VSEvent
{
public:


		ValueSeriesEvent( VT v=mUdf(VT), PT p=mUdf(PT) )
		{ val = v; pos = p;}

    VT		val;
    PT		pos;
};


/*!\brief Event finder in gate.

  The gate is absolute. The finder will start at the 'start' and stop at the
  'stop'. This is important because the event finding has a direction! Thus,
  the Interval<PT> you provide can go up or down.

  The 'occ' parameter is specifies the occurrence of the event; it is ignored
  for GateMin and Max.
 
 */

template <class VT,class PT>
class ValueSeriesEvFinder
{
public:
				ValueSeriesEvFinder( const ValueSeries<VT>& v,
						     int maxidx,
						     const SamplingData<PT>& s )
				: vs_(v)
				, maxidx_(maxidx)
				, sd_(s)
				, lastfound_(VSEvent::None)	{}

    const ValueSeries<VT>&	valueSeries() const { return vs_; }
    const SamplingData<PT>&	samplingData() const { return sd_; }

    ValueSeriesEvent<VT,PT>	find(VSEvent::Type,const Interval<PT>&,
	    				int occ=1) const;
    bool			findEvents(TypeSet<PT>&,Interval<PT>,
	    				   VSEvent::Type);

    static ValueSeriesEvent<VT,PT> exactExtreme(VSEvent::Type,
	    				   int idxminus1,int idx0,int idx1,
	    				   VT vminus1,VT v0,VT v1,
					   const SamplingData<PT>&);
    				//!< 2nd order polynome where values
    				//!< can be separated more than 1 sample

    VSEvent::Type		lastFound() const	{ return lastfound_; }
    				//!< Useful when finding Extr or ZC

protected:

    const ValueSeries<VT>&	vs_;
    const SamplingData<PT>	sd_;
    int				maxidx_;
    mutable VSEvent::Type	lastfound_;

    ValueSeriesEvent<VT,PT>	getZC(const Interval<int>&,int,
	    				VSEvent::Type) const;
    ValueSeriesEvent<VT,PT>	getExtreme(const Interval<int>&,int,
	    				VSEvent::Type) const;
    ValueSeriesEvent<VT,PT>	getGateExtr(const Interval<int>&,bool) const;

};


#undef mIncSampIdx
#define mIncSampIdx(idx) { \
	idx += inc; \
       	if ( idx == sg.stop+inc ) \
	    return ValueSeriesEvent<VT,PT>( 0, mUdf(PT) ); }
#undef mDecrOccAtZero
#define mDecrOccAtZero(idx) { \
	occ--; \
	if ( occ < 1 ) \
	    return ValueSeriesEvent<VT,PT>( 0, sd_.atIndex(idx) ); }


template <class VT,class PT>
inline ValueSeriesEvent<VT,PT> ValueSeriesEvFinder<VT,PT>::getZC(
	const Interval<int>& sg, int occ, VSEvent::Type evtype ) const
{
    const int inc = sg.start < sg.stop ? 1 : -1;

    int idx = sg.start;
    VT v0 = vs_.value( idx );
    while ( v0 == 0 )
    {
	mDecrOccAtZero(idx)
	mIncSampIdx(idx)
	v0 = vs_.value( idx );
    }

    int frompositive = v0 > 0;
    const bool needtopos = evtype != VSEvent::ZCPosNeg;
    const bool needtoneg = evtype != VSEvent::ZCNegPos;
    for ( ; idx!=sg.stop+inc; idx+=inc )
    {
	VT v1 = vs_.value( idx );
	while ( v1 == 0 )
	{
	    mIncSampIdx(idx)
	    v0 = v1;
	    v1 = vs_.value( idx );
	    if ( v1 != 0 )
	    {
		if ( ( (v1<0) != frompositive && needtoneg )
		  || ( (v1>0) != frompositive && needtopos ) )
		    mDecrOccAtZero(idx-inc)

		mIncSampIdx(idx)
		v0 = v1; frompositive = v0 > 0;
		v1 = vs_.value( idx );
	    }
	}
	// Here, both v0 and v1 are non-zero
	// Now we can do the main thing:
	const bool istopos = v0 < 0 && v1 > 0;
	const bool istoneg = v0 > 0 && v1 < 0;
	if ( ( istopos && needtopos ) || ( istoneg && needtoneg ) )
	{
	    occ--;
	    if ( occ < 1 )
	    {
		lastfound_ = istopos ? VSEvent::ZCNegPos
		    		     : VSEvent::ZCPosNeg;
		PT pos = idx - inc * (v1 / ( v1 - v0 ));
		return ValueSeriesEvent<VT,PT>( 0, sd_.start + pos * sd_.step );
	    }
	}
	v0 = v1;
    }

    return ValueSeriesEvent<VT,PT>( 0, mUdf(PT) );
}


template <class VT,class PT>
inline ValueSeriesEvent<VT,PT> ValueSeriesEvFinder<VT,PT>::exactExtreme(
			    VSEvent::Type evtype, int idxm1, int idx0, int idx1,
			    VT vm1, VT v0, VT v1,
       			    const SamplingData<PT>& sd )
{
    if ( idxm1 > idx1 )
	{ Swap( idxm1, idx1 ); Swap( vm1, v1 ); }

    vm1 -= v0; v1 -= v0;
    idxm1 -= idx0; idx1 -= idx0;

    vm1 /= idxm1; v1 /= idx1;

    VT a = (vm1 - v1) / (idxm1 - idx1);
    VT b = 0;
    PT relpos = 0;
    if ( a != 0 )
    {
	b = vm1 - a * idxm1;
	relpos = -b / (2*a);
    }

    ValueSeriesEvent<VT,PT> ret;
    ret.val = (VT)(v0 + a * relpos * relpos + b * relpos);
    ret.pos = sd.start + sd.step * (idx0 + relpos);
    return ret;
}


template <class VT,class PT>
inline ValueSeriesEvent<VT,PT> ValueSeriesEvFinder<VT,PT>::getGateExtr(
			    const Interval<int>& inpsg, bool needmax ) const
{
    Interval<int> sg( inpsg );
    sg.sort();

    // skip undefs at start
    int curidx;
    int extridx = sg.start; VT extrval = vs_.value( extridx );
    for ( curidx=sg.start+1; mIsUdf(extrval) && curidx<=sg.stop; curidx++ )
	{ extridx = curidx; extrval = vs_.value( curidx ); }
    if ( mIsUdf(extrval) )
	return ValueSeriesEvent<VT,PT>();

    // find min/max
    for ( ; curidx<=sg.stop; curidx++ )
    {
	const VT val = vs_.value( curidx );
	if ( mIsUdf(val) ) continue;

	if ( (needmax && val > extrval) || (!needmax && val < extrval) )
	    { extridx = curidx; extrval = val; }
    }

    // collect the data points around the extreme sample
    VT v0 = extrval;
    VT vm1 = extridx > sg.start ? vs_.value(extridx-1) : v0;
    if ( mIsUdf(vm1) ) vm1 = v0;
    VT v1 = extridx < sg.stop-1 ? vs_.value(extridx+1) : v0;
    if ( mIsUdf(v1) ) v1 = v0;

    return exactExtreme( needmax ? VSEvent::Max : VSEvent::Min,
	    		 extridx-1, extridx, extridx+1, vm1, v0, v1, sd_ );
}


template <class VT,class PT>
inline ValueSeriesEvent<VT,PT> ValueSeriesEvFinder<VT,PT>::getExtreme(
	const Interval<int>& sg, int occ, VSEvent::Type evtype ) const
{
    const int inc = sg.start < sg.stop ? 1 : -1;
    int idx0 = sg.start;
    VT v0 = vs_.value( idx0 );
    bool havevm1 = (inc > 0 && sg.start > 0) || (inc < 0 && sg.start < maxidx_);
    VT vm1 = havevm1 ? vs_.value( sg.start - inc ) : v0;
    if ( mIsUdf(vm1) ) vm1 = v0;
    bool upw0 = v0 > vm1;
    int idx1 = idx0;

    while ( true )
    {
	mIncSampIdx(idx1)

	VT v1 = vs_.value( idx1 );
	if ( mIsUdf(v1) || v1 == v0 )
	    continue;

	const bool upw1 = v1 > v0;
	bool ishit = havevm1 && !mIsUdf(v0) && upw0 != upw1;
	if ( ishit )
	{
	    const bool atmax = upw0 && !upw1;
	    ishit = (evtype != VSEvent::Min && atmax)
		 || (evtype != VSEvent::Max && !atmax);
	    if ( ishit )
	    {
		lastfound_ = atmax ? VSEvent::Max : VSEvent::Min;
		occ--;
	    }
	}

	if ( occ < 1 )
	    return exactExtreme( evtype, idx0-inc, idx0, idx1,
		    		 vm1, v0, v1, sd_ );

	upw0 = upw1; idx0 = idx1; vm1 = v0; v0 = v1;
	havevm1 = !mIsUdf(v0);
    }

    // not reached, mIncSampIdx or exactExtreme return
}


template <class VT,class PT>
inline ValueSeriesEvent<VT,PT> ValueSeriesEvFinder<VT,PT>::find(
		VSEvent::Type evtype, const Interval<PT>& pgin, int occ ) const
{
    lastfound_ = evtype;
    Interval<PT> pg( pgin );

    ValueSeriesEvent<VT,PT> ev;
    if ( occ < 1 )
    {
	pErrMsg("Weird request: less than first occ of event.");
	return ev;
    }

    if ( evtype == VSEvent::None )
	return ev;

    const int inc = pg.start < pg.stop ? 1 : -1;
    if ( pg.start < sd_.start )
	{ if ( inc < 0 ) return ev; pg.start = sd_.start; }
    if ( pg.stop < sd_.start )
	{ if ( inc > 0 ) return ev; pg.stop = sd_.start; }

    const PT endpos = sd_.atIndex( maxidx_ );
    if ( pg.start > endpos )
	{ if ( inc > 0 ) return ev; pg.start = endpos; }
    if ( pg.stop > endpos )
	{ if ( inc < 0 ) return ev; pg.stop = endpos; }

    SampleGate sg;
    if ( inc > 0 )
    {
	sg.start = (int)floor((pg.start-sd_.start)/sd_.step);
	sg.stop = (int)ceil((pg.stop-sd_.start)/sd_.step);
    }
    else
    {
	sg.start = (int)ceil((pg.start-sd_.start)/sd_.step);
	sg.stop = (int)floor((pg.stop-sd_.start)/sd_.step);
	if ( evtype == VSEvent::ZCNegPos )
	    evtype = VSEvent::ZCPosNeg;
	else if ( evtype == VSEvent::ZCPosNeg )
	    evtype = VSEvent::ZCNegPos;
    }

    bool iszc = false;
    if ( evtype==VSEvent::GateMax || evtype==VSEvent::GateMin )
	return getGateExtr( sg, evtype == VSEvent::GateMax );
    else if ( sg.start == sg.stop )
	return ev;
    else if ( evtype >= VSEvent::ZC && evtype <= VSEvent::ZCPosNeg )
	iszc = true;

    while ( true )
    {
	if ( !iszc )
	    ev = getExtreme( sg, occ, evtype );
	else
	{
	    ev = getZC( sg, occ, evtype );
	    if ( inc < 0 )
		lastfound_ = lastfound_ == VSEvent::ZCPosNeg
		    	   ? VSEvent::ZCNegPos : VSEvent::ZCPosNeg;
	}
	if ( mIsUdf(ev.pos) )
	    break;

	if ( ( inc > 0 && ev.pos < pg.start ) || 
	     ( inc < 0 && ev.pos > pg.start ) )
	    occ++;
	else
	    break;
    }

    return ev;
}


//  Gives a TypeSet of all the events of a type making sure that there is an 
//  'opposite'(with phase diff 180deg) event type between any two of them:
template <class VT,class PT>
inline bool ValueSeriesEvFinder<VT,PT>::findEvents( TypeSet<PT>& posset,
					Interval<PT> pg, VSEvent::Type evtype )
{
    Interval<PT> curg( pg );
    VSEvent::Type revtype;
    if ( evtype == VSEvent::Max )
	revtype = VSEvent::Min;
    else if ( evtype == VSEvent::Min )
	revtype = VSEvent::Max;
    else if ( evtype == VSEvent::ZCNegPos )
	revtype = VSEvent::ZCPosNeg;
    else if ( evtype == VSEvent::ZCPosNeg )
	revtype = VSEvent::ZCNegPos;

    const bool isascending = pg.stop > pg.start;
    posset.erase();
    while ( isascending == (curg.stop>curg.start) )
    {
	ValueSeriesEvent<VT,PT> reqev = find( evtype, curg, 1 );
	if ( mIsUdf(reqev.pos) ) break;

	posset += reqev.pos;
	curg.start = reqev.pos + 1e-5;
	ValueSeriesEvent<VT,PT> revev = find( revtype, curg, 1 );
	if ( mIsUdf(revev.pos) ) break;

	curg.start = revev.pos + 1e-5;
    }

    if ( !posset.size() ) return false;

    return true;
}


#undef mIncSampIdx
#undef mDecrOccAtZero

#endif

