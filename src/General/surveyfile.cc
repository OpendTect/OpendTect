/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "surveyfile.h"

#include "commandlineparser.h"
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


extern "C" { mGlobal(Basic) void SetCurBaseDataDir(const char*); }

SurveyCreator::SurveyCreator( const char* survnm, const char* dr,
			      bool ismanaged )
    : surveynm_(survnm)
    , ismanaged_(ismanaged)
{
    if ( surveynm_.isEmpty() )
	surveynm_ = "Temporary Survey";

    BufferString dataroot( dr );
    lasterrs_ = tr("Cannot set a temporary data root");
    if ( dataroot.isEmpty() )
    {
	const CommandLineParser clp;
	BufferString clpdr;
	if ( clp.getVal(CommandLineParser::sDataRootArg(),clpdr) &&
	     IOMan::isValidDataRoot(clpdr.buf()).isOK() )
	    dataroot.set( clpdr.buf() );
    }

    BufferString surveydirnm = surveynm_;
    surveydirnm.trimBlanks().clean();

    if ( dataroot.isEmpty() )
    {
	dataroot = IOMan::getNewTempDataRootDir();
	owndataroot_ = !dataroot.isEmpty() && File::exists( dataroot.buf() );
    }
    else
    {
	const bool hasexisting = File::exists( dataroot.buf() );
	owndataroot_ = !hasexisting;
	if ( !hasexisting && !File::createDir(dataroot.buf()) )
	    return;

	if ( !IOMan::prepareDataRoot(dataroot.buf()) )
	{
	    if ( !hasexisting )
		File::removeDir( dataroot.buf() );
	    return;
	}
    }

    const uiRetVal uirv = IOMan::isValidDataRoot( dataroot );
    if ( uirv.isOK() )
    {
	lasterrs_.setEmpty();
	surveyloc_ = new SurveyDiskLocation( surveydirnm.buf(), dataroot.buf());
    }
}


SurveyCreator::~SurveyCreator()
{
    if ( mounted_ )
	unmount( false );

    if ( surveyloc_ && ismanaged_ && owndataroot_ &&
	 File::exists(surveyloc_->basePath().buf()) )
	File::removeDir( surveyloc_->basePath().buf() );

    delete changer_;
    delete surveyloc_;
}


bool SurveyCreator::isOK() const
{
    if ( !surveyloc_ || !lasterrs_.isOK() )
	return false;

    if ( mounted_ )
    {
	const uiRetVal uirv =
			IOMan::isValidSurveyDir(surveyloc_->fullPath().str());
	if ( !uirv.isOK() )
	    lasterrs_ = uirv;
    }

    return lasterrs_.isOK();
}


BufferString SurveyCreator::getTempBaseDir() const
{
    BufferString ret;
    if ( surveyloc_ )
	ret.set( surveyloc_->basePath() );

    return ret;
}


BufferString SurveyCreator::getSurveyDir() const
{
    BufferString ret;
    if ( surveyloc_ )
	ret.set( surveyloc_->dirName() );

    return ret;
}


uiRetVal SurveyCreator::mount( bool isnew, TaskRunner* trun )
{
    if ( !surveyloc_ )
	return lasterrs_;

    if ( isnew && !surveyloc_->exists() )
    {
	lasterrs_ = tr("Temporary project creation failed."
			    "\nTarget folder no longer exists at '%1'")
					    .arg( surveyloc_->fullPath() );
	return lasterrs_;
    }
    else if ( !isnew && surveyloc_->exists() )
    {
	lasterrs_ = tr("Temporary project creation failed."
		"\nTarget folder exists at '%1'").arg( surveyloc_->fullPath() );
	return lasterrs_;
    }

    if ( !isnew )
    {
	if ( !createSurvey(trun) )
	    return lasterrs_;
    }

    if ( lasterrs_.isOK() )
	mounted_ = true;

    return lasterrs_;
}


uiRetVal SurveyCreator::activate()
{
    if ( !surveyloc_ )
	return lasterrs_;

    if ( !mounted_ )
    {
	lasterrs_.set( tr("%1 is not mounted").arg(surveyloc_->fullPath()) );
	return lasterrs_;
    }

    deleteAndNullPtr( changer_ );
    if ( IOMan::isOK() )
    {
	changer_ = new SurveyChanger( *surveyloc_ );
	lasterrs_ = changer_->message();
    }
    else
	lasterrs_ = IOMan::setDataSource( surveyloc_->fullPath() );

    return lasterrs_;
}

uiRetVal SurveyCreator::deactivate()
{
    const bool hasprevious = changer_;
    deleteAndNullPtr( changer_ );
    if ( !hasprevious )
    {
	SetCurBaseDataDir( nullptr );
	IOMan::newSurvey( nullptr );
    }

    return uiRetVal::OK();
}


