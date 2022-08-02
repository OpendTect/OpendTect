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
	errmsg_ = uiStrings::phrCannotFindDBEntry( mid_ );
    return ret;
}


Interval<int> PreLoader::inlRange() const
{
    //TODO: Implement for PS3D
    return Interval<int>::udf();
}


void PreLoader::getLineNames( BufferStringSet& lks ) const
{
    //TODO:: Implement for PS2D
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

    SequentialReader rdr( *ioobj, tkzstoload.isDefined() ? &tkzstoload
							 : nullptr );
    rdr.setName( caption.getFullString() );
    rdr.setScaler( scaler );
    rdr.setDataChar( type );
    if ( !trunnr.execute(rdr) )
    {
	errmsg_ = rdr.uiMessage();
	return false;
    }

    ConstRefMan<RegularSeisDataPack> dp = rdr.getDataPack();
    if ( !dp )
	return false;

    PLDM().add( *dp.ptr(), mid_, geomid_ );

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

	auto* rdr = new SequentialReader( *ioobj, &tkzs );
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
	ConstRefMan<RegularSeisDataPack> dp = rdr.getDataPack();
	if ( !dp )
	    continue;

	PLDM().add( *dp.ptr(), mid_, loadedgeomid );
    }

    return true;
}


bool PreLoader::loadPS3D( const Interval<int>* inlrg ) const
{
    //TODO: Implement
    return false;
}


bool PreLoader::loadPS2D( const char* lnm ) const
{
    //TODO: Implement (use GeomID instead)
    return false;
}


bool PreLoader::loadPS2D( const BufferStringSet& lnms ) const
{
    //TODO: Implement (Use GeomID instead)
    return false;
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

    Pos::GeomID geomid;
    iop.get( sKey::GeomID(), geomid );

    SeisIOObjInfo info( mid );
    if ( !info.isOK() )
	return;

    DataCharacteristics dc; info.getDataChar( dc );
    DataCharacteristics::UserType usertype( dc.userType() );
    DataCharacteristics::UserTypeDef().parse( iop, sKeyUserType(), usertype );

    BufferString scalerstr;
    iop.get( sKey::Scale(), scalerstr );
    PtrMan<Scaler> scaler;
    if ( !scalerstr.isEmpty() )
	scaler = Scaler::get( scalerstr.buf() );

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
	    Interval<int> nrrg; Interval<int>* toload = nullptr;
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
    const SeisIOObjInfo oinf( mid_ );
    if ( !oinf.isOK() )
	return;

    iop.set( sKey::ID(), mid_ );
    iop.set( sKey::GeomID(), geomid_ );
    ConstRefMan<RegularSeisDataPack> regsdp =
				PLDM().get<RegularSeisDataPack>( mid_,geomid_ );
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
	    BufferStringSet lnms;
	    getLineNames( lnms );
	    iop.set( sKeyLines(), lnms );
	} break;
    }
}


// PreLoadDataEntry
PreLoadDataEntry::PreLoadDataEntry( const DataPack& dp, const MultiID& mid,
				    Pos::GeomID geomid )
    : dp_(&dp)
    , mid_(mid)
    , geomid_(geomid)
    , is2d_(Survey::is2DGeom(geomid))
    , dpmgr_(DPM(DataPackMgr::SeisID()))
{
    if ( dpmgr_.isPresent(dp.id()) )
	{ pErrMsg("DP should not already have been added"); }

    dpmgr_.add( dp );

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
    return geomid.isUdf() ? samemid : samemid && geomid_==geomid;
}


DataPack::ID PreLoadDataEntry::dpID() const
{
    return dp_ ? dp_->id() : DataPack::cNoID();
}


RefMan<DataPack> PreLoadDataEntry::getDP()
{
    return dp_.getNonConstPtr();
}


ConstRefMan<DataPack> PreLoadDataEntry::getDP() const
{
    return mSelf().getDP();
}



// PreLoadDataManager
PreLoadDataManager::PreLoadDataManager()
    : changed(this)
{
    mAttachCB( IOM().surveyToBeChanged, PreLoadDataManager::surveyChangeCB );
}


PreLoadDataManager::~PreLoadDataManager()
{
    detachAllNotifiers();
}


void PreLoadDataManager::add( const DataPack& dp, const MultiID& mid )
{
    add( dp, mid, Pos::GeomID::udf() );
}


void PreLoadDataManager::add( const DataPack& dp, const MultiID& mid,
			      Pos::GeomID geomid )
{
    entries_ += new PreLoadDataEntry( dp, mid, geomid );
    changed.trigger();
}


void PreLoadDataManager::remove( const MultiID& mid, Pos::GeomID geomid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	const PreLoadDataEntry& entry = *entries_.get( idx );
	if ( !entry.equals(mid,geomid) )
	    continue;

	entries_.removeSingle( idx );
	changed.trigger();
	return;
    }
}


void PreLoadDataManager::remove( const DataPack::ID& dpid )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	const PreLoadDataEntry& entry = *entries_.get( idx );
	if ( entry.dpID() != dpid )
	    continue;

	entries_.removeSingle( idx );
	changed.trigger();
	return;
    }
}


void PreLoadDataManager::surveyChangeCB( CallBacker* )
{
    entries_.setEmpty();
}


ConstRefMan<DataPack> PreLoadDataManager::getDP( DataPack::ID dpid ) const
{
    for ( const auto* entry : entries_ )
	if ( entry->dpID() == dpid )
	    return entry->getDP();

    return nullptr;
}


ConstRefMan<DataPack> PreLoadDataManager::getDP( const MultiID& mid,
					    Pos::GeomID geomid ) const
{
    for ( const auto* entry : entries_ )
	if ( entry->equals(mid,geomid) )
	    return entry->getDP();

    return nullptr;
}


void PreLoadDataManager::getInfo( const MultiID& mid, Pos::GeomID geomid,
				  BufferString& info ) const
{
    ConstRefMan<DataPack> dp = getDP( mid, geomid );
    if ( !dp )
	return;

    IOPar par; dp->dumpInfo( par );
    par.removeWithKey( DataPack::sKeyCategory() );
    par.removeWithKey( "Pack.ID" );
    par.dumpPretty( info );
}


void PreLoadDataManager::getIDs( TypeSet<MultiID>& ids ) const
{
    for ( const auto* entry : entries_ )
	ids += entry->mid_;
}


bool PreLoadDataManager::isPresent( const MultiID& mid,
				    Pos::GeomID geomid ) const
{
    for ( const auto* entry : entries_ )
    {
	if ( entry->equals(mid,geomid) )
	    return true;
    }

    return false;
}


const ObjectSet<PreLoadDataEntry>& PreLoadDataManager::getEntries() const
{
    return entries_;
}


PreLoadDataManager& PLDM()
{
    mDefineStaticLocalObject(PtrMan<PreLoadDataManager>,pldm,
			     = new PreLoadDataManager() );
    return *pldm;
}

} // namespace Seis
