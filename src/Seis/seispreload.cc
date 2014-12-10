/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID mUsedVar = "$Id$";

#include "seispreload.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seiscbvs2d.h"
#include "seiscbvsps.h"
#include "seis2ddata.h"
#include "seisioobjinfo.h"
#include "filepath.h"
#include "file.h"
#include "keystrs.h"
#include "ioman.h"
#include "strmprov.h"
#include "perthreadrepos.h"
#include "cbvsio.h"

const char* Seis::PreLoader::sKeyLines()	{ return "Lines"; }
const char* Seis::PreLoader::sKeyAttrs()	{ return "Attributes"; }


IOObj* Seis::PreLoader::getIOObj() const
{
    IOObj* ret = IOM().get( id_ );
    if ( !ret )
	errmsg_ = tr("Cannot find ID in object manager");
    return ret;
}


Interval<int> Seis::PreLoader::inlRange() const
{
    Interval<int> ret( mUdf(int), -mUdf(int) );
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( id_.buf(), fnms );
    for ( int idx=0; idx<fnms.size(); idx++ )
	ret.include( SeisCBVSPSIO::getInlNr( fnms.get(idx) ), false );

    if ( mIsUdf(ret.start) ) ret.stop = ret.start;
    return ret;
}


void Seis::PreLoader::getLineNames( BufferStringSet& lks ) const
{
    lks.erase(); PtrMan<IOObj> ioobj = getIOObj();
    if ( !ioobj ) return;

    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( id_.buf(), fnms );
    if ( fnms.isEmpty() ) return;
    BufferStringSet nms;
    Seis2DDataSet ds( *ioobj );
    const int nrlns = ds.nrLines();
    for ( int iln=0; iln<nrlns; iln++ )
    {
	const char* fnm = SeisCBVS2DLineIOProvider::getFileName( *ioobj,
							ds.geomID(iln) );
	if ( fnms.isPresent(fnm) )
	    lks.add( ds.lineName(iln) );
    }
}


#define mPrepIOObj(is2dln) \
    PtrMan<IOObj> ioobj = getIOObj(); \
    if ( !ioobj ) \
	return false; \
    if ( !is2dln && ioobj->translator()!= \
		CBVSSeisTrcTranslator::translKey() ) \
	{ errmsg_ = "Cannot pre-load other than CBVS data"; return false; } \
    TaskRunner& trunnr mUnusedVar = getTr()


