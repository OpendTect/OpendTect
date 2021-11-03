#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2007
________________________________________________________________________

-*/


#include "algomod.h"
#include "factory.h"
#include "keystrs.h"
#include "veldesc.h"
#include "samplingdata.h"

#include "uistring.h"

class Scaler;
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

    static float	convertTo(const float* dpths,const float* times,int sz,
				    float z,bool targetistime);

    int		sz_;

    float*		times_;
    float*		depths_;

    uiString		errmsg_;
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

    bool		isOK() const;
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
protected:

    void		calcZ(const float*,int inpsz,
				ValueSeries<float>&,int outpsz,
				const SamplingData<double>&,bool istime) const;

    float		firstvel_;
    float		lastvel_;

    bool		regularinput_;
    int			sz_;
    SamplingData<double> sd_;
};


/*!
\brief Base class for computing a moveout curve.
*/

mExpClass(Algo) MoveoutComputer
{ mODTextTranslationClass(MoveoutComputer)
public:
    virtual		~MoveoutComputer()		{}

    virtual int		nrVariables() const				= 0;
    virtual const char* variableName(int) const				= 0;

    virtual bool	computeMoveout(const float* variables,
					     int nroffsets,
					     const float* offsets,
					     float* res) const		= 0;
    float		findBestVariable(float* variables, int variabletochange,
			    const Interval<float>& searchrg,int nroffsets,
			    const float* offsets, const float* moveout ) const;
			/*!<On success, rms error will be returned, otherwise
			    mUdf(float). On success variables[variabletochang]
			    will be set to the best fit. */
};


/*!
\brief Computes moveout in depth from RMO at a certain reference offset.
*/

mExpClass(Algo) RMOComputer : public MoveoutComputer
{ mODTextTranslationClass(RMOComputer)
public:
    int nrVariables() const	{ return 3; }
    const char* variableName(int idx) const
		{
		    switch ( idx )
		    {
			case 0: return sKey::Depth();
			case 1: return "RMO";
			case 2: return "Reference offset";
		    };

		    return 0;
		}
    bool	computeMoveout(const float*,int,const float*,float*) const;
    static bool computeMoveout(float d0, float rmo, float refoffset,
			       int,const float*,float*);
};


/*!
\brief Computes moveout with anisotropy, according to the equation
by Alkhalifah and Tsvankin 1995.
*/

mExpClass(Algo) NormalMoveout : public MoveoutComputer
{ mODTextTranslationClass(NormalMoveout)
public:
    int nrVariables() const	{ return 3; }
    const char* variableName( int idx ) const
		{
		    switch ( idx )
		    {
			case 0: return sKey::Time();
			case 1: return "Vrms";
			case 2: return "Effective anisotrophy";
		    };

		    return 0;
		}
    bool	computeMoveout(const float*,int,const float*,float*) const;
    static bool computeMoveout(float t0, float Vrms, float effectiveanisotropy,
			       int,const float*,float*);
};

/*!Converts a number of layers with Vrms to interval velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeDix(const float* Vrms, double t0, float v0,
			const double* t, int nrlayers, float* Vint);

mDeprecatedDef
mGlobal(Algo) bool computeDix(const float* Vrms, float t0, float v0,
			const float* t, int nrlayers, float* Vint);

/*!
\brief Rms velocity to interval velocity conversion.
*/

mExpClass(Algo) Vrms2Vint
{ mODTextTranslationClass(Vrms2Vint)
public:
			mDefineFactoryInClass( Vrms2Vint, factory );
    virtual		~Vrms2Vint()	{}

    virtual bool	compute(const float* Vrms, float t0, float v0,
				const float* t, int nrlayers, float* Vint) = 0;
};


/*!
\brief Rms velocity to interval velocity conversion using the Dix formula.
*/

mExpClass(Algo) DixConversion : public Vrms2Vint
{ mODTextTranslationClass(DixConversion)
public:
		mDefaultFactoryInstantiation( Vrms2Vint, DixConversion,
					      "Dix",
					      toUiString(sFactoryKeyword()));

    bool	compute(const float* Vrms, double t0, float v0,
			const double* t, int nrlayers, float* Vint)
		{ return computeDix( Vrms, t0, v0, t, nrlayers, Vint ); }

mStartAllowDeprecatedSection
    bool	compute(const float* Vrms, float t0, float v0,
			const float* t, int nrlayers, float* Vint)
		{ return computeDix( Vrms, t0, v0, t, nrlayers, Vint ); }
mStopAllowDeprecatedSection
};



/*!Converts a series of Vrms to Vint. Vrms may contain undefined values, as
   long as at least one is defined. */

mGlobal(Algo) bool computeDix(const float* Vrms,const SamplingData<double>& sd,
			int nrvels,float* Vint);

/*!Converts a number of layers with Vrms to interval velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeDix(const float* Vrms, double t0, float v0,
				const double* t, int nrlayers, float* Vint);

mDeprecatedDef
mGlobal(Algo) bool computeDix(const float* Vrms, float t0, float v0,
				const float* t, int nrlayers, float* Vint);


/*! Be very careful when using this one: the input Vint has to be regularly
  sampled according to sd. In case not, use the next one.*/
mGlobal(Algo) bool computeVrms(const float* Vint,const SamplingData<double>& sd,
			 int nrvels, float* Vrms);

