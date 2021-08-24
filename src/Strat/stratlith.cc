/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/


#include "stratlith.h"

#include "bufstringset.h"
#include "randcolor.h"
#include "separstr.h"
#include "stratcontent.h"


bool Strat::Content::getApearanceFrom( const char* str )
{
    FileMultiString fms( str );
    if ( fms.size() < 3 )
	return false;

    pattern_.type_ = fms.getIValue( 0 );
    pattern_.opt_ = fms.getIValue( 1 );
    color_.setStdStr( fms[2] );
    return true;
}


void Strat::Content::putAppearanceTo( BufferString& str ) const
{
    FileMultiString fms;
    fms += pattern_.type_;
    fms += pattern_.opt_;
    fms += color_.getStdStr();
    str = fms;
}


const Strat::Content& Strat::Content::unspecified()
{
    mDefineStaticLocalObject( Strat::Content, unspec, ("") );
    return unspec;
}


int Strat::ContentSet::getIndexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->name() == nm )
	    return idx;
    return -1;
}


const Strat::Lithology& Strat::Lithology::undef()
{
    mDefineStaticLocalObject( PtrMan<Strat::Lithology>, udf, = nullptr );
    if ( !udf )
    {
	Strat::Lithology* newudf = new Strat::Lithology( -1, "-", true );
	newudf->color() = Color::LightGrey();

	udf.setIfNull(newudf,true);
    }

    return *udf;
}


Strat::Lithology::Lithology( Strat::Lithology::ID li, const char* nm, bool por )
    : NamedObject(nm)
    , id_(li)
    , porous_(por)
{
    if ( id_ >= 0 )
	color_ = Color::stdDrawColor( id_ );
}


Strat::Lithology::Lithology( const char* fstr )
    : id_(-2)
{
    FileMultiString fms( fstr );
    const int sz = fms.size();
    setName( fms[0] );
    const_cast<ID&>(id_) = fms.getIValue( 1 );
    porous_ = *fms[2] == 'P';
    if ( sz > 3 )
	color_.setStdStr( fms[3] );
    else
	color_ = getRandStdDrawColor();
}


void Strat::Lithology::fill( BufferString& str ) const
{
    FileMultiString fms;
    fms += name();
    fms += id();
    fms += porous_ ? "P" : "N";
    fms += color_.getStdStr();
    str = fms.buf();
}



Strat::Lithology& Strat::Lithology::operator =( const Strat::Lithology& oth )
{
    if ( this != &oth )
    {
	setName( oth.name() );
	porous_ = oth.porous_;
	color_ = oth.color_;
	const_cast<ID&>(id_) = oth.id_;
    }
    return *this;
}


const char* Strat::LithologySet::add( Lithology* lith )
{
    if ( !lith )
	return "No object passed (null ptr)";

    if ( isPresent(lith->name()) )
    {
	delete lith;
	return "Lithology name already present";
    }

    if ( lith->id() == -2 )
	cCast(Lithology::ID&,lith->id_) = getFreeID();

    lths_ += lith;
    return nullptr;
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


Strat::Lithology::ID Strat::LithologySet::getFreeID() const
{
    Lithology::ID id = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	const Lithology& lith = *lths_[idx];
	id = mMAX( id, lith.id() );
    }

    return ++id;
}
