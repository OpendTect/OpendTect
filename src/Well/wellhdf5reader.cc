/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2018
-*/


#include "wellhdf5reader.h"
#include "wellhdf5writer.h"

#include "file.h"
#include "filepath.h"
#include "hdf5arraynd.h"
#include "ioobj.h"
#include "jsonkeystrs.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odjson.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welldisp.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"

// Groups
const char* Well::HDF5Access::sTrackGrpName()	{ return "Track"; }
const char* Well::HDF5Access::sLogsGrpName()	{ return "Logs"; }
const char* Well::HDF5Access::sMarkersGrpName() { return "Markers"; }
const char* Well::HDF5Access::sTDsGrpName()	{ return "Time-Depth Models"; }
const char* Well::HDF5Access::sCSsGrpName()	{ return "Checkshots"; }
const char* Well::HDF5Access::sDispParsGrpName(){ return "Display Parameters"; }

// Datasets
const char* Well::HDF5Access::sCoordsDSName()	{ return "Coordinates"; }
const char* Well::HDF5Access::sMDsDSName()	{ return "MDs"; }
const char* Well::HDF5Access::sTWTsDSName()	{ return "TWTs"; }
const char* Well::HDF5Access::sValuesDSName()	{ return "Values"; }
const char* Well::HDF5Access::sNamesDSName()	{ return "Names"; }
const char* Well::HDF5Access::sColorsDSName()	{ return "Colors"; }
const char* Well::HDF5Access::sLvlIDsDSName()	{ return "LevelIDs"; }
const char* Well::HDF5Access::sKeyLogDel()	{ return "Deleted"; }


bool Well::HDF5Access::isParallelEnabled()
{
    return HDF5::isAvailable() && HDF5::isParallelEnabled();
}


Well::HDF5Reader::HDF5Reader( const char* fnm, Well::Data& wd,
			      uiString& errmsg )
    : Well::ReadAccess(wd)
    , errmsg_(errmsg)
    , lock_(true)
{
    FilePath fp( fnm );
    fp.setExtension( nullptr );
    wd_.info().setName( fp.fileName() );
    init( fnm );
}


Well::HDF5Reader::HDF5Reader( const IOObj& ioobj, Well::Data& wd,
			      uiString& errmsg )
    : Well::ReadAccess(wd)
    , errmsg_(errmsg)
    , lock_(true)
{
    wd_.info().setName( ioobj.name() );
    wd_.setMultiID( ioobj.key() );
    init( ioobj.mainFileName() );
}


Well::HDF5Reader::HDF5Reader( const HDF5Writer& wrr, Well::Data& wd,
			      uiString& errmsg )
    : Well::ReadAccess(wd)
    , errmsg_(errmsg)
    , rdr_(wrr.createCoupledHDFReader())
    , lock_(true)
{
    if ( !rdr_ )
    {
	pErrMsg("No coupled reader created");
	return;
    }
}


Well::HDF5Reader::~HDF5Reader()
{
}


#define mErrRet( s )	{ errmsg_ = s; return; }

void Well::HDF5Reader::init( const char* fnm )
{
    if ( !HDF5::isAvailable() )
	mErrRet( HDF5::Access::sHDF5NotAvailable(fnm) )
    if ( !File::exists(fnm) )
	mErrRet( uiStrings::phrFileDoesNotExist(fnm) )
    if ( !HDF5::isHDF5File(fnm) )
	mErrRet( HDF5::Access::sNotHDF5File(fnm) )

    rdr_ = HDF5::mkReader().release();
    if ( !rdr_ )
    {
	pErrMsg("Available but no reader?");
	return;
    }

    uiRetVal uirv = rdr_->open( fnm );
    if ( !uirv.isOK() )
    {
	rdr_ = nullptr;
	mErrRet( uirv )
    }
}


bool Well::HDF5Reader::canReadInParallel() const
{
    return HDF5Access::isParallelEnabled();
}


bool Well::HDF5Reader::ensureFileOpen() const
{
    Threads::Locker locker( lock_ );
    if ( rdr_ && rdr_->isOpen() )
    {
	if ( loggrps_.isEmpty() )
	{
	    const HDF5::DataSetKey dsky( sLogsGrpName() );
	    rdr_->getSubGroups( sLogsGrpName(), getNonConst(loggrps_) );
	}

	return true;
    }

    if ( errmsg_.isEmpty() )
	errmsg_.set( HDF5::Access::sHDF5FileNoLongerAccessible() );

    return false;
}


