#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/app/Camera.h>
#include <vsg/app/ProjectionMatrix.h>
#include <vsg/core/observer_ptr.h>
#include <vsg/maths/box.h>
#include <vsg/threading/OperationQueue.h>

#include <atomic>

namespace vsgGeo
{

/*!\brief Moves a perspective camera's near and far planes with the eye.

  VSG records the distances stored on the projection. Call boundsChanged()
  when the scene extent changes; run() then refreshes the planes each frame
  from that box. Ratios match osg::CullVisitor::clampProjectionMatrix and
  the 0.002 near/far ratio used by the OSG scene camera.
*/

class VSGGEO_EXPORT NearFar : public vsg::Inherit<vsg::Operation,NearFar>
{
public:

    void		setCamera(vsg::ref_ptr<vsg::Camera>);
    void		setScene(vsg::ref_ptr<vsg::Node>);
    void		setOffscreen(vsg::ref_ptr<vsg::Perspective>);

    void		boundsChanged();

    void		run() override;

private:

    static void		apply(vsg::Perspective&,double znear,double zfar);

    std::atomic<bool>			boundsdirty_{ true };
    vsg::observer_ptr<vsg::Camera>	camera_;
    vsg::observer_ptr<vsg::Node>	scene_;
    vsg::observer_ptr<vsg::Perspective> offscreen_;
    vsg::dbox				bounds_;
    bool				hasbounds_ = false;
};

} // namespace vsgGeo
