/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/
 
static const char* rcsID = "$Id: settings.cc,v 1.12 2001-09-28 12:06:32 bert Exp $";

#include "settings.h"
#include "filegen.h"
#include "ascstream.h"
#include "errh.h"
#include <fstream>
#include <filegen.h>


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

    bool useold = false;
    const char* res = find( "Use new viewers" );
    bool do_write = false;
    if ( res )
    {
	useold = !yesNoFromString(res);
	removeWithKey("Use new viewers");
	do_write = true;
    }

    res = find( "Use old viewers" );
    if ( !res )
    {
	setYN( "Use old viewers", useold );
	do_write = true;
    }

    if ( do_write )
	write( false );
}


bool Settings::reRead()
{
    ifstream* strm = new ifstream( fname );
    bool do_write = false;
    BufferString usedfname = (const char*)fname;
    if ( !strm || !strm->good() )
    {
	do_write = true;
	usedfname = (const char*)fname;
	usedfname += ".old";
	delete strm;
	strm = new ifstream( usedfname );
	if ( !strm || !strm->good() )
	{
	    delete strm;
	    usedfname = GetDataFileName("dgbSettings");
	    strm = new ifstream( usedfname );
	}
    }
    if ( !strm || !strm->good() )
    {
	ErrMsg( "Cannot find any valid user settings file" );
	delete strm; return false;
    }

    ascistream stream( *strm, true );
    if ( !stream.isOfFileType( sKey ) )
    {
	BufferString emsg = "User settings file '";
	emsg += usedfname;
	emsg += "' seems to be corrupted.";
	delete strm; return false;
    }

    clear();
    while ( !atEndOfSection( stream.next() ) )
    {
	const char* ptr = stream.keyWord();
	if ( *ptr == '*' ) ptr++;
	set( ptr, stream.value() );
    }

    delete strm;
    if ( do_write ) write( false );
    return true;
}


Settings::~Settings()
{
}


bool Settings::write( bool do_merge ) const
{
    BufferString newfname = (const char*)fname;
    newfname += ".new";
    ofstream strm( newfname );
    if ( strm.fail() || strm.bad() )
    {
	ErrMsg( "Cannot open '.new' settings file" );
	return false;
    }

    if ( do_merge )
    {
	IOPar dup( *this );
	Settings* me = const_cast<Settings*>( this );
	me->reRead();
	me->merge( dup );
    }

    ascostream stream( strm );
    stream.putHeader( sKey );
    putTo( stream, false );

    BufferString oldfname = (const char*)fname;
    oldfname += ".old";
    if ( File_exists(oldfname) && !File_remove(oldfname,NO,NO) )
    {
	ErrMsg( "Cannot remove '.old' settings file" );
	return false;
    }
    if ( File_exists(fname) && !File_rename(fname,oldfname) )
    {
	ErrMsg( "Cannot rename settings file to '.old'" );
	return false;
    }
    if ( !File_rename(newfname,fname) )
    {
	ErrMsg( "Cannot rename new settings file" );
	return false;
    }
    return true;
}
