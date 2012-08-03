#ifndef horsampling_h
#define horsampling_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: horsampling.h,v 1.15 2012-08-03 13:00:12 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "ranges.h"
#include "position.h"

class IOPar;


/*\brief Horizontal sampling (inline and crossline range and steps) */

mClass(Basic) HorSampling
{
public:
			HorSampling( bool settoSI=true ) { init(settoSI); }
    HorSampling&	set(const Interval<int>& inlrg,
	    		    const Interval<int>& crlrg);
    			//!< steps copied if available
    void		get(Interval<int>& inlrg,Interval<int>& crlrg) const;
    			//!< steps filled if available
			
    StepInterval<int>	inlRange() const;
    StepInterval<int>	crlRange() const;
    void		setInlRange(const Interval<int>&);
    void		setCrlRange(const Interval<int>&);

    bool		includes( const HorSampling& hs,
	    			  bool ignoresteps=false ) const;
    inline bool		includes( const BinID& bid ) const
			{ return inlOK(bid.inl) && crlOK(bid.crl); }
    inline bool		inlOK( int inl ) const
			{ return inl >= start.inl && inl <= stop.inl && 
			    (step.inl ? !( (inl-start.inl) % step.inl )
				      : inl==start.inl); }

    inline bool		crlOK( int crl ) const
			{ return crl >= start.crl && crl <= stop.crl && 
			    (step.crl ? !( (crl-start.crl) % step.crl )
			     	      : crl==start.crl); }

    inline void		include( const BinID& bid )
			{ includeInl(bid.inl); includeCrl(bid.crl); }
    void		includeInl( int inl );
    void		includeCrl( int crl );
    void		include( const HorSampling&, bool ignoresteps=false );
    bool		isDefined() const;
    void		limitTo(const HorSampling&);
    void		limitToWithUdf(const HorSampling&);
    			/*!< handles undef values +returns reference horsampling
			     nearest limit if horsamplings do not intersect */

    inline int		inlIdx( int inl ) const
			{ return step.inl ? (inl-start.inl) / step.inl 
					  : (inl==start.inl ? 0 : -1); }
    inline int		crlIdx( int crl ) const
			{ return step.crl ? (crl-start.crl) / step.crl
					  : (crl==start.crl ? 0 : -1); }
    inline od_int64	globalIdx( const BinID& bid ) const
			{ return inlIdx(bid.inl)*nrCrl() + crlIdx(bid.crl); }
    BinID		atIndex( int i0, int i1 ) const
			{ return BinID( start.inl + i0*step.inl,
					start.crl + i1*step.crl ); }
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

    bool		getInterSection(const HorSampling&,HorSampling&) const;
    			//!< Returns false if intersection is empty

    void		snapToSurvey();
    			/*!< Checks if it is on valid bids. If not, it will
			     expand until it is */

    bool		operator==( const HorSampling& hs ) const
			{ return hs.start==start && hs.stop==stop 
			    			 && hs.step==step; }

    bool		usePar(const IOPar&);	//!< Keys as in keystrs.h
    void		fillPar(IOPar&) const;	//!< Keys as in keystrs.h
    static void		removeInfo(IOPar&);
    void		toString(BufferString&) const; //!< Nice text for info

    BinID		start;
    BinID		stop;
    BinID		step;

};


//\brief Finds next BinID in HorSampling; initializes to first position

mClass(Basic) HorSamplingIterator
{
public:
    		HorSamplingIterator() : hrg_( true ) { reset(); }
    		HorSamplingIterator( const HorSampling& hs )
		    : hrg_(hs)	{ reset(); }

    void	setSampling( const HorSampling& hs )
		{ hrg_ = hs; reset(); }

    void	reset(bool nextisfirstpos=true)	{ firstpos_ = nextisfirstpos; }
    		/*!<If nextisfirstpos, the next call to next will automatically
		    be hrg_.start. */
    bool	next(BinID&);

protected:

    HorSampling		hrg_;
    bool		firstpos_;

};


#endif

