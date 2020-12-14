/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2003
-*/


#include "stratunitref.h"
#include "stratreftree.h"
#include "stratlith.h"
#include "stratunitrefiter.h"
#include "property.h"
#include "propertyref.h"
#include "separstr.h"
#include "iopar.h"
#include "keystrs.h"
#include "randcolor.h"
#include "uistrings.h"

mDefineEnumUtils(Strat::UnitRef,Type,"Unit Type")
{ "Node", "Leaved", "Leaf", 0 };

template<>
void EnumDefImpl<Strat::UnitRef::Type>::init()
{
    uistrings_ += uiStrings::sNode();
    uistrings_ += mEnumTr("Leaved","Startigraphic Unit Type");
    uistrings_ += mEnumTr("Leaf","Startigraphic Unit Type");
}


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
    notifChange( true );
    detachAllNotifiers();
}


void Strat::UnitRef::prepareFWDelete()
{
    changed.setEmpty();
    toBeDeleted.setEmpty();
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


void Strat::UnitRef::doFill( BufferString& str, IntegerID<int> id ) const
{
    doFill( str, id.getI() );
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


void Strat::UnitRef::doUse( const char* str, IntegerID<int>& id )
{
    FileMultiString fms( str );
    const int sz = fms.size();
    int nr = 0;
    if ( sz > 1 )
    {
	const int i = fms.getIValue(nr);
	id.setI( i );
	if ( i < 0 )
	    id.setInvalid();
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
	kc += ref ? ref->code() : "";
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


void Strat::UnitRef::setColor( Color c )
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
    , timerg_(mUdf(float),0)
{
}


Strat::NodeUnitRef::~NodeUnitRef()
{
    for( int idx=0; idx<nrRefs(); idx++ )
	ref( idx ).toBeDeleted.disable();

    setEmpty();
}


void Strat::NodeUnitRef::prepareFWDelete()
{
    UnitRef::prepareFWDelete();
    for ( int idx=0; idx<refs_.size(); idx++ )
	refs_[idx]->prepareFWDelete();
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
    const bool oldudf = mIsUdf(timerg_.start);
    const bool newudf = mIsUdf(rg.start);
    bool ischgd = oldudf != newudf;
    if ( !ischgd && !oldudf )
	ischgd = timerg_.start != rg.start || timerg_.stop != rg.stop;
    if ( !ischgd ) return;

    timerg_ = rg;
    notifChange();
}


void Strat::NodeUnitRef::incTimeRange( const Interval<float>& rg )
{
    if ( mIsUdf(timerg_.start) )
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
    nurtimerg.start += dtime;
    nurtimerg.stop += dtime;
    setTimeRange( nurtimerg );
    while ( childitr.next() )
    {
	mDynamicCastGet(Strat::NodeUnitRef*,nur,childitr.unit())
	if ( nur )
	{
	    Interval<float> newtimerg = nur->timeRange();
	    newtimerg.start += dtime;
	    newtimerg.stop += dtime;
	    nur->setTimeRange( newtimerg );
	}
    }
}


void Strat::NodeUnitRef::moveChild( int idx, bool moveup )
{
    const int idx2 = moveup ? idx-1 : idx+1;
    if ( !refs_.validIdx(idx) || !refs_.validIdx(idx2) )
	return;

    const int topidx = idx < idx2 ? idx : idx2;
    const int bottomidx = idx < idx2 ? idx2 : idx;
    mDynamicCastGet(Strat::NodeUnitRef*,topnur,refs_[topidx]);
    mDynamicCastGet(Strat::NodeUnitRef*,bottomnur,refs_[bottomidx]);
    if ( !topnur || !bottomnur )
	return;

    const float topdtime = bottomnur->timeRange().width();
    const float bottomdtime = -topnur->timeRange().width();
    topnur->changeTimeRange( topdtime );
    bottomnur->changeTimeRange( bottomdtime );
    refs_.swap( idx, idx2 );
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
    return 0;
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

    refTree().reportAdd( *un );
    return true;
}


Strat::UnitRef* Strat::NodeUnitRef::replace( int unidx, Strat::UnitRef* un )
{
    if ( !un || hasLeaves() != un->isLeaf() )
	return 0;

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

const Strat::LeafUnitRef* Strat::NodeOnlyUnitRef::firstLeaf() const
{
    for ( int idx=0; idx<refs_.size(); idx++ )
    {
	const LeafUnitRef* ur = refs_[idx]->firstLeaf();
	if ( ur )
	    return ur;
    }
    return 0;
}


//class LeavedUnitRef

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
    return 0;
}


void Strat::LeavedUnitRef::setLevelID( Strat::Level::ID lid )
{
    if ( lid != levelid_ )
	{ levelid_ = lid; notifChange(); }
}


//class LeafUnitRef

Strat::LeafUnitRef::LeafUnitRef( Strat::NodeUnitRef* up, int lithidx,
				 const char* d )
    : UnitRef(up,d)
    , lith_(lithidx)
{
}


bool Strat::LeafUnitRef::isUndef() const
{
    return this == &refTree().undefLeaf();
}


void Strat::LeafUnitRef::getPropsFrom( const IOPar& iop )
{
    UnitRef::getPropsFrom( iop );
    pars_.removeWithKey( sKey::Time() );
}


Color Strat::LeafUnitRef::dispColor( bool lith ) const
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


void Strat::LeafUnitRef::setLithology( int lid )
{
    if ( lid != lith_ )
	{ lith_ = lid; notifChange(); }
}
