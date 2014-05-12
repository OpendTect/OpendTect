#ifndef basemapgeom2d_h
#define basemapgeom2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basemapmod.h"
#include "basemap.h"
#include "draw.h"
#include "multiid.h"

namespace Basemap
{

mExpClass(Basemap) Geom2DObject : public BaseMapObject
{
public:
			Geom2DObject(const MultiID&);
			~Geom2DObject();

    const char*		getType() const		{ return "Geom2D"; }
    void		updateGeometry();
    int			nrShapes() const;
    const char*		getShapeName(int) const;
    void		getPoints(int,TypeSet<Coord>&) const;
    const LineStyle*	getLineStyle(int) const	{ return &ls_; }

protected:
    LineStyle		ls_;
    MultiID		mid_;
};

} // namespace Basemap

#endif

