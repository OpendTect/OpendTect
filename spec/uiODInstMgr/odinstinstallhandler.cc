/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          May 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstinstallhandler.cc 7930 2013-05-31 11:40:13Z ranojay.sen@dgbes.com $";

#include "odinstinstallhandler.h"

#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odinstdlhandler.h"
#include "odinstappdata.h"
#include "odinstlogger.h"
#include "odinstpkgselmgr.h"
#include "odinstpkgprops.h"
#include "odinstrel.h"
#include "odinstwinutils.h"
#include "odinstziphandler.h"
#include "odinstver.h"
#include "strmprov.h"
#include "strmoper.h"
#include "task.h"
#include "ziputils.h"

#ifdef __win__
# include <direct.h>
#else
# include <unistd.h>
#endif

static const char* unix_script_filename = "unix_install.csh";
static const char* packageidx_filename = "packageidx.txt";


static BufferString getBackUpDir( const char* instdir )
{
    FilePath fp( instdir );
    FixedString ext = fp.extension();
    if ( ext == "app" )
	fp.setExtension( 0 );

    BufferString dirnm = fp.fileName();
    dirnm += "_old";
    if ( ext == "app" )
	dirnm += ".app";

    fp.setFileName( dirnm );
    return fp.fullPath();
}


static bool runSetupOD( const char* dirnm )
{
#ifndef __win__

    const BufferString curdir( File::getCurrentPath() );
    chdir( dirnm );
    BufferString cmd( "./setup.od" );
    if ( system(cmd) )
    {
	BufferString errmsg( "Apparently failed to run ./setup.od in " );
	errmsg.add(  dirnm ).add( "." );
	mODInstToLog( errmsg );
    }
    chdir( curdir.buf() );
#endif

    return true;
}


