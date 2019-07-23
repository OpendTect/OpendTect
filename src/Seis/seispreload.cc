/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/

#include "seispreload.h"

#include "cbvsio.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "prestackgather.h"
#include "seiscbvs.h"
#include "seiscbvsps.h"
#include "seiscbvs2d.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisloader.h"
#include "seispsioprov.h"
#include "seis2ddata.h"
#include "strmprov.h"
#include "uistrings.h"


namespace Seis
{

const char* PreLoader::sKeyLines()	{ return "Lines"; }
const char* PreLoader::sKeyUserType()	{ return "User Type"; }

PreLoader::PreLoader( const DBKey& dbky )
    : dbkey_(dbky), trprov_(0) {}
PreLoader::PreLoader( const DBKey& dbky, const TaskRunnerProvider& trprv )
    : dbkey_(dbky), trprov_(&trprv) {}


IOObj* PreLoader::getIOObj() const
{
    IOObj* ret = ::getIOObj( dbkey_ );
    if ( !ret )
	errmsg_ = uiStrings::phrCannotFindDBEntry( dbkey_ );
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


bool PreLoader::runTask( Task& tsk ) const
{
    if ( trprov_ )
	return trprov_->execute( tsk );
    return tsk.execute();
}


#define mPrepIOObj() \
    PtrMan<IOObj> ioobj = getIOObj(); \
    if ( !ioobj ) \
	return false; \

bool PreLoader::load( const TrcKeyZSampling& tkzs,
			 DataCharacteristics::UserType type,
			 const Scaler* scaler ) const
{
    mPrepIOObj();
    const uiString caption = tr("Pre-loading '%1'").arg( ioobj->name() );

    TrcKeyZSampling tkzstoload( tkzs );
    const SeisIOObjInfo info( dbkey_ );
    if ( !tkzs.hsamp_.isDefined() && info.is2D() )
	tkzstoload.hsamp_ = TrcKeySampling( defgeomid_ );

    SequentialFSLoader rdr( *ioobj, tkzstoload.isDefined() ? &tkzstoload : 0 );
    rdr.setName( toString(caption) );
    rdr.setScaler( scaler );
    rdr.setDataChar( type );
    if ( !runTask(rdr) )
    {
	errmsg_ = rdr.message();
	return false;
    }

    ConstRefMan<RegularSeisDataPack> dp = rdr.getDataPack();
    if ( !dp ) return false;

    PLDM().add( dbkey_, defgeomid_, dp.getNonConstPtr() );

    return true;
}


bool PreLoader::load( const TypeSet<TrcKeyZSampling>& tkzss,
		      const GeomIDSet& geomids,
		      DataCharacteristics::UserType type,
		      const Scaler* scaler ) const
{
    mPrepIOObj();
    const uiString caption = tr("Pre-loading '%1'").arg( ioobj->name() );

    TaskGroup taskgrp;
    ObjectSet<SequentialFSLoader> rdrs;
    GeomIDSet loadedgeomids;
    for ( int idx=0; idx<tkzss.size(); idx++ )
    {
	const Pos::GeomID& geomid = geomids[idx];
	TrcKeyZSampling tkzs( tkzss[idx] );
	if ( !tkzs.hsamp_.isDefined() )
	    tkzs.hsamp_ = TrcKeySampling( geomid );

	SequentialFSLoader* rdr = new SequentialFSLoader( *ioobj, &tkzs );
	rdr->setName( toString(caption) );
	rdr->setScaler( scaler );
	rdr->setDataChar( type );
	taskgrp.addTask( rdr );
	loadedgeomids.add( geomid );
	rdrs.add( rdr );
    }

    runTask( taskgrp );

    for ( int idx=0; idx<rdrs.size(); idx++ )
    {
	SequentialFSLoader& rdr = *rdrs[idx];
	const Pos::GeomID& loadedgeomid = loadedgeomids[idx];
	ConstRefMan<RegularSeisDataPack> dp = rdr.getDataPack();
	if ( !dp ) continue;
	PLDM().add( dbkey_, loadedgeomid, dp.getNonConstPtr() );
    }

    return true;
}


bool PreLoader::loadPS3D( const Interval<int>* inlrg ) const
{
    mPrepIOObj();

    Seis::SequentialPSLoader psrdr( *ioobj, inlrg );
    if ( !runTask(psrdr) )
	return false;

    PLDM().add( ioobj->key(), psrdr.getPSDataPack() );
    return true;
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

    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const auto geomid = SurvGeom::getGeomID( lnms.get(idx) );
	Seis::SequentialPSLoader psrdr( *ioobj, 0, geomid );
	if ( !runTask(psrdr) )
	    return false;

	PLDM().add( ioobj->key(), geomid, psrdr.getPSDataPack() );
    }

    return true;
}


void PreLoader::unLoad() const
{
    PLDM().remove( dbkey_, GeomID() );
}


void PreLoader::unLoad( GeomID gid ) const
{
    PLDM().remove( dbkey_, gid );
}


void PreLoader::load( const IOPar& iop )
{ doLoad(iop,0); }
void PreLoader::load( const IOPar& iop, const TaskRunnerProvider& trprov )
{ doLoad(iop,&trprov); }


void PreLoader::doLoad( const IOPar& iniop, const TaskRunnerProvider* trprov )
{
    PtrMan<IOPar> iop = iniop.subselect( "Seis" );
    if ( !iop || iop->isEmpty() ) return;

    for ( int ipar=0; ; ipar++ )
    {
	PtrMan<IOPar> objiop = iop->subselect( ipar );
	if ( !objiop || objiop->isEmpty() )
	    { if ( ipar ) break; continue; }
	doLoadObj( *objiop, trprov );
    }
}


void PreLoader::loadObj( const IOPar& iop )
{ doLoadObj(iop,0); }
void PreLoader::loadObj( const IOPar& iop, const TaskRunnerProvider& trprov )
{ doLoadObj(iop,&trprov); }


void PreLoader::doLoadObj( const IOPar& iop, const TaskRunnerProvider* trprov )
{
    DBKey dbky;
    iop.get( sKey::ID(), dbky );
    if ( dbky.isInvalid() )
	return;

    Pos::GeomID geomid = Pos::GeomID::get3D();
    iop.get( sKey::GeomID(), geomid );

    SeisIOObjInfo info( dbky );
    if ( !info.isOK() )
	return;

    DataCharacteristics dc; info.getDataChar( dc );
    DataCharacteristics::UserType usertype( dc.userType() );
    DataCharacteristics::UserTypeDef().parse( iop, sKeyUserType(), usertype );

    BufferString scalerstr;
    iop.get( sKey::Scale(), scalerstr );
    Scaler* scaler = !scalerstr.isEmpty() ? Scaler::get(scalerstr.buf()) : 0;

    PLDM().remove( dbky, geomid );

    PreLoader spl( dbky );
    if ( trprov )
	spl.setTaskRunner( *trprov );
    const GeomType gt = info.geomType();
    switch ( gt )
    {
	case Vol: {
	    TrcKeyZSampling tkzs( true );
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
    if ( defgeomid_.isValid() )
    {
	iop.set( sKey::GeomID(), defgeomid_ );
	auto regsdp = PLDM().get<RegularSeisDataPack>( dbkey_, defgeomid_ );
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
				    DataPack::ID dpid )
    : dbkey_(dbky), geomid_(geomid), dpid_(dpid), is2d_(geomid.is2D())
{
    name_ = dbky.name();
    is2d_ = geomid.is2D();
    if ( is2d_ )
    {
	name_.add( " - " );
	name_.add( geomid.name() );
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
{ add( dbky, Pos::GeomID::get3D(), dp ); }


void PreLoadDataManager::add( const DBKey& dbky, Pos::GeomID geomid,
			      DataPack* dp )
{
    if ( !dp ) return;

    dp->ref();
    dp->setDBKey( dbky );
    dpmgr_.add( dp );

    entries_ += new PreLoadDataEntry( dbky, geomid, dp->id() );
}


void PreLoadDataManager::remove( const DBKey& dbky, Pos::GeomID geomid )
{
    if ( !geomid.isValid() )
	removeAll();
    else
	for ( int idx=0; idx<entries_.size(); idx++ )
	    if ( entries_[idx]->equals(dbky,geomid) )
		remove( entries_[idx]->dpid_ );
}


void PreLoadDataManager::remove( PackID dpid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->dpid_ == dpid )
	{
	    entries_.removeSingle( idx );
	    dpmgr_.unRef( dpid );
	    return;
	}
    }
}


void PreLoadDataManager::removeAll()
{
    while ( entries_.size() )
    {
	dpmgr_.unRef( entries_[0]->dpid_ );
	entries_.removeSingle( 0 );
    }
}


RefMan<DataPack>
PreLoadDataManager::getDP( const DBKey& dbky, Pos::GeomID geomid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->equals(dbky,geomid) )
	    return getDP( entries_[idx]->dpid_ );
    }

    return 0;
}


ConstRefMan<DataPack> PreLoadDataManager::getDP( const DBKey& dbky,
					 Pos::GeomID geomid ) const
{ return const_cast<PreLoadDataManager*>(this)->getDP( dbky, geomid ); }


void PreLoadDataManager::getInfo( const DBKey& dbky, Pos::GeomID geomid,
				  BufferString& info ) const
{
    auto dp = getDP( dbky, geomid );
    if ( !dp )
	return;

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
