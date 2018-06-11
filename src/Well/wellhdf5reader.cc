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

const char* Well::HDF5Access::sTrackDSName()	{ return "Track"; }
const char* Well::HDF5Access::sLogsDSName()	{ return "Logs"; }
const char* Well::HDF5Access::sMarkersDSName()	{ return "Markers"; }
const char* Well::HDF5Access::sD2TDSName()	{ return "Depth/Time Data"; }
const char* Well::HDF5Access::sCSMdlDSName()	{ return "CheckShot Data"; }


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


bool Well::HDF5Reader::getInfo() const
{
    return false;
}


bool Well::HDF5Reader::getTrack() const
{
    return false;
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
