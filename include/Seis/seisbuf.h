#ifndef seisbuf_h
#define seisbuf_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		29-1-98
 RCS:		$Id$
________________________________________________________________________

*/


#include "seisinfo.h"
#include "executor.h"
class SeisTrc;
class SeisTrcReader;
class SeisPacketInfo;


/*!\brief set of seismic traces.
  
By default, the traces are not managed, but can be destroyed with deepErase().
buffer in which the traces are somehow related.
*/

mClass SeisTrcBuf
{
public:

			SeisTrcBuf( bool ownr )
				: owner_(ownr)	{}
			SeisTrcBuf( const SeisTrcBuf& b )
			    	: owner_(b.owner_) { b.copyInto( *this ); }
    virtual		~SeisTrcBuf()		{ if ( owner_ ) deepErase(); }
    void		setIsOwner( bool yn )	{ owner_ = yn; }
    bool		isOwner() const		{ return owner_; }

    void		copyInto(SeisTrcBuf&) const;
    void		stealTracesFrom(SeisTrcBuf&);
    virtual SeisTrcBuf*	clone() const		{ return new SeisTrcBuf(*this);}

    void		deepErase();
    void		erase()
    			{
			    if ( owner_ ) deepErase();
			    else trcs.erase();
			}

    inline int		size() const		{ return trcs.size(); }
    inline bool		isEmpty() const		{ return trcs.isEmpty(); }
    void		insert(SeisTrc*,int atidx=0);
    inline SeisTrc*	replace( int idx, SeisTrc* t )
						{ return trcs.replace(idx,t); }
    void		add( SeisTrc* t )	{ trcs += t; }
    void		add(SeisTrcBuf&);	//!< shallow copy if not owner

    int			find(const BinID&,bool is2d=false) const;
    int			find(const SeisTrc*,bool is2d=false) const;
    SeisTrc*		get( int idx )		{ return trcs[idx]; }
    const SeisTrc*	get( int idx ) const	{ return trcs[idx]; }
    void		remove( SeisTrc* t )	{ if ( t ) trcs -= t;  }
    SeisTrc*		remove( int idx )
			{ SeisTrc* t = trcs[idx]; if ( t ) trcs -= t; return t;}

    SeisTrc*		first()		{ return isEmpty()?0:get(0); }
    const SeisTrc*	first() const	{ return isEmpty()?0:get(0); }
    SeisTrc*		last()		{ return isEmpty()?0:get(size()-1); }
    const SeisTrc*	last() const	{ return isEmpty()?0:get(size()-1); }

    void		revert();
    void		fill(SeisPacketInfo&) const;

    bool		isSorted(bool ascending,SeisTrcInfo::Fld) const;
    void		sort(bool ascending,SeisTrcInfo::Fld);
    void		enforceNrTrcs(int nrrequired,SeisTrcInfo::Fld,
	    			      bool stack_before_remove=false);
    			//!< Makes sure nrtrcs per position is constant
    float*		getHdrVals(SeisTrcInfo::Fld,double& offs);
    			//!< The 'offs' ensures the values fit in floats
    			//!< returned new float [] becomes yours

    bool		dump(const char* filenm,bool is2d,bool isps,
	    		     int icomp=0) const;
    			//!< Simple file Ascii format

protected:

    ObjectSet<SeisTrc>	trcs;
    bool		owner_;

    int			probableIdx(const BinID&,bool is2d) const;

};


mClass SeisBufReader : public Executor
{
public:
    			SeisBufReader(SeisTrcReader&,SeisTrcBuf&);

    const char*		message() const		{ return msg_.buf(); }
    const char*		nrDoneText() const	{ return "Traces read"; }
    od_int64		nrDone() const		{ return buf_.size(); }
    od_int64		totalNr() const		{ return totnr_; }
    int			nextStep();

protected:

    SeisTrcReader&	rdr_;
    SeisTrcBuf&		buf_;
    int			totnr_;
    BufferString	msg_;

};


#endif
