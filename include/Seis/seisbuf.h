#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		29-1-98
________________________________________________________________________

*/


#include "seisinfo.h"
#include "executor.h"
#include "uistring.h"
class SeisTrc;
class SeisPacketInfo;
namespace Seis { class Provider; }


/*!\brief set of seismic traces.

By default, the traces are not managed, but can be destroyed with deepErase().
buffer in which the traces are somehow related.
*/

mExpClass(Seis) SeisTrcBuf
{ mIsContainer( SeisTrcBuf, ObjectSet<SeisTrc>, trcs_ )
public:

    typedef TypeSet<float>	ZValueSet;
    typedef ObjectSet<SeisTrc>	TrcSet;
    mUseType( Pos,		IdxPair );

			SeisTrcBuf( bool ownr )
				: owner_(ownr)		{}
			SeisTrcBuf( const SeisTrcBuf& oth )
				: owner_(oth.owner_)	{ oth.copyInto(*this); }
			SeisTrcBuf( const TrcSet& ts )
				: owner_(false)		{ trcs_ = ts; }

			    // shallow copy, do not set yourself to be owner!

    virtual		~SeisTrcBuf()		{ if ( owner_ ) deepErase(); }

    inline void		setIsOwner( bool yn )	{ owner_ = yn; }
    inline bool		isOwner() const		{ return owner_; }

    void		copyInto(SeisTrcBuf&) const;
    void		stealTracesFrom(SeisTrcBuf&);
    void		addTrcsFrom(TrcSet&);
    void		ensureCompatible(const TrcSet&);
    virtual SeisTrcBuf*	clone() const		{ return new SeisTrcBuf(*this);}

    void		deepErase();
    inline void		setEmpty()
			{ if ( owner_ ) deepErase(); else trcs_.erase(); }
    inline void		erase()			{ setEmpty(); }

    inline int		size() const		{ return trcs_.size(); }
    inline bool		isEmpty() const		{ return trcs_.isEmpty(); }
    inline bool		validIdx( od_int64 idx ) const
			{ return trcs_.validIdx(idx); }
    int			maxTrcSize() const;
    void		insert(SeisTrc*,int atidx=0);
    inline SeisTrc*	replace( int idx, SeisTrc* t )
			{ return trcs_.replace(idx,t); }
    inline void		add( SeisTrc* t )	{ trcs_ += t; }
    void		add(SeisTrcBuf&);	//!<shallow copy if not owner

    ZGate		zRange() const;
    ZGate		getZGate4Shifts(const ZValueSet&,bool upward) const;
    void		getShifted(ZGate,const ZValueSet&,bool upward,
				    float udfval,SeisTrcBuf&) const;

    int			find(const BinID&) const;
    int			find(const Bin2D&) const;
    int			findTrcNr(int) const;
    int			find(const SeisTrc*) const;
    inline SeisTrc*	get( int idx )
			{ return trcs_.validIdx(idx) ? trcs_[idx] : 0; }
    inline const SeisTrc* get( int idx ) const
			{ return trcs_.validIdx(idx) ? trcs_[idx] : 0; }
    inline void		remove( SeisTrc* t )	{ if ( t ) trcs_ -= t;  }
    inline SeisTrc*	remove( int idx )
			{
			    if ( !trcs_.validIdx(idx) )
				return 0;

			    SeisTrc* t = trcs_[idx];
			    if (t) trcs_-=t; return t;
			 }

    SeisTrc*		first()		{ return trcs_.first(); }
    const SeisTrc*	first() const	{ return trcs_.first(); }
    SeisTrc*		last()		{ return trcs_.last(); }
    const SeisTrc*	last() const	{ return trcs_.last(); }

    void		revert(); // last becomes first
    void		fill(SeisPacketInfo&) const;

    bool		isSorted(bool ascending,SeisTrcInfo::Fld) const;
    void		sort(bool ascending,SeisTrcInfo::Fld);
    void		sortForWrite(bool is2d);
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

    bool		owner_;

    int			probableIdx(const IdxPair&) const;
    int			doFind(const IdxPair&) const;

public:

			// usually not a good idea to peek into the impl
    TrcSet&		trcSet()	{ return trcs_; }
    const TrcSet&	trcSet() const	{ return trcs_; }

    static void		ensureCompatible(const TrcSet&,TrcSet&);
    static int		maxTrcSize(const TrcSet&);

};

mDefContainerSwapFunction( Seis, SeisTrcBuf )


mExpClass(Seis) SeisBufReader : public Executor
{ mODTextTranslationClass(SeisBufReader);
public:
			SeisBufReader(Seis::Provider&,SeisTrcBuf&);

    uiString		message() const	{ return msg_; }
    uiString		nrDoneText() const	{ return tr("Traces read"); }
    od_int64		nrDone() const		{ return buf_.size(); }
    od_int64		totalNr() const		{ return totnr_; }
    int			nextStep();

protected:

    Seis::Provider&	prov_;
    SeisTrcBuf&		buf_;
    od_int64		totnr_;
    uiString		msg_;

};
