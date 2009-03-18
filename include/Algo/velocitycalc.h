#ifndef velocitycalc_h
#define velocitycalc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Dec 2007
 RCS:		$Id: velocitycalc.h,v 1.11 2009-03-18 17:47:18 cvskris Exp $
________________________________________________________________________

-*/


#include "samplingdata.h"
#include "veldesc.h"

template <class T> class ValueSeries;


/*!Converts between time and depth given a velocity model. The velocity model
   can be either RMO-velocities in time, or interval velocity in either depth or
   time. */

mClass TimeDepthConverter
{
public:
    			TimeDepthConverter();
			~TimeDepthConverter();

    bool		isOK() const;

    const char*		errMsg() const;

    bool		setVelocityModel(const ValueSeries<float>&, int sz,
	    				 const SamplingData<double>&,
					 const VelocityDesc&,bool istime);

    bool		calcDepths(ValueSeries<float>&, int sz,
	    			   const SamplingData<double>& timesamp) const;
    bool		calcTimes(ValueSeries<float>&, int sz,
	    			   const SamplingData<double>& depthsamp) const;

    static bool		calcDepths(const ValueSeries<float>& vels, int velsz,
	    			   const SamplingData<double>&,
				   VelocityDesc::SampleSpan, float* depths );
    static bool		calcTimes(const ValueSeries<float>& vels, int velsz,
	    			   const SamplingData<double>&,
				   VelocityDesc::SampleSpan, float* depths );
protected:

    float			firstvel_;
    float			lastvel_;
    float*			depths_;
    float*			times_;
    int				sz_;
    SamplingData<double>	sd_;

    const char*			errmsg_;
};


/*! Computes moveout with anisotropy, according to the equation
by Alkhalifah and Tsvankin 1995. */

mGlobal bool computeMoveout( float t0, float Vrms, float effectiveanisotropy,
		     int nroffsets, const float* offsets, float* res );


/*!Converts a series of Vrms to Vint. Vrms may contain undefined values, as
   long as at least one is define. */

mGlobal bool computeDix(const float* Vrms,const SamplingData<double>& sd,
			int nrvels,VelocityDesc::SampleSpan,float* Vint);

/*!Converts a number of layers with Vrms to interval velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal bool computeDix(const float* Vrms, float t0, const float* t,
			int nrlayers, float* Vint);


mGlobal bool computeVrms(const float* Vint,const SamplingData<double>& sd,
			 int nrvels, VelocityDesc::SampleSpan,float* Vrms);

/*!Converts a number of layers with Vint to rms velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal bool computeVrms(const float* Vint,float t0,const float* t,int nrlayers,
		         float* Vrms);

/*!Given an irregularly sampled Vrms, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal bool sampleVrms(const float* Vin,float t0_in,const float* t_in,
			int nr_in, const SamplingData<double>& sd_out,
			float* Vout, int nr_out);


/*!Given an irregularly sampled Vint, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal bool sampleVint(const float* Vint,const float* t_in, int nr_in,
			VelocityDesc::SampleSpan inputspan,
			const SamplingData<double>& sd_out, float* Vout,
			int nr_out);
/*!Given a residual moveout at a reference offset, comput the residual moveout
 *    at other offsets */

mGlobal void computeResidualMoveouts( float z0, float rmo, float refoffset,
				      int nroffsets, bool outputdepth,
				      const float* offsets, float* output );

	        
	        
#endif
