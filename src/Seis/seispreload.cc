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


const char* Seis::PreLoader::sKeyLines()	{ return "Lines"; }
const char* Seis::PreLoader::sKeyUserType()	{ return "User Type"; }

Seis::PreLoader::PreLoader( const DBKey& dbky )
    : dbkey_(dbky), trprov_(0) {}
Seis::PreLoader::PreLoader( const DBKey& dbky, const TaskRunnerProvider& trprv )
    : dbkey_(dbky), trprov_(&trprv) {}


IOObj* Seis::PreLoader::getIOObj() const
{
    IOObj* ret = ::getIOObj( dbkey_ );
    if ( !ret )
	errmsg_ = uiStrings::phrCannotFindDBEntry( dbkey_ );
    return ret;
}


Interval<int> Seis::PreLoader::inlRange() const
{
    Interval<int> ret( mUdf(int), -mUdf(int) );
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( dbkey_.toString(), fnms );
    for ( int idx=0; idx<fnms.size(); idx++ )
	ret.include( SeisCBVSPSIO::getInlNr( fnms.get(idx) ), false );

    if ( mIsUdf(ret.start) )
	ret.stop = ret.start;
    return ret;
}


void Seis::PreLoader::getLineNames( BufferStringSet& lks ) const
{
    lks.erase(); PtrMan<IOObj> ioobj = getIOObj();
    if ( !ioobj )
	return;

    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( dbkey_.toString(), fnms );
    if ( fnms.isEmpty() )
	return;

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


bool Seis::PreLoader::runTask( Task& tsk ) const
{
    if ( trprov_ )
	return trprov_->execute( tsk );
    return tsk.execute();
}


#define mPrepIOObj() \
    PtrMan<IOObj> ioobj = getIOObj(); \
    if ( !ioobj ) \
	return false; \

bool Seis::PreLoader::load( const GeomSubSel* reqss,
			 DataCharacteristics::UserType type,
			 const Scaler* scaler ) const
{
    mPrepIOObj();
    const uiString caption = tr("Pre-loading '%1'").arg( ioobj->name() );

    PtrMan<GeomSubSel> ss2load;
    if ( reqss )
	ss2load = reqss->duplicate();
    else if ( defgeomid_.is3D() )
	ss2load = new CubeSubSel;
    else
	ss2load = new LineSubSel( defgeomid_ );

    SequentialFSLoader rdr( *ioobj, ss2load );
    rdr.setName( toString(caption) );
    rdr.setScaler( scaler );
    rdr.setDataChar( type );
    if ( !runTask(rdr) )
	{ errmsg_ = rdr.message(); return false; }

    ConstRefMan<RegularSeisDataPack> dp = rdr.getDataPack();
    if ( !dp )
	return false;

    PLDM().add( dbkey_, defgeomid_, dp.getNonConstPtr() );

    return true;
}


bool Seis::PreLoader::load( const ObjectSet<GeomSubSel>& sss,
		      DataCharacteristics::UserType type,
		      const Scaler* scaler ) const
{
    mPrepIOObj();
    const uiString caption = tr("Pre-loading '%1'").arg( ioobj->name() );

    TaskGroup taskgrp;
    ObjectSet<SequentialFSLoader> rdrs;
    GeomIDSet loadedgeomids;
    for ( int idx=0; idx<sss.size(); idx++ )
    {
	const auto* gss = sss.get( idx );
	if ( !gss )
	    continue;

	SequentialFSLoader* rdr = new SequentialFSLoader( *ioobj, gss );
	rdr->setName( toString(caption) );
	rdr->setScaler( scaler );
	rdr->setDataChar( type );
	taskgrp.addTask( rdr );
	loadedgeomids.add( gss->geomID() );
	rdrs.add( rdr );
    }

    runTask( taskgrp );

    for ( int idx=0; idx<rdrs.size(); idx++ )
    {
	SequentialFSLoader& rdr = *rdrs[idx];
	const auto loadedgeomid = loadedgeomids[idx];
	ConstRefMan<RegularSeisDataPack> dp = rdr.getDataPack();
	if ( !dp )
	    continue;
	PLDM().add( dbkey_, loadedgeomid, dp.getNonConstPtr() );
    }

    return true;
}


bool Seis::PreLoader::loadPS3D( const Interval<int>* inlrg ) const
{
    mPrepIOObj();

    Seis::SequentialPSLoader psrdr( *ioobj, inlrg );
    if ( !runTask(psrdr) )
	return false;

    PLDM().add( ioobj->key(), psrdr.getPSDataPack() );
    return true;
}


bool Seis::PreLoader::loadPS2D( const char* lnm ) const
{
    mPrepIOObj();
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


void Seis::PreLoader::unLoad() const
{
    PLDM().remove( dbkey_, GeomID() );
}


void Seis::PreLoader::unLoad( GeomID gid ) const
{
    PLDM().remove( dbkey_, gid );
}


void Seis::PreLoader::load( const IOPar& iop )
{ doLoad(iop,0); }
void Seis::PreLoader::load( const IOPar& iop, const TaskRunnerProvider& trprov )
{ doLoad(iop,&trprov); }


void Seis::PreLoader::doLoad( const IOPar& iniop,
			      const TaskRunnerProvider* trprov )
{
    PtrMan<IOPar> iop = iniop.subselect( "Seis" );
    if ( !iop || iop->isEmpty() )
	return;

    for ( int ipar=0; ; ipar++ )
    {
	PtrMan<IOPar> objiop = iop->subselect( ipar );
	if ( !objiop || objiop->isEmpty() )
	    { if ( ipar ) break; continue; }
	doLoadObj( *objiop, trprov );
    }
}


void Seis::PreLoader::loadObj( const IOPar& iop )
{ doLoadObj(iop,0); }
void Seis::PreLoader::loadObj( const IOPar& iop,
				const TaskRunnerProvider& trprov )
{ doLoadObj(iop,&trprov); }


void Seis::PreLoader::doLoadObj( const IOPar& iop,
				 const TaskRunnerProvider* trprov )
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
	    CubeSubSel css;
	    css.usePar( iop );
	    spl.load( &css, usertype, scaler );
	} break;
	case Line: {
	    LineSubSel lss( geomid );
	    lss.usePar( iop );
	    spl.load( &lss, usertype, scaler );
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


void Seis::PreLoader::fillPar( IOPar& iop ) const
{
    SeisIOObjInfo oinf( dbkey_ );
    if ( !oinf.isOK() )
	return;

    iop.set( sKey::ID(), dbkey_ );
    if ( defgeomid_.isValid() )
    {
	iop.set( sKey::GeomID(), defgeomid_ );
	auto regsdp = PLDM().get<RegularSeisDataPack>( dbkey_, defgeomid_ );
	if ( regsdp )
	{
	    iop.set( sKeyUserType(), DataCharacteristics::toString(
		    DataCharacteristics(regsdp->getDataDesc()).userType()) );
	    regsdp->subSel().fillPar( iop );
	    const Scaler* scaler = regsdp->getScaler();
	    if ( scaler )
	    {
		BufferString info( 256, false );
		scaler->put( info.getCStr(), info.bufSize() );
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
Seis::PreLoadDataEntry::PreLoadDataEntry( const DBKey& dbky, GeomID geomid,
					  PackID dpid )
    : dbkey_(dbky), geomid_(geomid), dpid_(dpid), is2d_(geomid.is2D())
{
    name_ = dbky.name();
    is2d_ = geomid.is2D();
    if ( is2d_ )
	{ name_.add( " - " ); name_.add( geomid.name() ); }
}


bool Seis::PreLoadDataEntry::equals( const DBKey& dbky, GeomID geomid ) const
{
    return dbkey_==dbky && geomid_==geomid;
}



// PreLoadDataManager
Seis::PreLoadDataManager::PreLoadDataManager()
    : dpmgr_(DPM(DataPackMgr::SeisID()))
{
}


Seis::PreLoadDataManager::~PreLoadDataManager()
{
}


void Seis::PreLoadDataManager::add( const DBKey& dbky, DataPack* dp )
{
    add( dbky, Pos::GeomID::get3D(), dp );
}


void Seis::PreLoadDataManager::add( const DBKey& dbky, Pos::GeomID geomid,
			      DataPack* dp )
{
    if ( !dp ) return;

    dp->ref();
    dp->setDBKey( dbky );
    dpmgr_.add( dp );

    entries_ += new PreLoadDataEntry( dbky, geomid, dp->id() );
}


void Seis::PreLoadDataManager::remove( const DBKey& dbky, Pos::GeomID geomid )
{
    if ( !geomid.isValid() )
	removeAll();
    else
	for ( int idx=0; idx<entries_.size(); idx++ )
	    if ( entries_[idx]->equals(dbky,geomid) )
		remove( entries_[idx]->dpid_ );
}


void Seis::PreLoadDataManager::remove( PackID dpid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
	if ( entries_[idx]->dpid_ == dpid )
	    { entries_.removeSingle( idx ); dpmgr_.unRef( dpid ); return; }
}


void Seis::PreLoadDataManager::removeAll()
{
    while ( entries_.size() )
	{ dpmgr_.unRef( entries_[0]->dpid_ ); entries_.removeSingle( 0 ); }
}


RefMan<DataPack>
Seis::PreLoadDataManager::getDP( const DBKey& dbky, Pos::GeomID geomid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
	if ( entries_[idx]->equals(dbky,geomid) )
	    return getDP( entries_[idx]->dpid_ );

    return 0;
}


ConstRefMan<DataPack> Seis::PreLoadDataManager::getDP( const DBKey& dbky,
					 Pos::GeomID geomid ) const
{
    return const_cast<PreLoadDataManager*>(this)->getDP( dbky, geomid );
}


void Seis::PreLoadDataManager::getInfo( const DBKey& dbky, Pos::GeomID geomid,
				  BufferString& info ) const
{
    auto dp = getDP( dbky, geomid );
    if ( !dp )
	return;

    IOPar par; dp->dumpInfo( par );
    par.dumpPretty( info );
}


void Seis::PreLoadDataManager::getIDs( DBKeySet& ids ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
	ids += entries_[idx]->dbkey_;
}


bool Seis::PreLoadDataManager::isPresent( const DBKey& dbky,
				    Pos::GeomID geomid ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
	if ( entries_[idx]->equals(dbky,geomid) )
	    return true;

    return false;
}


Seis::PreLoadDataManager& Seis::PLDM()
{
    static Seis::PreLoadDataManager pldm;
    return pldm;
}
