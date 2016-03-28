/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/

static const char* rcsID mUsedVar = "$Id$";

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

namespace Pick
{

// Pick::SetMgr
SetMgr& SetMgr::getMgr( const char* nm )
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


SetMgr::SetMgr( const char* nm )
    : NamedObject(nm)
    , locationChanged(this), setToBeRemoved(this)
    , setAdded(this), setChanged(this)
    , setDispChanged(this)
    , undo_( *new Undo() )
{
    mAttachCB( IOM().entryRemoved, SetMgr::objRm );
    mAttachCB( IOM().surveyToBeChanged, SetMgr::survChg );
    mAttachCB( IOM().applicationClosing, SetMgr::survChg );
}


SetMgr::~SetMgr()
{
    detachAllNotifiers();
    undo_.removeAll();
    delete &undo_;
}


void SetMgr::add( const MultiID& ky, Set* st )
{
    pss_ += st; ids_ += ky; changed_ += false;
    setAdded.trigger( st );
}


void SetMgr::set( const MultiID& ky, Set* newset )
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



const Undo& SetMgr::undo() const	{ return undo_; }
Undo& SetMgr::undo()			{ return undo_; }


const MultiID& SetMgr::id( int idx ) const
{ return ids_.validIdx(idx) ? ids_[idx] : MultiID::udf(); }


void SetMgr::setID( int idx, const MultiID& mid )
{
    ids_[idx] = mid;
}


int SetMgr::indexOf( const Set& st ) const
{
    return pss_.indexOf( &st );
}


int SetMgr::indexOf( const MultiID& ky ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ky == ids_[idx] )
	    return idx;
    }
    return -1;
}


int SetMgr::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( pss_[idx]->name() == nm )
	    return idx;
    }
    return -1;
}


Set* SetMgr::find( const MultiID& ky ) const
{
    const int idx = indexOf( ky );
    return idx < 0 ? 0 : const_cast<Set*>( pss_[idx] );
}


MultiID* SetMgr::find( const Set& st ) const
{
    const int idx = indexOf( st );
    return idx < 0 ? 0 : const_cast<MultiID*>( &ids_[idx] );
}


Set* SetMgr::find( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : const_cast<Set*>( pss_[idx] );
}


void SetMgr::reportChange( CallBacker* sender, const ChangeData& cd )
{
    const int setidx = pss_.indexOf( cd.set_ );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	locationChanged.trigger( const_cast<ChangeData*>( &cd ), sender );
    }
}


void SetMgr::reportChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	setChanged.trigger( const_cast<Set*>(&s), sender );
    }
}


void SetMgr::reportDispChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	setDispChanged.trigger( const_cast<Set*>(&s), sender );
    }
}


void SetMgr::removeCBs( CallBacker* cb )
{
    locationChanged.removeWith( cb );
    setToBeRemoved.removeWith( cb );
    setAdded.removeWith( cb );
    setChanged.removeWith( cb );
    setDispChanged.removeWith( cb );
}


void SetMgr::removeAll()
{
    for ( int idx=pss_.size()-1; idx>=0; idx-- )
    {
	Set* pset = pss_.removeSingle( idx );
	setToBeRemoved.trigger( pset );
	delete pset;
    }
}


void SetMgr::survChg( CallBacker* )
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


void SetMgr::objRm( CallBacker* cb )
{
    mCBCapsuleUnpack(MultiID,ky,cb);
    if ( indexOf(ky) >= 0 )
	set( ky, 0 );
}


class PickSetKnotUndoEvent: public UndoEvent
{
public:
    enum  UnDoType	    { Insert, PolygonClose, Remove, Move };
    PickSetKnotUndoEvent( UnDoType type, const MultiID& mid, int sidx,
	const Pick::Location& pos )
    { init( type, mid, sidx, pos ); }

    void init( UnDoType type, const MultiID& mid, int sidx,
	const Pick::Location& pos )
    {
	type_ = type;
	newpos_ = Coord3::udf();
	pos_ = Coord3::udf();

	mid_ = mid;
	index_ = sidx;
	Pick::SetMgr& mgr = Pick::Mgr();
	Pick::Set& set = mgr.get(mid);

	if ( type == Insert || type == PolygonClose )
	{
	    if ( &set && set.size()>index_ )
		pos_ = set[index_];
	}
	else if ( type == Remove )
	{
	    pos_ = pos;
	}
	else if ( type == Move )
	{
	    if ( &set && set.size()>index_ )
		pos_ = set[index_];
	    newpos_ = pos;
	}

    }


    const char* getStandardDesc() const
    {
	if ( type_ == Insert )
	    return "Insert Knot";
	else if ( type_ == Remove )
	    return "Remove";
	else if ( type_ == Move )
	    return "move";

	return "";
    }


