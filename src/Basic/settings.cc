/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/
 
static const char* rcsID = "$Id: settings.cc,v 1.20 2003-10-15 09:12:57 arend Exp $";

#include "settings.h"
#include "filegen.h"
#include "ascstream.h"
#include "errh.h"
#include "strmprov.h"
#include <filegen.h>

Settings* Settings::common_ = 0;
static const char* sKey = "Default settings";


Settings::Settings( const char* setupf )
	: IOPar("User Settings")
	, fname(setupf)
{
    if ( !setupf || !*setupf )
    {
	fname = GetSettingsDir();
	fname = File_getFullPath( fname, ".dgbSettings" );
	const char* ptr = GetSoftwareUser();
	if ( ptr )
	{
	    fname += ".";
	    fname += ptr;
	}
    }
    if ( !reRead() )
	ErrMsg( "Cannot find valid dgbSettings file" );

    removeWithKey("Use new viewers");
    removeWithKey("Use old viewers");
}


bool Settings::reRead()
{
    StreamData sd = StreamProvider( fname ).makeIStream();

    bool do_write = false;
    BufferString usedfname = (const char*)fname;
    if ( !sd.usable() )
    {
	do_write = true;
	usedfname = (const char*)fname;
	usedfname += ".old";
	sd.close();

	sd = StreamProvider( usedfname ).makeIStream();
	if ( !sd.usable() )
	{
	    sd.close();
	    usedfname = GetDataFileName("dgbSettings");
	    sd = StreamProvider( usedfname ).makeIStream();
	}
    }
    if ( !sd.usable() )
    {
	ErrMsg( "Cannot find any valid user settings file" );
	sd.close(); return false;
    }

    ascistream stream( *sd.istrm, true );
    if ( !stream.isOfFileType( sKey ) )
    {
	BufferString emsg = "User settings file '";
	emsg += usedfname;
	emsg += "' seems to be corrupted.";
	sd.close(); return false;
    }

    clear();
    while ( !atEndOfSection( stream.next() ) )
    {
	const char* ptr = stream.keyWord();
	if ( *ptr == '*' ) ptr++;
	set( ptr, stream.value() );
    }

    sd.close();

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
    StreamData sd = StreamProvider(newfname).makeOStream();

    if ( !sd.usable() )
    {
	ErrMsg( "Cannot open '.new' settings file" );
	sd.close();
	return false;
    }

    if ( do_merge )
    {
	IOPar dup( *this );
	Settings* me = const_cast<Settings*>( this );
	me->reRead();
	me->merge( dup );
    }

    ascostream stream( *sd.ostrm );
    stream.putHeader( sKey );
    putTo( stream, false );
    sd.close();

    BufferString oldfname = (const char*)fname;
    oldfname += ".old";
    if ( File_exists(oldfname) && !File_remove(oldfname,NO) )
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


extern "C" const char* GetSettingsDataDir()
{
    static BufferString dirnm;
    Settings::common().get( "Default DATA directory", dirnm );
    return dirnm.buf();
}
