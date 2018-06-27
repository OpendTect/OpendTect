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
const char* Well::HDF5Access::sColorsDSName()	{ return "Colors"; }
const char* Well::HDF5Access::sNamesDSName()	{ return "Names"; }
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

#define mEnsureScope(dsky) \
    mEnsureFileOpen(); \
    if ( !rdr_->setScope(dsky) ) \
    { \
	errmsg_.set( rdr_->sCantSetScope(dsky) ); \
	return false; \
    }


bool Well::HDF5Reader::getInfo() const
{
    const HDF5::DataSetKey rootdsky;
    mEnsureScope( rootdsky );

    infoiop_.setEmpty();
    uiRetVal uirv = rdr_->getInfo( infoiop_ );
    mErrRetIfUiRvNotOK( uirv );

    wd_.info().usePar( infoiop_ );
    return true;
}


bool Well::HDF5Reader::getTrack() const
{
    const HDF5::DataSetKey trackdsky( "", sTrackDSName() );
    mEnsureScope( trackdsky );

    const size_type sz = rdr_->dimSize( 1 );
    Array2DImpl<double> arr( 4, sz );
    HDF5::ArrayNDTool<double> arrtool( arr );
    uiRetVal uirv = arrtool.getAll( *rdr_ );
    mErrRetIfUiRvNotOK( uirv );

    wd_.track().setEmpty();
    mGetZFac( infoiop_ );
    for ( int idx=0; idx<sz; idx++ )
    {
	const float dah = (float)(zfac * arr.get(0,idx));
	Coord3 c( arr.get(1,idx), arr.get(2,idx) );
	c.z_ = zfac * arr.get( 3, idx );
	wd_.track().addPoint( c, dah );
    }

    return true;
}


bool Well::HDF5Reader::getLogs() const
{
    return false;
}


bool Well::HDF5Reader::getMarkers() const
{
    return false;
}


bool Well::HDF5Reader::getD2T() const
{
    return false;
}


bool Well::HDF5Reader::getCSMdl() const
{
    return false;
}


bool Well::HDF5Reader::getDispProps() const
{
    return false;
}


bool Well::HDF5Reader::getLog( const char* lognm ) const
{
    return false;
}


void Well::HDF5Reader::getLogNames( BufferStringSet& nms ) const
{
}


void Well::HDF5Reader::getLogInfo( ObjectSet<IOPar>& iops ) const
{
}
