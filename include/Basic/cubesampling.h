#ifndef cubesampling_h
#define cubesampling_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2002
 RCS:           $Id: cubesampling.h,v 1.32 2007-09-13 19:38:38 cvsnanne Exp $
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
			
    StepInterval<int>	inlRange() const;
    StepInterval<int>	crlRange() const;

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
    void		includeInl( int inl );
    void		includeCrl( int crl );
    bool		isDefined() const;
    void		limitTo(const HorSampling&);
    void		limitToWithUdf(const HorSampling&);
    			/*!< handles undef values +returns reference horsampling
			     nearest limit if horsamplings do not intersect */

    inline int		inlIdx( int inl ) const
			{ return (inl - start.inl) / step.inl; }
    inline int		crlIdx( int crl ) const
			{ return (crl - start.crl) / step.crl; }
    BinID		atIndex( int i0, int i1 ) const
			{ return BinID( start.inl + i0*step.inl,
					start.crl + i1*step.crl ); }
    int			nrInl() const;
    int			nrCrl() const;
    inline int		totalNr() const	{ return nrInl() * nrCrl(); }
    inline bool		isEmpty() const { return nrInl() < 1 || nrCrl() < 1; }

    void		init(bool settoSI=true);
    			//!< Sets to survey values or mUdf(int) (but step 1)
    void		set2DDef();
    			//!< Sets ranges to 0-maxint
    void		normalise();
    			//!< Makes sure start<stop and steps are non-zero

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

    BinID		start;
    BinID		stop;
    BinID		step;

};


//\brief Iterates over all BinID positions
class HorSamplingIterator
{
public:
    			HorSamplingIterator(const HorSampling&);

    bool		next(BinID&);

protected:
    HorSampling		hrg_;
    bool		firstpos_;
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
    bool		isFlat() const; //!< is one of directions size 1?

    void		init(bool settoSI=true);
    			//!< Sets hrg.init and zrg to survey values or zeros
    inline void		setEmpty()		{ init(false); }
    void		set2DDef();
    			//!< Sets to survey zrange and hrg.set2DDef
    void		normalise();
    			//!< Makes sure start<stop and steps are non-zero

    HorSampling		hrg;
    StepInterval<float>	zrg;

    inline int		inlIdx( int inl ) const	{ return hrg.inlIdx(inl); }
    inline int		crlIdx( int crl ) const	{ return hrg.crlIdx(crl); }
    inline int		zIdx( float z ) const	{ return zrg.getIndex(z); }
    inline int		nrInl() const		{ return hrg.nrInl(); }
    inline int		nrCrl() const		{ return hrg.nrCrl(); }
    inline int		nrZ() const		{ return zrg.nrSteps() + 1; }
    od_int64		totalNr() const;
    inline int		size( Dir d ) const	{ return d == Inl ? nrInl()
    						      : (d == Crl ? nrCrl()
							          : nrZ()); }
    inline float	zAtIndex( int idx ) const
						{ return zrg.atIndex(idx); }
    inline bool		isEmpty() const		{ return hrg.isEmpty(); }
    bool		isDefined() const;
    bool		includes( const CubeSampling& ) const;
    bool		getIntersection(const CubeSampling&,
	    				CubeSampling&) const;
    			//!< Returns false if intersection is empty
    void		include(const CubeSampling&);
    void		limitTo(const CubeSampling&);
    void		limitToWithUdf(const CubeSampling&);
    			/*!< handles undef values + returns reference cube 
			     nearest limit if the 2 cubes do not intersect */

    void		snapToSurvey();
    			/*!< Checks if it is on valid bids and sample positions.
			     If not, it will expand until it is */

    bool		operator==( const CubeSampling& cs ) const;
    bool		operator!=( const CubeSampling& cs ) const
			{ return !(cs==*this); }

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;
    static void		removeInfo(IOPar&);
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
