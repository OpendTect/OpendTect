/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurveyzip.h"

#include "uifiledlg.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "survinfo.h"
#include "ziputils.h"

bool uiSurvey_UnzipFile( uiParent* par, const char* inpfnm,
			   const char* destdir )
{
    if ( !destdir || !*destdir )
	destdir = GetBaseDataDir();
    if ( !File::exists(destdir) )
    {
	uiMSG().error( BufferString(destdir,"\ndoes not exist") );
	return false;
    }
    if ( !File::isDirectory(destdir) )
    {
	uiMSG().error( BufferString(destdir,"\nis not a directory") );
	return false;
    }
    if ( !File::isWritable(destdir) )
    {
	uiMSG().error( BufferString(destdir,"\nis not a writable") );
	return false;
    }

    BufferString zipfnm( inpfnm );
    if ( zipfnm.isEmpty() || !File::exists(zipfnm) )
    {
	uiFileDialog fd( par, true, 0, "*.zip", "Select survey zip file" );
	fd.setAllowAllExts( true );
	if ( !fd.go() )
	    return false;
	zipfnm = fd.fileName();
    }

    // The uiFileDialog should make sure an actual existing file is selected
    uiTaskRunner tr( par, false ); BufferString emsg;
    if ( !ZipUtils::unZipArchive(zipfnm,destdir,emsg,&tr) )
    {
	BufferStringSet detailedmsg( 1, emsg );
	uiMSG().errorWithDetails( detailedmsg, "Failed to unzip the survey" );
	return false;
    }

    return true;
}


bool uiSurvey_ZipDirectory( uiParent* par, const char* sdn, const char* outfnm )
{
    BufferString survdirnm( sdn );
    if ( survdirnm.isEmpty() )
	survdirnm = SI().getDirName();
    FilePath survfp( GetBaseDataDir(), survdirnm );
    BufferString inpdir( survfp.fullPath() );
    while ( File::isDirectory(inpdir) && File::isLink(inpdir) )
	inpdir = File::linkTarget(inpdir);
    if ( !File::isDirectory(inpdir) )
    {
	uiMSG().error( BufferString(inpdir,"\ndoes not exist") );
	return false;
    }

    BufferString zipfnm( outfnm );
    if ( !zipfnm.isEmpty() )
    {
	const FilePath fp( zipfnm );
	if ( !File::isWritable(zipfnm) || !File::isWritable(fp.pathOnly()) )
	    zipfnm.setEmpty();
    }
    if ( zipfnm.isEmpty() )
    {
	uiFileDialog fd( par, false, 0,"*.zip","Select output survey zip file");
	fd.setAllowAllExts( true );
	if ( !fd.go() )
	    return false;
	zipfnm = fd.fileName();
    }

    uiTaskRunner tr( par, false ); BufferString emsg;
    if ( !ZipUtils::makeZip(zipfnm,inpdir,emsg,&tr) )
    {
	BufferStringSet detailedmsg( 1, emsg );
	uiMSG().errorWithDetails( detailedmsg, "Failed to zip the survey" );
	return false;
    }

    return true;
}
