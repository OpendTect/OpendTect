/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/
 
static const char* rcsID = "$Id: settings.cc,v 1.6 2000-08-09 09:11:02 bert Exp $";

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
	    fname += ".";
	    fname += ptr;
	}
    }
    if ( !reRead() )
	ErrMsg( "Cannot find valid dgbSettings file" );
}


bool Settings::reRead()
{
    ifstream* strm = new ifstream( fname );
    bool do_write = false;
    if ( !strm || !strm->good() )
    {
	delete strm;
	strm = new ifstream( GetDataFileName( "dgbSettings" ) );
	do_write = true;
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
    if ( do_write ) write();
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
