/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


#include "picksetmgr.h"
#include "ioman.h"
#include "iopar.h"
#include "polygon.h"
#include "survinfo.h"
#include "tabledef.h"
#include "posimpexppars.h"
#include "unitofmeasure.h"
#include "od_iostream.h"
#include <ctype.h>

static const char* sKeyConnect = "Connect";


// Pick::SetMgr
Pick::SetMgr& Pick::SetMgr::getMgr( const char* nm )
{
    mDefineStaticLocalObject( PtrMan<ManagedObjectSet<SetMgr> >, mgrs, = 0 );
    SetMgr* newmgr = 0;
    if ( !mgrs )
    {
	mgrs = new ManagedObjectSet<SetMgr>;
	newmgr = new SetMgr( 0 );
	    // ensure the first mgr has the 'empty' name
    }
    else if ( (!nm || !*nm) )
	return *((*mgrs)[0]);
    else
    {
	for ( int idx=1; idx<mgrs->size(); idx++ )
	{
	    if ( (*mgrs)[idx]->name() == nm )
		return *((*mgrs)[idx]);
	}
    }

    if ( !newmgr )
	newmgr = new SetMgr( nm );

    *mgrs += newmgr;
    return *newmgr;
}


Pick::SetMgr::SetMgr( const char* nm )
    : NamedMonitorable(nm)
    , locationChanged(this), setToBeRemoved(this)
    , setAdded(this), setChanged(this)
    , setDispChanged(this)
    , undo_( *new Undo() )
{
    mAttachCB( IOM().entryRemoved, SetMgr::objRm );
    mAttachCB( IOM().surveyToBeChanged, SetMgr::survChg );
    mAttachCB( IOM().applicationClosing, SetMgr::survChg );
}


Pick::SetMgr::~SetMgr()
{
    sendDelNotif();
    detachAllNotifiers();
    undo_.removeAll();
    delete &undo_;
}


void Pick::SetMgr::add( const MultiID& ky, Set* st )
{
    if ( !st )
	{ pErrMsg("Huh"); return; }
    pss_ += st; ids_ += ky; changed_ += false;
    transfer( *st, *this );
    setAdded.trigger( st );
}


void Pick::SetMgr::set( const MultiID& ky, Set* newset )
{
    Set* oldset = find( ky );
    if ( !oldset )
    {
	if ( newset )
	    add( ky, newset );
    }
    else if ( newset != oldset )
    {
	const int idx = pss_.indexOf( oldset );
	//Must be removed from list before trigger, otherwise
	//other users may remove it in calls invoked by the cb.
	pss_.removeSingle( idx );
	setToBeRemoved.trigger( oldset );
	delete oldset;
	ids_.removeSingle( idx );
	changed_.removeSingle( idx );
	if ( newset )
	    add( ky, newset );
    }
}


const MultiID& Pick::SetMgr::id( int idx ) const
{
    return ids_.validIdx(idx) ? ids_[idx] : MultiID::udf();
}


void Pick::SetMgr::setID( int idx, const MultiID& mid )
{
    ids_[idx] = mid;
}


void Pick::SetMgr::transfer( Set& ps, SetMgr& oth )
{
    SetMgr* cursetmgr = &ps.getSetMgr();
    if ( cursetmgr == &oth )
	return;
    else if ( cursetmgr != this )
	cursetmgr->transfer( ps, oth );
    else if ( &oth == this )
	return;
    else
    {
	// really transfer one of mine to another SetMgr
	const int idxof = indexOf( ps );
	if ( idxof < 0 )
	    ps.mgr_ = &oth;
	else
	{
	    setToBeRemoved.trigger( &ps );
	    const MultiID setid = ids_[idxof];
	    pss_.removeSingle( idxof );
	    ids_.removeSingle( idxof );
	    changed_.removeSingle( idxof );
	    oth.set( setid, &ps );
	}
    }
}


int Pick::SetMgr::indexOf( const Set& st ) const
{
    return pss_.indexOf( &st );
}


