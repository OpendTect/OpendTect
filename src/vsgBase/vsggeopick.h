#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/app/Camera.h>
#include <vsg/core/Mask.h>
#include <vsg/maths/vec3.h>
#include <vsg/utils/Intersector.h>

namespace vsgGeo
{

/*!\brief One hit from vsgGeo::intersect. No OpendTect types. */

struct VSGGEO_EXPORT Pick
{
    bool				valid = false;
    vsg::Intersector::NodePath		nodePath;
    vsg::dvec3				worldPoint;
};


/*!\brief Ray through a window pixel, in the camera's world space. */

VSGGEO_EXPORT void windowRay(const vsg::Camera&,int x,int y,
			      vsg::dvec3& start,vsg::dvec3& end);

/*!\brief Front-most hit under a window pixel.

  Triangles use a line segment. Points and lines use a polytope of
  pixelRadius (default 1). A point or line in front of, or on, a
  triangle wins. traversalMask is the Switch child mask to visit.
*/

VSGGEO_EXPORT Pick intersect(const vsg::Camera&,vsg::Node&,
			      int x,int y,vsg::Mask traversalmask,
			      double pixelradius=1.);

} // namespace vsgGeo
