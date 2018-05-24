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
#include "ioobj.h"



Well::HDF5Writer::HDF5Writer( const char* fnm, const Well::Data& wd,
			      uiString& e )
    : Well::WriteAccess(wd)
    , errmsg_(e)
{
    init( fnm );
}


Well::HDF5Writer::HDF5Writer( const IOObj& ioobj, const Well::Data& wd,
			      uiString& e )
    : Well::WriteAccess(wd)
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
	{ pErrMsg("Available but no reader?"); return; }

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


bool Well::HDF5Writer::putInfoAndTrack() const
{
    return false;
}


bool Well::HDF5Writer::putD2T() const
{
    return false;
}


bool Well::HDF5Writer::putLogs() const
{
    return false;
}


bool Well::HDF5Writer::putMarkers() const
{
    return false;
}


bool Well::HDF5Writer::putCSMdl() const
{
    return false;
}


bool Well::HDF5Writer::putDispProps() const
{
    return false;
}