/*!Converts a number of layers with Vint to rms velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeVrms(const float* Vint,double t0,const double* t,
			       int nrlayers, float* Vrms);

mDeprecatedDef
mGlobal(Algo) bool computeVrms(const float* Vint,float t0,const float* t,
				int nrlayers, float* Vrms);


/*!Given an irregularly sampled Vrms, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal(Algo) bool sampleVrms(const float* Vin,double t0_in,float v0_in,
			const double* t_in, int nr_in,
			const SamplingData<double>& sd_out,
			float* Vout, int nr_out);

mDeprecatedDef
mGlobal(Algo) bool sampleVrms(const float* Vin,float t0_in,
			float v0_in,const float* t_in, int nr_in,
			const SamplingData<double>& sd_out,
			float* Vout, int nr_out);

//!Converts a number of layers with Vint to average velocities.

mGlobal(Algo) bool computeVavg(const float* Vint,const double* t,int nrvels,
			       float* Vavg);

mDeprecatedDef
mGlobal(Algo) bool computeVavg(const float* Vint, float t0,
				const float* t, int nrvels, float* Vavg);

//!Converts a number of layers with Vavg to Vint velocities.

mGlobal(Algo) bool computeVint(const float* Vavg,const double* t,int nrvels,
			       float* Vint);

mDeprecatedDef
mGlobal(Algo) bool computeVint(const float* Vavg, float t0,
				const float* t, int nrvels, float* Vint);


/*!Given an irregularly sampled Vint, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal(Algo) bool sampleVint(const float* Vint,const double* t_in,int nr_in,
			const SamplingData<double>& sd_out,float* Vout,
			 int nr_out);

mDeprecatedDef
mGlobal(Algo) bool sampleVint(const float* Vint,const float* t_in,
			int nr_in,
			const SamplingData<double>& sd_out, float* Vout,
			int nr_out);

/*!Given an irregularly sampled Vavg, create a regularly sampled one. The
   function assumes constant average velocity before and after the input
   interval.*/

mGlobal(Algo) bool sampleVavg(const float* Vavg,const double* t_in,int nr_in,
			const SamplingData<double>& sd_out,float* Vout,
			int nr_out);

mDeprecatedDef
mGlobal(Algo) bool sampleVavg(const float* Vavg, const float* t_in,
			int nr_in,
			const SamplingData<double>& sd_out, float* Vout,
			int nr_out);

/*!Given a residual moveout at a reference offset, comput the residual moveout
   at other offsets */

mGlobal(Algo) void computeResidualMoveouts(float z0,float rmo,float refoffset,
				      int nroffsets,bool outputdepth,
				      const float* offsets,float* output);

/*!Given a layered V_int model (in time or depth), compute the best fit for a
   V_int = V_0 + gradient * (z-reference_z). The fit is such that the time/depth
   pairs at the layer's boundary will be preserved. */
mGlobal(Algo) bool fitLinearVelocity( const float* Vint, const float* z_in,
			      int nr_in, const Interval<float>& zlayer,
			      float reference_z, bool zisdepth, float& V_0,
			      float& gradient, float& error);


/*!Given an irregularly sampled depth or time array, create a regularly sampled
  one. The function assumes initial depth and time are 0.
  if zarr is time, tord_in is corresponding depth and other way round */

mGlobal(Algo) void resampleZ(const double* zarr,const double* tord_in,int nr_in,
			const SamplingData<double>& sd_out,int nr_out,
			double* zsampled);

mDeprecatedDef
mGlobal(Algo) void resampleZ(const float* zarr,const float* tord_in,
			int nr_in,
			const SamplingData<double>& sd_out, int nr_out,
			float* zsampled);


/*!Given an irregularly sampled effective Thomsen parameter array, create a
  regularly sampled one. The function assumes constant value of the parameter
  before and after the input interval.*/
mGlobal(Algo) void sampleEffectiveThomsenPars(const float* vinarr,
	const double* t_in,int nr_in,const SamplingData<double>& sd_out,
	int nr_out,float* voutarr);

mDeprecatedDef
mGlobal(Algo) void sampleEffectiveThomsenPars(const float* vinarr,
	const float* t_in,int nr_in,const SamplingData<double>& sd_out,
	int nr_out,float* voutarr);


/*!Given an irregularly sampled interval Thomsen parameter array, create a
  regularly sampled one. The function assumes constant value of the parameter
  before and after the input interval.*/
mGlobal(Algo) void sampleIntvThomsenPars(const float* inarr,const double* t_in,
				int nr_in,const SamplingData<double>& sd_out,
				int nr_out,float* outarr);

mDeprecatedDef
mGlobal(Algo) void sampleIntvThomsenPars(const float* inarr,
				const float* t_in,int nr_in,
				const SamplingData<double>& sd_out,int nr_out,
				float* outarr);


/* Utility function for Depth and effective Thomsen parameters resampling*/
mGlobal(Algo) void resampleContinuousData(const double* in,const double* t_in,
				int nr_in,const SamplingData<double>& sd_out,
				int nr_out,double* outarr);

mDeprecatedDef
mGlobal(Algo) void resampleContinuousData(const float* inarr,
				const float* t_in,
				int nr_in,const SamplingData<double>& sd_out,
				int nr_out,float* outarr);


/*!Compute depth values for the times in timesampling, using v0 and dv. v0 is
   the interval velocity at depth v0depth. v0depth is also the depth at t=0. */

mGlobal(Algo) bool computeLinearT2D( double v0, double dv, double v0depth,
				     const SamplingData<float>& timesampling,
				     int sz, float* res );

/*!Compute time values for the depths in depthsampling, using v0 and dv. v0 is
   the interval velocity at depth v0depth. v0depth is also the depth at t=0. */

mGlobal(Algo) bool computeLinearD2T( double v0, double dv, double v0depth,
		      const SamplingData<float>& depthsampling,
		      int sz, float* res );


mGlobal(Algo) bool convertToVintIfNeeded(const float* inpvel,
					const VelocityDesc& veldesc,
					const StepInterval<float>& zrange,
					float* outvel);

mGlobal(Algo) SamplingData<double> getDoubleSamplingData(
						    const SamplingData<float>&);

