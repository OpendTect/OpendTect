/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "wellmarker.h"
#include "welltrack.h"
#include "wellextractdata.h"
#include "iopar.h"
#include "stratlevel.h"
#include "bufstringset.h"
#include "idxable.h"
#include "keystrs.h"

const char* Well::Marker::sKeyDah()	{ return "Depth along hole"; }

static const Well::Marker		udfmarker_("",mUdf(float));
const Well::Marker& Well::Marker::udf()	{ return udfmarker_; }
static Well::Marker			dummymarker_("~dummy~",0.f);
Well::Marker& Well::Marker::dummy()	{ return dummymarker_; }

mDefineInstanceCreatedNotifierAccess(Well::Marker);


Well::Marker::Marker( const char* nm, float dh, Color col )
    : NamedMonitorable(nm)
    , dah_(dh)
    , color_(col)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Marker::Marker( LevelID lvlid, ZType dh )
    : NamedMonitorable(BufferString("Level ",lvlid.getI()))
    , dah_(dh)
    , color_(Color::Black())
    , levelid_(lvlid)
{
    mTriggerInstanceCreatedNotifier();
}


Well::Marker::Marker( const Marker& oth )
    : NamedMonitorable(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::Marker::~Marker()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::Marker, NamedMonitorable )


void Well::Marker::copyClassData( const Marker& oth )
{
    dah_ = oth.dah_;
    color_ = oth.color_;
    levelid_ = oth.levelid_;
}


bool Well::Marker::operator ==( const Marker& oth ) const
{
    if ( this == &oth )
	return true;

    mLock4Read();
    const bool iseq = levelid_ == oth.levelid_
		      && gtName() == oth.gtName() && color_ == oth.color_
		      && dah_ == oth.dah_;


    return iseq;
}


bool Well::Marker::operator > ( const Marker& oth ) const
{
    mLock4Read();
    return dah_ > oth.dah_;
}


const OD::String& Well::Marker::name() const
{
    mDeclStaticString( lvnmret );
    mLock4Read();
    Strat::Level lvl = gtLevel();
    if ( !lvl.isUndef() )
    {
	lvnmret = lvl.name();
	return lvnmret;
    }

    return name_;
}


BufferString Well::Marker::getName() const
{
    mLock4Read();
    return gtName();
}


Color Well::Marker::color() const
{
    mLock4Read();
    const Strat::Level lvl = gtLevel();
    return lvl.isUndef() ? color_ : lvl.color();
}


Strat::Level Well::Marker::getLevel() const
{
    mLock4Read();
    return gtLevel();
}


BufferString Well::Marker::gtName() const
{
    const Strat::Level lvl = gtLevel();
    return lvl.isUndef() ? name_ : lvl.getName();
}


Strat::Level Well::Marker::gtLevel() const
{
    return Strat::LVLS().get( levelid_ );
}


void Well::Marker::setNoLevelID()
{
    setLevelID( LevelID::getInvalid() );
}


//Well::MarkerSet
mDefineInstanceCreatedNotifierAccess(Well::MarkerSet)

Well::MarkerSet::MarkerSet()
    : SharedObject()
{
    mTriggerInstanceCreatedNotifier();
}


Well::MarkerSet::MarkerSet( const MarkerSet& oth )
    : SharedObject(oth)
    , curmrkridnr_(oth.curmrkridnr_)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::MarkerSet::~MarkerSet()
{
    sendDelNotif();
}

mImplMonitorableAssignment( Well::MarkerSet, SharedObject )

void Well::MarkerSet::copyClassData( const MarkerSet& oth )
{
    markers_ = oth.markers_;
    markerids_ = oth.markerids_;
    curmrkridnr_ = oth.curmrkridnr_;
}


Well::MarkerSet::size_type Well::MarkerSet::size() const
{
    mLock4Read();
    return gtSize();
}


Well::MarkerSet::size_type Well::MarkerSet::gtSize() const
{
    return markers_.size();
}


Well::MarkerSet::MarkerID Well::MarkerSet::add( const Well::Marker& mrkr )
{
    return set( mrkr );
}


bool Well::MarkerSet::isEmpty() const
{
    mLock4Read();
    return markers_.isEmpty();
}


void Well::MarkerSet::setEmpty()
{
    mLock4Write();
    if ( markers_.isEmpty() )
	return;

    markers_.setEmpty(); markerids_.setEmpty();
    mSendEntireObjChgNotif();
}


Well::Marker Well::MarkerSet::getByIdx( IdxType idx ) const
{
    mLock4Read();
    return gtByIndex( idx );
}


