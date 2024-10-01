/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmarker.h"

#include "bufstringset.h"
#include "color.h"
#include "idxable.h"
#include "iopar.h"
#include "keystrs.h"
#include "stratlevel.h"
#include "wellextractdata.h"
#include "welltrack.h"

const char* Well::Marker::sKeyDah()	{ return "Depth along hole"; }


Well::Marker::Marker( const char* nm, float dh, OD::Color c )
    : ::NamedObject(nm)
    , dah_(dh)
    , color_(c)
    , levelid_(Strat::LevelID::udf())
{
}


Well::Marker::Marker( Strat::LevelID lvlid, float dh )
    : ::NamedObject("")
    , dah_(dh)
    , color_(OD::Color::Black())
    , levelid_(lvlid)
{
}


Well::Marker::Marker( const Marker& oth )
    : ::NamedObject("")
{
    *this = oth;
}


Well::Marker::~Marker()
{
}


Well::Marker& Well::Marker::operator =( const Well::Marker& oth )
{
    if ( this != &oth )
    {
	NamedObject::operator=( oth );
	dah_ = oth.dah_;
	color_ = oth.color_;
	levelid_ = oth.levelid_;
    }
    return *this;
}


OD::Color Well::Marker::color() const
{
    const Strat::LevelSet& lvls = Strat::LVLS();
    if ( levelid_.isValid() && lvls.isPresent(levelid_) )
	return lvls.colorOf( levelid_ );

    return color_;
}


Strat::Level Well::Marker::getLevel() const
{
    return Strat::LVLS().get( levelid_ );
}


void Well::Marker::setNoLevelID()
{
    setLevelID( Strat::LevelID::udf() );
}


// Well::MarkerSet
Well::MarkerSet::MarkerSet()
{}


Well::MarkerSet::~MarkerSet()
{}


void Well::MarkerSet::fillWithAll( TaskRunner* tr )
{
    setEmpty();

    Well::InfoCollector ic( false, true, false );
    if ( tr )
	tr->execute( ic );
    else
	ic.execute();

    if ( ic.markers().isEmpty() )
	return;

    *this = *ic.markers()[0];
    for ( int idx=1; idx<ic.markers().size(); idx++ )
	append( *ic.markers()[idx] );
}


ObjectSet<Well::Marker>& Well::MarkerSet::doAdd( Well::Marker* mrk )
{
    if ( mrk && !isPresent( mrk->name().buf() ) )
	ObjectSet<Well::Marker>::doAdd( mrk );

    return *this;
}


Well::Marker* Well::MarkerSet::gtByName( const char* mname ) const
{
    const int idx = indexOf( mname );
    return  idx < 0 ? 0 : const_cast<Well::Marker*>((*this)[idx]);
}


int Well::MarkerSet::getIdxAbove( float reqz, const Well::Track* trck ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Marker& mrk = *(*this)[idx];
	float mrkz = mrk.dah();
	if ( trck )
	    mrkz = mCast(float,trck->getPos(mrkz).z);
	if ( mrkz > reqz )
	    return idx-1;
    }
    return size() - 1;
}


int Well::MarkerSet::getIdxBelow( float reqz, const Well::Track* trck ) const
{
    for ( int idx=size()-1; idx>=0; idx-- )
    {
	const Marker& mrk = *(*this)[idx];
	float mrkz = mrk.dah();
	if ( trck )
	    mrkz = mCast(float,trck->getPos(mrkz).z);
	if ( mrkz < reqz )
	    return idx+1;
    }
    return 0;
}



int Well::MarkerSet::indexOf( const char* mname ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx]->name()==mname )
	    return idx;
    }
    return -1;
}


