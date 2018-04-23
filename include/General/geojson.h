#pragma once

#include "generalmod.h"
#include "odjson.h"

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
    static const char*	sKeyCRS()			{ return "crs"; }

			// use when constructing from general JSON Object
    void		doGeoJSonCheck(uiRetVal&);

protected:

    BufferString	filename_;

};

} // namespace OD
