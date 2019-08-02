/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "velocitypicks.h"

#include "velocitycalc.h"

#include "binnedvalueset.h"
#include "emmanager.h"
#include "executor.h"
#include "iopar.h"
#include "dbman.h"
#include "ioobj.h"
#include "emhorizon3d.h"
#include "picksetmanager.h"
#include "picksettr.h"
#include "ptrman.h"
#include "randcolor.h"
#include "smoother1d.h"
#include "settings.h"
#include "survinfo.h"
#include "velocitypicksundo.h"
#include "zdomain.h"
#include "trigonometry.h"
#include "uistrings.h"

#define mDepthIndex	0
#define mVelIndex	1

mDefineEnumUtils( Vel::Picks, PickType, "Pick types" )
{ "RMO", "RMS", "Delta", "Epsilon", "Eta", 0 };

template<>
void EnumDefImpl<Vel::Picks::PickType>::init()
{
    uistrings_ += uiStrings::sRMO();
    uistrings_ += uiStrings::sRMS();
    uistrings_ += uiStrings::sDelta();
    uistrings_ += uiStrings::sEpsilon();
    uistrings_ += uiStrings::sEta();
}

namespace Vel
{
Pick::Pick( float depth, float vel, float offset, DBKey oid )
    : depth_( depth )
    , vel_( vel )
    , offset_( offset )
    , emobjid_( oid )
{}


bool Pick::operator==( const Pick& b ) const
{
    return b.depth_==depth_ &&
	   b.vel_==vel_ &&
	   b.offset_==offset_ &&
	   b.emobjid_==emobjid_;
}



PicksMgr& VPM()
{
    mDefineStaticLocalObject( PicksMgr, mgr, );
    return mgr;
}


const char* Picks::sKeyVelocityPicks()	{ return "Velocity Picks"; }
const char* Picks::sKeyRefOffset()	{ return "Reference offset"; }
const char* Picks::sKeyGatherID()	{ return "Gather"; }
const char* Picks::sKeyNrHorizons()	{ return "Nr Horizons"; }
const char* Picks::sKeyHorizonPrefix()	{ return "Horizon "; }
const char* Picks::sKeyPickType()	{ return "Pick Type"; }


const char* Picks::sKeyIsTime()	{ return "Z is time"; }

Picks::Picks()
    : change(this)
    , changelate(this)
    , picks_( 2, 1 )
    , snapper_( SI().zRange() )
    , changed_( false )
    , zit_( SI().zIsTime() )
    , picktype_( SI().zIsTime() ? RMS : RMO )
    , smoother_( 0 )
    , undo_( 0 )
    , refoffset_(0)
    , storageid_( DBKey::getInvalid() )
    , gatherid_( DBKey::getInvalid() )
    , color_( getRandomColor(false) )
{
    getDefaultColor( color_ );
    picks_.allowDuplicates( true );
    VPM().velpicks_ += this;
    horizons_.setNullAllowed( true );
}


Picks::Picks( bool zit )
    : change(this)
    , changelate(this)
    , picks_( 2, 1 )
    , snapper_( SI().zRange() )
    , changed_( false )
    , zit_( zit )
    , picktype_( zit ? RMS : RMO )
    , smoother_( 0 )
    , undo_( 0 )
    , storageid_( DBKey::getInvalid() )
    , gatherid_( DBKey::getInvalid() )
    , color_( getRandomColor(false) )
{
    getDefaultColor( color_ );

    picks_.allowDuplicates( true );
    VPM().velpicks_ += this;
    horizons_.setNullAllowed( true );
}


Picks::~Picks()
{
    VPM().velpicks_ -= this;
    delete smoother_;
    delete undo_;

    removeHorizons();
}


void Picks::removeHorizons()
{
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	if ( horizons_[idx] )
	    horizons_[idx]->objectChanged().remove(
				mCB(this,Picks,horizonChangeCB) );
    }

