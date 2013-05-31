/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "matlablibmgr.h"

#include "filepath.h"
#include "ptrman.h"
#include "sharedlibs.h"


MatlabLibMgr::MatlabLibMgr()
{}


MatlabLibMgr::~MatlabLibMgr()
{}


bool MatlabLibMgr::load( const char* libfnm )
{
    errmsg_ = "";
    const FilePath fp( libfnm );
    const BufferString libnm = fp.fileName();
    if ( libnms_.isPresent(libnm) )
    {
	errmsg_ = "Library already loaded";
	return false;
    }

    SharedLibAccess* sla = new SharedLibAccess( libfnm );
    if ( !sla->isOK() )
	return false;

    slas_ += sla;
    libnms_.add( libnm );
    return true;
}


bool MatlabLibMgr::isLoaded( const char* libfnm ) const
{ return getSharedLibAccess( libfnm ); }


const SharedLibAccess*
    MatlabLibMgr::getSharedLibAccess( const char* libfnm ) const
{
    const FilePath fp( libfnm );
    const BufferString libnm = fp.fileName();
    const int idx = libnms_.indexOf( libnm );
    return slas_.validIdx(idx) ? slas_[idx] : 0;
}


MatlabLibMgr& MLM()
{
    static PtrMan<MatlabLibMgr> inst = new MatlabLibMgr;
    return *inst;
}