void Well::MarkerSet::sortByDAH()
{
    TypeSet<float> dahs; dahs.setSize( size(), mUdf(float) );
    TypeSet<int> idxs; idxs.setSize( size(), -1 );

    for ( int imrkr=0; imrkr<size(); imrkr++ )
    {
	dahs[imrkr] = (*this)[imrkr]->dah();
	idxs[imrkr] = imrkr;
    }

    sort_coupled( dahs.arr(), idxs.arr(), idxs.size() );
    ObjectSet<Well::Marker> newidxmarkers;
    for ( int idx=0; idx<idxs.size(); idx++ )
	newidxmarkers.add( (*this)[ idxs[idx] ] );
    this->::ObjectSet<Marker>::erase();
    for ( int imrkr=0; imrkr<newidxmarkers.size(); imrkr++ )
	add( newidxmarkers[imrkr] );
}


bool Well::MarkerSet::insertNew( Well::Marker* newmrk )
{
    if ( !newmrk || isPresent(newmrk->name().buf()) )
	{ delete newmrk; return false; }

    int newidx = 0;
    for ( int imrk=0; imrk<size(); imrk++ )
    {
	Well::Marker& mrk = *(*this)[imrk];
	if ( newmrk->dah() < mrk.dah() )
	    break;
	newidx++;
    }
    insertAt( newmrk, newidx );
    return true;
}


void Well::MarkerSet::addCopy( const ObjectSet<Well::Marker>& ms,
				int idx, float dah )
{
    Well::Marker* newwm = new Marker( *ms[idx] );
    newwm->setDah( dah );
    insertNew( newwm );
}


void Well::MarkerSet::addSameWell( const ObjectSet<Well::Marker>& ms )
{
    const int mssz = ms.size();
    for ( int idx=0; idx<mssz; idx++ )
    {
	if ( !isPresent(ms[idx]->name()) )
	    insertNew( new Well::Marker( *ms[idx] ) );
    }
}


bool Well::MarkerSet::addSameWell( const Marker& marker )
{
    return insertNew( new Marker(marker) );
}


void Well::MarkerSet::moveBlock( int fromidx, int toidxblockstart,
				 const TypeSet<int>& idxs )
{
    Interval<int> fromrg( fromidx, fromidx );
    for ( int idx=fromidx+1; idx<idxs.size(); idx++ )
    {
	if ( idxs[idx] < 0 )
	    fromrg.stop_ = idx;
	else
	    break;
    }

    ObjectSet<Marker> tomove;
    for ( int idx=fromrg.start_; idx<=fromrg.stop_; idx++ )
    {
	Marker& oldmrk = *(*this)[idx];
	tomove += new Marker( oldmrk );
	oldmrk.setName( "" );
    }

    int toidx = toidxblockstart;
    for ( int idx=toidxblockstart+1; idx<idxs.size(); idx++ )
    {
	if ( idxs[idx] < 0 )
	    toidx = idx;
	else
	    break;
    }

    insertNewAfter( toidx, tomove );

    for ( int idx=fromrg.start_; idx<=fromrg.stop_; idx++ )
	removeSingle( fromrg.start_ );
}


void Well::MarkerSet::insertNewAfter( int aftidx,
					ObjectSet<Well::Marker>& mrkrs )
{
    if ( mrkrs.isEmpty() )
	return;
    else if ( isEmpty() )
	{ ObjectSet<Marker>::append( mrkrs ); mrkrs.erase(); return; }

    Interval<float> dahbounds( (*this)[0]->dah() - 10,
				(*this)[size()-1]->dah() + 10 );

    Interval<int> idxs;
    if ( aftidx < 0 )
    {
	for ( int idx=mrkrs.size()-1; idx>-1; idx-- )
	    insertAt( mrkrs[idx], 0 );
	idxs = Interval<int>( 0, mrkrs.size()-1 );
    }
    else
    {
	for ( int idx=mrkrs.size()-1; idx>-1; idx-- )
	    insertAfter( mrkrs[idx], aftidx );
	idxs = Interval<int>( aftidx+1, aftidx+mrkrs.size() );
    }
    mrkrs.erase();

    if ( idxs.start_ > 0 )
	dahbounds.start_ = (*this)[idxs.start_-1]->dah();
    else if ( idxs.stop_ < size()-1 )
	dahbounds.stop_ = (*this)[idxs.stop_+1]->dah();

    if ( (*this)[idxs.start_]->dah() > dahbounds.start_
	 && (*this)[idxs.stop_]->dah() < dahbounds.stop_ )
	return;

    const float gapwdht = dahbounds.stop_ - dahbounds.start_;
    if ( gapwdht == 0 )
	for ( int idx=idxs.start_; idx<=idxs.stop_; idx++ )
	    (*this)[idx]->setDah( dahbounds.start_ );
    else
    {
	const float dahstep = gapwdht / (idxs.width() + 2);
	for ( int idx=idxs.start_; idx<=idxs.stop_; idx++ )
	    (*this)[idx]->setDah( dahbounds.start_
				  + dahstep * (idx-idxs.start_+1) );
    }
}


