/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Mar 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_setup.cc 7930 2013-05-31 11:40:13Z ranojay.sen@dgbes.com $";

#include "prog.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "strmoper.h"
#include "strmprov.h"
#include "winutils.h"
#include "ziputils.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "userinputobj.h"
#include "uitextedit.h"
#include "uitaskrunner.h"

#ifdef __win__

bool CreateLink( LPCSTR lpszPathObj, LPCSTR lpszworkdir, LPCSTR lpszPathLink, 
		 LPCSTR lpszDesc )
{
    HRESULT hres;
    IShellLink *psl;
    CoInitialize (NULL);

    hres = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
			     IID_IShellLink, (LPVOID*)&psl );
    if ( SUCCEEDED(hres) )
    {
	IPersistFile* ppf;
	hres = psl->SetPath( lpszPathObj );
	hres = psl->SetDescription( lpszDesc );
	hres = psl->SetWorkingDirectory( lpszworkdir );
	hres = psl->QueryInterface( IID_IPersistFile, (LPVOID*)&ppf );

	if ( SUCCEEDED(hres) )
	{
	    WCHAR wsz[MAX_PATH];
	    hres = MultiByteToWideChar( CP_ACP, 0, lpszPathLink,
					-1, wsz, MAX_PATH);
	    hres = ppf->Save( wsz, true );
	    ppf->Release();
	}

	psl->Release();
    }

    return SUCCEEDED(hres);
}



#endif


class uiSetupDlg : public uiDialog
{
public:
			    uiSetupDlg( uiParent* );
			    ~uiSetupDlg(){}

protected:

    bool		    acceptOK(CallBacker*);
    bool		    checIndexFile();
    bool		    installPackages();
    bool		    finaliseInstallation();
    bool		    createDesktopIcon(const BufferString&);
    bool		    createStartMenuLinks(const BufferString& fnm,
						const BufferString& version,
						const char* desc );
    bool		    makeUninstScript(const BufferString&);
    uiFileInput*	    basedirfld_;
    BufferStringSet	    packagenames_;
    BufferStringSet	    packagelist_;
    BufferString	    basedir_;
    BufferString	    desktoplnk_;
    BufferString	    startmenulnk_;
    BufferString	    version_;
    BufferString	    errmsg_;
};


uiSetupDlg::uiSetupDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("OpendTect Setup","Installation Setup",mNoHelpID)
	         .okcancelrev(true))
{
    setOkText( "Go" );
    FilePath fp( GetSpecialFolderLocation( CSIDL_PROGRAM_FILES ) );
    fp.add( "OpendTect" );
    uiFileInput::Setup su( fp.fullPath() ); su.forread( false )
			    .directories( true );
    basedirfld_ = new uiFileInput( this, "Installation location", su );
}


bool uiSetupDlg::finaliseInstallation()
{
    bool res = false;
#ifdef __win__
    res = createDesktopIcon( version_ );
    BufferString desc( "OpendTect ", version_ );
    res &= createStartMenuLinks( "od_start_dtect.exe", version_, desc );
    makeUninstScript( version_ );
    //res &= createStartMenuLinks( "od_runinst.exe", version_, "Installation Manager" );
    FilePath lmfp( basedir_, version_, "bin",
				  __is32bits__ ? "win32" : "win64", "lm.dgb"  );
    if ( File::exists(lmfp.fullPath()) )
	createStartMenuLinks( "lmtools.exe", version_,
			    "License Manager Tools" );

    res &= createStartMenuLinks( "od_remote_service_manager.exe", version_, 
			  "Remote Processing Service" );
    if ( !res )
    {
	errmsg_ = "Could not create Desktop Icons, "
		  "but you can still run the program from its"
		  " installation directory";
	return false;
    }
#else
    // TODO
#endif
    return true;
}


