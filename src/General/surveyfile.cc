/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "surveyfile.h"

#include "coordsystem.h"
#include "dbman.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odjson.h"
#include "uistrings.h"
#include "uistringset.h"
#include "ziputils.h"

extern "C" { mGlobal(Basic) void SetCurBaseDataDirOverrule(const char*); }

SurveyCreator::SurveyCreator( const char* survfilenm, const char* surveyname )
    : surveyfile_(survfilenm)
    , surveydirnm_(surveyname)
{}


BufferString SurveyFile::filtStr()
{
    BufferString filt( "OpendTect project files (*." );
    filt.add( extStr() );
    filt.add( ");;All Files(*)" );
    return filt;
}


SurveyFile::SurveyFile( const char* survfilenm, bool automount )
    : SurveyCreator(survfilenm,nullptr)
{
    readSurveyDirNameFromFile();
    if ( automount && isOK() )
	    mount();
}


SurveyFile::SurveyFile( const char* survfilenm, const char* surveyname )
    : SurveyCreator(survfilenm,surveyname)
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
    if ( !ZipUtils::makeZip(surveyfile_,surfp.fullPath(),errmsg,trun) )
    {
	lasterrs_.set( errmsg );
	if ( File::exists(backupFile) && File::remove(surveyfile_) )
	    File::rename( backupFile, surveyfile_ );
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



EmptyTempSurvey::EmptyTempSurvey( const char* surveybaseloc,
					const char* surveynm, bool ismanaged )
    : SurveyCreator(surveybaseloc,surveynm)
    , si_(SurveyInfo())
    , ismanaged_(ismanaged)
{
    tmpbasedir_ = surveybaseloc;
    initSurvey();
}


EmptyTempSurvey::EmptyTempSurvey( const OD::JSON::Object& obj )
    : SurveyCreator(nullptr,nullptr)
    , si_(SurveyInfo())
{
    tmpbasedir_ = obj.getStringValue( sKey::sKeySurveyLoc() );
    surveydirnm_ = obj.getStringValue( sKey::sKeySurveyNm() );
    saveloc_ = obj.getStringValue( sKeySaveLoc() );
    initSurvey( &obj );
}


EmptyTempSurvey::~EmptyTempSurvey()
{
    if ( !ismanaged_ && File::exists(tmpbasedir_) )
	File::removeDir( tmpbasedir_ );

    if ( !origsurveyfp_.isEmpty() )
	IOM().setSurvey( origsurveyfp_ );
}


bool EmptyTempSurvey::initSurvey( const OD::JSON::Object* obj )
{
    if ( surveydirnm_.isEmpty() )
	surveydirnm_ = "TemporarySurvey";

    if ( tmpbasedir_.isEmpty() || !File::isWritable(tmpbasedir_) )
    {
	lastwarning_ = tr("Write location was not writable, "
			    "creating directory at tempoarary location");
	tmpbasedir_ =
		    FilePath( File::getTempPath(), "OD_Survey" ).fullPath();
	surveyfile_ = tmpbasedir_;
    }

    if ( File::exists(tmpbasedir_) )
    {
	lastwarning_ = tr("%1 is not empty, "
	"might contain previous survey which might be lost.").arg(tmpbasedir_);
	ismanaged_ = false;

    }
    else if ( !File::createDir(tmpbasedir_) )
    {
	lasterrs_ = tr("Failed to create temporary survey data");
	return false;
    }

    si_.setName( surveydirnm_ );
    FilePath fp( tmpbasedir_, surveydirnm_ );
    si_.disklocation_ = SurveyDiskLocation( fp );
    const bool hasomf = File::exists(
				    FilePath(tmpbasedir_,".omf").fullPath() );
    if ( !createTempSurveySetup(hasomf) )
	return false;

    return fillSurveyInfo( obj );
}


bool EmptyTempSurvey::createOMFFile()
{
    IODir basedir( tmpbasedir_.buf() );
    return basedir.doWrite();
}


bool EmptyTempSurvey::createTempSurveySetup( bool hasomf )
{
    if ( !hasomf && !createOMFFile() )
    {
	lasterrs_ =
	    tr("Failed to create resource files at the specified location");
	return false;
    }

    const BufferString fnmin =
			mGetSetupFileName( SurveyInfo::sKeyBasicSurveyName() );
    const BufferString fnmout = FilePath( tmpbasedir_ )
					    .add( surveydirnm_ ).fullPath();
    if ( File::exists(fnmout) )
    {
	lasterrs_ = tr("Folder creation failed."
	    "\nTarget folder exists at %1.").arg(tmpbasedir_);
	return false;
    }

    const bool isok = File::copy( fnmin, fnmout );
    if ( !isok || !File::exists(fnmout) )
    {
	lasterrs_ = tr("Failed to create temporary survey at %1")
						    .arg( tmpbasedir_ );
	return false;
    }

    return true;
}



bool EmptyTempSurvey::fillSurveyInfo( const OD::JSON::Object* obj )
{
    TrcKeyZSampling cs;
    TrcKeySampling& hs = cs.hsamp_;
    if ( obj )
    {
	const BufferString zdom( obj->getStringValue(sKey::ZDomain()) );
	const bool istime = zdom.isEqual( sKey::Time() );
	const bool isfeet = obj->getBoolValue( SurveyInfo::sKeyDpthInFt() );
	si_.setZUnit( istime, isfeet );
	si_.getPars().setYN( SurveyInfo::sKeyDpthInFt(), isfeet );
	hs.start_.inl() = obj->getDoubleValue( sKey::FirstInl() );
	hs.start_.crl() = obj->getDoubleValue( sKey::FirstCrl() );
	hs.stop_.inl() = obj->getDoubleValue( sKey::LastInl() );
	hs.stop_.crl() = obj->getDoubleValue( sKey::LastCrl() );
	hs.step_.inl() = obj->getDoubleValue( sKey::StepInl() );
	hs.step_.crl() = obj->getDoubleValue( sKey::StepCrl() );
	const double srd = obj->getDoubleValue(
	    SurveyInfo::sKeySeismicRefDatum() );
	si_.setSeismicReferenceDatum( srd );
	const int crsid = obj->getIntValue( sKeyCRSID() );
	if ( crsid >= 0 )
	{
	    IOPar crspar;

	    crspar.set( "System name", "ProjectionBased System" );
	    crspar.set( "Projection.ID", crsid );
	    RefMan<Coords::CoordSystem> coordsys =
		Coords::CoordSystem::createSystem( crspar );
	    si_.setCoordSystem( coordsys );
	    si_.setXYInFeet( coordsys->isFeet() );
	}
	else
	{
	    RefMan<Coords::CoordSystem> coordsys =
		Coords::CoordSystem::getWGS84LLSystem();
	    si_.setCoordSystem( coordsys );
	    si_.setXYInFeet( coordsys->isFeet() );
	}
    }
    else
    {
	si_.setZUnit( true );
	si_.getPars().setYN( SurveyInfo::sKeyDpthInFt(), false );
	si_.setXYInFeet( false );
	si_.setSeismicReferenceDatum( 0 );
	si_.setCoordSystem( Coords::CoordSystem::getWGS84LLSystem() );
	hs.start_.inl() = 0;
	hs.start_.crl() = 0;
	hs.stop_.inl() = 1000000;
	hs.stop_.crl() = 1000000;
	hs.step_.inl() = 1;
	hs.step_.crl() = 1;
    }

    si_.setRange( cs, false );
    si_.setRange( cs, true );
    Coord crd[3];
    crd[0] = si_.transform( cs.hsamp_.start_ );
    crd[1] = si_.transform( cs.hsamp_.stop_ );
    crd[2] = si_.transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()));
    BinID bid[2];
    bid[0].inl() = cs.hsamp_.start_.inl();
    bid[0].crl() = cs.hsamp_.start_.crl();
    bid[1].inl() = cs.hsamp_.stop_.inl();
    bid[1].crl() = cs.hsamp_.stop_.crl();
    si_.set3PtsWithMsg( crd, bid, cs.hsamp_.stop_.crl() );
    return si_.write( si_.diskLocation().basePath() );
}