uiRetVal SurveyCreator::save( TaskRunner* trun )
{
    if ( !surveyloc_ )
	return lasterrs_;

    if ( !mounted_ )
    {
	lasterrs_ = tr("%1 is not mounted").arg( surveyloc_->fullPath() );
	return lasterrs_;
    }

    if ( !isOK() )
	return lasterrs_;

    BufferString backupfile;
    const BufferString targetzipfnm = getZipArchiveLocation();
    if ( targetzipfnm.isEmpty() )
    {
	lasterrs_ = tr("Cannot save project: No output path provided");
	return lasterrs_;
    }

    if ( File::exists(targetzipfnm.buf()) &&
	 File::isReadable(targetzipfnm.buf()) )
    {
	FilePath backup_fp( targetzipfnm );
	backup_fp.setExtension( bckupExtStr() );
	backupfile = backup_fp.fullPath();
	if ( File::exists(backupfile.buf()) )
	    File::remove( backupfile.buf() );

	File::rename( targetzipfnm.buf(), backupfile.buf() );
    }

    uiString errmsg;
    if ( !ZipUtils::makeZip(targetzipfnm.buf(),surveyloc_->fullPath(),
			    errmsg,trun) )
    {
	lasterrs_.set( errmsg );
	if ( File::exists(backupfile.buf()) &&
	     File::remove(targetzipfnm.buf()) )
	    File::rename( backupfile.buf(), targetzipfnm.buf() );
    }

    return lasterrs_;
}


uiRetVal SurveyCreator::unmount( bool dosave, TaskRunner* /* trun */ )
{
    if ( !surveyloc_ )
	return lasterrs_;

    if ( isManaged() && surveyloc_->isCurrentSurvey() )
	deactivate();

    if ( surveyloc_->exists() && dosave )
	lasterrs_ = save();

    if ( !mounted_ )
	return lasterrs_;

    if ( ismanaged_ && owndataroot_ &&
	 File::exists(surveyloc_->basePath().buf()) )
	File::removeDir( surveyloc_->basePath().buf() );

    mounted_ = false;

    return lasterrs_;
}



// SurveyFile

BufferString SurveyFile::filtStr()
{
    BufferString filt( "OpendTect project files (*." );
    filt.add( extStr() );
    filt.add( ");;All Files(*)" );
    return filt;
}


SurveyFile::SurveyFile( const char* survfilenm, bool automount, bool ismanaged )
    : SurveyCreator(nullptr,nullptr,ismanaged)
    , surveyfile_(survfilenm)
{
    readSurveyDirNameFromFile();
    if ( automount && isOK() )
	mount();
}


SurveyFile::~SurveyFile()
{
}


void SurveyFile::readSurveyDirNameFromFile()
{
    lasterrs_.setEmpty();
    if ( surveyfile_.isEmpty() || !File::exists(surveyfile_.buf()) )
    {
	lasterrs_.set( uiStrings::phrCannotOpenForRead( surveyfile_ ) );
	return;
    }

    BufferStringSet fnms;
    uiString errmsg;
    if ( !ZipUtils::makeFileList(surveyfile_,fnms,errmsg) )
    {
	lasterrs_.set( errmsg );
	return;
    }

    if ( fnms.isEmpty() )
    {
	lasterrs_.set( uiStrings::phrInvalid( tr("project file") ) );
	return;
    }

    BufferString survnm( fnms.get(0) );
    const BufferString omf( survnm, ".omf" );
    const bool isvalidsurvey = fnms.indexOf( omf ) > -1;
    if ( !isvalidsurvey )
    {
	lasterrs_.set( uiStrings::phrInvalid( tr("project file") ) );
	return;
    }

    if ( survnm.last() == '/' )
	survnm.last() = '\0';

    if ( surveyloc_ )
	surveyloc_->setDirName( survnm );

    surveynm_.set( survnm );
}


bool SurveyFile::createSurvey( TaskRunner* trun )
{
    lasterrs_.setEmpty();
    if ( !File::exists(surveyfile_) )
    {
	lasterrs_.set(
		tr("Cannot mount file: %1 does not exist").arg(surveyfile_) );
	return false;
    }

    uiString errmsg;
    const BufferString tmpbasedir = surveyloc_->basePath();
    if ( !ZipUtils::unZipArchive(surveyfile_,tmpbasedir.buf(),errmsg,trun) )
    {
	lasterrs_.set( errmsg );
	return false;
    }

    return lasterrs_.isOK();
}


uiRetVal SurveyFile::activate()
{
    const uiRetVal uirv = SurveyCreator::activate();
    if ( uirv.isOK() )
	surveynm_ = SI().name();

    return uirv;
}



// EmptyTempSurvey

EmptyTempSurvey::EmptyTempSurvey( const char* survnm, const char* dataroot,
				  bool automount, bool ismanaged )
    : SurveyCreator(survnm,dataroot,ismanaged)
{
    setSaveLocation( nullptr );
    if ( automount && isOK() )
	mount();
}


EmptyTempSurvey::EmptyTempSurvey( const OD::JSON::Object& obj, bool automount,
				  bool ismanaged )
    : SurveyCreator(
	    obj.getStringValue(CommandLineParser::sSurveyArg()).buf(),
	    obj.getStringValue(CommandLineParser::sDataRootArg()).buf(),
	    ismanaged)
{
    createpars_ = obj.clone();
    const BufferString saveloc = obj.getStringValue( sKeySaveLoc() );
    setSaveLocation( saveloc.isEmpty() ? nullptr : saveloc.buf() );

    if ( automount && isOK() )
	mount();
}


