#ifndef cubicbeziersurface_h
#define cubicbeziersurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl
Date:          December 2004
RCS:           $Id: cubicbeziersurface.h,v 1.10 2009-05-19 16:19:58 cvskris Exp $
________________________________________________________________________

-*/

#include "parametricsurface.h"

template <class T> class TypeSet;
class Line3;

namespace Geometry
{

mClass CubicBezierSurfacePatch
{
public:
			CubicBezierSurfacePatch(
					const Coord3& p00, const Coord3& p01,
					const Coord3& p02, const Coord3& p03,
					const Coord3& p10, const Coord3& p11,
					const Coord3& p12, const Coord3& p13,
					const Coord3& p20, const Coord3& p21,
					const Coord3& p22, const Coord3& p23,
					const Coord3& p30, const Coord3& p31,
					const Coord3& p32, const Coord3& p33 );

    CubicBezierSurfacePatch*		clone() const;

    Coord3		computePos(float u,float v) const; 
    Coord3		computeNormal(float u, float v) const;
    Coord3		computeUTangent(float u, float v) const;
    Coord3		computeVTangent(float u, float v) const;

    bool		intersectWithLine( const Line3& line,
	    				float& u, float& v, float eps ) const;

    IntervalND<float>	computeBoundingBox() const;

    Coord3		pos[16];
    static int		nrPos() { return 16; }
};



mClass CubicBezierSurface : public ParametricSurface
{
public:
    			CubicBezierSurface( const RCol& step=RowCol(1,1));
    			CubicBezierSurface( const CubicBezierSurface& );
			~CubicBezierSurface();

    CubicBezierSurface*	clone() const;
    bool		isEmpty() const { return !positions; }

    IntervalND<float>	boundingBox(bool approx) const;

    Coord3	computePosition(const Coord&) const;
    Coord3	computeNormal(const Coord&) const;

    bool	intersectWithLine(const Line3&, Coord& ) const;

    Coord3	getBezierVertex( const RCol& knot, const RCol& relpos ) const;

    bool	insertRow(int row,int nrnew=1);
    bool	insertCol(int col,int nrnew=1);
    bool	removeRow(int row);
    bool	removeCol(int col);
    Coord3	getKnot( const RCol&, bool estimateifundef=false ) const;

    Coord3	getRowDirection(const RCol&, bool computeifudf ) const;
    Coord3	getColDirection(const RCol&, bool computeifudf ) const;
    float	directionInfluence() const;
    void	setDirectionInfluence(float);

    const CubicBezierSurfacePatch*	getPatch(const RCol&) const;
    ParametricCurve*	createRowCurve( float row,
					const Interval<int>* colrange=0 ) const;
    ParametricCurve*	createColCurve( float col,
					const Interval<int>* rowrange=0 ) const;

protected:

    bool	checkSelfIntersection( const RCol& ) const;

    IntervalND<float>	boundingBox(const RCol&, bool ownvertices ) const;

    Coord3	computeRowDirection(const RCol&) const;
    Coord3	computeColDirection(const RCol&) const;

    void	_setKnot( int idx, const Coord3& );

    int		nrRows() const;
    int		nrCols() const;

    Array2D<Coord3>*	rowdirections;
    Array2D<Coord3>*	coldirections;
    Array2D<Coord3>*	positions;

    float		directioninfluence;
};

};

#endif
