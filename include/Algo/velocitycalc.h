#ifndef velocitycalc_h
#define velocitycalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2007
 RCS:		$Id$
________________________________________________________________________

-*/


#include "algomod.h"
#include "samplingdata.h"
#include "veldesc.h"
#include "keystrs.h"
#include "factory.h"

template <class T> class ValueSeries;

/*!
\ingroup Algo
\brief Converts between time, depth and velocity given a model. The velocity
model can be either RMO-velocities in time, or interval velocity in either
depth or time.
*/

mClass(Algo) TimeDepthModel
{
public:
    			TimeDepthModel();
    			TimeDepthModel(const TimeDepthModel&);
    			~TimeDepthModel();

    virtual bool	isOK() const;
    const char*		errMsg() const;
    int 		size() const 	{ return sz_; }

    bool		setModel(const float* dpths,const float* times,int sz);

    float		getDepth(int idx) const;
    float		getTime(int idx) const;
    float		getLastTime() const;

    float		getDepth(float time) const;
    float		getTime(float depth) const;
    float	 	getVelocity(float depth) const;

    static float	getDepth(const float* dpths,const float* times,int sz,
	    			float time);
    static float	getTime(const float* dpths,const float* times,int sz,
				float depth);
    static float 	getVelocity(const float* dpths,const float* times,
	    				int sz,float depth);
protected:

    static float	convertTo(const float* dpths,const float* times,int sz,
				    float z,bool targetistime);

    int 		sz_;

    float*		times_;
    float*		depths_;

    const char*		errmsg_;
};


/*!
\ingroup Algo
\brief Converts between time and depth given a model.
*/

