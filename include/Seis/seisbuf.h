#ifndef seisbuf_h
#define seisbuf_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		29-1-98
 RCS:		$Id: seisbuf.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

This object buffers seismic traces. The traces are not managed, but can be
destroyed with deepErase(). A SeisGather is a buffer in which the traces are
somehow related.

*/


#include <seistrc.h>
#include <ranges.h>
#include <executor.h>
class SeisTrc;
class BinID;
class SeisTrcWriter;


class SeisTrcBuf
{
public:

    virtual SeisTrcBuf*	clone() const
			{ SeisTrcBuf* b = new SeisTrcBuf; fill(*b); return b; }
    void		fill(SeisTrcBuf&) const;

    void		deepErase()		{ ::deepErase(trcs); }
    void		erase()			{ trcs.erase(); }

    int			size() const		{ return trcs.size(); }
    void		add( SeisTrc* t )	{ trcs += t; }
    void		insert(SeisTrc*,int);
    int			find(const BinID&) const;
    int			find(SeisTrc*) const;
    SeisTrc*		get( int idx )		{ return trcs[idx]; }
    const SeisTrc*	get( int idx ) const	{ return trcs[idx]; }
    void		remove( SeisTrc* t )	{ if ( t ) trcs -= t;  }
    SeisTrc*		remove( int idx )
			{ SeisTrc* t = trcs[idx]; if ( t ) trcs -= t; return t;}
    void		revert();

    void		fill(SeisPacketInfo&) const;

protected:

    ObjectSet<SeisTrc>	trcs;

    int			probableIdx(const BinID&) const;

};


class XFunction;


class SeisGather : public SeisTrcBuf
{
public:

    virtual SeisGather*	clone() const
			{ SeisGather* g = new SeisGather; fill(*g); return g; }

    void		getStack(SeisTrc&,const StepInterval<int>&) const;
    XFunction*		getValues(float t,const StepInterval<int>&,
				  int seisinfoattrnr) const;

protected:

    StepInterval<int>	getSI(const StepInterval<int>&) const;

};


class SeisTrcBufWriter : public Executor
{
public:

			SeisTrcBufWriter(const SeisTrcBuf&,SeisTrcWriter&);
			~SeisTrcBufWriter();

    const char*		message() const;
    int			nextStep();
    int			totalNr() const;
    int			nrDone() const;
    const char*		nrDoneText() const;

protected:

    const SeisTrcBuf&	trcbuf;
    SeisTrcWriter&	writer;
    Executor*		starter;
    int			nrdone;

};


#endif
