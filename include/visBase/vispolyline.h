#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visshape.h"
#include "position.h"
#include "draw.h"


namespace osgGeo {
    class PolyLineNode;
}

namespace visBase
{

class DrawStyle;

/*!\brief


*/

mExpClass(visBase) PolyLine : public VertexShape
{
public:
    static PolyLine*	create()
			mCreateDataObj(PolyLine);

    int		size() const;
    void		addPoint( const Coord3& pos );
    Coord3		getPoint( int ) const;
    void		setPoint( int, const Coord3& );
    void		removePoint( int );
    void		removeAllPoints();
    void		setLineStyle(const OD::LineStyle&) override;
    const OD::LineStyle& lineStyle() const;

    void		setDisplayTransformation(const mVisTrans*) override;
			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates,
			     you will have to setTransformation again.  */

protected:
					~PolyLine();
    DrawStyle*				drawstyle_;
    Geometry::RangePrimitiveSet*	coordrange_;
};



mExpClass(visBase) PolyLine3D : public VertexShape
{
public:
    static PolyLine3D*	create()
			mCreateDataObj(PolyLine3D);

    void		setLineStyle(const OD::LineStyle&) override;
    const OD::LineStyle& lineStyle() const;
    void		setResolution(int);
    int			getResolution() const;
    void		addPrimitiveSetToScene(osg::PrimitiveSet*) override;
    void		removePrimitiveSetFromScene(
					const osg::PrimitiveSet*) override;
    void		touchPrimitiveSet(int) override;
    void		setCoordinates(Coordinates*) override;
    void		setDisplayTransformation(const mVisTrans*) override;
			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates,
			     you will have to setTransformation again.  */

    void		setPixelDensity(float) override;
    float		getPixelDensity() const override
			{ return pixeldensity_; }

protected:
    void			updateRadius();
    osgGeo::PolyLineNode*	osgpoly_;
    OD::LineStyle		lst_;
    float			pixeldensity_;
};

} // namespace visBase
