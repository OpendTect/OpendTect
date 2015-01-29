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
#include "coord.h"

namespace Well { class Reader; class Data; }

namespace Basemap
{

mExpClass(Basemap) WellObject : public BaseMapObject
{
public:
				WellObject(const MultiID&);
				~WellObject();

    const MultiID&		getKey() const		{ return key_; }
    void			setKey(const MultiID&);

    virtual bool		isOK() const		{ return rdr_; }
    virtual const char*		getType() const		{ return "Well"; }
    virtual void		updateGeometry();
    virtual int			nrShapes() const;
    virtual const char*		getShapeName(int) const;
    virtual void		getPoints(int,TypeSet<Coord>&) const;
    virtual const MarkerStyle2D* getMarkerStyle(int) const;
    virtual void		setMarkerStyle(int,const MarkerStyle2D&);

private:

    MultiID		key_;
    MarkerStyle2D	ms_;

    Coord		coord_;
    Well::Data&		data_;
    Well::Reader*	rdr_;

    void		setInvalid();

};

} // namespace Basemap

#endif