mClass(Algo) TimeDepthConverter : public TimeDepthModel
{
public:
    			TimeDepthConverter();

    bool		isOK() const;
    static bool		isVelocityDescUseable(const VelocityDesc&,
	    				      bool velintime,
					      FixedString* errmsg = 0);

    bool		setVelocityModel(const ValueSeries<float>& vels, int sz,
					 const SamplingData<double>& sd,
					 const VelocityDesc&,bool istime);

    bool		calcDepths(ValueSeries<float>&, int sz,
	    			   const SamplingData<double>& timesamp) const;
    bool		calcTimes(ValueSeries<float>&, int sz,
	    			   const SamplingData<double>& depthsamp) const;

    static bool		calcDepths(const ValueSeries<float>& vels, int velsz,
	    			   const SamplingData<double>&,float* depths);
    			/*!<\param vels Velocity as Vint in time */

    static bool		calcDepths(const ValueSeries<float>& vels, int velsz,
	    			   const ValueSeries<float>& times,
				   float* depths); 
    			 /*!<\param vels Velocity as Vint in time */

    static bool		calcTimes(const ValueSeries<float>& vels,int velsz,
	    			  const ValueSeries<float>& depth,float* times);
    			 /*!<\param vels Velocity as Vint in depth*/

    static bool		calcTimes(const ValueSeries<float>& vels, int velsz,
	    			   const SamplingData<double>&, float* times);
    			 /*!<\param vels Velocity as Vint in depth*/
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
\ingroup Algo
\brief Base class for computing a moveout curve.
*/

mClass(Algo) MoveoutComputer
{
public:
    virtual 		~MoveoutComputer()		{}

    virtual int		nrVariables() const				= 0;
    virtual const char*	variableName(int) const				= 0;

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
\ingroup Algo
\brief Computes moveout in depth from RMO at a certain reference offset.
*/

mClass(Algo) RMOComputer : public MoveoutComputer
{
public:
    int 	nrVariables() const	{ return 3; }
    const char*	variableName(int idx) const
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
    static bool	computeMoveout(float d0, float rmo, float refoffset,
	    		       int,const float*,float*);
};


/*!
\ingroup Algo
\brief Computes moveout with anisotropy, according to the equation
by Alkhalifah and Tsvankin 1995.
*/

mClass(Algo) NormalMoveout : public MoveoutComputer
{
public:
    int 	nrVariables() const	{ return 3; }
    const char*	variableName( int idx ) const
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
    static bool	computeMoveout(float t0, float Vrms, float effectiveanisotropy,
	    		       int,const float*,float*);
};

/*!Converts a number of layers with Vrms to interval velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeDix(const float* Vrms, float t0, float v0, const float* t,
			int nrlayers, float* Vint);

/*!
\ingroup Algo
\brief Rms velocity to interval velocity conversion.
*/

mClass(Algo) Vrms2Vint
{
public:
			mDefineFactoryInClass( Vrms2Vint, factory );

    virtual bool	compute(const float* Vrms, float t0, float v0,
	    			const float* t, int nrlayers, float* Vint) = 0;
};


/*!
\ingroup Algo
\brief Rms velocity to interval velocity conversion using the Dix formula.
*/

mClass(Algo) DixConversion : public Vrms2Vint
{
public:
		mDefaultFactoryInstantiation( Vrms2Vint, DixConversion, "Dix",
					      sFactoryKeyword() );
    bool	compute(const float* Vrms, float t0, float v0,
	    		const float* t, int nrlayers, float* Vint)
		{ return computeDix( Vrms, t0, v0, t, nrlayers, Vint ); }
};



/*!Converts a series of Vrms to Vint. Vrms may contain undefined values, as
   long as at least one is defined. */

mGlobal(Algo) bool computeDix(const float* Vrms,const SamplingData<double>& sd,
			int nrvels,float* Vint);

/*!Converts a number of layers with Vrms to interval velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeDix(const float* Vrms, float t0, float v0, const float* t,
			int nrlayers, float* Vint);


/*! Be very careful when using this one: the input Vint has to be regularly
  sampled according to sd. In case not, use the next one.*/
mGlobal(Algo) bool computeVrms(const float* Vint,const SamplingData<double>& sd,
			 int nrvels, float* Vrms);

/*!Converts a number of layers with Vint to rms velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeVrms(const float* Vint,float t0, const float* t,
			 int nrlayers, float* Vrms);

/*!Given an irregularly sampled Vrms, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal(Algo) bool sampleVrms(const float* Vin,float t0_in,float v0_in,
			const float* t_in, int nr_in, 
			const SamplingData<double>& sd_out,
			float* Vout, int nr_out);


/*!Converts a number of layers with Vint to average velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeVavg(const float* Vint, float t0, const float* t,
			 int nrvels, float* Vavg);


/*!Converts a number of layers with Vavg to Vint velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal(Algo) bool computeVint(const float* Vavg, float t0, const float* t,
			 int nrvels, float* Vint);


/*!Given an irregularly sampled Vint, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal(Algo) bool sampleVint(const float* Vint,const float* t_in, int nr_in,
			const SamplingData<double>& sd_out, float* Vout,
			int nr_out);

/*!Given an irregularly sampled Vavg, create a regularly sampled one. The
   function assumes constant average velocity before and after the input
   interval.*/

mGlobal(Algo) bool sampleVavg(const float* Vavg, const float* t_in, int nr_in,
			const SamplingData<double>& sd_out, float* Vout,
			int nr_out);

/*!Given a residual moveout at a reference offset, comput the residual moveout
   at other offsets */

mGlobal(Algo) void computeResidualMoveouts( float z0, float rmo, float refoffset,
				      int nroffsets, bool outputdepth,
				      const float* offsets, float* output );

/*!Given a layered V_int model (in time or depth), compute the best fit for a
   V_int = V_0 + gradient * (z-reference_z). The fit is such that the time/depth
   pairs at the layer's boundary will be preserved. */
mGlobal(Algo) bool fitLinearVelocity( const float* Vint, const float* z_in, int nr_in,
			      const Interval<float>& zlayer, float reference_z,
			      bool zisdepth, float& V_0, float& gradient,
			      float& error);
	        
	        
/*!Given an irregularly sampled depth or time array, create a regularly sampled
  one. The function assumes initial depth and time are 0.
  if zarr is time, tord_in is corresponding depth and other way round */

mGlobal(Algo) void resampleZ(const float* zarr,const float* tord_in, int nr_in,
			const SamplingData<double>& sd_out, int nr_out,
			float* zsampled);

/*!Given an irregularly sampled effective Thomsen parameter array, create a
  regularly sampled one. The function assumes constant value of the parameter before and after the input interval.*/
mGlobal(Algo) void sampleEffectiveThomsenPars(const float* vinarr,const float* t_in,
				 int nr_in,const SamplingData<double>& sd_out,
				 int nr_out,float* voutarr);

/*!Given an irregularly sampled interval Thomsen parameter array, create a
  regularly sampled one. The function assumes constant value of the parameter before and after the input interval.*/
mGlobal(Algo) void sampleIntvThomsenPars(const float* inarr,const float* t_in,
				int nr_in,const SamplingData<double>& sd_out,
				int nr_out,float* outarr);

/* Utility function for Depth and effective Thomsen parameters resampling*/
mGlobal(Algo) void resampleContinuousData(const float* inarr,const float* t_in,
				int nr_in,const SamplingData<double>& sd_out,
				int nr_out,float* outarr);


/* Block velocities and resamples depths and vel arrays at bend points */
mGlobal(Algo) void BendPointVelBlock(TypeSet<float>& dpts,TypeSet<float>& vels,
				float threshold,TypeSet<int>* remidxs=0); 

#endif

