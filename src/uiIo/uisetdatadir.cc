/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          June 2002
 RCS:           $Id: uisetdatadir.cc,v 1.5 2004-01-16 10:34:36 bert Exp $
________________________________________________________________________

-*/

#include "uisetdatadir.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "ioman.h"
#include "settings.h"
#include "filegen.h"
#include <stdlib.h>


extern "C" { const char* GetBaseDataDir(); }


uiSetDataDir::uiSetDataDir( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Set Data Directory",
		    		     "Specify a data storage directory",
		    		     "8.0.1"))
	, oddirfld(0)
	, olddatadir(GetBaseDataDir())
{
    const bool oldok = isOK( olddatadir );
    BufferString oddirnm, basedirnm;
    const char* titltxt = 0;

    if ( olddatadir != "" )
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
		  "OpendTect needs a place to store its data.\n"
		  "The current OpendTect Data Root is invalid.\n"
		  "* Locate a valid data root directory\n"
		  "* Or specify a new directory name to create";

	    oddirnm = File_getFileName( olddatadir );
	    basedirnm = File_getPathOnly( olddatadir );
	}
    }
    else
    {
	titltxt =
	"OpendTect needs a place to store its data: the OpendTect Data Root.\n"
	"You have not yet specified a location for it,\n"
	"and there is no 'DTECT_DATA or dGB_DATA' set in your environment.\n\n"
	"Please specify where the OpendTect Data Root should\n"
	"be created or select an existing OpendTect Data Root."
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
				  uiFileInput::Setup(basedirnm).directories() );

    if ( !oldok )
    {
	oddirfld = new uiGenInput( this, "Directory name", oddirnm );
	oddirfld->attach( alignedBelow, basedirfld );
	olddatadir = "";
    }
}


bool uiSetDataDir::isOK( const char* d )
{
    const BufferString datadir( d ? d : GetBaseDataDir() );
    const BufferString omffnm( File_getFullPath( datadir, ".omf" ) );
    const BufferString seisdir( File_getFullPath( datadir, "Seismics" ) );
    return File_isDirectory( datadir )
	&& File_exists( omffnm )
	&& !File_exists( seisdir );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiSetDataDir::acceptOK( CallBacker* )
{
    BufferString datadir = basedirfld->text();
    if ( datadir == "" || !File_isDirectory(datadir) )
	mErrRet( "Please enter a valid (existing) location" )

    if ( oddirfld )
    {
	BufferString oddirnm = oddirfld->text();
	if ( oddirnm == "" )
	    mErrRet( "Please enter a (sub-)directory name" )

	datadir = File_getFullPath( datadir, oddirnm );
    }
    else if ( datadir == olddatadir )
	return true;

    const BufferString omffnm = File_getFullPath( datadir, ".omf" );
    const BufferString stdomf( GetDataFileName("omf") );

    if ( !File_exists(datadir) )
    {
#ifdef __win__
	if ( !strncasecmp("C:\\Program Files", datadir, 16)
	  || strstr( datadir, "Program Files" )
	  || strstr( datadir, "program files" )
	  || strstr( datadir, "PROGRAM FILES" ) )
	    mErrRet( "Please do not try to use 'Program Files' for data.\n"
		     "A directory like 'My Documents' would be good." )
#endif
	if ( !File_createDir( datadir, 0 ) )
	    mErrRet( "Cannot create the new directory.\n"
		     "Please check if you have the required write permissions" )
	File_copy( stdomf, omffnm, NO );
	if ( !File_exists(omffnm) )
	    mErrRet( "Cannot copy a file to the new directory!" )

        if ( getenv( "DTECT_DEMO_SURVEY") )
	{
            const BufferString surveynm( getenv( "DTECT_DEMO_SURVEY" ) );
            const BufferString todir( File_getFullPath( datadir,
                                      File_getFileName(surveynm) ) );

            if ( File_isDirectory(surveynm) && !File_exists(todir) )
	    {
		if ( uiMSG().askGoOn( 
			    "Do you want to install the demo survey\n"
			    "in your new OpendTect Data Root directory?" ) )
		    File_copy( surveynm, todir, YES );
	    }
	}

    }
    else if ( !isOK(datadir) )
    {
	if ( !File_isDirectory(datadir) )
	    mErrRet( "A file (not a directory) with this name already exists" )
	if ( !File_exists(omffnm) )
	{
	    if ( !uiMSG().askGoOn( "This is not an OpendTect data directory.\n"
				  "Do you want it to be converted into one?" ) )
		return false;
	    File_copy( stdomf, omffnm, NO );
	    if ( !File_exists(omffnm) )
		mErrRet( "Could not convert the directory.\n"
			 "Most probably you have no write permissions." )

	    if ( getenv( "DTECT_DEMO_SURVEY") )
	    {
		const BufferString surveynm( getenv( "DTECT_DEMO_SURVEY" ) );
		const BufferString todir( File_getFullPath( datadir,
					  File_getFileName(surveynm) ) );

		if ( File_isDirectory(surveynm) && !File_exists(todir) )
		{
		    if ( uiMSG().askGoOn( 
			    "Do you want to install the demo survey\n"
			    "in your OpendTect Data Root directory?" ) )
			File_copy( surveynm, todir, YES );
		}
	    }
	}
	else
	{
	    const BufferString seisdir( File_getFullPath(datadir,"Seismics") );
	    if ( File_exists(seisdir) )
	    {
		BufferString probdatadir( File_getPathOnly(datadir) );
		BufferString msg( "This seems to be a survey directory.\n" );
		msg += "We need the directory containing the survey dirs.\n"
			"Do you want to correct your input to\n"; 
		msg += probdatadir;
		msg += " ...?";
		int res = uiMSG().askGoOnAfter( msg );
		if ( res == 2 )
		    return false;
		else if ( res == 0 )
		    datadir = probdatadir;
	    }
	}
    }

    // OK - we're (almost) certain that the directory exists and is valid
    const bool haveenv = getenv("DTECT_DATA") || getenv( "dGB_DATA" )
#ifdef __win__
		      || getenv( "DTECT_WINDATA" ) || getenv( "dGB_WINDATA" )
#endif
	;
    if ( haveenv )
#ifdef __win__
	setEnvVar( "DTECT_WINDATA", datadir.buf() );
#else
	setEnvVar( "DTECT_DATA", datadir.buf() );
#endif

    Settings::common().set( "Default DATA directory", datadir );
    if ( !Settings::common().write() )
    {
	if ( !haveenv )
	    mErrRet( "Cannot write your user settings.\n"
		     "This means your selection cannot be used!" );
	uiMSG().warning( "Cannot write your user settings.\n"
			 "Preferences cannot be stored!" );
    }

    IOMan::newSurvey();
    return true;
}
