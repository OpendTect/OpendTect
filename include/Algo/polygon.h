#ifndef polygon_h
#define polygon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	J.C. Glas
 Date:		Dec 2006
 RCS:		$Id: polygon.h,v 1.1 2007-08-23 08:05:07 cvsjaap Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "sets.h"

/*!\brief Closed sequence of connected 2-D coordinates */

class Polygon
{
public:
			Polygon()					{}
			Polygon(const TypeSet<Coord>& plg) : poly_(plg)	{}

    int 		size() const		{ return poly_.size(); }
    bool 		validIdx(int idx) const	{ return poly_.validIdx(idx); }
    
    void		add(const Coord& vtx)	{ poly_+=vtx; }
    void		remove(int idx) 	{ poly_.remove(idx); }
    void		insert(int idx,const Coord& vtx);

    const Coord&	getVertex(int idx) const;
    const Coord&	nextVertex(int idx) const; 

    bool		isInside(const Coord&,bool inclborder=true,
	    			 double=mDefEps) const;

protected:

    TypeSet<Coord>	poly_;

};


#endif