bool Well::HDF5Reader::getInfo() const
{
    if ( !ensureFileOpen() )
	return false;

    infoiop_.setEmpty();
    uiRetVal uirv = rdr_->get( infoiop_ );
    mErrRetIfUiRvNotOK( uirv );

    wd_.info().usePar( infoiop_ );
    return true;
}


bool Well::HDF5Reader::getTrack() const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sTrackGrpName(), "" );
    dsky.setDataSetName( sCoordsDSName() );

    uiRetVal uirv;
    const int sz = rdr_->dimSize( dsky, 0, uirv );
    if ( sz < 1 )
	return false;

    Array2DImpl<double> arr( sz, 3 );
    TypeSet<double> mds;
    if ( !arr.isOK() || !mds.setCapacity(sz,false) )
	return false;

    HDF5::ArrayNDTool<double> arrtool( arr );
    auto& rdr = cCast(HDF5::Reader&,*rdr_);
    uirv = arrtool.getAll( dsky, rdr );
    mErrRetIfUiRvNotOK( uirv );

    dsky.setDataSetName( sMDsDSName() );
    uirv = rdr_->get( dsky, mds );
    mErrRetIfUiRvNotOK( uirv );
    IOPar mdiop;
    uirv = rdr_->get( mdiop, &dsky );

    Well::Track& trck = wd_.track();
    trck.setEmpty();
    for ( int idx=0; idx<sz; idx++ )
    {
	const double& dah = mds[idx];
	const Coord3 crd( arr.get(idx,0), arr.get(idx,1), arr.get(idx,2) );
	trck.addPoint( crd, dah );
    }

    return true;
}


bool Well::HDF5Reader::doGetD2T( bool csmdl ) const
{
    if ( !ensureFileOpen() )
	return false;

    PtrMan<D2TModel> d2tmodel = new D2TModel;
    if ( !d2tmodel )
	return false;

    const int modelid = 0; // TODO: suppport multiple models
    const HDF5::DataSetKey grpky =
	HDF5::DataSetKey::groupKey( csmdl ? sCSsGrpName() : sTDsGrpName(),
				    toString(modelid) );
    HDF5::DataSetKey dsky( nullptr, sMDsDSName() );
    dsky.setGroupName( grpky.fullDataSetName() );
    if ( !rdr_->hasDataSet(dsky) )
	return true;

    IOPar hdriop;
    uiRetVal uirv = rdr_->get( hdriop, &dsky );
    mErrRetIfUiRvNotOK( uirv );
    d2tmodel->useHeaderPar( hdriop );

    const int sz = rdr_->dimSize( dsky, 0, uirv );
    if ( sz < 1 )
	return true;

    TypeSet<double> mds;
    TypeSet<double> times;
    if ( !mds.setCapacity(sz,false) || !times.setCapacity(sz,false) )
	return false;

    uirv = rdr_->get( dsky, mds );
    mErrRetIfUiRvNotOK( uirv );

    dsky.setDataSetName( sTWTsDSName() );
    uirv = rdr_->get( dsky, times );
    mErrRetIfUiRvNotOK( uirv );

    for ( int idx=0; idx<sz; idx++ )
    {
	const double& dah = mds[idx];
	const double& twt = times[idx];
	d2tmodel->insertAtDah( dah, twt );
    }

    if ( !updateDTModel(d2tmodel.release(),csmdl,errmsg_) )
	return false;

    return true;
}


bool Well::HDF5Reader::getD2T() const
{
    return doGetD2T( false );
}


bool Well::HDF5Reader::getCSMdl() const
{
    return doGetD2T( true );
}


bool Well::HDF5Reader::getLogPars( const HDF5::DataSetKey& dsky,
				   IOPar& iop ) const
{
    if ( !rdr_->hasGroup(dsky.fullDataSetName()) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(dsky) );
	return false;
    }

    uiRetVal uirv = rdr_->get( iop, &dsky );
    mErrRetIfUiRvNotOK( uirv )
    return !iop.isTrue( sKeyLogDel() ) && getDefLogs();
}


