/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

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
	uiMSG().error( uiStrings::phrInvalid(od_static_tr("uiSurvey_UnzipFile",
							      "Zip Archive")) );
	return false;
    }
    const BufferString survnm( fnms.get(0) );
    const BufferString omf( survnm, ".omf" );
    const bool isvalidsurvey = fnms.indexOf( omf ) > -1;
    if ( !isvalidsurvey )
    {
	uiMSG().error( od_static_tr("uiSurvey_UnzipFile",
			    "This archive does not contain any valid survey") );
	return false;
    }
    if ( !destdir || !*destdir )
	destdir = GetBaseDataDir();
    if ( !File::exists(destdir) )
    {
	uiMSG().error( od_static_tr("uiSurvey_UnzipFile","%1\ndoes not exist")
				.arg(toUiString(destdir)) );
	return false;
    }
    if ( !File::isDirectory(destdir) )
    {
	uiMSG().error( od_static_tr("uiSurvey_UnzipFile","%1\nis not a folder.")
				.arg(toUiString(destdir)) );
	return false;
    }
    if ( !File::isWritable(destdir) )
    {
	uiMSG().error( od_static_tr("uiSurvey_UnzipFile","%1 \nis not writable")
				.arg(toUiString(destdir)) );
	return false;
    }

    FilePath surveypath( destdir, survnm );
    if ( File::exists(surveypath.fullPath()) )
    {
	 uiString errmsg( od_static_tr("uiSurvey_UnzipFile",
				 "%1 survey already exists.\nOverwrite the",
				 " existing survey?").arg(toUiString(
				 surveypath.fullPath())));
	if ( !uiMSG().askOverwrite(errmsg) )
	    return false;
    }

    BufferString zipfnm( inpfnm );
    if ( zipfnm.isEmpty() || !File::exists(zipfnm) )
    {
	uiFileDialog fd( par, true, 0, "*.zip", uiStrings::phrSelect(
			od_static_tr("uiSurvey_UnzipFile","survey zip file")) );
	if ( !fd.go() )
	    return false;
	zipfnm = fd.fileName();
    }

    // The uiFileDialog should make sure an actual existing file is selected
    uiTaskRunner taskrunner( par, false ); uiString emsg;
    if ( !ZipUtils::unZipArchive(zipfnm,destdir,emsg,&taskrunner) )
    {
	uiStringSet detailedmsg;
	detailedmsg += uiStrings::phrCannotUnZip( uiStrings::sSurvey() );
	detailedmsg += emsg;
	uiMSG().errorWithDetails( detailedmsg );
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
	uiMSG().error(od_static_tr("uiSurvey_ZipDirectory","%1\ndoes not exist")
		      .arg(toUiString(inpdir)) );
	return false;
    }

    BufferString zipfnm( outfnm );
    if ( !zipfnm.isEmpty() )
    {
	const FilePath fp( zipfnm );
	if ( !File::isWritable(fp.pathOnly()) )
	{
	    uiMSG().error(od_static_tr("uiSurvey_ZipDirectory",
			  "%1 is not writable").arg(toUiString(fp.pathOnly())));
	    return false;
	}

    }
    if ( zipfnm.isEmpty() )
    {
	uiFileDialog fd( par, false, 0,"*.zip",uiStrings::phrSelect(
			 uiStrings::phrOutput(uiStrings::phrJoinStrings(
			 uiStrings::sSurvey(), uiStrings::sZip(),
			 uiStrings::sFile()))));
	if ( !fd.go() )
	    return false;
	zipfnm = fd.fileName();
    }

    uiTaskRunner taskrunner( par, false ); uiString emsg;
    if ( !ZipUtils::makeZip(zipfnm,inpdir,emsg,&taskrunner) )
    {
	uiStringSet detailedmsg;
	detailedmsg += uiStrings::phrCannotZip( uiStrings::sSurvey() );
	detailedmsg += emsg;
	uiMSG().errorWithDetails( detailedmsg );
	return false;
    }

    return true;
}
