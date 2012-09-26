/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          Jan 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uicrdevenv.h"

#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uimain.h"
#include "uimsg.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "settings.h"
#include "strmprov.h"

#ifdef __win__
# include "winutils.h"
#endif

static void showProgrDoc()
{
    const FilePath fp( mGetProgrammerDocDir(),
			__iswin__ ? "windows.html" : "unix.html" );
    uiDesktopServices::openUrl( fp.fullPath() );
}

#undef mHelpFile


uiCrDevEnv::uiCrDevEnv( uiParent* p, const char* basedirnm,
			const char* workdirnm )
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
			  uiFileInput::Setup(basedirnm).directories(true) );

    workdirfld = new uiGenInput( this, "Directory name", workdirnm );
    workdirfld->attach( alignedBelow, basedirfld );

}


bool uiCrDevEnv::isOK( const char* datadir )
{
    FilePath datafp( datadir );

    if ( !datafp.nrLevels() ) return false;

    if ( !datafp.nrLevels() || !File::isDirectory( datafp.fullPath() ) )
	return false;

    datafp.add( "src" );
    if ( !File::isDirectory(datafp.fullPath()) )
	return false;

    datafp.set( datadir ).add( "include" );
    if ( !File::isDirectory(datafp.fullPath()) )
	return false;

    datafp.set( datadir ).add( "plugins" );
    if ( !File::isDirectory(datafp.fullPath()) )
	return false;

    return true;
}


#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return; }

void uiCrDevEnv::crDevEnv( uiParent* appl )
{
    BufferString swdir = GetSoftwareDir(0);
    if ( !isOK(swdir) )
    {
	uiMSG().error( "No source code found. Please download\n"
		       "and install development package first" );
	return;
    }

    FilePath oldworkdir( GetEnvVar("WORK") );
    const bool oldok = isOK( oldworkdir.fullPath() );

    BufferString workdirnm;

    if ( File::exists(oldworkdir.fullPath()) )
    {
	BufferString msg = "Your current work directory (";
	msg += oldworkdir.fullPath();
	msg += oldok ?  ") seems to be a valid work directory.\n" :
			") does not seem to be a valid work directory.\n";
	msg += "Do you want to completely remove the existing directory\n"
	       "and create a new work directory there?";

	if ( uiMSG().askGoOn(msg) )
	{
	    File::remove( workdirnm );
	    workdirnm = oldworkdir.fullPath();
	}
    }

    if ( workdirnm.isEmpty() )
    {
	BufferString worksubdirm = "ODWork";
	BufferString basedirnm = GetPersonalDir();

	// pop dialog
	uiCrDevEnv dlg( appl, basedirnm, worksubdirm );
	if ( !dlg.go() ) return;

	basedirnm = dlg.basedirfld->text();
	worksubdirm = dlg.workdirfld->text();

	if ( !File::isDirectory(basedirnm) )
	    mErrRet( "Invalid directory selected" )

	workdirnm = FilePath( basedirnm ).add( worksubdirm ).fullPath();
    }

    if ( workdirnm.isEmpty() ) return;
	
    if ( File::exists(workdirnm) )
    {
	BufferString msg;
	const bool isdir= File::isDirectory( workdirnm );
	const bool isok = isOK(workdirnm);

	if ( isdir )
	{
	    msg = "The directory you selected (";
	    msg += workdirnm;
	    msg += isok ? ")\nalready seems to be a work directory.\n\n" :
			  ")\ndoes not seem to be a valid work directory.\n\n";
	}
	else
	{
	    msg = "You selected a file.\n\n";
	}

	msg += "Do you want to completely remove the existing ";
	msg += isdir ? "directory\n" : "file\n";
	msg += "and create a new work directory there?";   

	if ( !uiMSG().askRemove(msg) )
	    return;

	File::remove( workdirnm );
    }


    if ( !File::createDir( workdirnm ) )
	mErrRet( "Cannot create the new directory.\n"
		 "Please check if you have the required write permissions" )

    const char* docmsg =
	"The OpendTect window will FREEZE during this process\n"
	"- for upto a few minutes.\n\n"
	"Meanwhile, do you want to take a look at the developers documentation?"
    ;
    if ( uiMSG().askGoOn(docmsg) )
	showProgrDoc();

    FilePath fp( swdir, "bin" );
#ifdef __win__
    BufferString cmd( "@" );
    fp.add( "od_cr_dev_env.bat" );
    cmd += fp.fullPath();
    cmd += " "; cmd += swdir;
    char shortpath[1024];
    GetShortPathName(workdirnm.buf(),shortpath,1024);
    cmd += " "; cmd += shortpath;
    StreamProvider( cmd ).executeCommand( false, true );
#else
    BufferString cmd( "@'" );
    fp.add( "od_cr_dev_env" );
    cmd += fp.fullPath();
    cmd += "' '"; cmd += swdir;
    cmd += "' '"; cmd += workdirnm; cmd += "'";
    StreamProvider( cmd ).executeCommand( false );
#endif
    
    BufferString relfile = FilePath(workdirnm).add(".rel.devel").fullPath();
    if ( !File::exists(relfile) )
	mErrRet( "Creation seems to have failed" )
    else
    {
	uiMSG().message( "Creation seems to have succeeded."
#ifndef __win__
			 "\n\nSource 'init.csh' or 'init.bash' before starting."
#endif
			);
    }
}



#undef mErrRet
#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiCrDevEnv::acceptOK( CallBacker* )
{
    BufferString workdir = basedirfld->text();
    if ( workdir.isEmpty() || !File::isDirectory(workdir) )
	mErrRet( "Please enter a valid (existing) location" )

    if ( workdirfld )
    {
	BufferString workdirnm = workdirfld->text();
	if ( workdirnm.isEmpty() )
	    mErrRet( "Please enter a (sub-)directory name" )

	workdir = FilePath( workdir ).add( workdirnm ).fullPath();
    }

    if ( !File::exists(workdir) )
    {
#ifdef __win__
	if ( !strncasecmp("C:\\Program Files", workdir, 16)
	  || strstr( workdir, "Program Files" )
	  || strstr( workdir, "program files" )
	  || strstr( workdir, "PROGRAM FILES" ) )
	    mErrRet( "Please do not use 'Program Files'.\n"
		     "A directory like 'My Documents' would be good." )
#endif
    }

    return true;
}
