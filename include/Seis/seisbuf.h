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


#include "seismod.h"
#include "seisinfo.h"
#include "executor.h"
class SeisTrc;
class SeisTrcReader;
class SeisPacketInfo;


/*!\brief set of seismic traces.
  
By default, the traces are not managed, but can be destroyed with deepErase().
buffer in which the traces are somehow related.
*/

mClass(Seis) SeisTrcBuf
{
public:

			SeisTrcBuf( bool ownr )
				: owner_(ownr)	{}
			SeisTrcBuf( const SeisTrcBuf& b )
			    	: owner_(b.owner_) { b.copyInto( *this ); }
    virtual		~SeisTrcBuf()		{ if ( owner_ ) deepErase(); }
    inline void		setIsOwner( bool yn )	{ owner_ = yn; }
    inline bool		isOwner() const		{ return owner_; }

    void		copyInto(SeisTrcBuf&) const;
    void		stealTracesFrom(SeisTrcBuf&);
    virtual SeisTrcBuf*	clone() const		{ return new SeisTrcBuf(*this);}

    void		deepErase();
    inline void		erase()
    			{ if ( owner_ ) deepErase(); else trcs_.erase(); }

    inline int		size() const		{ return trcs_.size(); }
    inline bool		isEmpty() const		{ return trcs_.isEmpty(); }
    void		insert(SeisTrc*,int atidx=0);
    inline SeisTrc*	replace( int idx, SeisTrc* t )
    						{ return trcs_.replace(idx,t); }
    inline void		add( SeisTrc* t )	{ trcs_ += t; }
    void		add(SeisTrcBuf&);	//!< shallow copy if not owner

    int			find(const BinID&,bool is2d=false) const;
    int			find(const SeisTrc*,bool is2d=false) const;
    inline SeisTrc*	get( int idx )		{ return trcs_[idx]; }
    inline const SeisTrc* get( int idx ) const	{ return trcs_[idx]; }
    inline void		remove( SeisTrc* t )	{ if ( t ) trcs_ -= t;  }
    inline SeisTrc*	remove( int idx )
			{ SeisTrc* t = trcs_[idx]; if (t) trcs_-=t; return t;}

    SeisTrc*		first()		{ return isEmpty()?0:get(0); }
    const SeisTrc*	first() const	{ return isEmpty()?0:get(0); }
    SeisTrc*		last()		{ return isEmpty()?0:get(size()-1); }
    const SeisTrc*	last() const	{ return isEmpty()?0:get(size()-1); }

    void		revert(); // last becomes first
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

    ObjectSet<SeisTrc>	trcs_;
    bool		owner_;

    int			probableIdx(const BinID&,bool is2d) const;

};


mClass(Seis) SeisBufReader : public Executor
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

