/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.1 2004-11-26 14:10:40 bert Exp $";

#include "stratunitrepos.h"


Strat::UnitRepository& Strat::UnR()
{
    static Strat::UnitRepository* unrepo = 0;
    if ( !unrepo )
    {
	if ( DBG::isOn() ) DBG::message( "Creating Strat::UnitRepository" );
	unrepo = new Strat::UnitRepository;
	if ( DBG::isOn() )
	{
	    BufferString msg( "Total strat trees found: " );
	    msg += unrepo->tops().size();
	    if ( unrepo->tops().size() > 0 )
	    {
		msg += "; first tree name: ";
		msg += unrepo->tops()[0]->treeName();
	    }
	    DBG::message( msg );
	}
    }
}

Strat::UnitRepository::UnitRepository()
{
    Repos::FileProvider rfp( filenamebase );
    addTreeFromFile( rfp );
    while ( rfp.next() )
	addTreeFromFile( rfp );
}


Strat::UnitRef* Strat::UnitRepository::gtUn( const char* code, int idx ) const
{
    if ( idx < 0 || idx >= tops_.size() )
	return 0;

    Strat::TreeTrav it( *tops_[idx] );
    while ( it.next() )
	if ( it.unit().fullCode() == code )
	    return it.unit();
    return 0;
}


int Strat::UnitRepository::treeOf( const char* code )
{
    for ( int idx=0; idx<tops_.size(); idx++ )
    {
	if ( gtUn(code,idx) )
	    return idx;
    }
    return -1;
}
