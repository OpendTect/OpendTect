/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          Jan 2004
 RCS:           $Id: uicrdevenv.cc,v 1.13 2004-01-27 13:47:27 dgb Exp $
________________________________________________________________________

-*/

#include "uicrdevenv.h"
#include "uifileinput.h"
#include "helpview.h"
#include "uimsg.h"
#include "ioman.h"
#include "settings.h"
#include "filegen.h"
#include "strmprov.h"
#include "uimain.h"


extern "C" { const char* GetBaseDataDir(); }

#ifdef __win__

#include <windows.h>
#include <regstr.h>
#include <ctype.h>
#include <winreg.h>
#include <iostream>


const char* getRegKey( HKEY hKeyRoot, const char* subkey, const char* Value )
{
    static BufferString answer;

    BYTE* Value_data = new BYTE[256];
    DWORD Value_size = 256;

    HKEY hKeyNew=0;
    DWORD retcode=0;
    DWORD Value_type=0;


    retcode = RegOpenKeyEx ( hKeyRoot, subkey, 0, KEY_QUERY_VALUE, &hKeyNew);
    if (retcode != ERROR_SUCCESS) return 0;


    retcode = RegQueryValueEx( hKeyNew, Value, NULL, &Value_type, Value_data,
                               &Value_size);
    if (retcode != ERROR_SUCCESS) return 0;


    answer = (const char*) Value_data;

    return answer;
}

const char* getCygDir()
{
    const char* cygdir = getRegKey( HKEY_LOCAL_MACHINE, 
			"SOFTWARE\\Cygnus Solutions\\Cygwin\\mounts v2\\/",
			"native" );

    if ( !File_isDirectory(cygdir) )
    {
	const char* cygdir = getRegKey( HKEY_CURRENT_USER, 
			    "Software\\Cygnus Solutions\\Cygwin\\mounts v2\\/",
			    "native" );
    }

    if ( !File_isDirectory(cygdir) ) return 0;

    BufferString bindir = File_getFullPath(cygdir,"bin");
    if ( !File_isDirectory(bindir) ) return 0;

    return cygdir;
}

#endif

static void showProgrDoc()
{


#ifdef __win__

    BufferString getstarted = File_getFullPath( "dTectDoc", "Programmer" );
    getstarted = File_getFullPath( getstarted, "windows.html");

    BufferString docpath( GetDataFileName(getstarted) );

    ShellExecute(NULL,"open",docpath,NULL,NULL,SW_NORMAL);

#else

    BufferString getstarted = File_getFullPath( "dTectDoc", "Programmer" );
    getstarted = File_getFullPath( getstarted, "unix.html");
    HelpViewer::doHelp( getstarted, 
		    "Get started with OpendTect development" );
#endif
}



uiCrDevEnv::uiCrDevEnv( uiParent* p, const char* basedirnm,
			const char* workdirnm, const char* cygwin )
	: uiDialog(p,uiDialog::Setup("Create Work Enviroment",
		    		     "Specify a work directory",
		    		     "8.0.1"))
	, workdirfld(0)
	, basedirfld(0)
{

    const char* titltxt =
    "For OpendTect development you'll need a $WORK dir\n"
    "Please specify where this directory should be created.";

    setTitleText( titltxt );

    basedirfld = new uiFileInput( this, "Parent Directory",
				  uiFileInput::Setup(basedirnm).directories() );

    workdirfld = new uiGenInput( this, "Directory name", workdirnm );
    workdirfld->attach( alignedBelow, basedirfld );

}


bool uiCrDevEnv::isOK( const char* d )
{
    if ( !d || !*d ) return false; 

    const BufferString datadir( (d && *d) ? d : getenv("WORK") );
    const BufferString pmakedir( File_getFullPath( datadir, "Pmake" ) );
    const BufferString incdir( File_getFullPath( datadir, "include" ) );
    const BufferString srcdir( File_getFullPath( datadir, "src" ) );
    const BufferString plugindir( File_getFullPath( datadir, "plugins" ) );
    return File_isDirectory( datadir )
        && File_isDirectory( pmakedir )
        && File_isDirectory( incdir )
        && File_isDirectory( srcdir )
        && File_isDirectory( plugindir );
}


#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return; }

