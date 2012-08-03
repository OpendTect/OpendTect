#ifndef seis2dlinemerge_h
#define seis2dlinemerge_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2009
 RCS:		$Id: seis2dlinemerge.h,v 1.6 2012-08-03 13:00:35 cvskris Exp $
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "executor.h"
#include "samplingdata.h"
class MultiID;
class SeisTrcBuf;
class SeisIOObjInfo;
class Seis2DLineSet;
class BufferStringSet;
class Seis2DLinePutter;
template <class T> class TypeSet;
namespace PosInfo { class Line2DData; }


/*!\brief merges two 2D lines into a new one (same Line Set) */

mClass(Seis) Seis2DLineMerger : public Executor
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
    bool		stckdupl_;
    SamplingData<int>	numbering_;
    double		snapdist_;

protected:

    SeisIOObjInfo&	oinf_;
    Seis2DLineSet*	ls_;
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
    int			lid1_, lid2_;
    bool		have1_, have2_;

    BufferString	msg_;
    BufferString	nrdonemsg_;
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


#endif

