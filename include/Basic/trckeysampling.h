#ifndef trckeysampling_h
#define trckeysampling_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
#include "ranges.h"


/*!
\brief Horizontal sampling (inline and crossline range and steps).
*/

mExpClass(Basic) TrcKeySampling
{
public:

			TrcKeySampling()			{ init( true ); }
			TrcKeySampling( bool settoSI )	{ init( settoSI ); }

    TrcKeySampling&	set(const Interval<int>& inlrg,
			    const Interval<int>& crlrg);
			    //!< steps copied if available
    void		get(Interval<int>& inlrg,Interval<int>& crlrg) const;
			    //!< steps filled if available

    StepInterval<int>	inlRange() const;
    StepInterval<int>	crlRange() const;
    void		setInlRange(const Interval<int>&);
    void		setCrlRange(const Interval<int>&);

    bool		includes( const TrcKeySampling& hs,
				  bool ignoresteps=false ) const;
    inline bool		includes( const BinID& bid ) const
			{ return inlOK(bid.inl()) && crlOK(bid.crl()); }
    inline bool		inlOK( int inl ) const
			{ return inl >= start.inl() && inl <= stop.inl() &&
			    (step.inl() ? !( (inl-start.inl()) % step.inl() )
				      : inl==start.inl()); }

    inline bool		crlOK( int crl ) const
			{ return crl >= start.crl() && crl <= stop.crl() &&
			    (step.crl() ? !( (crl-start.crl()) % step.crl() )
				      : crl==start.crl()); }

    inline void		include( const BinID& bid )
			{ includeInl(bid.inl()); includeCrl(bid.crl()); }
    void		includeInl( int inl );
    void		includeCrl( int crl );
    void		include( const TrcKeySampling&, bool ignoresteps=false );
    bool		isDefined() const;
    void		limitTo(const TrcKeySampling&,bool ignoresteps=false);
    void		limitToWithUdf(const TrcKeySampling&);
			    /*!< handles undef values +returns reference HS
				 nearest limit if HS's do not intersect */

    inline int		inlIdx( int inl ) const
			{ return step.inl() ? (inl-start.inl()) / step.inl()
					  : (inl==start.inl() ? 0 : -1); }
    inline int		crlIdx( int crl ) const
			{ return step.crl() ? (crl-start.crl()) / step.crl()
					  : (crl==start.crl() ? 0 : -1); }
    inline od_int64	globalIdx( const BinID& bid ) const
			{ return inlIdx(bid.inl())*nrCrl()
			       + crlIdx(bid.crl()); }
    BinID		atIndex( int i0, int i1 ) const
			{ return BinID( start.inl() + i0*step.inl(),
					start.crl() + i1*step.crl() ); }
    BinID		atIndex( od_int64 globalidx ) const;
    int			nrInl() const;
    int			nrCrl() const;
    inline od_int64	totalNr() const	{ return ((od_int64)nrInl())*nrCrl(); }
    inline bool		isEmpty() const { return nrInl() < 1 || nrCrl() < 1; }

    void		init(bool settoSI=true);
			    //!< Sets to survey values or mUdf(int) (but step 1)
    void		set2DDef();
			    //!< Sets ranges to 0-maxint
    void		normalise();
			    //!< Makes sure start<stop and steps are non-zero
    void		getRandomSet(int nr,TypeSet<BinID>&) const;

    bool		getInterSection(const TrcKeySampling&,TrcKeySampling&) const;
			    //!< Returns false if intersection is empty

    BinID		getNearest(const BinID&) const;
			    /*!< step-snap and outside -> edge.
				Assumes inldist == crldist */
    void		snapToSurvey();
			    /*!< Checks if it is on valid bids. If not, it will
				 expand until it is */

    bool		operator==( const TrcKeySampling& hs ) const
			{ return hs.start==start && hs.stop==stop
						 && hs.step==step; }
    bool		operator!=( const TrcKeySampling& hs ) const
			{ return !(*this==hs); }

    bool		usePar(const IOPar&);	//!< Keys as in keystrs.h
    void		fillPar(IOPar&) const;	//!< Keys as in keystrs.h
    static void		removeInfo(IOPar&);
    void		toString(BufferString&) const; //!< Nice text for info

    BinID		start;
    BinID		stop;
    BinID		step;

};


//!Old name, use the new one in all new code
typedef TrcKeySampling HorSampling;


/*!
\brief Finds next BinID in TrcKeySampling; initializes to first position.
*/

mExpClass(Basic) TrcKeySamplingIterator
{
public:
		TrcKeySamplingIterator() : hrg_( true ) { reset(); }
		TrcKeySamplingIterator( const TrcKeySampling& hs )
		    : hrg_(hs)	{ reset(); }

    void	setSampling( const TrcKeySampling& hs )
		{ hrg_ = hs; reset(); }

    void	reset();
    void	setNextPos(const BinID& bid) { curpos_ = hrg_.globalIdx(bid); }
    bool	next(BinID&) const;

    od_int64	curIdx() const		     { return curpos_; }

protected:

    TrcKeySampling 			hrg_;
    od_int64				totalnr_;
    mutable Threads::Atomic<od_int64>	curpos_;
};



#endif

