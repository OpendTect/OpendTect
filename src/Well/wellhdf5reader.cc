/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2018
-*/


#include "wellhdf5reader.h"
#include "wellhdf5writer.h"

#include "filepath.h"
#include "hdf5arraynd.h"
#include "ioobj.h"
#include "keystrs.h"
#include "oddirs.h"
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


Well::HDF5Reader::HDF5Reader( const char* fnm, Well::Data& wd,
			      uiString& errmsg )
    : Well::ReadAccess(wd)
    , errmsg_(errmsg)
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


bool Well::HDF5Reader::ensureFileOpen() const
{
    if ( rdr_ && rdr_->isOpen() )
	return true;

    errmsg_.set( HDF5::Access::sHDF5FileNoLongerAccessibe() );
    return false;
}


bool Well::HDF5Reader::getInfo() const
{
    infoiop_.setEmpty();
    uiRetVal uirv = rdr_->get( infoiop_ );
    mErrRetIfUiRvNotOK( uirv );

    wd_.info().usePar( infoiop_ );
    return true;
}


bool Well::HDF5Reader::getTrack() const
{
    HDF5::DataSetKey dsky( sTrackGrpName(), "" );
    dsky.setDataSetName( sCoordsDSName() );

    uiRetVal uirv;
    const int sz = rdr_->dimSize( dsky, 0, uirv );
    if ( sz < 1 )
	return false;

    Array2DImpl<double> arr( sz, 3 );
    HDF5::ArrayNDTool<double> arrtool( arr );
    auto& rdr = cCast(HDF5::Reader&,*rdr_);
    uirv = arrtool.getAll( dsky, rdr );
    mErrRetIfUiRvNotOK( uirv );

    dsky.setDataSetName( sMDsDSName() );
    TypeSet<double> mds;
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
    D2TModel* d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    if ( !d2t )
	d2t = new D2TModel;

    const int modelid = 0; // TODO: suppport multiple models
    const HDF5::DataSetKey grpky =
	HDF5::DataSetKey::groupKey( csmdl ? sCSsGrpName() : sTDsGrpName(),
				    toString(modelid) );
    HDF5::DataSetKey dsky( nullptr, sMDsDSName() );
    dsky.setGroupName( grpky.fullDataSetName() );
    if ( !rdr_->hasDataSet(dsky) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(dsky) );
	return false;
    }

    IOPar hdriop;
    uiRetVal uirv = rdr_->get( hdriop, &dsky );
    mErrRetIfUiRvNotOK( uirv );
    d2t->useHeaderPar( hdriop );

    const int sz = rdr_->dimSize( dsky, 0, uirv );
    TypeSet<double> mds;
    uirv = rdr_->get( dsky, mds );
    mErrRetIfUiRvNotOK( uirv );

    dsky.setDataSetName( sTWTsDSName() );
    TypeSet<double> times;
    uirv = rdr_->get( dsky, times );
    mErrRetIfUiRvNotOK( uirv );

    d2t->setEmpty();
    for ( int idx=0; idx<sz; idx++ )
    {
	const double& dah = mds.get( idx );
	const double& val = times.get( idx );
	d2t->insertAtDah( dah, val );
    }

    if ( !updateDTModel(d2t,csmdl,errmsg_) )
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


bool Well::HDF5Reader::getLogs( bool needjustinfo ) const
{
    if ( !ensureFileOpen() )
	return false;

    LogSet& logs = wd_.logs();
    HDF5::DataSetKey dsky( sLogsGrpName() );
    errmsg_.setEmpty();
    for ( int ilog=1; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	Log* wl = getWL( grpkey );
	if ( !wl )
	    break;

	logs.add( wl );
    }

    return errmsg_.isEmpty();
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
    return !iop.isTrue( sKeyLogDel() );
}


#define mErrRetNullIfUiRvNotOK() \
    if ( !uirv.isOK() ) \
	{ errmsg_.set( uirv ); return nullptr; }

