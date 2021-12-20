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
#include "timedepthmodel.h"

class RayTracer1D;
class ReflectivityModelSet;


/*!
\brief A data container for reflection coefficients and optionally
	uncorrected nmo times
*/

mClass(Algo) ReflectivityModelTrace
{
public:
			ReflectivityModelTrace(int nrspikes);
			~ReflectivityModelTrace();

    bool		isOK() const;

    const float_complex* getReflectivities() const  { return reflectivities_; }

private:

    float_complex*	reflectivities_;
    float_complex*	getReflectivities() { return reflectivities_; }

    friend class ReflectivityModelSet;

	    ReflectivityModelTrace(const ReflectivityModelTrace&) = delete;
    void		operator= (const ReflectivityModelTrace&) = delete;
};


/*!
\brief A TimeDepth model set that includes reflectivities.
       Base class for offset and angle based reflectivity models
*/

mExpClass(Algo) ReflectivityModelSet : public TimeDepthModelSet
{
public:

    int			nrRefModels() const;
    bool		isDefined(int imdl,int idz) const;

protected:

    mExpClass(Algo) Setup : public TimeDepthModelSet::Setup
    {
    public:
			Setup( bool offsetdomain )
			    : TimeDepthModelSet::Setup()
			    , offsetdomain_(offsetdomain)   {}
			~Setup()			    {}

	mDefSetupMemb(bool,offsetdomain);

	void		fillPar(IOPar&) const override;
	bool		usePar(const IOPar&) override;
    };

			ReflectivityModelSet(const ElasticModel&,
				const ReflectivityModelSet::Setup&,
				const TypeSet<float>* axisvals =nullptr,
				float* velmax=nullptr);
			ReflectivityModelSet(const ElasticModel&,
				const TypeSet<float>& anglevals,
			        const ReflectivityModelSet::Setup&);
			//!< Angle-based models only

    float_complex*	getRefs(int imdl);

			~ReflectivityModelSet();

private:

    ObjectSet<ReflectivityModelTrace>  reflectivities_;

    friend class RayTracer1D;
};


/*!
\brief An offset-based TimeDepth model set that includes reflectivities.
*/

mExpClass(Algo) OffsetReflectivityModelSet : public ReflectivityModelSet
{
public:
    mExpClass(Algo) Setup : public ReflectivityModelSet::Setup
    {
    public:
			Setup()
			    : ReflectivityModelSet::Setup(true)
			{}
    };

			OffsetReflectivityModelSet(const ElasticModel&,
				const OffsetReflectivityModelSet::Setup&,
				const TypeSet<float>* axisvals =nullptr,
				float* velmax =nullptr );
};


/*!
\brief An angle-based TimeDepth model set that includes reflectivities,
       for a given azimuth and angle distributions
*/

mExpClass(Algo) AngleReflectivityModelSet : public ReflectivityModelSet
{
public:
    mExpClass(Algo) Setup : public ReflectivityModelSet::Setup
    {
    public:
			Setup( double azimuth=0. )
			    : ReflectivityModelSet::Setup(false)
			    , azimuth_(azimuth)
			    , a0_(2500.)
			    , d0_(2000.)
			    , b0_(1500.)	{}
			~Setup() {}

	mDefSetupMemb(double,azimuth);
	mDefSetupMemb(double,a0);	    //<! Average Vp (m/s)
	mDefSetupMemb(double,d0);	    //<! Average Rhob (m/s)
	mDefSetupMemb(double,b0);	    //<! Average Vs (m/s)

	void		fillPar(IOPar&) const override;
	bool		usePar(const IOPar&) override;
    };

			AngleReflectivityModelSet(const ElasticModel&,
				const TypeSet<float>& anglevals,
				const AngleReflectivityModelSet::Setup& =
				      AngleReflectivityModelSet::Setup());
			AngleReflectivityModelSet(const ElasticModel&,
				const TypeSet<float>& anglevals,double azi);

private:
    double		azimuth_;
    double		a0_;
    double		d0_;
    double		b0_;
};


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

