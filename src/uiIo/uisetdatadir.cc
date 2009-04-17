/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisetdatadir.cc,v 1.26 2009-04-17 09:43:31 cvsbert Exp $";

#include "uisetdatadir.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "ioman.h"
#include "settings.h"
#include "filegen.h"
#include "filepath.h"
#include "envvars.h"
#include "oddirs.h"
#include "oddatadirmanip.h"
#include <stdlib.h>

#ifdef __win__
# include "winutils.h"
#endif


extern "C" { const char* GetBaseDataDir(); }


uiSetDataDir::uiSetDataDir( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Set Data Directory",
		    		     "Specify a data storage directory",
		    		     "8.0.1"))
	, oddirfld(0)
	, olddatadir(GetBaseDataDir())
{
    const bool oldok = OD_isValidRootDataDir( olddatadir );
    BufferString oddirnm, basedirnm;
    const char* titltxt = 0;

    if ( !olddatadir.isEmpty() )
    {
	if ( oldok )
	{
	    titltxt = 
		  "Locate an OpendTect Data Root directory\n"
		  "or specify a new directory name to create";
	    basedirnm = olddatadir;
	}
	else
	{
	    titltxt = 
		  "OpendTect needs a place to store your data files.\n"
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
	titltxt =
	"OpendTect needs a place to store your data files:"
	" the OpendTect Data Root.\n\n"
	"You have not yet specified a location for it,\n"
	"and there is no 'DTECT_DATA' set in your environment.\n\n"
	"Please specify where the OpendTect Data Root should\n"
	"be created or select an existing OpendTect Data Root.\n"
#ifndef __win__
	"\n\nNote that you can still put surveys and "
	"individual cubes on other disks;\nbut this is where the "
	"'base' data store will be."
#endif
	;
	oddirnm = "ODData";
	basedirnm = GetPersonalDir();
    }
    setTitleText( titltxt );

    const char* basetxt = oldok ? "OpendTect Data Root Directory"
				: "Location";
    basedirfld = new uiFileInput( this, basetxt,
			      uiFileInput::Setup(basedirnm).directories(true) );
    if ( !oldok )
    {
	oddirfld = new uiGenInput( this, "Directory name", oddirnm );
	oddirfld->attach( alignedBelow, basedirfld );
	olddatadir = "";
    }
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiSetDataDir::acceptOK( CallBacker* )
{
    BufferString datadir = basedirfld->text();
    if ( datadir.isEmpty() || !File_isDirectory(datadir) )
	mErrRet( "Please enter a valid (existing) location" )

    if ( oddirfld )
    {
	BufferString oddirnm = oddirfld->text();
	if ( oddirnm.isEmpty() )
	    mErrRet( "Please enter a (sub-)directory name" )

	datadir = FilePath( datadir ).add( oddirnm ).fullPath();
    }
    else if ( datadir == olddatadir )
	return true;

    FilePath fpdd( datadir ); FilePath fps( GetSoftwareDir() );
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


bool uiSetDataDir::setRootDataDir( const char* inpdatadir )
{
    BufferString datadir = inpdatadir;
    const char* msg = OD_SetRootDataDir( datadir );
    if ( !msg ) return true;

    const BufferString omffnm = FilePath( datadir ).add( ".omf" ).fullPath();
    const BufferString stdomf( mGetSetupFileName("omf") );
    bool trycpdemosurv = false;

    if ( !File_exists(datadir) )
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
	if ( !File_createDir( datadir, 0 ) )
	    mErrRet( "Cannot create the new directory.\n"
		     "Please check if you have the required write permissions" )
	File_copy( stdomf, omffnm, mFile_NotRecursive );
	if ( !File_exists(omffnm) )
	    mErrRet( "Cannot copy a file to the new directory!" )

	trycpdemosurv = true;
    }

    if ( !OD_isValidRootDataDir(datadir) )
    {
	if ( !File_isDirectory(datadir) )
	    mErrRet( "A file (not a directory) with this name already exists" )
	if ( !File_exists(omffnm) )
	{
	    if ( !uiMSG().askGoOn( "This is not an OpendTect data directory.\n"
				  "Do you want it to be converted into one?" ) )
		return false;
	    File_copy( stdomf, omffnm, mFile_NotRecursive );
	    if ( !File_exists(omffnm) )
		mErrRet( "Could not convert the directory.\n"
			 "Most probably you have no write permissions." )

	    trycpdemosurv = true;
	}
	else
	{
	    FilePath fp( datadir ); fp.add( "Seismics" );
	    if ( File_exists(fp.fullPath()) )
	    {
		fp.setFileName( 0 );
		BufferString probdatadir( fp.pathOnly() );
		BufferString msg( "This seems to be a survey directory.\n" );
		msg += "We need the directory containing the survey dirs.\n"
			"Do you want to correct your input to\n"; 
		msg += probdatadir;
		msg += " ...?";
		int res = uiMSG().askGoOnAfter( msg );
		if ( res == 2 )
		    return false;
		else if ( res == 0 )
		{
		    datadir = probdatadir;
		    return true;
		}
	    }
	}
    }

    if ( trycpdemosurv && GetEnvVar( "DTECT_DEMO_SURVEY" ) )
    {
	FilePath demosurvnm( GetSoftwareDir() );
	demosurvnm.add( GetEnvVar("DTECT_DEMO_SURVEY") );

	if ( File_isDirectory(demosurvnm.fullPath()) )
	{
	    FilePath fp( datadir );
	    fp.add( FilePath(demosurvnm).fileName() );
	    const BufferString todir( fp.fullPath() );
	    if ( !File_exists(todir) )
	    {
		if ( uiMSG().askGoOn( 
			"Do you want to install the demo survey\n"
			"in your OpendTect Data Root directory?" ) )
		    File_copy( demosurvnm.fullPath(), todir, mFile_Recursive );
	    }
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
