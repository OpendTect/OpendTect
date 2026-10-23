#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgobject.h"

#include "color.h"
#include "position.h"
#include "typeset.h"

#include <vsg/maths/vec3.h>

namespace vsg { class Camera; }
namespace vsgGeo { class MarkerSet; }

namespace dGBVis
{

/*!\brief Screen-sized markers, the visBase::MarkerSet used for picks.

  Every position is one child of a single set. The size is the visBase
  screen size in pixels; the drawn diameter is 2.5 times that, as in
  visBase::MarkerSet. The shape is a sphere.
*/

mExpClass(vsgBase) MarkerSet : public VisualObjectImpl
{
public:
    static RefMan<MarkerSet>	create();

    void			addPos(const Coord3&);
    void			setPos(int idx,const Coord3&);
    void			removePos(int idx);
    void			setScreenSize(float pixels);
    void			setMarkersSingleColor(const OD::Color&);
    void			setCamera(vsg::Camera*);

    void			setDisplayTransformation(
					const mDGBVisTrans*) override;
    const mDGBVisTrans*		getDisplayTransformation() const override
				{ return display_.ptr(); }

private:
				MarkerSet();
				~MarkerSet();

    vsg::dvec3			toDisplay(const Coord3&) const;

    vsg::ref_ptr<vsgGeo::MarkerSet>	markers_;
    TypeSet<Coord3>			positions_;
    ConstRefMan<mDGBVisTrans>		display_;
};

} // namespace dGBVis
