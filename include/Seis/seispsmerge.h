#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "executor.h"
#include "trckeyzsampling.h"
#include "uistring.h"
class IOObj;
class SeisTrc;
class SeisTrcBuf;
class SeisPSReader;
class SeisTrcWriter;
class SeisResampler;
namespace Seis { class SelData; }


/*!\brief Prestack seismic data merger

  The order in which the stores are in the ObjectSet is important: the first
  data store at a position will be used.

*/

mExpClass(Seis) SeisPSMerger : public Executor
{ mODTextTranslationClass(SeisPSMerger);
public:

			SeisPSMerger(const ObjectSet<const IOObj>& in,
				     const IOObj& out, bool dostack,
				     const Seis::SelData* sd=0);
    virtual		~SeisPSMerger();

    void		setOffsetRange( float r0, float r1 )
			{ offsrg_.start = r0; offsrg_.stop = r1; }

    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Gathers written"); }
    od_int64		nrDone() const override		{ return nrdone_; }
    od_int64		totalNr() const override	{ return totnr_; }
    int			nextStep() override;

protected:

    Seis::SelData*	sd_;
    SeisResampler*	resampler_;
    Interval<float>	offsrg_;

    TrcKeySamplingIterator*	iter_;
    ObjectSet<SeisPSReader>	readers_;
    SeisTrcWriter*		writer_;

    bool		dostack_;
    uiString		msg_;
    int			totnr_;
    int			nrdone_;

    void		init(const TrcKeySampling&);
    void		stackGathers(SeisTrcBuf&,const ObjectSet<SeisTrcBuf>&);
};


mExpClass(Seis) SeisPSCopier : public SeisPSMerger
{
public:
			SeisPSCopier(const IOObj& in,const IOObj& out,
				     const Seis::SelData* sd=0);
			~SeisPSCopier();
protected:

    ObjectSet<const IOObj>*	objs_;
    ObjectSet<const IOObj>&	mkObjs(const IOObj&);

};
