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
				ElasticPropSelection(bool withswave,
					const PropertyRefSelection&);
				/*<! Ensure the selection does not contain
				     math-derived properties */
				ElasticPropSelection(bool withswave=true);
				/*<! Not directly usable until either
				     usePar or setFor is called */
				ElasticPropSelection(
						const ElasticPropSelection&);
				~ElasticPropSelection();

    ElasticPropSelection&	operator =(const ElasticPropSelection&);

    bool			isElasticSel() const override	{ return true; }

    ElasticPropertyRef*		getByType(ElasticFormula::Type);
    const ElasticPropertyRef*	getByType(ElasticFormula::Type) const;

    static ElasticPropSelection* getByDBKey(const MultiID&);
    static ElasticPropSelection* getByIOObj(const IOObj*);
    bool			put(const IOObj*) const;

    bool			ensureHasType(ElasticFormula::Type);
    bool			isValidInput(uiString* errmsg = nullptr) const;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;
    bool			setFor(const PropertyRefSelection&);
				/*<! Ensure the selection does not contain
				     math-derived properties */

    void			erase() override	{ deepErase(*this); }

    bool			isOK() const;
    uiString			errMsg() { return errmsg_; }

    static const Mnemonic*	getByType(ElasticFormula::Type,const char* nm);

    static const char*		sKeyElasticProp();

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

    mClass(General) CalcData
    {
    public:
			CalcData( const ElasticPropertyRef& epr )
			    : epr_(epr)			{}

	const ElasticPropertyRef& epr_;
	const UnitOfMeasure* pruom_ = nullptr;
	TypeSet<int>	propidxs_;
	mutable TypeSet<double> formvals_;
    };

    ObjectSet<CalcData> propcalcs_;

    void		init(const PropertyRefSelection&,
			     const ElasticPropertyRef*);
    static float	getValue(const CalcData&,const float* proprefvals,
				 int proprefsz);
};


/*!
\brief Guesses elastic properties using parameters in ElasticPropSelection and
PropertyRefSelection.
*/

mClass(General) ElasticPropGuess
{
public:
			ElasticPropGuess(const PropertyRefSelection&,
					 ElasticPropSelection&);

    bool		isOK() const	{ return isok_; }

private:

    bool		guessQuantity(const PropertyRefSelection&,
				      ElasticPropertyRef&);
    static MnemonicSelection* selection(const PropertyRef*,
					const PropertyRef* pr2=nullptr,
					const PropertyRef* pr3=nullptr);

    bool		isok_ = true;
};