    bool unDo()
    {
	Pick::SetMgr& mgr = Pick::Mgr();
	Pick::Set& set = mgr.get(mid_);
	if ( !&set ) return false;

	if ( set.disp_.connect_==Pick::Set::Disp::Close &&
	    index_ == set.size()-1 && type_ != Move )
	    type_ = PolygonClose;

	Pick::SetMgr::ChangeData::Ev ev = type_ == Move ?
	    Pick::SetMgr::ChangeData::Changed :
	    ( type_ == Remove
	    ? Pick::SetMgr::ChangeData::Added
	    : Pick::SetMgr::ChangeData::ToBeRemoved );

	Pick::SetMgr::ChangeData cd( ev, &set, index_ );

	if ( type_ == Move )
	{
	   if ( &set && set.size()>index_  && pos_.pos_.isDefined() )
	       set[index_] = pos_;
	}
	else if ( type_ == Remove )
	{
	   if ( pos_.pos_.isDefined() )
	     set.insert( index_, pos_ );
	}
	else if ( type_ == Insert  )
	{
	    set.removeSingle( index_ );
	}
	else if ( type_ == PolygonClose )
	{
	    set.disp_.connect_ = Pick::Set::Disp::Open;
	    set.removeSingle(index_);
	}

	mgr.reportChange( 0, cd );

	return true;
    }


    bool reDo()
    {
	Pick::SetMgr& mgr = Pick::Mgr();
	Pick::Set& set = mgr.get( mid_ );
	if ( !&set ) return false;

	Pick::SetMgr::ChangeData::Ev ev = type_== Move ?
	    Pick::SetMgr::ChangeData::Changed :
	    ( type_ == Remove
	    ? Pick::SetMgr::ChangeData::ToBeRemoved
	    : Pick::SetMgr::ChangeData::Added );

	Pick::SetMgr::ChangeData cd( ev, &set, index_ );

	if ( type_ == Move )
	{
	    if ( &set && set.size()>index_ && newpos_.pos_.isDefined() )
		set[index_] = newpos_;
	}
	else if ( type_ == Remove )
	{
	    set.removeSingle( index_ );
	}
	else if ( type_ == Insert )
	{
	    if ( pos_.pos_.isDefined() )
		set.insert( index_,pos_ );
	}
	else if ( type_ == PolygonClose )
	{
	    if ( pos_.pos_.isDefined() )
	    {
		set.disp_.connect_=Pick::Set::Disp::Close;
		set.insert( index_, pos_ );
	    }
	}

	mgr.reportChange( 0, cd );

	return true;
    }

protected:
    Pick::Location  pos_;
    Pick::Location  newpos_;
    MultiID	    mid_;
    int		    index_;
    UnDoType	    type_;
};


// Both Pick set types

template <class PicksType>
static typename PicksType::size_type findIdx( const PicksType& picks,
	                                      const TrcKey& tk )
{
    const typename PicksType::size_type sz = picks.size();
    for ( typename PicksType::size_type idx=0; idx<sz; idx++ )
	if ( picks.get(idx).trcKey() == tk )
	    return idx;
    return -1;
}

