/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/
 
static const char* rcsID = "$Id: settings.cc,v 1.4 2000-08-08 14:14:28 bert Exp $";

#include "settings.h"
#include "filegen.h"
#include "ascstream.h"
#include "errh.h"
#include <fstream.h>


Settings* Settings::common_ = 0;
static const char* sKey = "Default settings";


Settings::Settings( const char* setupf )
	: IOPar("User Settings")
	, fname(setupf)
{
    if ( !setupf || !*setupf )
    {
	fname = getenv( "HOME" );
	fname = File_getFullPath( fname, ".dgbSettings" );
	const char* ptr = getenv( "dGB_USER" );
	if ( ptr )
	{
	    FileNameString tmp( fname );
	    tmp += ".";
	    tmp += ptr;
	    if ( File_exists(tmp) )
		fname = tmp;
	}
    }
    if ( !reRead() )
	ErrMsg( "Cannot find valid dgbSettings file" );
}


bool Settings::reRead()
{
    ifstream* strm = new ifstream( fname );
    if ( !strm || !strm->good() )
    {
	delete strm;
	strm = new ifstream( GetDataFileName( "dgbSettings" ) );
    }
    if ( !strm || !strm->good() )
	{ delete strm; return false; }

    ascistream stream( *strm, true );
    if ( !stream.isOfFileType( sKey ) )
	{ delete strm; return false; }

    clear();
    while ( !atEndOfSection( stream.next() ) )
    {
	const char* ptr = stream.keyWord();
	if ( *ptr == '*' ) ptr++;
	set( ptr, stream.value() );
    }

    delete strm;
    return true;
}


Settings::~Settings()
{
}


bool Settings::write() const
{
    ofstream strm( fname );
    if ( strm.fail() || strm.bad() ) return NO;

    ascostream stream( strm );
    stream.putHeader( sKey );
    putTo( stream, false );

    return true;
}
