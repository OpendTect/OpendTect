#ifndef posgeom_h
#define posgeom_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2003
 RCS:           $Id: posgeom.h,v 1.6 2008-10-08 15:54:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "geometry.h"
#include "position.h"

inline Geom::Point2D<int>	pt( const BinID& bid )
				{ return Geom::Point2D<int>(bid.inl,bid.crl); }
inline Geom::Point2D<double>	pt( const Coord& c )
				{ return Geom::Point2D<double>(c.x,c.y); }

inline BinID			bid( const Geom::Point2D<int>& p )
				{ return BinID(p.x,p.y); }
inline Coord			crd( const Geom::Point2D<double>& p )
				{ return Coord(p.x,p.y); }


#endif
