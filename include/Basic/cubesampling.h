#ifndef cubesampling_h
#define cubesampling_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2002
 RCS:           $Id: cubesampling.h,v 1.19 2005-03-30 15:52:55 cvsbert Exp $
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

    int			nrInl() const;
    int			nrCrl() const;
    inline int		totalNr() const
			{ return nrInl() * nrCrl(); }
    inline bool		isEmpty() const
			{ return nrInl() < 1 || nrCrl() < 1; }

    void		init(bool settoSI=true);
    			//!< Sets to survey values or mUndefIntVal (but step 1)
    void		set2DDef();
    			//!< Sets ranges to 0-maxint
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


/*\brief Hor+Vert sampling in 3D surveys

  When slices are to be taken from a CubeSampling, they should be ordered
  as follows:
 
  Dir |   Dim1    Dim2
  ----|---------------
  Inl |   Crl     Z
  Crl |   Inl     Z
  Z   |   Inl     Crl

  See also the direction() and dimension() free functions.

 */

struct CubeSampling
{
public:

    			CubeSampling( bool settoSI=true )
			: hrg(settoSI)		{ init(settoSI); }

    enum Dir		{ Z, Inl, Crl };
    Dir			defaultDir() const;
    			//!< 'flattest' direction, i.e. direction with
    			//!< smallest size. If equal, prefer Inl then Crl then Z

    void		init(bool settoSI=true);
    			//!< Sets hrg.init and zrg to survey values or zeros
    inline void		setEmpty()		{ init(false); }
    void		set2DDef();
    			//!< Sets to survey zrange and hrg.set2DDef
    void		normalise();
    			//!< Makes sure start<stop and steps are non-zero

    HorSampling		hrg;
    StepInterval<float>	zrg;

    inline int		nrInl() const		{ return hrg.nrInl(); }
    inline int		nrCrl() const		{ return hrg.nrCrl(); }
    inline int		nrZ() const		{ return zrg.nrSteps() + 1; }
			// No totalNr(): doesn't fit into int in general
    inline int		size( Dir d ) const	{ return d == Inl ? nrInl()
    						      : (d == Crl ? nrCrl()
							          : nrZ()); }
    inline bool		isEmpty() const		{ return hrg.isEmpty(); }
    bool		getIntersection(const CubeSampling&,
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


inline CubeSampling::Dir direction( CubeSampling::Dir slctype, int dimnr )
{
    if ( dimnr == 0 )
	return slctype;
    else if ( dimnr == 1 )
	return slctype == CubeSampling::Inl ? CubeSampling::Crl
	    				    : CubeSampling::Inl;
    else
	return slctype == CubeSampling::Z ? CubeSampling::Crl : CubeSampling::Z;
}


inline int dimension( CubeSampling::Dir slctype, CubeSampling::Dir direction )
{
    if ( slctype == direction )
	return 0;

    else if ( direction == CubeSampling::Z )
	return 2;
    else if ( direction == CubeSampling::Inl )
	return 1;

    return slctype == CubeSampling::Z ? 2 : 1;
}


#endif