    deepUnRef( horizons_ );
}


const char* Picks::zDomain() const
{ return zit_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth(); }


Picks::PickType Picks::pickType() const
{ return picktype_; }


void Picks::setPickType( Picks::PickType t, bool resetcolor )
{
    picktype_ = t;
    if ( !resetcolor )
	return;

    if ( !getDefaultColor( color_ ) )
	color_ = getRandomColor(false);
}


bool Picks::setColor( const Color& col, bool dosave )
{
    if ( col!=color_ )
    {
	color_ = col;
	changed_ = true;

	change.trigger(BinID(-1,-1));
	changelate.trigger(BinID(-1,-1));
    }

    if ( !dosave )
	return true;

    BufferString key;
    getColorKey( key );

    Settings& settings = Settings::fetch();
    settings.set( key.buf(), col );
    return settings.write();
}


void Picks::getColorKey( BufferString& key ) const
{
    key = "dTect.VelPick.";
    key += toString(picktype_);
    key += ".";
    key += sKey::Color();
}


bool Picks::getDefaultColor( Color& col ) const
{
    BufferString key;
    getColorKey( key );
    return Settings::fetch().get( key.buf(), col );
}


Undo& Picks::undo()
{
    if ( !undo_ )
	undo_ = new Undo;

    return *undo_;
}


void Picks::removeAll( bool addtoundo, bool interactionend )
{
    int lastevent = -1;
    while ( picks_.size() )
    {
	const RowCol arrpos( 0, 0 );

	if ( addtoundo )
	{
	    lastevent = undo().addEvent(
		    new PickRemoveEvent(*this, arrpos ) );
	}

	picks_.remove( arrpos );
    }

    if ( lastevent!=-1 )
	undo().setUserInteractionEnd( lastevent, interactionend );

    changed_ = false;
    storageid_.setInvalid();
    change.trigger(BinID(-1,-1));
    changelate.trigger(BinID(-1,-1));
}


bool Picks::isEmpty() const
{ return picks_.isEmpty(); }


void Picks::setSnappingInterval(const StepInterval<float>& si)
{
    snapper_ = si;
    //Snap everything?
}


RowCol Picks::find( const BinID& pickbid,const Pick& pick) const
{
    const int depthsample = snapper_.nearestIndex(pick.depth_);

    RowCol arrpos;
    BinID curbid;
    if ( picks_.findFirst(pickbid,arrpos) )
    {
	do
	{
	    const Pick& storedpick = picks_.getRef( arrpos, 0 );
	    if ( pick.emobjid_.isInvalid() )
	    {
		const int sample = snapper_.nearestIndex(storedpick.depth_);
		if ( sample==depthsample )
		{
		    return arrpos;
		}
	    }
	    else if ( pick.emobjid_==storedpick.emobjid_ )
	    {
		return arrpos;
	    }
	} while ( picks_.next(arrpos,false) && picks_.getPos(arrpos,curbid) &&
		  curbid==pickbid );
    }

    return RowCol(-1,-1);
}


RowCol Picks::set( const BinID& pickbid, const Pick& velpick,
			   bool addtoundo, bool interactionend )
{
    RowCol arrpos = find( pickbid, velpick );
    if ( picks_.isValidPos(arrpos) )
    {
	Pick& oldpick = picks_.getRef( arrpos, 0 );

	//snap depth?

	if ( addtoundo )
	{
	    int eventid = undo().addEvent(
		    new PickSetEvent(*this, oldpick, velpick, pickbid));
	    undo().setUserInteractionEnd( eventid, interactionend );
	}

	oldpick.vel_ = velpick.vel_;
	oldpick.offset_ = velpick.offset_;
    }
    else
    {
	Pick pick = velpick;
	RefMan<EM::Horizon3D> hor = getHorizon( velpick.emobjid_ );
	if ( hor )
	    pick.depth_ = hor->getZ( pickbid );
	else if ( mIsUdf( pick.depth_ ) )
	    return RowCol(-1,-1);
	else
	{
	    pick.depth_ = snapper_.snap(velpick.depth_);
	}

	picks_.add( &pick, pickbid, &arrpos );

	if ( addtoundo )
	{
	    const int eventid = undo().addEvent(
		    new PickAddEvent(*this,arrpos));
	    undo().setUserInteractionEnd( eventid, interactionend );
	}
    }

    changed_ = true;
    change.trigger(pickbid);
    changelate.trigger(pickbid);
    return arrpos;
}


