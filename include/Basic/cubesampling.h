#ifndef cubesampling_h
#define cubesampling_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2002
 RCS:           $Id: cubesampling.h,v 1.37 2010/08/26 11:39:30 cvsjaap Exp $
________________________________________________________________________

-*/

#include "horsampling.h"


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

mClass CubeSampling
{
public:

    			CubeSampling( bool settoSI=true )
			: hrg(settoSI)		{ init(settoSI); }

    enum Dir		{ Z, Inl, Crl };
    Dir			defaultDir() const;
    			//!< 'flattest' direction, i.e. direction with
    			//!< smallest size. If equal, prefer Inl then Crl then Z
    Coord3		defaultNormal() const;
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
    bool		includes(const CubeSampling&) const;
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
