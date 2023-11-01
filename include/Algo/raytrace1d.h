#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "factory.h"
#include "odcomplex.h"
#include "odcommonenums.h"
#include "paralleltask.h"
#include "reflectivitymodel.h"

class ElasticModel;


/*!
\brief Ray tracer in 1D.
*/

mExpClass(Algo) RayTracer1D : public ParallelTask
{ mODTextTranslationClass(RayTracer1D);
public:
    mDefineFactoryInClass( RayTracer1D, factory );

    mDeprecated("Provide RayTracer1D::Setup")
    static RayTracer1D* createInstance(const IOPar&,uiString&);
    mDeprecated("Provide RayTracer1D::Setup")
    static RayTracer1D* createInstance(const IOPar&,const ElasticModel*,
				       uiString&);

			~RayTracer1D();

    mExpClass(Algo) Setup
    {
    public:
			Setup();
			Setup(const Setup&);
	virtual		~Setup();

	Setup&		operator =(const Setup&);

	mDefSetupMemb(bool,pdown);		// def: true
	mDefSetupMemb(bool,pup);		// def: true
	mDefSetupMemb(bool,doreflectivity);	// def: true
	float		getStartTime() const;
	float		getStartDepth() const;
	Seis::OffsetType offsetType() const;
	ZDomain::DepthType depthType() const;
	Setup&		starttime(float newval);
	Setup&		startdepth(float newbal);
	Setup&		offsettype(Seis::OffsetType);
	Setup&		depthtype(ZDomain::DepthType);

	virtual void	fillPar(IOPar&) const;
	virtual bool	usePar(const IOPar&);

	bool		areOffsetsInFeet() const;
	bool		areDepthsInFeet() const;
    };

    virtual RayTracer1D::Setup&	setup()		= 0;
    virtual const RayTracer1D::Setup& setup() const	= 0;
    virtual bool	hasSameParams(const RayTracer1D&) const;
    bool		needsSwave() const;

    bool		setModel(const ElasticModel&);
    const ElasticModel&	getModel() const	{ return model_; }
			// model top depth must be TWT = 0ms
			/*!<Note, if either p-wave or s-wave are undef,
			  will fill them with Castagna
			  to compute zoeppritz coeffs <!*/

    mDeprecated("Provide Seis::OffsetType")
    void		setOffsets(const TypeSet<float>& offsets);
    void		setOffsets(const TypeSet<float>& offsets,
				   Seis::OffsetType);
    void		getOffsets(TypeSet<float>& offsets) const;
    bool		areOffsetsInFeet() const;
    bool		areDepthsInFeet() const;

    uiString		uiMessage() const override	{ return msg_; }

			//Available after execution
    ConstRefMan<OffsetReflectivityModel> getRefModel() const;

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyRayPar()	   { return "Ray Parameter"; }
    static const char*	sKeyOffset()	   { return "Offset Range"; }
    static const char*	sKeyOffsetInFeet() { return "Offset in Feet"; }
    static const char*	sKeyReflectivity() { return "Compute reflectivity"; }
    static const char*	sKeyWavetypes()    { return "Wavetypes"; }

    mDeprecated("Use Seis::OffsetType")
    static StepInterval<float>	sDefOffsetRange();
    static StepInterval<float>	sDefOffsetRange(Seis::OffsetType);

    static void		setIOParsToZeroOffset(IOPar&);
    static RayTracer1D* createInstance(const IOPar&,uiString&,
				       const Setup*);
    static RayTracer1D* createInstance(const IOPar&,const ElasticModel*,
				       uiString&,const Setup*);

protected:
			RayTracer1D();

    od_int64		nrIterations() const override;
    bool		doPrepare(int) override;
    bool		doFinish(bool) override;
    virtual bool	compute(int layer,int offidx,float rayparam);

			//Setup variables
    ElasticModel&	model_; // model top depth must be TWT = 0ms
    TypeSet<float>	offsets_;
    uiString		msg_;

			//Runtime variables
    TypeSet<int>	offsetpermutation_;
    float*		velmax_ = nullptr;
    float*		depths_ = nullptr;
    float*		zerooffstwt_ = nullptr;
    float**		twt_ = nullptr;
    float_complex**	reflectivities_ = nullptr;
    float**		sinarr_ = nullptr;

				//Results
    RefMan<OffsetReflectivityModel>	refmodel_;

public:

    float		getDepth(int layer) const;
    float		getTime(int layer,int offset) const;

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

    bool			compute(int,int,float) override;

    RayTracer1D::Setup		setup_;
};
