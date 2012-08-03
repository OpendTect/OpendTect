#ifndef raytrace1d_h
#define raytrace1d_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: raytrace1d.h,v 1.38 2012-08-03 13:00:05 cvskris Exp $
________________________________________________________________________

*/

#include "algomod.h"
#include "ailayer.h"
#include "fixedstring.h"
#include "factory.h"
#include "odcomplex.h"
#include "reflectivitymodel.h"
#include "survinfo.h"
#include "task.h"
#include "velocitycalc.h"

template <class T> class Array2DImpl;
class IOPar;
class TimeDepthModel;

mClass(Algo) RayTracer1D : public ParallelTask
{ 
public:
    mDefineFactoryInClass( RayTracer1D, factory );

    static RayTracer1D*	createInstance(const IOPar&,BufferString&);

			~RayTracer1D();

    mClass(Algo) Setup
    {
    public:
			Setup() 
			    : pdown_( true )
			    , pup_( true )
			    , sourcedepth_( 6 )
			    , receiverdepth_( 7 )
			    , doreflectivity_(true)			 
			{
			    if ( SI().depthsInFeetByDefault() )
			    {
				sourcedepth_ = 20;
				receiverdepth_ = 25;
			    }
			}

	mDefSetupMemb(bool,pdown);
	mDefSetupMemb(bool,pup);
	mDefSetupMemb(float,sourcedepth);
	mDefSetupMemb(float,receiverdepth);
	mDefSetupMemb(bool,doreflectivity);

	virtual void	fillPar(IOPar&) const;
	virtual bool	usePar(const IOPar&);
    };

    virtual RayTracer1D::Setup&	setup() 		= 0;
    virtual const RayTracer1D::Setup& setup() const 	= 0;

    void		setModel(const ElasticModel&);
			/*!<Note, if either p-wave or s-wave are undef, 
			  will fill them with Castagna 
			  to compute zoeppritz coeffs <!*/

    void		setOffsets(const TypeSet<float>& offsets);

    const char*		errMsg() const { return errmsg_.str(); }

    			//Available after execution
    float		getSinAngle(int layeridx,int offsetidx) const;
    bool                getReflectivity(int offset,ReflectivityModel&) const;
    bool		getTWT(int offset,TimeDepthModel&) const;

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyPWave()	   { return "Wavetypes"; }
    static const char*	sKeySRDepth()	   { return "Source/Receiver Depths"; }
    static const char*	sKeyOffset()	   { return "Offset Range"; }
    static const char*	sKeyReflectivity() { return "Compute reflectivity"; }
    static const char*  sKeyVelBlock()     { return "Block velocities"; }
    static const char*  sKeyVelBlockVal()  { return "Block threshold"; }

    static void		setIOParsToZeroOffset(IOPar& iop);

protected:
			RayTracer1D();

    od_int64		nrIterations() const;
    virtual bool	doPrepare(int);
    virtual bool	compute(int,int,float);

    			//Setup variables
    ElasticModel	model_;
    TypeSet<float>	offsets_;
    FixedString		errmsg_;

			//Runtime variables
    int			sourcelayer_;
    int			receiverlayer_;
    int			firstlayer_;
    TypeSet<int>	offsetpermutation_;
    TypeSet<float>	velmax_;
    TypeSet<float>	depths_;

				//Results
    Array2DImpl<float>*		sini_;
    Array2DImpl<float>*		twt_;
    Array2DImpl<float_complex>* reflectivity_;
};



mClass(Algo) VrmsRayTracer1D : public RayTracer1D
{ 
public:

    mDefaultFactoryInstantiation( RayTracer1D, VrmsRayTracer1D, "VrmsRayTracer",
	    			"Simple RayTracer" );

    RayTracer1D::Setup&		setup() 	{ return setup_; }
    const RayTracer1D::Setup&	setup() const	{ return setup_; }

protected:
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);

    bool			compute(int,int,float);

    RayTracer1D::Setup		setup_;
};


#endif

