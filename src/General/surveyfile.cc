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
#include "uistrings.h"
#include "uistringset.h"
#include "ziputils.h"

#include "hiddenparam.h"


extern "C" { mGlobal(Basic) void SetCurBaseDataDir(const char*); }

namespace Survey
{

static SurveyDiskLocation* init( const char* survnm, const char* dr,
				 bool ismanaged, BufferString& surveynm_,
				 bool& owndataroot_, uiRetVal& lasterrs_ )
{
    if ( surveynm_.isEmpty() )
	surveynm_ = "Temporary Survey";

    BufferString dataroot( dr );
    if ( dataroot.isEmpty() )
    {
	const CommandLineParser clp;
	BufferString clpdr;
	if ( clp.getVal(CommandLineParser::sDataRootArg(),clpdr) &&
	     IOMan::isValidDataRoot(clpdr.buf()) )
	    dataroot.set( clpdr.buf() );
    }

    lasterrs_ = od_static_tr("TempSurvey", "Cannot set a temporary data root");
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
	    return nullptr;

	if ( !IOMan::prepareDataRoot(dataroot.buf()) )
	{
	    if ( !hasexisting )
		File::removeDir( dataroot.buf() );
	    return nullptr;
	}
    }

    BufferString surveydirnm = surveynm_;
    surveydirnm.trimBlanks().clean();

    if ( IOMan::isValidDataRoot(dataroot.buf()) )
    {
	lasterrs_.setEmpty();
	return new SurveyDiskLocation( surveydirnm.buf(), dataroot.buf() );
    }

    return nullptr;
}

} // namespace Survey


BufferString SurveyFile::filtStr()
{
    BufferString filt( "OpendTect project files (*." );
    filt.add( extStr() );
    filt.add( ");;All Files(*)" );
    return filt;
}


static HiddenParam<SurveyFile,SurveyDiskLocation*>
						surveyfilesdlmgr_(nullptr);
static HiddenParam<SurveyFile,SurveyChanger*>
					surveyfilesurvchangermgr_( nullptr );
static HiddenParam<SurveyFile,int> surveyfileowndrmgr_(0);
static HiddenParam<SurveyFile,int> surveyfileismanagedmgr_(0);

SurveyFile::SurveyFile( const char* survfilenm, bool automount )
    : surveyfile_(survfilenm)
{
    bool owndataroot_ = false;
    const bool ismanaged = true;
    SurveyDiskLocation* surveyloc_ = Survey::init( nullptr, nullptr, ismanaged,
					   surveydirnm_, owndataroot_,
					   lasterrs_ );
    surveyfilesdlmgr_.setParam( this, surveyloc_ );
    surveyfileowndrmgr_.setParam( this, owndataroot_ ? 1 : 0);
    surveyfileismanagedmgr_.setParam( this, ismanaged ? 1 : 0 );
    surveyfilesurvchangermgr_.setParam( this, nullptr );

    readSurveyDirNameFromFile();
    if ( automount && isOK() )
	mount();
}


SurveyFile::SurveyFile( const char* survfilenm, const char* )
    : SurveyFile(survfilenm,false)
{
}


SurveyFile::~SurveyFile()
{
    if ( mounted_ )
	unmount( false );

    const bool owndataroot_ = surveyfileowndrmgr_.getParam(this) == 1;
    const SurveyDiskLocation* surveyloc_ = surveyfilesdlmgr_.getParam( this );
    if ( surveyloc_ && isManaged() && owndataroot_ &&
	 File::exists(surveyloc_->basePath().buf()) )
	File::removeDir( surveyloc_->basePath().buf() );

    surveyfilesurvchangermgr_.removeAndDeleteParam( this );
    surveyfilesdlmgr_.removeAndDeleteParam( this );
    surveyfileowndrmgr_.removeParam( this );
    surveyfileismanagedmgr_.removeParam( this );
}


void SurveyFile::setManaged( bool yn )
{
    surveyfileismanagedmgr_.setParam( this, yn ? 1 : 0 );
}


bool SurveyFile::isManaged() const
{
    return surveyfileismanagedmgr_.getParam( this ) == 1;
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

    SurveyDiskLocation* surveyloc_ = surveyfilesdlmgr_.getParam( this );
    if ( surveyloc_ )
	surveyloc_->setDirName( survnm );

    surveydirnm_.set( survnm );
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

    const SurveyDiskLocation* surveyloc_ = surveyfilesdlmgr_.getParam( this );
    uiString errmsg;
    const BufferString tmpbasedir = surveyloc_->basePath();
    if ( !ZipUtils::unZipArchive(surveyfile_,tmpbasedir.buf(),errmsg,trun) )
    {
	lasterrs_.set( errmsg );
	return false;
    }

    return lasterrs_.isOK();
}


