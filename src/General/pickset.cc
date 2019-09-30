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
#include "settings.h"
#include "keystrs.h"
#include <ctype.h>

mDefineInstanceCreatedNotifierAccess(Pick::Set)
const Pick::Set Pick::Set::emptyset_;
Pick::Set Pick::Set::dummyset_;

int Pick::Set::getSizeThreshold()
{
    int defthresholdval = 10000;
    Settings::common().get( sKeyThresholdSize(),defthresholdval );
    return defthresholdval;
}


#define mPrepRead(sz) \
    mLock4Read(); \
    const size_type sz = locs_.size()
#define mPrepLocChange(id) \
    mSendChgNotif( cLocationPreChange(), id.getI() ); \
    mReLock(); mLock2Write()
#define mSendLocChgNotif(istmp,id) \
    mSendChgNotif( istmp?cLocationChangeTemp():cLocationChange(), id.getI() )


mDefineEnumUtils( Pick::Set::Disp, Connection, "Connection" )
{ "None", "Open", "Close", 0 };
template<>
void EnumDefImpl<Pick::Set::Disp::Connection>::init()
{
    uistrings_ += uiStrings::sNone();
    uistrings_ += mEnumTr("Open","Display State, like Open Polygon");
    uistrings_ += mEnumTr("Close","Display State, like Close Polygon");
}

Pick::Set::Set( const char* nm, const char* cat )
    : SharedObject(nm)
    , curlocidnr_(0)
    , curlabelidnr_(0)
{
    if ( cat )
	setCategory( cat );
    mTriggerInstanceCreatedNotifier();
}


Pick::Set::Set( const Set& oth )
    : SharedObject(oth)
    , curlocidnr_(oth.curlocidnr_)
    , curlabelidnr_(oth.curlabelidnr_)
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
    grouplabels_ = oth.grouplabels_;
    curlocidnr_ = oth.curlocidnr_;
    curlabelidnr_ = oth.curlabelidnr_;
}


