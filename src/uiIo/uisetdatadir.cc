/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisetdatadir.h"

#include "uifileinput.h"
#include "uifiledlg.h"
#include "uitaskrunner.h"
#include "uiselsimple.h"
#include "uibutton.h"
#include "uidesktopservices.h"
#include "uisurveyzip.h"
#include "uimsg.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "dirlist.h"
#include "oddirs.h"
#include "ioman.h"
#include "odinst.h"
#include "settings.h"
#include "ziputils.h"

#include <stdlib.h>

#ifdef __win__
# include "winutils.h"
#endif


extern "C" { mGlobal(Basic) void SetCurBaseDataDir(const char*); }

static const char* doSetRootDataDir( const char* inpdatadir )
{
    BufferString datadir = inpdatadir;

    if ( !IOMan::isValidDataRoot(datadir) )
	return "Provided directory name is not a valid OpendTect root data dir";

    SetCurBaseDataDir( datadir );

    Settings::common().set( "Default DATA directory", datadir );
    return Settings::common().write() ? 0 : "Cannot write user settings file";
}


uiSetDataDir::uiSetDataDir( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Set Data Directory",
		    		     "Specify a data storage directory",
		    		     "8.0.1"))
	, curdatadir_(GetBaseDataDir())
{
    const bool oldok = IOMan::isValidDataRoot( curdatadir_ );
    BufferString oddirnm, basedirnm;
    const char* titletxt = 0;

    if ( !curdatadir_.isEmpty() )
    {
	if ( oldok )
	{
	    titletxt =	"Locate an OpendTect Data Root directory\n"
			"or specify a new directory name to create";
	    basedirnm = curdatadir_;
	}
	else
	{
	    titletxt =	"OpendTect needs a place to store your data files.\n"
			"\nThe current OpendTect Data Root is invalid.\n"
			"* Locate a valid data root directory\n"
			"* Or specify a new directory name to create";

	    FilePath fp( curdatadir_ );
	    oddirnm = fp.fileName();
	    basedirnm = fp.pathOnly();
	}
    }
    else
    {
	titletxt =
	    "OpendTect needs a place to store your data files:"
	    " the OpendTect Data Root.\n\n"
	    "You have not yet specified a location for it,\n"
	    "and there is no 'DTECT_DATA' set in your environment.\n\n"
	    "Please specify where the OpendTect Data Root should\n"
	    "be created or select an existing OpendTect Data Root.\n"
#ifndef __win__
	    "\nNote that you can still put surveys and "
	    "individual cubes on other disks;\nbut this is where the "
	    "'base' data store will be."
#endif
	    ;
	oddirnm = "ODData";
	basedirnm = GetPersonalDir();
    }
    setTitleText( titletxt );

    const char* basetxt = "OpendTect Data Root Directory";
    basedirfld_ = new uiFileInput( this, basetxt,
			      uiFileInput::Setup(uiFileDialog::Gen,basedirnm)
			      .directories(true) );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiSetDataDir::acceptOK( CallBacker* )
{
    seldir_ = basedirfld_->text();
    if ( seldir_.isEmpty() || !File::isDirectory(seldir_) )
	mErrRet( "Please enter a valid (existing) location" )

    if ( seldir_ == curdatadir_ && IOMan::isValidDataRoot(seldir_) )
	return true;

    FilePath fpdd( seldir_ ); FilePath fps( GetSoftwareDir(0) );
    const int nrslvls = fps.nrLevels();
    if ( fpdd.nrLevels() >= nrslvls )
    {
	const BufferString ddatslvl( fpdd.dirUpTo(nrslvls-1) );
	if ( ddatslvl == fps.fullPath() )
	{
	    uiMSG().error( "The directory you have chosen is"
		   "\n *INSIDE*\nthe software installation directory."
		   "\nThis leads to many problems, and we cannot support this."
		   "\n\nPlease choose another directory" );
	    return false;
	}
    }

    return true;
}


static BufferString getInstalledDemoSurvey()
{
    BufferString ret;
    if ( ODInst::getPkgVersion("demosurvey") )
    {
	FilePath demosurvfp( GetSoftwareDir(0), "data", "DemoSurveys",
			     "F3_Start.zip" );
	ret = demosurvfp.fullPath();
    }

    if ( !File::exists(ret) )
	ret.setEmpty();

    return ret;
}


bool uiSetDataDir::setRootDataDir( uiParent* par, const char* inpdatadir )
{
    BufferString datadir = inpdatadir;
    const char* retmsg = doSetRootDataDir( datadir );
    if ( !retmsg ) return true;

    const BufferString stdomf( mGetSetupFileName("omf") );

#define mCrOmfFname FilePath( datadir ).add( ".omf" ).fullPath()
    BufferString omffnm = mCrOmfFname;
    bool offerunzipsurv = false;

    if ( !File::exists(datadir) )
    {
#ifdef __win__
	BufferString progfiles=GetSpecialFolderLocation(CSIDL_PROGRAM_FILES);

	if ( ( progfiles.size() && 
	       !strncasecmp(progfiles, datadir, strlen(progfiles)) )
	  || strstr( datadir, "Program Files" )
	  || strstr( datadir, "program files" )
	  || strstr( datadir, "PROGRAM FILES" ) )
	    mErrRet( "Please do not try to use 'Program Files' for data.\n"
		     "A directory like 'My Documents' would be good." )
#endif
	if ( !File::createDir( datadir ) )
	    mErrRet( "Cannot create the new directory.\n"
		     "Please check if you have the required write permissions" )
    }

    while ( !IOMan::isValidDataRoot(datadir) )
    {
	if ( !File::isDirectory(datadir) )
	    mErrRet( "A file (not a directory) with this name already exists" )

	if ( File::exists(omffnm) )
	{
	    // must be a survey directory (see IOMan::isValidDataRoot())
	    datadir = FilePath(datadir).pathOnly();
	    omffnm = mCrOmfFname;
	    offerunzipsurv = false;
	    continue;
	}

	offerunzipsurv = true;
	if ( !DirList(datadir).isEmpty() )
	{
	    BufferString msg( "The target directory:\n", datadir,
		    "\nis not an OpendTect Data Root directory."
		    "\nIt already contains files though."
		    "\nDo you want to convert this directory into an "
		    "OpendTect Data Root directory?"
		    "\n(this process will not remove the existing files)" );
	    if ( !uiMSG().askGoOn( msg ) )
		return false;
	}

	File::copy( stdomf, omffnm );
	if ( !File::exists(omffnm) )
	    mErrRet( BufferString("Could not convert the directory.\n"
	     "Most probably you have no write permissions for:\n",datadir) )

	break;
    }

    if ( offerunzipsurv )
	offerUnzipSurv( par, datadir );

    retmsg = doSetRootDataDir( datadir );
    if ( retmsg )
	{ uiMSG().error( retmsg ); return false; }

    return true;
}


void uiSetDataDir::offerUnzipSurv( uiParent* par, const char* datadir )
{
    if ( !par ) return;

    BufferString zipfilenm = getInstalledDemoSurvey();
    const bool havedemosurv = !zipfilenm.isEmpty();
    BufferStringSet opts;
    opts.add( "&I will set up a new survey myself" );
    if ( havedemosurv )
	opts.add("Install the F3 &Demo Survey from the OpendTect installation");
    opts.add( "&Unzip a survey zip file" );

    struct OSRPageShower : public CallBacker
    {
	void go( CallBacker* )
	{
	    uiDesktopServices::openUrl( "https://opendtect.org/osr" );
	}
    };
    uiGetChoice uigc( par, opts, "Please select next action" );
    OSRPageShower ps;
    uiPushButton* pb = new uiPushButton( &uigc,
				 "visit &OSR web site (for free surveys)",
				 mCB(&ps,OSRPageShower,go), true );
    pb->attach( rightAlignedBelow, uigc.bottomFld() );
    if ( !uigc.go() || uigc.choice() == 0 )
	return;

    if ( (havedemosurv && uigc.choice() == 2) || 
         (!havedemosurv && uigc.choice() == 1))
    {
        uiFileDialog dlg( par, true, "", "*.zip", "Select zip file" );
        dlg.setDirectory( datadir );
        if ( !dlg.go() ) 
            return;

        zipfilenm = dlg.fileName();
    }

    (void)uiSurvey_UnzipFile( par, zipfilenm, datadir );
}
