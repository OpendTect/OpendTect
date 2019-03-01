#pragma once

#include "generalmod.h"
#include "odjson.h"
#include "color.h"
#include "pickset.h"
#include "googlexmlwriter.h"
#include "giswriter.h"
#include "ptrman.h"
#include "survinfo.h"

namespace Coords { class CoordSystem; }
typedef TypeSet<Coord3> coord3dset;
typedef TypeSet<Coord> coord2dset;


namespace OD
{

mExpClass(General) GeoJsonTree : public JSON::Object
{ mODTextTranslationClass(GeoJsonTree)

public:

    typedef JSON::Object	Object;
    typedef JSON::Array		Array;

			GeoJsonTree()
			    : coordsys_(SI().getCoordSystem())
			    , featarr_(0)
			    , polyarr_(0)
			    , topobj_(new Object())
			{}
			GeoJsonTree( const GeoJsonTree& oth )
			    : Object(oth)
			    , filename_(oth.filename_)
			    , coordsys_( SI().getCoordSystem() )
			    , topobj_(new Object())
			    , featarr_(0)
			    , polyarr_(0)
			{}
			GeoJsonTree( const Object& obj )
			    : Object(obj)
			    , coordsys_(SI().getCoordSystem())
			    , topobj_(new Object())
			    , featarr_(0)
			    , polyarr_(0)
			{}
    virtual		~GeoJsonTree()			{}

			// will do GeoJSon check
    uiRetVal		use(od_istream&);
    uiRetVal		use(const char* fnm);

    BufferString	crsName() const;

    static const char*	sKeyName()			{ return "name"; }
    static const char*	sKeyType()			{ return "type"; }
    static const char*	sKeyFeatures()			{ return "features"; }
    static const char*	sKeyProperties()		{ return "properties"; }
    static const char*	sKeyGeometry()			{ return "geometry"; }
    static const char*	sKeyCoord()			{ return "coordinates";}
    static const char*	sKeyCRS()			{ return "crs"; }

			// use when constructing from general JSON Object
    void		doGeoJSonCheck(uiRetVal&);

    ValueSet*		createJSON(BufferString geomtyp,
			   const coord2dset& crdset, const BufferStringSet& nms,
			   ConstRefMan<Coords::CoordSystem>);
    ValueSet*		createJSON(BufferString geomtyp,
			   const coord3dset& crdset, const BufferStringSet& nms,
			   ConstRefMan<Coords::CoordSystem>);
    ValueSet*		createJSON(BufferString geomtyp,
			      const pickset&,ConstRefMan<Coords::CoordSystem>);
    void		setProperties(const GISWriter::Property&);

protected:

    BufferString	filename_;
    bool		isfeatpoint_ = true;
    bool		isfeatpoly_  = false;
    bool		isfeatmulti_ = false;
    GISWriter::Property property_;

    void		addCoord(const Coord3& coords, Array& poly);
    void		addCoord(const Coord& coords, Array& poly);
    bool		isAntiMeridianCrossed(const coord3dset&);
    Array*		createFeatArray(BufferString);
    Array*		createFeatCoordArray(Array* featarr, BufferString typ);
    Object*		createCRSArray(Array* featarr);
    ConstRefMan<Coords::CoordSystem>	    coordsys_;
    Array*		featarr_;
    Array*		polyarr_;
    Object*		topobj_;
    void		setCRS(ConstRefMan<Coords::CoordSystem>);

};

} // namespace OD
