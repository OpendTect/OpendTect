#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"
#include "trckeyzsampling.h"
#include "paralleltask.h"

template<class T> class Array3D;

namespace VolProc
{

/*!
\brief Processor for Volume Statistics
*/

mExpClass(VolumeProcessing) StatsCalculator : public Step
{ mODTextTranslationClass(StatsCalculator);
public:
				mDefaultFactoryInstantiation(
					VolProc::Step, StatsCalculator,
					"Volume Statistics",
					tr("Volume Statistics") )

				StatsCalculator();

    static const char*		sZSampMargin()	{ return "Z samples margin"; }

    bool			needsInput() const override	{ return true; }
    void			setStepout( BinID bid ) { stepout_ = bid; }
    void			setZMargin( int nsamp ) { nzsampextra_ = nsamp;}
    void			setStatsType( BufferString str )
							{ statstype_ = str; }

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    TrcKeySampling		getInputHRg(
					const TrcKeySampling&) const override;
//    StepInterval<int>		getInputZRg(const StepInterval<int>&) const;

    Task*			createTask() override;
    bool		needsFullVolume() const override
				{ return false; }

    /* mDeprecated (this function will be protected virtual after 6.0) */
    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
				     const StepInterval<int>&) const override;

protected:

    BinID			stepout_;
    int				nzsampextra_;	//extra on both sides
    BufferString		statstype_;
};


mClass(VolumeProcessing) StatsCalculatorTask : public ParallelTask
{ mODTextTranslationClass(StatsCalculatorTask);
public:
				StatsCalculatorTask(const Array3D<float>&,
						const TrcKeyZSampling& tkzsin,
						const TrcKeyZSampling& tkzsout,
						BinID,int,BufferString,
						Array3D<float>& out);

    static const char*		sKeyEllipse()	{ return "Ellipse"; }
				//for now

    od_int64			totalNr() const override { return totalnr_; }
    uiString			uiMessage() const override;

protected:
    bool			doWork(od_int64,od_int64,int) override;
    void			prepareWork();

    od_int64			nrIterations() const override
				{ return totalnr_; }

    od_int64			totalnr_;

    const Array3D<float>&	input_;
    Array3D<float>&		output_;

    BinID			stepout_;
    int				nzsampextra_;	//extra on both sides
    BufferString		statstype_;
    BufferString		shape_;

    TypeSet<BinID>		positions_;

    TrcKeyZSampling		tkzsin_;
    TrcKeyZSampling		tkzsout_;

    Threads::Lock		datalock_;
};

} // namespace VolProc
