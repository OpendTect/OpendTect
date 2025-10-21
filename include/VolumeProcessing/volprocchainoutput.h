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
#include "volprocchain.h"

class IOPar;
class JobCommunic;
class ProgressRecorder;
class SeisDataPackWriter;


namespace VolProc
{

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

    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;

    void			setProgressMeter(ProgressMeter*) override;
    void			controlWork(Control) override;
    void			enableWorkControl(bool yn=true) override;
    void			setJobCommunicator(JobCommunic*);

protected:
    int				nextStep() override;

    od_int64			nrDone() const override;
    od_int64			totalNr() const override;

    MultiID			chainid_;
    MultiID			outid_;
    TrcKeyZSampling		cs_;
    TrcKeySampling		tkscalcscope_;
    TrcKeySampling		tkscalcdone_;
    IOPar*			chainpar_	= nullptr;

    RefMan<Chain>		chain_;
    ChainExecutor*		chainexec_	= nullptr;
    SeisDataPackWriter*		wrr_		= nullptr;
    bool			neednextchunk_	= true;
    int				nrexecs_	= -1;
    int				curexecnr_	= -1;
    StepInterval<int>		outputzrg_;
    ProgressRecorder&		progresskeeper_;

    mutable Threads::Lock	storerlock_;
    ObjectSet<ChainOutputStorer> storers_;
    ObjectSet<ChainOutputStorer> toremstorers_;
    bool			storererr_	= false;
    JobCommunic*		jobcomm_	= nullptr;

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
