#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
________________________________________________________________________

*/

#include "algomod.h"

#include "factory.h"
#include "odcomplex.h"
#include "paralleltask.h"

class ElasticModel;
class ReflectivitySpike;
typedef class TypeSet<ReflectivitySpike> ReflectivityModel;
class TimeDepthModel;
class TimeDepthModelSet;


/*!
\brief Ray tracer in 1D.
*/

mExpClass(Algo) RayTracer1D : public ParallelTask
{ mODTextTranslationClass(RayTracer1D);
public:
    mDefineFactoryInClass( RayTracer1D, factory );

    static RayTracer1D* createInstance(const IOPar&,uiString&);

			~RayTracer1D();

    mExpClass(Algo) Setup
    {
    public:
			Setup()
			    : pdown_( true )
			    , pup_( true )
			    , doreflectivity_(true) {}
	virtual ~Setup()	{}

	mDefSetupMemb(bool,pdown);
	mDefSetupMemb(bool,pup);
	mDefSetupMemb(bool,doreflectivity);

	virtual void	fillPar(IOPar&) const;
	virtual bool	usePar(const IOPar&);
    };

    virtual RayTracer1D::Setup&	setup()		= 0;
    virtual const RayTracer1D::Setup& setup() const	= 0;
    virtual bool	hasSameParams(const RayTracer1D&) const;

    bool		setModel(const ElasticModel&);
    const ElasticModel&	getModel() const	{ return model_; }
			// model top depth must be TWT = 0ms
			/*!<Note, if either p-wave or s-wave are undef,
			  will fill them with Castagna
			  to compute zoeppritz coeffs <!*/

    void		setOffsets(const TypeSet<float>& offsets);
    void		getOffsets(TypeSet<float>& offsets) const;

    uiString		errMsg() const { return errmsg_; }

			//Available after execution
    ConstRefMan<TimeDepthModelSet>	getTDModels() const;
    float		getSinAngle(int layeridx,int offsetidx) const;

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyPWave()	   { return "Wavetypes"; }
    static const char*	sKeyOffset()	   { return "Offset Range"; }
    static const char*	sKeyReflectivity() { return "Compute reflectivity"; }
    static const char*  sKeyBlock()	   { return "Block model"; }
    static const char*  sKeyBlockRatio()   { return "Blocking ratio threshold";}
    static const char*	sKeyOffsetInFeet() { return "Offset in Feet";}
    static float	cDefaultBlockRatio();

    static StepInterval<float>	sDefOffsetRange();

    static void		setIOParsToZeroOffset(IOPar& iop);

protected:
			RayTracer1D();

    od_int64		nrIterations() const;
    bool		doPrepare(int) override;
    bool		doFinish(bool) override;
    virtual bool	compute(int layer,int offidx,float rayparam);

    bool		getTDM(const float*,TimeDepthModel&) const;

			//Setup variables
    ElasticModel&	model_; // model top depth must be TWT = 0ms
    TypeSet<float>	offsets_;
    uiString		errmsg_;

			//Runtime variables
    TypeSet<int>	offsetpermutation_;
    float*		velmax_ = nullptr;
    float*		depths_ = nullptr;
    float*		zerooffstwt_ = nullptr;
    float**		twt_ = nullptr;
    float_complex**	reflectivities_ = nullptr;
    float**		sinarr_ = nullptr;

				//Results
    RefMan<TimeDepthModelSet>	tdmodels_;
    float*		sini_ = nullptr;

public:

    float		getDepth(int layer) const;
    float		getTime(int layer,int offset) const;

    //TODO mark as deprecated:
    bool                getReflectivity(int offset,ReflectivityModel&) const;
    bool		getTDModel(int offset,TimeDepthModel&) const;
    bool		getZeroOffsTDModel(TimeDepthModel&) const;
};


/*!
\brief Ray tracer in 1D based on Vrms.
*/

mExpClass(Algo) VrmsRayTracer1D : public RayTracer1D
{ mODTextTranslationClass(VrmsRayTracer1D);
public:

    mDefaultFactoryInstantiation( RayTracer1D, VrmsRayTracer1D,
				  "VrmsRayTracer",
				  tr("Simple RayTracer") );

    RayTracer1D::Setup&		setup() override	{ return setup_; }
    const RayTracer1D::Setup&	setup() const override	{ return setup_; }

protected:
    bool			doWork(od_int64,od_int64,int) override;

    bool			compute(int,int,float);

    RayTracer1D::Setup		setup_;
};


