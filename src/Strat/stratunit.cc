/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: stratunit.cc,v 1.1 2003-12-15 17:29:43 bert Exp $";

#include "stratunit.h"


Strat::Unit* Strat::Unit::upNode( int skip )
{
    if ( !upnode_ )
	return 0;

    return skip ? upnode_->upNode( skip-1 ) : upnode_;
}


CompoundKey Strat::Unit::fullCode() const
{
    CompoundKey kc;

    for ( int idx=level()-1; idx>=0; idx-- )
	kc += upNode( idx )->code_;
    kc += code_;

    return kc;
}


bool Strat::Unit::isBelow( const Strat::Unit* un ) const
{
    if ( !un || !upnode_ ) return false;
    return upnode_ == un || upnode_->isBelow( un );
}