void Picks::set( const RowCol& arrpos,
    const Pick& velpick, bool addtoundo, bool interactionend )
{
    if ( picks_.isValidPos( arrpos ) )
    {
	BinID bid;
	picks_.getPos( arrpos, bid );

	Pick& pick = picks_.getRef( arrpos, 0 );

	if ( addtoundo )
	{
	    int eventid = undo().addEvent(
		    new PickSetEvent(*this, pick, velpick, bid) );
	    undo().setUserInteractionEnd( eventid, interactionend );
	}

	pick = velpick;
	change.trigger( bid );
	changelate.trigger( bid );
        changed_ = true;
	return;
    }
}


void Picks::remove( const RowCol& arrpos,
			     bool addtoundo, bool interactionend )
{
    if ( picks_.isValidPos( arrpos ) )
    {
	BinID pickbid;
	picks_.getPos( arrpos, pickbid );
	if ( addtoundo )
	{
	    int eventid = undo().addEvent(
		    new PickRemoveEvent(*this, arrpos ) );
	    undo().setUserInteractionEnd( eventid, interactionend );
	}

	changed_ = true;
	picks_.remove(arrpos);
	change.trigger(pickbid);
	changelate.trigger(pickbid);
    }
}


bool Picks::isChanged() const
{ return changed_; }


bool Picks::store( const IOObj* passedioobj )
{
    if ( storageid_!=passedioobj->key() )
    {
	storageid_=passedioobj->key();
	change.trigger(BinID(-1,-1));
	changelate.trigger(BinID(-1,-1));
    }

    DBKeySet emids;
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	if ( horizons_[idx] )
	    emids += horizons_[idx]->id();
    }

    RefMan< ::Pick::Set > ps = new ::Pick::Set;
    ps->setDispColor( color_ );
    RowCol arrpos( 0, 0 );
    if ( picks_.isValidPos( arrpos ) )
    {
	do
	{
	    BinID bid;
	    picks_.getPos(arrpos,bid);
	    const Pick& pick = picks_.getRef( arrpos, 0 );
	    const Coord3 pos( SI().transform(bid), pick.depth_ );
	    const Sphere dir( pick.vel_, pick.offset_, mUdf(float) );
	    ::Pick::Location pickloc( pos, dir );

	    const int idx = emids.indexOf(pick.emobjid_);
	    if ( idx!=-1 )
		pickloc.setText( ::toString(idx) );

	    ps->add( pickloc );
	} while ( picks_.next(arrpos,false) );
    }

    IOPar psiop( ps->pars() );
    fillPar( psiop );
    psiop.set( sKey::Version(), 2 );
    ps->setPars( psiop );

    PtrMan<IOObj> ioobj = passedioobj->clone();
    fillIOObjPar( ioobj->pars() );
    SilentTaskRunnerProvider trprov;
    uiString errmsg = ::Pick::SetMGR().store( *ps, storageid_, trprov,
					      &ioobj->pars() );
    if ( !errmsg.isEmpty() )
	{ errmsg_ =  errmsg; return false; }

    changed_ = false;
    change.trigger(BinID(-1,-1));
    changelate.trigger(BinID(-1,-1));
    return true;
}


