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
#include "posinfo2d.h"

namespace Basemap
{

mExpClass(Basemap) Geom2DObject : public BaseMapObject
{
public:
			    Geom2DObject();
			    ~Geom2DObject();

    void		    setMultiID(const MultiID&);
    const MultiID&	    getMultiID() const	{ return mid_; }
    virtual const char*     getType() const		{ return "Geom2D"; }
    virtual float	    getTextRotation() const;
    virtual void	    updateGeometry();

    virtual int		    nrShapes() const;
    virtual const char*     getShapeName(int shapeidx) const;
    virtual void	    getPoints(int shapeidx,TypeSet<Coord>&) const;
    virtual const LineStyle*getLineStyle(int shapeidx) const { return &ls_; }
    virtual void	    setLineStyle(int shapeidx,const LineStyle&);

protected:
    LineStyle		ls_;
    MultiID		mid_;

    TypeSet<Coord>	bptcoords_;
};

} // namespace Basemap

#endif

