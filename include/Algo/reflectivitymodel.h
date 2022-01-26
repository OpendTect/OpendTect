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
class ReflectivityModelBase;
template <class T> class RefObjectSet;


/*!
\brief A data container for reflection coefficients
*/

mClass(Algo) ReflectivityModelTrace : public ReferencedObject
{
public:
			ReflectivityModelTrace(int nrspikes);

    bool		isOK() const;
    int			size() const { return sz_; }

    bool		setSize(int sz,bool settonull=true);

    const float_complex* arr() const		{ return reflectivities_; }
    float_complex*	arr()			{ return reflectivities_; }

private:
			~ReflectivityModelTrace();

    int			sz_;
    float_complex*	reflectivities_ = nullptr;

    friend class ReflectivityModelBase;

	    ReflectivityModelTrace(const ReflectivityModelTrace&) = delete;
    void		operator= (const ReflectivityModelTrace&) = delete;
};


/*!
\brief A TimeDepth model set that includes reflectivities.
       Base class for offset and angle based reflectivity models
*/

mExpClass(Algo) ReflectivityModelBase : public TimeDepthModelSet
{
public:

    int			nrRefModels() const;
    int			nrLayers() const;
    int			nrSpikes() const;
    bool		hasAngles() const;
    bool		hasReflectivities() const;
    bool		isSpikeDefined(int ioff,int idz) const;

    float		getSinAngle(int ioff,int idz) const;
    const ReflectivityModelTrace* getReflectivities(int ioff) const;
    const float*	getReflTimes(int ioff=-1) const;
			//!< ioff=-1 for default TD model
    const float*	getReflDepths() const;

    virtual bool	isOffsetDomain() const		{ return false; }
    virtual bool	isAngleDomain() const		{ return false; }

protected:

    mExpClass(Algo) Setup : public TimeDepthModelSet::Setup
    {
    public:
			Setup( bool offsetdomain, bool withangles,
			       bool withreflectivity )
			    : TimeDepthModelSet::Setup()
			    , offsetdomain_(offsetdomain)
			    , withangles_(withangles)
			    , withreflectivity_(withreflectivity) {}
			~Setup()				  {}

	mDefSetupMemb(bool,offsetdomain);
	mDefSetupMemb(bool,withangles);
	mDefSetupMemb(bool,withreflectivity);

	void		fillPar(IOPar&) const override;
	bool		usePar(const IOPar&) override;
    };

			ReflectivityModelBase(const ElasticModel&,
				const ReflectivityModelBase::Setup&,
				const TypeSet<float>* axisvals =nullptr,
				float* velmax=nullptr);
			ReflectivityModelBase(const ElasticModel&,
				const TypeSet<float>& anglevals,
				const ReflectivityModelBase::Setup&);
			//!< Angle-based models only

    float*		getAngles(int ioff);
    ReflectivityModelTrace* getReflectivities(int ioff);
    float_complex*	getRefs(int ioff);

			~ReflectivityModelBase();

private:

    RefObjectSet<ReflectivityModelTrace>* reflectivities_ = nullptr;
    float*		sini_ = nullptr;
    float**		sinarr_ = nullptr;
    int			nroffs_;

    friend class RayTracer1D;

};


/*!
\brief An offset-based TimeDepth model set that may include
       reflectivities and incidence angles
*/

mExpClass(Algo) OffsetReflectivityModel : public ReflectivityModelBase
{
public:
    mExpClass(Algo) Setup : public ReflectivityModelBase::Setup
    {
    public:
			Setup(bool withangles,bool withreflectivity)
			    : ReflectivityModelBase::Setup(true,withangles,
							   withreflectivity)
			{}
    };

			OffsetReflectivityModel(const ElasticModel&,
				const OffsetReflectivityModel::Setup&,
				const TypeSet<float>* axisvals =nullptr,
				float* velmax =nullptr );

    bool		isOffsetDomain() const override { return true; }
};


/*!
\brief An angle-based TimeDepth model set that includes reflectivities,
       for a given azimuth and angle distributions
*/

mExpClass(Algo) AngleReflectivityModel : public ReflectivityModelBase
{
public:
    mExpClass(Algo) Setup : public ReflectivityModelBase::Setup
    {
    public:
			Setup( double azimuth=0. )
			    : ReflectivityModelBase::Setup(false,false,true)
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

			AngleReflectivityModel(const ElasticModel&,
				const TypeSet<float>& anglevals,
				const AngleReflectivityModel::Setup& =
				      AngleReflectivityModel::Setup());
			AngleReflectivityModel(const ElasticModel&,
				const TypeSet<float>& anglevals,double azi);

    bool		isAngleDomain() const override	{ return true; }

private:
    double		azimuth_;
    double		a0_;
    double		d0_;
    double		b0_;
};


mExpClass(Algo) ReflectivityModelSet : public ReferencedObject
{
public:
			ReflectivityModelSet(const IOPar&);

    bool		hasSameParams(const ReflectivityModelSet&) const;
    bool		hasSameParams(const IOPar&) const;
    bool		validIdx(int modlidx) const;
    int			nrModels() const;
    const ReflectivityModelBase* get(int modlidx) const;
    void		getOffsets(TypeSet<float>&) const;
    void		getTWTrange(Interval<float>&,bool zeroff=true) const;

    void		add(const ReflectivityModelBase&);
    void		use(const ObjectSet<const TimeDepthModel>&,
			    bool defonly=false);
    void		use(const TimeDepthModel&,int imdl,bool defonly=false);

private:
			~ReflectivityModelSet();

    IOPar&		createpars_;
    RefObjectSet<const ReflectivityModelBase>& refmodels_;

		    ReflectivityModelSet(const ReflectivityModelSet&) = delete;
    void		operator =( const ReflectivityModelSet&) = delete;
};