void Picks::fillIOObjPar( IOPar& par ) const
{
    par.setEmpty();

    par.set( sKey::Type(), sKeyVelocityPicks() );
    par.set( sKeyGatherID(), gatherid_ );
    par.set( sKeyPickType(), toString( picktype_ ) );
    par.set( ZDomain::sKey(), zDomain() );
}


bool Picks::useIOObjPar( const IOPar& par )
{
    FixedString res = par.find( sKey::Type() );
    if ( res != sKeyVelocityPicks() )
	return false;

    par.get( sKeyGatherID(), gatherid_ );
    return true;
}


void Picks::fillPar( IOPar& par ) const
{
    par.set( sKeyNrHorizons(), horizons_.size() );
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	if ( !horizons_[idx] )
	    continue;

	BufferString key = sKeyHorizonPrefix();
	key += idx;

	par.set( key.buf(), horizons_[idx]->dbKey() );
    }

    par.setYN(sKeyIsTime(),zit_);
    par.set( sKeyPickType(), toString( picktype_ ) );
    if ( smoother_ ) smoother_->fillPar( par );
    par.set(sKeyRefOffset(),refoffset_);
}



bool Picks::usePar( const IOPar& par )
{
    if ( !par.getYN( sKeyIsTime(), zit_ ) )
	return false;

    const FixedString typestr = par.find( sKeyPickType() );
    if ( typestr )
    {
	if ( !PickTypeDef().parse(typestr,picktype_ ) )
	    return false;
    }
    else
    {
	picktype_ = zit_ ? RMS : RMO;
    }

    if ( !par.get( sKeyRefOffset(), refoffset_ ) )
	return false;

    if ( !smoother_ ) smoother_ = new Smoother1D<float>;
    if ( !smoother_->usePar( par ) )
    {
	delete smoother_;
	smoother_ = 0;
    }

    removeHorizons();

    int nrhorizons = 0;
    par.get( sKeyNrHorizons(), nrhorizons );
    for ( int idx=0; idx<nrhorizons; idx++ )
    {
	BufferString key = sKeyHorizonPrefix();
	key += idx;

	DBKey mid;
	if ( !par.get( key.buf(), mid ) )
	    continue;

	addHorizon( mid, true );
    }

    return true;
}


void Picks::horizonChangeCB( CallBacker* cb )
{
    if ( !cb ) return;

    mCBCapsuleUnpackWithCaller( EM::ObjectCallbackData, cbdata, caller, cb );
    if ( cbdata.changeType()==EM::Object::cPosIDChange() ||
	 cbdata.changeType()==EM::Object::cAttribChange() ||
	 cbdata.changeType()==EM::Object::cPrefColorChange() )
	return;

    mDynamicCastGet( const EM::Horizon3D*, hor, caller );
    if ( !hor || !horizons_.isPresent(hor) )
	return;

    RefMan<EM::ChangeAuxData> cbauxdata =
			cbdata.auxDataAs<EM::ChangeAuxData>();
    TypeSet<RowCol> rcs;
    if ( cbdata.changeType()==EM::Object::cBurstAlert() )
    {
	get( hor->id(), rcs );
    }
    else if ( cbauxdata )
    {
	BinID bid = cbauxdata->pid0.getBinID();
	RowCol arrpos;
	BinID curbid;
	if ( picks_.findFirst( bid, arrpos ) )
	{
	    do
	    {
		if ( picks_.getRef( arrpos, 0 ).emobjid_!=hor->id() )
		    continue;

		rcs += arrpos;

	    } while ( picks_.next(arrpos,false) &&
		      picks_.getPos(arrpos,curbid) && curbid==bid );
	}

    }

    for ( int idx=rcs.size()-1; idx>=0; idx-- )
    {
	BinID bid;
	picks_.getPos( rcs[idx], bid );
	const float depth = hor->getZ( bid );

	if ( mIsUdf(depth) )
	    continue;

	picks_.getRef( rcs[idx], 0 ).depth_ = depth;

	change.trigger( bid );
	changelate.trigger( bid );
    }
}


