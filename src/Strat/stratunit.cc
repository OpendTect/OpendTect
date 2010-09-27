/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: stratunit.cc,v 1.28 2010-09-27 09:59:47 cvsbert Exp $";

#include "stratunitref.h"
#include "stratlith.h"
#include "property.h"
#include "propertyref.h"
#include "separstr.h"
#include "errh.h"
#include "iopar.h"
#include "keystrs.h"
#include "randcolor.h"


const char* Strat::UnitRef::sKeyLevel() { return "Level"; }


const Strat::Lithology& Strat::Lithology::undef()
{
    static Strat::Lithology* udf = 0;
    if ( !udf )
	udf = new Strat::Lithology( "Undefined" );
    return *udf;
}


const Strat::LeafUnitRef& Strat::LeafUnitRef::undef()
{
    static Strat::LeafUnitRef* udf = 0;
    if ( !udf )
	udf = new Strat::LeafUnitRef( 0, "undef", Strat::Lithology::undef().id_,
				      "Undefined" );
    return *udf;
}


const Strat::NodeUnitRef& Strat::NodeUnitRef::undef()
{
    static Strat::NodeUnitRef* udf = 0;
    if ( !udf )
	udf = new Strat::NodeUnitRef( 0, "undef", "Undefined" );
    return *udf;
}


Strat::Lithology& Strat::Lithology::operator =( const Strat::Lithology& l )
{
    if ( this != &l )
    {
	setName( l.name() );
	id_ = l.id_;
	porous_ = l.porous_;
	src_ = l.src_;
    }
    return *this;
}


void Strat::Lithology::fill( BufferString& str ) const
{
    FileMultiString fms;
    fms += name();
    fms += id_;
    fms += porous_ ? "P" : "N";
    str = fms;
}


bool Strat::Lithology::use( const char* str )
{
    FileMultiString fms( str );
    const int sz = fms.size();
    if ( sz < 2 ) return false;

    setName( fms[0] );
    id_ = atoi( fms[1] );
    porous_ = sz > 2 ? *fms[2] == 'P' : false;

    return true;
}


Strat::UnitRef::ID Strat::UnitRef::getNewID() const
{
    static Strat::UnitRef::ID curid = 1;
    return curid++;
}


Strat::UnitRef::~UnitRef()
{
    deepErase( properties_ );
}


void Strat::UnitRef::fill( BufferString& str ) const
{
    str = desc_;
}


void Strat::UnitRef::putTo( IOPar& iop ) const
{
    iop.set( sKey::Time, timerg_ );
    iop.set( sKey::Color, color_ );
    iop.set( sKeyLevel(), lvlid_ );
}


void Strat::UnitRef::getFrom( const IOPar& iop )
{
    iop.get( sKey::Time, timerg_ );
    iop.get( sKey::Color, color_ );
    iop.get( sKeyLevel(), lvlid_ );
}


void Strat::UnitRef::copyParFrom( const Strat::UnitRef& unit )
{
    IOPar iop; 
    unit.putTo( iop ); 
    getFrom( iop ); 
}


bool Strat::UnitRef::use( const char* str )
{
    desc_ = str;
    return true;
}


void Strat::LeafUnitRef::fill( BufferString& str ) const
{
    FileMultiString fms;
    fms += lith_; fms += desc_;
    str = fms;
}


bool Strat::LeafUnitRef::use( const char* str )
{
    FileMultiString fms( str );
    const int sz = fms.size();
    if ( sz < 2 ) return false;

    lith_ = atoi( fms[0] );
    desc_ = fms[1];
    return true;
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
    kc += code_;

    return kc;
}


bool Strat::UnitRef::isBelow( const Strat::UnitRef* un ) const
{
    if ( !un || !upnode_ || un->isLeaf() )
	return false;
    return upnode_ == un || upnode_->isBelow( un );
}


bool Strat::UnitRef::precedes( const UnitRef& ur ) const
{
    Iter it( *topNode() );
    do
    {
	if ( it.unit() == this ) return true;
	if ( it.unit() == &ur ) return false;
    }
    while ( it.next() );

    pErrMsg( "Unreachable code reached. Iterator bug." );
    return false;
}


Property* Strat::UnitRef::gtProp( const PropertyRef* pr ) const
{
    if ( !pr ) return 0;
    for ( int idx=0; idx<properties_.size(); idx++ )
    {
	if ( properties_[idx]->ref() == *pr )
	    return const_cast<Property*>( properties_[idx] );
    }
    return 0;
}


Strat::NodeUnitRef::~NodeUnitRef()
{
    deepErase( refs_ );
}


Strat::UnitRef* Strat::NodeUnitRef::fnd( const char* unitkey ) const
{
    if ( !unitkey || !*unitkey )
	return code().isEmpty() ? (Strat::UnitRef*)this : 0;

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


void Strat::NodeUnitRef::add( UnitRef* ur, bool rev )
{
    if ( rev )
	refs_.insertAt( ur, 0 );
    else
	refs_ += ur;
}


Strat::UnitRef::Iter::Iter( const NodeUnitRef& ur, Pol p )
	: itnode_(const_cast<NodeUnitRef*>(&ur))
    	, pol_(p)
{
    reset();
}


void Strat::UnitRef::Iter::reset()
{
    curidx_ = -1;
    curnode_ = itnode_;
    next();
}


Strat::UnitRef* Strat::UnitRef::Iter::gtUnit() const
{
    const Strat::UnitRef* ret = curnode_;
    if ( curnode_ && curidx_ >= 0 )
	ret = &curnode_->ref( curidx_ );

    return const_cast<Strat::UnitRef*>( ret );
}


bool Strat::UnitRef::Iter::next()
{
    while ( toNext() )
    {
	if ( pol_ == All )
	    return true;
	const UnitRef* curun = unit();
	if ( (pol_ == Nodes && !curun->isLeaf())
	  || (pol_ == Leaves && curun->isLeaf()) )
	    return true;
    }

    return false;
}


bool Strat::UnitRef::Iter::toNext()
{
    if ( !curnode_ ) return false; // At end

    // First see if we can simply take next ref or go down
    UnitRef* curun = gtUnit();
    if ( curun->isLeaf() )
    {
	if ( curidx_ < curnode_->nrRefs() - 1 )
	    { curidx_++; return true; }
    }
    else
    {
	if ( curun != curnode_ )
	{
	    curnode_ = (NodeUnitRef*)curun;
	    if ( curnode_->nrRefs() > 0 )
	    {
		curidx_ = 0;
		return true;
	    }
	}
	else if ( curnode_->nrRefs() > 0 )
	    { curidx_ = 0; return true; }
    }

    // OK so this node (and everything below) is done.
    while ( true )
    {
	Strat::NodeUnitRef* par = curnode_->upNode();
	if ( !par ) break;

	curidx_ = par->indexOf( curnode_ );
	curnode_ = par;
	if ( curidx_ < par->nrRefs() - 1 )
	    { curidx_++; return true; }

	if ( curnode_ == itnode_ ) break;
    }

    curnode_ = 0;
    return false;
}