EmptyTempSurvey::~EmptyTempSurvey()
{
    delete createpars_;
}


bool EmptyTempSurvey::createSurvey( TaskRunner* /* trun */ )
{
    const BufferString basicsurv =
			mGetSetupFileName( SurveyInfo::sKeyBasicSurveyName() );
    if ( !File::exists(basicsurv) )
    {
	lasterrs_ = uiStrings::phrCannotRead( basicsurv.buf() );
	return false;
    }

    const BufferString newsurv = surveyloc_->fullPath();
    BufferString emsg;
    const bool isok = File::copy( basicsurv.buf(), newsurv.buf(), &emsg );
    if ( !isok || !File::exists(newsurv.buf()) )
    {
	lasterrs_ = tr("Failed to mount temporary survey at '%1' with error %2")
						    .arg( newsurv.buf() )
						    .arg( emsg.buf() );
	return false;
    }

    return writeSurveyInfo();
}


bool EmptyTempSurvey::writeSurveyInfo()
{
    const OD::JSON::Object* obj = createpars_;
    SurveyInfo si = SurveyInfo::empty();
    si.setName( surveynm_.buf() );
    si.disklocation_ = *surveyloc_;

    TrcKeyZSampling cs( false );
    TrcKeySampling& hs = cs.hsamp_;
    Coord crd[3];
    if ( obj )
    {
	const BufferString zdom( obj->getStringValue(sKey::ZDomain()) );
	const bool istime = zdom.isEqual( sKey::Time() );
	const bool isfeet = obj->getBoolValue( SurveyInfo::sKeyDpthInFt() );
	si.setXYInFeet( false );
	si.setZUnit( istime, isfeet );
	si.getPars().setYN( SurveyInfo::sKeyDpthInFt(), isfeet );
	const double srd = obj->getDoubleValue(
				    SurveyInfo::sKeySeismicRefDatum() );
	si.setSeismicReferenceDatum( srd );
	hs.start_.inl() = obj->getDoubleValue( sKey::FirstInl() );
	hs.start_.crl() = obj->getDoubleValue( sKey::FirstCrl() );
	hs.stop_.inl() = obj->getDoubleValue( sKey::LastInl() );
	hs.stop_.crl() = obj->getDoubleValue( sKey::LastCrl() );
	hs.step_.inl() = obj->getDoubleValue( sKey::StepInl() );
	hs.step_.crl() = obj->getDoubleValue( sKey::StepCrl() );
	//TODO: set crd[0]. crd[1], crd[2]
	const int crsid = obj->getIntValue( sKeyCRSID() );
	if ( crsid >= 0 )
	{
	    IOPar crspar;
	    crspar.set( Coords::CoordSystem::sKeyFactoryName(),
			sKey::ProjSystem() );
	    crspar.set( IOPar::compKey(sKey::Projection(),sKey::ID()), crsid );
	    RefMan<Coords::CoordSystem> coordsys =
				Coords::CoordSystem::createSystem( crspar );
	    si.setCoordSystem( coordsys );
	    si.setXYInFeet( coordsys->isFeet() );
	}
	else
	{
	    RefMan<Coords::CoordSystem> coordsys =
				Coords::CoordSystem::getWGS84LLSystem();
	    si.setCoordSystem( coordsys );
	    si.setXYInFeet( coordsys->isFeet() );
	}
    }
    else
    {
	si.setXYInFeet( false );
	si.setZUnit( true );
	si.getPars().setYN( SurveyInfo::sKeyDpthInFt(), false );
	si.setSeismicReferenceDatum( 0. );
	hs.start_.inl() = hs.start_.crl() = 0;
	hs.stop_.inl() = hs.stop_.crl() = 1000000;
	hs.step_.inl() = hs.step_.crl() = 1;
	crd[0] = Coord( 0., 0. );
	crd[1] = Coord( 1000000000., 1000000000. );
	crd[2] = Coord( 0., 1000000000. );
	si.setCoordSystem( Coords::CoordSystem::getWGS84LLSystem() );
    }

    si.setRange( cs, false );
    si.setRange( cs, true );
    BinID bid[2];
    bid[0].inl() = cs.hsamp_.start_.inl();
    bid[0].crl() = cs.hsamp_.start_.crl();
    bid[1].inl() = cs.hsamp_.stop_.inl();
    bid[1].crl() = cs.hsamp_.stop_.crl();
    si.set3PtsWithMsg( crd, bid, cs.hsamp_.stop_.crl() );

    return si.write( surveyloc_->basePath().buf() );
}


void EmptyTempSurvey::setSaveLocation( const char* saveloc )
{
    if ( saveloc && *saveloc )
    {
	zipfileloc_.set( saveloc );
	return;
    }

    if ( !surveyloc_ )
	return;

    FilePath savefp( surveyloc_->basePath() );
    savefp.setFileName( nullptr );
    const BufferString savenm = FilePath::getTempFileName("project", extStr() );
    savefp.add( savenm.buf() );
    zipfileloc_.set( savefp.fullPath() );
}
