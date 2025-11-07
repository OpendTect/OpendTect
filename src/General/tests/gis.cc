/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geojsonwriter.h"
#include "googlexmlwriter.h"

#include "file.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "pickset.h"
#include "statrand.h"
#include "testprog.h"

static const char* geojsonstr = OD::JSON::GeoJSONWriter::sFactoryKeyword();
static const char* kmlstr = ODGoogle::KMLWriter::sFactoryKeyword();
static const char* ED50str = "EPSG`23031";
static bool keep_ = false;

static const Coord wellcoords[] = { Coord(606554.00,6080126.00),
				    Coord(619101.00,6089491.00),
				    Coord(623255.98,6082586.87),
				    Coord(607903.00,6077213.00) };
static const char* wellnms[] = { "F02-1", "F03-2", "F03-4", "F06-1", nullptr };
static const Coord linecoords[] = { Coord(607039.82,6087745.34),
				    Coord(612471.36,6085796.37),
				    Coord(619649.01,6086697.23),
				    Coord(622847.34,6089487.61) };
static const Coord polygoncoords[] = { Coord(614683.61,6077330.08),
				       Coord(614484.66,6075498.85),
				       Coord(617150.38,6074973.13),
				       Coord(620346.90,6076037.82),
				       Coord(619923.19,6077776.62),
				       Coord(617432.96,6079182.57),
				       Coord(615925.31,6078540.21) };


static bool testInit( const Coords::CoordSystem* ed50crs )
{
    ConstRefMan<Coords::CoordSystem> wgs84llcrs =
			Coords::CoordSystem::getWGS84LLSystem();
    mRunStandardTest( wgs84llcrs && wgs84llcrs->isProjection(), "WGS84 CRS" );
    mRunStandardTest( ed50crs && ed50crs->isProjection(), "ED50 CRS" );

    FactoryBase& gisfact = GIS::Writer::factory();
    mRunStandardTestWithError( gisfact.hasName(geojsonstr),
			       "GeoJSON writer in factory", gisfact.errMsg() );
    mRunStandardTestWithError( gisfact.hasName(kmlstr),
			       "KML writer in factory", gisfact.errMsg() );

    return true;
}


