#ifndef posgeom_h
#define posgeom_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Jan 2003
 RCS:           $Id: posgeom.h,v 1.2 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________

-*/

#include <geometry.h>
#include <position.h>

inline Point<int>	pt( const BinID& bid )
			{ return Point<int>(bid.inl,bid.crl); }
inline Point<double>	pt( const Coord& c )
			{ return Point<double>(c.x,c.y); }

inline BinID		bid( const Point<int>& p )
			{ return BinID(p.x(),p.y()); }
inline Coord		crd( const Point<double>& p )
			{ return Coord(p.x(),p.y()); }


#endif
