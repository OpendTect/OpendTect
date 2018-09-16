/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiiocommon.h"

#include "uifileselector.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "survinfo.h"
#include "ziparchiveinfo.h"
#include "ziputils.h"
#include "uistring.h"


void uiSurvey::getDirectoryNames( BufferStringSet& list, bool addfullpath,
				const char* dataroot, const char* excludenm )
{
    return Survey::getDirectoryNames( list, addfullpath, dataroot, excludenm );
}


bool uiSurvey::userIsOKWithPossibleTypeChange( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn )
	return true;

     uiString warnmsg = od_static_tr("uiSurvey_userIsOKWithPossibleTypeChange",
			   "Your survey is set up as %1 only."
			   "\nTo be able to actually use %2 data"
			   "\nyou will have to change the survey setup."
			   "\n\nDo you wish to continue?")
	      .arg( is2d ? uiStrings::s3D() : uiStrings::s2D() )
	      .arg( is2d ? uiStrings::s2D() : uiStrings::s3D() );

    return gUiMsg().askContinue( warnmsg );
}


bool uiSurvey::unzipFile( uiParent* par, const char* inpfnm,
			   const char* destdir )
{
    ZipArchiveInfo zinfo( inpfnm );
    BufferStringSet fnms;
    zinfo.getAllFnms( fnms );
    if ( fnms.isEmpty() )
    {
	gUiMsg().error( uiStrings::phrInvalid(od_static_tr("uiSurvey_unzipFile",
							      "Zip Archive")) );
	return false;
    }
    const BufferString survnm( fnms.get(0) );
    const BufferString omf( survnm, ".omf" );
    const bool isvalidsurvey = fnms.indexOf( omf ) > -1;
    if ( !isvalidsurvey )
    {
	gUiMsg().error( od_static_tr("uiSurvey_unzipFile",
			    "This archive does not contain any valid survey") );
	return false;
    }
    if ( !destdir || !*destdir )
	destdir = GetBaseDataDir();
    if ( !File::exists(destdir) )
    {
	gUiMsg().error( od_static_tr("uiSurvey_unzipFile","%1\ndoes not exist")
					       .arg(toUiString(destdir)) );
	return false;
    }
    if ( !File::isDirectory(destdir) )
    {
	gUiMsg().error( od_static_tr("uiSurvey_unzipFile",
		      "%1\nis not a directory").arg(toUiString(destdir)) );
	return false;
    }
    if ( !File::isWritable(destdir) )
    {
	gUiMsg().error( od_static_tr("uiSurvey_unzipFile",
				    "%1 \nis not writable")
			            .arg(toUiString(destdir)) );
	return false;
    }

    File::Path surveypath( destdir, survnm );
    if ( File::exists(surveypath.fullPath()) )
    {
	 uiString errmsg( od_static_tr("uiSurvey_UnzipFile",
				 "%1 survey already exists.\nOverwrite the",
				 " existing survey?").arg(toUiString(
				 surveypath.fullPath())));
	if ( !gUiMsg().askOverwrite(errmsg) )
	    return false;
    }

    BufferString zipfnm( inpfnm );
    if ( zipfnm.isEmpty() || !File::exists(zipfnm) )
    {
	uiFileSelector::Setup fssu;
	fssu.setFormat( File::Format::zipFiles() );
	uiFileSelector uifs( par, fssu );
	uifs.caption() = uiStrings::phrSelect(
		    od_static_tr("uiSurvey_unzipFile","survey zip file") );
	if ( !uifs.go() )
	    return false;
	zipfnm = uifs.fileName();
    }

    // The uiFileSelector should make sure an actual existing file is selected
    uiTaskRunner taskrunner( par, false ); uiString emsg;
    if ( !ZipUtils::unZipArchive(zipfnm,destdir,emsg,&taskrunner) )
    {
	uiStringSet detailedmsg;
	detailedmsg += uiStrings::phrCannotUnZip( uiStrings::sSurvey() );
	detailedmsg += emsg;
	gUiMsg().errorWithDetails( detailedmsg );
	return false;
    }

    return true;
}


