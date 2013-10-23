#ifndef visaxes_h
#define visaxes_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          January 2013
 RCS:           $Id$
________________________________________________________________________

-*/


#include "visdata.h"


namespace osgGeo{ class AxesNode; }

namespace visBase
{

class Camera;
    
mExpClass(visBase) Axes : public DataObject
{
public:
    static Axes*		create()
    				mCreateDataObj(Axes);
    void			setRadius(float);
    float			getRadius() const;
    void			setLength(float);
    float			getLength() const;
    void			setPosition(float,float);
    void			setSize(float rad, float len);
    void			setMasterCamera(visBase::Camera*);

protected:
    				~Axes();

    osgGeo::AxesNode*		axesnode_;
    Camera*			mastercamera_;
    bool			ison_;
};

} // namespace visBase
#endif
