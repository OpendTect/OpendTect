#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "odcomplex.h"
#include "timedepthmodel.h"

class RayTracer1D;
class ReflCalc1D;
class ReflectivityModelBase;
template <class T> class RefObjectSet;


/*!
\brief A data container for reflection coefficients
*/

mExpClass(Algo) ReflectivityModelTrace : public ReferencedObject
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
			mOD_DisableCopy(ReflectivityModelTrace);

    int			sz_;
    float_complex*	reflectivities_ = nullptr;

    friend class ReflectivityModelBase;

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
			Setup(bool offsetdomain,bool withangles,
			      bool withreflectivity);
			~Setup();

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
    friend class ReflCalc1D;

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
			Setup(bool withangles,bool withreflectivity);
			~Setup();

	Setup&		offsettype(Seis::OffsetType);
	Seis::OffsetType offsettype_	= Seis::OffsetType::OffsetMeter;

	bool		areOffsetsInFeet() const;
    };

			OffsetReflectivityModel(const ElasticModel&,
				const OffsetReflectivityModel::Setup&,
				const TypeSet<float>* axisvals =nullptr,
				float* velmax =nullptr);
			~OffsetReflectivityModel();

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
			Setup(double azimuth=0.);
			~Setup();

	mDefSetupMemb(double,azimuth);
	mDefSetupMemb(double,a0);	    //<! Average Vp (m/s) - def 2500
	mDefSetupMemb(double,d0);	    //<! Average Rhob (m/s) - def 2000
	mDefSetupMemb(double,b0);	    //<! Average Vs (m/s) - def 1500

	void		fillPar(IOPar&) const override;
	bool		usePar(const IOPar&) override;
    };

			AngleReflectivityModel(const ElasticModel&,
				const TypeSet<float>& anglevals,
				const AngleReflectivityModel::Setup& =
				      AngleReflectivityModel::Setup());
			AngleReflectivityModel(const ElasticModel&,
				const TypeSet<float>& anglevals,double azi);
			~AngleReflectivityModel();

    bool		isAngleDomain() const override	{ return true; }

    double		getAzimuth() const		{ return azimuth_; }
    double		getMeanRhob() const		{ return d0_; }
    double		getMeanVp() const		{ return a0_; }
    double		getMeanVs() const		{ return b0_; }

    static const char* sKeyMeanRhob();
    static const char* sKeyMeanVp();
    static const char* sKeyMeanVs();

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
    bool		getGatherXAxis(TypeSet<float>&) const;
    void		getTWTrange(Interval<float>&,bool zeroff=true) const;

    void		add(const ReflectivityModelBase&);
    void		use(const ObjectSet<const TimeDepthModel>&,
			    bool defonly=false);
    void		use(const TimeDepthModel&,int imdl,bool defonly=false);

private:
			~ReflectivityModelSet();
			mOD_DisableCopy(ReflectivityModelSet);

    bool		getAngles(TypeSet<float>&) const;
    bool		getOffsets(TypeSet<float>&) const;

    IOPar&		createpars_;
    RefObjectSet<const ReflectivityModelBase>& refmodels_;

};
