#ifndef parametriccurve_h
#define parametriccurve_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        A.H. Bril
Date:          23-10-1996
RCS:           $Id$
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "geomelement.h"

class Plane3;

namespace Geometry
{

/*!
  \brief A curve that is defined by regularly sampled positions.  

  The curve's behaviour between the positions is determined by a function of a
  parameter u: (x(u), y(u), z(u)). The samplerange is determined by
  parameterRange().  The curve is guaranteed to be continious within its
  parameter range, and if isCircular() returns true, it connects between
  parameterRange().stop and parameterRange().start.
*/

mExpClass(Geometry) ParametricCurve : public Element
{
public:
    virtual Coord3 	computePosition( float ) const	= 0;
    			/*!<Computes the position between two samples */
    virtual Coord3 	computeTangent( float ) const	= 0;
    			/*!<\note the tangent is scaled so the components 
			    (x, y, z) forms the derivatives:
			    (dx/du, dy/du, dz/du ).
			*/

    virtual bool	findClosestPosition( float& p, const Coord3&,
	    				     float eps=1e-2 ) const;
    			/*!<Iterates over the curve to find the closest position
			    \param eps  the allowed error in parameter space
			    \param p    The initial position and the returned
					best position. If undef on start, the
					closest defined position will be used
					as start */
    virtual bool	findClosestIntersection( float& p, const Plane3&,
	    					 float eps=1e-2 ) const;
    			/*!<Iterates over the curve to find the closest
			    intersection.
			    \param eps  the allowed error in parameter space
			    \param p    The initial position and the returned
					intersection. If undef on start, the
					closest defined position will be used
					as start */

    virtual bool	isCircular() const { return false; }
    			/*!<If true, the curve is connected between 
			    parameterRange().stop and parameterRange().start. */
    void		getPosIDs( TypeSet<GeomPosID>&, bool=true ) const;
    			/*!<Returns a list with all defined positions. */
    Iterator*		createIterator() const;
    virtual bool	insertPosition(GeomPosID,const Coord3&)		= 0;
    virtual StepInterval<int>	parameterRange() const			= 0;
};

};

#endif

