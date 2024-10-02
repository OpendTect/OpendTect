/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "wellhdf5writer.h"

#include "hdf5arraynd.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welldisp.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltrack.h"


namespace Well
{
    static const int nrrowsperblock = 20;
};



Well::HDF5Writer::HDF5Writer( const char* fnm, const Data& wd,
			      uiString& errmsg )
    : WriteAccess(wd)
    , errmsg_(errmsg)
{
    init( fnm );
}


Well::HDF5Writer::HDF5Writer( const IOObj& ioobj, const Data& wd,
			      uiString& errmsg )
    : WriteAccess(wd)
    , errmsg_(errmsg)
{
    if ( !useHDF5(ioobj,errmsg_) || !errmsg_.isEmpty() )
	return;

    bool fnmchgd = false;
    init( ioobj.mainFileName(), &fnmchgd );
    if ( errmsg_.isEmpty() && fnmchgd )
    {
	PtrMan<IOObj> ioobjclone = ioobj.clone();
	mDynamicCastGet(IOStream*,iostrm,ioobjclone.ptr())
	if ( iostrm )
	{
	    iostrm->fileSpec().setFileName( filename_ );
	    IOM().commitChanges( *iostrm );
	}
    }
}


Well::HDF5Writer::~HDF5Writer()
{
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


void Well::HDF5Writer::init( const char* inpfnm, bool* fnmchgd )
{
    const BufferString orgfnm( inpfnm );
    if ( orgfnm.isEmpty() || !HDF5::isAvailable() )
    {
	errmsg_ = HDF5::Access::sHDF5NotAvailable( orgfnm );
	return;
    }

    wrr_ = HDF5::mkWriter().release();
    if ( !wrr_ )
    {
	pErrMsg("Available but no writer?");
	return;
    }

    filename_.set( orgfnm );
    if ( fnmchgd )
    {
	FilePath fp( orgfnm );
	fp.setExtension( HDF5::Access::sFileExtension() );
	filename_.set( fp.fullPath() );
	*fnmchgd = filename_ != orgfnm;
    }

    if ( !File::exists(filename_.buf()) )
	initGroups();
}


bool Well::HDF5Writer::initGroups()
{
    if ( !ensureFileOpen() )
	return false;

    uiRetVal uirv;
    if ( !wrr_->ensureGroup(sTrackGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sLogsGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sMarkersGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sTDsGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sCSsGrpName(),uirv) )
	return false;
    if ( !wrr_->ensureGroup(sDispParsGrpName(),uirv) )
	return false;

    return true;
}


bool Well::HDF5Writer::isFunctional() const
{
    return wrr_ && !filename_.isEmpty();
}


bool Well::HDF5Writer::ensureFileOpen() const
{
    if ( !wrr_ )
	return false;
    if ( wrr_->isOpen() )
	return true;
    if ( filename_.isEmpty() )
	return false;

    auto& wrr = cCast(HDF5::Writer&,*wrr_);
    const bool neededit = HDF5::isHDF5File( filename_ );
    uiRetVal uirv = neededit
		  ? wrr.open4Edit( filename_ )
		  : wrr.open( filename_ );

    if ( !uirv.isOK() )
    {
	const_cast<HDF5Writer*>(this)->wrr_ = nullptr;
	errmsg_.set( uirv );
	return false;
    }

    return true;
}


bool Well::HDF5Writer::put() const
{
    return putInfo()
	&& putTrack()
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


void Well::HDF5Writer::ensureCorrectDSSize( const HDF5::DataSetKey& dsky,
				int dim0, int dim1, uiRetVal& uirv ) const
{
    const int nrdims = dim1 > 0 ? 2 : 1;
    PtrMan<ArrayNDInfo> arrinf;
    if ( nrdims == 1 )
	arrinf = new Array1DInfoImpl( dim0 );
    else
	arrinf = new Array2DInfoImpl( dim0, dim1 );

    auto& wrr = cCast(HDF5::Writer&,*wrr_);
    uirv = wrr.resizeDataSet( dsky, *arrinf );
}


bool Well::HDF5Writer::putInfo() const
{
    if ( !ensureFileOpen() )
	return false;

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    IOPar iop;
    wd_.info().fillPar( iop );
    putDepthUnit( iop );
    uiRetVal uirv = wrr.set( iop );
    mErrRetIfUiRvNotOK( HDF5::DataSetKey() );

    return true;
}


bool Well::HDF5Writer::putTrack() const
{
    if ( !ensureFileOpen() )
	return false;

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    const int sz = wd_.track().size();
    Array2DImpl<double> crdarr( sz, 3 );
    Array1DImpl<double> mdarr( sz );
    TrackIter iter( wd_.track() ); // this locks not yet
    while ( iter.next() )
    {
	const Coord3 c = iter.pos();
	const int idx = iter.curIdx();
        crdarr.set( idx, 0, c.x_ );
        crdarr.set( idx, 1, c.y_ );
        crdarr.set( idx, 2, c.z_ );
	mdarr.set( idx, iter.dah() );
    }
    iter.retire();

    HDF5::DataSetKey dsky( sTrackGrpName(), "" );
    dsky.setMaximumSize( 0, nrrowsperblock );
    dsky.setDataSetName( sCoordsDSName() );
    const Array1DInfoImpl changedir( 1 );

    uiRetVal uirv;
    wrr.createDataSetIfMissing( dsky, OD::F64, crdarr.info(), changedir );
    ensureCorrectDSSize( dsky, iter.size(), 3, uirv );
    mErrRetIfUiRvNotOK( trackdsky );
    HDF5::ArrayNDTool<double> arrtool( crdarr );
    uirv = arrtool.put( wrr, dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sMDsDSName() );
    wrr.createDataSetIfMissing( dsky, OD::F64, mdarr.info(), changedir );
    ensureCorrectDSSize( dsky, iter.size(), -1, uirv );
    uirv = wrr.put( dsky, mdarr.arr(), sz );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::doPutD2T( bool csmdl ) const
{
    if ( !ensureFileOpen() )
	return false;

    const D2TModel* d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    if ( !d2t )
	return true;

    D2TModelIter iter( *d2t ); // this locks not yet
    const int sz = iter.size();
    Array1DImpl<double> mdarr( sz );
    Array1DImpl<double> timearr( sz );
    while ( iter.next() )
    {
	const int idx = iter.curIdx();
	mdarr.set( idx, iter.dah() );
	timearr.set( idx, iter.t() );
    }
    iter.retire();

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    uiRetVal uirv;
    const int modelid = 0; // TODO: suppport multiple models
    const HDF5::DataSetKey grpky =
	HDF5::DataSetKey::groupKey( csmdl ? sCSsGrpName() : sTDsGrpName(),
				    toString(modelid) );
    if ( !wrr.ensureGroup(grpky.groupName(),uirv) )
	return false;

    HDF5::DataSetKey dsky( nullptr, sMDsDSName() );
    dsky.setGroupName( grpky.fullDataSetName() );
    dsky.setMaximumSize( 0, nrrowsperblock );
    const Array1DInfoImpl changedir( 1 );
    wrr.createDataSetIfMissing( dsky, OD::F64, mdarr.info(), changedir );

    ensureCorrectDSSize( dsky, sz, -1, uirv );
    HDF5::ArrayNDTool<double> arrtool( mdarr );
    uirv = arrtool.put( wrr, dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sTWTsDSName() );
    wrr.createDataSetIfMissing( dsky, OD::F64, timearr.info(), changedir );
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr.put( dsky, timearr.arr(), sz );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( "" );
    IOPar iop;
    d2t->fillHeaderPar( iop );
    putDepthUnit( iop );
    uirv = wrr.set( iop, &dsky );
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
    if ( !ensureFileOpen() )
	return false;

    HDF5::DataSetKey dsky( sLogsGrpName() );
    dsky.setMaximumSize( 0, nrrowsperblock );
    const LogSet& logs = wd_.logs();
    const int nrlogs = logs.size();
    uiRetVal uirv;
    for ( int idx=0; idx<nrlogs; idx++ )
    {
	const Log& wl = logs.getLog( idx );
	dsky.setDataSetName( toString(idx) );
	putLog( wl );
	if ( !uirv.isOK() )
	    mErrRetIfUiRvNotOK( dsky );
    }

    PtrMan<HDF5::Reader> rdr = createCoupledHDFReader();
    if ( !rdr )
    {
	errmsg_.set( mINTERNAL("Write logs: cannot create coupled reader") );
	return false;
    }

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


bool Well::HDF5Writer::setLogAttribs( const HDF5::DataSetKey& dsky,
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

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    putDepthUnit( iop );
    uiRetVal uirv = wrr.set( iop, &dsky );
    return uirv.isOK();
}


int Well::HDF5Writer::getLogIndex( const char* lognm ) const
{
    const int nrlogs = wd_.logs().size();
    int logidx = wd_.logs().indexOf( lognm );
    //TODO: to be replaced by a proper well log identifier:
    if ( logidx < 0 )
    {
	//Unsafe !!!
	logidx = nrlogs < 0 ? 0 : nrlogs;
    }

    logidx++;
    return logidx;
}



bool Well::HDF5Writer::putLog( const Log& wl ) const
{
    if ( !ensureFileOpen() )
	return false;

    LogIter iter( wl );
    const int sz = iter.size();
    Array1DImpl<double> mdarr( sz );
    Array1DImpl<double> valarr( sz );
    while ( iter.next() )
    {
	const int arridx = iter.curIdx();
	mdarr.set( arridx, iter.dah() );
	valarr.set( arridx, iter.value() );
    }
    iter.retire();

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    uiRetVal uirv;

    const int logidx = getLogIndex( wl.name() );
    HDF5::DataSetKey grpky =
	HDF5::DataSetKey::groupKey( sLogsGrpName(), toString(logidx) );
    if ( !wrr.ensureGroup(grpky.groupName(),uirv) )
	return false;

    HDF5::DataSetKey dsky( nullptr, sMDsDSName() );
    dsky.setGroupName( grpky.fullDataSetName() );

    dsky.setMaximumSize( 0, nrrowsperblock );
    const Array1DInfoImpl changedir( 1 );
    wrr.createDataSetIfMissing( dsky, OD::F64, mdarr.info(), changedir );

    ensureCorrectDSSize( dsky, sz, -1, uirv );
    HDF5::ArrayNDTool<double> arrtool( mdarr );
    uirv = arrtool.put( wrr, dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sValuesDSName() );
    wrr.createDataSetIfMissing( dsky, OD::F64, valarr.info(), changedir );
    ensureCorrectDSSize( dsky, sz, -1, uirv );
    uirv = wrr.put( dsky, valarr.arr(), sz );
    mErrRetIfUiRvNotOK( dsky );

    return setLogAttribs( grpky, &wl );
}


bool Well::HDF5Writer::putMarkers() const
{
    if ( !ensureFileOpen() )
	return false;

    const MarkerSet& ms = wd_.markers();
    if ( ms.isEmpty() )
	return true;

    HDF5::DataSetKey dsky( sMarkersGrpName(), "" );
    dsky.setMaximumSize( 0, nrrowsperblock );

    BufferStringSet nms, colors;
    TypeSet<double> mds;
    TypeSet<int> lvlids;
    for ( const auto* mrkr : ms )
    {
	nms.add( mrkr->name() );
	colors.add( mrkr->color().getStdStr() );
	mds.add( mrkr->dah() );
	lvlids.add( mrkr->levelID().asInt() );
    }

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    const int nrmarkers = ms.size();
    const Array1DInfoImpl arrinfo( nrmarkers );
    const Array1DInfoImpl changedir( 1 );
    dsky.setDataSetName( sMDsDSName() );
    wrr.createDataSetIfMissing( dsky, OD::F64, arrinfo, changedir );
    uiRetVal uirv;
    ensureCorrectDSSize( dsky, nrmarkers, -1, uirv );
    uirv = wrr.put( dsky, mds );
    mErrRetIfUiRvNotOK( dsky );
    IOPar iop;
    putDepthUnit( iop );
    uirv = wrr.set( iop, &dsky );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sNamesDSName() );
    uirv = wrr.put( dsky, nms );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sColorsDSName() );
    uirv = wrr.put( dsky, colors );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sLvlIDsDSName() );
    wrr.createDataSetIfMissing( dsky, OD::SI32, arrinfo, changedir );
    ensureCorrectDSSize( dsky, nrmarkers, -1, uirv );
    uirv = wrr.put( dsky, lvlids );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::putDispProps() const
{
    if ( !ensureFileOpen() )
	return false;

    IOPar iop;
    wd_.displayProperties(true).fillPar( iop );
    wd_.displayProperties(false).fillPar( iop );
    putDepthUnit( iop );

    auto& wrr = cCast(HDF5::Writer&,*wrr_);

    const char* usernm = GetInterpreterName();
    const HDF5::DataSetKey grpdsky( sDispParsGrpName(), usernm );
    wrr.createTextDataSet( grpdsky );
    uiRetVal uirv = wrr.set( iop, &grpdsky );
    mErrRetIfUiRvNotOK( grpdsky );

    return true;
}