void Well::MarkerSet::alignOrderingWith( const ObjectSet<Well::Marker>& ms1 )
{
    const int ms0szs = size(); const int ms1sz = ms1.size();
    TypeSet<int> idx0s( ms1sz, -1 ); TypeSet<int> idx1s( ms0szs, -1 );
    for ( int ms1idx=0; ms1idx<ms1sz; ms1idx++ )
    {
	const int idx0 = indexOf( ms1[ms1idx]->name() );
	idx0s[ms1idx] = idx0;
	if ( idx0 >= 0 )
	    idx1s[idx0] = ms1idx;
    }

    int previdx0 = idx0s[0];
    for ( int ms1idx=0; ms1idx<ms1sz; ms1idx++ )
    {
	const int idx0 = idx0s[ms1idx];
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


void Well::MarkerSet::mergeOtherWell( const ObjectSet<Well::Marker>& ms1 )
{
    if ( ms1.isEmpty() )
	return;

    alignOrderingWith( ms1 );

	// Any new (i.e. not present in this) markers there?
    TypeSet<int> idx0s;
    const int ms1sz = ms1.size();
    bool havenew = false;
    for ( int ms1idx=0; ms1idx<ms1sz; ms1idx++ )
    {
	const int idx0 = indexOf( ms1[ms1idx]->name() );
	idx0s += idx0;
	if ( idx0 < 0 )
	    havenew = true;
    }
    if ( !havenew )
	return; // no? then we're cool already. Nothing to do.


	// Find first and last common markers.
    int ms1idxfirstmatch = -1; int ms1idxlastmatch = -1;
    for ( int ms1idx=0; ms1idx<idx0s.size(); ms1idx++ )
    {
	if ( idx0s[ms1idx] >= 0 )
	{
	    ms1idxlastmatch = ms1idx;
	    if ( ms1idxfirstmatch < 0 )
		ms1idxfirstmatch = ms1idx;
	}
    }
    if ( ms1idxfirstmatch < 0 )
	{ addSameWell( ms1 ); return; }

	// Add the markers above and below
    float edgediff = ms1[ms1idxfirstmatch]->dah()
			- (*this)[ idx0s[ms1idxfirstmatch] ]->dah();
    for ( int ms1idx=0; ms1idx<ms1idxfirstmatch; ms1idx++ )
	addCopy( ms1, ms1idx, ms1[ms1idx]->dah() - edgediff );

    edgediff = ms1[ms1idxlastmatch]->dah()
			- (*this)[ idx0s[ms1idxlastmatch] ]->dah();
    for ( int ms1idx=ms1idxlastmatch+1; ms1idx<ms1sz; ms1idx++ )
	addCopy( ms1, ms1idx, ms1[ms1idx]->dah() - edgediff );

    if ( ms1idxfirstmatch == ms1idxlastmatch )
	return;

	// There are new markers in the middle. Set up positioning framework.
    TypeSet<float> xvals, yvals;
    for ( int ms1idx=ms1idxfirstmatch; ms1idx<=ms1idxlastmatch; ms1idx++ )
    {
	const int idx0 = idx0s[ms1idx];
	if ( idx0 >= 0 )
	{
	    xvals += (*this)[idx0]->dah();
	    yvals += ms1[ms1idx]->dah();
	}
    }

	// Now add the new markers at a good place.
    const int nrpts = xvals.size();
    for ( int ms1idx=ms1idxfirstmatch+1; ms1idx<ms1idxlastmatch; ms1idx++ )
    {
	if ( idx0s[ms1idx] >= 0 )
	    continue;

	int loidx;
	const float ms1dah = ms1[ms1idx]->dah();
	if ( IdxAble::findFPPos(yvals,nrpts,ms1dah,ms1idxfirstmatch,loidx) )
	{
	    addCopy( ms1, ms1idx, xvals[loidx] );
	    continue;
	}

	const float relpos = (ms1dah - yvals[loidx])
			   / (yvals[loidx+1]-yvals[loidx]);
	addCopy( ms1, ms1idx, relpos*xvals[loidx+1] + (1-relpos)*xvals[loidx] );
    }
}


Well::Marker* Well::MarkerSet::gtByLvlID( Strat::LevelID lvlid ) const
{
    if ( !lvlid.isValid() )
	return nullptr;

    for ( int idmrk=0; idmrk<size(); idmrk++ )
    {
	Well::Marker* mrk = const_cast<Well::Marker*>((*this)[idmrk]);
	if ( mrk && mrk->levelID() == lvlid )
	    return mrk;
    }

    return nullptr;
}


void Well::MarkerSet::getNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( (*this)[idx]->name() );
}


void Well::MarkerSet::getColors( TypeSet<OD::Color>& cols ) const
{
    for ( int idx=0; idx<size(); idx++ )
	cols += (*this)[idx]->color();
}


void Well::MarkerSet::getNamesColorsMDs( BufferStringSet& nms,
			TypeSet<OD::Color>& cols, TypeSet<float>& mds) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	nms.add( (*this)[idx]->name() );
	cols += (*this)[idx]->color();
	mds += (*this)[idx]->dah();
    }
}


