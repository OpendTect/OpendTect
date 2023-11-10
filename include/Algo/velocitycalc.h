#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "factory.h"
#include "keystrs.h"
#include "odcommonenums.h"
#include "uistring.h"

class VelocityDesc;
class ZValueSeries;
namespace ZDomain { class Info; }
template <class T> class ValueSeries;


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
    int nrVariables() const override		{ return 3; }
    const char* variableName(int idx) const override
		{
		    switch ( idx )
		    {
			case 0: return sKey::Depth();
			case 1: return "RMO";
			case 2: return "Reference offset";
		    };

		    return nullptr;
		}
    bool	computeMoveout(const float*,int,
			       const float*,float*) const override;
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
    int nrVariables() const override		{ return 3; }
    const char* variableName( int idx ) const override
		{
		    switch ( idx )
		    {
			case 0: return sKey::Time();
			case 1: return "Vrms";
			case 2: return "Effective anisotrophy";
		    };

		    return nullptr;
		}
    bool	computeMoveout(const float*,int,
			       const float*,float*) const override;
    static bool computeMoveout(float t0, float Vrms, float effectiveanisotropy,
			       int,const float*,float*);
};


namespace Vel
{

/*!Given an array of interval velocities and corresponding times, compute the
  associated depth values. Velocities must be interval velocities in m/s,
  times must be as TWT in seconds.
  depths output values as TVDSD in meters */

mGlobal(Algo) bool calcDepthsFromVint(const ValueSeries<double>& Vint,
				      const ZValueSeries& times,
				      ValueSeries<double>& depths);


/*!Given an array of average velocities and corresponding times, compute the
  associated depth values. Velocities must be average velocities in m/s,
  times must be as TWT in seconds.
  depths output values as TVDSD in meters */

mGlobal(Algo) bool calcDepthsFromVavg(const ValueSeries<double>& Vavg,
				      const ZValueSeries& times,
				      ValueSeries<double>& depths);


/*!Given an array of RMS velocities and corresponding times, compute the
  associated depth values. Velocities must be RMS velocities in m/s,
  times must be as TWT in seconds.
  depths output values as TVDSD in meters */

mGlobal(Algo) bool calcDepthsFromVrms(const ValueSeries<double>& Vrms,
				      const ZValueSeries& times,
				      ValueSeries<double>& depths,
				      double t0=0.);


/*!Given an array of interval velocities and corresponding depths, compute the
  associated time values. Velocities must be interval velocities in m/s,
  depths values must be as TVDSD in meters
  times out values as TWT in seconds. */

mGlobal(Algo) bool calcTimesFromVint(const ValueSeries<double>& Vint,
				     const ZValueSeries& depths,
				     ValueSeries<double>& times);


/*!Given an array of average velocities and corresponding depths, compute the
  associated time values. Velocities must be average velocities in m/s,
  depths values must be as TVDSD in meters
  times out values as TWT in seconds. */

mGlobal(Algo) bool calcTimesFromVavg(const ValueSeries<double>& Vavg,
				     const ZValueSeries& depths,
				     ValueSeries<double>& times);


/*!Given an array of velocities and the corresponding times or depths,
  calculate a regularly sampled array of times or depth
  Velocities must be in m/s,
  input/output times values as TWT in seconds.
  input/output depths values as TVDSD in meters. */

mGlobal(Algo) bool getSampledZ(const ValueSeries<double>& vels,
			       const ZValueSeries& zvals_in,OD::VelocityType,
			       const ZValueSeries& zvals_out,
			       ValueSeries<double>& Zout,double t0=0.);


/*!Compute depth values for the times array using a V0/K function
   v0 is velocity at surface datum, in SI units (m/s)
   k is the velocity gradient with depth (always in s-1 thus) */

mGlobal(Algo) bool calcDepthsFromLinearV0k(double v0,double k,
					   const ZValueSeries& times,
					   ValueSeries<double>& depths);


/*!Compute time values for the depths array using a V0/K function
   v0 is velocity at surface datum, in SI units (m/s)
   k is the velocity gradient with depth (always in s-1 thus) */

mGlobal(Algo) bool calcTimesFromLinearV0k(double v0,double k,
					  const ZValueSeries& depths,
					  ValueSeries<double>& times);


/*!Given a layered V_int model (in time or depth), compute the best fit for a
   V_int = V_0 + gradient * (z-reference_z). The fit is such that the time/depth
   pairs at the layer's boundary will be preserved. */

mGlobal(Algo) bool fitLinearVelocity(const ValueSeries<double>& Vint,
				     const ZValueSeries&,
				     const ::Interval<double>& zlayer,
				     double reference_z,double& V_0,
				     double& gradient,double& error);


//<!Converts a number of layers with Vint to average velocities.

mGlobal(Algo) bool computeVavg(const ValueSeries<double>& Vint,
			       const ZValueSeries&,
			       ValueSeries<double>& Vavg);


/*!Converts a number of layers with Vint to rms velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeVrms(const ValueSeries<double>& Vint,
			       const ZValueSeries&,
			       ValueSeries<double>& Vrms,double t0=0.);


//!Converts a number of layers with Vavg to Vint velocities.

mGlobal(Algo) bool computeVint(const ValueSeries<double>& Vavg,
			       const ZValueSeries&,ValueSeries<double>& Vint);


/*!Converts a number of layers with Vrms to interval velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeDix(const ValueSeries<double>& Vrms,
			      const ZValueSeries&,ValueSeries<double>& Vint,
			      double t0=0.);


/*!Given an irregularly sampled Vint, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval. */

mGlobal(Algo) bool sampleVint(const ValueSeries<double>& Vin,
			     const ZValueSeries& z_in,const ZValueSeries& z_out,
			     ValueSeries<double>& Vout);


/*!Given an irregularly sampled Vavg, create a regularly sampled one. The
   function assumes constant average velocity before and after the input
   interval. */

mGlobal(Algo) bool sampleVavg(const ValueSeries<double>& Vin,
			     const ZValueSeries& z_in,const ZValueSeries& z_out,
			     ValueSeries<double>& Vout);


/*!Given an irregularly sampled Vrms, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal(Algo) bool sampleVrms(const ValueSeries<double>& Vin,
			     const ZValueSeries& z_in,const ZValueSeries& z_out,
			     ValueSeries<double>& Vout,double t0_in=0.);


/*!Given an irregularly sampled effective Thomsen parameter array, create a
  regularly sampled one. The function assumes constant value of the parameter
  before and after the input interval.*/

mGlobal(Algo) void sampleEffectiveThomsenPars(const ValueSeries<double>& inparr,
			    const ZValueSeries& z_in,const ZValueSeries& z_out,
			    ValueSeries<double>& res);


/*!Given an irregularly sampled interval Thomsen parameter array, create a
  regularly sampled one. The function assumes constant value of the parameter
  before and after the input interval.*/

mGlobal(Algo) void sampleIntvThomsenPars(const ValueSeries<double>& inparr,
			    const ZValueSeries& z_in,const ZValueSeries& z_out,
			    ValueSeries<double>& res);


/*!Given a residual moveout at a reference offset, comput the residual moveout
   at other offsets */

mGlobal(Algo) void computeResidualMoveouts(double z0,double rmo,
					   double refoffset,int nroffsets,
					   bool outputdepth,
					   const double* offsets,
					   double* output);

/*!
\brief Rms velocity to interval velocity conversion.
*/

mExpClass(Algo) Vrms2Vint
{ mODTextTranslationClass(Vrms2Vint)
public:
			mDefineFactoryInClass(Vrms2Vint,factory);
    virtual		~Vrms2Vint()	{}

    virtual bool	compute(const ValueSeries<double>& Vrms,
				const ZValueSeries&,
				ValueSeries<double>& Vint,double t0=0.) = 0;
};


/*!
\brief Rms velocity to interval velocity conversion using the Dix formula.
*/

mExpClass(Algo) DixConversion : public Vrms2Vint
{ mODTextTranslationClass(DixConversion)
public:
		mDefaultFactoryInstantiation(Vrms2Vint,DixConversion,"Dix",
					     ::toUiString(sFactoryKeyword()));

    bool	compute(const ValueSeries<double>& Vrms,const ZValueSeries&,
			ValueSeries<double>&,double t0=0.) override;
};

} // namespace Vel


