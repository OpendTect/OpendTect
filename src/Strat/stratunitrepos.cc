/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.6 2005-01-20 17:17:30 bert Exp $";

#include "stratunitrepos.h"
#include "stratlith.h"
#include "strmprov.h"
#include "ascstream.h"
#include "separstr.h"
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
	    msg += unrepo->nrTrees();
	    if ( unrepo->nrTrees() > 0 )
	    {
		msg += "; first tree name: ";
		msg += unrepo->tree(0)->treeName();
	    }
	    DBG::message( msg );
	}
    }
    return *unrepo;
}


bool Strat::RefTree::addUnit( const char* code, const char* dumpstr )
{
    if ( !code || !*code )
	use( dumpstr );

    CompoundKey ck( code );
    UnitRef* par = find( ck.upLevel().buf() );
    if ( !par )
	return false;

    const bool isleaf = strchr( dumpstr, '`' ); // a bit of a hack, really
    const BufferString ky( ck.key( ck.nrKeys()-1 ) );
    UnitRef* newun = isleaf ? (UnitRef*)new LeafUnitRef( par, ky )
			    : (UnitRef*)new NodeUnitRef( par, ky );
    if ( !newun->use(dumpstr) )
	{ delete newun; return false; }
    return true;
}


void Strat::RefTree::removeEmptyNodes()
{
    //TODO implement
    pErrMsg("Not impl: Strat::RefTree::removeEmptyNodes");
}


const Strat::RefTree::Level* Strat::RefTree::getLevel( const char* nm ) const
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	if ( lvls_[idx]->name_ == nm )
	    return lvls_[idx];
    }
    return 0;
}


const Strat::RefTree::Level* Strat::RefTree::getLevel( const Strat::UnitRef* u,
							bool top ) const
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	if ( lvls_[idx]->unit_ == u && lvls_[idx]->top_ == top )
	    return lvls_[idx];
    }
    return 0;
}


void Strat::RefTree::remove( const Strat::RefTree::Level*& lvl )
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	Strat::RefTree::Level* curlvl = lvls_[idx];
	if ( curlvl == lvl )
	{
	    lvls_ -= curlvl;
	    delete curlvl;
	    lvl = 0;
	    return;
	}
    }
}


Strat::UnitRepository::UnitRepository()
    	: curtreeidx_(-1)
{
    reRead();
}


Strat::UnitRepository::~UnitRepository()
{
    deepErase( trees_ );
    deepErase( liths_ );
    deepErase( unusedliths_ );
}


void Strat::UnitRepository::reRead()
{
    deepErase( trees_ );
    deepErase( liths_ );

    Repos::FileProvider rfp( filenamebase );
    addTreeFromFile( rfp );
    while ( rfp.next() )
	addTreeFromFile( rfp );

    if ( curtreeidx_ < 0 || curtreeidx_ > trees_.size() )
	curtreeidx_ = -1;
}


bool Strat::UnitRepository::write( Repos::Source )
{
    //TODO implement
    pErrMsg("Not impl: Strat::UnitRepository::write");
    return true;
}


void Strat::UnitRepository::addTreeFromFile( const Repos::FileProvider& rfp )
{
    BufferString fnm = rfp.fileName();
    if ( !File_exists(fnm) ) return;
    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() ) return;

    const Repos::Source src = rfp.source();
    ascistream astrm( *sd.istrm, true );
    Strat::RefTree* tree = 0;
    while ( !atEndOfSection( astrm.next() ) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    tree = new Strat::RefTree( astrm.value(), src );
	else if ( astrm.hasKeyword(sKeyLith) )
	    addLith( astrm.value(), src );
    }
    if ( !tree )
    {
	BufferString msg( "No name specified for Straigraphic tree in:\n" );
	msg += fnm; ErrMsg( fnm );
	sd.close(); delete tree; return;
    }

    while ( !atEndOfSection( astrm.next() ) )
    {
	if ( !tree->addUnit(astrm.keyWord(),astrm.value()) )
	{
	    BufferString msg( fnm );
	    msg += ": Invalid unit: '";
	    msg += astrm.keyWord(); msg += "'";
	    ErrMsg( msg );
	}
    }
    tree->removeEmptyNodes();

    while ( !atEndOfSection( astrm.next() ) )
    {
	FileMultiString fms( astrm.value() );
	const UnitRef* ur = tree->find( fms[0] );
	if ( !ur || !*astrm.keyWord() )
	    continue;

	const bool isbot = *fms[1] == 'B'; 
	tree->addLevel( new Strat::RefTree::Level(astrm.keyWord(),ur,!isbot) );
    }

    sd.close();
    if ( tree->nrRefs() > 0 )
	trees_ += tree;
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
    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	if ( trees_[idx]->treeName() == tnm )
	    return idx;
    }
    return -1;
}


Strat::UnitRef* Strat::UnitRepository::fnd( const char* code ) const
{
    return curtreeidx_ < 0 ? 0
	 : const_cast<Strat::UnitRef*>( trees_[curtreeidx_]->find( code ) );
}


Strat::UnitRef* Strat::UnitRepository::fndAny( const char* code ) const
{
    const Strat::UnitRef* ref = find( code );
    if ( !ref )
    {
	for ( int idx=0; idx<trees_.size(); idx++ )
	{
	    if ( idx == curtreeidx_ ) continue;
	    const Strat::UnitRef* r = trees_[idx]->find( code );
	    if ( r )
		{ ref = r; break; }
	}
    }

    return const_cast<Strat::UnitRef*>( ref );
}


Strat::UnitRef* Strat::UnitRepository::fnd( const char* code, int idx ) const
{
    if ( idx < 0 )			return fnd(code);
    else if ( idx >= trees_.size() )	return 0;

    return const_cast<Strat::UnitRef*>( trees_[idx]->find( code ) );
}


int Strat::UnitRepository::treeOf( const char* code ) const
{
    if ( fnd(code,curtreeidx_) )
	return curtreeidx_;

    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	if ( idx == curtreeidx_ ) continue;
	else if ( fnd(code,idx) )
	    return idx;
    }
    return -1;
}
