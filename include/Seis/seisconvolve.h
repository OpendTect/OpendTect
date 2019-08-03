#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		September 2018
________________________________________________________________________

*/

#include "seiscommon.h"

#include "paralleltask.h"
#include "ptrman.h"
#include "uistring.h"

class SeisTrc;
class TaskRunnerProvider;
class Wavelet;


namespace Seis
{


/*! Convolves a set of seismic traces with a given wavelet
*/

mExpClass(Seis) TrcConvolver : public ParallelTask
{ mODTextTranslationClass(TrcConvolver)
public:
			TrcConvolver(const ObjectSet<SeisTrc>&,const Wavelet&,
				     ObjectSet<SeisTrc>* out =0);
			~TrcConvolver();

    virtual uiString	nrDoneText() const final;
    virtual uiString	message() const final	{ return msg_; }

protected:

    virtual od_int64	nrIterations() const final	{ return totalnr_; }

private:

    virtual bool	doPrepare(int) final;
    virtual bool	doWork(od_int64,od_int64,int) final;
    inline bool		inpIsOut() const	{ return &bufin_ == &bufout_; }

    const ObjectSet<SeisTrc>&	bufin_;
    ObjectSet<SeisTrc>&	bufout_;
    ConstRefMan<Wavelet> wavelet_;

    od_int64		totalnr_;
    uiString		msg_;

};


mGlobal(Seis) bool reWavelet(const ObjectSet<SeisTrc>&,const DBKey& wvltrefid,
			     const DBKey& wvlttargetid,ObjectSet<SeisTrc>*,
			     TaskRunnerProvider&,uiString* msg =0);

} // namespace Seis