Well::Log* Well::HDF5Reader::rdLogHdr( const IOPar& iop, int idx )
{
    auto* newlog = new Log;
    if ( !newlog )
	return nullptr;

    if ( iop.isTrue(sKeyLogDel()) )
	return nullptr;

    BufferString lognm;
    if ( !iop.get(sKey::Name(),lognm) || lognm.isEmpty() )
	lognm.set( "[" ).add( idx+1 ).add( "]" );

    newlog->setName( lognm.str() );

    BufferString mnemlbl;
    if ( iop.get(Log::sKeyMnemLbl(),mnemlbl) )
	newlog->setMnemonicLabel( mnemlbl );

    BufferString uomlbl;
    if ( iop.get(Log::sKeyUnitLbl(),uomlbl) )
	newlog->setUnitMeasLabel( uomlbl );

    Interval<float> dahrange;
    if ( iop.get(Log::sKeyDahRange(),dahrange) )
    {
	newlog->addValue( dahrange.start_, mUdf(float) );
	newlog->addValue( dahrange.stop_, mUdf(float) );
	newlog->dahRange().set( dahrange.start_, dahrange.stop_ );
    }

    Interval<float> logrange;
    if ( iop.get(Log::sKeyLogRange(),logrange) )
	newlog->setValueRange( logrange );

    bool havehdrinfo = false;
    if ( iop.getYN(Log::sKeyHdrInfo(),havehdrinfo) && havehdrinfo )
    {
	IOPar logiop = iop;
	logiop.removeWithKey( sKey::Name() );
	logiop.removeWithKey( sKey::DepthUnit() );
	logiop.removeWithKey( Log::sKeyUnitLbl() );
	logiop.removeWithKey( Log::sKeyMnemLbl() );
	logiop.removeWithKey( Log::sKeyDahRange() );
	logiop.removeWithKey( Log::sKeyLogRange() );
	newlog->pars().merge( logiop );
    }

    return newlog;
}


bool Well::HDF5Reader::readLogData( const HDF5::DataSetKey& dsky, Log& wl) const
{
    uiRetVal uirv;
    HDF5::DataSetKey logkey( nullptr, sMDsDSName() );
    logkey.setGroupName( dsky.fullDataSetName() );
    if ( !rdr_->hasDataSet(logkey) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(logkey) );
	return false;
    }

    const int sz = rdr_->dimSize( logkey, 0, uirv );
    if ( sz < 1 )
	return true;

    TypeSet<double> mds;
    TypeSet<double> vals;
    if ( !mds.setCapacity(sz,false) ||
	 !vals.setCapacity(sz,false) )
	return false;

    uirv = rdr_->get( logkey, mds );
    if ( uirv.isError() )
    {
	errmsg_.set( uirv );
	return false;
    }

    logkey.setDataSetName( sValuesDSName() );
    uirv = rdr_->get( logkey, vals );
    if ( uirv.isError() )
    {
	errmsg_.set( uirv );
	return false;
    }

    wl.setEmpty();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = mCast(float,mds[idx]);
	const float val = mCast(float,vals[idx]);
	wl.addValue( dah, val );
    }

    return true;
}


bool Well::HDF5Reader::addLog( const HDF5::DataSetKey& dsky, Log* newlog,
			       bool needjustinfo ) const
{
    IOPar iop;
    if ( !getLogPars(dsky,iop) )
    {
	delete newlog;
	errmsg_ = tr("Cannot read HDF5 log header");
	return false;
    }

    if ( !newlog )
    {
	newlog = rdLogHdr( iop, wd_.logs().size() );
	if ( !newlog )
	{
	    errmsg_ = sCannotReadFileHeader();
	    return false;
	}
    }

    if ( !needsAdd(newlog->name().buf(),needjustinfo) )
	return true;

    if ( !needjustinfo && !readLogData(dsky,*newlog) )
    {
	delete newlog;
	return false;
    }

    return addToLogSet( newlog, needjustinfo );
}


bool Well::HDF5Reader::getLogs( bool needjustinfo ) const
{
    if ( !ensureFileOpen() )
	return false;

    bool haserrors = false;

    HDF5::DataSetKey dsky( sLogsGrpName() );
    for ( const auto* grpnm : loggrps_ )
    {
	dsky.setDataSetName( grpnm->buf() );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	if ( !addLog(grpkey,nullptr,needjustinfo) )
	{
	    errmsg_ = tr("read log data failed for '%2'")
						.arg( rdr_->fileName() );
	    ErrMsg( errmsg_ );
	    errmsg_.setEmpty();
	    haserrors = true;
	    continue;
	}
    }

    haserrors = !getDefLogs() || haserrors;
    return !haserrors;
}


bool Well::HDF5Reader::getLog( const char* lognm ) const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sLogsGrpName() );
    for ( const auto* grpnm : loggrps_ )
    {
	dsky.setDataSetName( grpnm->buf() );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	IOPar iop;
	if ( !getLogPars(grpkey,iop) )
	    return false;

	if ( iop.isTrue(sKeyLogDel()) )
	    continue;

	PtrMan<Log> log = rdLogHdr( iop, wd_.logs().size() );
	if ( log->name() == lognm )
	    return addLog( grpkey, log.release() );
    }

    errmsg_ = tr("Cannot find log '%1' in well '%2'").arg( lognm )
						     .arg( data().name() );
    return false;
}