int Pick::SetMgr::indexOf( const MultiID& ky ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ky == ids_[idx] )
	    return idx;
    }
    return -1;
}


int Pick::SetMgr::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( pss_[idx]->name() == nm )
	    return idx;
    }
    return -1;
}


Pick::Set* Pick::SetMgr::find( const MultiID& ky ) const
{
    const int idx = indexOf( ky );
    return idx < 0 ? 0 : const_cast<Set*>( pss_[idx] );
}


MultiID* Pick::SetMgr::find( const Set& st ) const
{
    const int idx = indexOf( st );
    return idx < 0 ? 0 : const_cast<MultiID*>( &ids_[idx] );
}


Pick::Set* Pick::SetMgr::find( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : const_cast<Set*>( pss_[idx] );
}


void Pick::SetMgr::reportChange( CallBacker* sender, const ChangeData& cd )
{
    const int setidx = pss_.indexOf( cd.set_ );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	locationChanged.trigger( const_cast<ChangeData*>( &cd ), sender );
    }
}


void Pick::SetMgr::reportChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	setChanged.trigger( const_cast<Set*>(&s), sender );
    }
}


void Pick::SetMgr::reportDispChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	setDispChanged.trigger( const_cast<Set*>(&s), sender );
    }
}


void Pick::SetMgr::removeCBs( CallBacker* cb )
{
    locationChanged.removeWith( cb );
    setToBeRemoved.removeWith( cb );
    setAdded.removeWith( cb );
    setChanged.removeWith( cb );
    setDispChanged.removeWith( cb );
}


void Pick::SetMgr::removeAll()
{
    for ( int idx=pss_.size()-1; idx>=0; idx-- )
    {
	Set* pset = pss_.removeSingle( idx );
	setToBeRemoved.trigger( pset );
	delete pset;
    }
}


void Pick::SetMgr::survChg( CallBacker* )
{
    removeAll();
    locationChanged.cbs_.erase();
    setToBeRemoved.cbs_.erase();
    setAdded.cbs_.erase();
    setChanged.cbs_.erase();
    setDispChanged.cbs_.erase();
    ids_.erase();
    changed_.erase();
}


void Pick::SetMgr::objRm( CallBacker* cb )
{
    mCBCapsuleUnpack(MultiID,ky,cb);
    if ( indexOf(ky) >= 0 )
	set( ky, 0 );
}


namespace Pick
{

class LocationUndoEvent: public UndoEvent
{
public:

    enum  Type	    { Insert, PolygonClose, Remove, Move };

