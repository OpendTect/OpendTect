/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.3 2004-11-29 17:17:52 bert Exp $";

#include "stratunitrepos.h"
#include "stratlith.h"
#include "debug.h"


static const char* filenamebase = "StratUnits";


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
	    msg += unrepo->nrTops();
	    if ( unrepo->nrTops() > 0 )
	    {
		msg += "; first tree name: ";
		msg += unrepo->tree(0)->treeName();
	    }
	    DBG::message( msg );
	}
    }
    return *unrepo;
}


Strat::UnitRepository::UnitRepository()
{
    Repos::FileProvider rfp( filenamebase );
    addTreeFromFile( rfp );
    while ( rfp.next() )
	addTreeFromFile( rfp );
}


void Strat::UnitRepository::addTreeFromFile( const Repos::FileProvider& rfp )
{
    //TODO a bit of work here would be nice :-)
}


int Strat::UnitRepository::indexOf( const char* tnm ) const
{
    for ( int idx=0; idx<tops_.size(); idx++ )
    {
	if ( tops_[idx]->treeName() == tnm )
	    return idx;
    }
    return -1;
}


Strat::UnitRef* Strat::UnitRepository::fnd( const char* code ) const
{
    return curtopidx_ < 0 ? 0
	 : const_cast<Strat::UnitRef*>( tops_[curtopidx_]->find( code ) );
}


Strat::UnitRef* Strat::UnitRepository::fndAny( const char* code ) const
{
    const Strat::UnitRef* ref = find( code );
    if ( !ref )
    {
	for ( int idx=0; idx<tops_.size(); idx++ )
	{
	    if ( idx == curtopidx_ ) continue;
	    const Strat::UnitRef* r = tops_[idx]->find( code );
	    if ( r )
		{ ref = r; break; }
	}
    }

    return const_cast<Strat::UnitRef*>( ref );
}


Strat::UnitRef* Strat::UnitRepository::fnd( const char* code, int idx ) const
{
    if ( idx < 0 )			return fnd(code);
    else if ( idx >= tops_.size() )	return 0;

    return const_cast<Strat::UnitRef*>( tops_[idx]->find( code ) );
}


int Strat::UnitRepository::treeOf( const char* code ) const
{
    if ( fnd(code,curtopidx_) )
	return curtopidx_;

    for ( int idx=0; idx<tops_.size(); idx++ )
    {
	if ( idx == curtopidx_ ) continue;
	else if ( fnd(code,idx) )
	    return idx;
    }
    return -1;
}
