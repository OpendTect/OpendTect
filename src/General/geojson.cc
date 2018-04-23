/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Arnaud
 * DATE     : April 2018
-*/

#include "geojson.h"
#include "od_istream.h"


uiRetVal OD::GeoJsonTree::use( const char* fnm )
{
    od_istream strm( fnm );
    return use( strm );
}


uiRetVal OD::GeoJsonTree::use( od_istream& strm )
{
    filename_.set( strm.fileName() );

    uiRetVal uirv = read( strm );
    if ( !uirv.isOK() )
	return uirv;

    doGeoJSonCheck( uirv );
    return uirv;
}


#define mCheckPresence(obj,ky,fn) \
    const auto* ky = (obj)->fn( #ky ); \
    if ( !ky ) \
    { \
	uirv.set( missing_key_str.arg( #ky ).arg( filename_ ) ); \
	return; \
    }

void OD::GeoJsonTree::doGeoJSonCheck( uiRetVal& uirv )
{
    uirv.setEmpty();
    uiString missing_key_str( tr("Missing %1 key in GeoJSON file '%2'") );

    mCheckPresence( this, crs, getObject )
    mCheckPresence( crs, properties, getObject )
    mCheckPresence( this, features, getArray )
    if ( features->isEmpty() )
	uirv.set( tr("No features in GeoJSON file '%1'").arg( filename_ ) );
}


BufferString OD::GeoJsonTree::crsName() const
{
    return getObject( "crs" )
	 ->getObject( "properties" )
	 ->getStringValue( sKeyName() );
}
