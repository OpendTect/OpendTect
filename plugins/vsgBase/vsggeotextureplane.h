#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include "vsggeolayeredtexture.h"

#include <vsg/core/Inherit.h>
#include <vsg/maths/vec2.h>
#include <vsg/maths/vec3.h>
#include <vsg/nodes/Group.h>
#include <vsg/utils/SharedObjects.h>

namespace vsgGeo
{

/*!\brief A rectangle that shows a LayeredTexture.

  The rectangle starts at origin and spans uaxis and vaxis. The texture
  coordinate at origin+a*uaxis+b*vaxis is tcorigin+a*tcu+b*tcv, by default the
  whole image with its first row along uaxis. A texture coordinate is the
  position in the image from 0 to 1.

  With blending the plane is drawn in the transparent bin, after the opaque
  objects and sorted back to front, as in OpenSceneGraph. Planes with the same
  shared objects share their pipeline.
*/

class VSGGEO_EXPORT TexturePlane : public vsg::Inherit<vsg::Group,TexturePlane>
{
public:

				TexturePlane(vsg::ref_ptr<LayeredTexture>,
					const vsg::vec3& origin,
					const vsg::vec3& uaxis,
					const vsg::vec3& vaxis,
					const vsg::vec2& tcorigin
						=vsg::vec2(0.f,0.f),
					const vsg::vec2& tcu
						=vsg::vec2(1.f,0.f),
					const vsg::vec2& tcv
						=vsg::vec2(0.f,1.f),
					vsg::ref_ptr<vsg::SharedObjects> shared
						=nullptr,
					bool blending=true);

protected:
				~TexturePlane();

private:

    vsg::ref_ptr<LayeredTexture>	texture_;
};

} // namespace vsgGeo
