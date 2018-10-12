#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jan 2011
________________________________________________________________________

-*/

#include "algomod.h"
#include "odcomplex.h"
#include "sets.h"
#include "manobjectset.h"
#include "typeset.h"


/*!
\brief A reflectivity spike.
*/

mClass(Algo) ReflectivitySpike
{
public:
			ReflectivitySpike()
			    : reflectivity_( mUdf(float), mUdf(float) )
			    , time_( mUdf(float) )
			    , correctedtime_( mUdf(float) )
			    , depth_( mUdf(float) )
			{}

    inline bool		isDefined() const;
    inline float	time(bool isnmo=true) const;

    inline bool		operator==(const ReflectivitySpike& s) const;
    inline bool		operator!=(const ReflectivitySpike& s) const;

    float_complex	reflectivity_;
    float		time_;
    float		correctedtime_; //!<Corrected for normal moveout
    float		depth_;
};


/*!\brief A table of reflectivies vs time and/or depth */

typedef TypeSet<ReflectivitySpike> ReflectivityModel;



mExpClass(Algo) ReflectivityModelSet
			: public ManagedObjectSet<ReflectivityModel>
			, public RefCount::Referenced
{
public:

    Interval<float>	getTimeRange(bool usenmo=false) const;

    static bool		getTimeRange(const ObjectSet<ReflectivityModel>&,
				   Interval<float>& sampling,bool usenmo=false);
};


//Implementations

inline bool ReflectivitySpike::operator==(const ReflectivitySpike& s) const
{
    return mIsEqualWithUdf( reflectivity_.real(),s.reflectivity_.real(),1e-5) &&
	   mIsEqualWithUdf( reflectivity_.imag(),s.reflectivity_.imag(),1e-5) &&
	   mIsEqualWithUdf( time_, s.time_, 1e-5 ) &&
	   mIsEqualWithUdf( correctedtime_, s.correctedtime_, 1e-5 ) &&
	   mIsEqualWithUdf( depth_, s.depth_, 1e-5 );
}

inline bool ReflectivitySpike::operator!=(const ReflectivitySpike& s) const
{ return !(*this==s); }


inline bool ReflectivitySpike::isDefined() const
{
    return !mIsUdf(reflectivity_) && !mIsUdf(time_) &&
	   !mIsUdf(correctedtime_) && !mIsUdf(depth_);
}


inline float ReflectivitySpike::time( bool isnmo ) const
{
    return isnmo ? correctedtime_ : time_;
}
