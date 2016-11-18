/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/

#include "picksetascio.h"
#include "pickset.h"
#include "polygon.h"
#include "survinfo.h"
#include "tabledef.h"
#include "posimpexppars.h"
#include "unitofmeasure.h"
#include "od_iostream.h"
#include <ctype.h>

mDefineInstanceCreatedNotifierAccess(Pick::Set)
const Pick::Set Pick::Set::emptyset_;
Pick::Set Pick::Set::dummyset_;
static const char* sKeyConnect = "Connect";


mDefineEnumUtils( Pick::Set::Disp, Connection, "Connection" )
{ "None", "Open", "Close", 0 };

Pick::Set::Set( const char* nm, const char* cat )
    : SharedObject(nm)
    , curlocidnr_(0)
{
    setCategory( cat );
    mTriggerInstanceCreatedNotifier();
}


Pick::Set::Set( const Set& oth )
    : SharedObject(oth)
    , curlocidnr_(oth.curlocidnr_)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Pick::Set::~Set()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Pick::Set, SharedObject )

void Pick::Set::copyClassData( const Set& oth )
{
    locs_ = oth.locs_;
    locids_ = oth.locids_;
    disp_ = oth.disp_;
    pars_ = oth.pars_;
    curlocidnr_ = oth.curlocidnr_;
}


Pick::Set::size_type Pick::Set::size() const
{
    mLock4Read();
    return locs_.size();
}


bool Pick::Set::validLocID( LocID id ) const
{
    mLock4Read();
    return gtIdxFor( id ) != -1;
}


bool Pick::Set::validIdx( IdxType idx ) const
{
    mLock4Read();
    return locs_.validIdx( idx );
}


Pick::Set::IdxType Pick::Set::gtIdxFor( LocID id ) const
{
    if ( id.isValid() )
    {
	const size_type sz = locs_.size();

	if ( id.getI() < sz && locids_[id.getI()].getI() == id.getI() )
	    return id.getI();

	for ( IdxType idx=0; idx<sz; idx++ )
	    if ( locids_[idx] == id )
		return idx;
    }
    return -1;
}


Pick::Set::IdxType Pick::Set::idxFor( LocID id ) const
{
    mLock4Read();
    return gtIdxFor( id );
}


Pick::Set::LocID Pick::Set::locIDFor( IdxType idx ) const
{
    mLock4Read();
    return locids_.validIdx(idx) ? locids_[idx] : LocID::getInvalid();
}


Pick::Location Pick::Set::get( LocID id ) const
{
    mLock4Read();
    const IdxType idx = gtIdxFor( id );
    return idx != -1 ? locs_[idx] : Location::udf();
}


Pick::Location Pick::Set::first() const
{
    mLock4Read();
    return locs_.isEmpty() ? Location::udf() : locs_[0];
}


Pick::Location Pick::Set::getByIndex( IdxType idx ) const
{
    mLock4Read();
    return locs_.validIdx(idx) ? locs_[idx] : Location::udf();
}


Coord Pick::Set::getPos( LocID id ) const
{
    mLock4Read();
    const IdxType idx = gtIdxFor( id );
    return idx != -1 ? Coord(locs_[idx].pos().getXY()) : Coord::udf();
}


double Pick::Set::getZ( LocID id ) const
{
    mLock4Read();
    const IdxType idx = gtIdxFor( id );
    return idx != -1 ? locs_[idx].pos().z_ : mUdf(double);
}


bool Pick::Set::isPolygon() const
{
    mLock4Read();

    const FixedString typ = pars_.find( sKey::Type() );
    if ( typ.isEmpty() )
	return disp_.connect_ != Set::Disp::None;

    return typ == sKey::Polygon();
}


void Pick::Set::setIsPolygon( bool yn )
{
    mLock4Read();
    const FixedString typ = pars_.find( sKey::Type() );
    if ( (yn && typ == sKey::Polygon()) || (!yn && typ == sKey::PickSet()) )
	return;

    mLock2Write();
    pars_.set( sKey::Type(), yn ? sKey::Polygon() : sKey::PickSet() );
    mSendChgNotif( cDispChange(), 0 );
}


