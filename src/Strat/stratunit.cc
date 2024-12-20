/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratunitref.h"

#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "property.h"
#include "propertyref.h"
#include "separstr.h"
#include "iopar.h"
#include "keystrs.h"
#include "randcolor.h"
#include "sorting.h"

mDefineEnumUtils(Strat::UnitRef,Type,"Unit Type")
{ "Node", "Leaved", "Leaf", nullptr };


//class UnitRef

Strat::UnitRef::UnitRef( NodeUnitRef* up, const char* d )
    : upnode_(up)
    , desc_(d)
    , changed(this)
    , toBeDeleted(this)
{
}


Strat::UnitRef::~UnitRef()
{
    if ( toBeDeleted.isEnabled() )
	notifChange( true );
}


int Strat::UnitRef::treeDepth() const
{
    return upnode_ ? upnode_->treeDepth() + 1 : 0;
}


void Strat::UnitRef::doFill( BufferString& str, int id ) const
{
    FileMultiString fms;
    if ( !mIsUdf(id) )
	fms += id;
    fms += desc_;
    str = fms;
}


void Strat::UnitRef::doUse( const char* str, int* id )
{
    FileMultiString fms( str );
    const int sz = fms.size();
    int nr = 0;
    if ( sz > 1 )
    {
	if ( id )
	    *id = fms.getIValue( nr );
	nr++;
    }
    desc_ = fms[nr];
}


void Strat::UnitRef::getPropsFrom( const IOPar& iop )
{
    pars_ = iop;
    pars_.setName( "Properties" );
    iop.get( sKey::Color(), color_ );
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( "Level" ); // legacy
}


void Strat::UnitRef::putPropsTo( IOPar& iop ) const
{
    iop = pars_;
    BufferString nm( sKeyPropsFor() );
    nm += this == topNode() ? sKeyTreeProps() : fullCode().buf();
    iop.setName( nm );
    iop.set( sKey::Color(), color_ );
    iop.merge( pars_ );
}


Strat::NodeUnitRef* Strat::UnitRef::upNode( int skip )
{
    if ( !upnode_ )
	return nullptr;

    return skip>0 ? upnode_->upNode( skip-1 ) : upnode_;
}


CompoundKey Strat::UnitRef::fullCode() const
{
    CompoundKey kc;
    for ( int idx=treeDepth()-1; idx>=0; idx-- )
    {
	const Strat::NodeUnitRef* ref = upNode( idx );
	kc += ref ? ref->code().buf() : "";
    }
    kc += code();

    return kc;
}


CompoundKey Strat::UnitRef::parentCode() const
{
    CompoundKey kc;

    for ( int idx=treeDepth()-1; idx>=0; idx-- )
	kc += upNode( idx )->code();

    return kc;
}


bool Strat::UnitRef::isBelow( const Strat::UnitRef* un ) const
{
    if ( !un || !upnode_ || un->isLeaf() )
	return false;
    return upnode_ == un || upnode_->isBelow( un );
}


bool Strat::UnitRef::precedes( const UnitRef& un ) const
{
    UnitRefIter it( *topNode() );
    do
    {
	if ( it.unit() == this ) return true;
	if ( it.unit() == &un ) return false;
    }
    while ( it.next() );

    pErrMsg( "Unreachable code reached. Iterator bug." );
    return false;
}


Strat::RefTree& Strat::UnitRef::refTree()
{
    return *((Strat::RefTree*)topNode());
}


const Strat::RefTree& Strat::UnitRef::refTree() const
{
    return *((Strat::RefTree*)topNode());
}


void Strat::UnitRef::setColor( OD::Color c )
{
    if ( c != color_ )
	{ color_ = c; notifChange(); }
}


void Strat::UnitRef::notifChange( bool isrem )
{
    Notifier<UnitRef>& na = isrem ? toBeDeleted : changed;
    na.trigger();
    if ( na.isEmpty() )
	return;

    RefTree& rt = refTree();
    if ( &rt != this )
	rt.reportChange( *this, isrem );
}


//class NodeUnitRef


Strat::NodeUnitRef::NodeUnitRef( NodeUnitRef* up, const char* uc,
				 const char* d )
    : UnitRef(up,d)
    , code_(uc)
    , timerg_(mUdf(float),0.f)
{
}


Strat::NodeUnitRef::~NodeUnitRef()
{
    for( int idx=0; idx<nrRefs(); idx++ )
	ref( idx ).toBeDeleted.disable();

    setEmpty();
}


void Strat::NodeUnitRef::setEmpty()
{
    deepErase( refs_ );
}


void Strat::NodeUnitRef::getPropsFrom( const IOPar& iop )
{
    UnitRef::getPropsFrom( iop );
    iop.get( sKey::Time(), timerg_ );
    pars_.removeWithKey( sKey::Time() );
}


