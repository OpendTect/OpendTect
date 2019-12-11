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
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "settings.h"
#include "oscommand.h"
#include "od_helpids.h"

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
    : uiDialog(p,uiDialog::Setup(tr("Create Development Environment"),
				 mNoDlgTitle,mODHelpKey(mSetDataDirHelpID)))
{
    uiLabel* lbl = new uiLabel( this,
	tr("Specify OpendTect plugin development location.\n") );
    lbl->attach( leftBorder );

    workdirfld = new uiGenInput( this, uiStrings::sName(), workdirnm );
    workdirfld->attach( ensureBelow, lbl );

    basedirfld = new uiFileInput( this, tr("Create in"),
			uiFileInput::Setup(basedirnm).directories(true) );
    basedirfld->attach( alignedBelow, workdirfld );
}


bool uiCrDevEnv::isOK( const char* datadir )
{
#ifdef __mac__
    FilePath datafp( datadir, "Resources" );
#else
    FilePath datafp( datadir );
#endif

    if ( !datafp.nrLevels() ) return false;

    if ( !datafp.nrLevels() || !File::isDirectory( datafp.fullPath() ) )
	return false;

    datafp.add( "CMakeLists.txt" );
    if ( !File::exists(datafp.fullPath()) )
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
	uiMSG().error(tr("No source code found. Please download\n"
			 "and install development package first"));
	return;
    }

    FilePath oldworkdir( GetEnvVar("WORK") );
    const bool oldok = isOK( oldworkdir.fullPath() );

    BufferString workdirnm;

    if ( File::exists(oldworkdir.fullPath()) )
    {
	uiString msg = tr("Your current development folder (%1) %2 to be "
			  "a valid environment."
			  "\n\nDo you want to completely remove "
			  "the existing folder "
			  "and create a new folder there?")
		     .arg(oldworkdir.fullPath())
		     .arg(oldok ? tr("seems")
				: tr("does not seem"));

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
	    mErrRet(tr("Invalid folder selected"))

	workdirnm = FilePath( basedirnm ).add( worksubdirm ).fullPath();
    }

    if ( workdirnm.isEmpty() ) return;

    if ( File::exists(workdirnm) )
    {
	uiString msg;
	const bool isdir= File::isDirectory( workdirnm );

	if ( isdir )
	{
	    msg = tr("The folder you selected (%1)\nalready exists.\n\n")
		.arg(workdirnm);
	}
	else
	{
	    msg = tr("You selected a file.\n\n");
	}

	msg.append("Do you want to completely remove the existing %1\n"
		"and create a new development location there?")
	    .arg(isdir ? tr("folder") : tr("file"));

	if ( !uiMSG().askRemove(msg) )
	    return;

	File::remove( workdirnm );
    }


    if ( !File::createDir( workdirnm ) )
	mErrRet( uiStrings::phrCannotCreateDirectory(toUiString(workdirnm)) )

    const uiString docmsg =
      tr( "Do you want to take a look at the developers documentation?");
    if ( uiMSG().askGoOn(docmsg) )
	showProgrDoc();

#ifdef __mac__
    FilePath fp( swdir, "Resources" );
#else
    FilePath fp( swdir, "bin" );
#endif

    fp.add( "od_cr_dev_env" );
    BufferString cmd;

#ifdef __win__
    fp.setExtension(  "bat" );
#endif

    cmd.add( fp.fullPath() );
    OS::CommandLauncher::addQuotesIfNeeded( cmd );
    OS::CommandLauncher::addQuotesIfNeeded( swdir );

    cmd.addSpace().add( swdir );

#ifdef __win__
    char shortpath[1024];
    GetShortPathName( workdirnm.buf(), shortpath, 1024 );
    workdirnm = shortpath;
#endif

    const BufferString workdirorig( workdirnm );//Need to keeping
	//old format of path to check File::exists() later without quotes

    OS::CommandLauncher::addQuotesIfNeeded( workdirnm );
    cmd.addSpace().add( workdirnm );

    uiMSG().message( cmd );
    const OS::MachineCommand mc( cmd );
    OS::CommandLauncher cl( mc );
    BufferString outmsg, errormsg;
    const bool res = cl.execute(  outmsg, &errormsg );
    if ( !res )
    {
	BufferString msg( "Failed to create Environment " );
	if ( !outmsg.isEmpty() )
	    msg.add( outmsg );

	if ( !errormsg.isEmpty() )
	    msg.add( errormsg );
    }

    const BufferString cmakefile =
			FilePath(workdirorig).add("CMakeLists.txt").fullPath();
    if ( !File::exists(cmakefile) )
	mErrRet(tr("Creation seems to have failed"))
    else
	uiMSG().message( tr("Creation seems to have succeeded.") );
}



#undef mErrRet
#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiCrDevEnv::acceptOK( CallBacker* )
{
    BufferString workdir = basedirfld->text();
    if ( workdir.isEmpty() || !File::isDirectory(workdir) )
	mErrRet( tr("Please enter a valid (existing) location") )

    if ( workdirfld )
    {
	BufferString workdirnm = workdirfld->text();
	if ( workdirnm.isEmpty() )
	    mErrRet( tr("Please enter a (sub-)folder name") )

	workdir = FilePath( workdir ).add( workdirnm ).fullPath();
    }

    if ( !File::exists(workdir) )
    {
#ifdef __win__
	if ( workdir.contains( "Program Files" )
	  || workdir.contains( "program files" )
	  || workdir.contains( "PROGRAM FILES" ) )
	  mErrRet(tr("Please do not use 'Program Files'.\n"
		     "Instead, a folder like 'My Documents' would be OK."))
#endif
    }

    return true;
}
