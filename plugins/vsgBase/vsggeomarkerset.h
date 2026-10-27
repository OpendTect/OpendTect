#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/core/observer_ptr.h>
#include <vsg/maths/vec3.h>
#include <vsg/maths/vec4.h>
#include <vsg/nodes/Group.h>
#include <vsg/nodes/MatrixTransform.h>

#include <vector>

namespace vsg { class Camera; class Intersector; }

namespace vsgGeo
{

inline const char* markerIndexKey()	{ return "vsgGeo.MarkerIndex"; }

/*!\brief One screen-sized sphere at each position.

  While the scene is recorded, and while it is intersected, each sphere
  is scaled so its diameter stays markerSize() pixels, also under a
  non-uniform parent. The scale is put back afterwards, so bounds stay
  at the position itself. The transform for a position carries that
  position's index.
*/

class VSGGEO_EXPORT MarkerSet
		: public vsg::Inherit<vsg::Group,MarkerSet>
{
public:
				MarkerSet();

    void			setCamera(vsg::Camera*);
    void			setMarkerSize(float pixels);
    void			setColor(const vsg::vec4&);

    vsg::ref_ptr<vsg::Node>	addPos(const vsg::dvec3&);
    void			setPos(int idx,const vsg::dvec3&);
    vsg::ref_ptr<vsg::Node>	removePos(int idx);

    void			accept(vsg::RecordTraversal&) const override;
    void			traverse(vsg::ConstVisitor&) const override;

private:

    vsg::dvec3			screenScale(const vsg::dmat4& modelview,
					    const vsg::dmat4& projection,
					    const vsg::dvec3& pos,
					    double viewheight) const;
    bool			applyScreenScale(const vsg::dmat4& modelview,
					    const vsg::dmat4& projection,
					    double viewheight) const;
    bool			applyScreenScale(vsg::RecordTraversal&) const;
    bool			applyPickScale(vsg::Intersector&) const;
    void			resetScale() const;
    void			ensureShape();

    struct Marker
    {
	vsg::dvec3				pos_;
	vsg::ref_ptr<vsg::MatrixTransform>	xform_;
    };

    vsg::observer_ptr<vsg::Camera>	camera_;
    float				markersize_ = 1.f;
    vsg::vec4				color_	    = { 1.f, 1.f, 1.f, 1.f };
    vsg::ref_ptr<vsg::Node>		shape_;
    mutable std::vector<Marker> markers_;
};

} // namespace vsgGeo
