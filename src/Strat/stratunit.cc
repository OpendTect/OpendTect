/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "stratunitref.h"
#include "stratreftree.h"
#include "stratlith.h"
#include "stratunitrefiter.h"
#include "property.h"
#include "propertyref.h"
#include "separstr.h"
#include "errh.h"
#include "iopar.h"
#include "keystrs.h"
#include "randcolor.h"

DefineEnumNames(Strat::UnitRef,Type,0,"Unit Type")
{ "Node", "Leaved", "Leaf", 0 };


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
	    *id = toInt( fms[nr] );
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
	return 0;

    return skip ? upnode_->upNode( skip-1 ) : upnode_;
}


CompoundKey Strat::UnitRef::fullCode() const
{
    CompoundKey kc;

    for ( int idx=treeDepth()-1; idx>=0; idx-- )
	kc += upNode( idx )->code();
    kc += code();

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
    (isrem ? toBeDeleted : changed).trigger();
    RefTree& rt = refTree();
    if ( &rt != this )
	rt.reportChange( this, isrem );
}


//class NodeUnitRef


Strat::NodeUnitRef::NodeUnitRef( NodeUnitRef* up, const char* uc, const char* d )
    : UnitRef(up,d)
    , code_(uc)
    , timerg_(mUdf(float),0)
{
}


Strat::NodeUnitRef::~NodeUnitRef()
{
    for( int idx=0; idx<nrRefs(); idx++ )
	ref( idx ).toBeDeleted.disable();

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


void Strat::NodeUnitRef::swapChildren( int idx1, int idx2 )
{
    if ( idx1 == idx2 ) return;
    refs_.swap( idx1, idx2 ); notifChange();
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

    refTree().reportAdd( un );
    return true;
}


bool Strat::NodeUnitRef::insert( UnitRef* un, int posidx )
{
    if ( !un || hasLeaves() != un->isLeaf() )
	return false;

    if ( refs_.validIdx( posidx ) )
	refs_.insertAt( un, posidx );

    refTree().reportAdd( un );
    return true;
}


Strat::UnitRef* Strat::NodeUnitRef::replace( int unidx, Strat::UnitRef* un )
{
    if ( !un || hasLeaves() != un->isLeaf() )
	return false;

    UnitRef* oldun = refs_.replace( unidx, un );
    refTree().reportAdd( un );
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


void Strat::LeafUnitRef::getPropsFrom( const IOPar& iop )
{
    UnitRef::getPropsFrom( iop );
    pars_.removeWithKey( sKey::Time() );
}


Color Strat::LeafUnitRef::dispColor( bool lith ) const
{
    return lith ? getLithology().color() : upNode()->color();
}


const Strat::Lithology& Strat::LeafUnitRef::getLithology() const
{
    const Lithology* lith = refTree().lithologies().get( lith_ );
    return lith ? *lith : Lithology::undef();
}


const BufferString& Strat::LeafUnitRef::code() const
{
    return getLithology().name();
}


void Strat::LeafUnitRef::setLithology( int lid )
{
    if ( lid != lith_ )
	{ lith_ = lid; notifChange(); }
}