Well::MarkerSet::MarkerID Well::MarkerSet::set( const Marker& newmrkr )
{
    mLock4Write();
    const IdxType idx = idxOf( newmrkr.name() );
    if ( idx < 0 )
    {
	insrtNew( newmrkr );
	const MarkerID newid = mrkrIDFor( idxOf(newmrkr.name()) );
	mSendChgNotif( cMarkerAdded(), newid.getI() );
	return newid;
    }

    Marker& oldmrkr = markers_[idx];
    if ( oldmrkr == newmrkr )
	return markerids_[idx];

    oldmrkr = newmrkr;

    const IdxType idxat = gtIdxForDah( newmrkr.dah() );
    if ( idxat != idx )
    {
	markers_.move( idx, idxat );
	markerids_.move( idx, idxat );
    }

    if ( idxat >= markerids_.size() )
	return  markerids_[idx];

    const MarkerID chgdid = markerids_[idxat];
    mSendChgNotif( cMarkerChanged(), chgdid.getI() );
    return chgdid;
}


void Well::MarkerSet::setNameByID( MarkerID mrkrid , const char* nm )
{
    mLock4Write();
    const IdxType idx = gtIdxFor( mrkrid );
    markers_[idx].setName( nm );
}


void Well::MarkerSet::setColor( MarkerID mrkrid , const Color& clr)
{
    mLock4Write();
    const IdxType idx = gtIdxFor( mrkrid );
    markers_[idx].setColor( clr );
}


void Well::MarkerSet::setDah( MarkerID id, float dah )
{
    const IdxType idx = getIdxFor( id );
    setDahByIdx( idx, dah );
}


Color Well::MarkerSet::getColor( MarkerID mrkrid ) const
{
    mLock4Read();
    const IdxType idx = gtIdxFor( mrkrid );
    return markers_[idx].color();
}


BufferString Well::MarkerSet::getNameByID( MarkerID mrkrid ) const
{
    mLock4Read();
    const IdxType idx = gtIdxFor( mrkrid );
    return markers_[idx].name();
}


float Well::MarkerSet::getDah( MarkerID mrkrid ) const
{
    mLock4Read();
    const IdxType idx = gtIdxFor( mrkrid );
    return markers_[idx].dah();
}


float Well::MarkerSet::getDahByIdx( IdxType idx ) const
{
    mLock4Read();
    return markers_[idx].dah();
}


void Well::MarkerSet::setDahByIdx( IdxType idx, float dah )
{
    mLock4Read();
    if ( idx < 0 || idx > gtSize()-1 )
	return;
    if ( markers_[idx].dah() == dah )
	return;

    IdxType dahidx = gtIdxForDah( dah );
    mLock2Write();
    markers_[idx].setDah( dah );
    if ( dahidx != idx )
    {
	markers_.move( idx, dahidx );
	markerids_.move( idx, dahidx );
    }

    mSendChgNotif( cMarkerChanged(), markerids_[dahidx].getI() );
}



float Well::MarkerSet::getDahFromMarkerName( const char* mname ) const
{
    mLock4Read();
    const IdxType idx = idxOf( mname );
    if ( idx<0 || !markers_.validIdx(idx) )
	return mUdf(float);
    return markers_[idx].dah();
}


Well::MarkerSet::MarkerID
    Well::MarkerSet::markerIDFromName( const char* mname ) const
{
    mLock4Read();
    const IdxType idx = idxOf( mname );
    if ( idx<0 || !markerids_.validIdx(idx) )
	return MarkerID::getInvalid();

    return markerids_[idx];
}


Well::Marker Well::MarkerSet::getByLvlID( LevelID lvid ) const
{
    mLock4Read();
    for ( IdxType idx=0; idx<gtSize(); idx++ )
    {
	if ( markers_[idx].levelID() == lvid )
	return markers_[idx];
    }

    return Marker::udf();
}


Well::Marker Well::MarkerSet::first() const
{
    mLock4Read();
    if ( markers_.isEmpty() )
	return Marker::udf();
    return markers_.first();
}


Well::Marker Well::MarkerSet::last() const
{
    mLock4Read();
    if ( !markers_.isEmpty() )
	return markers_.last();
    return Marker::udf();
}


Well::Marker Well::MarkerSet::gtByIndex( IdxType idx ) const
{
    return markers_[idx];
}


Well::Marker Well::MarkerSet::get( MarkerID id ) const
{
    mLock4Read();
    return gtByID( id );
}


Well::Marker Well::MarkerSet::gtByID( MarkerID id ) const
{
    const IdxType idx = gtIdxFor( id );
    return idx != -1 ? markers_[idx] : Marker::udf();
}


