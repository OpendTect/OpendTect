#ifndef seisbuf_h
#define seisbuf_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		29-1-98
 RCS:		$Id: seisbuf.h,v 1.5 2002-11-21 17:10:37 bert Exp $
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

			SeisTrcBuf()		{}
			SeisTrcBuf( const SeisTrcBuf& b )
						{ b.fill( *this ); }

    virtual SeisTrcBuf*	clone() const
			{ SeisTrcBuf* b = new SeisTrcBuf; fill(*b); return b; }
    void		fill(SeisTrcBuf&) const;

    void		deepErase()		{ ::deepErase(trcs); }
    void		erase()			{ trcs.erase(); }

    int			size() const		{ return trcs.size(); }
    void		insert(SeisTrc*,int);
    void		add( SeisTrc* t )	{ trcs += t; }
    void		add( SeisTrcBuf& tb )
			{
			    for ( int idx=0; idx<tb.size(); idx++ )
				add( tb.get(idx) );
			}

    int			find(const BinID&) const;
    int			find(SeisTrc*) const;
    SeisTrc*		get( int idx )		{ return trcs[idx]; }
    const SeisTrc*	get( int idx ) const	{ return trcs[idx]; }
    void		remove( SeisTrc* t )	{ if ( t ) trcs -= t;  }
    SeisTrc*		remove( int idx )
			{ SeisTrc* t = trcs[idx]; if ( t ) trcs -= t; return t;}

    void		fill(SeisPacketInfo&) const;
    void		transferData(FloatList&,int takeeach=1,
				     int icomp=0) const;

    void		revert();
    void		sort(int seisinfo_attrnr=6,bool ascending=true);
    			//!< See seisinfo.h ,
    			//!< Default will sort on xline. Inline is 5.
    void		removeDuplicates(int seisinfo_attrnr=6,
	    				 bool destroy=true);

protected:

    ObjectSet<SeisTrc>	trcs;

    int			probableIdx(const BinID&) const;

};


class XFunction;

class SeisGather : public SeisTrcBuf
{
public:

			SeisGather()		{}
			SeisGather( const SeisTrcBuf& sb )
			: SeisTrcBuf(sb)	{}
			SeisGather( const SeisGather& sg )
			: SeisTrcBuf(sg)	{}

    virtual mPolyRet(SeisTrcBuf,SeisGather)* clone() const
			{ SeisGather* g = new SeisGather; fill(*g); return g; }

    void		getStack(SeisTrc&,const StepInterval<int>&,
				 int icomp=0) const;
    XFunction*		getValues(float t,const StepInterval<int>&,
				  int seisinfoattrnr,int icomp=0) const;

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
