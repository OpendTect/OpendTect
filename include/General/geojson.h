#pragma once

#include "generalmod.h"
#include "odjson.h"
#include "color.h"
#include "pickset.h"
#include "googlexmlwriter.h"
#include "giswriter.h"


typedef TypeSet<Coord3> coord3dset;
typedef TypeSet<Coord> coord2dset;


namespace OD
{

mExpClass(General) GeoJsonTree : public JSON::Object
{ mODTextTranslationClass(GeoJsonTree)

public:

    typedef JSON::Object	Object;
    typedef JSON::Array		Array;

			GeoJsonTree()			{}
			GeoJsonTree( const GeoJsonTree& oth )
			    : Object( oth )
			    , filename_(oth.filename_)	{}
			GeoJsonTree( const Object& obj )
			    : Object( obj )		{}
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
					const coord2dset& crdset);
    ValueSet*		createJSON(BufferString geomtyp,
						const coord3dset& crdset);
    void		setProperties(const GISWriter::Property&);

protected:

    BufferString	filename_;
    bool		isfeatpoint_ = true;
    bool		isfeatpoly_  = false;
    bool		isfeatmulti_ = false;
    GISWriter::Property property_;

    void		addCoords(const TypeSet<Coord3>& coords, Array& poly);
    void		addCoords(const TypeSet<Coord>& coords, Array& poly);
    bool		isAntiMeridianCrossed(const coord3dset&);
    Array*		createFeatArray(BufferString);
    Array*		createFeatCoordArray(Array* featarr, BufferString typ);

};

} // namespace OD
