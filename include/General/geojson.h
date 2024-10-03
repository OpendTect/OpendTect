#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "odjson.h"

class LatLong;
namespace Coords { class CoordSystem; }
namespace GIS { class Property; }
namespace Pick { class Set; }

namespace OD
{

namespace JSON
{

mExpClass(General) GeoJsonTree : public Object
{
mODTextTranslationClass(GeoJsonTree)
public:
			GeoJsonTree();
			~GeoJsonTree();

			// will do GeoJSon check
    uiRetVal		use(od_istream&);
    uiRetVal		use(const char* fnm);

    static const char*	sKeyName()			{ return "name"; }
    static const char*	sKeyType()			{ return "type"; }
    static const char*	sKeyFeatures()			{ return "features"; }
    static const char*	sKeyProperties()		{ return "properties"; }
    static const char*	sKeyGeometry()			{ return "geometry"; }
    static const char*	sKeyCoord()			{ return "coordinates";}
    static const char*	sKeyCRS()			{ return "crs"; }

			// use when constructing from general JSON Object
    void		doGeoJSonCheck(uiRetVal&);

    void		setInputCoordSys(const Coords::CoordSystem*);

    bool		addPoint(const Coord&,const GIS::Property&);
    bool		addPoint(const Coord3&,const GIS::Property&);
    bool		addPoint(const LatLong&,double z,const GIS::Property&);
    bool		addFeatures(const TypeSet<Coord>&,const GIS::Property&);
    bool		addFeatures(const TypeSet<Coord3>&,
				    const GIS::Property&);
    bool		addFeatures(const Pick::Set&,const GIS::Property&);

private:
			mOD_DisableCopy(GeoJsonTree);

    BufferString	filename_;

    void		createFeatArray();
    Array*		createFeatCoordArray(const GIS::Property&);

    static void		addLatLong(const LatLong&,double z,Array&);
    static void		addCoord(const Coord&,const Coords::CoordSystem*,
				 Array&);
    static void		addCoord(const Coord3&,const Coords::CoordSystem*,
				 Array&);

    ConstRefMan<Coords::CoordSystem> inpcrs_;
    ConstRefMan<Coords::CoordSystem> coordsys_;

    Array*		featarr_			= nullptr;

};

} // namespace JSON

} // namespace OD
