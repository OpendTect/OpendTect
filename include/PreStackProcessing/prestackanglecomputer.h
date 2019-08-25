#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
________________________________________________________________________


-*/

#include "elasticmodel.h"
#include "enums.h"
#include "flatposdata.h"
#include "iopar.h"
#include "position.h"
#include "prestackprocessingmod.h"
#include "refcount.h"
#include "windowfunction.h"

template <class T> class Array2D;
class FFTFilter;
class Gather;
class RayTracerData;
class TrcKey;
class VelocityDesc;

namespace Vel { class FunctionSource; }

namespace PreStack
{

/*!
\brief Computes angles for Gather.
*/

mExpClass(PreStackProcessing) AngleComputer : public RefCount::Referenced
{ mODTextTranslationClass(AngleComputer)
public:

    enum smoothingType		{ None, MovingAverage, FFTFilter };
				mDeclareEnumUtils(smoothingType)

    virtual bool		isOK() const;
    virtual RefMan<Gather>	computeAngles();

    virtual bool		needsTrcKey() const			= 0;
    virtual void		setTrcKey(const TrcKey&)		{}

    void			setOutputSampling(const FlatPosData&);
    void			outputDegrees( bool yn )
						    { outputindegrees_ = yn; }
    void			gatherIsNMOCorrected(bool yn);
    void			setRayTracerPars(const IOPar&);
    void			setNoSmoother();
			    /*!<\param length Filter length in survey Z unit*/
    void			setMovingAverageSmoother(float length,
				        BufferString win=HanningWindow::sName(),
					float param=0.95);
    void			setFFTSmoother(float freqf3=10.f,
					       float freqf4=15.f);
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

    int				nrLayers() const;

    bool			fillAndInterpolateAngleData(Array2D<float>&);
    void			averageSmooth(Array2D<float>&);
    void			fftSmooth(Array2D<float>&);
    void			fftTimeSmooth(::FFTFilter&,Array2D<float>&);
    void			fftDepthSmooth(::FFTFilter&,Array2D<float>&);
    void			convertToDegrees(Array2D<float>&);

    IOPar			iopar_;
    IOPar			raypar_;
    FlatPosData			outputsampling_;
    bool			iscorrected_	= true;
    ElasticModel*		elasticmodel_	= 0;
    float			thresholdparam_ = 0.01f;
    float			maxthickness_	= 25.f;

    ConstRefMan<RayTracerData>	raytracedata_;

private:

    bool			needsRaytracing() const;
    bool			doRaytracing(uiString& msg);

    bool			outputindegrees_ = false;
};



/*!
\brief Computes angles for Gather from velocity model.
*/

mExpClass(PreStackProcessing) VelocityBasedAngleComputer : public AngleComputer
{ mODTextTranslationClass(VelocityBasedAngleComputer)
public:
				VelocityBasedAngleComputer();

    virtual bool		needsTrcKey() const final	{ return true; }
    virtual void		setTrcKey(const TrcKey&) final;

    bool			setDBKey(const DBKey&);
    virtual bool		isOK() const final;

    virtual RefMan<Gather>	computeAngles() final;

protected:
				~VelocityBasedAngleComputer();

    Vel::FunctionSource*	velsource_	= 0;
    TrcKey&			tk_;

};



/*!
\brief Computes angles for Gather from ElasticModel.
*/

mExpClass(PreStackProcessing) ModelBasedAngleComputer : public AngleComputer
{ mODTextTranslationClass(ModelBasedAngleComputer)
public:

				ModelBasedAngleComputer();

    virtual bool		needsTrcKey() const final { return false; }

    void			setElasticModel(const ElasticModel&,
						bool doblock,bool pvelonly);
    void			setRayTraceData(const RayTracerData&);

protected:

    virtual			~ModelBasedAngleComputer()	{}

private:

    void			splitModelIfNeeded();
};

} // namespace PreStack
