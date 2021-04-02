/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


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
#include "keystrs.h"
#include "iostrm.h"
#include "file.h"
#include "filepath.h"


namespace Well
{
    static const int nrrowsperblock = 20;
};



Well::HDF5Writer::HDF5Writer( const char* fnm, const Data& wd,
			      uiString& e )
    : WriteAccess(wd)
    , errmsg_(e)
{
    init( fnm );
}


Well::HDF5Writer::HDF5Writer( const IOObj& ioobj, const Data& wd,
			      uiString& e )
    : WriteAccess(wd)
    , errmsg_(e)
    , wrr_(nullptr)
{
    if ( !useHDF5(ioobj,errmsg_) || !errmsg_.isEmpty() )
	return;

    bool fnmchgd;
    init( ioobj.mainFileName(), &fnmchgd );
    if ( errmsg_.isEmpty() && fnmchgd )
    {
	IOObj* ioobjclone = ioobj.clone();
	mDynamicCastGet( IOStream*, iostrm, ioobjclone );
	if ( iostrm )
	{
	    iostrm->fileSpec().setFileName( filename_ );
	    iostrm->commitChanges();
	}
    }
}


Well::HDF5Writer::~HDF5Writer()
{
    delete wrr_;
}


bool Well::HDF5Writer::useHDF5( const IOObj& ioobj, uiString& emsg )
{
    const BufferString fnm( ioobj.mainFileName() );
    bool usehdf = HDF5::isEnabled( HDF5::sWellType() );
    emsg.setEmpty();
    if ( !File::isEmpty(fnm) )
    {
	usehdf = HDF5::isHDF5File(fnm);
	if ( usehdf && !HDF5::isAvailable() )
	    emsg = HDF5::Access::sHDF5NotAvailable( fnm );
    }
    return usehdf;
}


HDF5::Reader* Well::HDF5Writer::createCoupledHDFReader() const
{
    return wrr_ ? wrr_->createCoupledReader() : nullptr;
}


#define mErrRet( s )	{ errmsg_ = s; return; }

void Well::HDF5Writer::init( const char* inpfnm, bool* fnmchgd )
{
    const BufferString orgfnm( inpfnm );
    if ( orgfnm.isEmpty() || !HDF5::isAvailable() )
    {
	errmsg_ = HDF5::Access::sHDF5NotAvailable( orgfnm );
	return;
    }

    wrr_ = HDF5::mkWriter();
    if ( !wrr_ )
	{ pErrMsg("Available but no writer?"); return; }

    filename_.set( orgfnm );
    if ( fnmchgd )
    {
	File::Path fp( orgfnm );
	fp.setExtension( HDF5::Access::sFileExtension() );
	filename_.set( fp.fullPath() );
	*fnmchgd = filename_ != orgfnm;
    }
}


bool Well::HDF5Writer::isFunctional() const
{
    return wrr_ && !filename_.isEmpty();
}


bool Well::HDF5Writer::ensureFileOpen() const
{
    if ( !wrr_ )
	return false;
    else if ( wrr_->isOpen() )
	return true;
    else if ( filename_.isEmpty() )
	return false;

    const bool neededit = HDF5::isHDF5File( filename_ );
    uiRetVal uirv = neededit
		  ? wrr_->open4Edit( filename_ )
		  : wrr_->open( filename_ );

    if ( !uirv.isOK() )
    {
	delete wrr_; const_cast<HDF5Writer*>(this)->wrr_ = nullptr;
	errmsg_.set( uirv );
	return false;
    }

    return true;
}


bool Well::HDF5Writer::put() const
{
    return putInfoAndTrack()
	&& putD2T()
	&& putLogs()
	&& putMarkers()
	&& putCSMdl()
	&& putDispProps();
}


