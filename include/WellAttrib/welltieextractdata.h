#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "executor.h"
#include "position.h"
#include "uistring.h"

class TrcKeyZSampling;
class IOObj;
class SeisTrcReader;
class SeisTrcBuf;
class SeisTrc;

namespace WellTie
{

mExpClass(WellAttrib) SeismicExtractor : public Executor
{ mODTextTranslationClass(SeismicExtractor);
public:
			SeismicExtractor(const IOObj&);
			~SeismicExtractor();

    int			nextStep() override;
    od_int64		totalNr() const override
			{ return extrintv_.nrSteps(); }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiMessage() const override
			{ return tr("Computing..."); }
    uiString		uiNrDoneText() const override
			{ return tr("Points done"); }
    void		setBIDValues(const TypeSet<BinID>&);
    void		setInterval(const StepInterval<float>&);
    //Only 2D
    void		setLine( const BufferString& nm ) { linenm_ = nm; }

    const SeisTrc&	result() const		{ return *outtrc_; }
    uiString		errMsg() const		{ return errmsg_; }

protected:
			mOD_DisableCopy(SeismicExtractor);

    const char*		attrnm_;
    int			nrdone_ = 0;
    int			radius_ = 1;
    TrcKeyZSampling*	tkzs_;
    TypeSet<BinID>	bidset_;
    SeisTrc*		outtrc_ = nullptr;
    SeisTrcBuf*		trcbuf_;
    SeisTrcReader*	rdr_;
    StepInterval<float> extrintv_;
    BufferString	linenm_;
    uiString		errmsg_;

    bool		collectTracesAroundPath();
};

} // namespace WellTie
