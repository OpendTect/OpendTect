#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "giswriter.h"

class LatLong;
namespace OD
{

namespace JSON
{

class GeoJsonTree;

/*!
\brief GeoJSON Writer.
*/

mExpClass(General) GeoJSONWriter : public GIS::Writer
{
mODTextTranslationClass(GeoJSONWriter);
public:
    mDefaultFactoryInstantiation( GIS::Writer, GeoJSONWriter, "GeoJSON",
				  ::toUiString("GeoJSON") );

			~GeoJSONWriter();

private:
			GeoJSONWriter();
			mOD_DisableCopy(GeoJSONWriter);

    GIS::Writer&	setInputCoordSys(const Coords::CoordSystem*) override;
    GIS::Writer&	setStream(const char*,bool useexisting) override;
    bool		open(const char* fnm,bool useexisting) override;
    bool		close() override;

    bool		isOK() const override;
    BufferString	getExtension() const override
			{ return BufferString("geojson"); }
    void		getDefaultProperties(const GIS::FeatureType&,
					     GIS::Property&) const override;

			// Point
    bool		writePoint(const Coord&,
				   const char* nm=nullptr) override;
    bool		writePoint(const Coord3&,
				   const char* nm=nullptr) override;
    bool		writePoint(const LatLong&,
				   const char* nm=nullptr,
				   double z=0.) override;

			// LineString
    bool		writeLine(const TypeSet<Coord>&,
				  const char* nm=nullptr) override;
    bool		writeLine(const TypeSet<Coord3>&,
				  const char* nm=nullptr) override;
    bool		writeLine(const Pick::Set&) override;

			// Polygon
    bool		writePolygon(const TypeSet<Coord>&,
				     const char* nm=nullptr) override;
    bool		writePolygon(const TypeSet<Coord3>&,
				     const char* nm=nullptr) override;
    bool		writePolygon(const Pick::Set&) override;

			// MultiPoint
    bool		writePoints(const TypeSet<Coord>&,
				    const char* nm=nullptr) override;
    bool		writePoints(const TypeSet<Coord3>&,
				    const char* nm=nullptr) override;
    bool		writePoints(const Pick::Set&) override;

			// MultiLineString
    bool		writeLines(const Pick::Set&) override;

			// MultiPolygon
    bool		writePolygons(const Pick::Set&) override;

    GeoJsonTree*	geojsontree_;
};

} // namespace JSON

} // namespace OD
