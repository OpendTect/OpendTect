#ifndef cubesampling_h
#define cubesampling_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2002
 RCS:           $Id: cubesampling.h,v 1.16 2004-09-20 12:04:01 bert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "position.h"
class IOPar;


/*\brief Horizontal sampling (inline and crossline range and steps) */

struct HorSampling
{
			HorSampling( bool settoSI=true ) { init(settoSI); }
    HorSampling&	set(const Interval<int>& inlrg,
	    		    const Interval<int>& crlrg);
    			//!< steps copied if available
    void		get(Interval<int>& inlrg,Interval<int>& crlrg) const;
    			//!< steps filled if available

    inline bool		includes( const BinID& bid ) const
			{ return inlOK(bid.inl) && crlOK(bid.crl); }

    inline bool		inlOK( int inl ) const
			{ return inl >= start.inl && inl <= stop.inl
			      && !( (inl-start.inl) % step.inl ); }
    inline bool		crlOK( int crl ) const
			{ return crl >= start.crl && crl <= stop.crl
			      && !( (crl-start.crl) % step.crl ); }

    inline void		include( const BinID& bid )
			{ includeInl(bid.inl); includeCrl(bid.crl); }
    inline void		includeInl( int inl )
			{ if ( inl < start.inl ) start.inl = inl;
			  if ( inl > stop.inl ) stop.inl = inl; }
    inline void		includeCrl( int crl )
			{ if ( crl < start.crl ) start.crl = crl;
			  if ( crl > stop.crl ) stop.crl = crl; }
    void		limitTo(const HorSampling&);

    inline int		nrInl() const
			{ return step.inl ? (stop.inl-start.inl) / step.inl + 1
			    		  : 0; }
    inline int		nrCrl() const
			{ return step.crl ? (stop.crl-start.crl) / step.crl + 1
			    		  : 0; }
    inline int		totalNr() const
			{ return nrInl() * nrCrl(); }
    inline bool		isEmpty() const
			{ return nrInl() < 1 || nrCrl() < 1; }

    void		init(bool settoSI=true);
    			//!< Sets to survey values or zeros (step 1)
    void		normalise();
    			//!< Makes sure start<stop and steps are non-zero

    bool		getInterSection(const HorSampling&,HorSampling&) const;
    			//!< Returns false if intersection is empty

    void		snapToSurvey(bool work=true);
    			/*!< Checks if it is on valid bids. If not, it will
			     expand until it is */

    bool		operator==( const HorSampling& hs ) const
			{ return hs.start==start && hs.stop==stop 
			    			 && hs.step==step; }

    bool		usePar(const IOPar&);	//!< Keys as in keystrs.h
    void		fillPar(IOPar&) const;	//!< Keys as in keystrs.h
    static void		removeInfo(IOPar&);

    BinID		start;
    BinID		stop;
    BinID		step;

    static const char*	startstr;
    static const char*	stopstr;
    static const char*	stepstr;

};


/*\brief Hor+Vert sampling in 3D surveys */

struct CubeSampling
{
public:

    			CubeSampling( bool settoSI=true )
			: hrg(settoSI)		{ init(settoSI); }

    void		init(bool settoSI=true);
    			//!< Sets to survey values or zeros (step 1)
    void		normalise();
    			//!< Makes sure start<stop and steps are non-zero

    HorSampling		hrg;
    StepInterval<float>	zrg;

    inline int		nrInl() const		{ return hrg.nrInl(); }
    inline int		nrCrl() const		{ return hrg.nrCrl(); }
    inline int		nrZ() const		{ return zrg.nrSteps() + 1; }
    // int totalNr() was removed because for large surveys, it doesn't
    // fit into an int anymore!!
    inline bool		isEmpty() const
			{ return (hrg.start.crl == 0 && hrg.stop.crl == 0)
			      || hrg.isEmpty() || nrZ() < 1; }
    bool		getInterSection(const CubeSampling&,
	    				CubeSampling&) const;
    			//!< Returns false if intersection is empty
    void		include(const CubeSampling&);
    void		limitTo(const CubeSampling&);

    void		snapToSurvey(bool work=true);
    			/*!< Checks if it is on valid bids and sample positions.
			     If not, it will expand until it is */

    bool		operator==( const CubeSampling& cs ) const;
    bool		operator!=( const CubeSampling& cs ) const
			{ return !(cs==*this); }

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;
    static void		removeInfo(IOPar&);

protected:

    static const char*	zrangestr;

};


#endif
