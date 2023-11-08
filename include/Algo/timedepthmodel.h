#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "odcommonenums.h"
#include "refcount.h"
#include "uistring.h"

class ElasticModel;
class RayTracer1D;
class ReflCalc1D;
class TimeDepthModelSet;
template <class T> class SamplingData;
template <class T> class ValueSeries;

/*!
\brief Converts between time, depth and velocity given a model. The velocity
model can be either RMO-velocities in time, or interval velocity in either
depth or time.
The times are always TWT, in SI units (seconds).
The depths units correspond to SI().depthsInFeet(), and are TVDSS depths
(0 at sea-level, not at SRD, positive below sea-level and increasing downwards.
*/

mExpClass(Algo) TimeDepthModel
{ mODTextTranslationClass(TimeDepthModel)
public:
			TimeDepthModel();
			TimeDepthModel(const TimeDepthModel&);
    virtual		~TimeDepthModel();

    TimeDepthModel&	operator=(const TimeDepthModel&);
    virtual bool	isOK() const;
    const char*		errMsg() const;
    int			size() const	{ return sz_; }

    bool		setModel(const float* dpths,const float* times,int sz);

    float		getDepth(float time) const;
    float		getTime(float depth) const;
    float		getVelocity(float depth) const;
    float		getFirstTime() const;
    float		getLastTime() const;

    static float	getDepth(const float* dpths,const float* times,int sz,
				float time);
    static float	getTime(const float* dpths,const float* times,int sz,
				float depth);
    static float	getVelocity(const float* dpths,const float* times,
					int sz,float depth);

			// use only if you're sure about what you're doing
    float		getDepth(int idx) const;
    float		getTime(int idx) const;

protected:

    uiString		errmsg_;

    float*		getTimes()		{ return times_; }
    float*		getDepths()		{ return depths_; }

    void		setEmpty();
    void		setAllVals(float* dpths,float* times,int sz);
			//<! Both arrays become mine
    void		forceTimes(const TimeDepthModel&);
    void		setVals(float*,bool isdepth,bool becomesmine=true);
    void		setSize( int sz )	{ sz_ = sz; }

    static float	convertTo(const float* dpths,const float* times,int sz,
				    float z,bool targetistime);

private:

    int			sz_ = 0;
    float*		times_ = nullptr;
    float*		depths_ = nullptr;
    bool		owndepths_ = true;

    friend class RayTracer1D;
    friend class ReflCalc1D;
    friend class TimeDepthModelSet;

public:

    const float*	getTimes() const	{ return times_; }
    const float*	getDepths() const	{ return depths_; }

};


/*!
\brief Data holder for all TimeDepthModels that share the same
       depths distributions. There will always be at least one model in the set.
       Models may be annotated by a given value, typically offset or angle
       See the TimeDepthModel class for a description of the units of measure
*/

mExpClass(Algo) TimeDepthModelSet : public ReferencedObject
{ mODTextTranslationClass(TimeDepthModelSet)
public:

    mExpClass(Algo) Setup
    {
    public:
			Setup();
	virtual		~Setup();

	mDefSetupMemb(bool,pdown);
	mDefSetupMemb(bool,pup);
	mDefSetupMemb(float,starttime);
	mDefSetupMemb(float,startdepth);
	mDefSetupMemb(ZDomain::DepthType,depthtype);

	virtual void	fillPar(IOPar&) const;
	virtual bool	usePar(const IOPar&);

	bool		areDepthsInFeet() const;
    };

			TimeDepthModelSet(const ElasticModel&,
				const Setup& =Setup(),
				const TypeSet<float>* axisvals =nullptr,
				float* velmax=nullptr);
			TimeDepthModelSet(const TimeDepthModel&,
				const TypeSet<float>* axisvals =nullptr);

    virtual bool	isOK() const;
    int			nrModels() const;
    int			size() const		{ return nrModels(); }
    int			modelSize() const;

    const TimeDepthModel& getDefaultModel() const;
    const TimeDepthModel* get(int) const;

    void		setDepth(int idz,float);
    void		setDefTWT(int idz,float);
    void		setTWT(int imdl,int idz,float);
    void		forceTimes(const TimeDepthModel&,bool defonly);

protected:

    virtual		~TimeDepthModelSet();

    bool		isbad_ = false;

private:
			TimeDepthModelSet(int modelsz,
				const TypeSet<float>* axisvals =nullptr);
			mOD_DisableCopy(TimeDepthModelSet);

    void		init(int modelsz,const TypeSet<float>* axisvals);
    void		setFrom(const ElasticModel&,const Setup&,
				float* velmax);

    TimeDepthModel&	getDefaultModel();
    TimeDepthModel*	get(int);

    ObjectSet<TimeDepthModel> tdmodels_;
    TimeDepthModel*	defmodel_ = nullptr;
    bool		singleton_ = true;

   friend class RayTracer1D;
   friend class ReflCalc1D;
};
