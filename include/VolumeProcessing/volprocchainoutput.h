#ifndef volprocchainoutput_h
#define volprocchainoutput_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2016
 RCS:		$Id$
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
    void			setJobCommunicator(JobCommunic*);

protected:

    MultiID			chainid_;
    MultiID			outid_;
    TrcKeyZSampling		cs_;
    IOPar*			chainpar_;

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

    friend class		ChainOutputStorer;

};

} // namespace VolProc

#endif
