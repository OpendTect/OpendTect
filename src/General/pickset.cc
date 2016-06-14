/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2001 / Mar 2016
-*/


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
    : NamedMonitorable(nm)
{
    setCategory( cat );
    mTriggerInstanceCreatedNotifier();
}


Pick::Set::Set( const Set& oth )
{
    copyAll( oth );
    mTriggerInstanceCreatedNotifier();
}


Pick::Set::~Set()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Pick::Set, NamedMonitorable )

void Pick::Set::copyClassData( const Set& oth )
{
    locs_.copy( oth.locs_ );
    disp_ = oth.disp_;
    pars_ = oth.pars_;
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
    mSendChgNotif( 0, mUdf(int) );
}


void Pick::Set::setCategory( const char* newcat )
{
    mLock4Read();
    const FixedString curcat = pars_.find( sKey::Category() );
    if ( curcat == newcat )
	return;

    mLock2Write();
    pars_.update( sKey::Category(), newcat );
    mSendChgNotif( 0, mUdf(int) );
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
	const Coord coord( locs_[idx].pos() );
	poly.add( Geom::Point2D<double>( coord.x, coord.y ) );
    }
}


void Pick::Set::getPolygon( ODPolygon<float>& poly ) const
{
    mPrepRead( sz );
    for ( size_type idx=0; idx<sz; idx++ )
    {
	Coord coord( locs_[idx].pos() );
	//TODO should pass a Geometry I presume
	coord = SI().binID2Coord().transformBackNoSnap( coord );
	poly.add( Geom::Point2D<float>( (float)coord.x, (float)coord.y ) );
    }
}


void Pick::Set::getLocations( TypeSet<Coord>& coords ) const
{
    mPrepRead( sz );
    for ( size_type idx=0; idx<sz; idx++ )
	coords += locs_[idx].pos();
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

    mSendEntireObjChgNotif();
    return *this;
}


Pick::Set& Pick::Set::append( const Set& oth )
{
    if ( this != &oth && !oth.isEmpty() )
    {
	mLock4Write();
	MonitorLock monlock( oth );
	locs_.append( oth.locs_ );
	monlock.unlockNow();
	mSendEntireObjChgNotif();
    }
    return *this;
}


Pick::Set& Pick::Set::add( const Location& loc )
{
    mLock4Write();

    locs_ += loc;

    mSendChgNotif( cLocationInsert(), locs_.size()-1 );
    return *this;
}


Pick::Set& Pick::Set::insert( size_type idx, const Location& loc )
{
    mLock4Write();

    locs_.insert( idx, loc );

    mSendChgNotif( cLocationInsert(), idx );
    return *this;
}


Pick::Set& Pick::Set::set( size_type idx, const Location& loc )
{
    mLock4Read();
    if ( !locs_.validIdx(idx) || loc == locs_[idx] )
	return *this;

    mLock2Write();
    if ( !locs_.validIdx(idx) )
	return *this;

    locs_[idx] = loc;

    mSendChgNotif( cLocationChange(), idx );
    return *this;
}


Pick::Set& Pick::Set::remove( size_type idx )
{
    mLock4Read();
    if ( !locs_.validIdx(idx) )
	return *this;

    // I guess there is no way to resolve this in current design
    // We need to send the notification in an unlocked state, so the receiver
    // can ask for the actual location. But, after the call we have no
    // guarantee that we are removing the same location.

    mSendChgNotif( cLocationRemove(), idx );
    accesslockhandler_.reLock();

    mLock2Write();
    if ( !locs_.validIdx(idx) )
	return *this;

    locs_.removeSingle( idx );

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

    mLock2Write();
    if ( coordUnchanged(idx,locs_,coord) )
	return *this;

    locs_[idx].setPos( coord );

    mSendChgNotif( cLocationChange(), idx );
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

    mLock2Write();
    if ( zUnchanged(idx,locs_,z) )
	return *this;

    locs_[idx].setZ( z );

    mSendChgNotif( cLocationChange(), idx );
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