void Picks::addHorizon( const DBKey& dbky, bool addzeroonfail )
{
    SilentTaskRunnerProvider tprov;
    RefMan<EM::Object> emobj =
		EM::Hor3DMan().loadIfNotFullyLoaded( dbky, tprov );
    mDynamicCastGet( EM::Horizon3D*, hor3d, emobj.ptr() );
    if ( !hor3d )
    {
	if ( addzeroonfail )
	    horizons_ += 0;
	return;
    }

    addHorizon( hor3d );
}


void Picks::addHorizon( EM::Horizon3D* hor )
{
    if ( horizons_.isPresent(hor) )
	return;

    horizons_ += hor;
    if ( hor )
    {
	hor->ref();
	hor->objectChanged().notify( mCB(this,Picks,horizonChangeCB) );
    }
}


int Picks::nrHorizons() const { return horizons_.size(); }


DBKey Picks::getHorizonID( int idx ) const
{
    return horizons_[idx] ? horizons_[idx]->id() : DBKey::getInvalid();
}


void Picks::removeHorizon( const DBKey& id )
{
    for ( int idx=horizons_.size()-1; idx>=0; idx-- )
    {
	if ( horizons_[idx] && horizons_[idx]->id()==id )
	{
	    //TODO: Remove all picks on this horizon.
	    horizons_[idx]->objectChanged().remove(
		    mCB(this,Picks,horizonChangeCB) );
	    horizons_.removeSingle( idx )->unRef();
	    return;
	}
    }
}


EM::Horizon3D* Picks::getHorizon( const DBKey& id )
{
    for ( int idx=horizons_.size()-1; idx>=0; idx-- )
    {
	if ( horizons_[idx]->id()==id )
	    return horizons_[idx];
    }

    return 0;
}


const EM::Horizon3D* Picks::getHorizon( const DBKey& id ) const
{ return const_cast<Picks*>( this )->getHorizon( id ); }


char Picks::getHorizonStatus( const BinID& bid ) const
{
    bool defined = false;
    bool undefined = false;

    if ( !nrHorizons() )
	return 0;

    for ( int idx=nrHorizons()-1; idx>=0; idx-- )
    {
	ConstRefMan<EM::Horizon3D> hor = getHorizon( getHorizonID(idx) );
	if ( !hor ) continue;

	if ( hor->isDefined(EM::PosID::getFromRowCol(bid)) )
	    defined = true;
	else
	    undefined = true;
    }

    if ( !undefined )
	return 2;

    if ( !defined )
	return 0;

    return 1;
}


bool Picks::interpolateVelocity(const DBKey& emid, float searchradius,
					BinnedValueSet& res ) const
{
    ConstRefMan<EM::Horizon3D> horizon = getHorizon( emid );
    if ( !horizon )
	return false;

    if ( res.isEmpty() )
	return true;

    if ( res.nrVals()<2 )
	res.setNrVals( 2 );

    TypeSet<RowCol> picks;
    get( emid, picks );

    TypeSet<float> vels( picks.size(), mUdf(float) );
    TypeSet<BinID> bids( picks.size(), BinID(-1,-1) );
    for ( int idx=vels.size()-1; idx>=0; idx-- )
    {
	const Pick& pick = picks_.getRef( picks[idx], 0 );
	float vel = mUdf(float);

	if ( picktype_==RMO )
	{
	    if ( !mIsUdf( pick.offset_ ) )
		vel = normalizeRMO( pick.depth_, pick.vel_, pick.offset_ );
	}
	else
	    vel = pick.vel_;

	if ( mIsUdf(vel) )
	{
	    vels.removeSingle(idx);
	    picks.removeSingle(idx);
	    bids.removeSingle(idx);
	    continue;
	}

	picks_.getPos( picks[idx], bids[idx] );
	vels[idx] = vel;
    }

    const bool usesearchradius = !mIsUdf(searchradius);
    const double searchradius2 = searchradius*searchradius;

    BinnedValueSet::SPos pos( 0, 0 );
    do
    {
	const BinID bid = res.getBinID(pos);
	const Coord coord = SI().transform(bid);

	float* vals = res.getVals( pos );
	vals[0] = horizon->getZ( bid );

	float weightsum = 0;
	float weightvalsum = 0;
	for ( int idx=vels.size()-1; idx>=0; idx-- )
	{
	    const Coord pickcoord = SI().transform( bids[idx] );
	    const float sqdist = (float) pickcoord.sqDistTo( coord );
	    if ( usesearchradius && sqdist>searchradius2 )
		continue;

	    if ( !sqdist )
	    {
		weightsum = 1;
		weightvalsum = vels[idx];
		break;
	    }

	    const float weight = 1.0f/sqdist;

	    weightvalsum += weight * vels[idx];
	    weightsum += weight;
	}

	vals[1] = weightsum>0 ? weightvalsum/weightsum : mUdf(float);
    } while ( res.next( pos, false ) );

    return true;
}


