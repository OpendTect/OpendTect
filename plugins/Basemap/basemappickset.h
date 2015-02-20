#ifndef basemappickset_h
#define basemappickset_h

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

mExpClass(Basemap) PickSetObject : public BaseMapObject
{
public:
			PickSetObject(const MultiID&);
			~PickSetObject();

    const MultiID&	getMultiID() const	{ return picksetmid_; }
    void		setMultiID(const MultiID&);
    const char*		getType() const		{ return "PickSet"; }
    void		updateGeometry();

    int			nrShapes() const;
    const char*		getShapeName(int shapeidx) const;
    void		getPoints(int shapeidx,TypeSet<Coord>&) const;
    const MarkerStyle2D* getMarkerStyle(int shapeidx) const;
    void		setMarkerStyle(int shapeidx,const MarkerStyle2D&);

private:
    MultiID		picksetmid_;
    MarkerStyle2D	ms_;

    Pick::Set&		ps_;
};

} // namespace Basemap

#endif
