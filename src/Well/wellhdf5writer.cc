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
#include "ioobj.h"



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
    , wrr_(0)
{
    init( ioobj.mainFileName() );
}


Well::HDF5Writer::~HDF5Writer()
{
    delete wrr_;
}


HDF5::Reader* Well::HDF5Writer::createCoupledHDFReader() const
{
    return wrr_ ? wrr_->createCoupledReader() : 0;
}


#define mErrRet( s )	{ errmsg_ = s; return; }

void Well::HDF5Writer::init( const char* fnm )
{
    if ( !fnm || !*fnm || !HDF5::isAvailable() )
	mErrRet( HDF5::Access::sHDF5NotAvailable(fnm) )

    wrr_ = HDF5::mkWriter();
    if ( !wrr_ )
	{ pErrMsg("Available but no writer?"); return; }

    const bool neededit = HDF5::isHDF5File( fnm );
    uiRetVal uirv = neededit ? wrr_->open4Edit( fnm ) : wrr_->open( fnm );
    if ( !uirv.isOK() )
    {
	delete wrr_; wrr_ = 0;
	mErrRet( uirv )
    }
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


#define mErrRetIfUiRvNotOK(dsky) \
    if ( !uirv.isOK() ) \
	{ errmsg_.set( uirv ); return false; }


bool Well::HDF5Writer::putInfoAndTrack() const
{
    if ( !wrr_ )
	return false;

    IOPar iop;
    wd_.info().fillPar( iop );
    putDepthUnit( iop );
    const HDF5::DataSetKey rootdsky;
    uiRetVal uirv = wrr_->putInfo( rootdsky, iop );
    mErrRetIfUiRvNotOK( rootdsky );

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

    HDF5::ArrayNDTool<double> arrtool( arr );
    const HDF5::DataSetKey trackdsky( "", "Track" );
    uirv = arrtool.put( *wrr_, trackdsky );
    mErrRetIfUiRvNotOK( trackdsky );

    return true;
}


bool Well::HDF5Writer::doPutD2T( bool csmdl ) const
{
    const Well::D2TModel& d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    const HDF5::DataSetKey dsky( "", csmdl ? "CheckShot Model"
					   : "Depth/Time Model" );

    const size_type sz = d2t.size();
    Array2DImpl<float> arr( 2, sz );
    D2TModelIter iter( d2t );
    while ( iter.next() )
    {
	const idx_type idx = iter.curIdx();
	arr.set( 0, idx, iter.dah() );
	arr.set( 1, idx, iter.t() );
    }
    HDF5::ArrayNDTool<float> arrtool( arr );
    uiRetVal uirv = arrtool.put( *wrr_, dsky );
    mErrRetIfUiRvNotOK( trackdsky );

    IOPar iop;
    iop.set( sKey::Name(), d2t.name() );
    iop.set( sKey::Desc(), d2t.desc() );
    iop.set( D2TModel::sKeyDataSrc(), d2t.dataSource() );
    putDepthUnit( iop );
    uirv = wrr_->putInfo( dsky, iop );
    mErrRetIfUiRvNotOK( dsky );
    return false;
}


bool Well::HDF5Writer::putD2T() const
{
    return doPutD2T( false );
}


bool Well::HDF5Writer::putLogs() const
{
    errmsg_.set( mTODONotImplPhrase() );
    return false;
}


bool Well::HDF5Writer::putMarkers() const
{
    errmsg_.set( mTODONotImplPhrase() );
    return false;
}


bool Well::HDF5Writer::putCSMdl() const
{
    return doPutD2T( true );
}


bool Well::HDF5Writer::putDispProps() const
{
    errmsg_.set( mTODONotImplPhrase() );
    return false;
}
