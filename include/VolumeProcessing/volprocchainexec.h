#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "volprocchain.h"
#include "executor.h"


namespace VolProc
{

/*!\brief executes the work for a single VolProc::Chain. */

mExpClass(VolumeProcessing) ChainExecutor : public Executor
{ mODTextTranslationClass(ChainExecutor);
public:
				ChainExecutor(Chain&);
				~ChainExecutor();

				/*!< Return 0 if not enough memory for the
				     execution */
    int				nrChunks(const TrcKeySampling&,
					 const StepInterval<int>&,
					 int extranroutcomps=0);

    bool			setCalculationScope(const TrcKeySampling&,
						    const StepInterval<int>&);

    bool			areSamplesIndependent() const;
    bool			needsFullVolume() const;

    const Step::CVolRef		getOutput() const;
    Step::VolRef		getOutput();
    uiString			errMsg() const;

    virtual int			nextStep();
    virtual od_int64		nrDone() const;
    virtual od_int64		totalNr() const;
    virtual uiString		message() const;
    virtual uiString		nrDoneText() const;
    virtual void		controlWork(Control);

    static uiString		sGetStepErrMsg();

private:

    class Epoch
    {
    public:
				Epoch(const ChainExecutor&);
				~Epoch()		{ delete &taskgroup_; }

	void			addStep(Step* s)	{ steps_ += s; }
	const ObjectSet<Step>&	getSteps() const	{ return steps_; }

	bool			updateInputs();
	bool			doPrepare(ProgressMeter* progmeter=0);
	void			releaseData();
	Task&			getTask()		{ return taskgroup_; }

	bool			needsStepOutput(Step::ID) const;
	RegularSeisDataPack*	getOutput() const;

    private:

	BufferString		errmsg_;
	const ChainExecutor&	chainexec_;
	TaskGroup&		taskgroup_;
	ObjectSet<Step>		steps_;

    };

    bool			scheduleWork();
    od_int64		computeMaximumMemoryUsage(const TrcKeySampling&,
						const StepInterval<int>&);
    void			releaseMemory();
    int				computeLatestEpoch(Step::ID) const;
    void			computeComputationScope(Step::ID stepid,
				    TrcKeySampling& stepoutputhrg,
				    StepInterval<int>& stepoutputzrg ) const;
    int				getStepEpochIndex(Step::ID) const;
    od_int64			getStepOutputMemory(Step::ID,int nr,
				    const TypeSet<TrcKeySampling>& epochstks,
				    const TypeSet<StepInterval<int> >&) const;
    Step::ID			getChainOutputStepID() const;
    Step::OutputSlotID		getChainOutputSlotID() const;


    Epoch*			curepoch_;
    bool			isok_;
    Chain&			chain_;
    TrcKeySampling		outputhrg_;
    StepInterval<int>		outputzrg_;
    mutable uiString		errmsg_;
    ObjectSet<Step>		scheduledsteps_;
    ObjectSet<Epoch>		epochs_;
    Chain::Web			web_;
    int				totalnrepochs_;

    RefMan<RegularSeisDataPack> outputdp_;

};

} // namespace VolProc
