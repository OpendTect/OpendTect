/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID mUsedVar = "$Id$";

#include "seispreload.h"

#include "cbvsio.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "perthreadrepos.h"
#include "seiscbvs.h"
#include "seiscbvsps.h"
#include "seiscbvs2d.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "seispsioprov.h"
#include "seis2ddata.h"
#include "strmprov.h"


const char* Seis::PreLoader::sKeyLines()	{ return "Lines"; }

Seis::PreLoader::PreLoader( const MultiID& mid, TaskRunner* trn )
    : id_(mid), tr_(trn)
{}


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
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	FilePath fp( fnms.get(idx) );
	nms.add( fp.fileName() );
    }

    Seis2DDataSet ds( *ioobj );
    const int nrlns = ds.nrLines();
    for ( int iln=0; iln<nrlns; iln++ )
    {
	const char* fnm =
		SeisCBVS2DLineIOProvider::getFileName( *ioobj, ds.geomID(iln) );
	if ( nms.isPresent(fnm) )
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


bool Seis::PreLoader::loadVol( const TrcKeyZSampling& tkzs ) const
{
    mPrepIOObj( false );

    Seis::SequentialReader rdr( *ioobj, &tkzs );
    if ( !trunnr.execute(rdr) )
    {
	errmsg_ = rdr.uiMessage();
	return false;
    }

    RegularSeisDataPack* dp = rdr.getDataPack();
    Seis::PLDM().add( ioobj->key(), dp );
    return true;
}


bool Seis::PreLoader::loadLine( Pos::GeomID geomid,
				const TrcKeyZSampling& tkzs ) const
{
    mPrepIOObj( true );

    Seis::ParallelReader2D rdr( *ioobj, geomid, &tkzs );
    if ( !trunnr.execute(rdr) )
    {
	errmsg_ = rdr.uiMessage();
	return false;
    }

    Seis::PLDM().add( ioobj->key(), rdr.getDataPack() );
    return true;
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
    Seis::PLDM().remove( id_ );
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
	case Seis::Vol: {
	    TrcKeyZSampling tkzs(true);
	    tkzs.usePar( iop );
	    spl.loadVol( tkzs );
	} break;
	case Seis::Line: {
	    BufferStringSet lnms;
	    iop.get( sKeyLines(), lnms );
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



Seis::PreLoadDataManager::PreLoadDataManager()
    : dpmgr_(DPM(DataPackMgr::SeisID()))
{
}


Seis::PreLoadDataManager::~PreLoadDataManager()
{
    dpmgr_.releaseAll( true );
}


void Seis::PreLoadDataManager::add( const MultiID& mid, DataPack* dp )
{
    if ( !dp ) return;

    mids_ += mid;
    dpids_ += dp->id();
    dpmgr_.addAndObtain( dp );
}


void Seis::PreLoadDataManager::remove( const MultiID& mid )
{
    const int idx = mids_.indexOf( mid );
    if ( idx<0 ) return;

    const int dpid = dpids_[idx];
    remove( dpid );
}


void Seis::PreLoadDataManager::remove( int dpid )
{
    const int idx = dpids_.indexOf( dpid );
    if ( idx<0 ) return;

    mids_.removeSingle( idx );
    dpids_.removeSingle( idx );
    dpmgr_.release( dpid );
}


DataPack* Seis::PreLoadDataManager::get( const MultiID& mid )
{
    const int idx = mids_.indexOf( mid );
    if ( idx<0 ) return 0;

    const int dpid = dpids_[idx];
    return get( dpid );
}


const DataPack* Seis::PreLoadDataManager::get( const MultiID& mid ) const
{ return const_cast<Seis::PreLoadDataManager*>(this)->get( mid ); }


DataPack* Seis::PreLoadDataManager::get( int dpid )
{
    const int idx = dpids_.indexOf( dpid );
    return idx<0 ? 0 : dpmgr_.observe( dpid );
}


const DataPack* Seis::PreLoadDataManager::get( int dpid ) const
{ return const_cast<Seis::PreLoadDataManager*>(this)->get( dpid ); }


void Seis::PreLoadDataManager::getInfo( const MultiID& mid,
					BufferString& info ) const
{
    const DataPack* dp = get( mid );
    if ( !dp ) return;

    IOPar par; dp->dumpInfo( par );
    par.dumpPretty( info );
}


void Seis::PreLoadDataManager::getIDs( TypeSet<MultiID>& ids ) const
{ ids = mids_; }


Seis::PreLoadDataManager& Seis::PLDM()
{
    mDefineStaticLocalObject(PtrMan<Seis::PreLoadDataManager>,pldm,
			     (new Seis::PreLoadDataManager));
    return *pldm;
}
