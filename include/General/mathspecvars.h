#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2014
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
				 bool hasuns=false,
				 Mnemonic::StdType typ=Mnemonic::Other )
			    : varnm_(varnm), dispnm_(dispnm)
			    , hasunits_(hasuns), type_(typ)	{}
    bool		operator ==( const SpecVar& oth ) const
					{ return varnm_ == oth.varnm_; }

    BufferString	varnm_;
    BufferString	dispnm_;
    bool		hasunits_;
    Mnemonic::StdType	type_;

};


/*!\brief Set of special variables for Math Formulae/Expressions */

mExpClass(General) SpecVarSet : public TypeSet<SpecVar>
{
public:
			SpecVarSet()		{}

    int			getIndexOf(const char* varnm) const;
    void		getNames(BufferStringSet&,bool usrdisp=true) const;
    void		add( const char* varnm, const char* dispnm,
			   bool hasuns=false, Mnemonic::StdType typ
						=Mnemonic::Other )
			{ *this += SpecVar(varnm,dispnm,hasuns,typ); }

			// convenience

    SpecVar*		find( const char* nm )
			{ int idx=getIndexOf(nm); return idx<0?0:&(*this)[idx];}
    const SpecVar*	find( const char* nm ) const
			{ return const_cast<SpecVarSet*>(this)->find( nm ); }
    bool		isPresent( const char* nm ) const
			{ return getIndexOf(nm) >= 0; }
    const OD::String&	varName( int idx ) const
			{ return (*this)[idx].varnm_; }
    const OD::String&	dispName( int idx ) const
			{ return (*this)[idx].dispnm_; }
    bool		hasUnits( int idx )
			{ return (*this)[idx].hasunits_; }
    Mnemonic::StdType	propType( int idx )
			{ return (*this)[idx].type_; }

    static const SpecVarSet& getEmpty();

};



} // namespace Math


