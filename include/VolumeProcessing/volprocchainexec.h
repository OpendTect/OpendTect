#ifndef volprocchainexec_h
#define volprocchainexec_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
 RCS:		$Id$
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

    od_int64			computeMaximumMemoryUsage(const TrcKeySampling&,
						const StepInterval<int>&);

    uiString			errMsg() const;
    uiString			uiNrDoneText() const;

    bool			setCalculationScope(const TrcKeySampling&,
						    const StepInterval<int>&);
    void			setOutputZSampling(const StepInterval<float>&);

    const RegularSeisDataPack*	getOutput() const;
    virtual int			nextStep();
    virtual od_int64		nrDone() const;
    virtual od_int64		totalNr() const;
    virtual uiString		uiMessage() const;
    static uiString		sGetStepErrMsg();

    bool			areSamplesIndependent() const;
    bool			needsFullVolume() const;

    void			controlWork(Task::Control);
    void			setJobCommunicator(JobCommunic*);

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
	const RegularSeisDataPack* getOutput() const;

    private:

	BufferString		errmsg_;
	const ChainExecutor&	chainexec_;
	TaskGroup&		taskgroup_;
	ObjectSet<Step>		steps_;
    };

    bool			scheduleWork();
    void			releaseMemory();
    int				computeLatestEpoch(Step::ID) const;
    void			computeComputationScope(Step::ID stepid,
				    TrcKeySampling& stepoutputhrg,
				    StepInterval<int>& stepoutputzrg ) const;
    float			getSampleShift(float zstart) const;

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

    const RegularSeisDataPack*	outputdp_;
    JobCommunic*		jobcomm_;

    friend class ChainOutput;

};

} // namespace VolProc

#endif
