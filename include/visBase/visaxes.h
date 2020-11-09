#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          January 2013
________________________________________________________________________

-*/


#include "visdata.h"

class FontData;
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
    void			setAnnotationColor(const Color&);
    void			setAnnotationText(int dim,const uiString&);
    void			setAnnotationTextSize(int);
    void			setAnnotationFont(const FontData&);

    void			setPrimaryCamera(visBase::Camera*);

    virtual void		setPixelDensity(float dpi);

protected:
				~Axes();

    osgGeo::AxesNode*		axesnode_;
    Camera*			primarycamera_;
    bool			ison_;
    float			pixeldensity_;
    int				annottextsize_;
};

} // namespace visBase