uiRetVal SurveyFile::mount( bool isnew, TaskRunner* trun )
{
    const SurveyDiskLocation* surveyloc_ = surveyfilesdlmgr_.getParam( this );
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


uiRetVal SurveyFile::activate()
{
    const SurveyDiskLocation* surveyloc_ = surveyfilesdlmgr_.getParam( this );
    if ( !surveyloc_ )
	return lasterrs_;

    if ( !mounted_ )
    {
	lasterrs_.set( tr("%1 is not mounted").arg(surveyloc_->fullPath()) );
	return lasterrs_;
    }

    surveyfilesurvchangermgr_.deleteAndNullPtrParam( this );
    if ( IOMan::isOK() )
    {
	auto* changer_ = new SurveyChanger( *surveyloc_ );
	lasterrs_ = changer_->message();
	surveyfilesurvchangermgr_.setParam( this, changer_ );
    }
    else
	lasterrs_ = IOMan::setDataSource_( surveyloc_->fullPath() );

    if ( lasterrs_.isOK() )
	surveydirnm_ = SI().name();

    return lasterrs_;
}


uiRetVal SurveyFile::deactivate()
{
    const bool hasprevious = surveyfilesurvchangermgr_.getParam( this );
    surveyfilesurvchangermgr_.deleteAndNullPtrParam( this );
    if ( !hasprevious )
    {
	SetCurBaseDataDir( nullptr );
	IOMan::newSurvey( nullptr );
    }

    return uiRetVal::OK();
}


const char* SurveyFile::bckupExtStr()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
	ret.set( extStr() ).add( "_bck" );
    return ret.str();
}


uiRetVal SurveyFile::save( TaskRunner* trun )
{
    const SurveyDiskLocation* surveyloc_ = surveyfilesdlmgr_.getParam( this );
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


uiRetVal SurveyFile::unmount( bool dosave, TaskRunner* /* trun */ )
{
    const SurveyDiskLocation* surveyloc_ = surveyfilesdlmgr_.getParam( this );
    if ( !surveyloc_ )
	return lasterrs_;

    if ( isManaged() && surveyloc_->isCurrentSurvey() )
	deactivate();

    if ( surveyloc_->exists() && dosave )
	lasterrs_ = save();

    if ( !mounted_ )
	return lasterrs_;

    const bool owndataroot_ = surveyfileowndrmgr_.getParam(this) == 1;
    if ( isManaged() && owndataroot_ &&
	 File::exists(surveyloc_->basePath().buf()) )
	File::removeDir( surveyloc_->basePath().buf() );

    mounted_ = false;

    return lasterrs_;
}



static HiddenParam<EmptyTempSurvey,SurveyDiskLocation*>
						emptysurveysdlmgr_(nullptr);
static HiddenParam<EmptyTempSurvey,const OD::JSON::Object*>
					    emptysurveycreateparsmgr_(nullptr);
static HiddenParam<EmptyTempSurvey,SurveyChanger*>
					emptysurveyssurvchangermgr_( nullptr );
static HiddenParam<EmptyTempSurvey,int> emptysurveyowndrmgr_(0);

EmptyTempSurvey::EmptyTempSurvey( const char* survnm, const char* dataroot,
				  bool automount )
    : surveydirnm_(survnm)
    , ismanaged_(true)
{
    bool owndataroot_ = false;
    SurveyDiskLocation* surveyloc_ = Survey::init( survnm, dataroot, ismanaged_,
					   surveydirnm_, owndataroot_,
					   lasterrs_ );
    emptysurveysdlmgr_.setParam( this, surveyloc_ );
    emptysurveyowndrmgr_.setParam( this, owndataroot_ ? 1 : 0);
    emptysurveyssurvchangermgr_.setParam( this, nullptr );
    emptysurveycreateparsmgr_.setParam( this, nullptr );
    setSaveLocation( nullptr );
    if ( automount && isOK() )
	mount();
}


EmptyTempSurvey::EmptyTempSurvey( const OD::JSON::Object& obj )
    : ismanaged_(true)
{
    bool owndataroot_ = false;
    const BufferString survdirnm =
			obj.getStringValue( CommandLineParser::sSurveyArg() );
    const BufferString dataroot =
			obj.getStringValue( CommandLineParser::sDataRootArg() );
    SurveyDiskLocation* surveyloc_ = Survey::init( survdirnm.buf(),
			dataroot.buf(), ismanaged_, surveydirnm_, owndataroot_,
			lasterrs_ );
    emptysurveysdlmgr_.setParam( this, surveyloc_ );
    emptysurveyowndrmgr_.setParam( this, owndataroot_ ? 1 : 0);
    emptysurveyssurvchangermgr_.setParam( this, nullptr );

    emptysurveycreateparsmgr_.setParam( this, obj.clone() );
    const BufferString saveloc = obj.getStringValue( sKeySaveLoc() );
    setSaveLocation( saveloc.isEmpty() ? nullptr : saveloc.buf() );

    const bool automount = true;
    if ( automount && isOK() )
	mount();
}


EmptyTempSurvey::~EmptyTempSurvey()
{
    if ( mounted_ )
	unmount( false );

    const bool owndataroot_ = emptysurveyowndrmgr_.getParam(this) == 1;
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
    if ( surveyloc_ && ismanaged_ && owndataroot_ &&
	 File::exists(surveyloc_->basePath().buf()) )
	File::removeDir( surveyloc_->basePath().buf() );

    emptysurveyssurvchangermgr_.removeAndDeleteParam( this );
    emptysurveysdlmgr_.removeAndDeleteParam( this );
    emptysurveyowndrmgr_.removeParam( this );
    emptysurveycreateparsmgr_.removeAndDeleteParam( this );
}


bool EmptyTempSurvey::isOK() const
{
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
    if ( !surveyloc_ || !lasterrs_.isOK() )
	return false;

    if ( mounted_ )
    {
	if ( !IOMan::isValidSurveyDir(surveyloc_->fullPath().str()) )
	    mSelf().lasterrs_ = IOMan::isValidMsg();
    }

    return lasterrs_.isOK();
}


BufferString EmptyTempSurvey::getTempBaseDir() const
{
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
    BufferString ret;
    if ( surveyloc_ )
	ret.set( surveyloc_->basePath() );

    return ret;
}


BufferString EmptyTempSurvey::getSurveyDir() const
{
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
    BufferString ret;
    if ( surveyloc_ )
	ret.set( surveyloc_->dirName() );

    return ret;
}


bool EmptyTempSurvey::createOMFFile()
{ return false; }

bool EmptyTempSurvey::initSurvey( const OD::JSON::Object* /* obj */ )
{ return false; }


uiRetVal EmptyTempSurvey::mount( bool isnew, TaskRunner* trun )
{
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
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
	if ( !createTempSurveySetup(true) )
	    return lasterrs_;
    }

    if ( lasterrs_.isOK() )
	mounted_ = true;

    return lasterrs_;
}


