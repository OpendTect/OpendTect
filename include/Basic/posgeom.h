#ifndef posgeom_h
#define posgeom_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Jan 2003
 RCS:           $Id: posgeom.h,v 1.3 2004-04-15 13:12:24 macman Exp $
________________________________________________________________________

-*/

#include <geometry.h>
#include <position.h>

inline Geom::Point<int>		pt( const BinID& bid )
				{ return Geom::Point<int>(bid.inl,bid.crl); }
inline Geom::Point<double>	pt( const Coord& c )
				{ return Geom::Point<double>(c.x,c.y); }

inline BinID		bid( const Geom::Point<int>& p )
			{ return BinID(p.x(),p.y()); }
inline Coord		crd( const Geom::Point<double>& p )
			{ return Coord(p.x(),p.y()); }


#endif
