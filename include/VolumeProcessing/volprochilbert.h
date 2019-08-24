#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		05-07-2017
________________________________________________________________________
-*/

#include "hilberttransform.h"
#include "paralleltask.h"
#include "polynd.h"
#include "position.h"
#include "threadlock.h"
#include "volprocstep.h"

namespace PosInfo { class CubeData; }

namespace VolProc
{

mExpClass(VolumeProcessing) HilbertCalculator : public VolProc::Step
{ mODTextTranslationClass(HilbertCalculator)
public:

			mDefaultFactoryInstantiation(
				Step, HilbertCalculator,
				"Hilbert", toUiString("Hilbert") )

			HilbertCalculator()
			    : Step()		{}

private:

    virtual ReportingTask*	createTask();

    virtual bool	needsFullVolume() const		{ return false; }
    virtual bool	canInputAndOutputBeSame() const { return false; }
    virtual bool	areSamplesIndependent() const	{ return true; }
    virtual bool	canHandle2D() const		{ return true; }

    virtual od_int64	extraMemoryUsage(OutputSlotID,
					const TrcKeyZSampling&) const
			{ return 0; }
};

}; //namespace


mExpClass(VolumeProcessing) HilbertCalculatorTask : public ParallelTask
{ mODTextTranslationClass(HilbertCalculatorTask)
public:
				mTypeDefArrNDTypes;
				HilbertCalculatorTask(const Array2D<float>&,
						      Array2D<float>&);
				HilbertCalculatorTask(const Array3D<float>&,
						      Array3D<float>&);

    void			setTracePositions(const PosInfo::CubeData&,
						  const TrcKeySampling&);

    uiString			message() const { return msg_; }
    uiString			nrDoneText() const { return sTracesDone(); }

protected:

    od_int64			nrIterations() const	{ return totalnr_; }


private:
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    static void			muteHeadTailHilbert(const Array1D<float>& real,
						    Array1D<float>& imag);

    const ArrayND<float>&	realdata_;
    ArrayND<float>&		imagdata_;
    const PosInfo::CubeData*	trcposns_;
    const TrcKeySampling*	tks_;

    const bool			is3d_;
    od_int64			totalnr_;
    Threads::Atomic<od_int64>	nexttrcidx_;
    uiString			msg_;

};
