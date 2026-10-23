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
#include "fontdata.h"

#include <vsg/core/ref_ptr.h>

namespace vsgGeo { class AxesNode; }

namespace dGBVis
{

class Camera;

/*!\brief Counterpart of visBase::Axes: three arrows and a ball that turn with
  the primary camera, for a HUD. */

mExpClass(vsgBase) Axes : public DataObject
{
public:
    static RefMan<Axes>		create();
				mCreateDGBVisObj(Axes);

    enum Style			{ Classic, Modern };
    void			setStyle(Style);
    Style			getStyle() const;
				/*!Free space around the tips, in pixels */
    float			getMargin() const;

    void			setRadius(float);
    float			getRadius() const;
    void			setLength(float);
    float			getLength() const;
    void			setPosition(float,float);
    void			setSize(float rad,float len);
    void			setAnnotationColor(const OD::Color&);
    void			setAnnotationTextSize(int);
    void			setAnnotationFont(const FontData&);
    void			setPrimaryCamera(Camera*);

    void			setPixelDensity(float) override;

    vsgGeo::AxesNode*		vsgAxes()	{ return axesnode_.get(); }

protected:
				~Axes();

    vsg::ref_ptr<vsgGeo::AxesNode> axesnode_;
    RefMan<Camera>		primarycamera_;
    FontData			fontdata_;
    float			pixeldensity_;
    int				annottextsize_	= 18;

private:

    void			prepare();
    void			fontLoadedCB(CallBacker*);
};

} // namespace dGBVis
