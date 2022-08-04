#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Feb 2007
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"

namespace OD { class RGBImage; }
namespace Pick { class Location; }
namespace osgGeo { class LayeredTexture; class TexturePlaneNode; }

namespace visBase
{

class PolygonOffset;

/*!Displays an image that either is read from disk or in memory. */

mExpClass(visBase) ImageRect : public VisualObjectImpl
{
public:
    static ImageRect*	create()
			mCreateDataObj( ImageRect );

    void		setPick(const Pick::Location&);

    void		setCenterPos(const Coord3&);
    void		setCornerPos(const Coord3& tl,const Coord3& br);

    void		setRGBImage(const OD::RGBImage&);
    void		setDisplayTransformation(const mVisTrans*) override;

protected:
			    ~ImageRect();

    const mVisTrans*		trans_;
    int				layerid_;
    osgGeo::LayeredTexture*	laytex_;
    osgGeo::TexturePlaneNode*	texplane_;
    visBase::PolygonOffset*	polyoffset_;
};


}; // namespace visBase