    // Constructor will be called from Pick::Set in write-locked state.
    // you cannot use MT-protected methods like size() here, this is why
    // the class is a friend of Pick::Set

LocationUndoEvent( Type type, const MultiID& mid, int sidx,
		      const Location& loc, SetMgr& mgr )
    : mgr_(mgr)
{
    type_ = type;
    newloc_ = Coord3::udf();
    loc_ = Coord3::udf();

    mid_ = mid;
    index_ = sidx;
    const int idxof = mgr_.indexOf( mid_ );
    Pick::Set* set = idxof < 0 ? 0 : &mgr_.get( mid_ );

    if ( type == Insert || type == PolygonClose )
    {
	if ( set && set->locs_.size() > index_ )
	    loc_ = set->locs_[ index_ ];
    }
    else if ( type == Remove )
    {
	loc_ = loc;
    }
    else if ( type == Move )
    {
	if ( set && set->locs_.size()>index_ )
	    loc_ = set->locs_[ index_ ];
	newloc_ = loc;
    }
}

virtual const char* getStandardDesc() const
{
    switch ( type_ )
    {
    case Insert:	return "Insert";
    case Remove:	return "Remove";
    case Move:		return "Move";
    case PolygonClose:	return "Close Polygon";
    default:		{ pErrMsg("Huh"); return ""; }
    }

}

virtual bool unDo()
{
    const int setidx = mgr_.indexOf( mid_ );
    if ( setidx < 0 )
	return false;
    Set& set = mgr_.get( setidx );

    if ( set.connection() == Pick::Set::Disp::Close
      && index_ == set.size()-1 && type_ != Move )
	type_ = PolygonClose;

    SetMgr::ChangeData::Ev ev =
	    type_ == Move   ? SetMgr::ChangeData::Changed
	: ( type_ == Remove ? SetMgr::ChangeData::Added
			    : SetMgr::ChangeData::ToBeRemoved );

    SetMgr::ChangeData cd( ev, &set, index_ );

    if ( type_ == Move )
    {
       if ( set.size()>index_  && loc_.pos().isDefined() )
	   set.set( index_, loc_ );
    }
    else if ( type_ == Remove )
    {
       if ( loc_.pos().isDefined() )
	 set.insert( index_, loc_ );
    }
    else if ( type_ == Insert  )
    {
	set.remove( index_ );
    }
    else if ( type_ == PolygonClose )
    {
	set.setConnection( Pick::Set::Disp::Open );
	set.remove(index_);
    }

    mgr_.reportChange( 0, cd );

    return true;
}


virtual bool reDo()
{
    const int setidx = mgr_.indexOf( mid_ );
    if ( setidx < 0 )
	return false;
    Set& set = mgr_.get( setidx );

    SetMgr::ChangeData::Ev ev =
		type_ == Move	? SetMgr::ChangeData::Changed
	    : ( type_ == Remove ? SetMgr::ChangeData::ToBeRemoved
				: SetMgr::ChangeData::Added );

    Pick::SetMgr::ChangeData cd( ev, &set, index_ );

    if ( type_ == Move )
    {
	if ( set.size()>index_ && newloc_.pos().isDefined() )
	    set.set( index_, newloc_ );
    }
    else if ( type_ == Remove )
    {
	set.remove( index_ );
    }
    else if ( type_ == Insert )
    {
	if ( loc_.pos().isDefined() )
	    set.insert( index_, loc_ );
    }
    else if ( type_ == PolygonClose )
    {
	if ( loc_.pos().isDefined() )
	{
	    set.setConnection( Pick::Set::Disp::Close );
	    set.insert( index_, loc_ );
	}
    }

    mgr_.reportChange( 0, cd );

    return true;
}

    SetMgr&	mgr_;
    Location	loc_;
    Location	newloc_;
    MultiID	mid_;
    int		index_;
    Type	type_;

};

} // namespace Pick


// Pick::Set
mDefineEnumUtils( Pick::Set::Disp, Connection, "Connection" )
{ "None", "Open", "Close", 0 };

Pick::Set::Set( const char* nm, SetMgr* mgr )
    : NamedMonitorable(nm)
    , mgr_(mgr)
{
}


Pick::Set::Set( const Set& oth )
    : mgr_(0)
{
    *this = oth;
}


Pick::Set::~Set()
{
    sendDelNotif();
}


Pick::Set& Pick::Set::operator=( const Set& oth )
{
    if ( &oth != this )
    {
	NamedMonitorable::operator =( oth );
	mLock4Write();
	AccessLockHandler lh( oth );
	locs_.copy( oth.locs_ );
	disp_ = oth.disp_; pars_ = oth.pars_;
	// no copy of mgr_
    }
    return *this;
}


void Pick::Set::setSetMgr( SetMgr* newmgr )
{
    getSetMgr().transfer( *this, newmgr ? *newmgr : Mgr() );
}


#define mGetSetMgr() (mgr_ ? *mgr_ : Mgr())

Pick::SetMgr& Pick::Set::getSetMgr() const
{
    mLock4Read();
    return mGetSetMgr();
}


Pick::Set::size_type Pick::Set::size() const
{
    mLock4Read();
    return locs_.size();
}


bool Pick::Set::validIdx( size_type idx ) const
{
    mLock4Read();
    return locs_.validIdx( idx );
}


Pick::Location Pick::Set::get( size_type idx ) const
{
    mLock4Read();
    return locs_.validIdx(idx) ? locs_[idx] : Location::udf();
}


Coord Pick::Set::getPos( size_type idx ) const
{
    mLock4Read();
    return locs_.validIdx(idx) ? locs_[idx].pos() : Coord::udf();
}


