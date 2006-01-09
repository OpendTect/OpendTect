#ifndef indexinfo_h
#define indexinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Jan 2006
 RCS:		$Id: indexinfo.h,v 1.1 2006-01-09 16:31:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "samplingdata.h"


//! Info on (floating-point) position in an array or StepInterval

class IndexInfo
{
public:
			IndexInfo( int i, bool r=true, bool u=false )
			    : nearest(i)
			    , roundedtolow(r)
			    , inundef(u)		{}
    template <class X,class Y>
			IndexInfo( const StepInterval<X>& s, Y y )
			    { set( s, y ); }
    template <class X,class Y>
			IndexInfo( const SamplingData<X>& s, Y y, int len )
			    { set( s, y, len ); }

    template <class X,class Y>
    void		set(const StepInterval<X>&,Y);
    template <class X,class Y>
    void		set(const SamplingData<X>&,Y,int length);


    int			nearest;
    bool		roundedtolow;
    bool		inundef;
};



template <class X,class Y> inline
void IndexInfo::set( const StepInterval<X>& intv, Y y )
{
    const bool isrev = intv.step < 0;
    const Y hstep = intv.step * 0.5;

    if ( (isrev && y>intv.start+hstep) || (!isrev && y<intv.start-hstep) )
	{ inundef = true; roundedtolow = false; nearest = 0; }
    else if ( (isrev && y< intv.stop-hstep) || (!isrev && y>intv.stop+hstep) )
	{ inundef = true; roundedtolow = true; nearest = intv.nrSteps(); }
    else
    {
	inundef = false;
	nearest = intv.getIndex( y );
	const Y pred = intv.atIndex( nearest );
	roundedtolow = isrev ? pred < y : pred > y;
    }
}

template <class X,class Y> inline
void IndexInfo::set( const SamplingData<X>& sd, Y y, int nr )
{
    return set( StepInterval<X>( sd.start, sd.atIndex(nr-1), sd.step ), y );
}


#undef cloneTp


#endif
