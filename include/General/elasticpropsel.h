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

class IOObj;

/*!
\brief User parameters to compute values for an elastic layer (den,p/s-waves).
*/

mExpClass(General) ElasticPropSelection : public PropertyRefSelection
{ mODTextTranslationClass(ElasticPropSelection)
public:

				ElasticPropSelection(bool withswave=true);
				ElasticPropSelection(
					const ElasticPropSelection& elp)
				{ *this = elp; }
				~ElasticPropSelection();
    ElasticPropSelection&	operator =(const ElasticPropSelection&);

    ElasticPropertyRef&		getByIdx( int idx )	{ return gt(idx); }
    const ElasticPropertyRef&	getByIdx( int idx ) const { return gt(idx); }
    ElasticPropertyRef&		getByType( ElasticFormula::Type tp )
							{ return gt(tp); }
    const ElasticPropertyRef&	getByType( ElasticFormula::Type tp ) const
							{ return gt(tp); }

    static ElasticPropSelection* getByDBKey(const DBKey&);
    static ElasticPropSelection* getByIOObj(const IOObj*);
    bool			put(const IOObj*) const;

    bool			isValidInput(uiString*
				errmsg = 0) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    uiString			  errMsg() { return errmsg_; }

protected:

    ElasticPropertyRef&		gt(ElasticFormula::Type) const;
    ElasticPropertyRef&		gt(int idx) const;

    bool			checkForValidSelPropsDesc(
					    const ElasticFormula&,
					    BufferStringSet& faultynms,
					    BufferStringSet& corrnms);
    void			mkEmpty();

    uiString			errmsg_;

};


/*!
\brief Computes elastic properties using parameters in ElasticPropSelection and
PropertyRefSelection.
*/

mExpClass(General) ElasticPropGen
{
public:
			ElasticPropGen(const ElasticPropSelection& eps,
					const PropertyRefSelection& rps)
			    : elasticprops_(eps), refprops_(rps) {}

    float		getVal(const ElasticPropertyRef& ef,
				const float* proprefvals,
				int proprefsz) const
			{ return getVal(ef.formula(),proprefvals, proprefsz); }

    void		getVals(float& den,float& pbel,float& svel,
				const float* proprefvals,int proprefsz) const;

protected:

    const ElasticPropSelection& elasticprops_;
    const PropertyRefSelection& refprops_;

    float		getVal(const ElasticFormula& ef,
				const float* proprefvals,
				int proprefsz) const;
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
