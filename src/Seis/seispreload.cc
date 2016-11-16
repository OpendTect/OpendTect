/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/

#include "seispreload.h"

#include "cbvsio.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
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
#include "uistrings.h"


namespace Seis
{

const char* PreLoader::sKeyLines()	{ return "Lines"; }
const char* PreLoader::sKeyUserType()	{ return "User Type"; }

PreLoader::PreLoader( const DBKey& mid, Pos::GeomID geomid, TaskRunner* tskrn )
    : dbkey_(mid), geomid_(geomid), tr_(tskrn)
{}


IOObj* PreLoader::getIOObj() const
{
    IOObj* ret = DBM().get( dbkey_ );
    if ( !ret )
	errmsg_ = uiStrings::phrCannotFindDBEntry( toUiString(dbkey_) );
    return ret;
}


Interval<int> PreLoader::inlRange() const
{
    Interval<int> ret( mUdf(int), -mUdf(int) );
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( dbkey_.toString(), fnms );
    for ( int idx=0; idx<fnms.size(); idx++ )
	ret.include( SeisCBVSPSIO::getInlNr( fnms.get(idx) ), false );

    if ( mIsUdf(ret.start) ) ret.stop = ret.start;
    return ret;
}


void PreLoader::getLineNames( BufferStringSet& lks ) const
{
    lks.erase(); PtrMan<IOObj> ioobj = getIOObj();
    if ( !ioobj ) return;

    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( dbkey_.toString(), fnms );
    if ( fnms.isEmpty() ) return;

    BufferStringSet nms;
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	File::Path fp( fnms.get(idx) );
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


#define mPrepIOObj() \
    PtrMan<IOObj> ioobj = getIOObj(); \
    if ( !ioobj ) \
	return false; \
    TaskRunner& trunnr mUnusedVar = getTr()


bool PreLoader::load( const TrcKeyZSampling& tkzs,
			 DataCharacteristics::UserType type,
			 Scaler* scaler ) const
{
    mPrepIOObj();

    const SeisIOObjInfo info( dbkey_ );
    if ( info.is2D() )
    {
	ParallelReader2D rdr( *ioobj, geomid_, tkzs.isDefined() ? &tkzs : 0 );
	rdr.setScaler( scaler );
	rdr.setDataChar( type );
	if ( !trunnr.execute(rdr) )
	{
	    errmsg_ = rdr.message();
	    return false;
	}

	RegularSeisDataPack* dp = rdr.getDataPack();
	if ( !dp ) return false;

	PLDM().add( dbkey_, geomid_, dp );
    }
    else
    {
	SequentialReader rdr( *ioobj, tkzs.isDefined() ? &tkzs : 0 );
	uiString caption = tr("Pre-loading %1");
	caption.arg( ioobj->uiName() );
	rdr.setName( caption.getFullString() );
	rdr.setScaler( scaler );
	rdr.setDataChar( type );
	if ( !trunnr.execute(rdr) )
	{
	    errmsg_ = rdr.message();
	    return false;
	}

	RegularSeisDataPack* dp = rdr.getDataPack();
	if ( !dp ) return false;

	PLDM().add( dbkey_, geomid_, dp );
    }

    return true;
}


bool PreLoader::load( const TypeSet<TrcKeyZSampling>& tkzss,
		      const TypeSet<Pos::GeomID>& geomids,
			 DataCharacteristics::UserType type,
			 Scaler* scaler ) const
{
    mPrepIOObj();

    TaskGroup taskgrp;
    ObjectSet<ParallelReader2D> rdrs;
    TypeSet<Pos::GeomID> loadedgeomids;
    for ( int idx=0; idx<tkzss.size(); idx++ )
    {
	const TrcKeyZSampling& tkzs = tkzss[idx];
	const Pos::GeomID& geomid = geomids[idx];

	ParallelReader2D* rdr =
	    new ParallelReader2D( *ioobj, geomid, tkzs.isDefined() ? &tkzs : 0);
	rdr->setScaler( scaler ); rdr->setDataChar( type );
	taskgrp.addTask( rdr );
	loadedgeomids.add( geomid );
	rdrs.add( rdr );
    }

    TaskRunner::execute( &trunnr, taskgrp );

    for ( int idx=0; idx<rdrs.size(); idx++ )
    {
	ParallelReader2D& rdr = *rdrs[idx];
	const Pos::GeomID& loadedgeomid = loadedgeomids[idx];
	RegularSeisDataPack* dp = rdr.getDataPack();
	if ( !dp ) continue;
	PLDM().add( dbkey_, loadedgeomid, dp );
    }

    return true;
}


bool PreLoader::loadPS3D( const Interval<int>* inlrg ) const
{
    mPrepIOObj();

    SeisCBVSPSIO psio( ioobj->fullUserExpr(true) );
    BufferStringSet fnms;
    if ( !psio.get3DFileNames(fnms,inlrg) )
	{ errmsg_ = psio.errMsg(); return false; }

    return fnms.isEmpty() ? true
	 : StreamProvider::preLoad( fnms, trunnr, dbkey_.toString() );
}


bool PreLoader::loadPS2D( const char* lnm ) const
{
    mPrepIOObj();
    BufferStringSet lnms;
    if ( lnm && *lnm )
	lnms.add( lnm );
    else
	SPSIOPF().getLineNames( *ioobj, lnms );

    return loadPS2D( lnms );
}


bool PreLoader::loadPS2D( const BufferStringSet& lnms ) const
{
    if ( lnms.isEmpty() )
	return true;

    mPrepIOObj();

    BufferStringSet fnms;
    SeisCBVSPSIO psio( ioobj->fullUserExpr(true) );
    for ( int idx=0; idx<lnms.size(); idx++ )
	fnms.add( psio.get2DFileName(lnms.get(idx)) );

    return fnms.isEmpty() ? true
	: StreamProvider::preLoad( fnms, trunnr, dbkey_.toString() );
}


void PreLoader::unLoad() const
{
    PLDM().remove( dbkey_, geomid_ );
}


void PreLoader::load( const IOPar& iniop, TaskRunner* tskr )
{
    PtrMan<IOPar> iop = iniop.subselect( "Seis" );
    if ( !iop || iop->isEmpty() ) return;

    for ( int ipar=0; ; ipar++ )
    {
	PtrMan<IOPar> objiop = iop->subselect( ipar );
	if ( !objiop || objiop->isEmpty() )
	    { if ( ipar ) break; continue; }
	loadObj( *objiop, tskr );
    }
}


void PreLoader::loadObj( const IOPar& iop, TaskRunner* tskr )
{
    DBKey dbky;
    iop.get( sKey::ID(), dbky );
    if ( dbky.isInvalid() )
	return;

    Pos::GeomID geomid = -1;
    iop.get( sKey::GeomID(), geomid );

    SeisIOObjInfo info( dbky );
    if ( !info.isOK() ) return;

    DataCharacteristics dc; info.getDataChar( dc );
    DataCharacteristics::UserType usertype( dc.userType() );
    DataCharacteristics::UserTypeDef().parse( iop, sKeyUserType(), usertype );

    BufferString scalerstr;
    iop.get( sKey::Scale(), scalerstr );
    Scaler* scaler = !scalerstr.isEmpty() ? Scaler::get(scalerstr.buf()) : 0;

    PLDM().remove( dbky, geomid );

    PreLoader spl( dbky, geomid, tskr );
    const GeomType gt = info.geomType();
    switch ( gt )
    {
	case Vol: {
	    TrcKeyZSampling tkzs(true);
	    tkzs.usePar( iop );
	    spl.load( tkzs, usertype, scaler );
	} break;
	case Line: {
	    TrcKeyZSampling tkzs( false );
	    tkzs.usePar( iop );
	    spl.load( tkzs, usertype, scaler );
	} break;
	case VolPS: {
	    Interval<int> nrrg; Interval<int>* toload = 0;
	    if ( iop.get(sKey::Range(),nrrg) )
		toload = &nrrg;
	    spl.loadPS3D( toload );
	} break;
	case LinePS: {
	    BufferStringSet lnms; iop.get( sKeyLines(), lnms );
	    if ( lnms.isEmpty() )
		spl.loadPS2D();
	    else
		spl.loadPS2D( lnms );
	} break;
    }
}


void PreLoader::fillPar( IOPar& iop ) const
{
    SeisIOObjInfo oinf( dbkey_ );
    if ( !oinf.isOK() ) return;

    iop.set( sKey::ID(), dbkey_ );
    iop.set( sKey::GeomID(), geomid_ );
    RefMan<DataPack> dp = PLDM().get( dbkey_, geomid_ );
    mDynamicCastGet(const RegularSeisDataPack*,regsdp,dp.ptr());
    if ( regsdp )
    {
	iop.set( sKeyUserType(), DataCharacteristics::toString(
		    DataCharacteristics(regsdp->getDataDesc()).userType()) );
	regsdp->sampling().fillPar( iop );
	const Scaler* scaler = regsdp->getScaler();
	if ( scaler )
	{
	    BufferString info;
	    scaler->put( info.getCStr() );
	    iop.set( sKey::Scale(), info.buf() );
	}
    }

    switch ( oinf.geomType() )
    {
	case Vol:
	break;
	case Line: {
	    BufferStringSet lnms; getLineNames( lnms );
	    if ( !lnms.isEmpty() )
		iop.set( sKeyLines(), lnms );
	} break;
	case VolPS: {
	    iop.set( sKey::Range(), inlRange() );
	} break;
	case LinePS: {
	    BufferStringSet fnms;
	    StreamProvider::getPreLoadedFileNames( dbkey_.toString(), fnms );
	    if ( fnms.isEmpty() ) break;
	    BufferStringSet lnms;
	    for ( int idx=0; idx<fnms.size(); idx++ )
	    {
		File::Path fp( fnms.get(idx) );
		fp.setExtension( 0, true );
		lnms.add( fp.fileName() );
	    }
	    iop.set( sKeyLines(), lnms );
	} break;
    }
}


// PreLoadDataEntry
PreLoadDataEntry::PreLoadDataEntry( const DBKey& dbky, Pos::GeomID geomid,
				    int dpid )
    : dbkey_(dbky), geomid_(geomid), dpid_(dpid), is2d_(geomid!=-1)
{
    name_ = DBM().nameOf( dbky );
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    is2d_ = geom && geom->is2D();
    if ( is2d_ )
    {
	name_.add( " - " );
	name_.add( geom->getName() );
    }
}


bool PreLoadDataEntry::equals( const DBKey& dbky, Pos::GeomID geomid ) const
{
    return dbkey_==dbky && geomid_==geomid;
}



// PreLoadDataManager
PreLoadDataManager::PreLoadDataManager()
    : dpmgr_(DPM(DataPackMgr::SeisID()))
{
}


PreLoadDataManager::~PreLoadDataManager()
{
}


void PreLoadDataManager::add( const DBKey& dbky, DataPack* dp )
{ add( dbky, -1, dp ); }


void PreLoadDataManager::add( const DBKey& dbky, Pos::GeomID geomid,
			      DataPack* dp )
{
    if ( !dp ) return;

    dp->ref();
    dpmgr_.add( dp );

    entries_ += new PreLoadDataEntry( dbky, geomid, dp->id().getI() );
}


void PreLoadDataManager::remove( const DBKey& dbky, Pos::GeomID geomid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->equals(dbky,geomid) )
	    return remove( entries_[idx]->dpid_ );
    }
}


