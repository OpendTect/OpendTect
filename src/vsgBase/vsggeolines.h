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

/*!\brief Unlit line segments (VK_PRIMITIVE_TOPOLOGY_LINE_LIST), one colour.

  vertices holds two points per segment. color holds one colour for all
  segments; it is created with dataVariance DYNAMIC_DATA, so changing it
  and calling dirty() updates the colour without a new compile.
  \returns null when there are no segments.
*/

VSGGEO_EXPORT vsg::ref_ptr<vsg::Node> createLineList(
			const vsg::ref_ptr<vsg::vec3Array>& vertices,
			const vsg::ref_ptr<vsg::vec4Array>& color,
			const vsg::ref_ptr<const vsg::Options>& options={} );

VSGGEO_EXPORT vsg::ref_ptr<vsg::vec4Array> createLineColor(
			const vsg::vec4& );

} // namespace vsgGeo
