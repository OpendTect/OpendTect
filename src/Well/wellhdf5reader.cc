/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2018
-*/


#include "wellhdf5reader.h"
#include "wellhdf5writer.h"
#include "hdf5arraynd.h"
#include "welldata.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"

const char* Well::HDF5Access::sLogsGrpName()	{ return "Logs"; }
const char* Well::HDF5Access::sMarkersGrpName()	{ return "Markers"; }
const char* Well::HDF5Access::sDispParsGrpName() { return "Display"; }
const char* Well::HDF5Access::sTrackDSName()	{ return "Track"; }
const char* Well::HDF5Access::sD2TDSName()	{ return "MD2Time"; }
const char* Well::HDF5Access::sCSMdlDSName()	{ return "CheckShot"; }
const char* Well::HDF5Access::sMDsDSName()	{ return "MDs"; }
const char* Well::HDF5Access::sNamesDSName()	{ return "Names"; }
const char* Well::HDF5Access::sColorsDSName()	{ return "Colors"; }
const char* Well::HDF5Access::sLvlIDsDSName()	{ return "LevelIDs"; }
const char* Well::HDF5Access::sKeyLogDel()	{ return "Deleted"; }


Well::HDF5Reader::HDF5Reader( const char* fnm, Well::Data& wd, uiString& e )
    : Well::ReadAccess(wd)
    , errmsg_(e)
    , rdr_(0)
{
    File::Path fp( fnm ); fp.setExtension( 0 );
    wd_.info().setName( fp.fileName() );

    init( fnm );
}


Well::HDF5Reader::HDF5Reader( const IOObj& ioobj, Well::Data& wd, uiString& e )
    : Well::ReadAccess(wd)
    , errmsg_(e)
    , rdr_(0)
{
    wd_.info().setName( ioobj.name() );

    init( ioobj.mainFileName() );
}


Well::HDF5Reader::HDF5Reader( const HDF5Writer& wrr, Well::Data& wd,
			      uiString& e )
    : Well::ReadAccess(wd)
    , errmsg_(e)
    , rdr_(wrr.createCoupledHDFReader())
{
    if ( !rdr_ )
	{ pErrMsg("No coupled reader created"); return; }
}


Well::HDF5Reader::~HDF5Reader()
{
    delete rdr_;
}


#define mErrRet( s )	{ errmsg_ = s; return; }

void Well::HDF5Reader::init( const char* fnm )
{
    if ( !HDF5::isAvailable() )
	mErrRet( HDF5::Access::sHDF5NotAvailable(fnm) )
    else if ( !HDF5::isHDF5File(fnm) )
	mErrRet( HDF5::Access::sNotHDF5File(fnm) )

    rdr_ = HDF5::mkReader();
    if ( !rdr_ )
	{ pErrMsg("Available but no reader?"); return; }

    uiRetVal uirv = rdr_->open( fnm );
    if ( !uirv.isOK() )
    {
	delete rdr_; rdr_ = 0;
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


#define mGetZFac(iop) const float zfac = getZFac( iop )

#define mEnsureDataSet(dsky,act) \
    if ( !rdr_->hasDataSet(dsky) ) \
    { \
	errmsg_.set( rdr_->sCannotReadDataSet(dsky) ); \
	act; \
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
    const DataSetKey trackdsky( "", sTrackDSName() );
    mEnsureDataSet(trackdsky,return false)
    uiRetVal uirv;
    const size_type sz = rdr_->dimSize( trackdsky, 1, uirv );
    Array2DImpl<double> arr( 4, sz );
    HDF5::ArrayNDTool<double> arrtool( arr );
    uirv = arrtool.getAll( trackdsky, *rdr_ );
    mErrRetIfUiRvNotOK( uirv );

    Well::Track& trck = wd_.track();
    trck.setEmpty();
    mGetZFac( infoiop_ );
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = (float)(zfac * arr.get(0,idx));
	Coord3 c( arr.get(1,idx), arr.get(2,idx) );
	c.z_ = zfac * arr.get( 3, idx );
	trck.addPoint( c, dah );
    }

    return true;
}


bool Well::HDF5Reader::doGetD2T( bool csmdl ) const
{
    const DataSetKey dsky( "", csmdl ? sCSMdlDSName() : sD2TDSName() );
    D2TModel& d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    mEnsureDataSet(dsky,return false)

    IOPar hdriop;
    uiRetVal uirv = rdr_->get( hdriop, &dsky );
    mErrRetIfUiRvNotOK( uirv );
    d2t.useHdrPar( hdriop );

    const size_type sz = rdr_->dimSize( dsky, 1, uirv );
    Array2DImpl<float> arr( 2, sz );
    HDF5::ArrayNDTool<float> arrtool( arr );
    uirv = arrtool.getAll( dsky, *rdr_ );
    mErrRetIfUiRvNotOK( uirv );

    d2t.setEmpty();
    mGetZFac( hdriop );
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = (float)(zfac * arr.get(0,idx));
	const float val = arr.get( 1, idx );
	d2t.setValueAt( dah, val );
    }

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
    mEnsureFileOpen();

    LogSet& logs = wd_.logs();
    DataSetKey dsky( sLogsGrpName() );
    errmsg_.setEmpty();
    for ( int ilog=0; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	Log* wl = getWL( dsky );
	if ( !wl )
	    break;
	logs.add( wl );
    }
    return errmsg_.isEmpty();
}


