#ifndef basemapwell_h
#define basemapwell_h

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

namespace Well { class Data; }

namespace Basemap
{

mExpClass(Basemap) WellObject : public BaseMapObject
{
public:
			WellObject(const MultiID&);
			~WellObject();

    const MultiID&	getMultiID() const	{ return wellmid_; }
    void		setMultiID(const MultiID&);
    const char*		getType() const		{ return "Well"; }
    void		updateGeometry();

    int			nrShapes() const;
    const char*		getShapeName(int shapeidx) const;
    void		getPoints(int shapeidx,TypeSet<Coord>&) const;
    const MarkerStyle2D* getMarkerStyle(int shapeidx) const;
    void		setMarkerStyle(int shapeidx,const MarkerStyle2D&);

private:
    MultiID		wellmid_;
    MarkerStyle2D	ms_;

    Well::Data&		wd_;
};

} // namespace Basemap

#endif