void Well::MarkerSet::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	IOPar mpar;
	const Marker& mrk = *(*this)[ idx ];
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

    for ( int imrk=1; ; imrk++ )
    {
	PtrMan<IOPar> mpar = iop.subselect( imrk );
	if ( !mpar || mpar->isEmpty() )
	    break;

	BufferString nm;
	mpar->get( sKey::Name(), nm );
	if ( nm.isEmpty() || isPresent(nm) )
	    continue;

	float dpt = 0;
	mpar->get( sKey::Depth(), dpt );
	OD::Color col(0,0,0);
	mpar->get( sKey::Color(), col );

	Strat::LevelID lvlid;
	mpar->get( sKey::Level(), lvlid );

	Marker* mrk = new Marker( nm, dpt );
	mrk->setColor( col );
	mrk->setLevelID( lvlid );
	insertNew( mrk );
    }
}



// Well::MarkerRange
Well::MarkerRange::MarkerRange( const Well::MarkerSet& ms,
				const Interval<int>& rg )
    : markers_(ms)
{
    init( rg );
}


Well::MarkerRange::MarkerRange( const Well::MarkerSet& ms,
				const char* m1, const char* m2 )
    : markers_(ms)
{
    init( Interval<int>(ms.indexOf(m1),ms.indexOf(m2)) );
}


Well::MarkerRange::~MarkerRange()
{}


