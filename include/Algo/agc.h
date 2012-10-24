#ifndef agc_h
#define agc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "sorting.h"
#include "task.h"
#include "thread.h"
#include "valseries.h"

/*!Computes an AGC over a ValueSeries. */

template <class T>
class AGC : public ParallelTask
{
public:
    			AGC();
			~AGC();
    void		setInput(const ValueSeries<T>&,int sz);
    void		setOutput(ValueSeries<T>&);
    			//!<Output can be the same as input

    void		setSampleGate(const Interval<int>&);
    const Interval<int>& getSampleGate() const;

    void		setMuteFraction(float lvmf)	{ mutefraction_ = lvmf;}
    			//!<The lowest fraction will be muted
    float		getMuteFraction() const		{ return mutefraction_;}

    bool		doPrepare(int nrthreads);

protected:
    void		computeEnergyMute();
    bool		doWork(od_int64,od_int64,int);
    int			minThreadSize() const { return 200; }
    od_int64		nrIterations() const { return size_; }

    const ValueSeries<T>*	input_;
    od_int64			size_;
    ValueSeries<T>*		output_;
    Interval<int>		samplerg_;
    float			mutefraction_;
    TypeSet<T>			energies_;
    T				energymute_;

    int				threadsinenergycalc_;
    Threads::ConditionVar*	lock_;
};


template <class T> inline
AGC<T>::AGC()
    : input_( 0 )
    , output_( 0 )
    , samplerg_(-5,5)
    , lock_( 0 )
    , size_( 0 )
    , mutefraction_( 0 )
{}


template <class T> inline
AGC<T>::~AGC()
{ delete lock_; }


template <class T> inline
void AGC<T>::setInput( const ValueSeries<T>& nvs, int sz )
{ input_ = &nvs; size_ = sz; }


template <class T> inline
void AGC<T>::setOutput( ValueSeries<T>& nvs )
{ output_ = &nvs; }


template <class T> inline
void AGC<T>::setSampleGate( const Interval<int>& nrg )
{ samplerg_ = nrg; }


template <class T> inline
const Interval<int>& AGC<T>::getSampleGate() const
{ return samplerg_; }


template <class T> inline
bool AGC<T>::doPrepare( int nrthreads )
{
    if ( !input_ || !output_ ||
	 (output_->reSizeable() && !output_->setSize(size_)) )
	return false;

    energies_.setSize( size_, mUdf(T) );

    if ( nrthreads )
    {
	if ( !lock_ ) lock_ = new Threads::ConditionVar;
	threadsinenergycalc_ = nrthreads;
    }
    else
    { delete lock_; lock_ = 0; }

    return true;
}


template <class T> inline
void AGC<T>::computeEnergyMute()
{
    energymute_ = 0;
    if ( mIsUdf(mutefraction_) ||
	 mIsZero(mutefraction_,1e-5) )
	return;

    const int sample = mNINT32(size_*mutefraction_);
    if ( sample<0 || sample>=size_ )
	return;

    ArrPtrMan<T> energies = new T[size_];
    memcpy( energies.ptr(), energies_.arr(), size_*sizeof(T) );
    sortFor( energies.ptr(), size_, sample );
    energymute_ = energies[sample];
}


template <class T> inline
bool AGC<T>::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    for ( int idx=0; idx<=stop; idx++ )
    {
	const T value = input_->value(idx);
	energies_[idx] = mIsUdf( value ) ? mUdf(T) : value*value;
    }

    if ( lock_ )
    {
	lock_->lock();
	threadsinenergycalc_--;
	if ( !threadsinenergycalc_ )
	    lock_->signal(true);
	else while ( threadsinenergycalc_ )
	    lock_->wait();

	if ( !threadidx )
	{
	    computeEnergyMute();
	    lock_->signal(true);
	    threadsinenergycalc_--;
	}
	else while ( threadsinenergycalc_!=-1 )
	    lock_->wait();

	lock_->unLock();
    }
    else
    {
	computeEnergyMute();
    }

    for ( int idx=start; idx<=stop; idx++ )
    {
	int nrenergies = 0;
	float energysum = 0;
	for ( int energyidx=idx+samplerg_.start;
		  energyidx<=idx+samplerg_.stop;
		  energyidx++ )
	{
	    if ( energyidx<0 || energyidx>=size_ )
		continue;

	    const T energy = energies_[energyidx];
	    if ( mIsUdf(energy) )
		continue;

	    energysum += energy;
	    nrenergies++;
	}

	if ( nrenergies ) energysum /= nrenergies;

	float outputval = 0;
	if ( energysum>=energymute_ && energysum>0 )
	{
	    const T inpval = input_->value(idx);
	    outputval = mIsUdf( inpval ) 
		? inpval : inpval/Math::Sqrt( energysum );
	}

	output_->setValue( idx, outputval );
    }

    return true;
}

#endif