void PreLoadDataManager::remove( int dpid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->dpid_ == dpid )
	{
	    entries_.removeSingle( idx );
	    dpmgr_.unRef( DataPack::ID::get(dpid) );
	    return;
	}
    }
}


void PreLoadDataManager::removeAll()
{
    while ( entries_.size() )
    {
	dpmgr_.unRef( DataPack::ID::get(entries_[0]->dpid_) );
	entries_.removeSingle( 0 );
    }
}


RefMan<DataPack>
PreLoadDataManager::get( const DBKey& dbky, Pos::GeomID geomid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->equals(dbky,geomid) )
	    return get( entries_[idx]->dpid_ );
    }

    return 0;
}


ConstRefMan<DataPack> PreLoadDataManager::get( const DBKey& dbky,
					 Pos::GeomID geomid ) const
{ return const_cast<PreLoadDataManager*>(this)->get( dbky, geomid ); }


RefMan<DataPack> PreLoadDataManager::get( int dpid )
{
    const DataPack::ID id = DataPack::ID::get(dpid);
    return dpmgr_.haveID(id) ? dpmgr_.get( id ) : 0;
}


ConstRefMan<DataPack> PreLoadDataManager::get( int dpid ) const
{ return const_cast<PreLoadDataManager*>(this)->get( dpid ); }


void PreLoadDataManager::getInfo( const DBKey& dbky, Pos::GeomID geomid,
				  BufferString& info ) const
{
    const DataPack* dp = get( dbky, geomid );
    if ( !dp ) return;

    IOPar par; dp->dumpInfo( par );
    par.dumpPretty( info );
}


void PreLoadDataManager::getIDs( DBKeySet& ids ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
	ids += entries_[idx]->dbkey_;
}


bool PreLoadDataManager::isPresent( const DBKey& dbky,
				    Pos::GeomID geomid ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->equals(dbky,geomid) )
	    return true;
    }

    return false;
}


const ObjectSet<PreLoadDataEntry>& PreLoadDataManager::getEntries() const
{ return entries_; }


PreLoadDataManager& PLDM()
{
    mDefineStaticLocalObject(PtrMan<PreLoadDataManager>,pldm,
			     (new PreLoadDataManager));
    return *pldm;
}

} // namespace Seis