void Well::HDF5Writer::putDepthUnit( IOPar& iop ) const
{
    iop.set( sKey::DepthUnit(),
	     UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
}


void Well::HDF5Writer::ensureCorrectDSSize( const DataSetKey& dsky,
				int dim0, int dim1, uiRetVal& uirv ) const
{
    const int nrdims = dim1 > 0 ? 2 : 1;
    PtrMan<ArrayNDInfo> arrinf;
    if ( nrdims == 1 )
	arrinf = new Array1DInfoImpl( dim0 );
    else
	arrinf = new Array2DInfoImpl( dim0, dim1 );

    uirv = wrr_->resizeDataSet( dsky, *arrinf );
}


#define mGetCoupledReader() \
    PtrMan<HDF5::Reader> rdr = createCoupledHDFReader(); \
    if ( !rdr ) \
    { \
	errmsg_.set( mINTERNAL("Write logs: cannot create coupled reader") ); \
	return false; \
    }


bool Well::HDF5Writer::putInfoAndTrack() const
{
    mEnsureFileOpen();
    mGetCoupledReader();

    IOPar iop;
    wd_.info().fillPar( iop );
    putDepthUnit( iop );
    uiRetVal uirv = wrr_->set( iop );
    mErrRetIfUiRvNotOK( DataSetKey() );

    const size_type sz = wd_.track().size();
    Array2DImpl<double> arr( 4, sz );
    TrackIter iter( wd_.track() );
    while ( iter.next() )
    {
	const Coord3 c = iter.pos();
	const idx_type idx = iter.curIdx();
	arr.set( 0, idx, iter.dah() );
	arr.set( 1, idx, c.x_ );
	arr.set( 2, idx, c.y_ );
	arr.set( 3, idx, c.z_ );
    }

    DataSetKey trackdsky( "", sTrackDSName() );
    trackdsky.setMaximumSize( 0, nrrowsperblock );
    ensureCorrectDSSize( trackdsky, 4, iter.size(), uirv );
    mErrRetIfUiRvNotOK( trackdsky );
    HDF5::ArrayNDTool<double> arrtool( arr );
    uirv = arrtool.put( *wrr_, trackdsky );
    mErrRetIfUiRvNotOK( trackdsky );

    return true;
}


bool Well::HDF5Writer::doPutD2T( bool csmdl ) const
{
    mEnsureFileOpen();

    const D2TModel& d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    DataSetKey dsky( "", csmdl ? sCSMdlDSName() : sD2TDSName() );
    dsky.setMaximumSize( 0, nrrowsperblock );

    D2TModelIter iter( d2t ); // this locks
    const size_type sz = iter.size();
    Array2DImpl<ZType> arr( 2, sz );
    while ( iter.next() )
    {
	const idx_type idx = iter.curIdx();
	arr.set( 0, idx, iter.dah() );
	arr.set( 1, idx, iter.t() );
    }
    iter.retire();

    uiRetVal uirv;
    ensureCorrectDSSize( dsky, 2, sz, uirv );
    HDF5::ArrayNDTool<ZType> arrtool( arr );
    uirv = arrtool.put( *wrr_, dsky );
    mErrRetIfUiRvNotOK( trackdsky );

    IOPar iop;
    d2t.fillHdrPar( iop );
    putDepthUnit( iop );
    uirv = wrr_->set( iop, &dsky );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::putD2T() const
{
    return doPutD2T( false );
}


bool Well::HDF5Writer::putCSMdl() const
{
    return doPutD2T( true );
}


bool Well::HDF5Writer::putLogs() const
{
    mEnsureFileOpen();
    mGetCoupledReader();

    DataSetKey dsky( sLogsGrpName() );
    dsky.setMaximumSize( 0, nrrowsperblock );
    const LogSet& logs = wd_.logs();
    LogSetIter iter( logs );
    const int nrlogs = iter.size();
    uiRetVal uirv;
    while ( iter.next() )
    {
	const idx_type idx = iter.curIdx();
	const Log& wl = iter.log();

	dsky.setDataSetName( toString(iter.curIdx()) );
	ensureCorrectDSSize( dsky, 2, wl.size(), uirv );

	putLog( idx, wl, uirv );
	if ( !uirv.isOK() )
	    mErrRetIfUiRvNotOK( dsky );
    }
    iter.retire();

    // remove possible extra data sets (can be there if logs were removed)
    for ( int idx=nrlogs; ; idx++ )
    {
	dsky.setDataSetName( toString(idx) );
	if ( rdr->hasDataSet(dsky) )
	    setLogAttribs( dsky, nullptr );
	else
	    break;
    }

    return true;
}


bool Well::HDF5Writer::setLogAttribs( const DataSetKey& dsky,
				      const Log* wl ) const
{
    IOPar iop;
    if ( wl )
    {
	iop = wl->pars();
	const BufferString uomlbl = wl->unitMeasLabel();
	if ( uomlbl.isEmpty() )
	    iop.removeWithKey( Log::sKeyUnitLbl() );
	else
	    iop.set( Log::sKeyUnitLbl(), uomlbl );
	iop.set( sKey::Name(), wl->name() );
    }
    iop.setYN( sKeyLogDel(), !wl );

    putDepthUnit( iop );
    uiRetVal uirv = wrr_->set( iop, &dsky );
    return uirv.isOK();
}


bool Well::HDF5Writer::putLog( int logidx, const Log& wl, uiRetVal& uirv ) const
{
    LogIter iter( wl );
    const size_type sz = iter.size();
    Array2DImpl<ZType> arr( 2, sz );
    while ( iter.next() )
    {
	const idx_type arridx = iter.curIdx();
	arr.set( 0, arridx, iter.dah() );
	arr.set( 1, arridx, iter.value() );
    }
    iter.retire();

    DataSetKey dsky( sLogsGrpName(), toString(logidx) );
    dsky.setMaximumSize( 0, nrrowsperblock );
    HDF5::ArrayNDTool<ZType> arrtool( arr );
    uirv = arrtool.put( *wrr_, dsky );
    mErrRetIfUiRvNotOK( dsky );

    return setLogAttribs( dsky, &wl );
}


bool Well::HDF5Writer::putMarkers() const
{
    mEnsureFileOpen();

    const MarkerSet& ms = wd_.markers();
    DataSetKey dsky( sMarkersGrpName(), "" );
    dsky.setMaximumSize( 0, nrrowsperblock );
    typedef MarkerSet::LevelID::IDType LvlIDType;

    BufferStringSet nms, colors;
    TypeSet<ZType> mds; TypeSet<LvlIDType> lvlids;
    MarkerSetIter iter( ms );
    const int sz = iter.size();
    while ( iter.next() )
    {
	const Marker& mrkr = iter.get();
	nms.add( mrkr.name() );
	colors.add( mrkr.color().getStdStr() );
	mds.add( mrkr.dah() );
	lvlids.add( mrkr.levelID().getI() );
    }
    iter.retire();

    dsky.setDataSetName( sMDsDSName() );
    uiRetVal uirv;
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr_->put( dsky, mds );
    mErrRetIfUiRvNotOK( dsky );
    IOPar iop;
    putDepthUnit( iop );
    uirv = wrr_->set( iop, &dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sNamesDSName() );
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr_->put( dsky, nms );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sColorsDSName() );
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr_->put( dsky, colors );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sLvlIDsDSName() );
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr_->put( dsky, lvlids );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::putDispProps() const
{
    mEnsureFileOpen();

    IOPar iop;
    wd_.displayProperties2d().fillPar( iop );
    wd_.displayProperties3d().fillPar( iop );
    putDepthUnit( iop );
    const DataSetKey grpdsky( sDispParsGrpName() );
    uiRetVal uirv = wrr_->set( iop, &grpdsky );
    mErrRetIfUiRvNotOK( grpdsky );

    return true;
}