Monitorable::ChangeType Pick::Set::compareClassData( const Set& oth ) const
{
    if ( locs_ != oth.locs_ )
	return cEntireObjectChange();

    mStartMonitorableCompare();
    mHandleMonitorableCompare( disp_, cDispChange() );
    mHandleMonitorableCompare( pars_, cParsChange() );
    mHandleMonitorableCompare( grouplabels_, cGroupLabelsChange() );
    mDeliverMonitorableCompare();
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


bool Pick::Set::validIdx( idx_type idx ) const
{
    mLock4Read();
    return locs_.validIdx( idx );
}


Pick::Set::idx_type Pick::Set::gtIdxFor( LocID id ) const
{
    if ( id.isValid() )
    {
	const size_type sz = locs_.size();

	if ( id.getI() < sz && locids_[id.getI()].getI() == id.getI() )
	    return id.getI();

	for ( idx_type idx=0; idx<sz; idx++ )
	    if ( locids_[idx] == id )
		return idx;
    }
    return -1;
}


Pick::Set::idx_type Pick::Set::idxFor( LocID id ) const
{
    mLock4Read();
    return gtIdxFor( id );
}


Pick::Set::LocID Pick::Set::locIDFor( idx_type idx ) const
{
    mLock4Read();
    return locids_.validIdx(idx) ? locids_[idx] : LocID::getInvalid();
}


Pick::Location Pick::Set::get( LocID id ) const
{
    mLock4Read();
    const idx_type idx = gtIdxFor( id );
    return idx != -1 ? locs_[idx] : Location::udf();
}


Pick::Location Pick::Set::first() const
{
    mLock4Read();
    return locs_.isEmpty() ? Location::udf() : locs_[0];
}


Pick::Location Pick::Set::getByIndex( idx_type idx ) const
{
    mLock4Read();
    return locs_.validIdx(idx) ? locs_[idx] : Location::udf();
}


Coord Pick::Set::getPos( LocID id ) const
{
    mLock4Read();
    const idx_type idx = gtIdxFor( id );
    return idx != -1 ? Coord(locs_[idx].pos().getXY()) : Coord::udf();
}


double Pick::Set::getZ( LocID id ) const
{
    mLock4Read();
    const idx_type idx = gtIdxFor( id );
    return idx != -1 ? locs_[idx].pos().z_ : mUdf(double);
}


bool Pick::Set::isPolygon(const IOPar& par , bool doconnect)
{
    const FixedString typ = par.find( sKey::Type() );
    if( typ.isEmpty() )
	return doconnect != Set::Disp::None;
    return typ == sKey::Polygon();
}


bool Pick::Set::isPolygon() const
{
    mLock4Read();

    return isPolygon( pars_ , disp_.connect_);
}


void Pick::Set::setIsPolygon( bool yn )
{
    mLock4Read();
    const FixedString typ = pars_.find( sKey::Type() );
    if ( (yn && typ == sKey::Polygon()) || (!yn && typ == sKey::PickSet()) )
	return;

    mLock2Write();
    pars_.set( sKey::Type(), yn ? sKey::Polygon() : sKey::PickSet() );
    mSendChgNotif( cParsChange(), cUnspecChgID() );
}


void Pick::Set::setCategory( const char* newcat )
{
    mLock4Read();
    const FixedString curcat = pars_.find( sKey::Category() );
    if ( curcat == newcat )
	return;

    mLock2Write();
    pars_.update( sKey::Category(), newcat );
    mSendChgNotif( cParsChange(), cUnspecChgID() );
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


#define mDefHaveFn(what,locfn) \
bool Pick::Set::have##what() const \
{ \
    mPrepRead( sz ); \
 \
    for ( int idx=0; idx<sz; idx++ ) \
	if ( locs_[idx].locfn() ) \
	    return true; \
 \
    return false; \
}

mDefHaveFn( Texts, hasText )
mDefHaveFn( Directions, hasDir )
mDefHaveFn( GroupLabels, groupLabelID().isValid )
mDefHaveFn( TrcKeys, hasTrcKey )

bool Pick::Set::haveMultipleGeomIDs() const
{
    mPrepRead( sz );
    if ( sz < 2 )
	return false;

    const Pos::GeomID geomid = locs_[0].geomID();
    for ( int idx=1; idx<sz; idx++ )
	if ( locs_[idx].geomID() != geomid )
	    return true;

    return false;
}


Pick::Set::GroupLabelID Pick::Set::addGroupLabel( const GroupLabel& lbl )
{
    mLock4Write();
    GroupLabel newlbl( lbl );
    newlbl.id_ = GroupLabelID::get( curlabelidnr_++ );
    grouplabels_ += newlbl;
    mSendChgNotif( cGroupLabelsChange(), newlbl.id_.getI() );
    return newlbl.id_;
}


int Pick::Set::gtLblIdx( GroupLabelID lblid ) const
{
    if ( lblid.isInvalid() )
	return -1;

    const int sz = grouplabels_.size();
    for ( int idx=0; idx<sz; idx++ )
	if ( grouplabels_[idx].id() == lblid )
	    return idx;

    return -1;
}


void Pick::Set::getGroupLabelIDs( TypeSet<GroupLabelID>& gids,
				 bool onlyused ) const
{
    mLock4Read();
    if ( onlyused )
    {
	const int sz = locs_.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const GroupLabelID gid = locs_[idx].groupLabelID();
	    if ( gid.isValid() && !gids.isPresent(gid) )
		gids += gid;
	}
    }
    else
    {
	const int sz = grouplabels_.size();
	for ( int idx=0; idx<sz; idx++ )
	    gids += grouplabels_[idx].id();
    }
}


void Pick::Set::removeGroupLabel( GroupLabelID lblid )
{
    mLock4Write();
    const int idxof = gtLblIdx( lblid );
    if ( idxof >= 0 )
    {
	grouplabels_.removeSingle( idxof );
	mSendChgNotif( cGroupLabelsChange(), grouplabels_[idxof].id_.getI() );
    }
}


Pick::GroupLabel Pick::Set::getGroupLabel( GroupLabelID lblid ) const
{
    mLock4Read();
    const int idxof = gtLblIdx( lblid );
    return idxof < 0 ? GroupLabel() : grouplabels_[idxof];
}


Pick::Set::GroupLabelID Pick::Set::getGroupLabelByText(
						const char* lbltxt ) const
{
    mLock4Read();
    const int sz = grouplabels_.size();
    for ( int idx=0; idx<sz; idx++ )
	if ( grouplabels_[idx].text() == lbltxt )
	    return grouplabels_[idx].id();
    return GroupLabelID::getInvalid();
}


void Pick::Set::setGroupLabel( const GroupLabel& lbl )
{
    mLock4Read();
    const int idxof = gtLblIdx( lbl.id() );
    if ( idxof < 0 )
    {
	mUnlockAllAccess();
	addGroupLabel( lbl );
	return;
    }

    mLock2Write();
    grouplabels_[idxof] = lbl;
    mSendChgNotif( cGroupLabelsChange(), lbl.id_.getI() );
}


Pick::Set::GroupLabelID Pick::Set::groupLabelID( LocID locid ) const
{
    mLock4Read();
    const idx_type locidx = gtIdxFor( locid );
    return locidx < 0 ? GroupLabelID::getInvalid()
		      : locs_[locidx].groupLabelID();
}


Pick::Set::GroupLabelID Pick::Set::groupLabelIDByIdx( idx_type locidx ) const
{
    mLock4Read();
    return locidx < 0 ? GroupLabelID::getInvalid()
		      : locs_[locidx].groupLabelID();
}


void Pick::Set::setGroupLabelID( LocID locid, GroupLabelID labelid )
{
    mLock4Read();
    idx_type locidx = gtIdxFor( locid );
    if ( locidx < 0 || locs_[locidx].groupLabelID() == labelid )
	return;

    if ( !mLock2Write() )
    {
	locidx = gtIdxFor( locid );
	if ( locidx < 0 || locs_[locidx].groupLabelID() == labelid )
	    return;
    }

    mPrepLocChange( locid );
    locs_[locidx].setGroupLabelID( labelid );
    mSendLocChgNotif( false, locid );
}


void Pick::Set::setGroupLabelIDs( Interval<idx_type> idxs, GroupLabelID labelid)
{
    mLock4Write();
    idxs.sort();
    while ( idxs.start <= idxs.stop )
    {
	locs_[idxs.start].setGroupLabelID( labelid );
	idxs.start++;
    }
    mSendEntireObjChgNotif();
}


bool Pick::Set::isMultiGeom() const
{
    mPrepRead( sz );
    if ( sz < 2 )
	return false;
    const Pos::GeomID geomid0 = locs_[0].geomID();
    for ( idx_type idx=1; idx<sz; idx++ )
	if ( locs_[idx].geomID() != geomid0 )
	    return true;
    return false;
}


Pos::GeomID Pick::Set::firstGeomID() const
{
    mLock4Read();
    return locs_.isEmpty() ? Pos::GeomID::get3D() : locs_[0].trcKey().geomID();
}


bool Pick::Set::has2D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return false;
    for ( idx_type idx=0; idx<sz; idx++ )
	if ( locs_[idx].is2D() )
	    return true;
    return false;
}