bool Seis::PreLoader::loadVol() const
{
    mPrepIOObj( false );

    BufferStringSet fnms;
    const BufferString basefnm = CBVSIOMgr::baseFileName(
				 ioobj->fullUserExpr(true) );
    for ( int idx=0; true; idx++ )
    {
	const BufferString fnm( CBVSIOMgr::getFileName(basefnm,idx) );
	if ( File::exists(fnm) )
	    fnms.add( fnm );
	else
	    break;
    }

    return fnms.isEmpty() ? true
	: StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadLines( const BufferStringSet& lnms ) const
{
    if ( lnms.isEmpty() )
	{ errmsg_ = "Internal: No lines requested"; return false; }
    mPrepIOObj( true );

    Seis2DDataSet ds( *ioobj );
    const int nrlns = ds.nrLines();
    if ( nrlns < 1 )
    {
	errmsg_ = tr("Data set '%1' contains no data").arg( ioobj->name() );
	return false;
    }

    BufferStringSet fnms;
    for ( int iln=0; iln<nrlns; iln++ )
    {
	const char* lnm = ds.lineName( iln );
	if ( !lnms.isPresent(lnm) )
	    continue;

	fnms.add( SeisCBVS2DLineIOProvider::getFileName(*ioobj,
							ds.geomID(iln)) );
    }

    return fnms.isEmpty() ? true
	 : StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadLines() const
{
    mPrepIOObj( true );
    Seis2DDataSet ds( *ioobj );
    const int nrlns = ds.nrLines();
    if ( nrlns < 1 )
    {
	errmsg_ = tr( "Data Set '%1' contains no data").arg( ioobj->name() );
	return false;
    }

    BufferStringSet fnms;
    for ( int iln=0; iln<nrlns; iln++ )
	fnms.add( SeisCBVS2DLineIOProvider::getFileName(*ioobj,
							ds.geomID(iln)) );

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
    BufferStringSet lnms;
    if ( lnm && *lnm )
	lnms.add( lnm );
    else
	SPSIOPF().getLineNames( *ioobj, lnms );

    return loadPS2D( lnms );
}


bool Seis::PreLoader::loadPS2D( const BufferStringSet& lnms ) const
{
    if ( lnms.isEmpty() )
	return true;

    mPrepIOObj( false );

    BufferStringSet fnms;
    SeisCBVSPSIO psio( ioobj->fullUserExpr(true) );
    for ( int idx=0; idx<lnms.size(); idx++ )
	fnms.add( psio.get2DFileName(lnms.get(idx)) );

    return fnms.isEmpty() ? true
	: StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


void Seis::PreLoader::unLoad() const
{
    StreamProvider::unLoad( id_.buf(), true );
}


void Seis::PreLoader::load( const IOPar& iniop, TaskRunner* tr )
{
    PtrMan<IOPar> iop = iniop.subselect( "Seis" );
    if ( !iop || iop->isEmpty() ) return;

    for ( int ipar=0; ; ipar++ )
    {
	PtrMan<IOPar> objiop = iop->subselect( ipar );
	if ( !objiop || objiop->isEmpty() )
	    { if ( ipar ) break; continue; }
	loadObj( *objiop, tr );
    }
}


void Seis::PreLoader::loadObj( const IOPar& iop, TaskRunner* tr )
{
    const char* id = iop.find( sKey::ID() );
    if ( !id || !*id ) return;

    const MultiID ky( id );
    SeisIOObjInfo oinf( ky );
    if ( !oinf.isOK() ) return;

    Seis::PreLoader spl( ky, tr );
    const Seis::GeomType gt = oinf.geomType();
    switch ( gt )
    {
	case Seis::Vol:
	    spl.loadVol();
	break;
	case Seis::Line: {
	    BufferStringSet lnms;
	    iop.get( sKeyLines(), lnms );
	    if ( lnms.isEmpty() )
		spl.loadLines();
	    else
		spl.loadLines( lnms );
	} break;
	case Seis::VolPS: {
	    Interval<int> nrrg; Interval<int>* toload = 0;
	    if ( iop.get(sKey::Range(),nrrg) )
		toload = &nrrg;
	    spl.loadPS3D( toload );
	} break;
	case Seis::LinePS: {
	    BufferStringSet lnms; iop.get( sKeyLines(), lnms );
	    if ( lnms.isEmpty() )
		spl.loadPS2D();
	    else
		spl.loadPS2D( lnms );
	} break;
    }
}


void Seis::PreLoader::fillPar( IOPar& iop ) const
{
    SeisIOObjInfo oinf( id_ );
    if ( !oinf.isOK() ) return;
    iop.set( sKey::ID(), id_.buf() );
    const Seis::GeomType gt = oinf.geomType();

    switch ( gt )
    {
	case Seis::Vol:
	break;
	case Seis::Line: {
	    BufferStringSet lnms; getLineNames( lnms );
	    if ( !lnms.isEmpty() )
		iop.set( sKeyLines(), lnms );
	} break;
	case Seis::VolPS: {
	    iop.set( sKey::Range(), inlRange() );
	} break;
	case Seis::LinePS: {
	    BufferStringSet fnms;
	    StreamProvider::getPreLoadedFileNames( id_.buf(), fnms );
	    if ( fnms.isEmpty() ) break;
	    BufferStringSet lnms;
	    for ( int idx=0; idx<fnms.size(); idx++ )
	    {
		FilePath fp( fnms.get(idx) );
		fp.setExtension( 0, true );
		lnms.add( fp.fileName() );
	    }
	    iop.set( sKeyLines(), lnms );
	} break;
    }
}
