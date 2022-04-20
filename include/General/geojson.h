#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		March 2021
________________________________________________________________________
-*/

#include "generalmod.h"
#include "odjson.h"

#include "color.h"
#include "giswriter.h"
#include "googlexmlwriter.h"
#include "ptrman.h"
#include "survinfo.h"

namespace Coords { class CoordSystem; }
namespace Pick { class Set; }

namespace OD
{

mExpClass(General) GeoJsonTree : public JSON::Object
{ mODTextTranslationClass(GeoJsonTree)

public:

    typedef JSON::Object	Object;
    typedef JSON::Array		Array;

			GeoJsonTree();
			GeoJsonTree(const GeoJsonTree& oth);
			GeoJsonTree(const Object& obj);
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
				const TypeSet<Coord>& crdset,
				const BufferStringSet& nms,
				ConstRefMan<Coords::CoordSystem>,
				GISWriter::Property&);
    ValueSet*		createJSON(BufferString geomtyp,
				const TypeSet<Coord3>& crdset,
				const BufferStringSet& nms,
				ConstRefMan<Coords::CoordSystem>,
				GISWriter::Property&);
    ValueSet*		createJSON(BufferString geomtyp,
			    const RefObjectSet<const Pick::Set>&,
			    ConstRefMan<Coords::CoordSystem>,
			    const BufferString& iconnm=BufferString::empty());
    void		setProperties(const GISWriter::Property&);

protected:

    BufferString	filename_;
    bool		isfeatpoint_ = true;
    bool		isfeatpoly_  = false;
    bool		isfeatmulti_ = false;

    void		addCoord(const Coord3& coords, Array& poly);
    void		addCoord(const Coord& coords, Array& poly);
    bool		isAntiMeridianCrossed(const TypeSet<Coord3>&);
    Array*		createFeatArray(BufferString);
    Array*		createFeatCoordArray(Array* featarr, BufferString typ,
							GISWriter::Property);
    Object*		createCRSArray(Array* featarr);
    ConstRefMan<Coords::CoordSystem> coordsys_		= SI().getCoordSystem();
    Array*		featarr_			= nullptr;
    Array*		polyarr_			= nullptr;
    Object*		topobj_				= new Object();
    void		setCRS(ConstRefMan<Coords::CoordSystem>);

};

} // namespace OD