double Pick::Set::getZ( size_type idx ) const
{
    mLock4Read();
    return locs_.validIdx(idx) ? locs_[idx].pos().z : mUdf(double);
}


#define mPrepRead(sz) \
    mLock4Read(); \
    const size_type sz = locs_.size()


bool Pick::Set::isPolygon() const
{
    mLock4Read();
    const FixedString typ = pars_.find( sKey::Type() );
    return typ.isEmpty() ? disp_.connect_ != Set::Disp::None
			 : typ == sKey::Polygon();
}


bool Pick::Set::isMultiGeom() const
{
    mPrepRead( sz );
    if ( sz < 2 )
	return false;
    const Pos::GeomID geomid0 = locs_[0].geomID();
    for ( size_type idx=1; idx<sz; idx++ )
	if ( locs_[idx].geomID() != geomid0 )
	    return true;
    return false;
}


Pos::GeomID Pick::Set::firstGeomID() const
{
    mLock4Read();
    return locs_.isEmpty() ? -1 : locs_[0].trcKey().geomID();
}


bool Pick::Set::has2D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return false;
    for ( size_type idx=0; idx<sz; idx++ )
	if ( locs_[idx].is2D() )
	    return true;
    return false;
}


bool Pick::Set::has3D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return true;
    for ( size_type idx=0; idx<sz; idx++ )
	if ( !locs_[idx].is2D() )
	    return true;
    return false;
}


bool Pick::Set::hasOnly2D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return false;
    for ( size_type idx=0; idx<sz; idx++ )
	if ( !locs_[idx].is2D() )
	    return false;
    return true;
}


bool Pick::Set::hasOnly3D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return true;
    for ( size_type idx=0; idx<sz; idx++ )
	if ( locs_[idx].is2D() )
	    return false;
    return true;
}


void Pick::Set::getPolygon( ODPolygon<double>& poly ) const
{
    mPrepRead( sz );
    for ( size_type idx=0; idx<sz; idx++ )
    {
	const Coord c( locs_[idx].pos() );
	poly.add( Geom::Point2D<double>( c.x, c.y ) );
    }
}


void Pick::Set::getLocations( ObjectSet<const Location>& locs ) const
{
    mPrepRead( sz );
    for ( size_type idx=0; idx<sz; idx++ )
	locs += &locs_[idx];
}


float Pick::Set::getXYArea() const
{
    mPrepRead( sz );
    if ( sz < 3 || disp_.connect_ == Disp::None )
	return mUdf(float);

    TypeSet<Geom::Point2D<float> > posxy;
    for ( size_type idx=sz-1; idx>=0; idx-- )
    {
	const Coord localpos = locs_[idx].pos();
	posxy += Geom::Point2D<float>( (float)localpos.x, (float)localpos.y );
    }

    ODPolygon<float> polygon( posxy );
    if ( polygon.isSelfIntersecting() )
	return mUdf(float);

    float area = (float) polygon.area();
    if ( SI().xyInFeet() )
	area *= (mFromFeetFactorF*mFromFeetFactorF);

    return area;
}


Pick::Set::size_type Pick::Set::find( const TrcKey& tk ) const
{
    mPrepRead( sz );
    for ( size_type idx=0; idx<sz; idx++ )
	if ( locs_[idx].trcKey() == tk )
	    return idx;
    return -1;
}


Pick::Set::size_type Pick::Set::nearestLocation( const Coord& pos ) const
{
    return nearestLocation( Coord3(pos.x,pos.y,0.f), true );
}


