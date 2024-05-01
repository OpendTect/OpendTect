#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "visobject.h"
#include "vispolygonoffset.h"
#include "vistransform.h"

namespace OD { class RGBImage; }
namespace Pick { class Location; }
namespace osgGeo { class LayeredTexture; class TexturePlaneNode; }

namespace visBase
{

/*!Displays an image that either is read from disk or in memory. */

mExpClass(visBase) ImageRect : public VisualObjectImpl
{
public:
    static RefMan<ImageRect> create();
			mCreateDataObj( ImageRect );

    void		setPick(const Pick::Location&);

    void		setCenterPos(const Coord3&);
    void		setCornerPos(const Coord3& tl,const Coord3& br);

    void		setRGBImage(const OD::RGBImage&);
    void		setDisplayTransformation(const mVisTrans*) override;

protected:
			    ~ImageRect();

    ConstRefMan<mVisTrans>	trans_;
    int				layerid_;
    osgGeo::LayeredTexture*	laytex_;
    osgGeo::TexturePlaneNode*	texplane_;
    RefMan<PolygonOffset>	polyoffset_;
};


} // namespace visBase
