#ifndef velocitycalc_h
#define velocitycalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2007
 RCS:		$Id: velocitycalc.h,v 1.20 2010-11-22 14:48:55 cvskris Exp $
________________________________________________________________________

-*/


#include "samplingdata.h"
#include "veldesc.h"
#include "keystrs.h"
#include "factory.h"

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
    static bool		isVelocityDescUseable(const VelocityDesc&,
	    				      bool velintime,
					      FixedString* errmsg = 0);

    const char*		errMsg() const;

    bool		setVelocityModel(const ValueSeries<float>&, int sz,
	    				 const SamplingData<double>&,
					 const VelocityDesc&,bool istime);

    bool		calcDepths(ValueSeries<float>&, int sz,
	    			   const SamplingData<double>& timesamp) const;
    bool		calcTimes(ValueSeries<float>&, int sz,
	    			   const SamplingData<double>& depthsamp) const;

    static bool		calcDepths(const ValueSeries<float>& vels, int velsz,
	    			   const SamplingData<double>&, float* depths );
    static bool		calcTimes(const ValueSeries<float>& vels, int velsz,
	    			   const SamplingData<double>&, float* depths );
protected:

    float			firstvel_;
    float			lastvel_;
    float*			depths_;
    float*			times_;
    int				sz_;
    SamplingData<double>	sd_;

    const char*			errmsg_;
};

/*!Base class for computing a moveout curve. */
mClass MoveoutComputer
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


/*Computes moveout in depth from RMO at a certain reference offset */

mClass RMOComputer : public MoveoutComputer
{
public:
    int 	nrVariables() const	{ return 3; }
    const char*	variableName(int idx) const
		{
		    switch ( idx ) 
		    {
			case 0: return sKey::Depth;
			case 1: return "RMO";
			case 2: return "Reference offset";
		    };

		    return 0;
		}
    bool	computeMoveout(const float*,int,const float*,float*) const;
    static bool	computeMoveout(float d0, float rmo, float refoffset,
	    		       int,const float*,float*);
};


/*! Computes moveout with anisotropy, according to the equation
by Alkhalifah and Tsvankin 1995. */

mClass NormalMoveout : public MoveoutComputer
{
public:
    int 	nrVariables() const	{ return 3; }
    const char*	variableName(int idx)
		{
		    switch ( idx ) 
		    {
			case 0: return sKey::Time;
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

mGlobal bool computeDix(const float* Vrms, float t0, float v0, const float* t,
			int nrlayers, float* Vint);

mClass Vrms2Vint
{
public:
			mDefineFactoryInClass( Vrms2Vint, factory );

    virtual bool	compute(const float* Vrms, float t0, float v0,
	    			const float* t, int nrlayers, float* Vint) = 0;
};

mClass DixConversion : public Vrms2Vint
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

mGlobal bool computeDix(const float* Vrms,const SamplingData<double>& sd,
			int nrvels,float* Vint);

/*!Converts a number of layers with Vrms to interval velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal bool computeDix(const float* Vrms, float t0, float v0, const float* t,
			int nrlayers, float* Vint);


mGlobal bool computeVrms(const float* Vint,const SamplingData<double>& sd,
			 int nrvels, float* Vrms);

/*!Converts a number of layers with Vint to rms velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

mGlobal bool computeVrms(const float* Vint,float t0, float v0, const float* t,
			 int nrlayers, float* Vrms);

/*!Given an irregularly sampled Vrms, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal bool sampleVrms(const float* Vin,float t0_in,float v0_in,
			const float* t_in, int nr_in, 
			const SamplingData<double>& sd_out,
			float* Vout, int nr_out);


/*!Given an irregularly sampled Vint, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

mGlobal bool sampleVint(const float* Vint,const float* t_in, int nr_in,
			const SamplingData<double>& sd_out, float* Vout,
			int nr_out);
/*!Given a residual moveout at a reference offset, comput the residual moveout
   at other offsets */

mGlobal void computeResidualMoveouts( float z0, float rmo, float refoffset,
				      int nroffsets, bool outputdepth,
				      const float* offsets, float* output );

/*!Given a layered V_int model (in time or depth), compute the best fit for a
   V_int = V_0 + gradient * (z-reference_z). The fit is such that the time/depth
   pairs at the layer's boundary will be preserved. */

mGlobal void fitLinearVelocity( const float* Vint, const float* z_in, int nr_in,
			      const Interval<float>& zlayer, float reference_z,
			      bool zisdepth, float& V_0, float& gradient,
			      float& error);


	        
	        
#endif
