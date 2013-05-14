#ifndef prestackanglecomputer_h
#define prestackanglecomputer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include "ailayer.h"
#include "enums.h"
#include "flatposdata.h"
#include "iopar.h"
#include "position.h"
#include "prestackprocessingmod.h"
#include "refcount.h"

template <class T> class Array2D;
class MultiID;
class RayTracer1D;
class VelocityDesc;

namespace Vel { class FunctionSource; }

namespace PreStack
{

class Gather;

/*!
\brief Computes angles for PreStack::Gather.
*/

mExpClass(PreStackProcessing) AngleComputer
{ mRefCountImpl(AngleComputer)
public:
				AngleComputer();

    enum smoothingType		{ TimeAverage, FFTFilter };
				DeclareEnumUtils(smoothingType);

    virtual Gather*		computeAngles() = 0;
    virtual bool		isOK() const = 0;
    void			setTraceID(const TraceID& trcid)
				{ trcid_ = trcid; }
    virtual const ElasticModel&	curElasticModel() const = 0;
    virtual const RayTracer1D*	curRayTracer() const = 0;

    void			setOutputSampling(const FlatPosData&);
    void			setRayTracer(const IOPar& raypar);
    void			setSmoothingPars(const IOPar& iopar);

    static const char*		sKeySmoothType() { return "Smoothing type"; }
    static const char*		sKeyWinFunc() { return "Window function"; }
    static const char*		sKeyWinParam() { return "Window parameter"; }
    static const char*		sKeyWinLen() { return "Window length"; }
    static const char*		sKeyFreqF3() { return "F3 freq"; }
    static const char*		sKeyFreqF4() { return "F4 freq"; }

protected:

    bool			fillandInterpArray(Array2D<float>& angledata);
    Gather*			computeAngleData();
    void			averageSmoothing(Array2D<float>& angledata);
    void			fftSmoothing(Array2D<float>& angledata);
    
    IOPar			iopar_;
    FlatPosData			outputsampling_;
    RayTracer1D*		raytracer_;
    ElasticModel		elasticmodel_;
    float			thresholdparam_;
    float			maxthickness_;
    bool			needsraytracing_;
    TraceID			trcid_;
};

/*!
\brief Computes angles for PreStack::Gather from velocity model.
*/

mExpClass(PreStackProcessing) VelocityBasedAngleComputer : public AngleComputer
{ 
public:
				VelocityBasedAngleComputer();

    bool			setMultiID(const MultiID&);
    bool			isOK() const { return velsource_; }
    const ElasticModel&		curElasticModel() const	{ return elasticmodel_;}
    const RayTracer1D*		curRayTracer() const	{ return raytracer_; }

    Gather*			computeAngles();

protected:

				~VelocityBasedAngleComputer();
    bool			checkAndConvertVelocity(const float* inpvel,
					const VelocityDesc& veldesc,
					const StepInterval<float>& zrange,
					float* outvel);
    bool			createElasticModel(
					    const StepInterval<float>& zrange,
					    const float* pvel);

    Vel::FunctionSource*	velsource_;
};

/*!
\brief Computes angles for PreStack::Gather from ElasticModel.
*/

mExpClass(PreStackProcessing) ModelBasedAngleComputer : public AngleComputer
{
public:
    class ModelTool
    {
    public:
				ModelTool(const ElasticModel& em,
						 const TraceID& id )
				    : rt_(0), em_(new ElasticModel(em))
				    , trcid_(id) {}
				ModelTool(const RayTracer1D* rt,
						 const TraceID& id )
				    : rt_(rt), em_(0), trcid_(id) {}
				~ModelTool()	{ delete em_; }
	const RayTracer1D*	rayTracer() const { return rt_; }
	const ElasticModel&	elasticModel() const;
	const TraceID&		trcID() const	{ return trcid_; }
	bool 			operator ==( const ModelTool& a ) const
				{ return a.trcID() == trcid_; }
    protected:
	ElasticModel*		em_;
	const RayTracer1D*	rt_;
	TraceID			trcid_;
    };
				ModelBasedAngleComputer();
				
    void			setElasticModel(const ElasticModel& em,
						const TraceID& trcid,
	    					bool doblock,
						bool pvelonly=true );
    void			setRayTracer(const RayTracer1D* rt,
	    				     const TraceID&);
    const ElasticModel&		curElasticModel() const;
    const RayTracer1D*		curRayTracer() const;
    bool			isOK() const
				{ return curElasticModel().size(); }

    Gather*			computeAngles();
protected:
    ObjectSet<ModelTool>	tools_;
};


}; //namespace

#endif

