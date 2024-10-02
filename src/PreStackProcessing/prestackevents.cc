/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackevents.h"

#include "binidvalset.h"
#include "trckeyzsampling.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "ioman.h"
#include "rowcol.h"
#include "prestackeventtransl.h"
#include "prestackeventio.h"
#include "separstr.h"
#include "survinfo.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "thread.h"
#include "randcolor.h"
#include "binidvalue.h"

namespace PreStack
{

mDefineEnumUtils( EventManager::DipSource,Type,"Dip source" )
{ "None", "Horizon", "SteeringCube", nullptr };


Event::Event( int sz, bool quality )
{
    setSize( sz, quality );
}


Event::Event( const Event& oth )
{
    *this = oth;
}


Event::~Event()
{
    delete [] pick_;
    delete [] pickquality_;
    delete [] offsetazimuth_;
}


Event& Event::operator=( const Event& b )
{
    if ( &b == this )
	return *this;

    setSize( b.sz_, b.pickquality_ );

    OD::memCopy( pick_, b.pick_, sizeof(float)*sz_ );
    OD::memCopy( offsetazimuth_, b.offsetazimuth_, sizeof(OffsetAzimuth)*sz_ );
    if ( pickquality_ )
	OD::memCopy( pickquality_, b.pickquality_, sizeof(unsigned char)*sz_ );

    horid_ = b.horid_;
    quality_ = b.quality_;
    eventtype_ = b.eventtype_;

    return *this;
}


void Event::setSize( int sz, bool doquality )
{
    delete [] pick_;
    pick_ = new float[sz];
    for ( int idx=sz-1; idx>=0; idx-- )
	pick_[idx] = mUdf(float);

    delete [] offsetazimuth_;
    offsetazimuth_ = new OffsetAzimuth[sz];

    delete [] pickquality_;
    pickquality_ = 0;

    if ( doquality )
    {
	pickquality_ = new unsigned char[sz];

	for ( int idx=sz-1; idx>=0; idx-- )
	    pickquality_[idx] = 255;
    }

    sz_ = sz;
}


void Event::removePick( int idx )
{
    if ( sz_<=0 )
	{ pErrMsg( "Trying to remove non-existing pick" ); return; }

    sz_--;
    float* pick = sz_ ? new float[sz_] : 0;
    OffsetAzimuth* offsetazimuth = sz_ ? new OffsetAzimuth[sz_] : 0;
    unsigned char* pickquality = pickquality_ && sz_
	? new unsigned char[sz_] : 0;

    for ( int idy=0; idy<sz_; idy++ )
    {
	const int source = idy>=idx ? idy+1 : idy;
	pick[idy] = pick_[source];
	if ( pickquality ) pickquality[idy] = pickquality_[source];
	offsetazimuth[idy] = offsetazimuth_[source];
    }

    delete [] pick_; pick_ = pick;
    delete [] pickquality_; pickquality_ = pickquality;
    delete [] offsetazimuth_; offsetazimuth_ = offsetazimuth;
}


void Event::insertPick( int idx )
{
    float* pick = new float[sz_+1];
    OffsetAzimuth* offsetazimuth = new OffsetAzimuth[sz_+1];
    unsigned char* pickquality = pickquality_ ? new unsigned char[sz_+1] : 0;

    for ( int idy=0; idy<sz_; idy++ )
    {
	const int target = idy>=idx ? idy+1 : idy;
	pick[target] = pick_[idy];
	if ( pickquality ) pickquality[target] = pickquality_[idy];
	offsetazimuth[target] = offsetazimuth_[idy];
    }

    sz_++;

    delete [] pick_; pick_ = pick;
    delete [] pickquality_; pickquality_ = pickquality;
    delete [] offsetazimuth_; offsetazimuth_ = offsetazimuth;
}


void Event::addPick()
{
    insertPick( sz_ );
}


EventSet::EventSet()
{
}


int Event::indexOf( const OffsetAzimuth& oa ) const
{
    for ( int idx=0; idx<sz_; idx++ )
    {
	if ( offsetazimuth_[idx]==oa )
	    return idx;
    }

    return -1;
}


EventSet::EventSet( const EventSet& b )
{
    *this = b;
}


EventSet::~EventSet()
{
    deepErase( events_ );
}


EventSet& EventSet::operator=( const EventSet& b )
{
    if ( &b == this )
	return *this;

    deepCopy( events_, b.events_ );
    ischanged_ = b.ischanged_;
    return *this;
}


int EventSet::indexOf( int horid ) const
{
    for ( int idx=events_.size()-1; idx>=0; idx-- )
    {
	if ( events_[idx]->horid_==horid )
	    return idx;
    }

    return -1;
}


// EventManager::DipSource

EventManager::DipSource::DipSource()
{
}


EventManager::DipSource::~DipSource()
{
}


bool EventManager::DipSource::operator==(const EventManager::DipSource& b) const
{
    if ( &b == this )
	return true;

    if ( b.type_!=type_ )
	return false;

    if ( type_==SteeringVolume )
	return b.mid_==mid_;

    return true;
}


void EventManager::DipSource::fill( BufferString& buf ) const
{
    FileMultiString fms;
    fms += TypeDef().convert( (int) type_ );
    fms += mid_;
    buf = fms;
}


bool EventManager::DipSource::use( const char* str )
{
    const FileMultiString fms( str );
    const char* type = fms[0];
    if ( !type || !*type )
	return false;

    Type typeenum;
    if ( !parseEnumType( type, typeenum ) )
	return false;

    if ( typeenum==SteeringVolume )
    {
	if ( fms[1].isEmpty() )
	    return false;

	mid_.fromString( fms[1].buf() );
    }

    type_ = typeenum;
    return true;
}



// EventManager

EventManager::EventManager()
    : events_(2,1)
    , changebid_(-1,-1)
    , forceReload( this )
    , change(this)
    , resetChangeStatus(this)
    , reloadbids_(new BinIDValueSet(0,false))
    , notificationqueue_(new BinIDValueSet(0,false))
    , color_(OD::getRandomColor())
{
    events_.allowDuplicates( true );
    emhorizons_.setNullAllowed();
}


EventManager::~EventManager()
{
    cleanUp( false );
    if ( !events_.isEmpty() )
	{ pErrMsg("Leaving unreffed picks. Memory leak"); }

    delete reloadbids_;
    delete notificationqueue_;
    delete primarydipreader_;
    delete secondarydipreader_;

    deepUnRef( emhorizons_ );
}


int EventManager::addHorizon( int id )
{
    if ( id!=-1 && horids_.isPresent( id ) )
	{ pErrMsg("Horizon ID interference"); id = -1; }

    const int res = id==-1 ? nextHorizonID( true ) : id;
    horids_ += res;
    horrefs_ += MultiID();
    emhorizons_ += nullptr;

    auxdatachanged_ = true;
    reportChange();

    return  res;
}


bool EventManager::removeHorizon( int id )
{
    const int idx = horids_.indexOf( id );
    if ( idx<0 ) return false;

    horids_.removeSingle( idx );
    horrefs_.removeSingle( idx );
    if ( emhorizons_[idx] ) emhorizons_[idx]->unRef();
    emhorizons_.removeSingle( idx );

    RowCol arraypos( -1, -1 );
    while ( events_.next( arraypos, true ) )
    {
	RefMan<EventSet> ge = events_.getRef( arraypos, 0 );

	for ( int idy=0; idy<ge->events_.size(); idy++ )
	{
	    bool ischanged = false;
	    if ( ge->events_[idy]->horid_ == id )
	    {
		delete ge->events_.removeSingle(idy);
		ischanged = ge->ischanged_ = true;
		idy--;
	    }

	    if ( ischanged )
	    {
		BinID bid;
		events_.getPos( arraypos, bid );
		reportChange( bid );
	    }
	}
    }

    reportChange();
    return true;
}


int EventManager::nextHorizonID( bool inc )
{
    int res = nexthorid_;
    if ( inc ) nexthorid_++;
    return res;
}


void EventManager::setNextHorizonID( int ni )
{
    for ( int idx=horids_.size()-1; idx>=0; idx-- )
    {
	if ( ni<=horids_[idx] )
	    { pErrMsg("NextHorID interference"); break; }
    }

    nexthorid_ = ni;
}


void EventManager::setColor( const OD::Color& col )
{
    if ( color_==col )
	return;

    color_ = col;
    auxdatachanged_ = true;
    reportChange();
}


const MultiID& EventManager::horizonEMReference( int id ) const
{ return horrefs_[horids_.indexOf(id)]; }


void EventManager::setHorizonEMReference( int id, const MultiID& mid )
{
    if ( horrefs_[horids_.indexOf(id)]==mid )
	return;

    horrefs_[horids_.indexOf(id)] = mid;
    auxdatachanged_ = true;
    reportChange();
}


void EventManager::setDipSource(const EventManager::DipSource& ds,bool primary)
{
    (primary ? primarydipsource_ : secondarydipsource_) = ds;
}


const EventManager::DipSource&
EventManager::getDipSource( bool primary ) const
{
    return primary ? primarydipsource_ : secondarydipsource_;
}


Executor* EventManager::setStorageID( const MultiID& mid, bool reload )
{
    reloadbids_->setEmpty();
    storageid_ = mid;
    if ( !reload )
	return nullptr;

    horids_.erase();
    horrefs_.erase();
    deepUnRef( emhorizons_ );
    nexthorid_ = 0;
    auxdatachanged_ = false;

    forceReload.trigger();
    cleanUp( false );

    Executor* loader = load( *reloadbids_, true );
    reportChange(); //since blocked by loader, it will fire when
				  //loading finished
    return loader;
}


const MultiID& EventManager::getStorageID() const
{ return storageid_; }


bool EventManager::getHorRanges( TrcKeySampling& hrg ) const
{
    bool first = true;
    RowCol arraypos( -1, -1 );
    while ( events_.next( arraypos, false ) )
    {
	BinID bid;
	events_.getPos( arraypos, bid );
	if ( first )
	{
	    first = false;
	    hrg.start_.inl() = hrg.stop_.inl() = bid.inl();
	    hrg.start_.crl() = hrg.stop_.crl() = bid.crl();
	}
	else
	{
	    hrg.include( bid );
	}
    }

    IOObj* ioobj = IOM().get( storageid_ );
    if ( !ioobj )
	return !first;


    PtrMan<EventReader> reader = new EventReader(ioobj,0,false);
    if ( !reader->prepareWork() )
	return false;

    Interval<int> inlrg, crlrg;

    if ( reader->getBoundingBox( inlrg, crlrg ) )
    {
	if ( first )
	{
	    first = false;
	    hrg.start_.inl() = hrg.stop_.inl() = inlrg.start_;
	    hrg.start_.crl() = hrg.stop_.crl() = crlrg.start_;
	}

	hrg.include( BinID(inlrg.stop_,crlrg.stop_) );
    }

    return !first;
}


bool EventManager::getLocations( BinIDValueSet& bvs ) const
{
    RowCol arraypos( -1, -1 );
    while ( events_.next( arraypos, false ) )
    {
	BinID bid;
	events_.getPos( arraypos, bid );
	bvs.add( bid );
    }

    IOObj* ioobj = IOM().get( storageid_ );
    if ( !ioobj )
	return false;

    PtrMan<EventReader> reader = new EventReader(ioobj,0,false);
    if ( !reader )
	return false;

    return reader->prepareWork() && reader->getPositions( bvs );
}


Executor* EventManager::commitChanges()
{
    IOObj* ioobj = IOM().get( storageid_ );
    if ( !ioobj )
	{ pErrMsg("No ioobj"); return 0; }

    return PSEventTranslator::writer( *this, ioobj );
}


Executor* EventManager::load( const BinIDValueSet& bidset, bool trigger )
{
    IOObj* ioobj = IOM().get( storageid_ );
    if ( !ioobj ) return 0;
    return PSEventTranslator::reader( *this, &bidset, 0, ioobj, trigger );
}


bool EventManager::isChanged() const
{
    if ( auxdatachanged_ ) return true;

    RowCol pos( -1, -1 );
    while ( events_.next( pos, false ) )
    {
	EventSet* ge = events_.getRef( pos, 0 );
	if ( ge->ischanged_ )
	    return true;
    }

    return false;
}


void EventManager::resetChangedFlag( bool horflagonly )
{
    bool haschange = auxdatachanged_;
    auxdatachanged_ = false;
    if ( !horflagonly )
    {
	RowCol pos( -1, -1 );
	while ( events_.next( pos, false ) )
	{
	    EventSet* ge = events_.getRef( pos, 0 );
	    if ( ge->ischanged_ )
		haschange = true;

	    ge->ischanged_ = false;
	}
    }

    if ( haschange )
	resetChangeStatus.trigger();
}


EventSet* EventManager::getEvents( const BinID& bid, bool doload, bool create )
{
    if ( mIsUdf(bid.inl()) || mIsUdf(bid.crl()) )
	return 0;

    int arrpos[2];

    Threads::Locker lckr( eventlock_ );
    if ( !events_.findFirst( bid, arrpos ) )
    {
	if ( doload )
	{
	    BinIDValueSet bidvalset(0,false);
	    bidvalset.add( bid );
	    PtrMan<Executor> loader = load( bidvalset, false );
	    if ( loader )
	    {
		lckr.unlockNow();

		if ( loader->execute() )
		    return getEvents( bid, false, create );

		lckr.reLock();
	    }

	}

	if ( !create ) return nullptr;
	auto* ge = new EventSet;
	ge->ref();
	events_.add( &ge, bid );
	return ge;
    }

    return events_.getRef( arrpos, 0 );
}


const EventSet* EventManager::getEvents( const BinID& bid,
					 bool doload, bool create ) const
{
    return const_cast<EventManager*>(this)->getEvents(bid,doload,create);
}


void EventManager::cleanUp( bool keepchanged )
{
    RowCol pos( -1, -1 );
    RowCol prevkeptpos( -1, -1 );
    Threads::Locker lckr( eventlock_ );
    while ( events_.next( pos, false ) )
    {
	EventSet* ge = events_.getRef( pos, 0 );
	if ( (keepchanged && ge->ischanged_) || ge->nrRefs()>1 )
	    prevkeptpos = pos;
	else
	{
	    ge->unRef();
	    events_.remove( pos );
	    pos = prevkeptpos;
	}
    }
}


void EventManager::addReloadPositions( const BinIDValueSet& bvs )
{ reloadbids_->append( bvs ); }


void EventManager::addReloadPosition( const BinID& bid )
{ reloadbids_->add( bid ); }


void EventManager::blockChange( bool yn, bool sendnow )
{
    if ( yn )
	change.disable();
    else
    {
	change.enable();
	if ( sendnow )
	{
	    BinIDValueSet::SPos pos;
	    while ( notificationqueue_->next(pos) )
		reportChange( notificationqueue_->getBinID( pos ) );
	}

	notificationqueue_->setEmpty();
    }
}


void EventManager::reportChange( const BinID& bid )
{
    Threads::Locker lckr( changebidlock_ );
    if ( !change.isEnabled() )
	notificationqueue_->add( bid );
    else
    {
	changebid_ = bid;
	change.trigger();
    }
}


void EventManager::fillPar( IOPar& par ) const
{
    par.set( sKeyStorageID(), storageid_ );
}


bool EventManager::usePar( const IOPar& par )
{
    MultiID mid;
    if ( par.get( sKeyStorageID(), mid ) )
    {
	PtrMan<Executor> exec = setStorageID( mid, true );
	if ( exec )
	    return exec->execute();
    }

    return true;
}


bool EventManager::getDip( const BinIDValue& bidv,int horid,
			   float& inldip, float& crldip )
{
    return getDip( bidv, horid, true, inldip, crldip ) ||
           getDip( bidv, horid, false, inldip, crldip );
}


bool EventManager::getDip( const BinIDValue& bidv,int horid,
			   bool primary,
			   float& inldip, float& crldip )
{
    const DipSource& ds = primary ? primarydipsource_ : secondarydipsource_;
    if ( ds.type_==DipSource::None )
	return false;

    if ( ds.type_==DipSource::Horizon )
    {
	const int horidx = horids_.indexOf( horid );
	if ( horidx==-1 || horrefs_[horidx].isUdf() )
	    return false;

	if ( !emhorizons_[horidx] ||
	      emhorizons_[horidx]->multiID()!=horrefs_[horidx] )
	{
	    if ( emhorizons_[horidx] )
	    {
		emhorizons_[horidx]->unRef();
		emhorizons_.replace( horidx, 0 );
	    }

	    RefMan<EM::EMObject> emobj =
		EM::EMM().loadIfNotFullyLoaded( horrefs_[horidx] );
	    mDynamicCastGet( EM::Horizon3D*, hor, emobj.ptr() );
	    if ( !hor ) return false;

	    hor->ref();
	    emhorizons_.replace( horidx, hor );
	}

	const BinID horstep = emhorizons_[horidx]->geometry().loadedStep();
	BinID previnl( bidv.inl()-horstep.inl(), bidv.crl() );
	BinID nextinl( bidv.inl()+horstep.inl(), bidv.crl() );
	if ( !emhorizons_[horidx]->isDefined(previnl.toInt64() ) )
	    previnl = bidv;
	if ( !emhorizons_[horidx]->isDefined(nextinl.toInt64() ) )
	    nextinl = bidv;

	if ( previnl==nextinl )
	    return false;

	const float inldiff = (float)
                              (emhorizons_[horidx]->getPos(nextinl.toInt64() ).z_ -
                               emhorizons_[horidx]->getPos(previnl.toInt64() ).z_);

	BinID prevcrl( bidv.inl(), bidv.crl()-horstep.crl() );
	BinID nextcrl( bidv.inl(), bidv.crl()+horstep.crl() );
	if ( !emhorizons_[horidx]->isDefined(prevcrl.toInt64() ) )
	    prevcrl = bidv;
	if ( !emhorizons_[horidx]->isDefined(nextcrl.toInt64() ) )
	    nextcrl = bidv;

	if ( prevcrl==nextcrl )
	    return false;

	const float crldiff = (float)
                              (emhorizons_[horidx]->getPos(nextcrl.toInt64() ).z_ -
                               emhorizons_[horidx]->getPos(prevcrl.toInt64() ).z_);

	inldip = inldiff/((nextinl.inl()-previnl.inl())*SI().inlDistance() );
	crldip = crldiff/((nextcrl.crl()-prevcrl.crl())*SI().crlDistance() );

	return true;
    }

    if ( ds.type_==DipSource::SteeringVolume )
    {
	if ( ds.mid_.isUdf() )
	    return false;

	SeisTrcReader*& reader =
	    primary ? primarydipreader_ : secondarydipreader_;

	if ( !reader || !reader->ioObj() || reader->ioObj()->key()!=ds.mid_ )
	{
	    const SeisIOObjInfo seisinfo( ds.mid_ );
	    deleteAndNullPtr( reader );
	    reader = new SeisTrcReader( ds.mid_, seisinfo.geomType() );
	    if ( !reader->prepareWork() )
		return false;
	}

	mDynamicCastGet(SeisTrcTranslator*,translator,reader->translator());
	if ( !translator->supportsGoTo() || translator->goTo(bidv) )
	    return false;

	SeisTrc diptrc;
	if ( !reader->get(diptrc) || diptrc.nrComponents()<2 )
	    return false;

	float tmpinldip = diptrc.getValue( bidv.val(), 0 );
	float tmpcrldip = diptrc.getValue( bidv.val(), 1 );

	if ( mIsUdf(tmpinldip) || mIsUdf(tmpcrldip) )
	    return false;

	if ( SI().zIsTime() )
	{
	    inldip = tmpinldip/1e6f;
	    crldip = tmpcrldip/1e6f;
	}
	else
	{
	    inldip = tmpinldip;
	    crldip = tmpcrldip;
	}

	return true;
    }

    return false;
}


// SetPickUndo

SetPickUndo::SetPickUndo( EventManager& man, const BinID& bid, int horidx,
			  const OffsetAzimuth& oa,
			  float depth, unsigned char pickquality )
    : manager_( man )
    , oa_( oa )
    , bid_( bid )
    , horidx_( horidx )
    , olddepth_( depth )
    , oldquality_( pickquality )
{
    RefMan<EventSet> events = manager_.getEvents( bid, false, false );
    Event* event = events->events_[horidx_];
    const int idx = event->indexOf( oa_ );
    if ( idx==-1 )
	newdepth_ = mUdf(float);
    else
    {
	newdepth_ = event->pick_[idx];
	if ( event->pickquality_ )
	    newquality_ = event->pickquality_[idx];
    }
}


SetPickUndo::~SetPickUndo()
{
}


bool SetPickUndo::unDo()
{
    return doWork( olddepth_, oldquality_ );
}


bool SetPickUndo::reDo()
{
    return doWork( newdepth_, newquality_ );
}


bool SetPickUndo::doWork( float nd, unsigned char nq )
{
    RefMan<EventSet> events = manager_.getEvents( bid_, true, false );
    if ( !events )
	return false;

    if ( events->events_.size()<=horidx_ )
	return false;

    Event* event = events->events_[horidx_];
    int idx = event->indexOf( oa_ );
    const bool isdefined = !mIsUdf(nd);
    if ( idx==-1 )
    {
	if ( !isdefined )
	    return true;

	idx = event->sz_;
	event->addPick();
	event->offsetazimuth_[idx] = oa_;
    }

    if ( !isdefined )
    {
	event->removePick( idx );
    }
    else
    {
	event->pick_[idx] = nd;
	if ( event->pickquality_ )
	    event->pickquality_[idx] = nq;
    }

    events->ischanged_ = true;

    manager_.reportChange( bid_ );

    return true;
}


SetEventUndo::SetEventUndo( EventManager& man, const BinID& bid, int horidx,
			  short horid, VSEvent::Type evt,
			  unsigned char quality )
    : manager_( man )
    , bid_( bid )
    , horidx_( horidx )
    , horid_( horid )
    , eventtype_( evt )
    , quality_( quality )
    , isremove_( true )
{
}


SetEventUndo::SetEventUndo( EventManager& man, const BinID& bid, int horidx )
    : manager_( man )
    , bid_( bid )
    , horidx_( horidx )
    , isremove_( false )
{
    RefMan<EventSet> events = manager_.getEvents( bid, false, false );
    Event* event = events->events_[horidx_];
    quality_ = event->quality_;
    eventtype_ = event->eventtype_;
    horid_ = event->horid_;
}


SetEventUndo::~SetEventUndo()
{
}


bool SetEventUndo::unDo()
{
    if ( isremove_ )
	return addEvent();

    return removeEvent();
}


bool SetEventUndo::reDo()
{
    if ( isremove_ )
	return removeEvent();

    return addEvent();
}


bool SetEventUndo::addEvent()
{
    RefMan<EventSet> events = manager_.getEvents( bid_, true, false );
    if ( !events )
	return false;

    auto* ev = new Event( 0, true );
    ev->quality_ = quality_;
    ev->horid_ = horid_;
    ev->eventtype_ = eventtype_;

    events->events_.insertAt( ev, horidx_ );
    events->ischanged_ = true;

    manager_.reportChange( bid_ );

    return true;
}


bool SetEventUndo::removeEvent()
{
    RefMan<EventSet> events = manager_.getEvents( bid_, true, false );
    if ( !events )
	return false;

    delete events->events_.removeSingle( horidx_ );

    manager_.reportChange( bid_ );

    return true;
}


} // namespace PreStack
