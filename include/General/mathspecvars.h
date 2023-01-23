#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "mnemonics.h"


namespace Math
{


/*!\brief Special variables for Math Formulae/Expressions */

mExpClass(General) SpecVar
{
public:

			SpecVar( const char* varnm, const char* dispnm,
				 bool hasuns=false, const Mnemonic* mn=nullptr )
			    : varnm_(varnm), dispnm_(dispnm)
			    , hasunits_(hasuns), mn_(mn)	{}
    mDeprecatedDef	SpecVar( const char* varnm, const char* dispnm,
				 bool hasuns,
				 Mnemonic::StdType typ )
			    : SpecVar(varnm,dispnm,hasuns,
				    MnemonicSelection::getGuessed(nullptr,typ))
			{}

    bool		operator ==( const SpecVar& oth ) const
					{ return varnm_ == oth.varnm_; }

    const Mnemonic&	getMnemonic() const
			{ return mn_ ? *mn_ : Mnemonic::undef(); }

    BufferString	varnm_;
    BufferString	dispnm_;
    bool		hasunits_;

private:

    const Mnemonic*	mn_;

};


/*!\brief Set of special variables for Math Formulae/Expressions */

mExpClass(General) SpecVarSet : public TypeSet<SpecVar>
{
public:
			SpecVarSet()		{}

    int			getIndexOf(const char* varnm) const;
    void		getNames(BufferStringSet&,bool usrdisp=true) const;
    void		add( const char* varnm, const char* dispnm,
			     bool hasuns=false, const Mnemonic* mn=nullptr )
			{ *this += SpecVar(varnm,dispnm,hasuns,mn); }

			// convenience

    SpecVar*		find( const char* nm )
			{ int idx=getIndexOf(nm);
			  return idx<0 ? nullptr: &(*this)[idx]; }
    const SpecVar*	find( const char* nm ) const
			{ return const_cast<SpecVarSet*>(this)->find( nm ); }
    bool		isPresent( const char* nm ) const
			{ return getIndexOf(nm) >= 0; }
    const OD::String&	varName( int idx ) const
			{ return (*this)[idx].varnm_; }
    const OD::String&	dispName( int idx ) const
			{ return (*this)[idx].dispnm_; }
    bool		hasUnits( int idx ) const
			{ return (*this)[idx].hasunits_; }
    const Mnemonic&	mnemonic( int idx ) const
			{ return (*this)[idx].getMnemonic(); }
    Mnemonic::StdType	propType( int idx ) const
			{ return mnemonic( idx ).stdType(); }

    static const SpecVarSet& getEmpty();

public:

    mDeprecatedDef
    void		add( const char* varnm, const char* dispnm,
			   bool hasuns, Mnemonic::StdType typ )
			{
			    const Mnemonic* mn =
				MnemonicSelection::getGuessed( nullptr, typ );
			    *this += SpecVar( varnm, dispnm, hasuns, mn );
			}

};



} // namespace Math
