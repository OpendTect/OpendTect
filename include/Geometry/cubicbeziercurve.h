#ifndef cubicbeziercurve_h
#define cubicbeziercurve_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: cubicbeziercurve.h,v 1.3 2005-02-17 10:25:47 cvskris Exp $
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

    Coord3	getPosition( GeomPosID ) const;
    bool	setPosition( GeomPosID, const Coord3&);
    bool	unsetPosition( GeomPosID );
    		/*!<Sets position to undefined */
    bool	insertPosition( GeomPosID, const Coord3& );
    bool	removePosition( GeomPosID );
    		/*!<The opposite of insertPosition */
    bool	isDefined( GeomPosID ) const;

    Coord3	getBezierVertex( GeomPosID, bool before ) const;

    Coord3	getDirection( GeomPosID, bool computeifudf ) const;
    bool	setDirection( GeomPosID, const Coord3& );
    bool	unsetDirection( GeomPosID );
    bool	isDirectionDefined( GeomPosID ) const;
    float	directionInfluence() const;
    void	setDirectionInfluence(float);

    bool	isCircular() const;
    bool	setCircular(bool yn);

protected:
    int		getIndex( GeomPosID param ) const
    		{ return (param-firstparam)/paramstep; }

    Coord3	computeDirection( GeomPosID ) const;

    int			firstparam;
    int			paramstep;
    TypeSet<Coord3>	positions;
    TypeSet<Coord3>	directions;
    float		directioninfluence;

    bool		iscircular;
};


Coord3 cubicDeCasteljau( const Coord3&, const Coord3&,
		         const Coord3&, const Coord3&, float u );

};

#endif