Well::MarkerSet::IdxType Well::MarkerSet::getIdxFor( MarkerID id ) const
{
   mLock4Read();
   return gtIdxFor( id );
}


Well::MarkerSet::IdxType Well::MarkerSet::gtIdxFor( MarkerID id ) const
{
    if ( id.isValid() )
    {
	const size_type sz = gtSize();

	if ( id.getI() < sz && markerids_[id.getI()].getI() == id.getI() )
	    return id.getI();

	for ( IdxType idx=0; idx<sz; idx++ )
	    if ( markerids_[idx] == id )
		return idx;
    }

    return -1;
}


Well::MarkerSet::IdxType Well::MarkerSet::gtIdxForDah( float dah ) const
{
    for ( IdxType imrk=0; imrk<markers_.size(); imrk++ )
    {
	const Well::Marker& mrk = markers_[imrk];
	if ( dah < mrk.dah() )
	    return imrk;
    }

    return markers_.size() ? markers_.size() : -1;
}


bool Well::MarkerSet::validIdx( IdxType idx ) const
{
    mLock4Read();
    return markers_.validIdx( idx );
}


bool Well::MarkerSet::isPresent( const char* mname ) const
{
    mLock4Read();
    return isPrsnt( mname );
}


bool Well::MarkerSet::isPrsnt( const char* mname ) const
{
    const IdxType idx = idxOf( mname );
    return idx >= 0;
}

void Well::MarkerSet::fillWithAll( TaskRunner* tskr )
{
    setEmpty();

    Well::InfoCollector ic( false, true, false );
    if ( tskr )
	tskr->execute( ic );
    else
	ic.execute();

    if ( ic.markers().isEmpty() )
	return;

    *this = *ic.markers()[0];
    for ( IdxType idx=1; idx<ic.markers().size(); idx++ )
	append( *ic.markers()[idx] );
}


Well::Marker Well::MarkerSet::getByName( const char* mname ) const
{
    mLock4Read();
    return gtByName( mname );
}


Well::Marker Well::MarkerSet::gtByName( const char* mname ) const
{
    const IdxType idx = idxOf( mname );
    return  idx < 0 ? Marker::udf() : markers_[idx];
}


Well::MarkerSet::IdxType Well::MarkerSet::getIdxAbove( float reqz,
					    const Track* trck ) const
{
    PtrMan<MonitorLock> trckml = 0;
    if ( trck )
	trckml = new MonitorLock( *trck );

    mLock4Read();
    for ( IdxType idx=0; idx<gtSize(); idx++ )
    {
	const Marker& mrk = markers_[idx];
	float mrkz = mrk.dah();
	if ( trck )
	    mrkz = mCast(float,trck->getPos(mrkz).z_);
	if ( mrkz > reqz )
	    return idx-1;
    }
    return gtSize() - 1;
}


Well::MarkerSet::IdxType Well::MarkerSet::indexOf( const char* mname ) const
{
    mLock4Read();
    return idxOf( mname );
}


Well::MarkerSet::IdxType Well::MarkerSet::idxOf( const char* mname ) const
{
    const BufferString needle( mname );
    for ( IdxType idx=0; idx<markers_.size(); idx++ )
    {
	if ( markers_[idx].name() == needle )
	    return idx;
    }
    return -1;
}


Well::MarkerSet::MarkerID Well::MarkerSet::markerIDFor( IdxType idx ) const
{
    mLock4Read();
    return mrkrIDFor( idx );
}


Well::MarkerSet::MarkerID Well::MarkerSet::mrkrIDFor( IdxType idx ) const
{
    return markerids_.validIdx(idx) ? markerids_[idx] : MarkerID::getInvalid();
}


bool Well::MarkerSet::insertNew( const Well::Marker& newmrk )
{
    if (newmrk.isUdf() || isPrsnt(newmrk.name()) )
	{ return false; }

    mLock4Write();
    IdxType newidx = gtIdxForDah( newmrk.dah() );
    insrtAt( newidx, newmrk );
    const MarkerID& mid = mrkrIDFor( newidx );
    mSendChgNotif( cMarkerAdded(), mid.getI() );
    return true;
}


bool Well::MarkerSet::insrtNew( const Well::Marker& newmrk )
{
    if ( newmrk.isUdf() || isPrsnt(newmrk.name().buf()) )
	{ return false; }

    IdxType newidx = gtIdxForDah( newmrk.dah() );

    insrtAt( newidx, newmrk );
    return true;
}


