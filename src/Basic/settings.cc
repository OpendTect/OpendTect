/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/
 
static const char* rcsID = "$Id: settings.cc,v 1.24 2003-11-07 12:21:57 bert Exp $";

#include "settings.h"
#include "filegen.h"
#include "ascstream.h"
#include "errh.h"
#include "strmprov.h"
#include <filegen.h>

static const char* sKey = "Default settings";

static ObjectSet<Settings>& getSetts()
{
    static ObjectSet<Settings>* theinst_ = 0;
    if ( !theinst_ )
	theinst_ = new ObjectSet<Settings>;
    return *theinst_;
}


static void getFnm( const char* key, BufferString& fname )
{
    fname = GetSettingsDir();
    fname = File_getFullPath( fname, "settings" );
    if ( key )
	{ fname += "."; fname += key; }

    const char* ptr = GetSoftwareUser();
    if ( ptr )
    {
	fname += ".";
	fname += ptr;
    }
}


Settings& Settings::fetch( const char* key )
{
    BufferString settnm( key && *key ? key : "Common" );

    ObjectSet<Settings>& settlist = getSetts();
    for ( int idx=0; idx<settlist.size(); idx++ )
	if ( settlist[idx]->name() == settnm )
	    return *settlist[idx];

    const bool iscommon = settnm == "Common";
    if ( iscommon ) key = 0;
    BufferString fname;
    getFnm( key, fname );
    Settings* newsett = new Settings( fname );
    if ( !newsett->doReRead(iscommon) )
    {
	if ( iscommon )
	    ErrMsg( "Cannot find valid .od/settings file" );
    }

    newsett->setName( settnm );
    settlist += newsett;
    return *newsett;
}


bool Settings::doReRead( bool cpodsetts )
{
    StreamData sd = StreamProvider( fname ).makeIStream();

    bool do_write = false;
    BufferString usedfname = (const char*)fname;
    if ( !sd.usable() )
    {
	// Try using the '.old' file ...
	do_write = true;
	usedfname = (const char*)fname;
	usedfname += ".old";
	sd.close();

	sd = StreamProvider( usedfname ).makeIStream();
	if ( !sd.usable() )
	{
	    if ( !cpodsetts )
		return false;

	    sd.close();
	    usedfname = GetDataFileName("odSettings");
	    sd = StreamProvider( usedfname ).makeIStream();
	}
    }

    if ( !sd.usable() )
    {
	ErrMsg( "Cannot find any valid 'common' user settings file" );
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
