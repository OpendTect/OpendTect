/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uisetdatadir.h"

#include "uifileinput.h"
#include "uimsg.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "oddatadirmanip.h"
#include "odinst.h"
#include "settings.h"
#include "ziputils.h"

#include <stdlib.h>

#ifdef __win__
# include "winutils.h"
#endif


extern "C" { const char* GetBaseDataDir(); }


uiSetDataDir::uiSetDataDir( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Set Data Directory",
		    		     "Specify a data storage directory",
		    		     "8.0.1"))
	, olddatadir(GetBaseDataDir())
{
    const bool oldok = OD_isValidRootDataDir( olddatadir );
    BufferString oddirnm, basedirnm;
    const char* titletxt = 0;

    if ( !olddatadir.isEmpty() )
    {
	if ( oldok )
	{
	    titletxt =	"Locate an OpendTect Data Root directory\n"
			"or specify a new directory name to create";
	    basedirnm = olddatadir;
	}
	else
	{
	    titletxt =	"OpendTect needs a place to store your data files.\n"
			"\nThe current OpendTect Data Root is invalid.\n"
			"* Locate a valid data root directory\n"
			"* Or specify a new directory name to create";

	    FilePath fp( olddatadir );
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
    basedirfld = new uiFileInput( this, basetxt,
			      uiFileInput::Setup(uiFileDialog::Gen,basedirnm)
			      .directories(true) );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiSetDataDir::acceptOK( CallBacker* )
{
    BufferString datadir = basedirfld->text();
    if ( datadir.isEmpty() || !File::isDirectory(datadir) )
	mErrRet( "Please enter a valid (existing) location" )

    if ( datadir == olddatadir )
	return true;

    FilePath fpdd( datadir ); FilePath fps( GetSoftwareDir(0) );
    const int nrslvls = fps.nrLevels();
    if ( fpdd.nrLevels() >= nrslvls )
    {
	const BufferString ddatslvl( fpdd.dirUpTo(nrslvls-1) );
	if ( ddatslvl == fps.fullPath() )
	{
	    uiMSG().error( "The directory you have chosen is\n *INSIDE*\n"
			   "the software installation directory.\n"
			   "Please choose another directory" );
	    return false;
	}
    }

    return setRootDataDir( datadir );
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

    return ret;
}


bool uiSetDataDir::setRootDataDir( const char* inpdatadir )
{
    BufferString datadir = inpdatadir;
    const char* msg = OD_SetRootDataDir( datadir );
    if ( !msg ) return true;

    const BufferString omffnm = FilePath( datadir ).add( ".omf" ).fullPath();
    const BufferString stdomf( mGetSetupFileName("omf") );
    bool trycpdemosurv = false;

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
	File::copy( stdomf, omffnm );
	if ( !File::exists(omffnm) )
	    mErrRet( "Cannot copy a file to the new directory!" )

	trycpdemosurv = true;
    }

    if ( !OD_isValidRootDataDir(datadir) )
    {
	if ( !File::isDirectory(datadir) )
	    mErrRet( "A file (not a directory) with this name already exists" )
	if ( !File::exists(omffnm) )
	{
	    if ( !uiMSG().askGoOn( "This is not an OpendTect data directory.\n"
				  "Do you want it to be converted into one?" ) )
		return false;
	    File::copy( stdomf, omffnm );
	    if ( !File::exists(omffnm) )
		mErrRet( "Could not convert the directory.\n"
			 "Most probably you have no write permissions." )

	    trycpdemosurv = true;
	}
	else
	{
	    FilePath fp( datadir, "Seismics" );
	    if ( File::exists(fp.fullPath()) )
	    {
		fp.setFileName( 0 );
		const BufferString probdatadir( fp.pathOnly() );
		const BufferString umsg(
			"This seems to be a survey directory.\n"
			"We need the directory containing the survey dirs.\n"
			"Do you want to correct your input to\n",
			probdatadir, " ...?" );
		const int res = uiMSG().askGoOnAfter( umsg );
		if ( res == 2 )
		    return false;
		else if ( res == 0 )
		    { datadir = probdatadir; return true; }
	    }
	}
    }

    const char* demosurvenvvar = GetEnvVar( "DTECT_DEMO_SURVEY" );
    BufferString instdemosurv = getInstalledDemoSurvey();
    if ( trycpdemosurv && (demosurvenvvar || !instdemosurv.isEmpty()) )
    {
	if ( demosurvenvvar && *demosurvenvvar ) //TODO: May be this should go.
	{
	    FilePath demosurvnm( GetSoftwareDir(0) );
	    demosurvnm.add( GetEnvVar("DTECT_DEMO_SURVEY") );

	    if ( File::isDirectory(demosurvnm.fullPath()) )
	    {
		FilePath fp( datadir, FilePath(demosurvnm).fileName() );
		const BufferString todir( fp.fullPath() );
		if ( !File::exists(todir) )
		{
		    if ( uiMSG().askGoOn( 
			    "Do you want to install the demo survey\n"
			    "in your OpendTect Data Root directory?" ) )
			File::copy( demosurvnm.fullPath(), todir );
		}
	    }
	}
	else if ( !instdemosurv.isEmpty() )
	{
	    ZipUtils zu;
	    if ( uiMSG().askGoOn( "Do you want to install the demo survey\n"
				  "in your OpendTect Data Root directory?" ) )
		zu.UnZip( instdemosurv, datadir );
	}
    }

    msg = OD_SetRootDataDir( datadir );
    if ( msg )
    {
	uiMSG().error( msg );
	return false;
    }

    return true;
}
