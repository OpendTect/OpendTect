#ifndef posidxpair2coord_h
#define posidxpair2coord_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "posidxpair.h"
#include "coord.h"


namespace Pos
{


/*!\brief Encapsulates linear transform from (i,j) index to (x,y) coordinates. */

mExpClass(Basic) IdxPair2Coord
{
public:

    typedef IdxPair::IdxType IdxType;

			IdxPair2Coord()		{}

    bool		isValid() const		{ return xtr.valid(ytr); }
    Coord		firstDir() const	{ return Coord(xtr.b,ytr.b); }
    Coord		secondDir() const	{ return Coord(xtr.c,ytr.c); }

    Coord		transform(const IdxPair&) const;
    IdxPair		transformBack(const Coord&) const;
			    /*!< Transforms Coord to nearest IdxPair.  */
    IdxPair		transformBack(const Coord&,const IdxPair& start,
	    				const IdxPairStep&) const;
			    /*!< Transforms Coord to IdxPair, with snap.  */

    Coord		transformBackNoSnap(const Coord&) const;
			    /*!< transforms back, but doesn't go to int domain.
				 first == X, second == Y. */
    Coord		transform(const Coord& rc) const;
			    /*!< transforms a non-integer IdxPair
				 first == X, second == Y. */

    bool		set3Pts(const Coord& c0,const Coord& c1,const Coord& c2,
	    			const IdxPair& rc0,const IdxPair& rc1,
				od_int32 col2 );
			    /*!<Sets up the transform using three points.
				\note that the third point is assumed to be on
				the same row as the first point. */

    struct DirTransform
    {
			DirTransform()	{ a = b = c = 0; }

	inline double	det( const DirTransform& bct ) const
			{ return b * bct.c - bct.b * c; }
	inline bool	valid( const DirTransform& bct ) const
			{ double d = det( bct ); return !mIsZero(d,mDefEps); }

	double		a, b, c;
    };

    void		setTransforms(	const DirTransform& x,
					const DirTransform& y )
			{ xtr = x; ytr = y; }
    const DirTransform&	getTransform( bool x ) const
			{ return x ? xtr : ytr; }

    // aliases

    inline Coord	rowDir() const	{ return firstDir(); }
    inline Coord	colDir() const	{ return secondDir(); }
    inline Coord	inlDir() const	{ return firstDir(); }
    inline Coord	crlDir() const	{ return secondDir(); }


protected:

    DirTransform	xtr;
    DirTransform	ytr;

};


} // namespace Pos

#endif