bool Pick::Set::has3D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return true;
    for ( idx_type idx=0; idx<sz; idx++ )
	if ( !locs_[idx].is2D() )
	    return true;
    return false;
}


bool Pick::Set::hasOnly2D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return false;
    for ( idx_type idx=0; idx<sz; idx++ )
	if ( !locs_[idx].is2D() )
	    return false;
    return true;
}


bool Pick::Set::hasOnly3D() const
{
    mPrepRead( sz );
    if ( sz < 1 )
	return true;
    for ( idx_type idx=0; idx<sz; idx++ )
	if ( locs_[idx].is2D() )
	    return false;
    return true;
}


void Pick::Set::getPolygon( ODPolygon<double>& poly ) const
{
    mPrepRead( sz );
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	const Coord coord( locs_[idx].pos().getXY() );
	poly.add( Geom::Point2D<double>( coord.x_, coord.y_ ) );
    }
}


void Pick::Set::getPolygon( ODPolygon<float>& poly ) const
{
    mPrepRead( sz );
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	Coord coord( locs_[idx].pos().getXY() );
	coord = SI().binID2Coord().transformBackNoSnap( coord );
	poly.add( Geom::Point2D<float>( (float)coord.x_, (float)coord.y_ ) );
    }
}


void Pick::Set::getLocations( TypeSet<Coord>& coords ) const
{
    mPrepRead( sz );
    for ( idx_type idx=0; idx<sz; idx++ )
	coords += locs_[idx].pos().getXY();
}