uiRetVal EmptyTempSurvey::activate()
{
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
    if ( !surveyloc_ )
	return lasterrs_;

    if ( !mounted_ )
    {
	lasterrs_.set( tr("%1 is not mounted").arg(surveyloc_->fullPath()) );
	return lasterrs_;
    }

    emptysurveyssurvchangermgr_.deleteAndNullPtrParam( this );
    if ( IOMan::isOK() )
    {
	auto* changer_ = new SurveyChanger( *surveyloc_ );
	lasterrs_ = changer_->message();
	emptysurveyssurvchangermgr_.setParam( this, changer_ );
    }
    else
	lasterrs_ = IOMan::setDataSource_( surveyloc_->fullPath() );

    return lasterrs_;
}


uiRetVal EmptyTempSurvey::deactivate()
{
    const bool hasprevious = emptysurveyssurvchangermgr_.getParam( this );
    emptysurveyssurvchangermgr_.deleteAndNullPtrParam( this );
    if ( !hasprevious )
    {
	SetCurBaseDataDir( nullptr );
	IOMan::newSurvey( nullptr );
    }

    return uiRetVal::OK();
}


uiRetVal EmptyTempSurvey::save( TaskRunner* trun )
{
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
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
	backup_fp.setExtension( SurveyFile::bckupExtStr() );
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


uiRetVal EmptyTempSurvey::unmount( bool dosave, TaskRunner* trun )
{
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
    if ( !surveyloc_ )
	return lasterrs_;

    if ( isManaged() && surveyloc_->isCurrentSurvey() )
	deactivate();

    if ( surveyloc_->exists() && dosave )
	lasterrs_ = save();

    if ( !mounted_ )
	return lasterrs_;

    const bool owndataroot_ = emptysurveyowndrmgr_.getParam(this) == 1;
    if ( ismanaged_ && owndataroot_ &&
	 File::exists(surveyloc_->basePath().buf()) )
	File::removeDir( surveyloc_->basePath().buf() );

    mounted_ = false;

    return lasterrs_;
}


bool EmptyTempSurvey::createTempSurveySetup( bool /* hasomf */ )
{
    const BufferString basicsurv =
			mGetSetupFileName( SurveyInfo::sKeyBasicSurveyName() );
    if ( !File::exists(basicsurv) )
    {
	lasterrs_ = uiStrings::phrCannotRead( basicsurv.buf() );
	return false;
    }

    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
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

    return fillSurveyInfo( nullptr );
}


bool EmptyTempSurvey::fillSurveyInfo( const OD::JSON::Object* /* obj */ )
{
    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
    const OD::JSON::Object* obj = emptysurveycreateparsmgr_.getParam( this );
    SurveyInfo si = SurveyInfo::empty();
    si.setName( surveydirnm_.buf() );
    si.disklocation_ = *surveyloc_;

    TrcKeyZSampling cs;
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

    const SurveyDiskLocation* surveyloc_ = emptysurveysdlmgr_.getParam( this );
    if ( !surveyloc_ )
	return;

    FilePath savefp( surveyloc_->basePath() );
    savefp.setFileName( nullptr );
    const BufferString savenm = FilePath::getTempFileName("project",
							SurveyFile::extStr() );
    savefp.add( savenm.buf() );
    zipfileloc_.set( savefp.fullPath() );
}


