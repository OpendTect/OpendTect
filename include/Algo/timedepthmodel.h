#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2007
________________________________________________________________________

-*/


#include "algomod.h"
#include "refcount.h"
#include "uistring.h"

class ElasticModel;
class RayTracer1D;
class Scaler;
class TimeDepthModelSet;
class VelocityDesc;
template <class T> class SamplingData;
template <class T> class ValueSeries;

/*!
\brief Converts between time, depth and velocity given a model. The velocity
model can be either RMO-velocities in time, or interval velocity in either
depth or time.
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
    friend class TimeDepthModelSet;

public:

    const float*	getTimes() const	{ return times_; }
    const float*	getDepths() const	{ return depths_; }

};


/*!
\brief Data holder for all TimeDepthModels that share the same
       depths distributions. There will always be at least one model in the set.
       Models may be annotated by a given value, typically offset or angle
*/

mExpClass(Algo) TimeDepthModelSet : public ReferencedObject
{ mODTextTranslationClass(TimeDepthModelSet)
public:

    mExpClass(Algo) Setup
    {
    public:
			Setup()
			    : pdown_(true)
			    , pup_(true)
			    , starttime_(0.f)
			    , startdepth_(0.f)	    {}
	virtual		~Setup() {}

	mDefSetupMemb(bool,pdown);
	mDefSetupMemb(bool,pup);
	mDefSetupMemb(float,starttime);
	mDefSetupMemb(float,startdepth);

	virtual void	fillPar(IOPar&) const;
	virtual bool	usePar(const IOPar&);
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

protected:

    virtual		~TimeDepthModelSet();

    bool		isbad_ = false;

private:
			TimeDepthModelSet(int modelsz,
				const TypeSet<float>* axisvals =nullptr);

    void		init(int modelsz,const TypeSet<float>* axisvals);
    void		setFrom(const ElasticModel&,const Setup&,
				float* velmax);

    TimeDepthModel&	getDefaultModel();
    TimeDepthModel*	get(int);

    ObjectSet<TimeDepthModel> tdmodels_;
    TimeDepthModel*	defmodel_ = nullptr;
    bool		singleton_ = true;

			TimeDepthModelSet(const TimeDepthModelSet&) = delete;
   TimeDepthModelSet&	operator =(const TimeDepthModelSet&) = delete;

   friend class RayTracer1D;
};


/*!
\brief Converts between time and depth given a model.
It expects a valueseries, where unit of value should be SI unit.
Scaler provides factor value in case the valueseries in non SI unit.
*/

mExpClass(Algo) TimeDepthConverter : public TimeDepthModel
{ mODTextTranslationClass(TimeDepthConverter)
public:
			TimeDepthConverter();
			~TimeDepthConverter();

    bool		isOK() const override;
    static bool		isVelocityDescUseable(const VelocityDesc&,
					      bool velintime,
					      uiString* errmsg = 0);

    bool		setVelocityModel(const ValueSeries<float>& vels, int sz,
					 const SamplingData<double>& sd,
					 const VelocityDesc&,bool istime,
					 const Scaler* scaler=nullptr);

    bool		calcDepths(ValueSeries<float>&, int sz,
				   const SamplingData<double>& timesamp) const;
    bool		calcTimes(ValueSeries<float>&, int sz,
				   const SamplingData<double>& depthsamp) const;

    static bool		calcDepths(const ValueSeries<float>& vels,int velsz,
				   const SamplingData<double>&,float* depths,
				   const Scaler* scaler=nullptr);
			/*!<\param vels Velocity as Vint in time
			  \param velsz,depths
			 */

    static bool		calcDepths(const ValueSeries<float>& vels,int velsz,
				   const ValueSeries<double>& times,
				   double* depths,const Scaler* scaler=nullptr);
			 /*!<\param vels Velocity as Vint in time
			   \param velsz,times,depths
			  */

			mDeprecatedDef
    static bool		calcDepths(const ValueSeries<float>& vels,
				   int velsz,const ValueSeries<float>& times,
				   float* depths);

    static bool		calcTimes(const ValueSeries<float>& vels,int velsz,
				  const ValueSeries<float>& depth,float* times,
				  const Scaler* scaler=nullptr);
			 /*!<\param vels Velocity as Vint in depth
			   \param velsz,times,depth
			  */

    static bool		calcTimes(const ValueSeries<float>& vels, int velsz,
				   const SamplingData<double>&, float* times,
				   const Scaler* scaler=nullptr);
			 /*!<\param vels Velocity as Vint in depth
			   \param velsz,times
			  */
private:

    void		calcZ(ValueSeries<float>&,int outpsz,
			      const SamplingData<double>&,bool istime) const;

    float		firstvel_;
    float		lastvel_;

    bool		regularinput_ = true;
    SamplingData<double>& sd_;
};