bool Picks::load( const IOObj* ioobj )
{
    picks_.setEmpty();

    if ( !useIOObjPar( ioobj->pars() ) )
    {
	errmsg_ = ::toUiString("Internal: No valid storage selected");
	return false;
    }

    storageid_ = ioobj->key();

    uiRetVal uirv;
    ConstRefMan< ::Pick::Set > ps = ::Pick::SetMGR().fetch( ioobj->key(), uirv);
    if ( uirv.isError() )
	{ errmsg_ = ::toUiString(uirv.getText()); return false; }

    const IOPar psiop( ps->pars() );
    if ( !usePar(psiop) )
    {
	if ( !usePar( ioobj->pars() ) ) //Old format
	    return false;
    }

    int version = 1;
    psiop.get( sKey::Version(), version );

    ::Pick::SetIter psiter( *ps, true );
    while ( psiter.next() )
    {
	const ::Pick::Location& ploc = psiter.get();
	if ( !ploc.hasPos() || !ploc.hasDir() )
	    continue;

	const BinID bid = SI().transform( ploc.pos().getXY() );
	const float z = mCast(float,ploc.pos().z_);
	const Sphere& dir = ploc.dir();
	Pick pick = version==1	? Pick( z, dir.radius_, refoffset_ )
				: Pick( z, dir.radius_, dir.theta_-1 );

	if ( ploc.hasText() )
	{
	    const int horidx = Conv::to<int>( ploc.text() );
	    if ( horizons_.validIdx(horidx) )
		pick.emobjid_ = horizons_[horidx]->id();
	}

	picks_.add( &pick, bid );
    }

    color_ = ps->dispColor();

    changed_ = false;
    change.trigger( BinID(-1,-1) );
    changelate.trigger(BinID(-1,-1));

    return true;
}


const DBKey& Picks::storageID() const
{ return storageid_; }


void Picks::setSmoother(Smoother1D<float>* ns )
{
    if ( !ns && !smoother_ )
	return;

    if ( ns && smoother_ && *smoother_==*ns )
	return;

    delete smoother_;
    smoother_ = ns;
    changed_ = true;
    change.trigger(BinID(-1,-1));
    changelate.trigger(BinID(-1,-1));
}


