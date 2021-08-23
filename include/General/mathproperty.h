#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jan 2004
________________________________________________________________________


-*/

#include "property.h"
#include "uistring.h"

class UnitOfMeasure;
namespace Math { class Formula; class SpecVarSet; }


/*!\brief Calculated property

  When creating a formula, be sure to use ensureGoodVariableName() on the
  property names. This will be done on the available properties too to
  create the match. In theory, this may create ambiguous formulas, but
  at least we can keep things simple this way.

 */

mExpClass(General) MathProperty : public Property
{ mODTextTranslationClass(Property)
public:
			MathProperty(const PropertyRef&,
				     const char* def=nullptr);
			MathProperty(const MathProperty&);
			~MathProperty();

    bool		isFormula() const		{ return true; }

    Math::Formula&	getForm()		{ return form_; }
    const Math::Formula& getForm() const	{ return form_; }

    Mnemonic::StdType	inputType(int) const;
    bool		haveInput( int idx ) const    { return inps_[idx]; }
    void		setInput(int,const Property*);
			//!< Must be done for all inputs after each setDef()

    virtual bool	init(const PropertySet&) const;
    virtual uiString	errMsg() const		{ return errmsg_; }
    virtual bool	dependsOn(const Property&) const;
    bool		hasCyclicalDependency(BufferStringSet& inputnms) const;

    mDefPropertyFns(MathProperty,"Math");

				// convenience, shielding from Math::Formula
    const char*			formText(bool user_display=false) const;
    int				nrInputs() const;
    const char*			inputName(int) const;
    const UnitOfMeasure*	inputUnit(int) const;
    bool			isConst(int) const;
    void			setUnit(const UnitOfMeasure*);
    const UnitOfMeasure*	unit() const;

    static const Math::SpecVarSet& getSpecVars();

protected:

    Math::Formula&		form_;
    mutable ObjectSet<const Property>	inps_;
    mutable uiString		errmsg_;
    mutable BufferString	fulldef_;

    void			setPreV5Def(const char*);

private:

    void			doUnitChange(const UnitOfMeasure* olduom,
				const UnitOfMeasure* newuom) override;

};


