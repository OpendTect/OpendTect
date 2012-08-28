#ifndef reflectivitymodel_h
#define reflectivitymodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jan 2011
 RCS:		$Id: reflectivitymodel.h,v 1.5 2012-08-28 12:17:02 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "odcomplex.h"
#include "sets.h"


class ReflectivitySpike
{
public:
    			ReflectivitySpike()
			    : reflectivity_( mUdf(float), mUdf(float) )
			    , time_( mUdf(float) )
			    , correctedtime_( mUdf(float) )
			    , depth_( mUdf(float) )
			{}

    inline bool		operator==(const ReflectivitySpike& s) const;
    inline bool		operator!=(const ReflectivitySpike& s) const;

    float_complex	reflectivity_;
    float		time_;
    float		correctedtime_; //!<Corrected for normal moveout
    float		depth_;
};


/*!\brief A table of reflectivies vs time and/or depth */

typedef TypeSet<ReflectivitySpike> ReflectivityModel;


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


#endif

