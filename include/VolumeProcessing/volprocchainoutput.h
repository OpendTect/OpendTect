#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2016
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "executor.h"
#include "dbkey.h"
#include "trckeyzsampling.h"
#include "threadlock.h"
class SeisDataPackWriter;
class ProgressRecorder;
class JobCommunic;

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

    void			setChainID(const DBKey&);
    void			setOutputID(const DBKey&);
    void			setTrcKeyZSampling(const TrcKeyZSampling&);

    bool			setCalculationScope(const TrcKeySampling&,
						    const StepInterval<int>&);
    void			usePar(const IOPar&);

    virtual od_int64		nrDone() const;
    virtual od_int64		totalNr() const;
    virtual uiString		uiMessage() const;
    virtual uiString		uiNrDoneText() const;
    virtual int			nextStep();

    virtual void		setProgressMeter(ProgressMeter*);
    virtual void		controlWork(Control);
    virtual void		enableWorkControl(bool yn=true);

    void			setJobComm(JobCommunic* comm);

protected:

    DBKey			chainid_;
    DBKey			outid_;
    TrcKeyZSampling		tkzs_;

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

    JobCommunic*		comm_;

    friend class		ChainOutputStorer;

};

} // namespace VolProc
