#ifndef seispsmerge_h
#define seispsmerge_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		Oct 2007
 RCS:		$Id: seispsmerge.h,v 1.10 2011/10/07 13:15:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "cubesampling.h"
class IOObj;
class SeisTrc;
class SeisTrcBuf;
class SeisPSReader;
class SeisTrcWriter;
class SeisResampler;
namespace Seis { class SelData; }


/*!\brief Pre-Stack seismic data merger

  The order in which the stores are in the ObjectSet is important: the first
  data store at a position will be used.

*/

mClass SeisPSMerger : public Executor
{
public:
			SeisPSMerger(const ObjectSet<IOObj>& in,
				     const IOObj& out, bool dostack,
				     const Seis::SelData* sd=0);
    virtual		~SeisPSMerger();

    void		setOffsetRange( float r0, float r1 )
			{ offsrg_.start = r0; offsrg_.stop = r1; }

    virtual const char*	message() const		{ return msg_.buf(); }
    virtual const char*	nrDoneText() const	{ return "Gathers written"; }
    virtual od_int64	nrDone() const		{ return nrdone_; }
    virtual od_int64	totalNr() const		{ return totnr_; }
    virtual int		nextStep();

protected:

    BinID		curbid_;
    Seis::SelData*	sd_;
    SeisResampler*	resampler_;
    Interval<float>	offsrg_;

    HorSamplingIterator*	iter_;
    ObjectSet<SeisPSReader>	readers_;
    SeisTrcWriter*		writer_;

    bool		dostack_;
    BufferString	msg_;
    int			totnr_;
    int			nrdone_;

    void		init(const HorSampling&);
    void		stackGathers(SeisTrcBuf&,const ObjectSet<SeisTrcBuf>&);
};


#endif