void Pick::Set::setCategory( const char* newcat )
{
    mLock4Read();
    const FixedString curcat = pars_.find( sKey::Category() );
    if ( curcat == newcat )
	return;

    mLock2Write();
    pars_.update( sKey::Category(), newcat );
    mSendChgNotif( cDispChange(), 0 );
}


BufferString Pick::Set::type() const
{
    mLock4Read();
    return BufferString( pars_.find( sKey::Type() ) );
}


BufferString Pick::Set::category() const
{
    mLock4Read();
    return BufferString( pars_.find( sKey::Category() ) );
}


#define mPrepRead(sz) \
    mLock4Read(); \
    const size_type sz = locs_.size()


bool Pick::Set::isMultiGeom() const
{
    mPrepRead( sz );
    if ( sz < 2 )
	return false;
    const Pos::GeomID geomid0 = locs_[0].geomID();
    for ( IdxType idx=1; idx<sz; idx++ )
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
    for ( IdxType idx=0; idx<sz; idx++ )
	if ( locs_[idx].is2D() )
	    return true;
    return false;
}


bool Pick::Set::has3D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return true;
    for ( IdxType idx=0; idx<sz; idx++ )
	if ( !locs_[idx].is2D() )
	    return true;
    return false;
}


bool Pick::Set::hasOnly2D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return false;
    for ( IdxType idx=0; idx<sz; idx++ )
	if ( !locs_[idx].is2D() )
	    return false;
    return true;
}


bool Pick::Set::hasOnly3D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return true;
    for ( IdxType idx=0; idx<sz; idx++ )
	if ( locs_[idx].is2D() )
	    return false;
    return true;
}


void Pick::Set::getPolygon( ODPolygon<double>& poly ) const
{
    mPrepRead( sz );
    for ( IdxType idx=0; idx<sz; idx++ )
    {
	const Coord coord( locs_[idx].pos().getXY() );
	poly.add( Geom::Point2D<double>( coord.x_, coord.y_ ) );
    }
}


void Pick::Set::getPolygon( ODPolygon<float>& poly ) const
{
    mPrepRead( sz );
    for ( IdxType idx=0; idx<sz; idx++ )
    {
	Coord coord( locs_[idx].pos().getXY() );
	coord = SI().binID2Coord().transformBackNoSnap( coord );
	poly.add( Geom::Point2D<float>( (float)coord.x_, (float)coord.y_ ) );
    }
}


void Pick::Set::getLocations( TypeSet<Coord>& coords ) const
{
    mPrepRead( sz );
    for ( IdxType idx=0; idx<sz; idx++ )
	coords += locs_[idx].pos().getXY();
}


float Pick::Set::getXYArea() const
{
    mPrepRead( sz );
    if ( sz < 3 || disp_.connect_ == Disp::None )
	return mUdf(float);

    TypeSet<Geom::Point2D<float> > posxy;
    for ( IdxType idx=sz-1; idx>=0; idx-- )
    {
	const Coord localpos = locs_[idx].pos().getXY();
	posxy += Geom::Point2D<float>( (float)localpos.x_, (float)localpos.y_ );
    }

    ODPolygon<float> polygon( posxy );
    if ( polygon.isSelfIntersecting() )
	return mUdf(float);

    float area = (float) polygon.area();
    if ( SI().xyInFeet() )
	area *= (mFromFeetFactorF*mFromFeetFactorF);

    return area;
}


Pick::Set::LocID Pick::Set::find( const TrcKey& tk ) const
{
    mPrepRead( sz );
    for ( IdxType idx=0; idx<sz; idx++ )
	if ( locs_[idx].trcKey() == tk )
	    return locids_[idx];
    return LocID::getInvalid();
}


Pick::Set::LocID Pick::Set::nearestLocation( const Coord& pos ) const
{
    return nearestLocation( Coord3(pos.x_,pos.y_,0.f), true );
}


