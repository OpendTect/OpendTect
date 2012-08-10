#ifndef axislayout_h
#define axislayout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jan 2005 / Dec 2009
 RCS:		$Id: axislayout.h,v 1.5 2012-08-10 03:50:03 cvsaneesh Exp $
________________________________________________________________________

-*/

#include "algomod.h"
#include "ranges.h"
#include "samplingdata.h"

#include <math.h>


/*!\brief helps making nice axes for graphs */

template <class T>
mClass(Algo) AxisLayout
{
public:
			// Have layout calculated
			AxisLayout( const Interval<T>& dr )
			    { setDataRange(dr); }
    void		setDataRange(const Interval<T>&);

    SamplingData<T>	sd_;
    T			stop_;

    				// Init with explicit layout
			AxisLayout( T start=0, T st_op=1,
				    T step=1 )
			    : sd_(start,step), stop_(st_op)	{}
			AxisLayout( const StepInterval<T>& rg )
			    : sd_(rg.start,rg.step)
			    , stop_(rg.stop)			{}

			// Returns 'modulo' end with this sd_ and stop_
    T			findEnd(T datastop) const;
};


template <class T> inline
void AxisLayout<T>::setDataRange( const Interval<T>& dr )
{
    Interval<T> intv = dr;
    sd_.start = intv.start;
    const bool rev = intv.start > intv.stop;
    if ( rev ) Swap( intv.start, intv.stop );
    T wdth = intv.width();


    // guard against zero interval
    T indic = intv.start + wdth;
    T indic_start = intv.start;
    if ( mIsZero(indic,mDefEps) ) { indic_start += 1; indic += 1; }
    indic = 1 - indic_start / indic;
    if ( mIsZero(indic,mDefEps) )
    {
	sd_.start = intv.start - 1;
	sd_.step = 1;
	stop_ = intv.start + 1;
	return;
    }

    double scwdth = wdth < 1e-30 ? -30 : log10( wdth );
    int tenpow = 1 - (int)scwdth; if ( scwdth < 0 ) tenpow++;
    double stepfac = Math::IntPowerOf( ((double)10), tenpow );
    scwdth = wdth * stepfac;

    double scstep;
    if ( scwdth < 15 )          scstep = 2.5;
    else if ( scwdth < 30 )     scstep = 5;
    else if ( scwdth < 50 )     scstep = 10;
    else                        scstep = 20;

    sd_.step = (T) ( scstep / stepfac );
    if ( wdth > 1e-30 )
    {
	const T fidx = (T) ( rev
	    ? ceil( intv.stop / sd_.step + 1e-6 )
	    : floor( intv.start / sd_.step + 1e-6 ) );
	sd_.start = mNINT32( fidx ) * sd_.step;
    }
    if ( rev ) sd_.step = -sd_.step;

    stop_ = findEnd( rev ? intv.start : intv.stop );
}


template <class T> inline
T AxisLayout<T>::findEnd( T datastop ) const
{
    SamplingData<T> worksd = sd_;
    const bool rev = worksd.step < 0;
    if ( rev )
    {
	worksd.start = -worksd.start;
	worksd.step = -worksd.step;
	datastop = -datastop;
    }

    if ( worksd.start + 10000 * worksd.step < datastop )
	return datastop;

    T pos = (T) ( ceil( (datastop-worksd.start) / worksd.step - 1e-6 ) );
    if ( pos < .5 ) pos = 1;
    T wdth = mNINT32(pos) * worksd.step;

    return sd_.start + (rev ? -wdth : wdth);
}


#endif