void Well::MarkerSet::addCopy( const MarkerSet& ms, IdxType idx, float dah )
{
    Well::Marker newwm = ms.getByIdx( idx );
    newwm.setDah( dah );
    insrtNew( newwm );
}


void Well::MarkerSet::addSameWell( const MarkerSet& ms )
{
    mLock4Write();
    const size_type mssz = ms.size();
    for ( IdxType idx=0; idx<mssz; idx++ )
    {
	const Well::Marker& mrkr = ms.gtByIndex( idx );
	if ( !isPrsnt(mrkr.name()) )
	    insrtNew( mrkr );
    }

     mSendEntireObjChgNotif();
}


void Well::MarkerSet::moveBlock( IdxType fromidx, IdxType toidxblockstart,
				 const TypeSet<IdxType>& idxs )
{
    Interval<IdxType> fromrg( fromidx, fromidx );
    for ( IdxType idx=fromidx+1; idx<idxs.size(); idx++ )
    {
	if ( idxs[idx] < 0 )
	    fromrg.stop = idx;
	else
	    break;
    }

    Well::MarkerSet tomove;
    for ( IdxType idx=fromrg.start; idx<=fromrg.stop; idx++ )
    {
	Marker& oldmrk = markers_[idx];oldmrk.setName( "" );
	tomove.add( oldmrk );
    }

    IdxType toidx = toidxblockstart;
    for ( IdxType idx=toidxblockstart+1; idx<idxs.size(); idx++ )
    {
	if ( idxs[idx] < 0 )
	    toidx = idx;
	else
	    break;
    }


    insrtNewAfter( toidx, tomove );

    for ( IdxType idx=fromrg.start; idx<=fromrg.stop; idx++ )
	rmoveSingle( fromrg.start );
}


void Well::MarkerSet::insrtAt( IdxType idx, const Marker& mrkr )
{
    if ( idx < 0 )
    {
	markers_.add( mrkr );
	markerids_.add(MarkerID::get(curmrkridnr_++));
	return;
    }

    markers_.insert( idx, mrkr );
    markerids_.insert( idx, MarkerID::get(curmrkridnr_++) );
}


void Well::MarkerSet::insrtAfter( IdxType idx ,const Marker& mrkr )
{
    const IdxType aftidx = idx < 0 ? 0 : idx + 1;
    if ( aftidx <= markers_.size()-1 )
	insrtAt( aftidx, mrkr );
    else
    {
	markers_.add( mrkr );
	markerids_.add( MarkerID::get(curmrkridnr_++) );
    }
}


void Well::MarkerSet::rmoveSingle( IdxType idx )
{
    markers_.removeSingle( idx );
    markerids_.removeSingle( idx );
}


void Well::MarkerSet::removeSingle( MarkerID mrkrid )
{
    mLock4Write();

    const IdxType idx = getIdxFor( mrkrid );
    rmoveSingle( idx );
    mSendChgNotif( cMarkerRemoved(), mrkrid.getI() );
}


void Well::MarkerSet::removeSingleByIdx( IdxType idx )
{
    mLock4Write();

    const MarkerID& mid = mrkrIDFor( idx );
    rmoveSingle( idx );
    mSendChgNotif( cMarkerRemoved(), mid.getI() );
}


void Well::MarkerSet::insrtNewAfter( IdxType aftidx, const MarkerSet& mrkrs )
{
    if ( mrkrs.isEmpty() )
	return;
    else if ( markers_.isEmpty() )
	{ copyAll( mrkrs ); return; }

    const size_type mrkrsz = markers_.size();
    Interval<float> dahbounds( markers_[0].dah() - 10,
			       markers_[mrkrsz-1].dah() + 10 );

    Interval<IdxType> idxs;
    if ( aftidx < 0 )
    {
	for ( IdxType idx=mrkrs.size()-1; idx>-1; idx-- )
	    insrtAt( 0, mrkrs.getByIdx(idx) );
	idxs = Interval<IdxType>( 0, mrkrs.size()-1 );
    }
    else
    {
	for ( IdxType idx=mrkrs.size()-1; idx>-1; idx-- )
	    insrtAfter( aftidx, mrkrs.getByIdx(idx) );
	idxs = Interval<IdxType>( aftidx+1, aftidx+mrkrs.size() );
    }

    if ( idxs.start > 0 )
	dahbounds.start = markers_[idxs.start-1].dah();
    else if ( idxs.stop < size()-1 )
	dahbounds.stop = markers_[idxs.stop+1].dah();

    if ( markers_[idxs.start].dah() > dahbounds.start
	&& markers_[idxs.stop].dah() < dahbounds.stop )
	return;

    const float gapwdht = dahbounds.stop - dahbounds.start;
    if ( gapwdht == 0 )
	for ( IdxType idx=idxs.start; idx<=idxs.stop; idx++ )
	    markers_[idx].setDah( dahbounds.start );
    else
    {
	const float dahstep = gapwdht / (idxs.width() + 2);
	for ( IdxType idx=idxs.start; idx<=idxs.stop; idx++ )
	   markers_[idx].setDah( dahbounds.start
				  + dahstep * (idx-idxs.start+1) );
    }

}