void Well::MarkerRange::init( const Interval<int>& rg )
{
    rg_ = rg;

    const int inpsz = markers_.size();
    if ( inpsz < 1 )
	rg_.start_ = rg_.stop_ = -1;
    else
    {
	if ( rg_.start_ < 0 ) rg_.start_ = 0;
	if ( rg_.stop_ < 0 ) rg_.stop_ = inpsz - 1;
	rg_.sort();
	if ( rg_.start_ >= inpsz ) rg_.start_ = inpsz - 1;
	if ( rg_.stop_ >= inpsz ) rg_.stop_ = inpsz - 1;
    }
}


bool Well::MarkerRange::isValid() const
{
    const int inpsz = markers_.size();
    return inpsz > 0
	    && rg_.start_ >= 0 && rg_.stop_ >= 0
	    && rg_.start_ < inpsz && rg_.stop_ < inpsz
	    && rg_.start_ <= rg_.stop_;
}


bool Well::MarkerRange::isIncluded( const char* nm ) const
{
    if ( !isValid() ) return false;

    for ( int idx=rg_.start_; idx<=rg_.stop_; idx++ )
	if ( markers_[idx]->name() == nm )
	    return true;
    return false;
}


bool Well::MarkerRange::isIncluded( float z ) const
{
    if ( !isValid() ) return false;

    return z >= markers_[rg_.start_]->dah()
	    && z <= markers_[rg_.stop_]->dah();
}


float Well::MarkerRange::thickness() const
{
    return markers_[rg_.stop_]->dah() - markers_[rg_.start_]->dah();
}


void Well::MarkerRange::getNames( BufferStringSet& nms ) const
{
    if ( !isValid() ) return;

    for ( int idx=rg_.start_; idx<=rg_.stop_; idx++ )
	nms.add( markers_[idx]->name() );
}


Well::MarkerSet* Well::MarkerRange::getResultSet() const
{
    Well::MarkerSet* ret = new Well::MarkerSet;
    if ( !isValid() ) return ret;

    for ( int idx=rg_.start_; idx<=rg_.stop_; idx++ )
	*ret += new Well::Marker( *markers_[idx] );
    return ret;
}


// Well::MarkerChgRange
Well::MarkerChgRange::MarkerChgRange( MarkerSet& ms, const Interval<int>& idxrg)
    : MarkerRange(ms,idxrg)
{}


Well::MarkerChgRange::MarkerChgRange( MarkerSet& ms, const char* m1,
				const char* m2 )
    : MarkerRange(ms,m1,m2)
{}


Well::MarkerChgRange::~MarkerChgRange()
{}


void Well::MarkerChgRange::setThickness( float newth )
{
    if ( !isValid() || rg_.start_ == rg_.stop_ )
	return;
    if ( newth < 0 )
	newth = 0;

    const float startdah = markers_[rg_.start_]->dah();
    const float oldth = markers_[rg_.stop_]->dah() - startdah;

    if ( mIsZero(newth,mDefEps) )
    {
	for ( int idx=rg_.start_+1; idx<rg_.stop_; idx++ )
	    getMarkers()[idx]->setDah( startdah );
    }
    else
    {
	const float comprfac = newth / oldth;
	for ( int idx=rg_.start_+1; idx<rg_.stop_; idx++ )
	{
	    const float newdist = comprfac * (markers_[idx]->dah() - startdah);
	    getMarkers()[idx]->setDah( startdah + newdist );
	}
    }

    const float deltath = oldth - newth;
    const float lastdah = markers_[rg_.stop_-1]->dah();
    for ( int idx=rg_.stop_; idx<markers_.size(); idx++ )
    {
	float newdah = markers_[idx]->dah() - deltath;
	if ( newdah < lastdah ) // just a guard against rounding errors
	    newdah = lastdah;
	getMarkers()[idx]->setDah( newdah );
    }
}


void Well::MarkerChgRange::remove()
{
    if ( !isValid() ) return;

    const int nrlays = rg_.width() + 1;
    for ( int idx=0; idx<nrlays; idx++ )
	getMarkers().removeSingle( rg_.start_, true );

    rg_.stop_ = rg_.start_ = rg_.start_ - 1;
}
