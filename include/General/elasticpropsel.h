#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
________________________________________________________________________

-*/

#include "generalmod.h"
#include "elasticprop.h"
#include "uistrings.h"
#include "mnemonics.h"

class IOObj;

/*!
\brief User parameters to compute values for an elastic layer (den,p/s-waves).
*/

mExpClass(General) ElasticPropSelection : public PropertySelection
{ mODTextTranslationClass(ElasticPropSelection)
public:

				ElasticPropSelection(bool withswave=true);
				ElasticPropSelection(
					const ElasticPropSelection& elp)
				{ *this = elp; }
				~ElasticPropSelection();
    ElasticPropSelection&	operator =(const ElasticPropSelection&);

    ElasticProperty&		getByIdx( int idx )	{ return gt(idx); }
    const ElasticProperty&	getByIdx( int idx ) const { return gt(idx); }
    ElasticProperty&		getByType( ElasticFormula::Type tp )
							{ return gt(tp); }
    const ElasticProperty&	getByType( ElasticFormula::Type tp ) const
							{ return gt(tp); }

    static ElasticPropSelection* getByDBKey(const MultiID&);
    static ElasticPropSelection* getByIOObj(const IOObj*);
    bool			put(const IOObj*) const;

    bool			isValidInput(uiString*
				errmsg = 0) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    uiString			errMsg() { return errmsg_; }

protected:

    ElasticProperty&		gt(ElasticFormula::Type) const;
    ElasticProperty&		gt(int idx) const;
    bool			checkForValidSelPropsDesc(
					const ElasticFormula&,
					BufferStringSet& faultynms,
					BufferStringSet& corrnms);
    void			mkEmpty();

    uiString			errmsg_;
};


/*!
\brief Computes elastic properties using parameters in ElasticPropSelection and
MnemonicSelection.
*/

mExpClass(General) ElasticPropGen
{
public:
			ElasticPropGen(const ElasticPropSelection& eps,
					const MnemonicSelection& mns)
			    : elasticprops_(eps), mns_(mns) {}

    float		getVal(const ElasticProperty& ef,
				const float* proprefvals,
				int proprefsz) const
			{ return getVal(ef.formula(),proprefvals, proprefsz); }

    void		getVals(float& den,float& pbel,float& svel,
				const float* proprefvals,int proprefsz) const;

protected:

    const ElasticPropSelection& elasticprops_;
    const MnemonicSelection&	mns_;

    float		getVal(const ElasticFormula& ef,
				const float* proprefvals,
				int proprefsz) const;
};


/*!
\brief Guesses elastic properties using parameters in ElasticPropSelection and
MnemonicSelection.
*/

mExpClass(General) ElasticPropGuess
{
public:
			ElasticPropGuess(const MnemonicSelection&,
						ElasticPropSelection&);
protected:

    void		guessQuantity(const MnemonicSelection&,
					ElasticFormula::Type);
    bool		guessQuantity(const Mnemonic&,ElasticFormula::Type);

    ElasticPropSelection& elasticprops_;
};