void Strat::NodeUnitRef::putPropsTo( IOPar& iop ) const
{
    UnitRef::putPropsTo( iop );
    iop.set( sKey::Time(), timerg_ );
}


void Strat::NodeUnitRef::setTimeRange( const Interval<float>& rg )
{
    const bool oldudf = mIsUdf(timerg_.start_);
    const bool newudf = mIsUdf(rg.start_);
    bool ischgd = oldudf != newudf;
    if ( !ischgd && !oldudf )
	ischgd = timerg_.start_ != rg.start_ || timerg_.stop_ != rg.stop_;
    if ( !ischgd ) return;

    timerg_ = rg;
    notifChange();
}


void Strat::NodeUnitRef::incTimeRange( const Interval<float>& rg )
{
    if ( mIsUdf(timerg_.start_) )
	setTimeRange( rg );
    else
    {
	Interval<float> newrg( timerg_ );
	newrg.include( rg );
	setTimeRange( newrg );
    }
}


void Strat::NodeUnitRef::changeTimeRange( float dtime )
{
    Strat::UnitRefIter childitr( *this );
    Interval<float> nurtimerg = timeRange();
    nurtimerg.start_ += dtime;
    nurtimerg.stop_ += dtime;
    setTimeRange( nurtimerg );
    while ( childitr.next() )
    {
	mDynamicCastGet(Strat::NodeUnitRef*,nur,childitr.unit())
	if ( nur )
	{
	    Interval<float> newtimerg = nur->timeRange();
	    newtimerg.start_ += dtime;
	    newtimerg.stop_ += dtime;
	    nur->setTimeRange( newtimerg );
	}
    }
}


void Strat::NodeUnitRef::swapChildren( int idx1, int idx2 )
{
    if ( idx1 == idx2 ) return;
    if ( !refs_.validIdx(idx1) || !refs_.validIdx(idx2) )
	return;

    mDynamicCastGet(Strat::NodeUnitRef*,idx1nur,refs_[idx1]);
    mDynamicCastGet(Strat::NodeUnitRef*,idx2nur,refs_[idx2]);
    if ( !idx1nur || !idx2nur )
	return;

    if ( abs(idx1-idx2)>1 )
    {
	pErrMsg( "swapping distant children not allowed" );
	return;
    }

    const int topidx = idx1 < idx2 ? idx1 : idx2;
    const int bottomidx = idx1 < idx2 ? idx2 : idx1;
    mDynamicCastGet(Strat::NodeUnitRef*,topnur,refs_[topidx]);
    mDynamicCastGet(Strat::NodeUnitRef*,bottomnur,refs_[bottomidx]);
    if ( !topnur || !bottomnur )
	return;

    const float topdtime = bottomnur->timeRange().width();
    const float bottomdtime = -topnur->timeRange().width();
    topnur->changeTimeRange( topdtime );
    bottomnur->changeTimeRange( bottomdtime );

    refs_.swap( idx1, idx2 );
    notifChange();
}


Strat::UnitRef* Strat::NodeUnitRef::fnd( const char* unitkey ) const
{
    if ( !unitkey || !*unitkey )
	return code().isEmpty() ? const_cast<Strat::NodeUnitRef*>(this) : 0;

    CompoundKey ck( unitkey );
    const BufferString codelvl1( ck.key(0) );
    for ( int idx=0; idx<refs_.size(); idx++ )
    {
	const Strat::UnitRef& un = ref( idx );
	if ( codelvl1 != un.code() )
	    continue;

	unitkey += codelvl1.size();
	if ( ! *unitkey )
	    return const_cast<Strat::UnitRef*>(&un);
	else if ( !un.isLeaf() )
	{
	    if ( *unitkey == '.' && *(unitkey+1) )
		return ((Strat::NodeUnitRef&)un).fnd( unitkey+1 );
	}
	break;
    }
    return nullptr;
}


bool Strat::NodeUnitRef::add( UnitRef* un, bool rev )
{
    if ( !un || hasLeaves() != un->isLeaf() )
	return false;

    if ( rev )
	refs_.insertAt( un, 0 );
    else
	refs_ += un;

    refTree().reportAdd( *un );
    return true;
}


bool Strat::NodeUnitRef::insert( UnitRef* un, int posidx )
{
    if ( !un || hasLeaves() != un->isLeaf() )
	return false;

    if ( refs_.validIdx( posidx ) )
	refs_.insertAt( un, posidx );
    else if ( posidx == refs_.size() )
	refs_ += un;
    else
	return false;

    refTree().reportAdd( *un );
    return true;
}


Strat::UnitRef* Strat::NodeUnitRef::replace( int unidx, Strat::UnitRef* un )
{
    if ( !un || hasLeaves() != un->isLeaf() )
	return nullptr;

    UnitRef* oldun = refs_.replace( unidx, un );
    refTree().reportAdd( *un );
    return oldun;
}