void Well::MarkerSet::alignOrderingWith( const MarkerSet& ms1 )
{
    const size_type ms0szs = markers_.size(); const size_type ms1sz =ms1.size();
    TypeSet<IdxType> idx0s( ms1sz, -1 ); TypeSet<IdxType> idx1s( ms0szs, -1 );
    for ( IdxType ms1idx=0; ms1idx<ms1sz; ms1idx++ )
    {
	const IdxType idx0 = idxOf( ms1.getByIdx(ms1idx).name() );
	idx0s[ms1idx] = idx0;
	if ( idx0 >= 0 )
	    idx1s[idx0] = ms1idx;
    }

    IdxType previdx0 = idx0s[0];
    for ( IdxType ms1idx=0; ms1idx<ms1sz; ms1idx++ )
    {
	const IdxType idx0 = idx0s[ms1idx];
	if ( previdx0 < 0 )
	    { previdx0 = idx0; continue; }
	else if ( idx0 < 0 )
	    continue;
	if ( idx0 >= previdx0 )
	    previdx0 = idx0;
	else
	{
	    moveBlock( idx0, previdx0, idx1s );
	    alignOrderingWith( ms1 );
	    return;
	}
    }
}


void Well::MarkerSet::mergeOtherWell( const MarkerSet& ms1 )
{
    mLock4Write();

    if ( ms1.isEmpty() )
	return;

    alignOrderingWith( ms1 );

    // Any new (i.e. not present in this) markers there?
    TypeSet<IdxType> idx0s;
    const size_type ms1sz = ms1.size();
    bool havenew = false;
    for ( IdxType ms1idx=0; ms1idx<ms1sz; ms1idx++ )
    {
	const IdxType idx0 = idxOf( ms1.getByIdx(ms1idx).name() );
	idx0s += idx0;
	if ( idx0 < 0 )
	    havenew = true;
    }
    if ( !havenew )
	return; // no? then we're cool already. Nothing to do.


	// Find first and last common markers.
    IdxType ms1idxfirstmatch = -1; IdxType ms1idxlastmatch = -1;
    for ( IdxType ms1idx=0; ms1idx<idx0s.size(); ms1idx++ )
    {
	if ( idx0s[ms1idx] >= 0 )
	{
	    ms1idxlastmatch = ms1idx;
	    if ( ms1idxfirstmatch < 0 )
		ms1idxfirstmatch = ms1idx;
	}
    }
    if ( ms1idxfirstmatch < 0 )
    {
	mUnlockAllAccess();
	addSameWell( ms1 );
	return;
    }

	// Add the markers above and below
    float edgediff = ms1.getByIdx(ms1idxfirstmatch).dah()
			- markers_[ idx0s[ms1idxfirstmatch] ].dah();
    for ( IdxType ms1idx=0; ms1idx<ms1idxfirstmatch; ms1idx++ )
	addCopy( ms1, ms1idx, ms1.getByIdx(ms1idx).dah() - edgediff );

    edgediff = ms1.getByIdx(ms1idxlastmatch).dah()
			- markers_[ idx0s[ms1idxlastmatch] ].dah();
    for ( IdxType ms1idx=ms1idxlastmatch+1; ms1idx<ms1sz; ms1idx++ )
	addCopy( ms1, ms1idx, ms1.getByIdx(ms1idx).dah() - edgediff );

    if ( ms1idxfirstmatch == ms1idxlastmatch )
	return;

	// There are new markers in the middle. Set up positioning framework.
    TypeSet<float> xvals, yvals;
    for ( IdxType ms1idx=ms1idxfirstmatch; ms1idx<=ms1idxlastmatch; ms1idx++ )
    {
	const IdxType idx0 = idx0s[ms1idx];
	if ( idx0 >= 0 )
	{
	    xvals += markers_[idx0].dah();
	    yvals += ms1.getByIdx(ms1idx).dah();
	}
    }

	// Now add the new markers at a good place.
    const size_type nrpts = xvals.size();
    for ( IdxType ms1idx=ms1idxfirstmatch+1; ms1idx<ms1idxlastmatch; ms1idx++ )
    {
	if ( idx0s[ms1idx] >= 0 )
	    continue;

	IdxType loidx;
	const float ms1dah = ms1.getByIdx(ms1idx).dah();
	if ( IdxAble::findFPPos(yvals,nrpts,ms1dah,ms1idxfirstmatch,loidx) )
	    continue; // Two markers in ms1 at same pos. Ignore this one.

	const float relpos = (ms1dah - yvals[loidx])
			   / (yvals[loidx+1]-yvals[loidx]);
	addCopy( ms1, ms1idx, relpos*xvals[loidx+1] + (1-relpos)*xvals[loidx] );
    }

     mSendEntireObjChgNotif();
}


