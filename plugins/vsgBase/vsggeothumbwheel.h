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
#include <vsg/core/Visitor.h>
#include <vsg/maths/vec2.h>
#include <vsg/maths/vec4.h>
#include <vsg/nodes/Group.h>
#include <vsg/ui/PointerEvent.h>

#include <chrono>
#include <functional>
#include <vector>

namespace vsg { class RecordTraversal; }

namespace vsgGeo
{

/*!\brief Counterpart of osgGeo::ThumbWheel: a wheel in a HUD, in pixels with
  y up, that the user turns by dragging it with the left mouse button. */

class VSGGEO_EXPORT ThumbWheel : public vsg::Inherit<vsg::Group,ThumbWheel>
{
public:

    enum class Pointer		{ Press, Move, Release };
    enum class Style		{ Classic, Glass };

				ThumbWheel();

    void			setStyle(Style);
    Style			style() const		{ return style_; }

    void			setShape(bool horizontal,const vsg::vec2& min,
					 const vsg::vec2& max,float zval=0.f);
    bool			isHorizontal() const	{ return horizontal_; }
    const vsg::vec2&		getMin() const		{ return min_; }
    const vsg::vec2&		getMax() const		{ return max_; }

    void			setAngle(float angle,float rotationtime=0.f);
    float			getAngle() const	{ return angle_; }
    void			enableFadeInOut(bool);
    bool			isFadeInOutEnabled() const
				{ return fadeinout_; }
    void			setFadeTime(float seconds);
    float			getOpacity() const	{ return opacity_; }
    void			setGrowOnHover(bool);
    bool			isGrowOnHover() const	{ return growonhover_; }
				/*!-1, 0 or 1: grows at the min end, at both
				    ends, at the max end */
    void			setGrowDirection(int);
    float			getGrowth() const	{ return growth_; }
    void			setBorderColor(const vsg::vec4&);

    void			setRotationCallback(
					const std::function<void(float)>&);

    bool			handlePointer(Pointer,float x,float y);
    bool			isTracking() const	{ return tracking_; }

    void			accept(vsg::RecordTraversal&) const override;
    void			advance(std::chrono::steady_clock::time_point);

protected:
				~ThumbWheel();

private:

    enum class Proximity	{ Above, Nearby, None };

    Proximity			proximity(float x,float y) const;
    void			setProximity(Proximity);
    void			startFade();
    void			currentExtents(vsg::vec2& min,
					       vsg::vec2& max) const;
    void			extentsAt(float growth,vsg::vec2& min,
					  vsg::vec2& max) const;
    void			updateShape();
    void			updateColors();
    int				dim() const	{ return horizontal_ ? 0 : 1; }

    Style			style_ = Style::Classic;
    bool			horizontal_ = true;
    vsg::vec2			min_{ 200.f, 200.f };
    vsg::vec2			max_{ 400.f, 250.f };
    float			zval_ = 0.f;
    float			angle_ = 0.f;
    float			phase_ = 0.f;
    float			rotationleft_ = 0.f;
    bool			fadeinout_ = true;
    float			fadetime_ = 1.f;
    float			opacity_ = 0.f;
    bool			growonhover_ = false;
    int				growdirection_ = 0;
    float			growth_ = 0.f;
    bool			ingrowarea_ = false;
    float			startlength_ = 1.f;
    float			fadefrom_ = 0.f;
    bool			fading_ = false;
    bool			fadestartpending_ = false;
    std::chrono::steady_clock::time_point fadestart_;
    std::chrono::steady_clock::time_point lastadvance_;
    bool			hasadvanced_ = false;
    Proximity			proximity_ = Proximity::None;
    vsg::vec4			bordercolor_{ 1.f, 1.f, 1.f, 0.2f };
    bool			tracking_ = false;
    float			startpos_ = 0.f;
    float			startangle_ = 0.f;
    std::function<void(float)>	rotationcb_;

    vsg::ref_ptr<vsg::vec3Array> vertices_;
    vsg::ref_ptr<vsg::vec4Array> colors_;
};


/*!\brief Counterpart of osgGeo::ThumbWheelEventHandler. */

class VSGGEO_EXPORT ThumbWheelHandler : public vsg::Inherit<vsg::Visitor,
							    ThumbWheelHandler>
{
public:

    void			addThumbWheel(ThumbWheel*);
    void			removeThumbWheel(ThumbWheel*);
    void			setWindowHeight(uint32_t);

    void			apply(vsg::ButtonPressEvent&) override;
    void			apply(vsg::ButtonReleaseEvent&) override;
    void			apply(vsg::MoveEvent&) override;

private:

    bool			handle(ThumbWheel::Pointer,
				       vsg::PointerEvent&) const;

    std::vector<vsg::ref_ptr<ThumbWheel>>	wheels_;
    uint32_t			windowheight_ = 0;
};

} // namespace vsgGeo
