/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          Jan 2004
________________________________________________________________________

-*/

#include "uicrdevenv.h"

#include "uidesktopservices.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uimain.h"
#include "uimsg.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "oscommand.h"
#include "od_helpids.h"


static void showProgrDoc()
{
    const File::Path fp( mGetProgrammerDocDir(),
			__iswin__ ? "windows.html" : "unix.html" );
    uiDesktopServices::openUrl( fp.fullPath() );
}

#undef mHelpFile


uiCrDevEnv::uiCrDevEnv( uiParent* p, const char* basedirnm,
			const char* workdirnm )
	: uiDialog(p,uiDialog::Setup(tr("Create Work Enviroment"),
				     tr("Specify a work directory"),
				     mODHelpKey(mSetDataDirHelpID) ))
	, workdirfld(0)
	, basedirfld(0)
{
    const uiString titltxt =
	tr("For OpendTect development you'll need a %1 dir\n"
        "Please specify where this directory should be created.")
	.arg(toUiString("$WORK"));

    setTitleText( titltxt );

    uiFileSel::Setup fssu( basedirnm );
    fssu.selectDirectory();
    basedirfld = new uiFileSel( this, tr("Parent Directory"), fssu );

    workdirfld = new uiGenInput( this, tr("Directory name %1")
					    .arg(toUiString(workdirnm)) );
    workdirfld->attach( alignedBelow, basedirfld );

}


bool uiCrDevEnv::isOK( const char* datadir )
{
    File::Path datafp( datadir );

    if ( !datafp.nrLevels() ) return false;

    if ( !datafp.nrLevels() || !File::isDirectory( datafp.fullPath() ) )
	return false;

    datafp.add( "CMakeLists.txt" );
    if ( !File::exists(datafp.fullPath()) )
	return false;

    return true;
}


#define mUiMsg() gUiMsg(appl)
#define mErrRetRv(s,rv) { mUiMsg().error(s); return rv; }
#undef mErrRet
#define mErrRet(s) mErrRetRv(s,)

void uiCrDevEnv::crDevEnv( uiParent* appl )
{
    BufferString swdir = GetSoftwareDir(0);
    if ( !isOK(swdir) )
	mErrRet( tr("No source code found. Please download\n"
                         "and install development package first") )

    File::Path oldworkdir( GetEnvVar("WORK") );
    const bool oldok = isOK( oldworkdir.fullPath() );

    BufferString workdirnm;

    if ( File::exists(oldworkdir.fullPath()) )
    {
	uiString msg = tr("Current work directory:\n%1\na valid work dir? [%2]"
			  "\n\nDo you want to completely remove "
			  "the existing directory "
			  "and create a new work directory there?")
		     .arg(oldworkdir.fullPath())
		     .arg(oldok ? uiStrings::sYes() : uiStrings::sNo() );

	if ( mUiMsg().askGoOn(msg) )
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
	    mErrRet(tr("Invalid directory selected"))

	workdirnm = File::Path( basedirnm ).add( worksubdirm ).fullPath();
    }

    if ( workdirnm.isEmpty() ) return;

    if ( File::exists(workdirnm) )
    {
	uiString msg;
	const bool isdir= File::isDirectory( workdirnm );

	if ( isdir )
	{
	    msg = tr("The directory you selected (%1)\nalready exists.\n\n")
		.arg(workdirnm);
	}
	else
	{
	    msg = tr("You selected a file.\n\n");
	}

	msg.arg("Do you want to completely remove the existing %1\n"
		"and create a new work directory there?")
	    .arg(isdir ? uiStrings::sDirectory().toLower()
		       : uiStrings::sDirectory().toLower());

	if ( !mUiMsg().askRemove(msg) )
	    return;

	File::remove( workdirnm );
    }


    if ( !File::createDir( workdirnm ) )
	mErrRet( uiStrings::phrCannotCreateDirectory(workdirnm) )

    const uiString docmsg =
      tr("The OpendTect window will FREEZE during this process\n"
      "- for upto a few minutes.\n\n"
      "Meanwhile, do you want to take a look at the developers documentation?");

    if ( mUiMsg().askGoOn(docmsg) )
	showProgrDoc();

#ifdef __win__
    char shortpath[1024];
    GetShortPathName( workdirnm.str(), shortpath, 1024 );
    workdirnm = shortpath;
#endif

    const char* scriptfnm = __iswin__ ? "od_cr_dev_env.bat" : "od_cr_dev_env";
    File::Path fp( swdir, "bin", scriptfnm );
    OS::MachineCommand mc( fp.fullPath() );
    mc.addArg( swdir );
    mc.addArg( workdirnm );
    BufferString outmsg, errormsg, msgstr;
    const bool res = mc.execute(  outmsg, &errormsg );
    if ( !res )
    {
        msgstr.set( "Failed to create Environment " );
        if ( !outmsg.isEmpty() )
            msgstr.add( outmsg );

        if ( !errormsg.isEmpty() )
            msgstr.add( errormsg );
    }

    const BufferString cmakefile =
			File::Path(workdirnm).add("CMakeLists.txt").fullPath();
    if ( !File::exists(cmakefile) )
	mErrRet(tr("Creation seems to have failed:\n%1").arg(msgstr))
    else
	mUiMsg().message( tr("Creation seems to have succeeded.") );
}



#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiCrDevEnv::acceptOK()
{
    BufferString workdir = basedirfld->text();
    if ( workdir.isEmpty() || !File::isDirectory(workdir) )
	mErrRet( uiStrings::phrEnter(tr("a valid (existing) location")) )

    if ( workdirfld )
    {
	BufferString workdirnm = workdirfld->text();
	if ( workdirnm.isEmpty() )
	    mErrRet( uiStrings::phrEnter(tr("a (sub-)directory name")) )

	workdir = File::Path( workdir ).add( workdirnm ).fullPath();
    }

    if ( !File::exists(workdir) )
    {
#ifdef __win__
	if ( workdir.contains( "Program Files" )
	  || workdir.contains( "program files" )
	  || workdir.contains( "PROGRAM FILES" ) )
	  mErrRet(tr("Do not use 'Program Files'.\n"
		     "Instead, a directory like 'My Documents' would be OK."))
#endif
    }

    return true;
}
