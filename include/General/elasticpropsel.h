#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "elasticprop.h"

#include "ailayer.h"
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
				ElasticPropSelection(RefLayer::Type,
						const PropertyRefSelection&);
				/*<! Ensure the selection does not contain
				     math-derived properties */
				ElasticPropSelection(RefLayer::Type
							= RefLayer::Elastic);
				/*<! Not directly usable until either
				     usePar or setFor is called */
				ElasticPropSelection(
						const ElasticPropSelection&);
				~ElasticPropSelection();

    ElasticPropSelection&	operator =(const ElasticPropSelection&);
    bool			operator ==(const ElasticPropSelection&) const;
    bool			operator !=(const ElasticPropSelection&) const;

    bool			isElasticSel() const override	{ return true; }

    bool			isOK(
				  const TypeSet<ElasticFormula::Type>& reqtypes,
				  const PropertyRefSelection&,
				  uiString* msg =nullptr) const;
    bool			isOK(const PropertyRefSelection*) const;
				//<! Try to provide a selection when possible
    bool			isValidInput(uiString* errmsg =nullptr) const;
				//<! Checks for inter/self dependencies only
    uiString			errMsg() const		{ return errmsg_; }

    ElasticPropertyRef*		getByType(ElasticFormula::Type);
    const ElasticPropertyRef*	getByType(ElasticFormula::Type) const;
    MultiID			getStoredID() const	{ return storedid_; }

    static ElasticPropSelection* getByDBKey(const MultiID&,
					    const PropertyRefSelection*);
    static ElasticPropSelection* getByIOObj(const IOObj*,
					    const PropertyRefSelection*);
    bool			put(const IOObj*) const;

    bool			ensureHasType(ElasticFormula::Type);

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;
    bool			setFor(const PropertyRefSelection&);
				/*<! Ensure the selection does not contain
				     math-derived properties */

    void			erase() override	{ deepErase(*this); }

    static const Mnemonic*	getByType(ElasticFormula::Type,const char* nm);

    static const char*		sKeyElasticProp();
    static const char*		sKeyElasticPropSel();

private:

    ElasticPropSelection&	doAdd(const PropertyRef*) override;

    static bool			checkForValidSelPropsDesc(
					const ElasticFormula&,
					const Mnemonic&,
					BufferStringSet& faultynms,
					BufferStringSet& corrnms);

    MultiID			storedid_;
    uiString			errmsg_;

public:

    mDeprecated("Use RefLayer::Type") ElasticPropSelection(bool needswave);
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

    void		getVals(const float* proprefvals,int proprefsz,
				float* elvals,int elrefsz) const;

private:

    mClass(General) CalcData
    {
    public:
			CalcData(const ElasticPropertyRef&);
			~CalcData();

	const ElasticPropertyRef& epr_;
	const UnitOfMeasure* pruom_ = nullptr;
	TypeSet<int>	propidxs_;
	mutable TypeSet<double> formvals_;
    };

    ObjectSet<CalcData> propcalcs_;

    bool		init(const PropertyRefSelection&,
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
			~ElasticPropGuess();

    bool		isOK() const	{ return isok_; }

private:

    bool		guessQuantity(const PropertyRefSelection&,
				      ElasticPropertyRef&);
    static MnemonicSelection* selection(const PropertyRef*,
					const PropertyRef* pr2=nullptr,
					const PropertyRef* pr3=nullptr,
					const PropertyRef* pr4=nullptr,
					const PropertyRef* pr5=nullptr);

    bool		isok_ = true;
};
