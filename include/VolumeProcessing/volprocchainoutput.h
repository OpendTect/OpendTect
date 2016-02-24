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
    virtual uiString		uiMessage() const	{ return msg_; }
    virtual uiString		uiNrDoneText() const;
    virtual int			nextStep();

protected:

    MultiID			chainid_;
    MultiID			outid_;
    TrcKeyZSampling		cs_;

    uiString			msg_;
    bool			neednextchunk_;
    int				nrexecs_;
    int				curexecnr_;
    StepInterval<int>		outputzrg_;
    bool			calculating_;

    mutable Threads::Lock	storerlock_;
    ObjectSet<ChainOutputStorer> storers_;
    ObjectSet<ChainOutputStorer> toremstorers_;
    bool			storererr_;

    Chain*			chain_;
    ChainExecutor*		chainexec_;
    SeisDataPackWriter*		wrr_;

    int				getChain();
    int				setupChunking();
    int				setNextChunk();
    bool			openOutput();
    void			startWriteChunk();
    void			manageStorers();
    void			reportFinished(ChainOutputStorer&);

    friend class		ChainOutputStorer;

};

} // namespace VolProc

#endif
