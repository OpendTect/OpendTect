#ifndef indexinfo_h
#define indexinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jan 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "samplingdata.h"


//! Info on (floating-point) position in an array or StepInterval

mClass IndexInfo
{
public:
			IndexInfo( int i, bool r=true, bool u=false )
			    : nearest_(i)
			    , roundedtolow_(r)
			    , inundef_(u)		{}
    template <class X,class Y>
			IndexInfo( const StepInterval<X>& s, Y y )
			    { set( s, y ); }
    template <class X,class Y>
			IndexInfo( const SamplingData<X>& s, Y y, int len )
			    { set( s, y, len ); }
    template <class T>
			IndexInfo(const T*,int sz,T val);

    template <class X,class Y>
    void		set(const StepInterval<X>&,Y);
    template <class X,class Y>
    void		set(const SamplingData<X>&,Y,int length);


    int			nearest_;
    bool		roundedtolow_;
    bool		inundef_;
};


template <class T> inline
IndexInfo::IndexInfo( const T* arr, int sz, T val )
    : nearest_(0)
    , roundedtolow_(true)
    , inundef_(true)
{
    if ( sz < 1 || !arr )
	return;
    if ( sz == 1 )
	{ inundef_ = val != arr[0]; return; }
    const bool isrev = arr[0] > arr[sz-1];
    if ( (isrev && val >= arr[0]) || (!isrev && val<=arr[0]) )
	{ inundef_ = val != arr[0]; roundedtolow_ = isrev; return; }
    if ( (!isrev && val >= arr[sz-1]) || (isrev && val<=arr[sz-1]) )
	{ nearest_ = sz-1; inundef_ = val != arr[sz-1]; roundedtolow_ = !isrev;
	  return; }

    inundef_ = false;
    for ( nearest_=1; nearest_<sz; nearest_++ )
    {
	if ( arr[nearest_] == val )
	    return;
	if ( (!isrev && val < arr[nearest_]) || (isrev && val > arr[nearest_]) )
	{
	    T halfway = (arr[nearest_] + arr[nearest_-1]) * .5;
	    roundedtolow_ = isrev ? val > halfway : val < halfway;
	    if ( (!isrev && roundedtolow_) || (isrev && !roundedtolow_) )
		nearest_ -= 1;
	    return;
	}
    }
    // Can we get here? Better safe than sorry.
    nearest_ = sz - 1; inundef_ = true; roundedtolow_ = !isrev;
}


template <class X,class Y> inline
void IndexInfo::set( const StepInterval<X>& intv, Y y )
{
    const bool isrev = intv.step < 0;
    const Y hstep = intv.step * 0.5;

    if ( (isrev && y>intv.start+hstep) || (!isrev && y<intv.start-hstep) )
	{ inundef_ = true; roundedtolow_ = false; nearest_ = 0; }
    else if ( (isrev && y< intv.stop-hstep) || (!isrev && y>intv.stop+hstep) )
	{ inundef_ = true; roundedtolow_ = true; nearest_ = intv.nrSteps(); }
    else
    {
	inundef_ = false;
	nearest_ = intv.getIndex( y );
	const Y pred = intv.atIndex( nearest_ );
	roundedtolow_ = isrev ? pred < y : pred > y;
    }
}

template <class X,class Y> inline
void IndexInfo::set( const SamplingData<X>& sd, Y y, int nr )
{
    return set( StepInterval<X>( sd.start, sd.atIndex(nr-1), sd.step ), y );
}


#endif
