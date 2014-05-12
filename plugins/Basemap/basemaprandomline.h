#ifndef basemaprandomline_h
#define basemaprandomline_h

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

namespace Geometry { class RandomLineSet; }

namespace Basemap
{

mExpClass(Basemap) RandomLineObject : public BaseMapObject
{
public:
			RandomLineObject(const MultiID&);
			~RandomLineObject();

    void		setMultiID(const MultiID&);
    const MultiID&	getMultiID() const	{ return mid_; }
    const char*		getType() const		{ return "RandomLine"; }
    void		updateGeometry();

    int			nrShapes() const;
    const char*		getShapeName(int shapeidx) const;
    void		getPoints(int shapeidx,TypeSet<Coord>&) const;

    const MarkerStyle2D* getMarkerStyle(int shapeidx) const;
    void		setMarkerStyle(int shapeidx,const MarkerStyle2D&);
    const LineStyle*	getLineStyle(int) const	{ return &ls_; }

private:
    MultiID		mid_;
    MarkerStyle2D	ms_;
    LineStyle		ls_;

    Geometry::RandomLineSet& rdlset_;
};

} // namespace Basemap

#endif
