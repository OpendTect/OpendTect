#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/app/Trackball.h>

#include <functional>

namespace vsgGeo
{

/*!\brief vsg::Trackball that can ignore buttons.

  Counterpart of osgGeo::TrackballManipulator::enableDragging. Scroll
  and keys are unchanged. A button absent from the mask does not start
  a drag, and a drag already under way ends when its button is removed.
*/

class VSGGEO_EXPORT Trackball : public vsg::Inherit<vsg::Trackball,Trackball>
{
public:
				Trackball(vsg::ref_ptr<vsg::Camera>);

    void			enableDragging(vsg::ButtonMask);
    vsg::ButtonMask		dragMask() const	{ return dragmask_; }

    void			setLeftButtonPans(bool);
				/*!<While true, a left-button drag pans. The
				    drag mask still accepts or ignores the
				    button. */

    void			turn(double angle,bool horizontal);
    void			changeDistance(double factor);

    using MovementCB		= std::function<void(float,float,float,bool)>;
    void			setMovementCallback(const MovementCB&);

    void			rotate(double angle,const vsg::dvec3&) override;
    void			zoom(double ratio) override;
    void			apply(vsg::ButtonPressEvent&) override;
    void			apply(vsg::ButtonReleaseEvent&) override;
    void			apply(vsg::MoveEvent&) override;
    void			apply(vsg::ScrollWheelEvent&) override;

private:

    struct Pose
    {
	vsg::dvec3	eye_;
	vsg::dvec3	center_;
	vsg::dvec3	up_;
    };

    Pose			pose() const;
    void			notifyMovement(const Pose& before);
    bool			modeAllowed() const;
    void			endDrag();

    MovementCB			movementcb_;
    bool			wheelstep_ = false;
    bool			leftpans_ = false;
    vsg::ButtonMask		dragmask_ = vsg::ButtonMask(
				    int(vsg::BUTTON_MASK_1) |
				    int(vsg::BUTTON_MASK_2) |
				    int(vsg::BUTTON_MASK_3) );
};

} // namespace vsgGeo