int Picks::get( const BinID& pickbid, TypeSet<float>* depths,
			 TypeSet<float>* velocities, TypeSet<RowCol>* positions,
			 DBKeySet* emobjres,
			 bool interpolhors ) const
{
    if ( depths ) depths->erase();
    if ( velocities ) velocities->erase();
    if ( positions ) positions->erase();
    if ( emobjres ) emobjres->erase();

    DBKeySet internalemobjects;
    DBKeySet& emids = emobjres ? *emobjres : internalemobjects;

    RowCol arrpos;
    BinID curbid;
    if ( picks_.findFirst( pickbid, arrpos ) )
    {
	do
	{
	    const Pick& pick = picks_.getRef( arrpos, 0 );
	    if ( depths )	(*depths) += pick.depth_;
	    if ( velocities )	(*velocities) += pick.vel_;
	    if ( positions )	(*positions) += arrpos;
	    emids += pick.emobjid_;
	} while ( picks_.next(arrpos,false) && picks_.getPos(arrpos,curbid) &&
		  curbid==pickbid );
    }

    if ( interpolhors )
    {
	BinnedValueSet bidset( 2, false );
	const float vals[] = { mUdf(float), mUdf(float) };
	BinnedValueSet::SPos pos = bidset.add( pickbid );
	for ( int idx=nrHorizons()-1; idx>=0; idx-- )
	{
	    ConstRefMan<EM::Horizon3D> hor = horizons_[idx];
	    if ( !hor ) continue;

	    //We don't want the same pick twice
	    if ( emids.isPresent(hor->id()) )
		continue;

	    bidset.set( pos, vals );

	    if ( !interpolateVelocity( hor->id(), mUdf(float), bidset ) )
		continue;

	    const float* pick = bidset.getVals( pos );
	    if ( mIsUdf(pick[0]) || mIsUdf(pick[1] ) )
		continue;

	    if ( depths )		(*depths) += pick[0];
	    if ( velocities )	(*velocities) += pick[1];
	    if ( positions )	(*positions) += RowCol(-1,-1);
	    emids += hor->id();
	}
    }

    return emids.size();
}


void Picks::get( const BinID& pickbid, TypeSet<Pick>& picks,
			 bool interpolhors, bool normalizeoffset ) const
{
    picks.erase();

    DBKeySet emids;

    RowCol arrpos;
    BinID curbid;
    if ( picks_.findFirst( pickbid, arrpos ) )
    {
	do
	{
	    const Pick& pick = picks_.getRef( arrpos, 0 );
	    picks += pick;
	} while ( picks_.next(arrpos,false) && picks_.getPos(arrpos,curbid) &&
		  curbid==pickbid );
    }

    if ( interpolhors )
    {
	BinnedValueSet bidset( 2, false );
	const float vals[] = { mUdf(float), mUdf(float) };
	BinnedValueSet::SPos pos = bidset.add( pickbid );
	for ( int idx=nrHorizons()-1; idx>=0; idx-- )
	{
	    ConstRefMan<EM::Horizon3D> hor = horizons_[idx];
	    if ( !hor ) continue;

	    //We don't want the same pick twice
	    if ( emids.isPresent(hor->id()) )
		continue;

	    bidset.set( pos, vals );

	    if ( !interpolateVelocity( hor->id(), mUdf(float), bidset ) )
		continue;

	    const float* pick = bidset.getVals( pos );
	    if ( mIsUdf(pick[0]) || mIsUdf(pick[1] ) )
		continue;

	    Pick vp( pick[0], pick[1], refOffset(), hor->id() );
	    picks += vp;
	    emids += hor->id();
	}
    }

    if ( normalizeoffset )
    {
	for ( int idx=picks.size()-1; idx>=0; idx-- )
	{
	    if ( !mIsUdf( picks[idx].offset_ ) ||
		 !mIsEqual(picks[idx].offset_, refoffset_, 1e-3 ) )
	    {
		picks[idx].vel_ =
		    normalizeRMO( picks[idx].depth_, picks[idx].vel_,
				  picks[idx].offset_ );
		picks[idx].offset_ = refoffset_;
	    }
	}
    }
}


float Picks::normalizeRMO(float depth, float rmo, float offset) const
{
    float curve = mUdf(float);
    computeResidualMoveouts( depth, rmo, offset, 1, false, &refoffset_, &curve);
    return curve;
}