template <class PicksType>
static typename PicksType::size_type getNearestLocation( const PicksType& ps,
				    const Coord3& pos, bool ignorez )
{
    const typename PicksType::size_type sz = ps.size();
    if ( sz < 2 )
	return sz - 1;
    if ( pos.isUdf() )
	return 0;

    typename PicksType::size_type ret = 0;
    const Coord3& p0 = ps.get( ret ).pos();
    double minsqdist = p0.isUdf() ? mUdf(double)
		     : (ignorez ? pos.sqHorDistTo( p0 ) : pos.sqDistTo( p0 ));

    for ( typename PicksType::size_type idx=1; idx<sz; idx++ )
    {
	const Coord3& curpos = ps.get( idx ).pos();
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


// Pick::Set
    mDefineEnumUtils( Pick::Set::Disp, Connection, "Connection" )
{ "None", "Open", "Close", 0 };

Set::Set( const char* nm )
    : NamedObject(nm)
    , pars_(*new IOPar)
{
    pars_.set( sKey::SurveyID(), TrcKey::cUndefSurvID() );
}


Set::Set( const Set& s )
    : pars_(*new IOPar)
{
    *this = s;
}

Set::~Set()
{
    delete &pars_;
}


Set& Set::operator=( const Set& s )
{
    if ( &s == this ) return *this;
    copy( s ); setName( s.name() );
    disp_ = s.disp_; pars_ = s.pars_;
    return *this;
}


Pos::SurvID Set::getSurvID() const
{
    Pos::SurvID survid( TrcKey::cUndefSurvID() );
    return pars_.get(sKey::SurveyID(),survid) ? survid
	 : Pick::getSurvID( *this );
}


bool Set::is2D() const
{
    return Pick::is2D( *this );
}


bool Set::isPolygon() const
{
    const FixedString typ = pars_.find( sKey::Type() );
    return typ.isEmpty() ? disp_.connect_!=Set::Disp::None
			 : typ == sKey::Polygon();
}


void Set::getPolygon( ODPolygon<double>& poly ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord c( (*this)[idx].pos_ );
	poly.add( Geom::Point2D<double>( c.x, c.y ) );
    }
}


void Set::getLocations( ObjectSet<Location>& locs )
{
    for ( int idx=0; idx<size(); idx++ )
	locs += &((*this)[idx]);
}


void Set::getLocations( ObjectSet<const Location>& locs ) const
{
    for ( int idx=0; idx<size(); idx++ )
	locs += &((*this)[idx]);
}


float Set::getXYArea() const
{
    if ( size()<3 || disp_.connect_==Set::Disp::None )
	return mUdf(float);

    TypeSet<Geom::Point2D<float> > posxy;
    for ( int idx=size()-1; idx>=0; idx-- )
    {
	const Coord localpos = (*this)[idx].pos_;
	posxy += Geom::Point2D<float>(( float )localpos.x,( float )localpos.y);
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
    return findIdx( *this, tk );
}


Pick::Set::size_type Pick::Set::nearestLocation( const Coord& pos ) const
{
    return getNearestLocation( *this, Coord3(pos.x,pos.y,0.f), true );
}


Pick::Set::size_type Pick::Set::nearestLocation( const Coord3& pos,
						 bool ignorez ) const
{
    return getNearestLocation( *this, pos, ignorez );
}


void Set::fillPar( IOPar& par ) const
{
    BufferString colstr;
    if ( disp_.color_ != Color::NoColor() )
    {
	disp_.color_.fill( colstr );
	par.set( sKey::Color(), colstr );
    }

    par.set( sKey::Size(), disp_.pixsize_ );
    par.set( sKeyMarkerType(), disp_.markertype_ );
    par.set( sKeyConnect, Disp::getConnectionString(disp_.connect_) );

    par.merge( pars_ );
}


bool Set::usePar( const IOPar& par )
{
    BufferString colstr;
    if ( par.get(sKey::Color(),colstr) )
	disp_.color_.use( colstr.buf() );
    else
	disp_.color_ = Color::Red(); // change default color from none to red
    disp_.pixsize_ = 3;
    par.get( sKey::Size(), disp_.pixsize_ );
    par.get( sKeyMarkerType(), disp_.markertype_ );
    bool doconnect;
    par.getYN( sKeyConnect, doconnect );	// For Backward Compatibility
    if ( doconnect ) disp_.connect_ = Disp::Close;
    else
    {
	if ( !Disp::parseEnumConnection(par.find(sKeyConnect), disp_.connect_) )
	    disp_.connect_ = Disp::None;
    }

    pars_ = par;
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::Size() );
    pars_.removeWithKey( sKeyMarkerType() );
    pars_.removeWithKey( sKeyConnect );

    return true;
}


void Set::addUndoEvent( EventType type, int idx, const Pick::Location& loc )
{
    Pick::SetMgr& mgr = Pick::Mgr();
    if ( mgr.indexOf(*this) == -1 )
	return;

    const MultiID mid = mgr.get(*this);
    if ( !mid.isEmpty() )
    {
	PickSetKnotUndoEvent::UnDoType undotype =
	    (PickSetKnotUndoEvent::UnDoType) type;

	const Pick::Location pos = type == Insert
				   ? Coord3::udf()
				   : loc;
	PickSetKnotUndoEvent* undo = new PickSetKnotUndoEvent(
	undotype, mid, idx, pos );
	Pick::Mgr().undo().addEvent( undo, 0 );
    }
}


void Set::insertWithUndo( int idx, const Pick::Location& loc )
{
    insert( idx, loc );
    addUndoEvent( Insert, idx, loc );
 }


void Set::appendWithUndo( const Pick::Location& loc )
{
    *this += loc;
    addUndoEvent( Insert, size()-1, loc );
 }


void Set::removeSingleWithUndo( int idx )
{
    const Pick::Location pos = (*this)[idx];
    addUndoEvent( Remove, idx, pos );
    removeSingle( idx );
}


void Set::moveWithUndo( int idx, const Pick::Location& undoloc,
    const Pick::Location& loc )
{
    if ( size()<idx ) return;
    (*this)[idx] = undoloc;
    addUndoEvent( Move, idx, loc );
    (*this)[idx] = loc;
}


} // namespace Pick


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

	ps += Pick::Location( pos, zread );
    }

    return true;
}


Pick::List& Pick::List::add( const Location& loc, bool mkcopy )
{
    if ( mkcopy )
	*this += new Location( loc );
    else
	*this += const_cast<Location*>( &loc );
    return *this;
}


Pick::List::size_type Pick::List::find( const TrcKey& tk ) const
{
    return findIdx( *this, tk );
}


Pick::List::size_type Pick::List::nearestLocation( const Coord& pos ) const
{
    return getNearestLocation( *this, Coord3(pos.x,pos.y,0.f), true );
}


Pick::List::size_type Pick::List::nearestLocation( const Coord3& pos,
						 bool ignorez ) const
{
    return getNearestLocation( *this, pos, ignorez );
}
