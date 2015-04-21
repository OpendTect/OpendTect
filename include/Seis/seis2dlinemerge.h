#ifndef seis2dlinemerge_h
#define seis2dlinemerge_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2009
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "executor.h"
#include "samplingdata.h"
class SeisTrcBuf;
class SeisIOObjInfo;
class Seis2DLineSet;
class BufferStringSet;
class Seis2DLinePutter;
namespace PosInfo { class Line2DData; }


/*!\brief merges two 2D lines into a new one (same Line Set) */

mExpClass(Seis) Seis2DLineMerger : public Executor
{ mODTextTranslationClass(Seis2DLineMerger)
public:

    enum Opt		{ MatchTrcNr, MatchCoords, SimpleAppend };

    			Seis2DLineMerger(const MultiID&);
			Seis2DLineMerger(const BufferStringSet& datanms);
    			~Seis2DLineMerger();

    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return nrdonemsg_; }
    od_int64		totalNr() const		{ return totnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    void		setOutGeomID( const Pos::GeomID& outgid )
			{ outgeomid_ = outgid; }
    int			nextStep();

    MultiID		lsID() const; //deprecated

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
    Pos::GeomID		outgeomid_;

    uiString		msg_;
    uiString		nrdonemsg_;
    od_int64		nrdone_;
    od_int64		totnr_;

    int			doWork();
    bool		getLineID(const char*,int&) const;//deprecated
    bool		nextAttr();
    bool		nextFetcher();
    void		mergeBufs();
    void		makeBufsCompat();
    void		mergeOnCoords();
    void		doMerge(const TypeSet<int>&,bool);

};


#endif

