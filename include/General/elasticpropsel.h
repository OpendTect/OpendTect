#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
________________________________________________________________________

-*/

#include "elasticprop.h"
#include "propertyref.h"
#include "uistrings.h"

namespace Math { class Expression; }

class IOObj;

/*!
\brief User parameters to compute values for an elastic layer (den,p/s-waves).
*/

mExpClass(General) ElasticPropSelection : public PropertyRefSelection
{ mODTextTranslationClass(ElasticPropSelection)
public:

				ElasticPropSelection(bool withswave=true);
				ElasticPropSelection(
						const ElasticPropSelection&);
				~ElasticPropSelection();

    ElasticPropSelection&	operator =(
					const ElasticPropSelection&);

    bool			isElasticSel() const override	{ return true; }

    ElasticPropertyRef*		getByType(ElasticFormula::Type);
    const ElasticPropertyRef*	getByType(ElasticFormula::Type) const;

    static ElasticPropSelection* getByDBKey(const MultiID&);
    static ElasticPropSelection* getByIOObj(const IOObj*);
    bool			put(const IOObj*) const;

    bool			isValidInput(uiString* errmsg = nullptr) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			erase() override	{ deepErase(*this); }

    uiString			errMsg() { return errmsg_; }

    static const Mnemonic*	getByType(ElasticFormula::Type,const char* nm);

private:

    ElasticPropSelection&	doAdd(const PropertyRef*) override;

    static bool			checkForValidSelPropsDesc(
					const ElasticFormula&,
					const Mnemonic&,
					BufferStringSet& faultynms,
					BufferStringSet& corrnms);

    uiString			errmsg_;
};


/*!
\brief Computes elastic properties using parameters in ElasticPropSelection and
PropertyRefSelection.
*/

mExpClass(General) ElasticPropGen
{
public:
			ElasticPropGen(const ElasticPropSelection&,
				       const PropertyRefSelection&);
			~ElasticPropGen();

    bool		isOK() const;

    void		getVals(float& den,float& pvel,float& svel,
				const float* proprefvals,int proprefsz) const;

private:

    const ElasticFormula*	denform_;
    const ElasticFormula*	pvelform_;
    const ElasticFormula*	svelform_;
    ObjectSet<TypeSet<int> >	propidxsset_;
    ObjectSet<ObjectSet<const UnitOfMeasure> > propuomsset_;
    ObjectSet<Math::Expression> exprs_;

    const ElasticFormula* init(const ElasticFormula&,
			       const PropertyRefSelection&);
    static float	getValue(const ElasticFormula&,const TypeSet<int>&,
				 const ObjectSet<const UnitOfMeasure>&,
				 const Math::Expression*,
				 const float* proprefvals,int proprefsz);
};


/*!
\brief Guesses elastic properties using parameters in ElasticPropSelection and
PropertyRefSelection.
*/

mExpClass(General) ElasticPropGuess
{
public:
			ElasticPropGuess(const PropertyRefSelection&,
						ElasticPropSelection&);
protected:

    void		guessQuantity(const PropertyRefSelection&,
					ElasticFormula::Type);
    bool		guessQuantity(const PropertyRef&,ElasticFormula::Type);

    ElasticPropSelection& elasticprops_;
};


