#ifndef raytrace1d_h
#define raytrace1d_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: raytrace1d.h,v 1.24 2011-08-10 15:03:51 cvsbruno Exp $
________________________________________________________________________

*/

#include "ailayer.h"
#include "fixedstring.h"
#include "odcomplex.h"
#include "reflectivitymodel.h"
#include "task.h"

template <class T> class Array2DImpl;
class IOPar;
class TimeDepthModel;


mClass RayTracer1D : public ParallelTask
{
public:
    mClass Setup
    {
    public:
			Setup() 
			    : pdown_( true )
			    , pup_( true )
			    , sourcedepth_( 0 )
			    , receiverdepth_( 0 )
			    , pvel2svelafac_(0.348) 
			    , pvel2svelbfac_(-0.959) 
			{}

				mDefSetupMemb(bool,pdown);
				mDefSetupMemb(bool,pup);
				mDefSetupMemb(float,sourcedepth);
				mDefSetupMemb(float,receiverdepth);
				mDefSetupMemb(float,pvel2svelafac);
				mDefSetupMemb(float,pvel2svelbfac);

	virtual void		fillPar(IOPar&) const;
	virtual bool		usePar(const IOPar&);

	static const char*	sKeyPWave()	{ return "Wavetypes"; }
	static const char*	sKeySRDepth()	{ return "SR Depths"; }
	static const char*	sKeyPSVelFac()	{ return "PWave/SWave factor"; }
    };


				RayTracer1D(const Setup&);
    virtual			~RayTracer1D();			
    virtual const Setup&	setup() const		{ return setup_; }
    virtual void		setSetup(const Setup&);

    void			setModel(const ElasticModel&);
				/*!<Note, if either p-wave or s-wave are undef, 
				  will fill them with Castagna 
				  to compute zoeppritz coeffs <!*/

    void			setOffsets(const TypeSet<float>& offsets);

    const char*			errMsg() const { return errmsg_.str(); }

    			//Available after execution
    float		getSinAngle(int layeridx,int offsetidx) const;
    float*		getSinAngleData() const;

    bool                getReflectivity(int offset,ReflectivityModel&) const;
    bool		getTWT(int offset,TimeDepthModel&) const;

protected:

    od_int64		nrIterations() const;
    virtual bool	doPrepare(int);
    virtual bool	doWork(od_int64,od_int64,int);

    virtual bool	init();
    virtual bool	compute(int,int,float);

    int			findLayer(const ElasticModel& model,
	    			float targetdepth) const;

    			//Setup variables
    ElasticModel	model_;
    TypeSet<float>	offsets_;
    Setup		setup_;

			//Runtime variables
    int			sourcelayer_;
    int			receiverlayer_;
    int			firstlayer_;
    TypeSet<int>	offsetpermutation_;
    FixedString		errmsg_;
    TypeSet<float>	velmax_;
    TypeSet<float>	depths_;

			//Results
    Array2DImpl<float>*	sini_;
    Array2DImpl<float>* twt_;
    Array2DImpl<float_complex>* reflectivity_;
};


#endif