Well::Marker Well::MarkerSet::gtByLvlID( LevelID lvlid ) const
{
    if ( lvlid.isInvalid() )
	return 0;

    mLock4Read();
    for ( IdxType idmrk=0; idmrk<size(); idmrk++ )
    {
	Well::Marker mrk = markers_[idmrk];
	if ( mrk.levelID() == lvlid )
	    return mrk;
    }

    return 0;
}


void Well::MarkerSet::getNames( BufferStringSet& nms ) const
{
    mLock4Read();
    for ( IdxType idx=0; idx<size(); idx++ )
	nms.add( markers_[idx].name() );
}


void Well::MarkerSet::getColors( TypeSet<Color>& cols ) const
{
    mLock4Read();
    for ( IdxType idx=0; idx<size(); idx++ )
	cols += markers_[idx].color();
}


void Well::MarkerSet::fillPar( IOPar& iop ) const
{
    mLock4Read();
    for ( IdxType idx=0; idx<gtSize(); idx++ )
    {
	IOPar mpar;
	const Marker& mrk = markers_[idx];
	mpar.set( sKey::Name(), mrk.name() );
	mpar.set( sKey::Color(), mrk.color() );
	mpar.set( sKey::Depth(), mrk.dah() );
	mpar.set( sKey::Level(), mrk.levelID() );
	iop.mergeComp( mpar, ::toString(idx+1) );
    }
}


void Well::MarkerSet::usePar( const IOPar& iop )
{
    setEmpty();

    for ( IdxType imrk=1; ; imrk++ )
    {
	PtrMan<IOPar> mpar = iop.subselect( imrk );
	if ( !mpar || mpar->isEmpty() )
	    break;

	BufferString nm; mpar->get( sKey::Name(), nm );
	if ( nm.isEmpty() || isPresent(nm) )
	    continue;

	float dpt = 0;
	mpar->get( sKey::Depth(), dpt );
	Color col;
	mpar->get( sKey::Color(), col );
	LevelID lvlid;
	mpar->get( sKey::Level(), lvlid );

	Marker mrk( nm, dpt, col ); mrk.setLevelID( lvlid );
	insertNew( mrk );
    }
}


// MarkerSetIter's

static Well::MarkerSetIter::IdxType gtIterIdx( const Well::MarkerSet& ms,
	Well::MarkerSet::MarkerID id, bool isstart )
{
    if ( ms.isEmpty() )
	return -1;

    if ( id.isInvalid() )
	return isstart ? 0 : ms.size()-1;

    return ms.getIdxFor( id );
}


static Well::MarkerSetIter::IdxType gtIterIdx( const Well::MarkerSet& ms,
						const char* nm, bool isstart )
{
    if ( ms.isEmpty() )
	return -1;

    if ( !nm || !*nm )
	return isstart ? 0 : ms.size()-1;

    return ms.indexOf( nm );
}


Well::MarkerSetIter::MarkerSetIter( const Well::MarkerSet& ms, bool dorev )
    : MonitorableIter4Read<Well::MarkerSet::IdxType>( ms,
	    dorev?ms.size()-1:0, dorev?0:ms.size()-1 )
{
}


Well::MarkerSetIter::MarkerSetIter( const MarkerSet& ms,
				    MarkerID startid, MarkerID stopid )
    : MonitorableIter4Read<IdxType>(ms,gtIterIdx(ms,startid,true),
				    gtIterIdx(ms,stopid,false) )
{
}

Well::MarkerSetIter::MarkerSetIter( const Well::MarkerSet& ms,
				    const char* startnm, const char* stopnm )
    : MonitorableIter4Read<IdxType>(ms,gtIterIdx(ms,startnm,true),
				    gtIterIdx(ms,stopnm,false) )
{
}


