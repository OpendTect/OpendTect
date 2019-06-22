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
#include "trckeysampling.h"
#include "executor.h"


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
						    const ZSampling&,
						    od_int64& maxmemusage,
						    int* nrchunks=0);

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
	bool			doPrepare();
	void			releaseData();
	Task&			getTask()		{ return taskgroup_; }
	uiString		errMsg() const		{ return errmsg_; }

	bool			needsStepOutput(Step::ID) const;
	RegularSeisDataPack*	getOutput() const;

    private:

	uiString		errmsg_;
	const ChainExecutor&	chainexec_;
	TaskGroup&		taskgroup_;
	ObjectSet<Step>		steps_;

    };

    bool			scheduleWork();
    void			updateScheduledStepsSampling(
					const TrcKeySampling&,const ZSampling&);
    int				nrChunks(const TrcKeySampling&,const ZSampling&,
					 od_int64& memusage);

    od_int64			calculateMaximumMemoryUsage(
							const TrcKeySampling&,
							const ZSampling&);
    void			adjustStepsNrComponents(Pos::GeomID) const;
    bool			needSplit(od_int64 usage,od_int64& freemem,
					  int& nrchunks) const;

    void			releaseMemory();
    int				calculateLatestEpoch(Step::ID) const;
    bool			getCalculationScope(Step::ID stepid,
						    TrcKeyZSampling&) const;
    Step::ID			getChainOutputStepID() const;
    Step::OutputSlotID		getChainOutputSlotID() const;

    Epoch*			curepoch_;
    bool			isok_;
    Chain&			chain_;
    mutable uiString		errmsg_;
    ObjectSet<TrcKeyZSampling>	tkzss_;
    ObjectSet<Step>		scheduledsteps_;
    ObjectSet<Epoch>		epochs_;
    Chain::Web			web_;
    int				totalnrepochs_;

    RefMan<RegularSeisDataPack> outputdp_;

};

} // namespace VolProc
