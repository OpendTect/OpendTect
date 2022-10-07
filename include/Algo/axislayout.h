#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "ranges.h"
#include "samplingdata.h"

#include <math.h>


/*!
\brief Helps making nice axes for graphs.
*/

template <class T>
mClass(Algo) AxisLayout
{
public:
			// Have layout calculated
			AxisLayout( const Interval<T>& dr, bool asint=false,
				    bool inside=false )
			    : annotinint_(asint)
			    , annotinsiderg_(inside)
			{ setDataRange( dr ); }
			~AxisLayout()
			{}

    void		setDataRange(const Interval<T>&);
    StepInterval<T>	getSampling() const;

    SamplingData<T>	sd_;
    T			stop_;
    bool		annotinint_;
    bool		annotinsiderg_;

			// Init with explicit layout
			AxisLayout( T start=0, T st_op=1,
				    T step=1 )
			    : sd_(start,step), stop_(st_op)
			    , annotinsiderg_(false)
			    , annotinint_(false)	{}

			AxisLayout( const StepInterval<T>& rg )
			    : sd_(rg.start,rg.step)
			    , stop_(rg.stop)
			    , annotinsiderg_(false)
			    , annotinint_(false)    {}

			// Returns 'modulo' end with this sd_ and stop_
    T			findEnd(T datastop) const;
};


template <class T> inline
StepInterval<T> AxisLayout<T>::getSampling() const
{ return StepInterval<T>( sd_.start, stop_, sd_.step ); }


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

    double scwdth = wdth < 1e-30 ? -30 : log10( (double)wdth );
    int tenpow = 1 - (int)scwdth; if ( scwdth < 0 ) tenpow++;
    double stepfac = Math::IntPowerOf( ((double)10), tenpow );
    scwdth = wdth * stepfac;

    double scstep;
    if ( scwdth < 15 )          scstep = annotinint_ ? 2. : 2.5;
    else if ( scwdth < 30 )     scstep = 5;
    else if ( scwdth < 50 )     scstep = 10;
    else                        scstep = 20;

    sd_.step = (T) ( scstep / stepfac );
    if ( annotinint_ )
    {
	int istep = mNINT32( sd_.step );
	if ( istep == 0 )
	    istep = 1;
	sd_.step = (T)istep;
    }

    if ( mIsZero(sd_.step,mDefEps) )
	sd_.step = 1;

    if ( wdth > 1e-30 )
    {
	double idx0 = 0;
	if ( annotinsiderg_ )
	{
	    idx0 = rev ? Math::Floor(intv.stop / sd_.step + 1e-6)
		       : Math::Ceil(intv.start / sd_.step + 1e-6);
	}
	else
	{
	    idx0 = rev ? Math::Ceil(intv.stop / sd_.step + 1e-6)
		       : Math::Floor(intv.start / sd_.step + 1e-6);
	}

	sd_.start = mNINT32( idx0 ) * sd_.step;
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

    const double dnrsteps = double(datastop-worksd.start)/worksd.step - 1e-6;
    int nrsteps =
	mNINT32( (annotinsiderg_ ? Math::Floor(dnrsteps)
				 : Math::Ceil(dnrsteps)) );
    if ( nrsteps < 1 ) nrsteps = 1;
    T wdth = nrsteps * worksd.step;
    return sd_.start + (rev ? -wdth : wdth);
}
