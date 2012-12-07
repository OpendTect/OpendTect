/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "plugins.h"

#include "debugmasks.h"
#include "dirlist.h"
#include "envvars.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "separstr.h"
#include "settings.h"
#include "strmprov.h"
#include "staticstring.h"
#include "moddepmgr.h"

#include <iostream>

#ifndef __win__
# include <dlfcn.h>
#endif


static const char* sPluginDir = "plugins";
#ifndef __cmake__

#ifndef __win__
    static const char* sPluginBinDir = sPluginDir;
#else
    static const char* sPluginBinDir = "bin";
#endif

#else

    static const char* sPluginBinDir = "bin";

#endif

static const char* sKeyNoDispName = "??";

extern "C" {

    typedef int (*VoidIntRetFn)(void);
    typedef const char* (*ArgcArgvCCRetFn)(int,char**);
    typedef PluginInfo* (*PluginInfoRetFn)(void);

    void LoadAutoPlugins( int argc, char** argv, int inittype )
    {
	static int first_time = 1;
	if ( first_time )
	{
	    first_time = 0;
	    PIM().setArgs( argc, argv );
	}
	PIM().loadAuto( inittype == PI_AUTO_INIT_LATE );
    }

    int LoadPlugin( const char* libnm )
    {
	return PIM().load( libnm ) ? mC_True : mC_False;
    }
};


SharedLibAccess::SharedLibAccess( const char* lnm )
    	: handle_(0)
{
    if ( !lnm || !*lnm  )
	return;

#ifdef __win__

    BufferString targetlibnm( lnm );
    if ( File::isLink(lnm) )
	targetlibnm = File::linkTarget(lnm);

    if ( File::exists(targetlibnm) )
    {
	handle_ = LoadLibrary( targetlibnm );
	if ( !handle_ )
	{
	    char* ptr = NULL;
	    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
			   FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			   GetLastError(), 0, (char* )&ptr, 1024, NULL );
	    ErrMsg( ptr );
	}
    }

#else

    if ( File::exists(lnm) )
    {
	handle_ = dlopen( lnm, RTLD_GLOBAL | RTLD_NOW );

	if ( !handle_ )
	    ErrMsg( dlerror() );
    }

#endif

    if( DBG::isOn(DBG_PROGSTART) )
    {
	BufferString msg( "Attempt to get open handle for sh lib " );
	msg += lnm; msg += handle_ ? " succeeded" : " failed";
	DBG::message( msg );
    }
}


void SharedLibAccess::close()
{
    if ( !handle_ ) return;
#ifdef __win__
    FreeLibrary( handle_ );
#else
    dlclose( handle_ );
#endif
    handle_ = 0;
}


void* SharedLibAccess::getFunction( const char* fnnm ) const
{
    if ( !handle_ )
	return 0;

#ifdef __win__
    return (void*)GetProcAddress( handle_, fnnm );
#else
    return dlsym( handle_, fnnm );
#endif
}



void SharedLibAccess::getLibName( const char* modnm, char* out )
{
#ifdef __win__
    strcpy( out, modnm ); strcat( out, ".dll" );
#else
    strcpy( out, "lib" ); strcat( out, modnm );
# ifdef __mac__
    strcat( out, ".dylib" );
# else
    strcat( out, ".so" );
# endif
#endif
}


static const char* errargv[] = { "<not set>", 0 };

PluginManager::PluginManager()
    : argc_(1)
    , argv_(const_cast<char**>(errargv))
{
    getDefDirs();
}


void PluginManager::setArgs( int argc, char** argv )
{
    argc_ = argc, argv_ = argv;
    // poss todo: get new defdirs from argv
    mkALOList();
}


static BufferString getProgNm( const char* argv0 )
{
    FilePath fp( argv0 );
#ifdef __win__
    fp.setExtension( 0 );
#endif
    return fp.fileName();
}


static const char* getFnNm( const char* libnm, const char* fnbeg,
			      const char* fnend )
{
    static StaticStringManager stm;
    BufferString& ret = stm.getString();

    ret = fnbeg;

    if ( (*libnm     == 'l' || *libnm     == 'L')
      && (*(libnm+1) == 'i' || *(libnm+1) == 'I')
      && (*(libnm+2) == 'b' || *(libnm+2) == 'B') )
	libnm += 3;
    ret += libnm;

    char* ptr = strchr( ret.buf(), '.' );
    if ( ptr ) *ptr = '\0';
    ret += fnend;

    return ret.buf();
}


