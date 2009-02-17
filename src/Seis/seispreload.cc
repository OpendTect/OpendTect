/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID = "$Id: seispreload.cc,v 1.5 2009-02-17 15:12:03 cvsbert Exp $";

#include "seispreload.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seiscbvs2d.h"
#include "seiscbvsps.h"
#include "seis2dline.h"
#include "filepath.h"
#include "filegen.h"
#include "keystrs.h"
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


void Seis::PreLoader::getLineKeys( BufferStringSet& lks ) const
{
    lks.erase(); PtrMan<IOObj> ioobj = getIOObj();
    if ( !ioobj ) return;

    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( id_.buf(), fnms );
    if ( fnms.isEmpty() ) return;
    BufferStringSet nms;
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	FilePath fp( fnms.get(idx) );
	nms.add( fp.fileName() );
    }

    Seis2DLineSet ls( *ioobj );
    const int nrlns = ls.nrLines();
    for ( int iln=0; iln<nrlns; iln++ )
    {
	const IOPar& iop = ls.getInfo( iln );
	const char* fnm = iop.find( sKey::FileName );
	if ( !fnm ) continue;

	const int idxof = nms.indexOf( fnm );
	if ( idxof >= 0 )
	    lks.add( ls.lineKey(iln).buf() );
    }
}


#define mPrepIOObj(is2dln) \
    PtrMan<IOObj> ioobj = getIOObj(); \
    if ( !ioobj ) \
	return false; \
    if ( !is2dln && strcmp(ioobj->translator(),"CBVS") ) \
	{ errmsg_ = "Cannot pre-load other than CBVS data"; return false; } \
    TaskRunner& trunnr = getTr()


bool Seis::PreLoader::loadVol() const
{
    mPrepIOObj( false );

    BufferStringSet fnms;
    const BufferString basefnm = CBVSIOMgr::baseFileName(
				 ioobj->fullUserExpr(true) );
    for ( int idx=0; true; idx++ )
    {
	const BufferString fnm( CBVSIOMgr::getFileName(basefnm,idx) );
	if ( File_exists(fnm) )
	    fnms.add( fnm );
	else
	    break;
    }

    return StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadLines( const BufferStringSet& lnms,
				 const BufferStringSet& attrnms ) const
{
    if ( lnms.isEmpty() || attrnms.isEmpty() )
	{ errmsg_ = "Internal: No line or no attr requested"; return false; }
    mPrepIOObj( true );

    Seis2DLineSet ls( *ioobj );
    const int nrlns = ls.nrLines();
    if ( nrlns < 1 )
    {
	errmsg_ = "Line Set '"; errmsg_ += ioobj->name();
	errmsg_ += "' contains no data";
	return false;
    }

    BufferStringSet fnms;
    for ( int iln=0; iln<nrlns; iln++ )
    {
	const char* lnm = ls.lineName( iln );
	const char* attrnm = ls.attribute( iln );
	if ( lnms.indexOf(lnm) < 0 || attrnms.indexOf(attrnm) < 0 )
	    continue;

	fnms.add( SeisCBVS2DLineIOProvider::getFileName(ls.getInfo(iln)) );
    }

    return fnms.isEmpty() ? true
	 : StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadPS3D( const Interval<int>* inlrg ) const
{
    mPrepIOObj( false );

    SeisCBVSPSIO psio( ioobj->fullUserExpr(true) );
    BufferStringSet fnms;
    if ( !psio.get3DFileNames(fnms,inlrg) )
	{ errmsg_ = psio.errMsg(); return false; }

    return fnms.isEmpty() ? true
	 : StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadPS2D( const char* lnm ) const
{
    mPrepIOObj( false );

    SeisCBVSPSIO psio( ioobj->fullUserExpr(true) );
    const BufferString fnm( psio.get2DFileName(lnm) );
    return fnm.isEmpty() ? true
	 : StreamProvider::preLoad( fnm, trunnr, id_.buf() );
}


void Seis::PreLoader::unLoad() const
{
    StreamProvider::unLoad( id_.buf(), true );
}
