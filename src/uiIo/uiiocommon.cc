/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiiocommon.h"

#include "uifiledlg.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "survinfo.h"
#include "ziparchiveinfo.h"
#include "ziputils.h"
#include "dirlist.h"
#include "uistring.h"


void uiSurvey::getDirectoryNames( BufferStringSet& list, bool addfullpath,
				const char* dataroot, const char* excludenm )
{
    BufferString basedir = dataroot;
    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();
    DirList dl( basedir, DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString& dirnm = dl.get( idx );
	if ( excludenm && dirnm == excludenm )
	    continue;

	const FilePath fp( basedir, dirnm, SurveyInfo::sKeySetupFileName() );
	if ( File::exists(fp.fullPath()) )
	{
	    if ( addfullpath )
		list.add( dl.fullPath(idx) );
	    else
		list.add( dirnm );
	}
    }

    list.sort();
}


bool uiSurvey::userIsOKWithPossibleTypeChange( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn )
	return true;

     uiString warnmsg = od_static_tr("uiSurvey_userIsOKWithPossibleTypeChange",
			   "Your survey is set up as '%1 data"
			   "\nyou will have to change the survey setup."
			   "\n\nDo you wish to continue?")
      .arg( is2d
	  ? od_static_tr("uiSurvey_userIsOKWithPossibleTypeChange",
		"3-D only'.\nTo be able to actually use 2-D")
	    : od_static_tr("uiSurvey_userIsOKWithPossibleTypeChange",
		"2-D only'.\nTo be able to actually use 3-D"));

    return uiMSG().askContinue( warnmsg );
}


bool uiSurvey::unzipFile( uiParent* par, const char* inpfnm,
			   const char* destdir )
{
    ZipArchiveInfo zinfo( inpfnm );
    BufferStringSet fnms;
    zinfo.getAllFnms( fnms );
    if ( fnms.isEmpty() )
    {
	uiMSG().error( uiStrings::phrInvalid(od_static_tr("uiSurvey_unzipFile",
							      "Zip Archive")) );
	return false;
    }
    const BufferString survnm( fnms.get(0) );
    const BufferString omf( survnm, ".omf" );
    const bool isvalidsurvey = fnms.indexOf( omf ) > -1;
    if ( !isvalidsurvey )
    {
	uiMSG().error( od_static_tr("uiSurvey_unzipFile",
			    "This archive does not contain any valid survey") );
	return false;
    }
    if ( !destdir || !*destdir )
	destdir = GetBaseDataDir();
    if ( !File::exists(destdir) )
    {
	uiMSG().error( od_static_tr("uiSurvey_unzipFile","%1\ndoes not exist")
					       .arg(toUiString(destdir)) );
	return false;
    }
    if ( !File::isDirectory(destdir) )
    {
	uiMSG().error( od_static_tr("uiSurvey_unzipFile",
		      "%1\nis not a directory").arg(toUiString(destdir)) );
	return false;
    }
    if ( !File::isWritable(destdir) )
    {
	uiMSG().error( od_static_tr("uiSurvey_unzipFile",
				    "%1 \nis not writable")
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
			od_static_tr("uiSurvey_unzipFile","survey zip file")) );
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


bool uiSurvey::zipDirectory( uiParent* par, const char* sdn,
			     const char* outfnm )
{
    BufferString survdirnm( sdn );
    if ( survdirnm.isEmpty() )
	survdirnm = SI().getDirName();
    FilePath survfp( GetBaseDataDir(), survdirnm );
    BufferString inpdir( survfp.fullPath() );
    while ( File::isDirectory(inpdir) && File::isLink(inpdir) )
	inpdir = File::linkEnd(inpdir);
    if ( !File::isDirectory(inpdir) )
    {
	uiMSG().error(od_static_tr("uiSurvey_zipDirectory","%1\ndoes not exist")
		      .arg(toUiString(inpdir)) );
	return false;
    }

    BufferString zipfnm( outfnm );
    if ( !zipfnm.isEmpty() )
    {
	const FilePath fp( zipfnm );
	if ( !File::isWritable(fp.pathOnly()) )
	{
	    uiMSG().error(od_static_tr("uiSurvey_zipDirectory",
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