void PluginManager::getDefDirs()
{
    bool fromenv = false;
    BufferString dnm = GetEnvVar( "OD_APPL_PLUGIN_DIR" );
    if ( dnm.isEmpty() )
	dnm = GetSoftwareDir(0);
    else
	fromenv = true;

    FilePath fp( dnm );
    appdir_ = fp.fullPath();
    if ( !fromenv )
	fp.add( sPluginBinDir );
    fp.add( GetPlfSubDir() );
#ifdef __win__ 
# ifdef __debug__
    fp.add( "debug" );
# endif
#endif
    
#ifdef __mac__
# ifdef __debug__
    fp.add( "Debug" );
# endif
#endif

    applibdir_ = fp.fullPath();

    fromenv = false;
    dnm = GetEnvVar( "OD_USER_PLUGIN_DIR" );
    if ( dnm.isEmpty() )
	dnm = GetSettingsDir();
    else
	fromenv = true;

    fp.set( dnm );
    userdir_ = fp.fullPath();
    if ( !fromenv )
	fp.add( sPluginBinDir );
    fp.add( GetPlfSubDir() );
#ifdef __win__
#ifdef __debug__
    fp.add( "debug" );
#endif
#endif
    userlibdir_ = fp.fullPath();

    if( DBG::isOn(DBG_PROGSTART) )
    {
        BufferString msg( "plugins.cc - getDefDirs\n" );
	msg += "appdir_="; msg += appdir_;
	msg += " userdir_="; msg += userdir_;
	msg += "\napplibdir_="; msg += applibdir_;
	msg += " userlibdir_="; msg += userlibdir_;
	DBG::message( msg );
    }
}


static bool isALO( const char* fnm )
{
    const char* extptr = strrchr( fnm, '.' );
    if ( !extptr || !*extptr ) return false;
    return caseInsensitiveEqual( extptr+1, "alo", 0 );
}


const PluginManager::Data* PluginManager::findDataWithDispName(
			const char* nm ) const
{
    if ( !nm || !*nm ) return 0;
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	const Data* data = data_[idx];
	const PluginInfo* piinf = data->info_;
	if ( piinf && piinf->dispname && !strcmp(piinf->dispname,nm) )
	    return data;
    }
    return 0;
}


const char* PluginManager::getFileName( const PluginManager::Data& data ) const
{
    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    if ( data.autosource_ == Data::None )
	ret = data.name_;
    else
	ret = FilePath(
		data.autosource_ == Data::AppDir ?  applibdir_ : userlibdir_,
#ifndef __cmake__
#ifndef __win__
		"libs",
#endif
#endif
		data.name_ ).fullPath();
    return ret.buf();
}


PluginManager::Data* PluginManager::fndData( const char* nm ) const
{
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	const Data* data = data_[idx];
	if ( data->name_ == nm )
	    return const_cast<Data*>(data);
    }
    return 0;
}


bool PluginManager::isPresent( const char* nm ) const
{
    return findData( nm );
}


const char* PluginManager::userName( const char* nm ) const
{
    const Data* data = findData( nm );
    const PluginInfo* piinf = data ? data->info_ : 0;
    if ( !piinf )
    {
	FilePath fp( nm );
	return getFnNm( fp.fileName(), "", "" );
    }

    return piinf->dispname;
}


const char* PluginManager::moduleName( const char* libnm )
{
    return getFnNm( libnm, "", "" );
}

static PluginInfo* mkEmptyInfo()
{
    PluginInfo* piinf = new PluginInfo;
    piinf = new PluginInfo;
    piinf->dispname = sKeyNoDispName;
    piinf->creator = piinf->version = "";
    piinf->text = "No info available";
    return piinf;
}


#define mGetFn(typ,sla,nm1,nm2,libnm) \
	typ fn = sla ? (typ)sla->getFunction( getFnNm(libnm,nm1,nm2) ) : 0


static PluginInfo* getPluginInfo( SharedLibAccess* sla, const char* libnm )
{
    mGetFn(PluginInfoRetFn,sla,"Get","PluginInfo",libnm);
    return fn ? (*fn)() : mkEmptyInfo();
}


static int getPluginType( SharedLibAccess* sla, const char* libnm )
{
    mGetFn(VoidIntRetFn,sla,"Get","PluginType",libnm);
    return fn ? (*fn)() : PI_AUTO_INIT_LATE;
}


void PluginManager::getNotLoadedByUser( FileMultiString& dontloadlist ) const
{
    Settings::common().get( sKeyDontLoad(), dontloadlist.rep() );
}


void PluginManager::openALOEntries()
{
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	Data& data = *data_[idx];
	data.sla_ = 0;
	if ( data.autosource_ == Data::None )
	    continue;

	data.sla_ = new SharedLibAccess( getFileName(data) );
	if ( !data.sla_->isOK() )
	{
	    delete data.sla_; data.sla_ = 0;

	    if ( data.autosource_ == Data::Both )
	    {
		data.autosource_ = data.autosource_ == Data::UserDir
				    ? Data::AppDir : Data::UserDir;
		data.sla_ = new SharedLibAccess( getFileName(data) );
		if ( !data.sla_->isOK() )
		    { delete data.sla_; data.sla_ = 0; }
	    }
	}

	if ( !data.sla_ )
	    OD::ModDeps().ensureLoaded( moduleName( data.name_ ) );
	else
	{
	    data.autotype_ = getPluginType( data.sla_, data.name_ );
	    data.info_ = getPluginInfo( data.sla_, data.name_ );
	}
    }
}


