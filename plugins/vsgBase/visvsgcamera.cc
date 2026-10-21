/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgcamera.h"

#include "visvsgdataman.h"

#include <vsg/app/ViewMatrix.h>
#include <vsg/state/ViewportState.h>

mCreateDGBVisFactoryEntry( dGBVis::Camera );

namespace dGBVis
{

Camera::Camera()
    : camera_(vsg::Camera::create(vsg::Perspective::create(30.,1.,0.1,1000.),
		vsg::LookAt::create(vsg::dvec3(0.,-5.,0.),vsg::dvec3(0.,0.,0.),
				    vsg::dvec3(0.,0.,1.)),
		vsg::ViewportState::create(VkExtent2D{1,1})))
{}


Camera::~Camera()
{}

} // namespace dGBVis
