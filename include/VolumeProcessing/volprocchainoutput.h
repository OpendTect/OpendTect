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

    void			setChainID(const DBKey&);
    void			setOutputID(const DBKey&);
    void			setTrcKeyZSampling(const TrcKeyZSampling&);

    void			usePar(const IOPar&);

    virtual od_int64		nrDone() const;
    virtual od_int64		totalNr() const;
    virtual uiString		message() const;
    virtual uiString		nrDoneText() const;
    virtual int			nextStep();

    virtual void		setProgressMeter(ProgressMeter*);
    virtual void		controlWork(Control);
    virtual void		enableWorkControl(bool yn=true);
    void			setJobCommunicator(JobCommunic*);

private:

    DBKey			chainid_;
    DBKey			outid_;
    TrcKeyZSampling		tkzs_;
    IOPar*			chainpar_;

    Chain*			chain_;
    ChainExecutor*		chainexec_;
    SeisDataPackWriter*		wrr_;
    bool			neednextchunk_;
    int				nrexecs_;
    int				curexecnr_;
    TrcKeySampling		scopetks_;
    TrcKeySampling		scheduledtks_;
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
    void			startWriteChunk();
    void			manageStorers();
    void			reportFinished(ChainOutputStorer&);

    friend class		ChainOutputStorer;

};

} // namespace VolProc
