#ifndef seis2dlinemerge_h
#define seis2dlinemerge_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2009
 RCS:		$Id: seis2dlinemerge.h,v 1.2 2009-12-15 16:15:25 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "executor.h"
#include "samplingdata.h"
class MultiID;
class SeisTrcBuf;
class SeisIOObjInfo;
class Seis2DLineSet;
class BufferStringSet;
class Seis2DLinePutter;
namespace PosInfo { class Line2DData; }


/*!\brief merges two 2D lines into a new one (same Line Set) */

mClass Seis2DLineMerger : public Executor
{
public:

    enum Opt		{ MatchTrcNr, MatchCoords, SimpleAppend };

    			Seis2DLineMerger(const MultiID&);
    			~Seis2DLineMerger();

    const char*		message() const		{ return msg_.buf(); }
    const char*		nrDoneText() const	{ return nrdonemsg_.buf(); }
    od_int64		totalNr() const		{ return totnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    int			nextStep();

    MultiID		lsID() const;

    Opt			opt_;
    BufferString	lnm1_;
    BufferString	lnm2_;
    BufferString	outlnm_;
    bool		renumber_;
    SamplingData<int>	numbering_;
    double		snapdist_;

protected:

    SeisIOObjInfo&	oinf_;
    Seis2DLineSet*	ls_;
    PosInfo::Line2DData& l2dd1_;
    PosInfo::Line2DData& l2dd2_;
    SeisTrcBuf&		tbuf1_;
    SeisTrcBuf&		tbuf2_;
    SeisTrcBuf&		outbuf_;
    Executor*		fetcher_;
    Seis2DLinePutter*	putter_;
    BufferStringSet&	attrnms_;
    int			curattridx_;
    int			currentlyreading_;
    int			lid1_, lid2_;
    bool		have1_, have2_;

    BufferString	msg_;
    BufferString	nrdonemsg_;
    od_int64		nrdone_;
    od_int64		totnr_;

    int			doWork();
    int			doIO();
    void		mergeBufs();
    bool		getLineID(const char*,int&) const;
    bool		nextAttr();
    bool		nextFetcher();

};


#endif
