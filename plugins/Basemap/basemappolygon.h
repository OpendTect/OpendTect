#ifndef basemappolygon_h
#define basemappolygon_h

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
#include "multiid.h"

namespace Pick { class Set; }

namespace Basemap
{

mExpClass(Basemap) PolygonObject : public BaseMapObject
{
public:
			PolygonObject(const MultiID&);
			~PolygonObject();

    const MultiID&	getMultiID() const	{ return polygonmid_; }
    void		setMultiID(const MultiID&);
    const char*		getType() const		{ return "Polygon"; }
    void		updateGeometry();

    int			nrShapes() const;
    const char*		getShapeName(int shapeidx) const;
    void		getPoints(int shapeidx,TypeSet<Coord>&) const;
    const MarkerStyle2D* getMarkerStyle(int shapeidx) const;
    void		setMarkerStyle(int shapeidx,const MarkerStyle2D&);
    const LineStyle*	getLineStyle(int shapeidx) const;
    void		setLineStyle(int shapeidx,const LineStyle&);

private:
    MultiID		polygonmid_;
    MarkerStyle2D	ms_;
    LineStyle		ls_;

    Pick::Set&		ps_;
};

} // namespace Basemap

#endif
