#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          October 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include "visosg.h"
#include "visdata.h"

namespace osg { class Group; class MatrixTransform; }
namespace osgGeo { class ThumbWheel; }


namespace visBase
{

class ThumbWheelMessenger;


mExpClass(visBase) ThumbWheel : public DataObject
{
public:
    static ThumbWheel*		create()
				mCreateDataObj(ThumbWheel);

    void			setPosition(bool horizontal,
                                    float center_x, float center_y, float len,
                                    float width,float zval = 0);

    void			setAnnotationColor(const Color&);

    void			enableFadeInOut(bool);
    bool			isFadeInOutEnabled() const;

    float			getAngle() const;
    void			setAngle(float angle,float rotationtime=0.0);
				//!<angle in rad, rotation time in seconds
    CNotifier<ThumbWheel,float>	rotation;
				//!<passes rotation since last notification

protected:
				~ThumbWheel();

    osgGeo::ThumbWheel*		thumbwheel_;
    ThumbWheelMessenger*	messenger_;
};

} // namespace visBase

