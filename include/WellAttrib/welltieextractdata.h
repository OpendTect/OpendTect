#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:	       Feb 2009
RCS:	       $Id: welltieextractdata.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
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

    const char*		attrnm_;
    int			nrdone_;
    int			radius_;
    TrcKeyZSampling*	tkzs_;
    TypeSet<BinID>	bidset_;
    SeisTrc*		outtrc_;
    SeisTrcBuf*		trcbuf_;
    SeisTrcReader*	rdr_;
    StepInterval<float> extrintv_;
    BufferString	linenm_;
    uiString		errmsg_;

    bool		collectTracesAroundPath();
};

} // namespace WellTie

