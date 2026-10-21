#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgdata.h"

#include <vsg/app/Camera.h>

namespace dGBVis
{

/*!\brief Counterpart of visBase::Camera: keeps the vsg::Camera of a view. */

mExpClass(vsgBase) Camera : public DataObject
{
public:
    static RefMan<Camera>	create();
				mCreateDGBVisObj(Camera);

    vsg::Camera*		vsgCamera() const
				{ return camera_.get(); }

private:
				~Camera();

    vsg::ref_ptr<vsg::Camera>	camera_;
};

} // namespace dGBVis