namespace ODInst
{

InstallHandler::InstallHandler( AppData& appdata, const RelData& rd,
				const Platform& plf,
				PkgSelMgr& pslmgr, DLHandler& dlh )
		: oldappdata_(appdata)
		, appdata_(appdata)
		, reldata_(rd)
		, platform_(plf)
		, pkgselmgr_(pslmgr)
		, dlhandler_(dlh)
		, isnewinst_(appdata_.isNewInst())
		, status(this)
{
    checkInstallerVersion();
}


InstallHandler::~InstallHandler()
{
}


#define mHandleDirIssues( dirnm ) \
    if ( !File::exists(dirnm) ) \
    { \
	if ( !File::createDir(dirnm) ) \
	{ \
	    errmsg_ = "could not create directory in "; \
	    errmsg_ += dirnm; \
	    errmsg_ += ". Please check if you have write permission"; \
	    return false; \
	} \
    } \
    if ( !File::isWritable(dirnm) ) \
    { \
	errmsg_ = "You do not have write permission in "; \
	errmsg_ += dirnm; \
	return false; \
    } \


#define mErrRet( msg ) \
{ \
    errmsg_ = msg; \
    mODInstToLog( errmsg_ ); \
    return false; \
}

#define mSendMsg( msg ) \
    mODInstToLog( msg ); \
    status.trigger( msg );


bool InstallHandler::downLoadPackages( TaskRunner* tr, bool isoffl )
{
    if ( !isNewInst() && !prepareForUpdate(tr) )
	return false;

    mHandleDirIssues( appdata_.baseDirName() );
    pkgselmgr_.getPackageListsForAction( pckgstoinstall_, &toinstallpkgs_,
			    &updatedpkgs_, &reinstalledpkgs_ );
    mSendMsg( "Downloading Packages .... " );
    downloadedpkgs_.erase();
    BufferStringSet allpackages = pckgstoinstall_;
    BufferString nloc;
    if ( isoffl )
    {
	pkgselmgr_.getOfflineInstallPackages( allpackages );
	FilePath fp( appdata_.baseDirName(), "packages" );
	nloc = fp.fullPath();
	mHandleDirIssues( nloc );
    }

    for ( int idx=0; idx<allpackages.size(); idx++ )
    {
	const BufferString& pkgnm = allpackages.get(idx);
	BufferString zipfilenm( pkgselmgr_.getFullPackageName(pkgnm) );
	FilePath tmfp( !nloc.isEmpty() ? nloc.buf() : sTempDataDir(),zipfilenm);
	BufferString tmpzipfnm( tmfp.fullPath() );
	BufferString remotezipfnm(
	    FilePath(reldata_.version_.dirName(false),zipfilenm).fullPath() );
	BufferString msg( "Downloading ",  pkgselmgr_.getUserName(pkgnm) );
	msg += "...";
	status.trigger( pkgnm );
	mSendMsg( msg );
	if ( !dlHandler().fetchFile(remotezipfnm,tmpzipfnm,tr,msg) )
	    mErrRet( dlHandler().errMsg() )
	downloadedpkgs_.add( zipfilenm );
	downloadedzipfiles_.add( tmpzipfnm );
	mSendMsg( " " );
    }
    
    return true;
}


bool InstallHandler::installZipPackages( bool isoffline, TaskRunner* tr )
{
    if ( isoffline )
    {
	mSendMsg( "Preparing offline installation packages" );
	return prepareOfflineInstaller(tr);
    }

    mSendMsg( "Starting Installation" );
    mSendMsg( "Unpacking downloaded files" );
 
    ZipHandler zh( downloadedzipfiles_, appdata_ );
    if ( !zh.installZipPackages(tr) )
	mErrRet( zh.errMsg() );

    zh.makeFileList();

    mSendMsg( "Unpacking completed successfully" );
    if ( !isNewInst() && !restoreUpdates() )
	mErrRet( errmsg_ )
 
    return 1;
}


bool InstallHandler::configureInstallation()
{
    errmsg_ = "While configuring OpendTect, ";
#ifdef __win__
    ODInst::WinUtils wu( isNewInst() ? appdata_ : oldappdata_, platform_ );
    if ( isNewInst() )
    {
	mSendMsg( "Creating Desktop Icon ");
	if ( !wu.createDeskTopLinks("od_start_dtect.exe") )
	{
	    errmsg_+= wu.errMsg();
	    mODInstToLog( errmsg_ );
	    errmsg_ += " \nBut you can still run OpendTect from the ";
	    errmsg_ += "Installation folder located at ";
	    errmsg_ += appdata_.binPlfDirName();
	    return false;
	}
    }

    mODInstToLog( "Adding od_runinst to Start Menu" );
    if ( !wu.createStartMenuLinks("od_runinst.exe",
				"Installation manager",platform_.is32Bits(), true) )
    {
	errmsg_ += "od_runinst not found";
	mODInstToLog( errmsg_ );
    }

    mODInstToLog( "Adding the License Manager Tools to Start Menu" );
    if ( pckgstoinstall_.isPresent("dgbbase") 
	&& !wu.createStartMenuLinks("lmtools.exe",
					"License Manager Tools",true,false) )
    {
	errmsg_+= wu.errMsg();
	mODInstToLog( errmsg_ );
	errmsg_ += " \nBut you can still run OpendTect.";
	return false;
    }

#else

    BufferString appdir( appdata_.fullDirName() );
 
#ifdef __lux__ 
    if ( isNewInst() )
    {
	chdir( appdata_.fullDirName() );
	BufferString cmd( FilePath(appdir,"install").fullPath() );
	cmd += " -q > ";
	cmd += FilePath( sTempDataDir(), "instmsg.txt" ).fullPath();
	const int res = system( cmd );
	if ( res )
	{
	    errmsg_ += "apparently the install script failed. ";
	    mODInstToLog( errmsg_ );
	}

	return true;
    }
#endif

    BufferString backupdir = getBackUpDir( appdir );
    runSetupOD( backupdir );
    return runSetupOD( appdir );
#endif

    return true;
}


bool InstallHandler::prepareForUpdate( TaskRunner* tr )
{
    status.trigger( "Preparing for updates . . ." );
    BufferString oldbasedirnm = appdata_.baseDirName();
    FilePath curinst( appdata_.baseDirName(), appdata_.dirName() );
    FilePath updatefp( appdata_.baseDirName(), ODInst::AppData::sKeyUpdateDir(),
			appdata_.dirName() );
    mHandleDirIssues( updatefp.fullPath() );
    BufferString updtdir = updatefp.dirUpTo( updatefp.nrLevels()-2 );
    FilePath oldupd( updtdir, appdata_.dirName() );
    if ( File::exists(oldupd.fullPath()) )
	File::remove( oldupd.fullPath() );

    PtrMan<Executor> copier = File::getRecursiveCopier( curinst.fullPath(),
	    						updatefp.fullPath() );
    if ( !TaskRunner::execute( tr, *copier ) )
    {
	BufferString msg( "Failed to copy installation for update: ",
			  copier->message() );
	mErrRet(msg.buf())
    }

    return appdata_.setUpdateMode();
}


bool InstallHandler::restoreUpdates()
{
    mSendMsg( "Restoring updates . . ." );
    BufferString vernm = appdata_.dirName();
    BufferString oldpath = FilePath( oldappdata_.baseDirName(), vernm ).fullPath();
    BufferString newpath = FilePath( appdata_.baseDirName(), vernm ).fullPath();
    BufferString backuppath = getBackUpDir( oldpath );
    if ( File::exists(backuppath.buf()) && !File::remove(backuppath.buf()) )
    {
	appdata_.set( oldappdata_.baseDirName(), oldappdata_.dirName() );
	errmsg_ = " Could not remove ";
	errmsg_ += backuppath.buf();
	errmsg_ += ". Please manually remove this folder and try again.";
	return false;
    }
    
    if ( !File::rename( oldpath.buf(),backuppath.buf()) )
    {
	appdata_.set( oldappdata_.baseDirName(), oldappdata_.dirName() );
	errmsg_ = "It seems some of the OpendTect"
		" files or folders inside ";
	errmsg_ += oldpath;
	errmsg_ += " are open/running"
		   ". Please close all of them and try again";
	return false;
    }
    
    if ( !File::rename( newpath, oldpath ) )
    {
	if ( !File::rename(backuppath.buf(),oldpath.buf()) )
	{
	    errmsg_ = "Severe error: You need to install "
		      "OpendTect from scratch";
	    return false;
	}

	errmsg_ = "Update failed: Could not restore updated packages,"
	          " but your existing installation will still work";
	return false;
    }
    
    mSendMsg( "Finished restoring updates" );
    return true;
}


void InstallHandler::checkInstallerVersion() const
{
    FilePath instverfp( appdata_.baseDirName(), mInstallerDirNm, "relinfo" );
    BufferString versnfile( "ver.instmgr_", platform_.shortName() );
    versnfile += ".txt";
    instverfp.add( versnfile );
    ODInst::Version instver = getVersionFromPath( instverfp.fullPath() );

    FilePath myverfp( GetSoftwareDir(0), "relinfo" );
    BufferString myversnfile( "ver.instmgr_", platform_.shortName() );
    myversnfile += ".txt";
    myverfp.add( versnfile );
    ODInst::Version myver = getVersionFromPath( myverfp.fullPath() );

    myverfp.setFileName( "README.txt" ); // safety against copying od itself
    const bool inmainapplication = File::exists( myverfp.fullPath() );
    if ( myver > instver && !inmainapplication )
    {
	const BufferString instpath = FilePath(appdata_.baseDirName(),
					       mInstallerDirNm).fullPath();
	const BufferString bupath = getBackUpDir( instpath.buf() );
	if ( File::exists(instpath) )
	{
	    if( File::exists(bupath) )
		File::remove( bupath );

	    File::rename( instpath, bupath );
	}
	    
	File::copyDir( GetSoftwareDir(0), instpath );
    }
}


ODInst::Version InstallHandler::getVersionFromPath( 
						const BufferString& path ) const
{
    StreamData sd = StreamProvider( path ).makeIStream();
    BufferString instver;
    if ( sd.usable() )
	StrmOper::readLine( *sd.istrm, &instver );
    sd.close();
    ODInst::Version version( instver.buf() );
    return version;
}


bool InstallHandler::prepareOfflineInstaller( TaskRunner* tr )
{
    const BufferString idxfnm(
	    	FilePath(appdata_.baseDirName(),packageidx_filename).fullPath() );
    StreamData sd = StreamProvider(idxfnm).makeOStream();
    if ( !sd.usable() )
    {
	sd.close();
	BufferString msg( "Failed to write file '", packageidx_filename );
	msg += "' into the output directory";
	mErrRet( msg );
    }
    
    BufferStringSet packages = downLoadedPackages();
    IOPar idxfilepar;
    idxfilepar.set( "Version", reldata_.version_.dirName(false) );
    idxfilepar.set( "Packages", packages );
    idxfilepar.write( *sd.ostrm, "indexfile" );
    sd.close();

    if ( !downloadSetupFiles(tr) )
    	return false;
        
    return true;
}


bool InstallHandler::downloadSetupFiles( TaskRunner* tr )
{
    if ( !platform_.isWindows() )
    {
	const BufferString scriptfnm(
	    FilePath(appdata_.baseDirName(),unix_script_filename).fullPath() );
	if ( !dlHandler().fetchFile(unix_script_filename,scriptfnm,tr) )
	{
	    BufferString msg( "Could not download ", unix_script_filename ); // ask error msg
	    mErrRet( msg );
	}
	File::makeExecutable( scriptfnm, true );
	return true;
    }
    BufferString zipfilenm( "od_", platform_.shortName(), "_setup" );
    zipfilenm += ".zip";

    BufferString remotezipfnm( FilePath("Installer",zipfilenm).fullPath() );
    BufferString tmpzipfnm( FilePath((sTempDataDir()),zipfilenm).fullPath() );
    if ( !dlHandler().fetchFile(remotezipfnm,tmpzipfnm,tr,"Downloading setup") )
	mErrRet( dlHandler().errMsg() )

    if ( !File::exists(tmpzipfnm) )
    {
	BufferString erms( "Could not find ", tmpzipfnm );
	mErrRet( erms )
    }

    if ( !ZipUtils::unZipArchive(tmpzipfnm,appdata_.baseDirName(),errmsg_,tr) )
	mErrRet( errmsg_ );
    
    return true;
}

} // namespace ODInst
