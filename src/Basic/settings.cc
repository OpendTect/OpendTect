/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/


#include "settings.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "oddirs.h"
#include "safefileio.h"
#include "perthreadrepos.h"

static const char* sKeyDeflt = "Default settings";
static const char* sKeyCommon = "Common";
#define mGetKey(key) (key && *key ? key : sKeyCommon)
#define mIsCommon(key) (!key || !*key || StringView(key)==sKeyCommon)

class SettingsManager
{
public:

	    ~SettingsManager()
	    {
		deleteSettings();
	    }

    ObjectSet<Settings>& getSetts() { return setts_; }

    Settings& add( Settings& setts )
	    {
		setts_.add( &setts );
		return setts;
	    }

    void    deleteSettings()
	    {
		for ( int idx=setts_.size()-1; idx>=0; idx-- )
		    delete setts_.removeSingle( idx );
		setts_.setEmpty();
	    }

    static Threads::Lock	lock_;

private:
    ObjectSet<Settings> setts_;
};

static PtrMan<SettingsManager> settingsmanager = nullptr;
Threads::Lock SettingsManager::lock_( true );

void DeleteSettings()
{
    Threads::Locker locker( SettingsManager::lock_ );
    if ( settingsmanager )
	settingsmanager->deleteSettings();
}

ObjectSet<Settings>& getSetts()
{
    Threads::Locker locker( SettingsManager::lock_ );

    if ( !settingsmanager )
    {
	settingsmanager = new SettingsManager;
	NotifyExitProgram( DeleteSettings );
    }

    return settingsmanager->getSetts();
}


static BufferString getFileName( const char* key, const char* dtectusr,
				 const char* dirnm )
{
    FilePath fp( dirnm ? dirnm : GetSettingsDir(), "settings" );
    BufferString fname = fp.fullPath();
    if ( !mIsCommon(key) )
    {
	fname += "_";
	fname += key;
    }

    if ( dtectusr && *dtectusr )
    {
	fname += ".";
	fname += dtectusr;
    }

    return fname;
}


bool Settings::settsFileExist( const char* key )
{
    return File::exists( getFileName(key,GetSoftwareUser(),GetSettingsDir()) );
}

Settings& Settings::fetch( const char* key )
{
    const char* settkey = mGetKey( key );
    ObjectSet<Settings>& settlist = getSetts();
    for ( int idx=0; idx<settlist.size(); idx++ )
	if ( settlist[idx]->name() == settkey )
	    return *settlist[idx];

    Settings* newsett = doFetch( key, GetSoftwareUser(), GetSettingsDir(),
				 false );
    if ( !newsett )
    {
	if ( mIsCommon(key) )
	    ErrMsg( "Cannot find valid settings file in .od directory" );
	newsett = new Settings( settkey );
    }

    newsett->setName( settkey );
    newsett->fname_ = getFileName( key, GetSoftwareUser(), GetSettingsDir() );
    return settingsmanager->add( *newsett );
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

    auto* ret = new Settings( fname );
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
    const bool iscommon = name() == sKeyCommon;

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
    if ( do_merge )
    {
	IOPar dup( *this );
	Settings* me = const_cast<Settings*>( this );
	me->reRead();
	me->merge( dup );
    }

    SafeFileIO sfio( fname_ );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot open user settings file for write" );
	if ( sfio.errMsg() )
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


mExternC(Basic) const char* GetSettingsDataDir()
{
    mDeclStaticString( ret );
    Settings::common().get( "Default DATA directory", ret );
    return ret.buf();
}


bool SetSettingsDataDir( const char* dataroot, uiRetVal& uirv )
{
    const StringView curdataroot = GetSettingsDataDir();
    if ( !curdataroot.isEmpty() && curdataroot == dataroot )
	return true;

    Settings::common().set( "Default DATA directory", dataroot );
    if ( !Settings::common().write() )
    {
	uirv.add( od_static_tr("SetSettingsDataDir",
		    "Could not save Survey Data Root "
		    "location in the settings file") );
	return false;
    }

    return true;
}
