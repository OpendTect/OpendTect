#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"

namespace visBase
{
class TriangleStripSet;
class Transformation;
class VertexShape;

/*!This class displays the triangulated body based on a set of given points.
   For example

   visBase::RandomPos2Body* nb = visBase::RandomPos2Body::create();
   nb->ref();
   nb->setPoints( known_points );

   will do the display work after add nb to your scene!
*/

mExpClass(visBase) RandomPos2Body : public VisualObjectImpl
{
public:
    static RandomPos2Body*	create()
				mCreateDataObj(RandomPos2Body);

    bool			setPoints(const TypeSet<Coord3>& pts,
					  bool ispoly);
    bool			setPoints(const TypeSet<Coord3>& pts);
    const TypeSet<Coord3>&	getPoints() const { return picks_; }

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setRenderMode(RenderMode);

protected:
				~RandomPos2Body();

    TypeSet<Coord3>		picks_;
    VertexShape*		vtxshape_;
    const mVisTrans*		transformation_;
};


} // namespace visBase
