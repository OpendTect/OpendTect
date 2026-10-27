#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/core/Array.h>
#include <vsg/io/Options.h>
#include <vsg/nodes/Node.h>

namespace vsgGeo
{

/*!\brief Unlit triangles drawn from both sides, with a color per vertex. With
  blending, the alpha of the colors is used.
  \returns null when there are no triangles or the arrays do not match. */

VSGGEO_EXPORT vsg::ref_ptr<vsg::Node> createTriangleList(
			const vsg::ref_ptr<vsg::vec3Array>& vertices,
			const vsg::ref_ptr<vsg::vec4Array>& colors,
			const vsg::ref_ptr<const vsg::Options>& options={},
			bool blending=false );

} // namespace vsgGeo