Well::MarkerSetIter::MarkerSetIter( const MarkerSetIter& oth )
    : MonitorableIter4Read<MarkerSet::IdxType>(oth)
{
}


const Well::MarkerSet& Well::MarkerSetIter::markerSet() const
{
    return static_cast<const MarkerSet&>( monitored() );
}


Well::MarkerSet::MarkerID Well::MarkerSetIter::ID() const
{
    return isValid() ? markerSet().markerIDFor( curidx_ )
		     : MarkerID::getInvalid();
}


const Well::Marker& Well::MarkerSetIter::get() const
{
    return isValid() ? markerSet().markers_[curidx_] : Marker::udf();
}


float Well::MarkerSetIter::getDah() const
{
    return isValid() ? markerSet().markers_[curidx_].dah() : mUdf(float);
}


BufferString Well::MarkerSetIter::markerName() const
{
    return isValid() ? markerSet().markers_[curidx_].name()
		     : BufferString::empty();
}


// Well::MarkerSet4Edit
Well::MarkerSetIter4Edit::MarkerSetIter4Edit( Well::MarkerSet& ms, bool dorev )
    : MonitorableIter4Write<IdxType>(ms,dorev?ms.size()-1:0,dorev?0:ms.size()-1)
{
}


Well::MarkerSetIter4Edit::MarkerSetIter4Edit( MarkerSet& ms,
					      Interval<IdxType> rg )
    : MonitorableIter4Write<IdxType>(ms,rg.start,rg.stop)
{
}


Well::MarkerSetIter4Edit::MarkerSetIter4Edit( MarkerSet& ms,
					      const char* startnm,
					      const char* stopnm )
    : MonitorableIter4Write<IdxType>(ms,gtIterIdx(ms,startnm,true),
				    gtIterIdx(ms,stopnm,false) )
{
}


Well::MarkerSetIter4Edit::MarkerSetIter4Edit( const MarkerSetIter4Edit& oth )
    : MonitorableIter4Write<IdxType>(oth)
{
}


Well::MarkerSet& Well::MarkerSetIter4Edit::markerSet()
{
    return static_cast<MarkerSet&>( edited() );
}


const Well::MarkerSet& Well::MarkerSetIter4Edit::markerSet() const
{
    return static_cast<const MarkerSet&>( monitored() );
}


BufferString Well::MarkerSetIter4Edit::markerName() const
{
    if ( isValid() )
	return markerSet().markers_[curidx_].name();
    return BufferString::empty();
}


Well::MarkerSet::MarkerID Well::MarkerSetIter4Edit::ID() const
{
    return isValid() ? markerSet().markerIDFor( curidx_ )
		     : MarkerID::getInvalid();
}


Well::Marker& Well::MarkerSetIter4Edit::get() const
{
    return !isValid() ? Marker::dummy()
	: const_cast<Marker&>(markerSet().markers_[curidx_]);
}


float Well::MarkerSetIter4Edit::getDah() const
{
    return isValid() ? markerSet().markers_[curidx_].dah() : mUdf(float);
}


void Well::MarkerSetIter4Edit::setDah( float dah )
{
    if ( isValid() )
	markerSet().markers_[curidx_].setDah( dah );
}


void Well::MarkerSetIter4Edit::setColor( const Color& clr )
{
    if ( isValid() )
	markerSet().markers_[curidx_].setColor( clr );
}


void Well::MarkerSetIter4Edit::removeCurrent()
{
    if ( isValid() )
    {
	markerSet().markers_.removeSingle( curidx_ );
	currentRemoved();
    }
}


void Well::MarkerSetIter4Edit::insert( const Marker& mrkr )
{
    if ( isValid() )
    {
	markerSet().markers_.insert( curidx_, mrkr );
	insertedAtCurrent();
    }
}



//Well::MarkerRange

Well::MarkerRange::MarkerRange( const Well::MarkerSet& ms,
				MarkerID tpid, MarkerID btid )
    : markerset_(ms)
    , topid_(tpid)
    , botid_(btid)
{
    ms.ref();
}


Well::MarkerRange::MarkerRange( const Well::MarkerSet& ms,
				const char* topnm, const char* botnm )
    : markerset_(ms)
    , topid_(ms.markerIDFor(gtIterIdx(ms,topnm,true)))
    , botid_(ms.markerIDFor(gtIterIdx(ms,botnm,false)))
{
    ms.ref();
}


Well::MarkerRange::~MarkerRange()
{
    markerset_.unRef();
}


Interval<Well::MarkerRange::IdxType> Well::MarkerRange::idxRange() const
{
    return Interval<IdxType>( gtIterIdx(markerset_,topid_,true),
			      gtIterIdx(markerset_,botid_,false) );
}


