#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "refcount.h"

#include "ailayer.h"
#include "enums.h"
#include "flatposdata.h"
#include "iopar.h"
#include "position.h"
#include "prestackprocessingmod.h"
#include "windowfunction.h"

template <class T> class Array2D;
class FFTFilter;
class RayTracer1D;
class VelocityDesc;

namespace Vel { class FunctionSource; }

namespace PreStack
{

class Gather;

/*!
\brief Computes angles for PreStack::Gather.
*/

mExpClass(PreStackProcessing) AngleComputer : public ReferencedObject
{
public:
    enum smoothingType { None, MovingAverage, FFTFilter };
    mDeclareEnumUtils(smoothingType)

    virtual Gather*		computeAngles() = 0;
    virtual bool		isOK() const = 0;
    void			setTrcKey( const TrcKey & tk )
				{ trckey_ = tk; }

    void			setOutputSampling(const FlatPosData&);
    void			setRayTracer(const IOPar & raypar);
    void			setGatherIsNMOCorrected( bool yn )
				{ gatheriscorrected_ = yn; }
    void			setNoSmoother();
    void			setMovingAverageSmoother(float length,
				        BufferString win=HanningWindow::sName(),
					float param=0.95);
				/*!<\param length Filter length in survey
				  Z unit
				  \param win
				  \param param
				 */
    void			setFFTSmoother(float freqf3,float freqf4);
    void			setSmoothingPars(const IOPar&);

    static const char*		sKeySmoothType();
    static const char*		sKeyWinFunc();
    static const char*		sKeyWinParam();
    static const char*		sKeyWinLen();
    static const char*		sKeyFreqF3();
    static const char*		sKeyFreqF4();

protected:
				AngleComputer();
    virtual			~AngleComputer();

    bool			fillandInterpArray(Array2D<float>& angledata);
    Gather*			computeAngleData();
    void			averageSmooth(Array2D<float>& angledata);
    void			fftSmooth(Array2D<float>& angledata);
    void			fftTimeSmooth(::FFTFilter& fftfilter,
					      Array2D<float>& angledata);
    void			fftDepthSmooth(::FFTFilter& fftfilter,
					       Array2D<float>& angledata);

    virtual const ElasticModel&	curElasticModel() const = 0;
    virtual const RayTracer1D*	curRayTracer() const = 0;
    RayTracer1D*		curRayTracer();

    IOPar			iopar_;
    FlatPosData			outputsampling_;
    RayTracer1D*		raytracer_ = nullptr;
    ElasticModel		elasticmodel_;
    float			thresholdparam_ = 0.01f;
    float			maxthickness_ = 25.f;
    bool			needsraytracing_ = true;
    TrcKey			trckey_;
    bool			gatheriscorrected_ = true;
};



/*!
\brief Computes angles for PreStack::Gather from velocity model.
*/

mExpClass(PreStackProcessing) VelocityBasedAngleComputer : public AngleComputer
{
public:
				VelocityBasedAngleComputer();
				~VelocityBasedAngleComputer();

    bool			setMultiID(const MultiID&);
    bool			isOK() const { return velsource_; }

    Gather*			computeAngles();

protected:

    const ElasticModel&		curElasticModel() const	{ return elasticmodel_;}
    const RayTracer1D*		curRayTracer() const	{ return raytracer_; }

    RefMan<Vel::FunctionSource> velsource_;
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
				ModelTool(const ElasticModel&,const TrcKey&);
				ModelTool(const RayTracer1D*,const TrcKey&);
				~ModelTool();

	const RayTracer1D*	rayTracer() const { return rt_; }
	const ElasticModel&	elasticModel() const;
	const TrcKey&		trcKey() const	{ return trckey_; }
	bool			operator ==( const ModelTool& oth ) const
				{ return oth.trcKey() == trckey_; }

    protected:
	ElasticModel*		em_ = nullptr;
	RayTracer1D*		rt_ = nullptr;
	bool			ownrt_;
	TrcKey			trckey_;

    private:
				ModelTool(const ModelTool&) = delete;
	ModelTool&			operator=(const ModelTool&) = delete;
    };

				ModelBasedAngleComputer();
				~ModelBasedAngleComputer();

    void			setElasticModel(const TrcKey&,bool doblock,
						bool pvelonly,ElasticModel&);
    void			setRayTracer(const RayTracer1D*,
					     const TrcKey&);

    bool			isOK() const
				{ return curElasticModel().size(); }

    Gather*			computeAngles();
    RayTracer1D*		curRayTracer();

protected:

    const ModelTool*		curModelTool() const;
    const ElasticModel&		curElasticModel() const;
    const RayTracer1D*		curRayTracer() const;
    void			splitModelIfNeeded();

    ObjectSet<ModelTool>	tools_;
};

} // namespace PreStack