Well::Log* Well::HDF5Reader::getWL( const HDF5::DataSetKey& dsky ) const
{
    IOPar iop;
    if ( !getLogPars(dsky,iop) )
	return nullptr;

    uiRetVal uirv;
    HDF5::DataSetKey logkey( nullptr, sMDsDSName() );
    logkey.setGroupName( dsky.fullDataSetName() );
    if ( !rdr_->hasDataSet(logkey) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(logkey) );
	return nullptr;
    }

    const int sz = rdr_->dimSize( logkey, 0, uirv );
    TypeSet<double> mds;
    uirv = rdr_->get( logkey, mds );
    mErrRetNullIfUiRvNotOK();

    logkey.setDataSetName( sValuesDSName() );
    TypeSet<double> vals;
    uirv = rdr_->get( logkey, vals );
    mErrRetNullIfUiRvNotOK();

    BufferString lognm;
    iop.get( sKey::Name(), lognm );
    auto* wl = new Log( lognm );

    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = mds.get( idx );
	const float val = vals.get( idx );
	wl->addValue( dah, val );
    }

    BufferString uomlbl;
    if ( iop.get(Log::sKeyUnitLbl(),uomlbl) )
	wl->setUnitMeasLabel( uomlbl );

    iop.removeWithKey( sKey::Name() );
    iop.removeWithKey( sKey::DepthUnit() );
    iop.removeWithKey( Log::sKeyUnitLbl() );
    // TODO_HDF5Reader
    // wl->setPars( iop );

    return wl;
}


bool Well::HDF5Reader::getLog( const char* reqlognm ) const
{
    HDF5::DataSetKey dsky( sLogsGrpName() );
    BufferStringSet loggrps;
    rdr_->getSubGroups( sLogsGrpName(), loggrps );
    for ( int ilog=1; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	IOPar iop;
	if ( !getLogPars(grpkey,iop) )
	    return false;

	BufferString lognm;
	iop.get( sKey::Name(), lognm );
	if ( lognm == reqlognm )
	    return getWL( grpkey );
    }
}


void Well::HDF5Reader::getLogInfo( BufferStringSet& nms ) const
{
    HDF5::DataSetKey dsky( sLogsGrpName() );
    for ( int ilog=1; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	HDF5::DataSetKey grpkey;
	grpkey.setGroupName( dsky.fullDataSetName() );
	IOPar iop;
	if ( !getLogPars(grpkey,iop) )
	    return;

	BufferString lognm;
	iop.get( sKey::Name(), lognm );
	nms.add( lognm );
    }
}


bool Well::HDF5Reader::getMarkers() const
{
    HDF5::DataSetKey dsky( sMarkersGrpName(), "" );
    MarkerSet& ms = wd_.markers();

    dsky.setDataSetName( sMDsDSName() );
    TypeSet<float> mds;
    uiRetVal uirv = rdr_->get( dsky, mds );
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
    TypeSet<int> lvlids;
    uirv = rdr_->get( dsky, lvlids );
    mErrRetIfUiRvNotOK( uirv )

    ms.setEmpty();
    const int sz = mds.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = mds[idx];
	const BufferString nm( nms.validIdx(idx) ? nms.get(idx).buf()
						 : "" );
	OD::Color col( OD::Color::NoColor() );
	if ( colors.validIdx(idx) )
	    col.setStdStr( colors.get(idx) );
	const int lvlid = lvlids.validIdx(idx) ? lvlids[idx] : -1;

	auto* mrkr = new Marker( nm, dah, col );
	mrkr->setLevelID( Strat::LevelID(lvlid) );
	ms.insertNew( mrkr );
    }

    return true;
}


bool Well::HDF5Reader::getDispProps() const
{
    const char* usernm = GetInterpreterName();
    const HDF5::DataSetKey dsky( sDispParsGrpName(), usernm );
    if ( !rdr_->hasDataSet(dsky) )
    {
	errmsg_.set( rdr_->sCannotReadDataSet(dsky) );
	return false;
    }

    IOPar iop;
    uiRetVal uirv = rdr_->get( iop, &dsky );
    mErrRetIfUiRvNotOK( uirv )
    wd_.displayProperties(false).usePar( iop );
    wd_.displayProperties(true).usePar( iop );
    return true;
}
