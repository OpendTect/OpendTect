/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "surveyfile.h"

#include "dbman.h"
#include "file.h"
#include "filepath.h"
#include "uistrings.h"
#include "uistringset.h"
#include "ziputils.h"

extern "C" { mGlobal(Basic) void SetCurBaseDataDirOverrule(const char*); }

BufferString SurveyFile::filtStr()
{
    BufferString filt( "OpendTect project files (*." );
    filt.add( extStr() );
    filt.add( ");;All Files(*)" );
    return filt;
}


SurveyFile::SurveyFile( const char* survfilenm, bool automount )
    : surveyfile_(survfilenm)
{
    readSurveyDirNameFromFile( );
    if ( automount && isOK() )
	    mount();
}


SurveyFile::SurveyFile( const char* survfilenm, const char* surveyname )
    : surveyfile_(survfilenm)
    , surveydirnm_(surveyname)
{
}


SurveyFile::~SurveyFile()
{
    if ( mounted_ )
	unmount( false, nullptr );
}


void SurveyFile::readSurveyDirNameFromFile()
{
    lasterrs_.setEmpty();
    surveydirnm_.setEmpty();
    if ( surveyfile_.isEmpty() || !File::exists(surveyfile_) )
    {
	lasterrs_.set( uiStrings::phrCannotOpenForRead( surveyfile_ ) );
	return;
    }

    BufferStringSet fnms;
    uiString errmsg;
    if ( !ZipUtils::makeFileList(surveyfile_, fnms, errmsg) )
    {
	lasterrs_.set( errmsg );
	return;
    }

    if ( fnms.isEmpty() )
    {
	lasterrs_.set( uiStrings::phrInvalid( tr("project file") ) );
	return;
    }
    const BufferString survnm( fnms.get(0) );
    const BufferString omf( survnm, ".omf" );
    const bool isvalidsurvey = fnms.indexOf( omf ) > -1;
    if ( !isvalidsurvey )
    {
	lasterrs_.set( uiStrings::phrInvalid( tr("project file") ) );
	return;
    }
    surveydirnm_ = survnm;
}


uiRetVal SurveyFile::activate()
{
    lasterrs_.setEmpty();
    if ( !mounted_ )
    {
	lasterrs_.set( tr("%1 is not mounted").arg(surveyfile_) );
	return lasterrs_;
    }
    SetCurBaseDataDirOverrule( tmpbasedir_ );
    IOM().setDataSource( tmpbasedir_, surveydirnm_, true );
    IOM().setSurvey( surveydirnm_ );
    return lasterrs_;
}


uiRetVal SurveyFile::save( TaskRunner* trun )
{
    lasterrs_.setEmpty();
    FilePath surfp( tmpbasedir_, surveydirnm_ );
    if ( !mounted_ || !File::exists(surfp.fullPath()) )
    {
	lasterrs_.set( tr("%1 is not mounted").arg(surveyfile_) );
	return lasterrs_;
    }

    BufferString backupFile;
    if ( File::exists(surveyfile_) && File::isReadable(surveyfile_) )
    {
	FilePath backup_fp( surveyfile_ );
	backup_fp.setExtension( bckupExtStr() );
	backupFile = backup_fp.fullPath();
	if ( File::exists( backupFile ) )
	    File::remove( backupFile );
	File::rename( surveyfile_, backupFile );
    }

    uiString errmsg;
    if ( !ZipUtils::makeZip(surveyfile_, surfp.fullPath(), errmsg, trun) )
    {
	lasterrs_.set( errmsg );
	if ( File::exists(backupFile) && File::remove(surveyfile_) )
	    File::rename( backupFile, surveyfile_ );
	return lasterrs_;
    }
    return lasterrs_;
}


uiRetVal SurveyFile::mount( bool isnew, TaskRunner* trun )
{
    lasterrs_.setEmpty();
    if ( !isnew && !File::exists(surveyfile_) )
    {
	lasterrs_.set(
		tr("Cannot mount file: %1 does not exist").arg(surveyfile_) );
	return lasterrs_;
    }

    if ( tmpbasedir_.isEmpty() )
    {
	tmpbasedir_ = IOM().getNewTempDataRootDir();
	if ( tmpbasedir_.isEmpty() )
	{
	    lasterrs_.set( tr("Cannot create temporary data root") );
	    return lasterrs_;
	}
    }

    if ( !isnew )
    {
	uiString errmsg;
	if ( !ZipUtils::unZipArchive(surveyfile_, tmpbasedir_, errmsg, trun) )
	{
	    lasterrs_.set( errmsg );
	    File::removeDir( tmpbasedir_ );
	    tmpbasedir_.setEmpty();
	    mounted_ = false;
	    return lasterrs_;
	}
    }

    mounted_ = true;
    return lasterrs_;
}


uiRetVal SurveyFile::unmount( bool saveIt, TaskRunner* trun )
{
    lasterrs_.setEmpty();
    const FilePath surfp( tmpbasedir_, surveydirnm_ );
    if ( mounted_ && File::exists(surfp.fullPath()) && saveIt
							&& !save(trun).isOK() )
	return lasterrs_;

    File::removeDir( tmpbasedir_ );
    tmpbasedir_.setEmpty();
    mounted_ = false;

    return lasterrs_;
}
