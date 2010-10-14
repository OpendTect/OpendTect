/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratlith.cc,v 1.2 2010-10-14 09:58:06 cvsbert Exp $";

#include "stratlith.h"
#include "separstr.h"
#include "bufstringset.h"

const Strat::Lithology& Strat::Lithology::undef()
{
    static Strat::Lithology udf( -1, "<Undef>" );
    return udf;
}


Strat::Lithology::Lithology( Strat::Lithology::ID id, const char* nm, bool por )
    : NamedObject(nm)
    , id_(id)
    , porous_(por)
{
}


Strat::Lithology::Lithology( const char* fstr )
    : id_(0)
{
    FileMultiString fms( fstr );
    setName( fms[0] );
    const_cast<ID&>(id_) = toInt(fms[1]);
    porous_ = *fms[2] == 'P';
}


void Strat::Lithology::fill( BufferString& str ) const
{
    FileMultiString fms;
    fms += name();
    fms += id();
    fms += porous_ ? "P" : "N";
    str = fms.buf();
}



Strat::Lithology& Strat::Lithology::operator =( const Strat::Lithology& oth )
{
    if ( this != &oth )
    {
	setName( oth.name() );
	porous_ = oth.porous_;
    }
    return *this;
}


const char* Strat::LithologySet::add( Lithology* lith )
{
    if ( !lith )
	return "No object passed (null ptr)";
    if ( isPresent(lith->name()) )
	{ delete lith; return "Lithology name already present"; }

    const_cast<Lithology::ID&>(lith->id_) = size() + 1;
    lths_ += lith;
    return 0;
}


void Strat::LithologySet::getNames( BufferStringSet& nms,
				    Strat::LithologySet::PorSel ps ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Lithology& lith = *lths_[idx];
	if ( ps == AllPor || (lith.porous_ == (ps == OnlyPorous)) )
	    nms.add( lith.name() );
    }
}


int Strat::LithologySet::idxOf( const char* nm, Lithology::ID id ) const
{
    const bool havenm = nm && *nm;
    for ( int idx=0; idx<size(); idx++ )
    {
	const Lithology& lith = *lths_[idx];
	if ( (havenm && lith.name() == nm) || (!havenm && lith.id() == id) )
	    return idx;
    }
    return -1;
}
