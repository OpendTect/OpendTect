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
#include "reflectivitymodel.h"
#include "windowfunction.h"

template <class T> class Array2D;
class FFTFilter;
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
    void			setRayTracerPars(const IOPar&);
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

    virtual const ElasticModel* curElasticModel() const = 0;
    virtual const OffsetReflectivityModel* curRefModel() const = 0;
    virtual void		setRefModel(const OffsetReflectivityModel&) = 0;

    IOPar			iopar_;
    IOPar			raypars_;
    FlatPosData			outputsampling_;
    ElasticModel		elasticmodel_;
    ConstRefMan<OffsetReflectivityModel> refmodel_;
    float			thresholdparam_ = 0.01f;
    float			maxthickness_ = 25.f;
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
    bool			isOK() const override { return velsource_; }

    Gather*			computeAngles() override;

protected:

    const ElasticModel*		curElasticModel() const override
				{ return &elasticmodel_; }
    const OffsetReflectivityModel* curRefModel() const override;
    void			setRefModel(const OffsetReflectivityModel&)
								      override;

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
				ModelTool(const OffsetReflectivityModel&,
					  const TrcKey&);
				~ModelTool();

	const ElasticModel*	elasticModel() const	{ return em_; }
	const OffsetReflectivityModel* curRefModel() const;
	const TrcKey&		trcKey() const	{ return trckey_; }
	bool			operator ==( const ModelTool& oth ) const
				{ return oth.trcKey() == trckey_; }

	void			setRefModel(const OffsetReflectivityModel&);
	void			splitModelIfNeeded(float maxthickness);

    protected:
	ElasticModel*		em_ = nullptr;
	ConstRefMan<OffsetReflectivityModel> refmodel_;
	TrcKey			trckey_;

    private:
				ModelTool(const ModelTool&) = delete;
	ModelTool&		operator=(const ModelTool&) = delete;
    };

				ModelBasedAngleComputer();
				~ModelBasedAngleComputer();

    void			setElasticModel(const TrcKey&,bool doblock,
						bool pvelonly,ElasticModel&);
    void			setRefModel(const OffsetReflectivityModel&,
					    const TrcKey&);

    bool			isOK() const override;

    Gather*			computeAngles() override;

private:

    const ElasticModel*		curElasticModel() const override;
    const OffsetReflectivityModel* curRefModel() const override;
    void			setRefModel(const OffsetReflectivityModel&)
								      override;

    const ModelTool*		curModelTool() const;
    ModelTool*			curModelTool();

    ObjectSet<ModelTool>	tools_;
};

} // namespace PreStack
