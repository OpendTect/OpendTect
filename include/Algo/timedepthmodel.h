#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2007
________________________________________________________________________

-*/


#include "algomod.h"
#include "samplingdata.h"
#include "objectset.h"
#include "uistring.h"

class VelocityDesc;
template <class T> class ValueSeries;

/*!\brief Converts between time, depth and velocity given a model. The velocity
model can be either RMO-velocities in time, or interval velocity in either
depth or time. */

mExpClass(Algo) TimeDepthModel
{ mODTextTranslationClass(TimeDepthModel)
public:

			TimeDepthModel();
			TimeDepthModel(const TimeDepthModel&);
    virtual		~TimeDepthModel();

    TimeDepthModel&	operator=(const TimeDepthModel&);
    virtual bool	isOK() const;
    const uiString	errMsg() const;
    int			size() const		{ return sz_; }

    uiRetVal		setModel(const double* dpths,const double* times,
				 int sz);

    float		getDepth(float time) const;
    float		getTime(float depth) const;
    float		getVelocity(float depth) const;
    float		getFirstTime() const;
    float		getLastTime() const;

    static float	getDepth(const double* dpths,const double* times,int sz,
				 float time);
    static float	getTime(const double* dpths,const double* times,int sz,
				float depth);
    static float	getVelocity(const double* dpths,const double* times,
				    int sz,float depth);

    double		getDepthByIdx(int idx) const;
    double		getTimeByIdx(int idx) const;
    double		getDepth(int idx) const		    = delete;
    double		getTime(int idx) const		    = delete;

    void		shiftDepths(double);
    void		shiftTimes(double);

protected:

    static double	convertTo(const double* dpths,const double* times,
				  int sz,double z,bool targetistime);

    int			sz_		= 0;
    double*		times_		= 0;
    double*		depths_		= 0;

};


mExpClass(Algo) TimeDepthModelSet : public ObjectSet<TimeDepthModel>
{
    typedef TypeSet<float>  ZValueSet;

		    // In the following fns, input and output can be the same:
    void	    getTimes(const float* depths,float* times,int sz=-1) const;
    void	    getTimes(const ZValueSet& depths,ZValueSet& times) const;
    void	    getDepths(const float* times,float* depths,int sz=-1) const;
    void	    getDepths(const ZValueSet& times,ZValueSet& depths) const;

};



/*!\brief Converts between time and depth given a model. */

mExpClass(Algo) TimeDepthConverter : public TimeDepthModel
{ mODTextTranslationClass(TimeDepthConverter)
public:
			TimeDepthConverter();

    bool		isOK() const;
    static bool		isVelocityDescUseable(const VelocityDesc&,
					      bool velintime,
					      uiString* errmsg=0);

    uiRetVal		setVelocityModel(const ValueSeries<float>& vels,int sz,
					 const SamplingData<double>& sd,
					 const VelocityDesc&,bool istime);

    bool		calcDepths(ValueSeries<float>&,int sz,
				   const SamplingData<double>& timesamp) const;
    bool		calcTimes(ValueSeries<float>&,int sz,
				   const SamplingData<double>& depthsamp) const;

			// Pass the velocities as Vint in TIME
    static bool		calcDepths(const ValueSeries<float>& vels,int velsz,
				   const SamplingData<double>&,double* depths);
    static bool		calcDepths(const ValueSeries<float>& vels,int velsz,
				   const ValueSeries<double>& times,
				   double* depths);
    static bool		calcTimes(const ValueSeries<float>& vels,int velsz,
				  const ValueSeries<double>& depth,
				  double* times);
    static bool		calcTimes(const ValueSeries<float>& vels,int velsz,
				  const SamplingData<double>&,double* times);

protected:

    void		calcZ(const double*,int inpsz,
			      ValueSeries<float>&,int outpsz,
			      const SamplingData<double>&,bool istime) const;

    float		firstvel_;
    float		lastvel_;

    bool		regularinput_;
    int			sz_;
    SamplingData<double> sd_;

};