void Strat::NodeUnitRef::takeChildrenFrom( Strat::NodeUnitRef* un )
{
    if ( !un ) return;
    refs_ = un->refs_; un->refs_.erase();
    for ( int idx=0; idx<refs_.size(); idx++ )
	refs_[idx]->upnode_ = this;
}


int Strat::NodeUnitRef::nrLeaves() const
{
    UnitRefIter it( *this, UnitRefIter::Leaves );
    int nr = 0;
    while ( it.next() ) nr++;
    return nr;
}


bool Strat::NodeUnitRef::isParentOf( const UnitRef& ur ) const
{
    const UnitRef* upnd = ur.upNode();
    if ( !upnd ) return false;
    return upnd == this ? true : isParentOf( *upnd );
}


//class NodeOnlyUnitRef


Strat::NodeOnlyUnitRef::NodeOnlyUnitRef( NodeUnitRef* up, const char* c,
					 const char* d )
    : NodeUnitRef(up,c,d)
{}


Strat::NodeOnlyUnitRef::~NodeOnlyUnitRef()
{}


const Strat::LeafUnitRef* Strat::NodeOnlyUnitRef::firstLeaf() const
{
    for ( int idx=0; idx<refs_.size(); idx++ )
    {
	const LeafUnitRef* ur = refs_[idx]->firstLeaf();
	if ( ur )
	    return ur;
    }
    return nullptr;
}


void Strat::NodeOnlyUnitRef::ensureTimeSorted()
{
    TypeSet<float> toptimes;
    TypeSet<int> idxs;
    float prevtoptime = -mUdf(float);
    bool needsorting = false;
    for ( int idx=0; idx<nrRefs(); idx++ )
    {
	mDynamicCastGet(const NodeUnitRef*,nur,refs_[idx])
	if ( !nur )
	    return;

	const float toptime = nur->timeRange().start_;
	if ( mIsUdf(toptime) )
	    return;

	if ( toptime < prevtoptime )
	    needsorting = true;

	toptimes += toptime;
	idxs += idx;
	prevtoptime = toptime;
    }

    if ( !needsorting )
	return;

    sort_coupled( toptimes.arr(), idxs.arr(), idxs.size() );
    refs_.useIndexes( idxs.arr() );
}

//class LeavedUnitRef

Strat::LeavedUnitRef::LeavedUnitRef( NodeUnitRef* up, const char* c,
				   const char* d )
    : NodeUnitRef(up,c,d)
    , levelid_(-1)
{}


Strat::LeavedUnitRef::~LeavedUnitRef()
{}


Strat::LeafUnitRef* Strat::LeavedUnitRef::getLeaf( int idx )
{
    return static_cast<LeafUnitRef*>( refs_[idx] );
}


Strat::LeafUnitRef* Strat::LeavedUnitRef::getLeaf( const Strat::Lithology& lith)
{
    for ( int idx=0; idx<nrLeaves(); idx++ )
    {
	LeafUnitRef* ur = getLeaf( idx );
	if ( ur->lithology() == lith.id() )
	    return ur;
    }
    return nullptr;
}


void Strat::LeavedUnitRef::setLevelID( Strat::LevelID lid )
{
    if ( lid != levelid_ )
	{ levelid_ = lid; notifChange(); }
}


void Strat::LeavedUnitRef::use( const char* s )
{
    int lvlid = levelid_.asInt();
    doUse( s, &lvlid );
    levelid_.set( lvlid );
}


//class LeafUnitRef

Strat::LeafUnitRef::LeafUnitRef( Strat::NodeUnitRef* up,
				 const LithologyID& lithidx,
				 const char* desc )
    : UnitRef(up,desc)
    , lith_(lithidx)
{
}


Strat::LeafUnitRef::~LeafUnitRef()
{}


bool Strat::LeafUnitRef::isUndef() const
{
    return this == &refTree().undefLeaf();
}


void Strat::LeafUnitRef::getPropsFrom( const IOPar& iop )
{
    UnitRef::getPropsFrom( iop );
    pars_.removeWithKey( sKey::Time() );
}


OD::Color Strat::LeafUnitRef::dispColor( bool lith ) const
{
    if ( isUndef() )
	return color();

    return lith ? getLithology().color() : upNode()->color();
}


const Strat::Lithology& Strat::LeafUnitRef::getLithology() const
{
    const Lithology* lith = refTree().lithologies().get( lith_ );
    return lith ? *lith : Lithology::undef();
}


const OD::String& Strat::LeafUnitRef::code() const
{
    return getLithology().name();
}


void Strat::LeafUnitRef::setLithology( const LithologyID& lid )
{
    if ( lid != lith_ )
	{ lith_ = lid; notifChange(); }
}


void Strat::LeafUnitRef::use( const char* s )
{
    int lithid = lith_.asInt();
    doUse( s, &lithid );
    lith_.set( lithid );
}