bool Well::HDF5Reader::getLogByID( const LogID& id ) const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sLogsGrpName() );
    dsky.setDataSetName( toString(id.asInt()) );
    HDF5::DataSetKey grpkey;
    grpkey.setGroupName( dsky.fullDataSetName() );
    IOPar iop;
    if ( !getLogPars(grpkey,iop) )
	return false;

    PtrMan<Log> log = rdLogHdr( iop, wd_.logs().size() );
    return log ? addLog( grpkey, log.release() ) : false;
}


bool Well::HDF5Reader::getDefLogs() const
{
    if ( !ensureFileOpen() )
	return false;

    const HDF5::DataSetKey dsky( sLogsGrpName(), nullptr );
    OD::JSON::Array jsonarr( true );
    const uiRetVal uirv = rdr_->readJSonAttribute( Well::LogSet::sKeyDefMnem(),
						   jsonarr, &dsky );
    if ( !uirv.isOK() || jsonarr.size() < 1 )
	return true; //do not return false if missing, this can happen

    IOPar defmniop;
    for ( int idx=0; idx<jsonarr.size(); idx++ )
    {
	const OD::JSON::Object& subobj = jsonarr.object( idx );
	const BufferString key = subobj.getStringValue( sJSONKey::Mnemonic() );
	const BufferString val = subobj.getStringValue( sJSONKey::Log() );
	defmniop.set( key.buf(), val.buf() );
    }

    IOPar defiop;
    defiop.mergeComp( defmniop, Well::LogSet::sKeyDefMnem() );
    wd_.logs().defaultLogUsePar( defiop );

    return true;
}


bool Well::HDF5Reader::getMarkers() const
{
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sMarkersGrpName(), "" );
    uiRetVal uirv;

    dsky.setDataSetName( sMDsDSName() );
    const int sz = rdr_->dimSize( dsky, 0, uirv );
    if ( sz < 1 )
	return true;

    TypeSet<double> mds;
    TypeSet<int> lvlids;
    if ( !mds.setCapacity(sz,false) || !lvlids.setCapacity(sz,false) )
	return false;

    uirv = rdr_->get( dsky, mds );
    mErrRetIfUiRvNotOK( uirv );
    IOPar mdiop;
    uirv = rdr_->get( mdiop, &dsky );

    dsky.setDataSetName( sNamesDSName() );
    BufferStringSet nms;
    uirv = rdr_->get( dsky, nms );
    mErrRetIfUiRvNotOK( uirv )

    dsky.setDataSetName( sColorsDSName() );
    BufferStringSet colors;
    uirv = rdr_->get( dsky, colors );
    mErrRetIfUiRvNotOK( uirv )

    dsky.setDataSetName( sLvlIDsDSName() );
    uirv = rdr_->get( dsky, lvlids );
    mErrRetIfUiRvNotOK( uirv )

    MarkerSet newms;
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = mds[idx];
	const BufferString nm( nms.validIdx(idx) ? nms.get(idx).buf() : "" );
	OD::Color col( OD::Color::NoColor() );
	if ( colors.validIdx(idx) )
	    col.setStdStr( colors.get(idx) );

	const int lvlid = lvlids.validIdx(idx) ? lvlids[idx] : -1;

	auto* mrkr = new Marker( nm, dah, col );
	mrkr->setLevelID( Strat::LevelID(lvlid) );
	newms.insertNew( mrkr );
    }

    wd_.markers() = newms;

    return true;
}


bool Well::HDF5Reader::getDispProps() const
{
    if ( !ensureFileOpen() )
	return false;

    const char* usernm = GetInterpreterName();
    HDF5::DataSetKey dsky( sDispParsGrpName(), usernm );
    if ( !rdr_->hasDataSet(dsky) )
    {
	BufferStringSet dispgrps;
	rdr_->getDataSets(dsky.groupName(), dispgrps );
	if ( dispgrps.isEmpty() )
	    return true;

	dsky.setDataSetName( dispgrps.first()->buf() );
    }

    IOPar iop;
    uiRetVal uirv = rdr_->get( iop, &dsky );
    mErrRetIfUiRvNotOK( uirv )
    wd_.displayProperties(false).usePar( iop );
    wd_.displayProperties(true).usePar( iop );
    return true;
}