mDeprecated("Use RegularZValues::getDoubleSamplingData")
mGlobal(Algo) SamplingData<double> getDoubleSamplingData(
						    const SamplingData<float>&);


mDeprecated("Use Vel namespace")
mGlobal(Algo) bool computeLinearT2D(double v0,double dv,double v0depth,
				    const SamplingData<float>& timesampling,
				    int sz,float* res);

mDeprecated("Use Vel namespace")
mGlobal(Algo) bool computeLinearD2T(double v0,double dv,double v0depth,
				    const SamplingData<float>& depthsampling,
				    int sz,float* res);

mDeprecated("Use Vel namespace")
mGlobal(Algo) bool fitLinearVelocity(const float* Vint,const float* z_in,
			      int nr_in,const Interval<float>& zlayer,
			      float reference_z,bool zisdepth,float& V_0,
			      float& gradient,float& error);

mDeprecatedDef
mGlobal(Algo) bool convertToVintIfNeeded(const float* inpvel,
					 const VelocityDesc&,
					 const StepInterval<float>&,
					 float* outvel);

mDeprecatedDef
mGlobal(Algo) bool computeVavg(const float* Vint,const double* zvals,int nrvels,
			       float* Vavg);

mDeprecatedDef
mGlobal(Algo) bool computeVavg(const float* Vint,float z0,
			       const float* zvals,int nrvels,float* Vavg);

