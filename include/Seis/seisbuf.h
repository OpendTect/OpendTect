#ifndef seisbuf_h
#define seisbuf_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		29-1-98
 RCS:		$Id: seisbuf.h,v 1.17 2007-10-17 04:46:59 cvsnanne Exp $
________________________________________________________________________

*/


#include "seisinfo.h"


/*!\brief set of seismic traces.
  
By default, the traces are not managed, but can be destroyed with deepErase().
buffer in which the traces are somehow related.
*/

class SeisTrcBuf
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
    void		insert(SeisTrc*,int);
    void		add( SeisTrc* t )	{ trcs += t; }
    void		add(SeisTrcBuf&);	//!< shallow copy if not owner

    int			find(const BinID&,bool is2d=false) const;
    int			find(const SeisTrc*,bool is2d=false) const;
    SeisTrc*		get( int idx )		{ return trcs[idx]; }
    const SeisTrc*	get( int idx ) const	{ return trcs[idx]; }
    void		remove( SeisTrc* t )	{ if ( t ) trcs -= t;  }
    SeisTrc*		remove( int idx )
			{ SeisTrc* t = trcs[idx]; if ( t ) trcs -= t; return t;}

    void		revert();
    void		fill(SeisPacketInfo&) const;

    bool		isSorted(bool ascending,SeisTrcInfo::Fld) const;
    void		sort(bool ascending,SeisTrcInfo::Fld);
    void		enforceNrTrcs(int nrrequired,SeisTrcInfo::Fld);
    			//!< Makes sure nrtrcs per position is constant
    float*		getHdrVals(SeisTrcInfo::Fld,double& offs);
    			//!< The 'offs' ensures the values fit in floats
    			//!< returned new float [] becomes yours

protected:

    ObjectSet<SeisTrc>	trcs;
    bool		owner_;

    int			probableIdx(const BinID&,bool is2d) const;

};


#endif
