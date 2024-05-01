#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "color.h"

#include "viscamera.h"
#include "visdata.h"

class FontData;
namespace osgGeo{ class AxesNode; }

namespace visBase
{

mExpClass(visBase) Axes : public DataObject
{
public:
    static RefMan<Axes>		create();
				mCreateDataObj(Axes);

    void			setRadius(float);
    float			getRadius() const;
    void			setLength(float);
    float			getLength() const;
    void			setPosition(float,float);
    void			setSize(float rad, float len);
    void			setAnnotationColor(const OD::Color&);
    void			setAnnotationTextSize(int);
    void			setAnnotationFont(const FontData&);
    void			setPrimaryCamera(visBase::Camera*);

    void			setPixelDensity(float dpi) override;

protected:
				Axes( bool /*internal*/ )
				    : Axes()		{}
				~Axes();

    osgGeo::AxesNode*		axesnode_;
    RefMan<Camera>		primarycamera_;
    bool			ison_;
    float			pixeldensity_;
    int				annottextsize_	= 18;

public:
    mDeprecated("Use setPrimaryCamera")
    void			setMasterCamera(visBase::Camera*);

};

} // namespace visBase
