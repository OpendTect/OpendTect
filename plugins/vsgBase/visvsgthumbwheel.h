#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgdata.h"

#include "color.h"
#include "notify.h"

#include <vsg/core/ref_ptr.h>

namespace vsgGeo { class ThumbWheel; }

namespace dGBVis
{

/*!\brief Counterpart of visBase::ThumbWheel. */

mExpClass(vsgBase) ThumbWheel : public DataObject
{
public:
    static RefMan<ThumbWheel>	create();
				mCreateDGBVisObj(ThumbWheel);

    void			setPosition(bool horizontal,float center_x,
					    float center_y,float len,
					    float width,float zval=0.f);

    void			setAnnotationColor(const OD::Color&);

    void			enableFadeInOut(bool);
    bool			isFadeInOutEnabled() const;
    void			setGrowOnHover(bool);
    bool			isGrowOnHover() const;
    void			setGrowDirection(int);
    void			setGlassLook(bool);
    bool			isGlassLook() const;

    float			getAngle() const;
    void			setAngle(float angle,float rotationtime=0.f);
    CNotifier<ThumbWheel,float>	rotation;

    vsgGeo::ThumbWheel*		vsgWheel()	{ return wheel_.get(); }

protected:
				~ThumbWheel();

    vsg::ref_ptr<vsgGeo::ThumbWheel>	wheel_;
};

} // namespace dGBVis
