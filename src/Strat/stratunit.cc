/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: stratunit.cc,v 1.3 2004-11-29 17:04:26 bert Exp $";

#include "stratunitref.h"
#include "stratlith.h"


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

