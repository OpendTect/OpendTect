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
{ 
public:
				AngleComputer();
			        ~AngleComputer();

				enum SmoothingType { TimeAverage, FFTFilter };
    virtual Gather*		computeAngles() = 0;
    virtual bool		isOK() const = 0;

    void			setOutputSampling(const FlatPosData&);
    void			setThresholdParam(float param)
				{ thresholdparam_ = param; } 
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
    ElasticModel		elasticmodel_;
    RayTracer1D*		raytracer_;
    float			thresholdparam_;
    float			maxthickness_;
    bool			needsraytracing_;
};


mExpClass(PreStackProcessing) VelocityBasedAngleComputer : public AngleComputer
{ mRefCountImpl(VelocityBasedAngleComputer);
public:
				VelocityBasedAngleComputer();

    bool			setMultiID(const MultiID&);
    void			setTraceID(const TraceID& trcid)
				{ trcid_ = trcid; }
    bool			isOK() const { return velsource_; }

    Gather*			computeAngles();

protected:

    bool			checkAndConvertVelocity(const float* inpvel,
					const VelocityDesc& veldesc,
					const StepInterval<float>& zrange,
					float* outvel);
    bool			createElasticModel(
					    const StepInterval<float>& zrange,
					    const float* pvel);

    Vel::FunctionSource*	velsource_;
    TraceID			trcid_;
};


mExpClass(PreStackProcessing) ModelBasedAngleComputer : public AngleComputer
{
public:
				ModelBasedAngleComputer();
				
    void			setElasticModel(ElasticModel& em,bool doblock,
						bool pvelonly=true);
    void			setRayTracer(RayTracer1D* rt);
				//! <Ray Tracer becomes mine
    bool			isOK() const
				{ return elasticmodel_.size(); }

    Gather*			computeAngles();
};


}; //namespace

#endif