mDeprecatedDef
mGlobal(Algo) bool computeVrms(const float* Vint,const SamplingData<double>&,
			       int nrvels,float* Vrms);

mDeprecatedDef
mGlobal(Algo) bool computeVrms(const float* Vint,double t0,const double* tvals,
			       int nrvels,float* Vrms);

mDeprecatedDef
mGlobal(Algo) bool computeVrms(const float* Vint,float t0,const float* tvals,
			       int nrvels,float* Vrms);

mDeprecatedDef
mGlobal(Algo) bool computeVint(const float* Vavg,const double* zvals,int nrvels,
			       float* Vint);

mDeprecatedDef
mGlobal(Algo) bool computeVint(const float* Vavg,float t0,
			       const float* t,int nrvels,float* Vint);

mDeprecatedDef
mGlobal(Algo) bool computeDix(const float* Vrms,const SamplingData<double>&,
			      int nrvels,float* Vint);

mDeprecatedDef
mGlobal(Algo) bool computeDix(const float* Vrms,double t0,float v0,
			      const double* tvals,int nrvels,float* Vint);

mDeprecatedDef
mGlobal(Algo) bool computeDix(const float* Vrms,float t0,float v0,
			      const float* tvals,int nrvels,float* Vint);

mDeprecatedDef
mGlobal(Algo) bool sampleVint(const float* Vint,const double* zvals,int nr_in,
			      const SamplingData<double>& sd_out,float* Vout,
			      int nr_out);

mDeprecatedDef
mGlobal(Algo) bool sampleVint(const float* Vint,const float* zvals,int nr_in,
			      const SamplingData<double>& sd_out,float* Vout,
			      int nr_out);

mDeprecatedDef
mGlobal(Algo) bool sampleVavg(const float* Vavg,const double* zvals,int nr_in,
			      const SamplingData<double>& sd_out,float* Vout,
			      int nr_out);

mDeprecatedDef
mGlobal(Algo) bool sampleVavg(const float* Vavg, const float* zvals,int nr_in,
			      const SamplingData<double>& sd_out,float* Vout,
			      int nr_out);

mDeprecatedDef
mGlobal(Algo) bool sampleVrms(const float* Vin,double t0_in,
			      float v0_in,const double* t_in,int nr_in,
			      const SamplingData<double>& sd_out,
			      float* Vout,int nr_out);

mDeprecatedDef
mGlobal(Algo) bool sampleVrms(const float* Vin,float t0_in,
			      float v0_in,const float* t_in,int nr_in,
			      const SamplingData<double>& sd_out,
			      float* Vout,int nr_out);

mDeprecatedDef
mGlobal(Algo) void sampleEffectiveThomsenPars(const float* inparr,
				const float* zvals,int nr_in,
				const SamplingData<double>& sd_out,
				int nr_out,float* res);

mDeprecatedDef
mGlobal(Algo) void sampleIntvThomsenPars(const float* inparr,
				const float* zvals,int nr_in,
				const SamplingData<double>& sd_out,int nr_out,
				float* res);

mDeprecatedDef
mGlobal(Algo) void resampleZ(const double* zarr,const double* tord_in,int nr_in,
			     const SamplingData<double>& sd_out,int nr_out,
			     double* zsampled);

mDeprecatedDef
mGlobal(Algo) void resampleZ(const float* zarr,const float* tord_in,int nr_in,
			     const SamplingData<double>& sd_out, int nr_out,
			     float* zsampled);

mDeprecatedDef
mGlobal(Algo) void resampleContinuousData(const double* in,
				const double* tord_in,int nr_in,
				const SamplingData<double>& sd_out,int nr_out,
				double* outarr);

mDeprecatedDef
mGlobal(Algo) void resampleContinuousData(const float* inarr,
				const float* tord_in,int nr_in,
				const SamplingData<double>& sd_out,int nr_out,
				float* outarr);

/*!
\brief Rms velocity to interval velocity conversion.
*/

mExpClass(Algo) Vrms2Vint
{ mODTextTranslationClass(Vrms2Vint)
public:
			mDefineFactoryInClass( Vrms2Vint, factory );
    virtual		~Vrms2Vint()	{}

    virtual bool	compute(const float* Vrms,float t0,float v0,
				const float* t,int layers,float* Vint) = 0;
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

mStartAllowDeprecatedSection
    bool	compute( const float* Vrms, double t0, float v0,
			 const double* t, int nrvels, float* Vint )
		{ return computeDix( Vrms, t0, v0, t, nrvels, Vint ); }

    bool	compute( const float* Vrms, float t0, float v0,
			 const float* t, int nrvels, float* Vint ) override
		{ return computeDix( Vrms, t0, v0, t, nrvels, Vint ); }
mStopAllowDeprecatedSection
};
