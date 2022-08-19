#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocchain.h"
#include "executor.h"

class JobCommunic;

namespace VolProc
{

/*!\brief executes the work for a single VolProc::Chain. */

mExpClass(VolumeProcessing) ChainExecutor : public Executor
{ mODTextTranslationClass(ChainExecutor);
public:
				ChainExecutor(Chain&);
				~ChainExecutor();

				/*!< Mandatory before the execution is started.
				  The execution can start if success.
				  However if nrchunks > 1, the execution
				  will use a smaller sampling than requested
				  to avoid running short of memory.
				 */
    bool			setCalculationScope(const TrcKeySampling&,
						    const StepInterval<float>&,
						    od_uint64& maxmemusage,
						    int* nrchunks=0);

    uiString			errMsg() const;
    uiString			uiNrDoneText() const override;

    ConstRefMan<RegularSeisDataPack>	getOutput() const;
    virtual int			nextStep() override;
    od_int64			nrDone() const override;
    od_int64			totalNr() const override;
    uiString			uiMessage() const override;
    static uiString		sGetStepErrMsg();

    bool			areSamplesIndependent() const;
    bool			needsFullVolume() const;

    void			controlWork(Task::Control) override;
    void			setJobCommunicator(JobCommunic*);

private:

    class Epoch
    {
    public:
				Epoch(const ChainExecutor&);
				~Epoch()		{ delete &taskgroup_; }

	void			addStep(Step* s)	{ steps_ += s; }
	const ObjectSet<Step>&	getSteps() const	{ return steps_; }
	BufferString		errMsg() const		{ return errmsg_; }

	bool			updateInputs();
	bool			doPrepare(ProgressMeter* progmeter=0);
	void			releaseData();
	Task&			getTask()		{ return taskgroup_; }

	bool			needsStepOutput(Step::ID) const;
	RefMan<RegularSeisDataPack>	getOutput();

    private:

	BufferString		errmsg_;
	const ChainExecutor&	chainexec_;
	TaskGroup&		taskgroup_;
	ObjectSet<Step>		steps_;
    };

    bool			scheduleWork();
    void			updateScheduledStepsSampling(
					    const TrcKeySampling&,
					    const StepInterval<float>&);
    int				nrChunks(const TrcKeySampling&,
					 const StepInterval<float>&,
					 od_uint64& memusage);
    od_uint64			calculateMaximumMemoryUsage(
					    const TrcKeySampling&,
					    const StepInterval<float>&);
    void			progressChanged(CallBacker*);

    void			releaseMemory();
    int				computeLatestEpoch(Step::ID) const;
    mDeprecatedDef void		computeComputationScope(Step::ID stepid,
				    TrcKeySampling& stepoutputhrg,
				    StepInterval<int>& stepoutputzrg ) const;
    bool			getCalculationScope(Step::ID stepid,
						    TrcKeyZSampling&) const;
    mDeprecatedDef float	getSampleShift(float) const;
    int				getStepEpochIndex(Step::ID) const;
    mDeprecatedDef od_int64	getStepOutputMemory(Step::ID,int nr,
				    const TypeSet<TrcKeySampling>& epochstks,
				    const TypeSet<StepInterval<int> >&) const;
    void			adjustStepsNrComponents(bool is2d);
    bool			checkAndSplit(od_int64 usage,od_int64& freemem,
					      int& nrchunks) const;

    Epoch*			curepoch_		= nullptr;
    bool			isok_			= false;
    Chain&			chain_;
    TrcKeySampling		outputhrg_; // deprecated
    StepInterval<int>		outputzrg_; // deprecated
    mutable uiString		errmsg_;
    ObjectSet<Step>		scheduledsteps_;
    ObjectSet<Epoch>		epochs_;
    Chain::Web			web_;
    int				totalnrepochs_		= 1;
    ObjectSet<TrcKeyZSampling>	stepstkzs_;

    RefMan<RegularSeisDataPack> outputdp_;
    JobCommunic*		jobcomm_		= nullptr;

    friend class ChainOutput;
};

} // namespace VolProc
