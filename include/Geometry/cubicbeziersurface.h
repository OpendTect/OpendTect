#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "parametricsurface.h"

class Line3;
template <class T> class Array2D;

namespace Geometry
{

mExpClass(Geometry) CubicBezierSurfacePatch
{ mODTextTranslationClass(CubicBezierSurfacePatch);
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
			~CubicBezierSurfacePatch();

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



mExpClass(Geometry) CubicBezierSurface : public ParametricSurface
{ mODTextTranslationClass(CubicBezierSurface);
public:
			CubicBezierSurface( const RowCol& step=RowCol(1,1));
			CubicBezierSurface( const CubicBezierSurface& );
			~CubicBezierSurface();

    CubicBezierSurface* clone() const override;
    bool		isEmpty() const override { return !positions; }

    IntervalND<float>	boundingBox(bool approx) const override;

    Coord3	computePosition(const Coord&) const override;
    Coord3	computeNormal(const Coord&) const override;

    bool	intersectWithLine(const Line3&,Coord&) const;

    Coord3	getBezierVertex(const RowCol& knot,const RowCol& relpos) const;

    bool	insertRow(int row,int nrnew=1) override;
    bool	insertCol(int col,int nrnew=1) override;
    bool	removeRow(int row,int stoprow=-1) override;
		//!< stoprow will be ignored

    bool	removeCol(int col,int stopcol=-1) override;
		//!< stopcol will be ignored

    Coord3	getKnot( const RowCol& rc ) const override
		{ return getKnot(rc,false); }
    Coord3	getKnot(const RowCol&,bool estimateifundef) const override;

    Coord3	getRowDirection(const RowCol&,bool computeifudf) const;
    Coord3	getColDirection(const RowCol&,bool computeifudf) const;
    float	directionInfluence() const;
    void	setDirectionInfluence(float);

    const CubicBezierSurfacePatch*	getPatch(const RowCol&) const;
    ParametricCurve*	createRowCurve( float row,
			    const Interval<int>* colrange=0 ) const override;
    ParametricCurve*	createColCurve(float col,
			    const Interval<int>* rowrange=0 ) const override;

protected:

    bool	checkSelfIntersection( const RowCol& ) const override;

    IntervalND<float>	boundingBox(const RowCol&, bool ownvertices ) const;

    Coord3	computeRowDirection(const RowCol&) const;
    Coord3	computeColDirection(const RowCol&) const;

    void	_setKnot( int idx, const Coord3& ) override;

    int		nrRows() const override;
    int		nrCols() const override;

    Array2D<Coord3>*	rowdirections;
    Array2D<Coord3>*	coldirections;
    Array2D<Coord3>*	positions;

    float		directioninfluence;
};

} // namespace Geometry