Pick::Set::LocID Pick::Set::nearestLocation( const Coord3& pos,
					     bool ignorez ) const
{
    mPrepRead( sz );
    if ( sz < 2 )
	return sz < 1 ? LocID::getInvalid() : locids_[0];
    if ( pos.isUdf() )
	return LocID::getInvalid();

    LocID ret = locids_[0];
    IdxType idx = 0;
    const Coord3& p0 = locs_[idx].pos();
    double minsqdist = p0.isUdf()
	? mUdf(double)
	: (ignorez ? pos.xySqDistTo( p0 ) : pos.sqDistTo( p0 ));
    if ( minsqdist == 0 )
	return ret;

    for ( idx=1; idx<sz; idx++ )
    {
	const Coord3& curpos = locs_[idx].pos();
	if ( pos.isUdf() )
	    continue;

	const double sqdist = ignorez ? pos.xySqDistTo( curpos )
				      : pos.sqDistTo( curpos );
	if ( sqdist == 0 )
	    return locids_[idx];
	else if ( sqdist < minsqdist )
	    { minsqdist = sqdist; ret = locids_[idx]; }
    }
    return ret;
}


bool Pick::Set::removeWithPolygon( const ODPolygon<double>& wpoly, bool inside )
{
    RefMan<Pick::Set> workps = new Pick::Set( *this );
    Pick::SetIter4Edit psiter( *workps, true );

    int nrchgs = 0;
    while ( psiter.next() )
    {
	const Coord pos( psiter.get().pos().getXY() );
	if ( !pos.isDefined() || inside == wpoly.isInside(pos,true,1e-3) )
	    { psiter.removeCurrent(); nrchgs++; }
    }
    psiter.retire();

    if ( nrchgs < 1 )
	return false;

    *this = *workps;
    return true;
}



void Pick::Set::fillPar( IOPar& par ) const
{
    mLock4Read();
    par.merge( pars_ );
    BufferString parstr;
    disp_.mkstyle_.toString( parstr );
    par.set( sKey::MarkerStyle(), parstr );
    par.set( sKeyConnect, Disp::toString(disp_.connect_) );
}