bool Picks::get( const RowCol& arrpos, BinID& bid, Pick& pick)
{
    if ( picks_.isValidPos(arrpos) )
    {
	pick = picks_.getRef( arrpos, 0 );
	picks_.getPos( arrpos, bid );
	return true;
    }

    return false;
}


void Picks::get(const DBKey& emid, TypeSet<RowCol>& res ) const
{
    RowCol arrpos( -1, -1 );
    while ( picks_.next(arrpos,false) )
    {
	if ( picks_.getRef( arrpos, 0 ).emobjid_==emid )
	    res += arrpos;
    }
}



const uiString Picks::errMsg() const
{ return errmsg_; }


void Picks::setAll( float vel, bool addtoundo )
{
    RowCol arrpos( -1, -1 );
    int eventid = -1;
    while ( picks_.next(arrpos,false) )
    {
	Pick& pick = picks_.getRef( arrpos, 0 );
	if ( addtoundo )
	{
	    const float olddepth = pick.depth_;
	    const float oldvel = pick.vel_;
	    BinID bid;
	    picks_.getPos( arrpos, bid );

	    eventid = undo().addEvent( new PickSetEvent(*this,
				       Pick(olddepth, oldvel,-1),
				       Pick(olddepth, vel,-1), bid) );
	}

	pick.vel_ = vel;
	changed_ = true;
    }

    if ( eventid!=-1 )
	undo().setUserInteractionEnd( eventid );

    change.trigger(BinID(-1,-1));
    changelate.trigger(BinID(-1,-1));
}


const DBKey& Picks::gatherID() const
{ return gatherid_; }


void Picks::setGatherID( const DBKey& m )
{ gatherid_ = m; }


const IOObjContext& Picks::getStorageContext()
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, ret, = 0 );
    if ( !ret )
    {
	IOObjContext* newret =
		new IOObjContext(PickSetTranslatorGroup::ioContext());
	newret->setName( "Velocity picks" );
	newret->toselect_.require_.set( sKey::Type(), sKeyVelocityPicks() );

	ret.setIfNull(newret,true);
    }

    return *ret;
}


void Picks::setContextPickType( IOObjContext& ctxt, PickType type )
{
    ctxt.toselect_.require_.set( sKeyPickType(), toString( type ) );
}


void Picks::setReferenceOffset( float n )
{
    if ( picktype_!=RMO )
	return;

    if ( mIsEqual(n,refoffset_,1e-3) )
	return;

    refoffset_ = n;
    if ( undo_ ) undo_->removeAll();
}


PicksMgr::PicksMgr()
{ DBM().surveyToBeChanged.notify( mCB(this,PicksMgr,surveyChange) ); }


PicksMgr::~PicksMgr()
{ DBM().surveyToBeChanged.remove( mCB(this,PicksMgr,surveyChange) ); }


Picks* PicksMgr::get( const DBKey& mid, bool sgmid, bool create,
		      bool forceread )
{
    Picks* res = 0;
    for ( int idx=0; idx<velpicks_.size(); idx++ )
    {
	if ( mid.isValid() && ((!sgmid && velpicks_[idx]->storageID()==mid) ||
	     (sgmid && velpicks_[idx]->gatherID()==mid)) )
	{
	    res = velpicks_[idx];
	    res->ref();
	    break;
	}
    }

    if ( res && !forceread )
    {
	res->unRefNoDelete();
	return res;
    }

    if ( !res && !create )
	return 0;

    if ( !res )
    {
	res = new Picks();
	res->ref();
    }

    PtrMan<IOObj> ioobj = mid.getIOObj();
    if ( forceread && !ioobj )
    {
	res->unRef();
	return 0;
    }

    if ( ioobj && !res->load( ioobj ) )
    {
	errmsg_ = res->errMsg();
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void PicksMgr::surveyChange( CallBacker* )
{
    velpicks_.erase();
}



const uiString PicksMgr::errMsg() const
{
    return errmsg_;
}

} // namespace Vel