static bool testGISWriter( const Coords::CoordSystem& ed50crs,
			   GIS::Writer& wrr )
{
    wrr.setInputCoordSys( &ed50crs );

    GIS::Property pointsdisps;
    wrr.getDefaultProperties( GIS::FeatureType::Point, pointsdisps );
    wrr.setProperties( pointsdisps );
    wrr.setDescription( "This is a well location" );
    const int wellssz = sizeof(wellcoords) / sizeof(Coord);
    for ( int idx=0; idx<wellssz; idx++ )
    {
	mRunStandardTestWithError( wrr.writePoint(wellcoords[idx],wellnms[idx]),
				   BufferString( "Write well ", wellnms[idx]),
				   toString(wrr.errMsg()) );
    }

    GIS::Property linedisps;
    wrr.getDefaultProperties( GIS::FeatureType::LineString, linedisps );
    wrr.setProperties( linedisps );
    wrr.setDescription( "This is a line geometry" );
    const int linesz = sizeof(linecoords) / sizeof(Coord);
    TypeSet<Coord> linecrds; linecrds.setCapacity( linesz, false );
    for ( int idx=0; idx<linesz; idx++ )
	linecrds  += linecoords[idx];
    mRunStandardTestWithError( wrr.writeLine( linecrds, "line" ), "Write line",
			       toString(wrr.errMsg()) );

    GIS::Property polydisps;
    wrr.getDefaultProperties( GIS::FeatureType::Polygon, polydisps );
    wrr.setProperties( polydisps );
    wrr.setDescription( "This is a polygon geometry" );
    const int polysz = sizeof(polygoncoords) / sizeof(Coord);
    TypeSet<Coord> polycrds; polycrds.setCapacity( polysz, false );
    for ( int idx=0; idx<polysz; idx++ )
	polycrds += polygoncoords[idx];
    mRunStandardTestWithError( wrr.writePolygon( polycrds, "polygon" ),
			       "Write polygon", toString(wrr.errMsg()) );

    GIS::Property pointsetdisps;
    wrr.getDefaultProperties( GIS::FeatureType::MultiPoint, pointsetdisps );
    wrr.setProperties( pointsetdisps );
    wrr.setDescription( "This is a pointset folder" );
    const int pointsetsz = 100;
    TypeSet<Coord> pointsetcrds; pointsetcrds.setCapacity( pointsetsz, false );
    Stats::RandGen gen;
    for ( int idx=0; idx<pointsetsz; idx++ )
    {
	const Coord crd( 624000.+(gen.get()*2000), 6077000.+(gen.get()*3000) );
	pointsetcrds += crd;
    }
    mRunStandardTestWithError( wrr.writePoints( pointsetcrds, "random points" ),
			       "Write PointSet", toString(wrr.errMsg()) );

    RefMan<Pick::Set> lines = new Pick::Set( "lines", false );
    wrr.setDescription( "This is a folder for multiple lines" );
    const int nrlines = 3;
    for ( int iline=0; iline<nrlines; iline++ )
    {
	lines->addStartIdx( lines->size() );
	for ( const auto& crd : linecrds )
	{
	    Coord newpos( crd );
	    newpos.x_ += (iline+1) * 4e3;
	    newpos.y_ -= (iline+1) * 1e4;
	    lines->add( newpos, mUdf(double) );
	}
    }

    mRunStandardTestWithError( wrr.writeLines( *lines.ptr() ),
			       "Write lines", toString(wrr.errMsg()) );

    RefMan<Pick::Set> polygons = new Pick::Set( "polygons", true );
    Pick::Set::Disp& polydisp = polygons->disp2d();
    mRunStandardTestWithError( polydisp.polyDisp(),
			       "Has polygon display properties.",
			       "Polygon display properties not created." );
    polydisp.polyDisp()->linestyle_.color_ = OD::Color::Green().darker( 0.5 );
    polydisp.polyDisp()->linestyle_.width_ = 4;
    polydisp.polyDisp()->dofill_ = true;
    polydisp.polyDisp()->fillcolor_ = OD::Color::Orange();
    polydisp.polyDisp()->fillcolor_.setTransparencyF( 0.5f );
    wrr.setDescription( "This is a folder for multiple polygons" );
    const int nrpolygons = 4;
    for ( int ipoly=0; ipoly<nrpolygons; ipoly++ )
    {
	polygons->addStartIdx( polygons->size() );
	for ( const auto& crd : polycrds )
	{
	    Coord newpos( crd );
	    newpos.x_ += (ipoly+1) * 4e3;
	    newpos.y_ -= (ipoly+1) * 1e4;
	    polygons->add( newpos, 0. );
	}
    }

    mRunStandardTestWithError( wrr.writePolygons( *polygons.ptr() ),
			       "Write polygons", toString(wrr.errMsg()) );

    return true;
}


static bool testGeoJSON( const Coords::CoordSystem& ed50crs )
{
    PtrMan<GIS::Writer> writer = GIS::Writer::factory().create( geojsonstr );
    mRunStandardTest( writer, "GeoJSON writer" );
    const BufferString outfile =
	FilePath::getTempFullPath( "test_giswriter", writer->getExtension() );
    writer->setSurveyName( "Test Program survey" )
	   .setElemName( "Various items" )
	   .setStream( outfile.str() );
    const bool res = testGISWriter( ed50crs, *writer.ptr() );
    writer = nullptr;
    if ( !keep_ )
	File::remove( outfile.str() );

    return res;
}


static bool testKML( const Coords::CoordSystem& ed50crs )
{
    PtrMan<GIS::Writer> writer = GIS::Writer::factory().create( kmlstr );
    mRunStandardTest( writer, "KML writer" );
    const BufferString outfile =
	FilePath::getTempFullPath( "test_giswriter", writer->getExtension() );
    writer->setSurveyName( "Test Program survey" )
	   .setElemName( "Various items" )
	   .setStream( outfile.str() );
    const bool res = testGISWriter( ed50crs, *writer.ptr() );
    writer = nullptr;
    if ( !keep_ )
	File::remove( outfile.str() );

    return res;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProgDR();

    OD::ModDeps().ensureLoaded( "General" );

    keep_ = clParser().hasKey( "keep" );

    BufferString msg;
    ConstRefMan<Coords::CoordSystem> ed50crs =
			    Coords::CoordSystem::createSystem( ED50str, msg );

    if ( !testInit(ed50crs.ptr()) ||
	 !testGeoJSON(*ed50crs.ptr()) ||
	 !testKML(*ed50crs.ptr()) )
	return 1;

    return 0;
}
