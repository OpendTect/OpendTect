#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/core/Array.h>
#include <vsg/core/Inherit.h>
#include <vsg/maths/vec3.h>
#include <vsg/maths/vec4.h>
#include <vsg/nodes/Group.h>

namespace vsgGeo
{

/*!\brief The background of a view, in pixels with y up: the center color in the
  middle and the corner color in the corners. The color changes linearly with
  the distance from the middle, so that the lines of equal color are circles.
  Meant for a view of its own that comes before the scene, with the depth
  cleared after it. */

class VSGGEO_EXPORT GradientBackground
    : public vsg::Inherit<vsg::Group,GradientBackground>
{
public:

				GradientBackground();

    void			setColors(const vsg::vec4& center,
					  const vsg::vec4& corner);
    void			setSize(float width,float height);

private:

    void			update();
    void			setVertex(int idx,float x,float y);

    vsg::vec4			center_{ 0.f, 0.f, 0.f, 1.f };
    vsg::vec4			corner_{ 0.f, 0.f, 0.f, 1.f };
    float			width_ = 1.f;
    float			height_ = 1.f;
    float			halfdiagonal_ = 1.f;

    vsg::ref_ptr<vsg::vec3Array> vertices_;
    vsg::ref_ptr<vsg::vec4Array> colors_;
};

} // namespace vsgGeo
