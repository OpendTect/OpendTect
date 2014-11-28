#ifndef basemapoutline_h
#define basemapoutline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basemapmod.h"
#include "basemap.h"
#include "draw.h"
#include "multiid.h"
#include "posinfo2d.h"

//namespace Seis { class Data; }

namespace Basemap
{

mExpClass(Basemap) OutlineObject : public BaseMapObject
{
public:
			OutlineObject(const MultiID&);
			~OutlineObject();

    const MultiID&	getMultiID() const	{ return seismid_; }
    void		setMultiID(const MultiID&);
    const char*		getType() const     { return "Outline"; }
    void		updateGeometry();

    int			nrShapes() const;
    const char*		getShapeName(int shapeidx) const;
    void		getPoints(int shapeidx, TypeSet<Coord>&) const;
    const LineStyle*	getLineStyle(int shapeidx) const
			{ return &ls_;}

protected:
    MultiID		seismid_;
    LineStyle		ls_;
};

} // namespace Basemap

#endif
