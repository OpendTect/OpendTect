/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Prajjaval Singh
 Date:          January 2017
________________________________________________________________________

-*/

#include "stratsynthlevel.h"

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



BufferString StratSynth::Level::name() const
{
    return Strat::LVLS().levelName( id_ );
}


Color StratSynth::Level::color() const
{
    return Strat::LVLS().levelColor( id_ );
}


StratSynth::LevelSet& StratSynth::LevelSet::operator =( const LevelSet& oth )
{
    if ( this != &oth )
    {
	deepCopy( lvls_, oth.lvls_ );
    }
    return *this;
}


StratSynth::Level& StratSynth::LevelSet::add( ID id )
{
    const auto idx = indexOf( id );
    if ( idx >= 0 )
	return *lvls_.get( idx );

    Level* newlvl = new Level( id );
    lvls_ += newlvl;
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
