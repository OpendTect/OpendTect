#ifndef cubicbeziercurve_h
#define cubicbeziercurve_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: cubicbeziercurve.h,v 1.1 2005-01-06 09:44:18 kristofer Exp $
________________________________________________________________________

-*/

#include "parametriccurve.h"
#include "sets.h"

namespace Geometry
{

class CubicBezierCurve : public ParametricCurve
{
public:
			CubicBezierCurve( const Coord3&, const Coord3&,
					  int firstparam=0, int step=1 );
    CubicBezierCurve*	clone() const;

    IntervalND<float>	boundingBox(bool) const;

    Coord3 		computePosition( float ) const;
    Coord3 		computeDirection( float ) const;

    StepInterval<int>	parameterRange() const;

    Coord3		getPosition( GeomPosID ) const;
    bool		setPosition( GeomPosID, const Coord3& );
    bool		unSetPosition( GeomPosID );
    			/*!<Sets position to undefined */
    bool		insertPosition( GeomPosID, const Coord3& );
    bool		removePosition( GeomPosID );
    			/*!<The opposite of insertPosition */
    bool		isDefined( GeomPosID ) const;

    Coord3		getDirection( GeomPosID, bool computeifudf ) const;
    bool		setDirection( GeomPosID, const Coord3& );
    bool		unSetDirection( GeomPosID );
    bool		isDirectionDefined( GeomPosID ) const;
    float		directionInfluence() const;
    void		setDirectionInfluence(float);

    bool		isCircular() const { return iscircular; }
    void		setCircular(bool yn);

protected:
    int			getIndex( GeomPosID param ) const
    			{ return (param-firstparam)/paramstep; }

    Coord3		computeDirection( GeomPosID ) const;

    int			firstparam;
    int			paramstep;
    TypeSet<Coord3>	positions;
    TypeSet<Coord3>	directions;
    float		directioninfluence;

    bool		iscircular;
};

};

#endif
