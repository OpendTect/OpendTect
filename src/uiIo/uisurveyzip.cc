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
#include "ziparchiveinfo.h"
#include "ziputils.h"

bool uiSurvey_UnzipFile( uiParent* par, const char* inpfnm,
			   const char* destdir )
{
    ZipArchiveInfo zinfo( inpfnm );
    BufferStringSet fnms;
    zinfo.getAllFnms( fnms );
    if ( fnms.isEmpty() )
    {
	uiMSG().error( "Invalid Zip archive" );
	return false;
    }
    const BufferString survnm( fnms.get(0) );
    const BufferString omf( survnm, ".omf" );
    const bool isvalidsurvey = fnms.indexOf( omf ) > -1;
    if ( !isvalidsurvey )
    {
	uiMSG().error( "This archive does not contain any valid survey" );
	return false;
    }
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

    FilePath surveypath( destdir, survnm );
    if ( File::exists(surveypath.fullPath()) )
    {
	 BufferString errmsg( surveypath.fullPath()," survey already exists.",
				"\nOverwrite the existing survey?");
	if ( !uiMSG().askOverwrite(errmsg) )
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
    uiTaskRunner taskrunner( par, false ); uiString emsg;
    if ( !ZipUtils::unZipArchive(zipfnm,destdir,emsg,&taskrunner) )
    {
	TypeSet<uiString> detailedmsg( 1, emsg );
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
	if ( !File::isWritable(fp.pathOnly()) )
	{
	    uiMSG().error( fp.pathOnly(), " is not writable" );
	    return false;
	}
	    
    }
    if ( zipfnm.isEmpty() )
    {
	uiFileDialog fd( par, false, 0,"*.zip","Select output survey zip file");
	fd.setAllowAllExts( true );
	if ( !fd.go() )
	    return false;
	zipfnm = fd.fileName();
    }

    uiTaskRunner taskrunner( par, false ); uiString emsg;
    if ( !ZipUtils::makeZip(zipfnm,inpdir,emsg,&taskrunner) )
    {
	TypeSet<uiString> detailedmsg( 1, emsg );
	uiMSG().errorWithDetails( detailedmsg, "Failed to zip the survey" );
	return false;
    }

    return true;
}
