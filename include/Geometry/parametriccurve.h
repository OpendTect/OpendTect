#ifndef parametriccurve_h
#define parametriccurve_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
RCS:           $Id: parametriccurve.h,v 1.3 2005-03-18 11:21:27 cvskris Exp $
________________________________________________________________________

-*/

#include "geomelement.h"

namespace Geometry
{
class ParametricCurve : public Element
{
public:
    virtual Coord3 	computePosition( float ) const	= 0;
    virtual Coord3 	computeTangent( float ) const	= 0;

    virtual bool	findClosestPosition( float& p, const Coord3&,
	    				     float eps=1e-2 ) const;
    			/*!<Iterates over the curve to find the closest position
			    \param eps  the allowed error in parameter space
			    \param p    The initial position and the returned
					best position. If undef on start, the
					closest defined position will be used
					as start */

    virtual bool	isCircular() const { return false; }
    void		getPosIDs( TypeSet<GeomPosID>& ) const;
    virtual bool	insertPosition( const GeomPosID,const Coord3& )	= 0;
    virtual StepInterval<int>	parameterRange() const			= 0;
};

};

#endif
