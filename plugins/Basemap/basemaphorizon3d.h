#ifndef basemaphorizon3d_h
#define basemaphorizon3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
 ________________________________________________________________________

-*/


#include "basemapmod.h"
#include "basemap.h"
#include "draw.h"
#include "multiid.h"

namespace Basemap
{

mExpClass(Basemap) Horizon3DObject : public BaseMapObject
{
public:
				Horizon3DObject();
				~Horizon3DObject();

    virtual const char*		getType() const		{ return "horizon"; }
    virtual void		updateGeometry();

    virtual void		setImage(int idx,OD::RGBImage*);
				// image becomes mine
    virtual OD::RGBImage*	getImage(int shapeidx) const
					    { return rgbimage_; }

    virtual int			nrShapes() const;
    virtual const char*		getShapeName(int shapeidx) const;

protected:

private:
    OD::RGBImage*		rgbimage_;
};

} // namespace Basemap

#endif