bool uiSetupDlg::createDesktopIcon( const BufferString& version )
{
    BufferString desc( "OpendTect ", version.buf() );
    BufferString desktoplnk( desc );
    desktoplnk += ".lnk";
    FilePath link( GetSpecialFolderLocation(CSIDL_COMMON_DESKTOPDIRECTORY),
	desktoplnk.buf() );
    desktoplnk_ = link.fullPath();
    FilePath work( basedir_, version_, "bin", __is32bits__ ? "win32" : "win64" );
    FilePath binrelfp( work.fullPath(), "Release" );
    FilePath bin;
    if ( File::exists(binrelfp.fullPath()) )
	bin = FilePath( binrelfp.fullPath(), "od_start_dtect.exe" );
    else
	bin = FilePath( work.fullPath(), "od_start_dtect.exe" );
    
    return CreateLink( bin.fullPath(), work.fullPath(),
			link.fullPath(), desc.buf() );
}


bool uiSetupDlg::createStartMenuLinks( const BufferString& fnm,
					const BufferString& version,
					 const char* desc )
{
    FilePath startmenu( GetSpecialFolderLocation(CSIDL_COMMON_PROGRAMS),
			"OpendTect" );
    if ( !File::exists(startmenu.fullPath()) )
    {
	if ( !File::createDir(startmenu.fullPath()) )
	   return false;
    }

    startmenu.add( version.buf() ); 
    if ( !File::exists(startmenu.fullPath()) )
    {
	if ( !File::createDir(startmenu.fullPath()) )
	    return false;
    }

    if ( fnm == "uninst.bat" )
    {
	FilePath bin( basedir_ );
	BufferString batfile( "uninst", version );
	batfile.add( ".bat" );
	bin.add( batfile );
	BufferString linkname( desc );
	linkname += ".lnk";
	startmenu.add( linkname.buf() );
	startmenulnk_ = startmenu.fullPath();
	CreateLink( bin.fullPath(), bin.pathOnly(),
	    startmenu.fullPath(), "Uninstall OpendTect" );
	return true;
    }

    FilePath bin( basedir_, version, "bin", __is32bits__ ? "win32" : "win64" );
    FilePath binrelfp( bin.fullPath(), "Release" );
    const bool isrsmorlmtools = fnm == "od_remote_service_manager.exe" 
			     || fnm == "lmtools.exe";
    if ( File::exists(binrelfp.fullPath()) && !isrsmorlmtools  )
	bin = binrelfp;

    if ( fnm == "lmtools.exe" )
	bin.add( "lm.dgb" );

    if ( fnm == "od_remote_service_manager.exe" )
	bin.add( "rsm" );

    bin.add( fnm );
    BufferString linkname( desc );
    linkname += ".lnk";
    startmenu.add( linkname.buf() );
    startmenulnk_ = startmenu.fullPath();
    CreateLink( bin.fullPath(), bin.pathOnly(), startmenu.fullPath(), desc );
    return true;
}


bool uiSetupDlg::makeUninstScript( const BufferString& version )
{
    FilePath startmenu( GetSpecialFolderLocation(CSIDL_COMMON_PROGRAMS),
			"OpendTect-", version );
    BufferString filenm( "uninst", version );
    filenm += ".bat";
    FilePath batchpath( basedir_, filenm.buf() );
    BufferString uninstscript = batchpath.fullPath();
    StreamData sd = StreamProvider( uninstscript ).makeOStream();
    BufferString batchheader( "@echo off\necho You need sufficient"
			      " administrative rights"
			      " to perform this operation. Please run this as"
			      " \"Run as Adminstrator\" to completely remove"
			      " the installation\n\n"
			      "SET /p param=Are you sure ? y/n:\nif %param% == y "
			      "goto REMOVE\ngoto END\n:REMOVE" );
    BufferString path = FilePath( basedir_, version ).fullPath();
    BufferString linkpath = FilePath( startmenulnk_ ).pathOnly();
    *sd.ostrm << batchheader << std::endl;
    *sd.ostrm << "rd /S /Q " << "\"" << path << "\"" << std::endl;
    *sd.ostrm << "rd /S /Q " << "\"" << linkpath << "\"" << std::endl;
    *sd.ostrm << "del " << "\"" << desktoplnk_ << "\"" << std::endl;
    *sd.ostrm << "del " << "\"" << uninstscript << "\"" << std::endl;
    *sd.ostrm << ":END" << std::endl;
    sd.close();
    BufferString desc( "Uninstall OpendTect " );
    desc += version;
    return createStartMenuLinks( "uninst.bat", version, desc );
}