bool Well::HDF5Reader::getLogPars( const DataSetKey& dsky, IOPar& iop ) const
{
    mEnsureDataSet( dsky, return false );
    uiRetVal uirv = rdr_->get( iop, &dsky );
    mErrRetIfUiRvNotOK( uirv )
    return !iop.isTrue( sKeyLogDel() );
}


#define mErrRetNullIfUiRvNotOK() \
    if ( !uirv.isOK() ) \
	{ errmsg_.set( uirv ); return 0; }

Well::Log* Well::HDF5Reader::getWL( const DataSetKey& dsky ) const
{
    IOPar iop;
    if ( !getLogPars(dsky,iop) )
	return nullptr;

    uiRetVal uirv;
    const size_type sz = rdr_->dimSize( dsky, 1, uirv );
    Array2DImpl<float> arr( 2, sz );
    HDF5::ArrayNDTool<float> arrtool( arr );
    uirv = arrtool.getAll( dsky, *rdr_ );
    mErrRetNullIfUiRvNotOK()

    BufferString lognm;
    iop.get( sKey::Name(), lognm );
    Log* wl = new Log( lognm );

    mGetZFac( iop );
    for ( int idx=0; idx<sz; idx++ )
    {
	const ZType dah = (ZType)(zfac * arr.get(0,idx));
	const float val = arr.get( 1, idx );
	wl->addValue( dah, val );
    }

    BufferString uomlbl;
    if ( iop.get(Log::sKeyUnitLbl(),uomlbl) )
	wl->setUnitMeasLabel( uomlbl );

    iop.removeWithKey( sKey::Name() );
    iop.removeWithKey( sKey::DepthUnit() );
    iop.removeWithKey( Log::sKeyUnitLbl() );
    wl->setPars( iop );

    return wl;
}


bool Well::HDF5Reader::getLog( const char* reqlognm ) const
{
    DataSetKey dsky( sLogsGrpName() );
    for ( int ilog=0; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	IOPar iop;
	if ( getLogPars(dsky,iop) )
	{
	    BufferString lognm;
	    iop.get( sKey::Name(), lognm );
	    if ( lognm == reqlognm )
		return getWL( dsky );
	}
    }
}


void Well::HDF5Reader::getLogNames( BufferStringSet& nms ) const
{
    DataSetKey dsky( sLogsGrpName() );
    for ( int ilog=0; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	IOPar iop;
	if ( getLogPars(dsky,iop) )
	{
	    BufferString lognm;
	    iop.get( sKey::Name(), lognm );
	    nms.add( lognm );
	}
    }
}


void Well::HDF5Reader::getLogInfo( ObjectSet<IOPar>& iops ) const
{
    DataSetKey dsky( sLogsGrpName() );
    for ( int ilog=0; ; ilog++ )
    {
	dsky.setDataSetName( toString(ilog) );
	IOPar* iop = new IOPar;
	if ( getLogPars(dsky,*iop) )
	    iops += iop;
	else
	    { delete iop; break; }
    }
}


bool Well::HDF5Reader::getMarkers() const
{
    DataSetKey dsky( sMarkersGrpName(), "" );
    MarkerSet& ms = wd_.markers();
    typedef MarkerSet::LevelID::IDType LvlIDType;

    dsky.setDataSetName( sMDsDSName() );
    TypeSet<ZType> mds;
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
    TypeSet<LvlIDType> lvlids;
    uirv = rdr_->get( dsky, lvlids );
    mErrRetIfUiRvNotOK( uirv )

    ms.setEmpty();
    mGetZFac( mdiop );
    const int sz = mds.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const ZType dah = (ZType)(zfac * mds[idx]);
	const BufferString nm( nms.validIdx(idx) ? nms.get(idx)
						 : Marker::udf().name() );
	Color col( Color::NoColor() );
	if ( colors.validIdx(idx) )
	    col.setStdStr( colors.get(idx) );
	const LvlIDType lvlid = lvlids.validIdx(idx) ? lvlids[idx] : -1;

	Marker mrkr( nm, dah, col );
	mrkr.setLevelID( Marker::LevelID(lvlid) );
	ms.add( mrkr );
    }

    return true;
}


bool Well::HDF5Reader::getDispProps() const
{
    const DataSetKey dsky( sDispParsGrpName() );
    mEnsureDataSet(dsky,return false)
    IOPar iop;
    uiRetVal uirv = rdr_->get( iop, &dsky );
    mErrRetIfUiRvNotOK( uirv )
    wd_.displayProperties3d().usePar( iop );
    wd_.displayProperties2d().usePar( iop );
    return true;
}