bool uiSurvey::zipDirectory( uiParent* par, const char* sdn,
			     const char* outfnm )
{
    BufferString survdirnm( sdn );
    if ( survdirnm.isEmpty() )
	survdirnm = SI().dirName();
    File::Path survfp( GetBaseDataDir(), survdirnm );
    BufferString inpdir( survfp.fullPath() );
    while ( File::isDirectory(inpdir) && File::isLink(inpdir) )
	inpdir = File::linkEnd(inpdir);
    if ( !File::isDirectory(inpdir) )
    {
	gUiMsg().error(od_static_tr("zipDirectory","%1\ndoes not exist")
		      .arg(toUiString(inpdir)) );
	return false;
    }

    BufferString zipfnm( outfnm );
    if ( !zipfnm.isEmpty() )
    {
	const File::Path fp( zipfnm );
	if ( !File::isWritable(fp.pathOnly()) )
	{
	    gUiMsg().error(od_static_tr("zipDirectory",
			  "%1 is not writable").arg(toUiString(fp.pathOnly())));
	    return false;
	}

    }
    if ( zipfnm.isEmpty() )
    {
	uiFileSelector::Setup fssu;
	fssu.setForWrite().setFormat( File::Format::zipFiles() );
	uiFileSelector uifs( par, fssu );
	uifs.caption() = uiStrings::phrSelect(
		    od_static_tr("zipDirectory","Output Survey Zip File"));
	if ( !uifs.go() )
	    return false;
	zipfnm = uifs.fileName();
    }

    uiTaskRunner taskrunner( par, false ); uiString emsg;
    if ( !ZipUtils::makeZip(zipfnm,inpdir,emsg,&taskrunner) )
    {
	uiStringSet detailedmsg;
	detailedmsg += uiStrings::phrCannotZip( uiStrings::sSurvey() );
	detailedmsg += emsg;
	gUiMsg().errorWithDetails( detailedmsg );
	return false;
    }

    return true;
}


SurveyInfo* uiSurvey::copySurvey( uiParent* uiparent, const char* survnm,
			const char* dataroot, const char* survdirnm,
			const char* targetpath )
{
    if ( !File::isDirectory(targetpath) )
    {
	gUiMsg().error( od_static_tr("uiSurvey_finishSurveyCopy",
		"Invalid target directory for copy:\n%1").arg( targetpath ) );
	return 0;
    }

    uiString alreadyexiststr = od_static_tr("uiSurvey_finishSurveyCopy",
	    "A directory\n\n%1\n\nalready exists.\nPlease remove or rename it");

    const BufferString newsurvdirnm( SurveyInfo::dirNameForName(survnm) );
    const BufferString todir = File::Path(targetpath,newsurvdirnm).fullPath();
    if ( File::exists(todir) )
    {
	gUiMsg().error( alreadyexiststr.arg( todir ) );
        return 0;
    }
    const BufferString linktodir = File::Path(dataroot,newsurvdirnm).fullPath();
    if ( File::exists(linktodir) )
    {
	if ( File::isLink(linktodir) )
	    File::remove( linktodir );
	if ( File::exists(linktodir) )
	{
	    gUiMsg().error( alreadyexiststr.arg( linktodir ) );
	    return 0;
	}
    }

    const BufferString tocopydirfullpath = File::Path(dataroot,survdirnm)
						.fullPath();
    const BufferString fromdir = File::linkEnd( tocopydirfullpath );

    uiTaskRunner taskrunner( uiparent );
    PtrMan<Executor> copier = File::getRecursiveCopier( fromdir, todir );
    if ( !taskrunner.execute(*copier) )
	return 0;

    File::makeWritable( todir, true, true );

    if ( FixedString(targetpath) != dataroot )
    {
	if ( !File::createLink(todir,linktodir) )
	{
	    gUiMsg().error( od_static_tr("uiSurvey_finishSurveyCopy",
		"Copy was successful, but could not create a link from:"
		    "\n\n%1\n\nto:\n\n%2\n\n"
		"The new survey will therefore not appear in the list.")
		    .arg( todir ).arg( linktodir ) );
	    return 0;
	}
    }

    uiRetVal uirv;
    SurveyInfo* survinfo = SurveyInfo::read( todir, uirv );
    if ( uirv.isOK() )
    {
	survinfo->setName( survnm );
	const SurveyDiskLocation sdl( SurveyInfo::dirNameForName(survnm) );
	survinfo->setDiskLocation( sdl );
	survinfo->write( todir );
    }
    else
    {
	delete survinfo; survinfo = 0;
        gUiMsg().error( od_static_tr("uiSurvey_finishSurveyCopy",
	    "The copied survey's information cannot be read."
	    "\nThis is probably related to file access permission problems.") );
    }

    return survinfo;
}
