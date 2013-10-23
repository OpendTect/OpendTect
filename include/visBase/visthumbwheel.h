#ifndef visthumbwheel_h
#define visthumbwheel_h

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

class ThumbWheelMess;
    
    
mExpClass(visBase) ThumbWheel : public DataObject
{
public:
    static ThumbWheel*		create()
				mCreateDataObj(ThumbWheel);
    
    void			setPosition(bool horizontal,
					    float x, float y, float len,
					    float width,float zval = 0);

    float			getAngle() const;
    void			setAngle(float);
    CNotifier<ThumbWheel,float>	rotation;
				//!<passes rotation since last notification
    
protected:
    				~ThumbWheel();
    
    osgGeo::ThumbWheel*	    	thumbwheel_;
    ThumbWheelMess*		messenger_;
};

} // namespace visBase

#endif

