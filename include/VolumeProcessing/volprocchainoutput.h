#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "executor.h"
#include "multiid.h"
#include "trckeyzsampling.h"
#include "threadlock.h"

class IOPar;
class JobCommunic;
class ProgressRecorder;
class SeisDataPackWriter;


namespace VolProc
{

class Chain;
class ChainExecutor;
class ChainOutputStorer;

/*!\brief Manages output creation for a VolProc::Chain. */

mExpClass(VolumeProcessing) ChainOutput : public Executor
{ mODTextTranslationClass(ChainOutput);
public:

				ChainOutput();
				~ChainOutput();

    void			setChainID(const MultiID&);
    void			setOutputID(const MultiID&);
    void			setTrcKeyZSampling(const TrcKeyZSampling&);
    bool			setCalculationScope(const TrcKeySampling&,
						    const StepInterval<int>&);
    void			usePar(const IOPar&);

    od_int64			nrDone() const override;
    od_int64			totalNr() const override;
    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;
    int				nextStep() override;

    void			setProgressMeter(ProgressMeter*) override;
    void			controlWork(Control) override;
    void			enableWorkControl(bool yn=true) override;
    void			setJobCommunicator(JobCommunic*);

protected:

    MultiID			chainid_;
    MultiID			outid_;
    TrcKeyZSampling		cs_;
    TrcKeySampling		tkscalcscope_;
    TrcKeySampling		tkscalcdone_;
    IOPar* chainpar_;

    Chain*			chain_;
    ChainExecutor*		chainexec_;
    SeisDataPackWriter*		wrr_;
    bool			neednextchunk_;
    int				nrexecs_;
    int				curexecnr_;
    StepInterval<int>		outputzrg_;
    ProgressRecorder&		progresskeeper_;

    mutable Threads::Lock	storerlock_;
    ObjectSet<ChainOutputStorer> storers_;
    ObjectSet<ChainOutputStorer> toremstorers_;
    bool			storererr_;
    JobCommunic*		jobcomm_;

    int				getChain();
    int				setupChunking();
    int				setNextChunk();
    bool			openOutput();
    void			createNewChainExec();
    int				retError(const uiString&);
    int				retMoreToDo();
    void			startWriteChunk();
    void			manageStorers();
    void			reportFinished(ChainOutputStorer&);

    friend class		ChainOutputStorer;

};

} // namespace VolProc
