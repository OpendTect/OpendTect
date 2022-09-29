/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratlith.h"

#include "bufstringset.h"
#include "randcolor.h"
#include "separstr.h"
#include "stratcontent.h"


Strat::Content::Content( const char* nm )
    : NamedObject(nm)
{}


Strat::Content::Content( const Strat::Content& c )
    : NamedObject(c)
    , pattern_(c.pattern_)
    , color_(c.color_)
{}


Strat::Content::~Content()
{}


Strat::Content& Strat::Content::operator =( const Strat::Content& c )
{
    setName(c.name());
    pattern_=c.pattern_;
    color_ = c.color_;
    return *this;
}


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


Strat::ContentSet::ContentSet()
{}


Strat::ContentSet::~ContentSet()
{
    deepErase(*this);
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
	auto* newudf = new Strat::Lithology( LithologyID::udf() , "-", true );
	newudf->color() = OD::Color::LightGrey();

	udf.setIfNull(newudf,true);
    }

    return *udf;
}


Strat::LithologyID::LithologyID()
{}


Strat::LithologyID::~LithologyID()
{}


Strat::Lithology::Lithology( const Strat::LithologyID& li, const char* nm,
			     bool por )
    : NamedObject(nm)
    , id_(li)
    , porous_(por)
{
    if ( id_.isValid() )
	color_ = OD::Color::stdDrawColor( id_.asInt() );
}


Strat::Lithology::Lithology( const char* fstr )
    : id_(LithologyID::unsetID())
{
    FileMultiString fms( fstr );
    const int sz = fms.size();
    setName( fms[0] );
    const_cast<LithologyID&>( id_ ).set( fms.getIValue( 1 ) );
    porous_ = *fms[2] == 'P';
    if ( sz > 3 )
	color_.setStdStr( fms[3] );
    else
	color_ = OD::getRandStdDrawColor();
}


Strat::Lithology::Lithology( const Lithology& l )
    : id_(l.id_)
{
    *this = l;
}


Strat::Lithology::~Lithology()
{}


void Strat::Lithology::fill( BufferString& str ) const
{
    FileMultiString fms;
    fms += name();
    fms += id().asInt();
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
	const_cast<LithologyID&>(id_) = oth.id_;
    }
    return *this;
}


Strat::LithologySet::LithologySet()
    : anyChange(this)
{}


Strat::LithologySet::~LithologySet()
{}


int Strat::LithologySet::indexOf( const char* nm ) const
{
    return idxOf( nm, LithologyID::unsetID() );
}


bool Strat::LithologySet::isPresent( const char* nm ) const
{
    return gtLith( nm, LithologyID::unsetID() );
}


int Strat::LithologySet::indexOf( const LithologyID& id ) const
{
    return idxOf( nullptr, id );
}


bool Strat::LithologySet::isPresent( const LithologyID& id ) const
{
    return gtLith( nullptr, id );
}


Strat::Lithology* Strat::LithologySet::get( const char* nm )
{
    return gtLith( nm, LithologyID::unsetID() );
}


const Strat::Lithology* Strat::LithologySet::get( const char* nm ) const
{
    return gtLith( nm, LithologyID::unsetID() );
}


Strat::Lithology* Strat::LithologySet::get( const LithologyID& id )
{
    return gtLith( nullptr, id );
}


const Strat::Lithology* Strat::LithologySet::get( const LithologyID& id ) const
{
    return gtLith( nullptr, id );
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

    if ( !lith->id().isValid() && !lith->id().isUdf() )
	cCast(LithologyID&,lith->id_) = getFreeID();

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


int Strat::LithologySet::idxOf( const char* nm, const LithologyID& id ) const
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


Strat::Lithology* Strat::LithologySet::gtLith( const char* nm,
					       const LithologyID& id ) const
{
    const int idx = idxOf( nm, id );
    return lths_.validIdx(idx) ? const_cast<Lithology*>(lths_[idx]) : nullptr;
}


Strat::LithologyID Strat::LithologySet::getFreeID() const
{
    int id = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	const Lithology& lith = *lths_[idx];
	id = mMAX( id, lith.id().asInt() );
    }

    return LithologyID( ++id );
}