bool Pick::Set::usePar( const IOPar& par )
{
    mLock4Write();

    BufferString mkststr;
    if ( par.get(sKey::MarkerStyle(),mkststr) )
	disp_.mkstyle_.fromString( mkststr );
    else
    {
	BufferString colstr;
	if ( par.get(sKey::Color(),colstr) )
	    disp_.mkstyle_.color_.use( colstr.buf() );
	par.get( sKey::Size(),disp_.mkstyle_.size_ );
	int pstype = 0;
	par.get( sKeyMarkerType(), pstype );
	pstype++;
	disp_.mkstyle_.type_ = (OD::MarkerStyle3D::Type)pstype;
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

    mLock2Write();
    if ( locs_.isEmpty() )
	return *this;

    locs_.setEmpty();
    locids_.setEmpty();

    mSendEntireObjChgNotif();
    return *this;
}


Pick::Set& Pick::Set::append( const Set& oth )
{
    if ( this != &oth && !oth.isEmpty() )
    {
	mLock4Write();
	MonitorLock monlock( oth );
	for ( int idx=0; idx<oth.locids_.size(); idx++ )
	{
	    locs_.add( oth.locs_[idx] );
	    const LocID newlocid = LocID::get( curlocidnr_++ );
	    locids_.add( newlocid );
	}
	monlock.unlockNow();
	mSendEntireObjChgNotif();
    }
    return *this;
}


Pick::Set::LocID Pick::Set::insNewLocID( IdxType idx,
				    AccessLocker& mAccessLocker() )
{
    const LocID newlocid = LocID::get( curlocidnr_++ );
    locids_.insert( idx, newlocid );

    mSendChgNotif( cLocationInsert(), newlocid.getI() );
    return newlocid;
}


Pick::Set::LocID Pick::Set::add( const Location& loc )
{
    mLock4Write();

    locs_ += loc;
    return insNewLocID( locids_.size(), mAccessLocker() );
}


Pick::Set::LocID Pick::Set::insertBefore( LocID id, const Location& loc )
{
    if ( !id.isValid() )
	return add( loc );

    mLock4Read();
    IdxType idx = gtIdxFor( id );
    if ( idx == -1 )
	return LocID::getInvalid();

    if ( !mLock2Write() )
    {
	idx = gtIdxFor( id );
	if ( idx == -1 )
	    return LocID::getInvalid();
    }

    locs_.insert( idx, loc );
    return insNewLocID( idx, mAccessLocker() );
}


#define mPrepLocChange(id) \
    mSendChgNotif( cLocationPreChange(), id.getI() ); \
    mReLock(); mLock2Write()
#define mSendLocChgNotif(istmp,id) \
    mSendChgNotif( istmp?cLocationChangeTemp():cLocationChange(), id.getI() )


Pick::Set& Pick::Set::set( LocID id, const Location& loc, bool istmp )
{
    mLock4Read();
    int idx = gtIdxFor( id );
    if ( idx == -1 || loc == locs_[idx] )
	return *this;

    mPrepLocChange( id );
    idx = gtIdxFor( id );
    if ( idx == -1 || loc == locs_[idx] )
	return *this;

    locs_[idx] = loc;

    mSendLocChgNotif( istmp, id );
    return *this;
}


Pick::Set& Pick::Set::setByIndex( IdxType idx, const Location& loc, bool istmp )
{
    mLock4Read();
    if ( !locs_.validIdx(idx) || loc == locs_[idx] )
	return *this;

    mPrepLocChange( locids_[idx] );
    if ( !locs_.validIdx(idx) || loc == locs_[idx] )
	return *this;

    locs_[idx] = loc;

    mSendLocChgNotif( istmp, locids_[idx] );
    return *this;
}


Pick::Set::LocID Pick::Set::locIDAfter( LocID id ) const
{
    mLock4Read();
    int idx = gtIdxFor( id );
    if ( idx == -1 || idx > locids_.size() - 2 )
	return LocID::getInvalid();

    return locids_[idx+1];
}



Pick::Set::LocID Pick::Set::remove( LocID id )
{
    mLock4Read();
    int idx = gtIdxFor( id );
    if ( idx == -1 )
	return LocID::getInvalid();

    mSendChgNotif( cLocationRemove(), id.getI() );
    mReLock();

    idx = gtIdxFor( id );
    if ( !mLock2Write() )
    {
	idx = gtIdxFor( id );
	if ( idx == -1 )
	    return LocID::getInvalid(); // notif has been sent 2x ... too bad
    }

    locs_.removeSingle( idx );
    locids_.removeSingle( idx );

    return idx < locids_.size()-1 ? locids_[idx+1] : LocID::getInvalid();
}


void Pick::Set::replaceID( LocID from, LocID to )
{
    mLock4Write();
    IdxType targetidx = -1;
    for ( int idx=0; idx<locids_.size(); idx++ )
    {
	const LocID locid = locids_[idx];
	if ( locid == to )
	    { pErrMsg("Attempt to replace with existing ID"); return; }
	else if ( locid == from )
	    targetidx = idx;
    }

    if ( targetidx >= 0 )
	locids_[targetidx] = to;
}


static inline bool coordUnchanged( Pick::Set::IdxType idx,
	const TypeSet<Pick::Location>& locs, const Coord& coord )
{
    return idx == -1 || locs[idx].pos().xySqDistTo(coord) < 0.01;
}


Pick::Set& Pick::Set::setPos( LocID id, const Coord& coord, bool istmp )
{
    mLock4Read();
    int idx = gtIdxFor( id );
    if ( coordUnchanged(idx,locs_,coord) )
	return *this;

    mPrepLocChange( id );
    idx = gtIdxFor( id );
    if ( coordUnchanged(idx,locs_,coord) )
	return *this;

    locs_[idx].setPos( coord );

    mSendLocChgNotif( istmp, id );
    return *this;
}



static inline bool zUnchanged( Pick::Set::IdxType idx,
	const TypeSet<Pick::Location>& locs, double z )
{
    if ( idx == -1 )
	return true;
    const double zdiff = locs[idx].z() - z;
    return mIsZero(zdiff,1e-6);
}

Pick::Set& Pick::Set::setZ( LocID id, double z, bool istmp )
{
    mLock4Read();
    int idx = gtIdxFor( id );
    if ( zUnchanged(idx,locs_,z) )
	return *this;

    mPrepLocChange( id );
    idx = gtIdxFor( id );
    if ( zUnchanged(idx,locs_,z) )
	return *this;

    locs_[idx].setZ( z );

    mSendLocChgNotif( istmp, id );
    return *this;
}


// Pick::SetIter

Pick::SetIter::SetIter( const Set& ps, bool atend )
    : MonitorableIter4Read<Pick::Set::IdxType>( ps,
	    atend?ps.size()-1:0, atend?0:ps.size()-1 )
{
}


Pick::SetIter::SetIter( const SetIter& oth )
    : MonitorableIter4Read<Pick::Set::IdxType>(oth)
{
}


const Pick::Set& Pick::SetIter::pickSet() const
{
    return static_cast<const Pick::Set&>( monitored() );
}


Pick::Set::LocID Pick::SetIter::ID() const
{
    return isValid() ? pickSet().locIDFor( curidx_ ) : Set::LocID::getInvalid();
}


const Pick::Location& Pick::SetIter::get() const
{
    return isValid() ? pickSet().locs_[curidx_] : Pick::Location::udf();
}


Coord Pick::SetIter::getPos() const
{
    return isValid() ? Coord(pickSet().locs_[curidx_].pos().getXY())
		     : Coord::udf();
}


double Pick::SetIter::getZ() const
{
    return isValid() ? pickSet().locs_[curidx_].pos().z_ : mUdf(double);
}


// Pick::SetIter4Edit

Pick::SetIter4Edit::SetIter4Edit( Set& ps, bool atend )
    : MonitorableIter4Write(ps,
	    atend?ps.size()-1:0, atend?0:ps.size()-1 )
{
}


Pick::SetIter4Edit::SetIter4Edit( const SetIter4Edit& oth )
    : MonitorableIter4Write(oth)
{
}


Pick::Set& Pick::SetIter4Edit::pickSet()
{
    return static_cast<Pick::Set&>( edited() );
}


const Pick::Set& Pick::SetIter4Edit::pickSet() const
{
    return static_cast<const Pick::Set&>( monitored() );
}


Pick::Set::LocID Pick::SetIter4Edit::ID() const
{
    return isValid() ? pickSet().locIDFor( curidx_ ) : Set::LocID::getInvalid();
}


Pick::Location& Pick::SetIter4Edit::get() const
{
    return isValid()
	? const_cast<Location&>(pickSet().locs_[curidx_])
	: Pick::Location::dummy();
}


void Pick::SetIter4Edit::removeCurrent()
{
    if ( isValid() )
    {
	pickSet().locs_.removeSingle( curidx_ );
	currentRemoved();
    }
}


void Pick::SetIter4Edit::insert( const Location& loc )
{
    if ( isValid() )
    {
	pickSet().locs_.insert( curidx_, loc );
	insertedAtCurrent();
    }
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
    ChangeNotifyBlocker cnb( ps );

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


Pick::SetPresentationInfo::SetPresentationInfo( const DBKey& key )
    : OD::ObjPresentationInfo(key)
{
    objtypekey_ = sFactoryKey();
}


Pick::SetPresentationInfo::SetPresentationInfo()
    : OD::ObjPresentationInfo()
{
    objtypekey_ = sFactoryKey();
}


OD::ObjPresentationInfo* Pick::SetPresentationInfo::createFrom(
	const IOPar& par )
{
    Pick::SetPresentationInfo* psprinfo = new Pick::SetPresentationInfo;
    if ( !psprinfo->usePar(par) )
    {
	delete psprinfo;
	return 0;
    }

    return psprinfo;
}


const char* Pick::SetPresentationInfo::sFactoryKey()
{
    return sKey::PickSet();
}


void Pick::SetPresentationInfo::initClass()
{
    OD::PRIFac().addCreateFunc( createFrom, sFactoryKey() );
}
