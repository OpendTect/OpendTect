#ifndef seisbuf_h
#define seisbuf_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		29-1-98
 RCS:		$Id: seisbuf.h,v 1.9 2004-07-16 15:35:25 bert Exp $
________________________________________________________________________

*/


#include <seistrc.h>
#include <ranges.h>
#include <executor.h>
class SeisTrc;
class BinID;
class SeisTrcWriter;


/*!\brief set of seismic traces.
  
By default, the traces are not managed, but can be destroyed with deepErase().
buffer in which the traces are somehow related.
*/

class SeisTrcBuf
{
public:

			SeisTrcBuf( bool ownr=false )
				: owner_(ownr)	{}
			SeisTrcBuf( const SeisTrcBuf& b )
			    	: owner_(b.owner_) { b.copyInto( *this ); }
    virtual		~SeisTrcBuf()		{ if ( owner_ ) deepErase(); }
    void		setIsOwner( bool yn )	{ owner_ = yn; }
    bool		isOwner() const		{ return owner_; }

    void		copyInto(SeisTrcBuf&) const;
    void		stealTracesFrom(SeisTrcBuf&);
    virtual SeisTrcBuf*	clone() const		{ return new SeisTrcBuf(*this);}

    void		deepErase()		{ ::deepErase(trcs); }
    void		erase()
    			{
			    if ( owner_ ) deepErase();
			    else trcs.erase();
			}

    int			size() const		{ return trcs.size(); }
    void		insert(SeisTrc*,int);
    void		add( SeisTrc* t )	{ trcs += t; }
    void		add(SeisTrcBuf&);	//!< shallow copy if not owner

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
    			//!< makes a copy of data

    void		revert();
    bool		isSorted(bool ascend=true,int siattrnr=6) const;
    			//!< See sort() for parameters.
    void		sort(bool ascending=true,int seisinfo_attrnr=6);
    			//!< See also seisinfo.h ,
    			//!< Default will sort on xline. Inline is 5.
    void		enforceNrTrcs(int nrrequired,int seisinfo_attrnr=6);
    			//!< Makes sure nrtrcs per position is constant

protected:

    ObjectSet<SeisTrc>	trcs;
    bool		owner_;

    int			probableIdx(const BinID&) const;

};


class XFunction;

/*!\brief SeisTrcBuf in which the traces are somehow related. */

class SeisGather : public SeisTrcBuf
{
public:

			SeisGather()		{}
			SeisGather( const SeisTrcBuf& sb )
			: SeisTrcBuf(sb)	{}
			SeisGather( const SeisGather& sg )
			: SeisTrcBuf(sg)	{}

    virtual mPolyRet(SeisTrcBuf,SeisGather)* clone() const
			{ return new SeisGather(*this);}

    void		getStack(SeisTrc&,const StepInterval<int>&,
				 int icomp=0) const;
    XFunction*		getValues(float t,const StepInterval<int>&,
				  int seisinfoattrnr,int icomp=0) const;

protected:

    StepInterval<int>	getSI(const StepInterval<int>&) const;

};


/*!\brief Executor writing the traces in a SeisTrcBuf. */

class SeisTrcBufWriter : public Executor
{
public:

			SeisTrcBufWriter(const SeisTrcBuf&,SeisTrcWriter&);

    const char*		message() const;
    int			nextStep();
    int			totalNr() const;
    int			nrDone() const;
    const char*		nrDoneText() const;

protected:

    const SeisTrcBuf&	trcbuf;
    SeisTrcWriter&	writer;
    int			nrdone;

};


#endif