Well::MarkerRange::size_type Well::MarkerRange::size() const
{
    MonitorLock ml( markerset_ );
    const Interval<IdxType> rg = idxRange();
    return rg.width() + 1;
}


bool Well::MarkerRange::isValid() const
{
    MonitorLock ml( markerset_ );
    const Interval<IdxType> rg = idxRange();
    const size_type inpsz = markerset_.size();
    return inpsz > 0
	&& rg.start >= 0 && rg.stop >= 0
	&& rg.start < inpsz && rg.stop < inpsz
	&& rg.start <= rg.stop;
}


bool Well::MarkerRange::isIncluded( const char* nm ) const
{
    if ( !isValid() ) return false;

    Well::MarkerSetIter miter( markerset_, topid_, botid_ );
    while( miter.next() )
    {
	if ( miter.markerName() == nm )
	    return true;
    }

    return false;
}


bool Well::MarkerRange::isIncluded( IdxType idx ) const
{
    MonitorLock ml( markerset_ );
    const Interval<IdxType> rg = idxRange();
    return rg.includes(idx,false);
}


bool Well::MarkerRange::isIncluded( Well::MarkerSet::MarkerID mid ) const
{
    const MarkerSet::IdxType idx = markerset_.getIdxFor( mid );
    return isIncluded( idx );
}


bool Well::MarkerRange::isIncluded( float z ) const
{
    if ( !isValid() ) return false;

    MonitorLock ml( markerset_ );
    const Interval<IdxType> rg = idxRange();
    return z >= markerset_.getByIdx(rg.start).dah()
	&& z <= markerset_.getByIdx(rg.stop).dah();
}


float Well::MarkerRange::thickness() const
{
    MonitorLock ml( markerset_ );
    const Interval<IdxType> rg = idxRange();
    return markerset_.getByIdx(rg.stop).dah()
	    - markerset_.getByIdx(rg.start).dah();
}


void Well::MarkerRange::getNames( BufferStringSet& nms ) const
{
    if ( !isValid() ) return;

    Well::MarkerSetIter miter( markerset_, topid_, botid_ );
    while( miter.next() )
        nms.add( miter.markerName() );
}


Well::MarkerSet* Well::MarkerRange::getResultSet() const
{
    MarkerSet* ret = new MarkerSet;
    if ( !isValid() ) return ret;

    Well::MarkerSetIter miter( markerset_, topid_, botid_ );
    while( miter.next() )
	ret->add( miter.get() );

    return ret;
}


void Well::MarkerChgRange::setThickness( float newth )
{
    Interval<IdxType> rg = idxRange();
    if ( !isValid() || rg.start == rg.stop )
	return;
    if ( newth < 0 )
	newth = 0;

    MonitorLock ml( markerset_ );
    const float startdah = markerset_.getByIdx(rg.start).dah();
    const float oldth = markerset_.getByIdx(rg.stop).dah() - startdah;

    RefMan<MarkerSet> newms = new MarkerSet( markerset_ );

    const Interval<IdxType> rg1( rg.start+1, rg.stop );
    MarkerSetIter4Edit msiter( *newms, rg1 );
    if ( mIsZero(newth,mDefEps) )
    {
	while( msiter.next() )
	    msiter.setDah( startdah );
    }
    else
    {
	const float comprfac = newth / oldth;
	while( msiter.next() )
	{
	    const IdxType idx = msiter.curIdx();
	    const float newdist = comprfac * (markerset_.getByIdx(idx).dah()
								    -startdah);
	    msiter.setDah( startdah + newdist );
	}
    }
    msiter.retire();

    const float deltath = oldth - newth;
    const float lastdah = markerset_.getByIdx(rg.stop-1).dah();
    const Interval<IdxType> rg2( rg.stop, markerset_.size() );
    MarkerSetIter4Edit msiter2( *newms, rg2 );
    while( msiter2.next() )
    {
	float newdah = markerset_.getByIdx(msiter2.curIdx()).dah() - deltath;
	if ( newdah < lastdah ) // just a guard against rounding errors
	    newdah = lastdah;
	msiter2.setDah( newdah );
    }

    getMarkers() = *newms;
}


void Well::MarkerChgRange::remove()
{
    if ( !isValid() ) return;

    Interval<IdxType> rg = idxRange();
    const size_type nrlays = rg.width() + 1;
    for ( IdxType idx=0; idx<nrlays; idx++ )
	getMarkers().removeSingleByIdx( rg.start );

    rg.stop = rg.start = rg.start - 1;
}