Pick::Set::size_type Pick::Set::nearestLocation( const Coord3& pos,
						 bool ignorez ) const
{
    mPrepRead( sz );
    if ( sz < 2 )
	return sz - 1;
    if ( pos.isUdf() )
	return 0;

    size_type ret = 0;
    const Coord3& p0 = locs_[ret].pos();
    double minsqdist = p0.isUdf() ? mUdf(double)
		     : (ignorez ? pos.sqHorDistTo( p0 ) : pos.sqDistTo( p0 ));

    for ( size_type idx=1; idx<sz; idx++ )
    {
	const Coord3& curpos = locs_[idx].pos();
	if ( pos.isUdf() )
	    continue;

	const double sqdist = ignorez ? pos.sqHorDistTo( curpos )
				      : pos.sqDistTo( curpos );
	if ( sqdist == 0 )
	    return idx;
	else if ( sqdist < minsqdist )
	    { minsqdist = sqdist; ret = idx; }
    }
    return ret;
}


void Pick::Set::fillPar( IOPar& par ) const
{
    mLock4Read();
    BufferString parstr;
    disp_.mkstyle_.toString( parstr );
    par.set( sKey::MarkerStyle(), parstr );
    par.set( sKeyConnect, Disp::toString(disp_.connect_) );
    par.merge( pars_ );
}


bool Pick::Set::usePar( const IOPar& par )
{
    mLock4Write();
    const bool v6_or_earlier = ( par.majorVersion()+par.minorVersion()*0.1 )>0
	&& ( par.majorVersion()+par.minorVersion()*0.1 )<=6;

    BufferString mkststr;
    if ( par.get(sKey::MarkerStyle(),mkststr) && v6_or_earlier )
	disp_.mkstyle_.fromString( mkststr );
    else
    {
	BufferString colstr;
	if ( par.get(sKey::Color(),colstr) )
	    disp_.mkstyle_.color_.use( colstr.buf() );
	par.get( sKey::Size(),disp_.mkstyle_.size_ );
	int type = 0;
	par.get( sKeyMarkerType(),type );
	type++;
	disp_.mkstyle_.type_ = (OD::MarkerStyle3D::Type)type;
    }

    bool doconnect;
    par.getYN( sKeyConnect, doconnect );	// For Backward Compatibility
    if ( doconnect )
	disp_.connect_ = Disp::Close;
    else if ( !Disp::ConnectionDef().parse(par.find(sKeyConnect),
					   disp_.connect_) )
	disp_.connect_ = Disp::None;

    pars_ = par;
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::Size() );
    pars_.removeWithKey( sKeyMarkerType() );
    pars_.removeWithKey( sKeyConnect );

    return true;
}


void Pick::Set::updateInPar( const char* ky, const char* val )
{
    mLock4Write();
    pars_.update( ky, val );
}


Pick::Set& Pick::Set::setEmpty()
{
    mLock4Read();
    if ( locs_.isEmpty() )
	return *this;

    if ( mLock2Write() && !locs_.isEmpty() )
    {
	locs_.setEmpty();
	mSendChgNotif();
    }
    return *this;
}


Pick::Set& Pick::Set::append( const Set& oth )
{
    if ( !oth.isEmpty() )
    {
	mLock4Write();
	MonitorLock monlock( oth );
	locs_.append( oth.locs_ );
	monlock.unlockNow();
	mSendChgNotif();
    }
    return *this;
}


void Pick::Set::addUndoEvent( int type, size_type idx, const Location& loc )
{
    // Method will be called in write-locked state.
    // do not use MT-rptected methods like size() here
    SetMgr& mgr = mGetSetMgr();
    if ( mgr.indexOf(*this) < 0 )
	return;

    const MultiID mid = mgr.get(*this);
    const LocationUndoEvent::Type undotype = (LocationUndoEvent::Type)type;
    if ( !mid.isEmpty() )
    {
	const Location touse = type == LocationUndoEvent::Insert
			     ? Location(Coord3::udf()) : loc;
	LocationUndoEvent* undo = new LocationUndoEvent( undotype, mid, idx,
						       touse, mGetSetMgr() );
	Pick::Mgr().undo().addEvent( undo, 0 );
    }
}


#define mAddUndoEvent(typ,idx,loc) \
    if ( withundo ) \
	addUndoEvent( (int)LocationUndoEvent::typ, idx, loc ); \
    mSendChgNotif()


