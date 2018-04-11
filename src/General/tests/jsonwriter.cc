/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bart de Groot
 * DATE     : March 2018
-*/


#include "testprog.h"

#include "bufstring.h"
#include "filepath.h"
#include "ascstream.h"
#include "od_ostream.h"
#include "geojson.h"
#include "autosaver.h"
#include "objectset.h"
#include "uistring.h"


int mTestMainFnName( int argc, char** argv )
{
	mInitTestProg();

	const File::Path fnm( "C:/appman/win64/gason/data/NLOG_Wells_POS_only.geojson" );
	od_istream strm( fnm );
	ascistream ascstrm( strm );
	Json::GeoJsonObject obj( ascstrm );
	strm.close();

	return 0;
}
