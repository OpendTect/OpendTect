/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/


#include "settings.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "oddirs.h"
#include "odruncontext.h"
#include "safefileio.h"
#include "staticstring.h"

static const char* sKeyDeflt = "Default settings";
static const char* sKeyCommon = "Common";
#define mGetKey(key) (key && *key ? key : sKeyCommon)
#define mIsCommon(key) (!key || !*key || FixedString(key)==sKeyCommon)

static PtrMan<ObjectSet<Settings> > theinst_ = 0;

static ObjectSet<Settings>& getSetts()
{
    if ( !theinst_ )
    {
	ObjectSet<Settings>* ptr = new ObjectSet<Settings>;
	if ( !theinst_.setIfNull(ptr,true) )
	    delete ptr;
    }

    return *theinst_;
}


static BufferString getFileName( const char* key, const char* dtectusr,
				 const char* dirnm )
{
    File::Path fp( dirnm ? dirnm : GetSettingsDir(), "settings" );
    BufferString fname = fp.fullPath();
    if ( !mIsCommon(key) )
	{ fname += "_"; fname += key; }
    if ( dtectusr && *dtectusr )
	{ fname += "."; fname += dtectusr; }
    return fname;
}


Settings& Settings::fetch( const char* key )
{
    const char* settkey = mGetKey( key );
    ObjectSet<Settings>& settlist = getSetts();
    for ( int idx=0; idx<settlist.size(); idx++ )
	if ( settlist[idx]->hasName(settkey) )
	    return *settlist[idx];

    Settings* newsett = doFetch( key, GetSoftwareUser(), GetSettingsDir(),
				 false );
    if ( !newsett )
    {
	if ( mIsCommon(key) && !OD::InInstallerRunContext() )
	    ErrMsg( "Continuing with fresh settings file in .od directory" );
	newsett = new Settings( settkey );
    }

    newsett->setName( settkey );
    newsett->fname_ = getFileName( key, GetSoftwareUser(), GetSettingsDir() );
    settlist += newsett;
    return *newsett;
}


Settings* Settings::fetchExternal( const char* key, const char* dtectusr,
				   const char* dirnm )
{
    return doFetch( key, dtectusr, dirnm, true );
}


Settings* Settings::doFetch( const char* key, const char* dtectusr,
			     const char* dirnm, bool ext )
{
    BufferString fname( getFileName(key,dtectusr,dirnm) );

    Settings* ret = new Settings( fname );
    ret->setName( mGetKey(key) );
    if ( !ret->doRead(ext) )
	{ delete ret; ret = nullptr; }
    return ret;
}


static void handleLegacyPar( Settings& setts, const char* key,
			     const char* settfnm )
{
    IOPar* legacypar = setts.subselect( key );
    if ( legacypar && legacypar->size() )
    {
	Settings& modernsetts( Settings::fetch(settfnm) );
	modernsetts.merge( *legacypar );
	modernsetts.write( false );
	BufferString rmkey( key ); rmkey += ".*";
	setts.removeWithKeyPattern( rmkey );
	setts.write( false );
    }
    delete legacypar;
}


bool Settings::doRead( bool ext )
{
    const bool empty_initially = File::isEmpty(fname_);
    const bool iscommon = hasName( sKeyCommon );

    SafeFileIO sfio( fname_, false );
    if ( empty_initially || !sfio.open(true) )
    {
	if ( ext )
	    return false;

	BufferString tmplfname( iscommon ? "od" : name().buf() );
	tmplfname += "Settings";
	tmplfname = mGetSetupFileName(tmplfname);
	bool okaftercopy = false;
	if ( File::exists(tmplfname) )
	{
	    File::copy( tmplfname, fname_ );
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
	emsg += fname_;
	emsg += "' seems to be corrupted.";
	sfio.closeFail();
	return false;
    }

    setEmpty();
    getFrom( stream );
    sfio.closeSuccess();

    if ( empty_initially )
    {
	write( false );
    }
    else if ( iscommon && stream.majorVersion() < 3 )
    {
	handleLegacyPar( *this, "Color table", "coltabs" );
	handleLegacyPar( *this, "Shortcuts", "shortcuts" );
    }
    return true;
}


bool Settings::write( bool do_merge ) const
{
    IOPar wriop( *this );
    if ( do_merge )
    {
	Settings* me = const_cast<Settings*>( this );
	me->reRead();
	me->merge( wriop );
	wriop = *this;
    }
    wriop.sortOnKeys();

    SafeFileIO sfio( fname_ );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot open user settings file for write" );
	if ( !sfio.errMsg().isEmpty() )
	    { msg += "\n\t"; msg += toString( sfio.errMsg() ); }
	ErrMsg( msg );
	return false;
    }

    ascostream stream( sfio.ostrm() );
    stream.putHeader( sKeyDeflt );
    wriop.putTo( stream, true );
    if ( !sfio.closeSuccess() )
    {
	const BufferString msg( "Error closing user settings file:\n",
				toString( sfio.errMsg() ) );
	ErrMsg( msg );
	return false;
    }

    return true;
}


mExternC(Basic) const char* GetSettingsDataDir()
{
    mDeclStaticString( ret );
    Settings::common().get( "Default DATA directory", ret );
    return ret.buf();
}