Pick::Set& Pick::Set::add( const Location& loc, bool withundo )
{
    mLock4Write();
    locs_ += loc;
    mAddUndoEvent( Insert, locs_.size()-1, loc );
    return *this;
}


Pick::Set& Pick::Set::insert( size_type idx, const Location& loc,
			      bool withundo )
{
    mLock4Write();
    locs_.insert( idx, loc );
    mAddUndoEvent( Insert, idx, loc );
    return *this;
}


Pick::Set& Pick::Set::set( size_type idx, const Location& loc, bool withundo )
{
    mLock4Read();
    if ( !locs_.validIdx(idx) || loc == locs_[idx] )
	return *this;

    if ( mLock2Write() && locs_.validIdx(idx) )
    {
	locs_[idx] = loc;
	mAddUndoEvent( Move, idx, loc );
    }

    return *this;
}


Pick::Set& Pick::Set::remove( size_type idx, bool withundo )
{
    mLock4Read();
    if ( !locs_.validIdx(idx) )
	return *this;

    if ( mLock2Write() && locs_.validIdx(idx) )
    {
	const Location loc = locs_[idx];
	locs_.removeSingle( idx );
	mAddUndoEvent( Remove, idx, loc );
    }

    return *this;
}


static inline bool coordUnchanged( Pick::Set::size_type idx,
	const TypeSet<Pick::Location>& locs, const Coord& coord )
{
    return !locs.validIdx(idx) || locs[idx].pos().sqHorDistTo(coord) < 0.01;
}


Pick::Set& Pick::Set::setPos( size_type idx, const Coord& coord )
{
    mLock4Read();
    if ( coordUnchanged(idx,locs_,coord) )
	return *this;

    if ( mLock2Write() && !coordUnchanged(idx,locs_,coord) )
    {
	locs_[idx].setPos( coord );
	mSendChgNotif();
    }

    return *this;
}



static inline bool zUnchanged( Pick::Set::size_type idx,
	const TypeSet<Pick::Location>& locs, double z )
{
    if ( !locs.validIdx(idx) )
	return true;
    const double zdiff = locs[idx].z() - z;
    return mIsZero(zdiff,1e-6);
}

Pick::Set& Pick::Set::setZ( size_type idx, double z )
{
    mLock4Read();
    if ( zUnchanged(idx,locs_,z) )
	return *this;

    if ( mLock2Write() && !zUnchanged(idx,locs_,z) )
    {
	locs_[idx].setZ( z );
	mSendChgNotif();
    }

    return *this;
}


// PickSetAscIO

Table::FormatDesc* PickSetAscIO::getDesc( bool iszreq )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "PickSet" );
    createDescBody( fd, iszreq );
    return fd;
}


void PickSetAscIO::createDescBody( Table::FormatDesc* fd, bool iszreq )
{
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    if ( iszreq )
	fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
}


void PickSetAscIO::updateDesc( Table::FormatDesc& fd, bool iszreq )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, iszreq );
}


bool PickSetAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


#define mErrRet(s) { if ( !s.isEmpty() ) errmsg_ = s; return 0; }

bool PickSetAscIO::get( od_istream& strm, Pick::Set& ps,
			bool iszreq, float constz ) const
{
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) mErrRet(errmsg_)
	if ( ret == 0 ) break;

	const double xread = getDValue( 0 );
	const double yread = getDValue( 1 );
	if ( mIsUdf(xread) || mIsUdf(yread) ) continue;

	Coord pos( xread, yread );
	mPIEPAdj(Coord,pos,true);
	if ( !isXY() || !SI().isReasonable(pos) )
	{
	    BinID bid( mNINT32(xread), mNINT32(yread) );
	    mPIEPAdj(BinID,bid,true);
	    SI().snap( bid );
	    pos = SI().transform( bid );
	}

	if ( !SI().isReasonable(pos) ) continue;

	float zread = constz;
	if ( iszreq )
	{
	    zread = getFValue( 2 );
	    if ( mIsUdf(zread) )
		continue;

	    mPIEPAdj(Z,zread,true);
	}

	ps.add( Pick::Location(pos,zread) );
    }

    return true;
}
