#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2009
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "executor.h"
#include "samplingdata.h"
#include "uistring.h"
class SeisTrcBuf;
class SeisIOObjInfo;
class BufferStringSet;
class Seis2DLinePutter;
namespace PosInfo { class Line2DData; }


/*!\brief merges two 2D lines into a new one (same Line Set) */

mExpClass(Seis) Seis2DLineMerger : public Executor
{ mODTextTranslationClass(Seis2DLineMerger);
public:

    enum Opt		{ MatchTrcNr, MatchCoords, SimpleAppend };

			Seis2DLineMerger(const BufferStringSet& datanms,
					 const Pos::GeomID&);
			~Seis2DLineMerger();

    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override	{ return nrdonemsg_; }
    od_int64		totalNr() const override	{ return totnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }
    int			nextStep() override;


    Opt			opt_;
    BufferString	lnm1_;
    BufferString	lnm2_;
    BufferString	outlnm_;
    bool		renumber_;
    bool		stckdupl_;
    SamplingData<int>	numbering_;
    double		snapdist_;

protected:

    Seis2DDataSet*	ds_;
    PosInfo::Line2DData& l2dd1_;
    PosInfo::Line2DData& l2dd2_;
    PosInfo::Line2DData& outl2dd_;
    SeisTrcBuf&		tbuf1_;
    SeisTrcBuf&		tbuf2_;
    SeisTrcBuf&		outbuf_;
    Executor*		fetcher_;
    Seis2DLinePutter*	putter_;
    BufferStringSet&	attrnms_;
    int			curattridx_;
    int			currentlyreading_;
    Pos::GeomID		lid1_, lid2_;
    bool		have1_, have2_;
    const Pos::GeomID&	outgeomid_;

    uiString		msg_;
    uiString		nrdonemsg_;
    od_int64		nrdone_;
    od_int64		totnr_;

    int			doWork();
    int			doIO();
    bool		getLineID(const char*,int&) const;
    bool		nextAttr();
    bool		nextFetcher();
    void		mergeBufs();
    void		makeBufsCompat();
    void		mergeOnCoords();
    void		doMerge(const TypeSet<int>&,bool);

};


