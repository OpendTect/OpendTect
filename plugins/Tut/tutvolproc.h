#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"

#include "paralleltask.h"
#include "trckeyzsampling.h"
#include "uistring.h"
#include "volprocstep.h"

template<class T> class Array3D;

namespace VolProc
{

/*!
\brief tutorial for various Volume Operations
*/

mExpClass(Tut) TutOpCalculator : public Step
{ mODTextTranslationClass(TutOpCalculator);
public:
			mDefaultFactoryInstantiation(
					VolProc::Step, TutOpCalculator,
					"VolumeProcessingTutorial",
					tr("Volume Processing Tutorial") )

			TutOpCalculator();
			~TutOpCalculator();

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    static const char*	sKeyTypeIndex()		{ return "Type Index"; }

private:

    bool		needsInput() const override	{ return true; }
    bool		needsFullVolume() const override { return false; }

    bool		prepareWork();
    Task*		createTask() override;

    void		setStepParameters();
			//Replaced by virtual functions after 6.2

    od_int64		extraMemoryUsage(OutputSlotID,const TrcKeySampling&,
				const StepInterval<int>&) const override
			{ return 0; }

    BinID		shift_;
    int			type_;

};


mExpClass(Tut) TutOpCalculatorTask : public ParallelTask
{ mODTextTranslationClass(TutOpCalculatorTask);
public:
				TutOpCalculatorTask(const Array3D<float>&,
						const TrcKeyZSampling& tkzsin,
						const TrcKeyZSampling& tkzsout,
						int,BinID,Array3D<float>& out);

    od_int64			totalNr() const		{ return totalnr_; }
    uiString			uiMessage() const;

private:

    bool			doWork(od_int64,od_int64,int);

    od_int64			nrIterations() const	{ return totalnr_; }

    od_int64			totalnr_;

    const Array3D<float>&	input_;
    Array3D<float>&		output_;

    BinID			shift_;
    int				type_;

    TrcKeyZSampling		tkzsin_;
    TrcKeyZSampling		tkzsout_;
};

} // namespace VolProc
