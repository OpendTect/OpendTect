/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratsynthlevel.h"

#include "perthreadrepos.h"

namespace StratSynth
{

const Level& Level::undef()
{
    static Level ret( Strat::Level::undef().id() );
    return ret;
}

Level& Level::dummy()
{
    static Level ret( Strat::Level::dummy().id() );
    return ret;
}

} // namespace StratSynth



const OD::String& StratSynth::Level::name() const
{
    mDeclStaticString( ret );
    ret = Strat::levelNameOf( id_ );

    return ret;
}


OD::Color StratSynth::Level::color() const
{
    return Strat::LVLS().colorOf( id_ );
}


StratSynth::LevelSet& StratSynth::LevelSet::operator =( const LevelSet& oth )
{
    if ( this != &oth )
	deepCopy( lvls_, oth.lvls_ );

    return *this;
}


StratSynth::Level& StratSynth::LevelSet::add( ID id )
{
    const int idx = indexOf( id );
    if ( idx >= 0 )
	return *lvls_.get( idx );

    auto* newlvl = new Level( id );
    lvls_.add( newlvl );
    return *newlvl;
}


int StratSynth::LevelSet::indexOf( ID id ) const
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
	if ( lvls_[idx]->id() == id )
	    return idx;
    return -1;
}


int StratSynth::LevelSet::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
	if ( lvls_[idx]->name() == nm )
	    return idx;
    return -1;
}


#define mDefGetFn(fnnm,args,varnm,cnst,udfdum) \
cnst StratSynth::Level& StratSynth::LevelSet::fnnm( args ) cnst \
{ \
    const int idxof = indexOf( varnm ); \
    return idxof < 0 ? Level::udfdum() : *lvls_[idxof]; \
}

mDefGetFn( get, ID id, id, , dummy )
mDefGetFn( get, ID id, id, const, undef )
mDefGetFn( getByName, const char* nm, nm, , dummy )
mDefGetFn( getByName, const char* nm, nm, const, undef )
