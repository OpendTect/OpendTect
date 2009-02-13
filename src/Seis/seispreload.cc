/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID = "$Id: seispreload.cc,v 1.1 2009-02-13 13:35:48 cvsbert Exp $";

#include "seispreload.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seiscbvsps.h"
#include "filegen.h"
#include "ioman.h"
#include "strmprov.h"
#include "cbvsio.h"


IOObj* Seis::PreLoader::getIOObj() const
{
    IOObj* ret = IOM().get( id_ );
    if ( !ret )
	errmsg_ = "Cannot find ID in object manager";
    return ret;
}


Interval<int> Seis::PreLoader::inlRange() const
{
    Interval<int> ret( -mUdf(int), mUdf(int) );
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( id_.buf(), fnms );
    for ( int idx=0; idx<fnms.size(); idx++ )
	ret.include( SeisCBVSPSIO::getInlNr( fnms.get(idx) ) );

    if ( mIsUdf(ret.stop) ) ret.start = ret.stop;
    return ret;
}


#define mGetIOObj() \
    notloaded_.erase(); \
    PtrMan<IOObj> ioobj = getIOObj(); \
    if ( !ioobj ) \
	return false; \
    if ( strcmp(ioobj->translator(),"CBVS") ) \
	{ errmsg_ = "Cannot pre-load other than CBVS data"; return false; }

#define mUnLoadIfLoaded() \
    if ( StreamProvider::isPreLoaded(id_.buf(),true) ) \
	StreamProvider::unLoad( id_.buf(), true )

bool Seis::PreLoader::loadVol() const
{
    mGetIOObj(); mUnLoadIfLoaded();

    const BufferString basefnm = CBVSIOMgr::baseFileName(
					    ioobj->fullUserExpr(true) );
    for ( int idx=0; true; idx++ )
    {
	const BufferString fnm( CBVSIOMgr::getFileName(basefnm,idx) );
	if ( !File_exists(fnm)
	  || !StreamProvider::preLoad(fnm,getTr(),id_.buf()) )
	{
	    if ( idx )
		break;
	    else
	    {
		errmsg_ = "Cannot load '"; errmsg_ += fnm;
		errmsg_ += "'"; return false;
		notloaded_.add( fnm );
	    }
	}
    }

    return true;
}


bool Seis::PreLoader::loadPS3D( const Interval<int>* inlrg ) const
{
    mGetIOObj();

    SeisCBVSPSIO psio( ioobj->fullUserExpr(true) );
    BufferStringSet fnms;
    if ( !psio.get3DFileNames(fnms,inlrg) )
	{ errmsg_ = psio.errMsg(); return false; }
    if ( fnms.isEmpty() ) return true;

    mUnLoadIfLoaded();

    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get( idx );
	if ( !StreamProvider::preLoad(fnm,getTr(),id_.buf()) )
	    notloaded_.add( fnm );
    }

    if ( notloaded_.size() == fnms.size() )
    {
	if ( notloaded_.size() > 1 )
	    errmsg_ = "Could not pre-load any required file";
	else
	{
	    errmsg_ = "Could not pre-load '";
	    errmsg_ += fnms.get( 0 ); errmsg_ += "'";
	}
	return false;
    }

    return true;
}


void Seis::PreLoader::unLoad() const
{
    StreamProvider::unLoad( id_.buf(), true );
}
