/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/
 
static const char* rcsID = "$Id: settings.cc,v 1.30 2006-02-20 18:49:48 cvsbert Exp $";

#include "settings.h"
#include "filegen.h"
#include "filepath.h"
#include "ascstream.h"
#include "safefileio.h"
#include "oddirs.h"
#include "errh.h"
#include <filegen.h>

static const char* sKeyDeflt = "Default settings";

static ObjectSet<Settings>& getSetts()
{
    static ObjectSet<Settings>* theinst_ = 0;
    if ( !theinst_ )
	theinst_ = new ObjectSet<Settings>;
    return *theinst_;
}


static void getFnm( const char* key, BufferString& fname )
{
    FilePath fp( GetSettingsDir() ); fp.add( "settings" );
    fname = fp.fullPath();
    if ( key )
	{ fname += "_"; fname += key; }

    const char* ptr = GetSoftwareUser();
    if ( ptr )
    {
	fname += ".";
	fname += ptr;
    }
}


static const char* sKeyCommon = "Common";

Settings& Settings::fetch( const char* key )
{
    BufferString settnm( key && *key ? key : sKeyCommon );

    ObjectSet<Settings>& settlist = getSetts();
    for ( int idx=0; idx<settlist.size(); idx++ )
	if ( settlist[idx]->name() == settnm )
	    return *settlist[idx];

    const bool iscommon = settnm == sKeyCommon;
    BufferString fname;
    getFnm( iscommon ? 0 : settnm.buf(), fname );
    Settings* newsett = new Settings( fname );
    newsett->setName( settnm );
    if ( !newsett->reRead() && iscommon )
	ErrMsg( "Cannot find valid .od/settings file" );

    settlist += newsett;
    return *newsett;
}


bool Settings::reRead()
{
    SafeFileIO sfio( fname, false );

    const bool do_write = File_isEmpty(fname);
    const bool iscommon = name() == sKeyCommon;
    if ( !sfio.open(true) )
    {
	BufferString tmplfname( iscommon ? "od" : name().buf() );
	tmplfname += "Settings";
	tmplfname = GetDataFileName(tmplfname);
	bool okaftercopy = false;
	if ( File_exists(tmplfname) )
	{
	    File_copy( tmplfname, fname, NO );
	    if ( sfio.open(true) )
		okaftercopy = true;
	}
	if ( !okaftercopy )
	{
	    if ( iscommon )
		ErrMsg( sfio.errMsg() );
	    return false;
	}
    }

    ascistream stream( sfio.istrm(), true );
    if ( !stream.isOfFileType( sKeyDeflt ) )
    {
	BufferString emsg = "User settings file '";
	emsg += fname;
	emsg += "' seems to be corrupted.";
	sfio.closeFail();
	return false;
    }

    clear();
    while ( !atEndOfSection( stream.next() ) )
    {
	const char* ptr = stream.keyWord();
	if ( *ptr == '*' ) ptr++;
	set( ptr, stream.value() );
    }
    sfio.closeSuccess();

    if ( do_write ) write( false );
    return true;
}


bool Settings::write( bool do_merge ) const
{
    if ( do_merge )
    {
	IOPar dup( *this );
	Settings* me = const_cast<Settings*>( this );
	me->reRead();
	me->merge( dup );
    }

    SafeFileIO sfio( fname );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot open user settings file for write" );
	if ( *sfio.errMsg() )
	{ msg += "\n\t"; msg += sfio.errMsg(); }
	    ErrMsg( msg );
	return false;
    }

    ascostream stream( sfio.ostrm() );
    stream.putHeader( sKeyDeflt );
    putTo( stream );
    if ( !sfio.closeSuccess() )
    {
	BufferString msg( "Error closing user settings file:\n" );
	msg += sfio.errMsg();
	ErrMsg( msg );
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
