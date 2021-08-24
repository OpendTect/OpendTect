/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/

#include "seispreload.h"

#include "cbvsio.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "mousecursor.h"
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
#include "task.h"
#include "uistrings.h"


namespace Seis
{

const char* PreLoader::sKeyLines()	{ return "Lines"; }
const char* PreLoader::sKeyUserType()	{ return "User Type"; }

PreLoader::PreLoader( const MultiID& mid, Pos::GeomID geomid, TaskRunner* trn )
    : mid_(mid), geomid_(geomid), tr_(trn)
{}


IOObj* PreLoader::getIOObj() const
{
    IOObj* ret = IOM().get( mid_ );
    if ( !ret )
	errmsg_ = uiStrings::phrCannotFindDBEntry( toUiString(mid_) );
    return ret;
}


Interval<int> PreLoader::inlRange() const
{
    Interval<int> ret( mUdf(int), -mUdf(int) );
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( mid_.buf(), fnms );
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
    StreamProvider::getPreLoadedFileNames( mid_.buf(), fnms );
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
    const uiString caption = tr("Pre-loading %1").arg( ioobj->uiName() );

    TrcKeyZSampling tkzstoload( tkzs );
    const SeisIOObjInfo info( mid_ );
    if ( !tkzs.hsamp_.isDefined() && info.is2D() )
	tkzstoload.hsamp_ = TrcKeySampling( geomid_ );

    SequentialReader rdr( *ioobj, tkzstoload.isDefined() ? &tkzstoload : 0 );
    rdr.setName( caption.getFullString() );
    rdr.setScaler( scaler );
    rdr.setDataChar( type );
    if ( !trunnr.execute(rdr) )
    {
	errmsg_ = rdr.uiMessage();
	return false;
    }

    RegularSeisDataPack* dp = rdr.getDataPack();
    if ( !dp ) return false;

    PLDM().add( mid_, geomid_, dp );

    return true;
}


bool PreLoader::load( const TypeSet<TrcKeyZSampling>& tkzss,
		      const TypeSet<Pos::GeomID>& geomids,
			 DataCharacteristics::UserType type,
			 Scaler* scaler ) const
{
    mPrepIOObj();
    const uiString caption = tr("Pre-loading %1").arg( ioobj->uiName() );

    MouseCursorChanger mcc( MouseCursor::Wait );
    TaskGroup taskgrp;
    taskgrp.showCumulativeCount( true );
    ObjectSet<SequentialReader> rdrs;
    TypeSet<Pos::GeomID> loadedgeomids;
    for ( int idx=0; idx<tkzss.size(); idx++ )
    {
	const Pos::GeomID& geomid = geomids[idx];
	TrcKeyZSampling tkzs( tkzss[idx] );
	if ( !tkzs.hsamp_.isDefined() )
	    tkzs.hsamp_ = TrcKeySampling( geomid );

	SequentialReader* rdr = new SequentialReader( *ioobj, &tkzs );
	rdr->setName( caption.getFullString() );
	rdr->setScaler( scaler );
	rdr->setDataChar( type );
	taskgrp.addTask( rdr );
	loadedgeomids.add( geomid );
	rdrs.add( rdr );
    }

    mcc.restore();
    TaskRunner::execute( &trunnr, taskgrp );

    for ( int idx=0; idx<rdrs.size(); idx++ )
    {
	SequentialReader& rdr = *rdrs[idx];
	const Pos::GeomID& loadedgeomid = loadedgeomids[idx];
	RegularSeisDataPack* dp = rdr.getDataPack();
	if ( !dp ) continue;
	PLDM().add( mid_, loadedgeomid, dp );
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
	 : StreamProvider::preLoad( fnms, trunnr, mid_.buf() );
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
	: StreamProvider::preLoad( fnms, trunnr, mid_.buf() );
}


void PreLoader::unLoad() const
{
    PLDM().remove( mid_, geomid_ );
}


void PreLoader::load( const IOPar& iniop, TaskRunner* tr )
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


void PreLoader::loadObj( const IOPar& iop, TaskRunner* tr )
{
    MultiID mid = MultiID::udf();
    iop.get( sKey::ID(), mid );
    if ( mid.isUdf() )
	return;

    Pos::GeomID geomid = -1;
    iop.get( sKey::GeomID(), geomid );

    SeisIOObjInfo info( mid );
    if ( !info.isOK() ) return;

    DataCharacteristics dc; info.getDataChar( dc );
    DataCharacteristics::UserType usertype( dc.userType() );
    DataCharacteristics::UserTypeDef().parse( iop, sKeyUserType(), usertype );

    BufferString scalerstr;
    iop.get( sKey::Scale(), scalerstr );
    Scaler* scaler = !scalerstr.isEmpty() ? Scaler::get(scalerstr.buf()) : 0;

    PLDM().remove( mid, geomid );

    PreLoader spl( mid, geomid, tr );
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
    SeisIOObjInfo oinf( mid_ );
    if ( !oinf.isOK() ) return;

    iop.set( sKey::ID(), mid_ );
    iop.set( sKey::GeomID(), geomid_ );
    mDynamicCastGet(const RegularSeisDataPack*,regsdp,PLDM().get(mid_,geomid_));
    if ( regsdp )
    {
	iop.set( sKeyUserType(), DataCharacteristics::toString(
		    DataCharacteristics(regsdp->getDataDesc()).userType()) );
	regsdp->sampling().fillPar( iop );
	const Scaler* scaler = regsdp->getScaler();
	if ( scaler )
	{
	    BufferString info( 256, false );
	    scaler->put( info.getCStr(), info.bufSize() );
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
	    StreamProvider::getPreLoadedFileNames( mid_.buf(), fnms );
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


// PreLoadDataEntry
PreLoadDataEntry::PreLoadDataEntry( const MultiID& mid, Pos::GeomID geomid,
				    int dpid )
    : mid_(mid), geomid_(geomid), dpid_(dpid), is2d_(geomid!=-1)
{
    name_ = IOM().nameOf( mid );
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    is2d_ = geom && geom->is2D();
    if ( is2d_ )
    {
	name_.add( " - " );
	name_.add( geom->getName() );
    }
}


bool PreLoadDataEntry::equals( const MultiID& mid, Pos::GeomID geomid ) const
{
    const bool samemid = mid_ == mid;
    return geomid==-1 ? samemid : samemid && geomid_==geomid;
}



// PreLoadDataManager
PreLoadDataManager::PreLoadDataManager()
    : dpmgr_(DPM(DataPackMgr::SeisID()))
    , changed(this)
{
}


PreLoadDataManager::~PreLoadDataManager()
{
    dpmgr_.releaseAll( true );
}


void PreLoadDataManager::add( const MultiID& mid, DataPack* dp )
{ add( mid, -1, dp ); }


void PreLoadDataManager::add( const MultiID& mid, Pos::GeomID geomid,
			      DataPack* dp )
{
    if ( !dp ) return;

    entries_ += new PreLoadDataEntry( mid, geomid, dp->id() );
    dpmgr_.addAndObtain( dp );
    changed.trigger();

}


void PreLoadDataManager::remove( const MultiID& mid, Pos::GeomID geomid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->equals(mid,geomid) )
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
	    dpmgr_.release( dpid );
	    changed.trigger();
	    return;
	}
    }
}


void PreLoadDataManager::removeAll()
{
    while ( entries_.size() )
    {
	dpmgr_.release( entries_[0]->dpid_ );
	entries_.removeSingle( 0 );
    }

    changed.trigger();
}


DataPack* PreLoadDataManager::get( const MultiID& mid, Pos::GeomID geomid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->equals(mid,geomid) )
	    return get( entries_[idx]->dpid_ );
    }

    return 0;
}


const DataPack* PreLoadDataManager::get( const MultiID& mid,
					 Pos::GeomID geomid ) const
{ return const_cast<PreLoadDataManager*>(this)->get( mid, geomid ); }


DataPack* PreLoadDataManager::get( int dpid )
{
    return dpmgr_.haveID(dpid) ? dpmgr_.observe( dpid ) : 0;
}


const DataPack* PreLoadDataManager::get( int dpid ) const
{ return const_cast<PreLoadDataManager*>(this)->get( dpid ); }


void PreLoadDataManager::getInfo( const MultiID& mid, Pos::GeomID geomid,
				  BufferString& info ) const
{
    const DataPack* dp = get( mid, geomid );
    if ( !dp ) return;

    IOPar par; dp->dumpInfo( par );
    par.removeWithKey( DataPack::sKeyCategory() );
    par.removeWithKey( "Pack.ID" );
    par.dumpPretty( info );
}


void PreLoadDataManager::getIDs( TypeSet<MultiID>& ids ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
	ids += entries_[idx]->mid_;
}


bool PreLoadDataManager::isPresent( const MultiID& mid,
				    Pos::GeomID geomid ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->equals(mid,geomid) )
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