float Pick::Set::getXYArea() const
{
    mPrepRead( sz );
    if ( sz < 3 || disp_.connect_ == Disp::None )
	return mUdf(float);

    TypeSet<Geom::Point2D<float> > posxy;
    for ( idx_type idx=sz-1; idx>=0; idx-- )
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
    for ( idx_type idx=0; idx<sz; idx++ )
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
    idx_type idx = 0;
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


bool Pick::Set::isSizeLargerThanThreshold() const
{
    bool usethreshold = true;
    Settings::common().getYN( sKeyUseThreshold(), usethreshold );
    return usethreshold && size() >= getSizeThreshold();
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



void Pick::Set::fillDisplayPar( IOPar& par ) const
{
    mLock4Read();
    par.merge( disppars_ );
    BufferString mkparstr,lnparstr;
    disp_.mkstyle_.toString( mkparstr );
    disp_.lnstyle_.toString( lnparstr );
    par.set( sKeyLineStyle(), lnparstr );
    par.set( sKeyLineType(), disp_.lnstyle_.type_ );
    par.set( sKey::MarkerStyle(), mkparstr );
    par.set( sKeyConnect(), Disp::toString(disp_.connect_) );
    par.setYN( sKeyFill(), disp_.filldodraw_ );
    par.setYN( sKeyLine(), disp_.linedodraw_ );
}


void Pick::Set::fillPar( IOPar& par ) const
{
    mLock4Read();
    par.merge( pars_ );
    par.merge( disppars_ );
    if ( !grouplabels_.isEmpty() )
    {
	for ( int igrp=0; igrp<grouplabels_.size(); igrp++ )
	{
	    const GroupLabelID grplblid = grouplabels_[igrp].id();
	    bool present = false;
	    for ( int iloc=0; iloc<locs_.size(); iloc++ )
	    {
		if ( locs_[iloc].groupLabelID() == grplblid )
		    { present = true; break; }
	    }
	    if ( present )
		grouplabels_[igrp].fillPar( par, igrp );
	}
    }
}



bool Pick::Set::usePar( const IOPar& par )
{
    IOPar mergedpar( par );
    if( pars_.isPresent(sKey::Type()) )
    {
	BufferString typ;
	pars_.get( sKey::Type(),typ );
	mergedpar.set( sKey::Type(),typ );
    }
    mLock4Write();

    BufferString mkststr;
    if ( par.get(sKey::MarkerStyle(),mkststr) )
	disp_.mkstyle_.fromString( mkststr );
    else
    {
	BufferString colstr,lncolstr;
	if ( par.get(sKey::Color(),colstr) )
	    disp_.mkstyle_.color_.use( colstr.buf() );

	if ( par.get(sKeyLineColor(),lncolstr) )
	    disp_.lnstyle_.color_.use( lncolstr.buf() );

	par.get( sKey::Size(),disp_.mkstyle_.size_ );
	par.get( sKeyWidth(),disp_.lnstyle_.width_ );
	int pstype = 0;
	par.get( sKeyMarkerType(), pstype );
	pstype++;
	disp_.mkstyle_.type_ = (OD::MarkerStyle3D::Type)pstype;
    }
    BufferString lnststr;
    if ( par.get(sKeyLineStyle(),lnststr) )
	disp_.lnstyle_.fromString( lnststr );
    else
    {
	BufferString lncolstr;
	if ( par.get(sKeyLineColor(),lncolstr) )
	    disp_.lnstyle_.color_.use( lncolstr.buf() );
	par.get( sKeyWidth(),disp_.lnstyle_.width_ );
	int pstype = 0;
        par.get( sKeyLineType(), pstype );
        pstype++;
        disp_.lnstyle_.type_ = (OD::LineStyle::Type)pstype;
    }


    par.getYN( sKeyFill(), disp_.filldodraw_ );
    par.getYN( sKeyLine(), disp_.linedodraw_ );

    bool doconnect = true;
    if ( par.getYN( sKeyConnect(), doconnect ) )
    {
	if ( doconnect )
	    disp_.connect_ = Disp::Close;
	else if ( !Disp::ConnectionDef().parse(par.find(sKeyConnect()),
					   disp_.connect_) )
	    disp_.connect_ = Disp::None;
    }
    else
    {
	if( isPolygon( mergedpar, disp_.connect_ ) )
	    disp_.connect_ = Disp::Close;
    }


    for ( int grpnr=0; ; grpnr++ )
    {
	GroupLabel glbl;
	if ( !glbl.usePar(par,grpnr) )
	    break;
	grouplabels_ += glbl;
    }

    pars_ = par;
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::Size() );
    pars_.removeWithKey( sKeyMarkerType() );
    pars_.removeWithKey( sKeyLineType() );
    pars_.removeWithKey( sKeyWidth() );
    pars_.removeWithKey( sKeyConnect() );
    pars_.removeWithKey( sKeyFill() );
    pars_.removeWithKey( sKeyLine() );
    GroupLabel::removeFromPar( pars_ );

    mSendEntireObjChgNotif();
    return true;
}


void Pick::Set::updateInPar( const char* ky, const char* val )
{
    mLock4Write();
    pars_.update( ky, val );
    mSendChgNotif( cParsChange(), cUnspecChgID() );
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


Pick::Set::LocID Pick::Set::insNewLocID( idx_type idx,
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
    idx_type idx = gtIdxFor( id );
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


Pick::Set& Pick::Set::setByIndex( idx_type idx, const Location& loc, bool istmp)
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
    idx_type targetidx = -1;
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


static inline bool coordUnchanged( Pick::Set::idx_type idx,
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



static inline bool zUnchanged( Pick::Set::idx_type idx,
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


void Pick::Set::dumpLocations( od_ostream* strm ) const
{
    SetIter it( *this );
    if ( !strm )
	strm = &od_cout();
    while ( it.next() )
    {
	*strm << it.getPos().x_ << ' ';
	*strm << it.getPos().y_ << ' ';
	*strm << it.getZ() << od_endl;
    }
}


// Pick::SetIter

Pick::SetIter::SetIter( const Set& ps, bool atend )
    : MonitorableIter4Read<Pick::Set::idx_type>( ps,
	    atend?ps.size()-1:0, atend?0:ps.size()-1 )
{
}


Pick::SetIter::SetIter( const SetIter& oth )
    : MonitorableIter4Read<Pick::Set::idx_type>(oth)
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
    : MonitorableIter4Write<Set::idx_type>(ps,
	    atend?ps.size()-1:0, atend?0:ps.size()-1 )
{
}


Pick::SetIter4Edit::SetIter4Edit( const SetIter4Edit& oth )
    : MonitorableIter4Write<Set::idx_type>(oth)
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
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true, false, true );
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

	Coord pos( getPos(0, 1) );
	if ( pos.isUdf() )
	    continue;
	mPIEPAdj(Coord,pos,true);
	if ( !isXY() || !SI().isReasonable(pos) )
	{
	    BinID bid( mNINT32(pos.x_), mNINT32(pos.y_) );
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
    : Presentation::ObjInfo(key)
{
    objtypekey_ = sFactoryKey();
}


Pick::SetPresentationInfo::SetPresentationInfo()
{
    objtypekey_ = sFactoryKey();
}


Presentation::ObjInfo* Pick::SetPresentationInfo::createFrom(
						const IOPar& par )
{
    Pick::SetPresentationInfo* psprinfo = new Pick::SetPresentationInfo;
    if ( !psprinfo->usePar(par) )
	{ delete psprinfo; psprinfo = 0; }

    return psprinfo;
}


const char* Pick::SetPresentationInfo::sFactoryKey()
{
    return sKey::PickSet();
}


void Pick::SetPresentationInfo::initClass()
{
    OD::PrIFac().addCreateFunc( createFrom, sFactoryKey() );
}
