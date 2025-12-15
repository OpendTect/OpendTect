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

			SpecVar(const char* varnm,const char* dispnm,
				bool hasuns =false,const Mnemonic* =nullptr);
    mDeprecatedDef	SpecVar(const char* varnm,const char* dispnm,
				bool hasuns,Mnemonic::StdType);
			~SpecVar();

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

mExpClass(General) SpecVarSet : public ManagedObjectSet<SpecVar>
{
public:
			SpecVarSet();
			~SpecVarSet();

    int			getIndexOf(const char* varnm) const;
    void		getNames(BufferStringSet&,bool usrdisp=true) const;
    void		add(const char* varnm,const char* dispnm,
			    bool hasuns =false,const Mnemonic* =nullptr);

			// convenience

    SpecVar*		find(const char* nm);
    const SpecVar*	find(const char* nm) const;
    bool		isPresent( const char* nm ) const
			{ return getIndexOf(nm) >= 0; }
    const OD::String&	varName( int idx ) const
			{ return get(idx)->varnm_; }
    const OD::String&	dispName( int idx ) const
			{ return get(idx)->dispnm_; }
    bool		hasUnits( int idx ) const
			{ return get(idx)->hasunits_; }
    const Mnemonic&	mnemonic( int idx ) const
			{ return get(idx)->getMnemonic(); }
    Mnemonic::StdType	propType( int idx ) const
			{ return mnemonic( idx ).stdType(); }

    static const SpecVarSet& getEmpty();

public:

    mDeprecatedDef
    void		add(const char* varnm,const char* dispnm,
			    bool hasuns,Mnemonic::StdType);

};

} // namespace Math