void uiCrDevEnv::crDevEnv( uiParent* appl )
{

    BufferString oldworkdir(getenv("WORK"));
    const bool oldok = isOK( oldworkdir );

    const char* cygwin = 0;

#ifdef __win__

    cygwin = getCygDir();

    if( !cygwin )
    {
	const char* msg =
	    "Cygwin installation not found."
	    "Please close OpendTect and use the Cygwin installer\n"
	    "you can find in the start-menu under"
	    "\"Start-Programs-OpendTect-Install Cygwin\""
	    "\nIf you are sure you have cygwin installed, "
	    "you can safely continue."
	    "\n\nDo you want to continue anyway?";

	if ( ! uiMSG().askGoOn(msg) )
	{
	    const char* closemsg = 
		"Please run the Cygwin (bash) shell "
		"at least once after the installation.\n\n"
		"This will make sure you have a Cygwin home "
		"directory for your work environment.\n\n"
		"Please refer to the documentation that will pop up to "
		"see which packages to install." 
		"\n\nIt is required to close OpendTect before installing"
		"Cygwin.\nDo you want to close OpendTect now?";

	    bool close = uiMSG().askGoOn( closemsg );

	    showProgrDoc();

	    if ( close )
	    {
		if( uiMain::theMain().topLevel() )
		   uiMain::theMain().topLevel()->close();
		else
		   uiMain::theMain().exit();
	    }

	    return;
	}
    }

#endif

    BufferString workdirnm;

    if ( oldworkdir != "" )
    {
	BufferString msg = "Your current work directory (";
	msg += oldworkdir;
	msg += oldok ?  ") seems to be Ok.\n" :
			") does not seem to be a valid work directory.\n";
	msg += "Do you want to completely remove the existing directory\n"
	       "and create a new work directory there?";

	if ( uiMSG().askGoOn(msg) )
	{
	    File_remove( workdirnm, true );
	    workdirnm = oldworkdir;
	}
    }

    if ( workdirnm == "" )
    {
	BufferString worksubdirm = "ODWork";

	BufferString basedirnm = GetPersonalDir();

	if ( cygwin )
	{
	    basedirnm = File_getFullPath( cygwin, "home" );

	    if ( getenv("USERNAME") )
		basedirnm = File_getFullPath( basedirnm, getenv("USERNAME") );
	    else if ( getenv("USER") )
		basedirnm = File_getFullPath( basedirnm, getenv("USER") );

	    if ( !File_isDirectory(basedirnm) )
	    {
		const char* msg =
		"You have installed Cygwin but you have never used it.\n"
		"Unfortunately, this means you have no Cygwin home directory.\n"
		"\nWe advise to close OpendTect and start a Cygwin shell.\n"
		"Then use this utility again.\n"
		"\nDo you still wish to continue?";

		if ( !uiMSG().askGoOn(msg) )
		    return;

		basedirnm = "C:\\";
	    }
	}

	// pop dialog
	uiCrDevEnv dlg( appl, basedirnm, worksubdirm, cygwin );
	if ( !dlg.go() ) return;

	basedirnm = dlg.basedirfld->text();
	worksubdirm = dlg.workdirfld->text();

	if ( !File_isDirectory(basedirnm) )
	    mErrRet( "Invalid directory selected" )

	workdirnm = File_getFullPath( basedirnm, worksubdirm );
    }

    if ( workdirnm == "" ) return;
	
    if ( File_exists(workdirnm) )
    {
	BufferString msg;
	const bool isdir= File_isDirectory( workdirnm );
	const bool isok = isOK(workdirnm);

	if ( isdir )
	{
	    msg = "The directory you selected(";
	    msg += workdirnm;
	    msg += isok ? ") seems to be a valid work directory.\n\n" :
			  ") does not seem to be a valid work directory.\n\n";
	}
	else
	{
	    msg = "You selected a file.\n\n";
	}

	msg += "Do you want to completely remove the existing";
	msg + isdir ?  "directory\n" : "file\n" ;
	msg += "and create a new work directory there?";   

	if ( !uiMSG().askGoOn(msg) )
	    return;

	File_remove( workdirnm, true );
    }


    if ( !File_createDir( workdirnm, 0 ) )
	mErrRet( "Cannot create the new directory.\n"
		 "Please check if you have the required write permissions" )

    const char* aboutto =
	"The OpendTect window will FREEZE during this process\n"
	"- for upto a few minutes.\n\n"
	"Meanwhile, please take a look at the developers documentation."
    ;

    uiMSG().message(aboutto);

    showProgrDoc();

    BufferString cmd( "@'" );
    cmd += GetSoftwareDir();
    cmd = File_getFullPath( cmd, "bin" );
    cmd = File_getFullPath( cmd, "od_cr_dev_env" );
    cmd += "' '"; cmd += GetSoftwareDir();
    cmd += "' '"; cmd += workdirnm; cmd += "'";

    StreamProvider( cmd ).executeCommand( false );

    BufferString relfile = File_getFullPath( workdirnm, ".rel.od.doc" );
    if ( !File_exists(relfile) )
	mErrRet( "Creation seems to have failed" )
    else
	uiMSG().message( "Creation seems to have succeeded.\n\n"
			 "Source 'init.csh' or 'init.bash' before starting." );
}



#undef mErrRet
#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiCrDevEnv::acceptOK( CallBacker* )
{
    BufferString workdir = basedirfld->text();
    if ( workdir == "" || !File_isDirectory(workdir) )
	mErrRet( "Please enter a valid (existing) location" )

    if ( workdirfld )
    {
	BufferString workdirnm = workdirfld->text();
	if ( workdirnm == "" )
	    mErrRet( "Please enter a (sub-)directory name" )

	workdir = File_getFullPath( workdir, workdirnm );
    }


    const BufferString omffnm = File_getFullPath( workdir, ".omf" );
    const BufferString stdomf( GetDataFileName("omf") );

    if ( !File_exists(workdir) )
    {
#ifdef __win__
	if ( !strncasecmp("C:\\Program Files", workdir, 16)
	  || strstr( workdir, "Program Files" )
	  || strstr( workdir, "program files" )
	  || strstr( workdir, "PROGRAM FILES" ) )
	    mErrRet( "Please do not try to use 'Program Files' for data.\n"
		     "A directory like 'My Documents' would be good." )
#endif
    }

    return true;
}