#define mErrRet(msg) \
    { uiMSG().error( msg ); \
      button( uiDialog::CANCEL )->setSensitive( true );\
    return false; } \

bool uiSetupDlg::acceptOK( CallBacker* )
{
    button( uiDialog::CANCEL )->setSensitive( false );
    if ( !checIndexFile() )
	return false;
    
    if ( installPackages() )
    {
	if ( !finaliseInstallation() )
	    uiMSG().warning( errmsg_ );
	uiMSG().message( "Installation successfull" );
    }
    else
	 mErrRet( errmsg_ );
    button( uiDialog::CANCEL )->setSensitive( true );
    return true;
}
	

bool uiSetupDlg::checIndexFile()
{
    const BufferString idxfile( "packageidx.txt" );
    if ( !File::exists(idxfile) )
	mErrRet( "No Index file found. Please re-download packages using the\n"
	" \"Offline installation\" button from the installation manager"  );

    IOPar pkgpar;
    pkgpar.read( idxfile, "idxfile" );
    pkgpar.get( "Version", version_ );
    pkgpar.get( "Packages", packagenames_ );

    if ( packagenames_.isEmpty() )
	mErrRet( "Index file corrupt, try to re-download packages" ) 

    for ( int idx=0; idx<packagenames_.size(); idx++ )
    {
	FilePath pkgfp( "packages", packagenames_.get(idx) );
	if ( !File::exists(pkgfp.fullPath()) )
	{
	    BufferString err( pkgfp.fullPath(), " package is missing" );
	    mErrRet( err );
	}
	
	packagelist_.add( pkgfp.fullPath() );
    }

    return true;
}


bool uiSetupDlg::installPackages()
{
    if ( !packagelist_.size() )
	mErrRet( "package list empty" )
	
    basedir_ = basedirfld_->fileName();
    FilePath basefp( basedir_, version_ );
    if ( File::exists(basefp.fullPath()) )
    {
	BufferString msg( "A previous installation of OpendTect ", version_, 
			    " found in " );
	msg += basedir_;
	msg += ".\nPlease un-install this version and try again.";
	msg += "\nProceeding will remove the previous installation.";
	msg += "\nWould you like to continue ?"; // ask
	if ( !uiMSG().askContinue(msg) )
	    return false;
	else
	    File::removeDir( basefp.fullPath() );
    }

    if ( !File::exists(basedir_) )
	File::createDir( basedir_ );
    
    uiTaskRunner uitr( this );
    if ( !ZipUtils::unZipArchives(packagelist_,basedir_,errmsg_,&uitr) )
    {
	if ( uitr.getState() == (int) Task::Stop )
	{
	    errmsg_ = "Operation aborted by user";
	    File::removeDir( basefp.fullPath() );
	}
	return false;
    }

    FilePath work( basedir_, version_, "bin", __is32bits__ ? "win32" : "win64" );
    if ( File::exists( FilePath(work,"Release").fullPath() ) )
	work.add( "Release" );
    FilePath bin( work.fullPath(), "od_start_dtect.exe" );
    if ( !File::exists(bin.fullPath()) )
    {
	errmsg_ = "Failed to extract files. Possible causes could be :-\n\n"
	    "1) Unzip.exe could be missing from the setup folder, prepare the"
		" setup packages again\n"
	    "2) Installation could not proceed from the shared folder,"
		" copy the offline setup folder to the local drive "
		" and try again.";
	return false;
    }
    return true;
}


void runMe()
{
    executeWinProg( "od_setup.exe", "1" );
    exit(0);
}


int main( int argc, char** argv )
{
    if( argc == 1 )
	runMe();
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );
    uiSetupDlg dlg( 0 );
    app.setTopLevel( &dlg );
    dlg.go();
    ExitProgram( app.exec() );
}
