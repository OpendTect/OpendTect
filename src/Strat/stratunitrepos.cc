/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.4 2004-12-01 16:42:48 bert Exp $";

#include "stratunitrepos.h"
#include "stratlith.h"
#include "strmprov.h"
#include "ascstream.h"
#include "filegen.h"
#include "keystrs.h"
#include "errh.h"
#include "debug.h"


static const char* filenamebase = "StratUnits";
static const char* filetype = "Stratigraphic Tree";
const char* Strat::UnitRepository::sKeyLith = "Lithology";


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


Strat::UnitRepository::~UnitRepository()
{
    deepErase( tops_ );
    deepErase( liths_ );
    deepErase( unusedliths_ );
}


void Strat::UnitRepository::addTreeFromFile( const Repos::FileProvider& rfp )
{
    BufferString fnm = rfp.fileName();
    if ( !File_exists(fnm) ) return;
    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() ) return;

    const Repos::Source src = rfp.source();
    ascistream astrm( *sd.istrm, true );
    TopUnitRef* tree = 0;
    while ( !atEndOfSection( astrm.next() ) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    tree = new Strat::TopUnitRef( astrm.value() );
	else if ( astrm.hasKeyword(sKeyLith) )
	    addLith( astrm.value(), src );
    }
    if ( !tree )
    {
	BufferString msg( "No name specified for Straigraphic tree in:\n" );
	msg += fnm; ErrMsg( fnm );
	sd.close(); delete tree; return;
    }

    NodeUnitRef* lastnode = tree;
    while ( !atEndOfSection( astrm.next() ) )
    {
	//TODO get the units
    }
    //TODO get rid of empty nodes

    sd.close();
    if ( tree->nrRefs() > 0 )
	tops_ += tree;
    else
    {
	BufferString msg( "No valid layers found in:\n" );
	msg += fnm; ErrMsg( fnm );
	delete tree;
    }
}


int Strat::UnitRepository::findLith( const char* str ) const
{
    for ( int idx=0; idx<liths_.size(); idx++ )
    {
	if ( liths_[idx]->name() == str )
	    return idx;
    }
    return -1;
}


void Strat::UnitRepository::addLith( const char* str, Repos::Source src )
{
    if ( !str || !*str ) return;

    Strat::Lithology* newlith = new Strat::Lithology;
    if ( !newlith->use(str) )
	{ delete newlith; return; }
    newlith->setSource( src );
    if ( findLith(str) >= 0 )
	unusedliths_ += newlith;
    else
	liths_ += newlith;
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
