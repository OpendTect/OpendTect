/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: stratunit.cc,v 1.7 2005-01-20 17:17:30 bert Exp $";

#include "stratunitref.h"
#include "stratlith.h"
#include "separstr.h"


const Strat::Lithology& Strat::Lithology::undef()
{
    static Strat::Lithology* udf = 0;
    if ( !udf )
    {
	udf = new Strat::Lithology( "Undefined" );
	udf->setId( -1 );
    }
    return *udf;
}


const Strat::LeafUnitRef& Strat::LeafUnitRef::undef()
{
    static Strat::LeafUnitRef* udf = 0;
    if ( !udf )
	udf = new Strat::LeafUnitRef( 0, "undef",
				      Strat::Lithology::undef().id(),
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


void Strat::UnitRef::fill( BufferString& str ) const
{
    str = desc_;
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


Strat::UnitRef* Strat::UnitRef::upNode( int skip )
{
    if ( !upnode_ )
	return 0;

    return skip ? upnode_->upNode( skip-1 ) : upnode_;
}


CompoundKey Strat::UnitRef::fullCode() const
{
    CompoundKey kc;

    for ( int idx=level()-1; idx>=0; idx-- )
	kc += upNode( idx )->code_;
    kc += code_;

    return kc;
}


bool Strat::UnitRef::isBelow( const Strat::UnitRef* un ) const
{
    if ( !un || !upnode_ ) return false;
    return upnode_ == un || upnode_->isBelow( un );
}


Strat::UnitRef* Strat::NodeUnitRef::fnd( const char* code ) const
{
    if ( !code || !*code )
	return const_cast<Strat::NodeUnitRef*>(this);

    CompoundKey ck( code );
    const BufferString codelvl1( ck.key(0) );
    for ( int idx=0; idx<refs_.size(); idx++ )
    {
	const Strat::UnitRef& un = ref( idx );
	if ( codelvl1 == un.code() )
	{
	    code += codelvl1.size();
	    if ( un.isLeaf() )
		return *code ? 0 : const_cast<Strat::UnitRef*>(&un);
	    else if ( *code )
		return ((Strat::NodeUnitRef&)un).fnd( code+1 );
	    else
		return 0;
	}
    }
    return 0;
}
