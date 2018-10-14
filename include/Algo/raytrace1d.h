#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
________________________________________________________________________

*/

#include "algomod.h"

#include "elasticmodel.h"
#include "factory.h"
#include "paralleltask.h"
#include "reflectivitymodel.h"
#include "timedepthmodel.h"

template <class T> class Array1D;
template <class T> class Array2D;
class RayTracer1D;


/*!\brief Ray tracer data in 1D. */

mExpClass(Algo) RayTracerData : public RefCount::Referenced
{ mODTextTranslationClass(RayTracerData)
public:

    typedef RefMan<ReflectivityModelSet>	RflMdlSetRef;
    typedef ConstRefMan<ReflectivityModelSet>	ConstRflMdlSetRef;

			RayTracerData(const RayTracerData&);
    RayTracerData&	operator=(const RayTracerData&);

    bool		isOK() const;
    bool		isFinalised() const;

    int			nrLayers() const	{ return depths_.size(); }
    bool		validDepthIdx(int) const;
    int			nrOffset() const	{ return offsets_.size(); }
    bool		validOffsetIdx(int) const;
    bool		hasZeroOffsetOnly() const;
    bool		hasReflectivity() const { return reflectivity_; }

    float		getOffset(int) const;
    float		getDepth(int) const;
    float		getTnmo(int layer) const;
    float		getTime(int layer,int offset) const;
    float		getSinAngle(int layer,int offset) const;
    float_complex	getReflectivity(int layer,int offset) const;

    const TimeDepthModel&	getZeroOffsTDModel() const;
    const TimeDepthModel&	getTDModel(int offset) const;
    const ReflectivityModel&	getReflectivity(int offset) const;

    ReflectivityModelSet&	getReflectivities()	{ return reflmodels_; }
    const ReflectivityModelSet& getReflectivities() const
							{ return reflmodels_; }

protected:
			RayTracerData(const ElasticModel& layers,
				      const TypeSet<float>& offsets);
    virtual		~RayTracerData();

    virtual bool	finalise();

    const TypeSet<float>	depths_;
    const TypeSet<float>	offsets_;

    Array1D<float>&		zerooffstwt_;
    Array2D<float>&		twt_;
    Array2D<float>&		sini_;
    Array2D<float_complex>*	reflectivity_	= 0;

private:

    void		init(const ElasticModel&);
    bool		setWithReflectivity(bool yn);

    bool		getZeroOffsTDModel(TimeDepthModel&) const;
    bool		getTDModel(int offset,TimeDepthModel&) const;
    bool		getTDM(const Array1D<float>&,TimeDepthModel&) const;
    bool		getReflectivity(int offset,ReflectivityModel&) const;

    TimeDepthModel*	zerooffsett2dmodel_	= 0;
    ManagedObjectSet<TimeDepthModel>	t2dmodels_;
    ReflectivityModelSet	reflmodels_;

    friend class RayTracer1D;

public:
    //Only for RayTracer1D implementations:

    void		setTnmo(int layer,float);
    void		setTWT(int layer,int offset,float);
    void		setSinAngle(int layer,int offset,float);
    void		setReflectivity(int layer,int offset,float_complex);

};


/*!\brief Ray tracer in 1D. */

mExpClass(Algo) RayTracer1D : public ParallelTask
{ mODTextTranslationClass(RayTracer1D)
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
			    , doreflectivity_(true)	{}
	virtual ~Setup()				{}

	mDefSetupMemb(bool,pdown);
	mDefSetupMemb(bool,pup);
	mDefSetupMemb(bool,doreflectivity);

	virtual void	fillPar(IOPar&) const;
	virtual bool	usePar(const IOPar&);
    };

    virtual Setup&	setup()				= 0;
    inline const Setup&	setup() const
			{ return const_cast<RayTracer1D*>(this)->setup(); }
    virtual bool	hasSameParams(const RayTracer1D&) const;

    bool		setModel(const ElasticModel&);
    const ElasticModel&	getModel() const		{ return model_; }
			    //!< model top depth must be TWT = 0ms
			    /*!<Note, if either p-wave or s-wave are undef,
			      will fill them with Castagna
			      to compute zoeppritz coeffs */

    void		setOffsets(const TypeSet<float>& offsets);
			/*!<Note, offsets will be sorted */
    void		getOffsets(TypeSet<float>& offsets) const;

    virtual uiString	message() const;
    virtual uiString	nrDoneText() const;
    uiString		errMsg() const { return errmsg_; }

			//Available after execution
    ConstRefMan<RayTracerData> results() const	{ return result_; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyPWave()	   { return "Wavetypes"; }
    static const char*	sKeyOffset()	   { return "Offset Range"; }
    static const char*	sKeyReflectivity() { return "Compute reflectivity"; }
    static const char*  sKeyBlock()	   { return "Block model"; }
    static const char*  sKeyBlockRatio()   { return "Blocking ratio threshold";}
    static float	cDefaultBlockRatio();

    static void		setIOParsToZeroOffset(IOPar& iop);

protected:
			RayTracer1D();

    od_int64		nrIterations() const;
    virtual bool	doPrepare(int);
    virtual bool	doWork(od_int64,od_int64,int)		= 0;
    virtual bool	doFinish(bool);
    virtual bool	compute(int,int,float);

			//Setup variables
    ElasticModel	model_; // model top depth must be TWT = 0ms
    TypeSet<float>	offsets_;
    uiString		errmsg_;

			//Runtime variable
    TypeSet<float>	velmax_;

    RefMan<RayTracerData>	result_;

private:

    void		setZeroOffsetTWT();
};


/*!\brief Ray tracer in 1D based on Vrms. */

mExpClass(Algo) VrmsRayTracer1D : public RayTracer1D
{ mODTextTranslationClass(VrmsRayTracer1D);
public:

    mDefaultFactoryInstantiation( RayTracer1D, VrmsRayTracer1D,
				  "VrmsRayTracer",
				  tr("Simple RayTracer") );

    virtual Setup&	setup()		{ return setup_; }

protected:

    virtual bool	doWork(od_int64,od_int64,int) final;
    virtual bool	compute(int,int,float) final;

    Setup		setup_;

};