void PluginManager::getALOEntries( const char* dirnm, bool usrdir )
{
    FilePath fp( dirnm, sPluginDir, GetPlfSubDir() );
    DirList dl( fp.fullPath(), DirList::FilesOnly );
    const BufferString prognm = getProgNm( argv_[0] );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString fnm = dl.get(idx);
	if ( !isALO(fnm) ) continue;

	*strchr( fnm.buf(), '.' ) = '\0';
	if ( fnm != prognm ) continue;

	StreamData sd = StreamProvider( dl.fullPath(idx) ).makeIStream(false);
	if ( !sd.usable() ) { sd.close(); continue; }

	char name[128];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( name, 128 );
	    if ( name[0] == '\0' ) continue;

	    char libnm[256]; SharedLibAccess::getLibName( name, libnm );
	    Data* data = findData( libnm );
	    if ( !data )
	    {
		data = new Data( libnm );
		data->autosource_ = usrdir ? Data::UserDir : Data::AppDir;
		data_ += data;
	    }
	    else if ( usrdir != Data::isUserDir(data->autosource_) )
		data->autosource_ = Data::Both;
	}

	sd.close();
    }
}


void PluginManager::mkALOList()
{
    getALOEntries( userdir_, true );
    getALOEntries( appdir_, false );
    openALOEntries();
}


static bool loadPlugin( SharedLibAccess* sla, int argc, char** argv,
       			const char* libnm )
{
    mGetFn(ArgcArgvCCRetFn,sla,"Init","Plugin",libnm);
    if ( !fn ) // their bad
	return false;

    const char* ret = (*fn)( argc, argv );
    if ( ret )
    {
	const BufferString libnmonly = FilePath(libnm).fileName();
	BufferString msg( "Message from " );
	msg += libnm; msg += ":\n\t"; msg += ret;
	UsrMsg( msg );
	return false;
    }

    return true;
}


bool PluginManager::load( const char* libnm )
{
    FilePath fp( libnm );
    const BufferString libnmonly( fp.fileName() );

    Data* data = new Data( libnmonly );
    data->sla_ = new SharedLibAccess( libnm );
    if ( !data->sla_->isOK() )
	{ delete data; return false; }

    data->info_ = getPluginInfo( data->sla_, libnmonly );
    if ( !data->info_ )
	{ delete data; return false; }

    Data* existing = const_cast<Data*>(
	findDataWithDispName( data->info_->dispname ) );

    if ( existing && existing->sla_ && existing->sla_->isOK() )
    {
	data->sla_->close();
	delete data;
	data = 0;

	if ( existing->isloaded_ )
	    return false;

	if ( !loadPlugin(existing->sla_,argc_,argv_,libnmonly) )
	{
	    existing->info_ = 0;
	    existing->sla_->close();
	    delete existing->sla_; existing->sla_ = 0;
	    return false;
	}

	existing->isloaded_ = true;
    }
    else
    {
	if ( !loadPlugin(data->sla_,argc_,argv_,libnmonly) )
	{
	    data->sla_->close();
	    delete data;
	    return false;
	}

	data->isloaded_ = true;
	data_ += data;
    }

    return true;
}


void PluginManager::loadAuto( bool late )
{
    FileMultiString dontloadlist;
    getNotLoadedByUser( dontloadlist );

    for ( int idx=0; idx<data_.size(); idx++ )
    {
	Data& data = *data_[idx];
	if ( !data.sla_ || !data.sla_->isOK() || data.autosource_==Data::None )
	    continue;

	const int pitype = late ? PI_AUTO_INIT_LATE : PI_AUTO_INIT_EARLY;
	if ( data.autotype_ != pitype )
	    continue;

	if ( data.info_ && dontloadlist.indexOf( data.info_->dispname )!=-1 )
	    continue;

	if ( !loadPlugin(data.sla_,argc_,argv_,data.name_) )
	{
	    data.info_ = 0;
	    data.sla_->close();
	    delete data.sla_; data.sla_ = 0;
	}

	data.isloaded_ = true;

	static bool shw_load = GetEnvVarYN( "OD_SHOW_PLUGIN_LOAD" );
	if ( shw_load )
	{
	    BufferString msg;
	    if ( data.sla_ )
		msg = "Successfully loaded plugin '";
	    else
		msg = "Failed to load plugin '";
	    msg += userName(data.name_); msg += "'";
	    UsrMsg( msg );
	}
    }
}


PluginManager& PIM()
{
    static PluginManager* inst = new PluginManager;
    return *inst;
}