uiRetVal EmptyTempSurvey::mount( bool, TaskRunner* )
{
    origsurveyfp_ = IOM().fullSurveyPath();
    IOM().setTempSurvey( si_.diskLocation() );
    BufferString errmsg;
    if ( !IOM().validSurveySetup(errmsg) )
    {
	lasterrs_ = toUiString( errmsg );
	IOM().setSurvey( origsurveyfp_ );
	origsurveyfp_.setEmpty();
    }

    return lasterrs_;
}


uiRetVal EmptyTempSurvey::unmount( bool dosave, TaskRunner* )
{
    IOM().cancelTempSurvey();
    if ( dosave )
	return save();

    if ( !ismanaged_ )
	File::removeDir( tmpbasedir_ );

    return lasterrs_;
}


void EmptyTempSurvey::setSaveLocation( const char* saveloc )
{
    saveloc_.setEmpty();
    saveloc_.set( saveloc );
}

uiRetVal EmptyTempSurvey::activate()
{
    return lasterrs_;
}


uiRetVal EmptyTempSurvey::save( TaskRunner* )
{
    if ( saveloc_.isEmpty() )
    {
	lastwarning_ = tr("Location to save project not specified, "
	    "it will be saved as zip folder at %1").arg( File::getTempPath() );
	saveloc_.set( File::getTempPath() );
    }

    if ( !File::isWritable(saveloc_) )
    {
	lasterrs_.set(
		    tr("User has no write permission at %1").arg(saveloc_) );
	return lasterrs_;
    }

    if ( File::exists(surveyfile_) && File::isReadable(surveyfile_) )
    {
	FilePath backup_fp( saveloc_, "SurveySetup" );
	backup_fp.setExtension( "zip" );
	zipfileloc_ = backup_fp.fullPath();
	if ( File::exists( zipfileloc_ ) )
	    File::remove( zipfileloc_ );
    }

    uiString errmsg;
    if ( !ZipUtils::makeZip(zipfileloc_,surveyfile_,errmsg) )
    {
	lasterrs_.set( errmsg );
	if ( File::exists(zipfileloc_) )
	    File::removeDir( zipfileloc_ );
    }

    return lasterrs_;
}
