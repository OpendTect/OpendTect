#ifndef prestackanglecomputer_h
#define prestackanglecomputer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
________________________________________________________________________


-*/

#include "ailayer.h"
#include "enums.h"
#include "flatposdata.h"
#include "iopar.h"
#include "position.h"
#include "prestackprocessingmod.h"
#include "refcount.h"
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

mExpClass(PreStackProcessing) AngleComputer :  public RefCount::Referenced
{
public:
				AngleComputer();

    enum smoothingType		{ None, MovingAverage, FFTFilter };
				mDeclareEnumUtils(smoothingType)

    virtual Gather*		computeAngles() = 0;
    virtual bool		isOK() const = 0;
    void			setTrcKey( const TrcKey& tk )
				{ trckey_ = tk; }

    void			setOutputSampling(const FlatPosData&);
    void			setRayTracer(const IOPar& raypar);
    void			setNoSmoother();
			    /*!<\param length Filter length in survey Z unit*/
    void			setMovingAverageSmoother(float length,
				        BufferString win=HanningWindow::sName(),
					float param=0.95);
    void			setFFTSmoother(float freqf3,float freqf4);
    void			setSmoothingPars(const IOPar&);

    static const char*		sKeySmoothType();
    static const char*		sKeyWinFunc();
    static const char*		sKeyWinParam();
    static const char*		sKeyWinLen();
    static const char*		sKeyFreqF3();
    static const char*		sKeyFreqF4();

protected:
                                ~AngleComputer();

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
    
    IOPar			iopar_;
    FlatPosData			outputsampling_;
    RayTracer1D*		raytracer_;
    ElasticModel		elasticmodel_;
    float			thresholdparam_;
    float			maxthickness_;
    bool			needsraytracing_;
    TrcKey			trckey_;
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
						 const TrcKey& tk )
				    : rt_(0), em_(new ElasticModel(em))
				    , trckey_(tk) {}
				ModelTool(const RayTracer1D* rt,
						 const TrcKey& tk )
				    : rt_(rt), em_(0), trckey_(tk) {}
				~ModelTool()	{ delete em_; }

	const RayTracer1D*	rayTracer() const { return rt_; }
	const ElasticModel&	elasticModel() const;
	const TrcKey&		trcKey() const	{ return trckey_; }
	bool 			operator ==( const ModelTool& a ) const
				{ return a.trcKey() == trckey_; }
    protected:
	ElasticModel*		em_;
	const RayTracer1D*	rt_;
	TrcKey			trckey_;
    };

				ModelBasedAngleComputer();

    void			setElasticModel(const TrcKey&,bool doblock,
						bool pvelonly,ElasticModel&);
    void			setRayTracer(const RayTracer1D*,
	    				     const TrcKey&);

    bool			isOK() const
				{ return curElasticModel().size(); }

    Gather*			computeAngles();

protected:

    const ElasticModel&		curElasticModel() const;
    const RayTracer1D*		curRayTracer() const;

    ObjectSet<ModelTool>	tools_;
};

} // namespace PreStack

#endif
