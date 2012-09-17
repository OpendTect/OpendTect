#ifndef cubicbeziercurve_h
#define cubicbeziercurve_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl
Date:          2005
Contents:      Ranges
RCS:           $Id: cubicbeziercurve.h,v 1.8 2009/07/22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "parametriccurve.h"
#include "sets.h"

namespace Geometry
{

mClass CubicBezierCurve : public ParametricCurve
{
public:
			CubicBezierCurve( const Coord3&, const Coord3&,
					  int firstparam=0, int step=1 );
    CubicBezierCurve*	clone() const;
    IntervalND<float>	boundingBox(bool) const;

    Coord3 		computePosition( float ) const;
    Coord3 		computeTangent( float ) const;

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

    Coord3	getTangent( GeomPosID, bool computeifudf ) const;
    bool	setTangent( GeomPosID, const Coord3& );
    bool	unsetTangent( GeomPosID );
    bool	isTangentDefined( GeomPosID ) const;
    float	directionInfluence() const;
    void	setTangentInfluence(float);

    bool	isCircular() const;
    bool	setCircular(bool yn);

protected:
    int		getIndex( GeomPosID param ) const
    		{ return (param-firstparam)/paramstep; }

    Coord3	computeTangent( GeomPosID ) const;

    int			firstparam;
    int			paramstep;
    TypeSet<Coord3>	positions;
    TypeSet<Coord3>	directions;
    float		directioninfluence;

    bool		iscircular;
};

/*! Implementation of deCastaleau's algoritm. For more info, refer to
 *  "The NURBS book", figure 1.17. */

inline Coord3 cubicDeCasteljau( const Coord3* p, char i0, char di, float u )
{
    p += i0;
    if ( mIsZero(u,1e-3) ) return *p;
    else if ( mIsEqual(u,1,1e-3) ) return p[3*di];

    const float one_minus_u = 1-u;
    Coord3 interpolpos1 = 	p[di]*one_minus_u+p[di*2]	* u;

    const Coord3 interpolpos0 = (*p*one_minus_u+p[di]*u)	* one_minus_u +
				interpolpos1			* u;

    interpolpos1 = 		interpolpos1 			* one_minus_u +
				(p[di*2]*one_minus_u+p[3*di]*u) * u;

    return interpolpos0*one_minus_u+interpolpos1*u;
}


inline Coord3 cubicDeCasteljauTangent( const Coord3* p, char i0, char di,
				       float u)
{
    p += i0;
    if ( mIsZero(u,1e-3) ) return p[di]-*p;
    else if ( mIsEqual(u,1,1e-3) ) return p[3*di]-p[2*di];

    const float one_minus_u = 1-u;
    Coord3 interpolpos1 =	p[di]*one_minus_u+p[2*di]	* u;

    const Coord3 interpolpos0 = (*p*one_minus_u+p[di]*u)	* one_minus_u +
				interpolpos1			* u;

    interpolpos1 =		interpolpos1			* one_minus_u +
				(p[2*di]*one_minus_u+p[3*di]*u) * u;

    return interpolpos1-interpolpos0;
}

};

#endif
