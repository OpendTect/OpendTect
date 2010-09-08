/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratlevel.cc,v 1.1 2010-09-08 15:13:54 cvsbert Exp $";

#include "bufstringset.h"
#include "iopar.h"
#include "color.h"
#include "separstr.h"
#include "keystrs.h"
#include "stratlevel.h"


Strat::Level::Level( const char* nm )
    : NamedObject(nm)
    , id_(-1)
    , pars_(*new IOPar)
{
}


Strat::Level::Level( const Level& lvl )
	: NamedObject(lvl) 
	, id_(lvl.id_)
	, color_(lvl.color_)
	, pars_(*new IOPar(lvl.pars_))
{
}


Strat::Level::~Level()
{
    delete &pars_;
}


Strat::Level& Strat::Level::operator =( const Level& lvl )
{
    if ( this != &lvl )
    {
	setName( lvl.name() );
	id_ = lvl.id_;
	color_ = lvl.color_;
	pars_ = lvl.pars_;
    }
    return *this;
}


void Strat::Level::putTo( IOPar& iop ) const
{
    iop.set( sKey::ID, id_ );
    iop.set( sKey::Name, name() );
    iop.set( sKey::Color, color_ );
    iop.merge( pars_ );
}


void Strat::Level::getFrom( const IOPar& iop )
{
    iop.get( sKey::ID, id_ );
    BufferString nm; iop.get( sKey::Name, nm ); setName( nm );
    iop.get( sKey::Color, color_ );

    pars_.merge( iop );
    pars_.removeWithKey( sKey::Name );
    pars_.removeWithKey( sKey::Color );
    pars_.removeWithKey( sKey::ID );
}


const Strat::Level* Strat::LevelSet::getByName( const char* lvlname ) const
{
    const int idx = indexOf( lvlname );
    return  idx < 0 ? 0 : (*this)[idx];
}


int Strat::LevelSet::indexOf( const char* lvlname ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !strcmp(lvlname,(*this)[idx]->name()) )
	return idx;
    }
    return -1;
}


const Strat::Level* Strat::LevelSet::getByID( int id ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx]->id_ == id )
	return (*this)[idx];
    }
    return 0;
}


bool Strat::LevelSet::isPresent( const char* lvlnm ) const 
{
    bool alreadyinlist = false;
    for ( int idlvl=0; idlvl<size(); idlvl ++ )
    {
	if ( !strcmp( (*this)[idlvl]->name(), lvlnm ) )
	    return true;
    }
    return false;
}


void Strat::LevelSet::insert( const char* lvlnm, const Color& col, int idx ) 
{
    if ( !isPresent( lvlnm ) )
    {
	Level* lvl = new Level( lvlnm );
	lvl->color_ = col;
	lvl->id_ = getNewID();
	insertAt( lvl, idx );
    }
}


int Strat::LevelSet::addToList( const char* lvlnm, const Color& col ) 
{
    return add( lvlnm, col );
}


int Strat::LevelSet::add( const char* lvlnm, const Color& col ) 
{
    if ( isPresent( lvlnm ) )
	return getByName( lvlnm )->id_;
    else
    {
	Level* lvl = new Level( lvlnm );
	lvl->id_ = getNewID();
	lvl->color_ = col;
	(*this) += lvl;
	return lvl->id_; 
    }
    return -1; 
}


void Strat::LevelSet::add( const BufferStringSet& lvlnms, 
				const TypeSet<Color>& cols ) 
{
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	add( lvlnms.get(idx), cols[idx] );
}


void Strat::LevelSet::addToList( const BufferStringSet& lvlnms, 
				    const TypeSet<Color>& cols ) 
{
    if ( isEmpty() )
    {
	add( lvlnms, cols );
	return;
    }
    int idlist = 0;
    BufferStringSet tmplvlnms( lvlnms );
    TypeSet<Color> tmpcols( cols );
    while ( !tmplvlnms.isEmpty() )
    {
	while ( !tmplvlnms.isEmpty() && isPresent( tmplvlnms.get(0) ) )
	{
	    idlist = indexOf( tmplvlnms.get(0) );
	    tmplvlnms.remove(0);
	    tmpcols.remove(0);
	}
	while ( !tmplvlnms.isEmpty() && !isPresent( tmplvlnms.get(0) ) )
	{
	    insert( tmplvlnms.get(0), tmpcols[0], idlist );
	    idlist ++;
	    tmplvlnms.remove(0);
	    tmpcols.remove(0);
	}
    }
}


